using System.Buffers.Binary;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;

namespace Saex.Tools;

public sealed record StartupTls(bool Present, uint[] CallbackRvas, uint ZeroFillBytes);
public sealed record StartupModule(string Name, long Bytes, string Sha256, bool IsDll, uint EntryRva,
    string[] Imports, string[] DelayImports, bool HasBoundImports, StartupTls Tls, string[] LayoutNotes, EngineInspection? Engine);

// Diagnostic metadata only. No assembly/native load, process creation or runtime permission.
public static class PeStartupInspector
{
    public const int MaxModuleBytes = 64 * 1024 * 1024;
    public const int MaxImports = 128;
    public const int MaxCallbacks = 32;

    internal sealed class Budget(long bytes)
    {
        public long Remaining { get; private set; } = bytes;
        public void Charge(long size)
        {
            if (size > Remaining) throw new InvalidDataException("startup_total_byte_limit");
            Remaining -= size; // Charged even if metadata parsing later fails.
        }
    }

    public static StartupModule Inspect(string path, bool expectDll) => Read(path, expectDll, new(MaxModuleBytes));

    internal static StartupModule Read(string path, bool expectDll, Budget budget)
    {
        NativeStartupInspector.RejectReparsePath(path);
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        if (stream.Length is < 64 or > MaxModuleBytes) throw new InvalidDataException("startup_module_size");
        budget.Charge(stream.Length);
        using var pe = new PEReader(stream, PEStreamOptions.LeaveOpen);
        var headers = pe.PEHeaders;
        var h = headers.PEHeader ?? throw new InvalidDataException("startup_missing_pe_header");
        var dll = (headers.CoffHeader.Characteristics & Characteristics.Dll) != 0;
        if (headers.CoffHeader.Machine != Machine.I386 || h.Magic != PEMagic.PE32 ||
            pe.HasMetadata || h.CorHeaderTableDirectory.RelativeVirtualAddress != 0 || h.CorHeaderTableDirectory.Size != 0 ||
            (headers.CoffHeader.Characteristics & Characteristics.ExecutableImage) == 0 || dll != expectDll)
            throw new InvalidDataException("startup_native_x86_kind");
        var reader = new Reader(stream, headers);
        var notes = reader.ValidateLayout();
        if (h.AddressOfEntryPoint != 0 || !dll) reader.Executable(checked((uint)h.AddressOfEntryPoint));
        var imports = reader.Imports(h.ImportTableDirectory, false);
        var delay = reader.Imports(h.DelayImportTableDirectory, true);
        var tls = reader.Tls(h.ThreadLocalStorageTableDirectory);
        var bound = reader.Directory(h.BoundImportTableDirectory, 8);
        stream.Position = 0;
        var hash = Convert.ToHexStringLower(SHA256.HashData(stream));
        EngineInspection? engine = null;
        if (!dll)
        {
            stream.Position = 0;
            var observed = EngineObservation.Match(stream, headers, hash, true);
            engine = new(Path.GetFileName(path), stream.Length, hash, "I386", true, false, false,
                observed.Reason, observed.Id, observed.Matched, observed.SourceDigest);
        }
        return new(Path.GetFileName(path), stream.Length, hash, dll, checked((uint)h.AddressOfEntryPoint),
            imports, delay, bound, tls, notes, engine);
    }

    internal sealed class Reader(Stream stream, PEHeaders headers)
    {
        private PEHeader Header => headers.PEHeader!;
        private static void Require(bool value, string error)
        {
            if (!value) throw new InvalidDataException(error);
        }
        public string[] ValidateLayout()
        {
            var h = Header;
            var notes = new List<string>();
            static bool Power(int n) => n > 0 && (n & (n - 1)) == 0;
            Require(headers.SectionHeaders.Length is >= 1 and <= 96 && h.NumberOfRvaAndSizes is >= 0 and <= 16 &&
                headers.CoffHeader.SizeOfOptionalHeader >= 96 + h.NumberOfRvaAndSizes * 8,
                "startup_header_layout");
            Require(Power(h.FileAlignment) && h.FileAlignment is >= 512 and <= 65536 &&
                Power(h.SectionAlignment) && h.SectionAlignment >= h.FileAlignment &&
                h.SizeOfImage is > 0 and <= 256 * 1024 * 1024 && h.SizeOfImage % h.SectionAlignment == 0 &&
                h.SizeOfHeaders > 0 && h.SizeOfHeaders <= stream.Length && h.SizeOfHeaders <= h.SizeOfImage &&
                h.SizeOfHeaders % h.FileAlignment == 0 &&
                (long)headers.PEHeaderStartOffset + headers.CoffHeader.SizeOfOptionalHeader + 40L * headers.SectionHeaders.Length <= h.SizeOfHeaders &&
                h.ImageBase > 0 && h.ImageBase % 65536 == 0 && h.ImageBase + (ulong)h.SizeOfImage <= 0x100000000UL,
                "startup_image_layout");
            for (var i = 0; i < headers.SectionHeaders.Length; i++)
            {
                var s = headers.SectionHeaders[i];
                var extent = Math.Max(s.VirtualSize, s.SizeOfRawData);
                Require(s.VirtualSize >= 0 && s.SizeOfRawData >= 0 && extent > 0 &&
                    s.VirtualAddress >= h.SizeOfHeaders && s.VirtualAddress % h.SectionAlignment == 0 &&
                    (long)s.VirtualAddress + extent <= h.SizeOfImage && s.PointerToRawData >= 0 &&
                    (long)s.PointerToRawData + s.SizeOfRawData <= stream.Length &&
                    (s.SizeOfRawData == 0 || (s.PointerToRawData >= h.SizeOfHeaders &&
                        s.PointerToRawData % h.FileAlignment == 0)), "startup_section_bounds");
                // Some packed DLLs omit final raw padding. Observe bounded bytes and report
                // the deviation; this metadata reader never grants Windows loader eligibility.
                if (s.SizeOfRawData % h.FileAlignment != 0) notes.Add("section_raw_size_unaligned:" + i);
                for (var j = 0; j < i; j++)
                {
                    var t = headers.SectionHeaders[j];
                    static bool Overlap(int a, int an, int b, int bn) => an > 0 && bn > 0 &&
                        (long)a < (long)b + bn && (long)b < (long)a + an;
                    Require(!Overlap(s.VirtualAddress, extent, t.VirtualAddress, Math.Max(t.VirtualSize, t.SizeOfRawData)) &&
                        !Overlap(s.PointerToRawData, s.SizeOfRawData, t.PointerToRawData, t.SizeOfRawData), "startup_section_overlap");
                }
            }
            return notes.ToArray();
        }
        internal long Offset(uint rva, long size)
        {
            Require(size > 0 && (ulong)rva + (ulong)size <= (ulong)Header.SizeOfImage, "startup_rva_bounds");
            long? match = null;
            foreach (var s in headers.SectionHeaders)
            {
                var delta = (long)rva - s.VirtualAddress;
                if (delta < 0 || delta + size > s.SizeOfRawData) continue;
                Require(match is null, "startup_ambiguous_rva");
                match = s.PointerToRawData + delta;
            }
            return match ?? throw new InvalidDataException("startup_rva_not_file_backed");
        }
        internal uint Number(uint rva)
        {
            stream.Position = Offset(rva, 4);
            Span<byte> bytes = stackalloc byte[4]; stream.ReadExactly(bytes);
            return BinaryPrimitives.ReadUInt32LittleEndian(bytes);
        }
        private uint FromVa(uint va)
        {
            Require(va >= Header.ImageBase && (ulong)va - Header.ImageBase < (ulong)Header.SizeOfImage, "startup_va_bounds");
            return checked((uint)((ulong)va - Header.ImageBase));
        }
        public void Executable(uint rva)
        {
            Require(rva != 0, "startup_entry_zero");
            _ = Offset(rva, 1);
            Require(headers.SectionHeaders.Any(s => rva >= s.VirtualAddress &&
                (long)rva - s.VirtualAddress < s.SizeOfRawData && (s.SectionCharacteristics & SectionCharacteristics.MemExecute) != 0),
                "startup_target_not_executable");
        }
        public bool Directory(DirectoryEntry directory, int minimum)
        {
            if (directory.RelativeVirtualAddress == 0 && directory.Size == 0) return false;
            Require(directory.RelativeVirtualAddress > 0 && directory.Size >= minimum && directory.Size <= 1024 * 1024,
                "startup_directory_bounds");
            _ = Offset(checked((uint)directory.RelativeVirtualAddress), directory.Size);
            return true;
        }
        internal string ModuleName(uint rva)
        {
            Span<byte> bytes = stackalloc byte[128];
            for (var i = 0; i < bytes.Length; i++)
            {
                stream.Position = Offset(checked(rva + (uint)i), 1);
                var b = stream.ReadByte();
                if (b == 0)
                {
                    var name = Encoding.ASCII.GetString(bytes[..i]).ToLowerInvariant();
                    Require(name.Length > 0 && name != "." && !name.Contains("..", StringComparison.Ordinal) &&
                        !name.EndsWith('.') && name.All(c => char.IsAsciiLetterOrDigit(c) || c is '.' or '_' or '-'),
                        "startup_import_name");
                    return name;
                }
                Require(b is > 0 and < 128, "startup_import_name");
                bytes[i] = (byte)b;
            }
            throw new InvalidDataException("startup_import_name_limit");
        }
        public string[] Imports(DirectoryEntry directory, bool delay)
        {
            var stride = delay ? 32 : 20;
            if (!Directory(directory, stride)) return [];
            var names = new HashSet<string>(StringComparer.Ordinal);
            var start = checked((uint)directory.RelativeVirtualAddress);
            Span<uint> fields = stackalloc uint[8];
            for (var i = 0; i <= MaxImports && (long)(i + 1) * stride <= directory.Size; i++)
            {
                var at = checked(start + (uint)(i * stride));
                var allZero = true;
                for (var j = 0; j < stride / 4; j++) { fields[j] = Number(at + (uint)(4 * j)); allZero &= fields[j] == 0; }
                if (allZero) return names.Order(StringComparer.Ordinal).ToArray();
                Require(i < MaxImports, "startup_import_limit");
                uint nameRva;
                if (delay)
                {
                    Require(fields[0] is 0 or 1, "startup_delay_attributes");
                    nameRva = fields[0] == 1 ? fields[1] : FromVa(fields[1]);
                }
                else nameRva = fields[3];
                Require(names.Add(ModuleName(nameRva)), "startup_duplicate_import");
            }
            throw new InvalidDataException("startup_import_termination");
        }
        public StartupTls Tls(DirectoryEntry directory)
        {
            if (!Directory(directory, 24)) return new(false, [], 0);
            var at = checked((uint)directory.RelativeVirtualAddress);
            var start = Number(at); var end = Number(at + 4); var index = Number(at + 8);
            var array = Number(at + 12); var zeroFill = Number(at + 16);
            Require((start == 0) == (end == 0) && end >= start && (long)end - start + zeroFill <= MaxModuleBytes,
                "startup_tls_data_bounds");
            if (start != 0)
            {
                var rva = FromVa(start);
                if (end != start) _ = Offset(rva, (long)end - start);
            }
            if (index != 0)
            {
                var rva = FromVa(index);
                // The loader-written index can reside in a zero-filled image section.
                Require(headers.SectionHeaders.Any(s => rva >= s.VirtualAddress &&
                    (long)rva - s.VirtualAddress + 4 <= Math.Max(s.VirtualSize, s.SizeOfRawData)), "startup_tls_index_bounds");
            }
            var callbacks = new List<uint>();
            if (array != 0)
            {
                var rva = FromVa(array);
                for (var i = 0; i <= MaxCallbacks; i++)
                {
                    var va = Number(checked(rva + (uint)(4 * i)));
                    if (va == 0) return new(true, callbacks.ToArray(), zeroFill);
                    Require(i < MaxCallbacks, "startup_tls_callback_limit");
                    var target = FromVa(va); Executable(target); callbacks.Add(target);
                }
            }
            return new(true, callbacks.ToArray(), zeroFill);
        }
    }
}
