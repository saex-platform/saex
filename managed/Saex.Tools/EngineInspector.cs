using System.Reflection.PortableExecutable;
using System.Security.Cryptography;

namespace Saex.Tools;

public sealed record EngineInspection(string FileName, long Bytes, string Sha256, string Machine,
    bool NativeX86Executable, bool RecognizedProfile, bool CanAttach, string Reason,
    string? ObservedProfileId, bool FileProfileMatched, string ProfileSourceDigest);

public static class EngineInspector
{
    public const long MaxBytes = 256 * 1024 * 1024;
    public static EngineInspection Inspect(string path)
    {
        // Share-read prevents concurrent writers on Windows. No Write/Execute access requested.
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        if (stream.Length is < 64 or > MaxBytes) throw new InvalidDataException("engine_file_size");
        using var reader = new PEReader(stream, PEStreamOptions.LeaveOpen);
        var headers = reader.PEHeaders;
        if (headers.PEHeader is null || headers.SectionHeaders.Length is < 1 or > 96)
            throw new InvalidDataException("invalid_pe_image");
        foreach (var section in headers.SectionHeaders)
            if (section.PointerToRawData < 0 || section.SizeOfRawData < 0 ||
                (long)section.PointerToRawData + section.SizeOfRawData > stream.Length)
                throw new InvalidDataException("invalid_pe_section");
        var nativeX86 = headers.CoffHeader.Machine == Machine.I386 && headers.PEHeader.Magic == PEMagic.PE32 &&
            !reader.HasMetadata && (headers.CoffHeader.Characteristics & Characteristics.ExecutableImage) != 0 &&
            (headers.CoffHeader.Characteristics & Characteristics.Dll) == 0;
        stream.Position = 0;
        var digest = Convert.ToHexStringLower(SHA256.HashData(stream));
        stream.Position = 0;
        var observation = EngineObservation.Match(stream, headers, digest, nativeX86);
        // RecognizedProfile means verified runtime support, not a matching disk observation.
        return new(Path.GetFileName(path), stream.Length, digest, headers.CoffHeader.Machine.ToString(),
            nativeX86, false, false, observation.Reason, observation.Id, observation.Matched, observation.SourceDigest);
    }
}
