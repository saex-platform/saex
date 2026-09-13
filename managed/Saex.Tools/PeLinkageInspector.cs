using System.Buffers.Binary;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;

namespace Saex.Tools;

public sealed record NativeImportSymbol(string Module, string Kind, string? Name, uint? Ordinal,
    ushort? Hint, uint LookupRva, uint IatRva);
public sealed record NativeExportSymbol(uint Ordinal, string[] Names, uint TargetRva, string Kind, string? Forwarder);
public sealed record NativeLinkageModule(string Name, long Bytes, string Sha256, bool IsDll,
    NativeImportSymbol[] Imports, NativeExportSymbol[] Exports, string[] LayoutNotes);
public sealed record NativeExportTarget(uint Ordinal, uint TargetRva, string Kind, string? Forwarder);
public sealed record NativeSymbolBinding(NativeImportSymbol Import, NativeExportTarget? Export, string Status);
public sealed record NativeLinkageReport(NativeLinkageModule Consumer, string RequestedModule,
    NativeLinkageModule Candidate, NativeSymbolBinding[] Bindings, string[] Issues)
{
    public int SchemaVersion => 1;
    public string Scope => "static-native-linkage";
    public bool StaticSymbolsComplete => Bindings.Length > 0 && Issues.Length == 0;
    public bool CanAttach => false;
    public bool CanInitialize => false;
    public bool RuntimeResolutionVerified => false;
    public bool CallingConventionVerified => false;
    public bool DynamicLoadsEnumerated => false;
}

// Reads two explicitly selected files. Never loads modules, follows forwarders or changes a loader policy.
public static class PeLinkageInspector
{
    public const int MaxSymbols = 4096;
    public const int MaxSymbolBytes = 512; // Includes NUL; an independent bound for every string.

    public static NativeLinkageReport Compare(string consumerPath, string requestedModule, string candidatePath)
    {
        Require(requestedModule.Length is > 0 and < 128 && !requestedModule.Contains("..", StringComparison.Ordinal) &&
            !requestedModule.EndsWith('.') && requestedModule.All(c => char.IsAsciiLetterOrDigit(c) || c is '.' or '_' or '-'),
            "linkage_requested_module");
        var module = requestedModule.ToLowerInvariant();
        var consumer = Inspect(consumerPath);
        var candidate = Read(candidatePath, true);
        var byOrdinal = candidate.Exports.ToDictionary(e => e.Ordinal);
        var byName = candidate.Exports.SelectMany(e => e.Names.Select(n => (Name: n, Export: e)))
            .ToDictionary(e => e.Name, e => e.Export, StringComparer.Ordinal);
        var bindings = new List<NativeSymbolBinding>();
        var issues = new List<string>();
        foreach (var import in consumer.Imports.Where(i => i.Module == module))
        {
            NativeExportSymbol? export;
            if (import.Name is { } name) byName.TryGetValue(name, out export);
            else byOrdinal.TryGetValue(import.Ordinal!.Value, out export);
            var status = export is null ? "missing_export" : export.Kind == "forwarder" ? "forwarder_unresolved" : "direct_metadata_match";
            // Do not repeat all aliases for every import: an alias-rich DLL must not amplify JSON quadratically.
            var target = export is null ? null : new NativeExportTarget(export.Ordinal, export.TargetRva, export.Kind, export.Forwarder);
            bindings.Add(new(import, target, status));
            if (status != "direct_metadata_match") issues.Add(status + ":" + (import.Name ?? "#" + import.Ordinal));
        }
        if (bindings.Count == 0) issues.Add("requested_module_not_imported");
        return new(consumer, module, candidate, bindings.ToArray(), issues.ToArray());
    }

    public static NativeLinkageModule Inspect(string path) => Read(path, null);

    private static NativeLinkageModule Read(string path, bool? expectDll)
    {
        NativeStartupInspector.RejectReparsePath(path);
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        Require(stream.Length is >= 64 and <= PeStartupInspector.MaxModuleBytes, "startup_module_size");
        using var pe = new PEReader(stream, PEStreamOptions.LeaveOpen);
        var headers = pe.PEHeaders;
        var h = headers.PEHeader ?? throw new InvalidDataException("startup_missing_pe_header");
        var dll = (headers.CoffHeader.Characteristics & Characteristics.Dll) != 0;
        Require(headers.CoffHeader.Machine == Machine.I386 && h.Magic == PEMagic.PE32 && !pe.HasMetadata &&
            h.CorHeaderTableDirectory.RelativeVirtualAddress == 0 && h.CorHeaderTableDirectory.Size == 0 &&
            (headers.CoffHeader.Characteristics & Characteristics.ExecutableImage) != 0 && (expectDll is null || dll == expectDll),
            "startup_native_x86_kind");
        var layout = new PeStartupInspector.Reader(stream, headers);
        var notes = layout.ValidateLayout();
        if (h.AddressOfEntryPoint != 0 || !dll) layout.Executable(checked((uint)h.AddressOfEntryPoint));
        var reader = new Reader(stream, headers, layout);
        var imports = new List<NativeImportSymbol>();
        reader.Imports(h.ImportTableDirectory, false, imports);
        reader.Imports(h.DelayImportTableDirectory, true, imports);
        var exports = reader.Exports(h.ExportTableDirectory);
        stream.Position = 0;
        var hash = Convert.ToHexStringLower(SHA256.HashData(stream));
        return new(Path.GetFileName(path), stream.Length, hash, dll, imports.ToArray(), exports, notes);
    }

    private static void Require(bool value, string reason)
    {
        if (!value) throw new InvalidDataException(reason);
    }

    private sealed class Reader(Stream stream, PEHeaders headers, PeStartupInspector.Reader layout)
    {
        private ushort Word(uint rva)
        {
            stream.Position = layout.Offset(rva, 2);
            Span<byte> bytes = stackalloc byte[2]; stream.ReadExactly(bytes);
            return BinaryPrimitives.ReadUInt16LittleEndian(bytes);
        }

        private string Symbol(uint rva, ulong end = ulong.MaxValue)
        {
            Span<byte> bytes = stackalloc byte[MaxSymbolBytes];
            for (var i = 0; i < bytes.Length; i++)
            {
                var at = checked(rva + (uint)i);
                Require(at < end, "linkage_string_bounds");
                stream.Position = layout.Offset(at, 1);
                var b = stream.ReadByte();
                if (b == 0)
                {
                    Require(i > 0, "linkage_empty_symbol");
                    return Encoding.ASCII.GetString(bytes[..i]);
                }
                Require(b is >= 32 and < 127, "linkage_symbol_ascii");
                bytes[i] = (byte)b;
            }
            throw new InvalidDataException("linkage_symbol_length");
        }

        public void Imports(DirectoryEntry directory, bool delay, List<NativeImportSymbol> output)
        {
            // Reuse the descriptor/name/termination checks of the startup inventory.
            var modules = layout.Imports(directory, delay);
            if (modules.Length == 0) return;
            var stride = delay ? 32U : 20U;
            var start = checked((uint)directory.RelativeVirtualAddress);
            for (uint i = 0; i < modules.Length; i++)
            {
                var at = checked(start + i * stride);
                uint lookup, iat, nameRva;
                if (delay)
                {
                    // Legacy VA descriptors require a separate compatibility corpus; do not guess pointer semantics.
                    Require(layout.Number(at) == 1, "linkage_legacy_delay_unsupported");
                    nameRva = layout.Number(at + 4); iat = layout.Number(at + 12); lookup = layout.Number(at + 16);
                }
                else
                {
                    nameRva = layout.Number(at + 12); iat = layout.Number(at + 16); lookup = layout.Number(at);
                    if (lookup == 0)
                    {
                        Require(layout.Number(at + 4) == 0, "linkage_bound_iat_without_lookup");
                        lookup = iat;
                    }
                }
                Require(lookup != 0 && iat != 0, "linkage_missing_thunk_table");
                var module = layout.ModuleName(nameRva);
                for (uint j = 0; ; j++)
                {
                    var lookupAt = checked(lookup + 4 * j);
                    var iatAt = checked(iat + 4 * j);
                    var value = layout.Number(lookupAt);
                    var iatValue = layout.Number(iatAt);
                    if (value == 0)
                    {
                        Require(iatValue == 0, "linkage_iat_termination");
                        break;
                    }
                    Require(output.Count < MaxSymbols, "linkage_import_symbol_limit");
                    Require(iatValue != 0, "linkage_iat_early_termination");
                    string? name = null; uint? ordinal = null; ushort? hint = null;
                    if ((value & 0x80000000U) != 0)
                    {
                        Require((value & 0x7fff0000U) == 0, "linkage_ordinal_reserved_bits");
                        ordinal = value & 0xffffU;
                        Require(ordinal != 0, "linkage_ordinal_zero");
                    }
                    else { hint = Word(value); name = Symbol(checked(value + 2)); }
                    output.Add(new(module, delay ? "delay" : "normal", name, ordinal, hint, lookupAt, iatAt));
                }
            }
        }

        public NativeExportSymbol[] Exports(DirectoryEntry directory)
        {
            if (!layout.Directory(directory, 40)) return [];
            var at = checked((uint)directory.RelativeVirtualAddress);
            var ordinalBase = layout.Number(at + 16);
            var count = layout.Number(at + 20); var nameCount = layout.Number(at + 24);
            Require(count <= MaxSymbols && nameCount <= MaxSymbols, "linkage_export_symbol_limit");
            Require(count != 0 || nameCount == 0, "linkage_export_name_without_table");
            if (count == 0) return [];
            Require((ulong)ordinalBase + count - 1 <= uint.MaxValue, "linkage_export_ordinal_overflow");
            var eat = layout.Number(at + 28); var names = layout.Number(at + 32); var ordinals = layout.Number(at + 36);
            _ = layout.Offset(eat, count * 4L);
            if (nameCount != 0) { _ = layout.Offset(names, nameCount * 4L); _ = layout.Offset(ordinals, nameCount * 2L); }
            var aliases = new Dictionary<uint, List<string>>();
            string? previous = null;
            for (uint i = 0; i < nameCount; i++)
            {
                var name = Symbol(layout.Number(checked(names + 4 * i)));
                Require(previous is null || StringComparer.Ordinal.Compare(previous, name) < 0, "linkage_export_name_order");
                previous = name;
                var index = Word(checked(ordinals + 2 * i));
                Require(index < count, "linkage_export_name_index");
                if (!aliases.TryGetValue(index, out var list)) aliases.Add(index, list = []);
                list.Add(name);
            }
            var result = new List<NativeExportSymbol>();
            for (uint i = 0; i < count; i++)
            {
                var rva = layout.Number(checked(eat + 4 * i));
                if (rva == 0) continue; // An ordinal hole, including a named hole, never resolves.
                string kind; string? forwarder = null;
                if (rva >= at && (ulong)rva < (ulong)at + (uint)directory.Size)
                {
                    forwarder = Symbol(rva, (ulong)at + (uint)directory.Size);
                    kind = "forwarder"; // Text only. No automatic module resolution or chain traversal.
                }
                else
                {
                    var section = headers.SectionHeaders.SingleOrDefault(s => rva >= s.VirtualAddress &&
                        (long)rva - s.VirtualAddress < Math.Max(s.VirtualSize, s.SizeOfRawData));
                    Require(section.Name is not null, "linkage_export_target_bounds");
                    var code = (section.SectionCharacteristics & SectionCharacteristics.MemExecute) != 0;
                    if (code) _ = layout.Offset(rva, 1);
                    kind = code ? "executable_section" : "data_section";
                }
                result.Add(new(checked(ordinalBase + i), aliases.TryGetValue(i, out var list) ? list.ToArray() : [], rva, kind, forwarder));
            }
            return result.ToArray();
        }
    }
}
