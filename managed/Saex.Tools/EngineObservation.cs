using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text.Json;

namespace Saex.Tools;

// Build-time embedded observation. No user supplied profile can authorize runtime access.
internal static class EngineObservation
{
    internal sealed record Result(string? Id, bool Matched, string SourceDigest, string Reason);

    internal static Result Match(Stream file, PEHeaders headers, string digest, bool nativeX86)
    {
        using var source = typeof(EngineObservation).Assembly.GetManifestResourceStream("Saex.ObservedEngineProfile")
            ?? throw new InvalidDataException("missing_embedded_engine_profile");
        if (source.Length > 65536) throw new InvalidDataException("profile_size_limit");
        var bytes = new byte[checked((int)source.Length)]; source.ReadExactly(bytes);
        var sourceDigest = Convert.ToHexStringLower(SHA256.HashData(bytes));
        using var document = CheckedJson.Parse(bytes);
        var p = document.RootElement;
        if (p.GetProperty("schemaVersion").GetInt32() != 1 || !p.GetProperty("observationOnly").GetBoolean())
            throw new InvalidDataException("profile_must_remain_observation_only");
        Result Reject(string why) => new(null, false, sourceDigest, why);
        if (!nativeX86) return Reject("unsupported_engine_architecture");
        if (file.Length != p.GetProperty("fileBytes").GetInt64() || digest != p.GetProperty("sha256").GetString())
            return Reject("unknown_fingerprint");
        var l = p.GetProperty("layout");
        var h = headers.PEHeader!;
        bool Equal(string key, long value) => l.GetProperty(key).GetInt64() == value;
        if (!Equal("timestamp", (uint)headers.CoffHeader.TimeDateStamp) ||
            !Equal("characteristics", (ushort)headers.CoffHeader.Characteristics) ||
            !Equal("imageBase", checked((long)h.ImageBase)) || !Equal("entryRva", h.AddressOfEntryPoint) ||
            !Equal("imageSize", h.SizeOfImage) || !Equal("headersSize", h.SizeOfHeaders) ||
            !Equal("sectionAlignment", h.SectionAlignment) || !Equal("fileAlignment", h.FileAlignment) ||
            !Equal("dllCharacteristics", (ushort)h.DllCharacteristics)) return Reject("profile_layout_mismatch");
        var sections = l.GetProperty("sections");
        if (sections.GetArrayLength() != headers.SectionHeaders.Length) return Reject("profile_layout_mismatch");
        for (var i = 0; i < sections.GetArrayLength(); i++)
        {
            var expected = sections[i]; var actual = headers.SectionHeaders[i];
            if (expected.GetProperty("name").GetString() != actual.Name ||
                expected.GetProperty("virtualSize").GetInt64() != actual.VirtualSize ||
                expected.GetProperty("rva").GetInt64() != actual.VirtualAddress ||
                expected.GetProperty("rawSize").GetInt64() != actual.SizeOfRawData ||
                expected.GetProperty("rawOffset").GetInt64() != actual.PointerToRawData ||
                expected.GetProperty("characteristics").GetUInt32() != (uint)actual.SectionCharacteristics)
                return Reject("profile_layout_mismatch");
        }
        var anchors = p.GetProperty("anchors");
        if (anchors.GetArrayLength() is < 1 or > 32) return Reject("invalid_anchor");
        Span<byte> buffer = stackalloc byte[16];
        foreach (var anchor in anchors.EnumerateArray())
        {
            var index = anchor.GetProperty("sectionIndex").GetInt32();
            if (index < 0 || index >= headers.SectionHeaders.Length) return Reject("invalid_anchor");
            var hex = anchor.GetProperty("bytes").GetString()!;
            if (hex.Length is < 2 or > 32 || hex.Length % 2 != 0) return Reject("invalid_anchor");
            var expected = Convert.FromHexString(hex);
            var section = headers.SectionHeaders[index];
            var offset = anchor.GetProperty("rva").GetInt64() - section.VirtualAddress;
            if ((section.SectionCharacteristics & SectionCharacteristics.MemExecute) == 0 || offset < 0 ||
                offset + expected.Length > section.SizeOfRawData || section.PointerToRawData + offset + expected.Length > file.Length)
                return Reject("invalid_anchor");
            file.Position = section.PointerToRawData + offset;
            var actual = buffer[..expected.Length]; file.ReadExactly(actual);
            if (!actual.SequenceEqual(expected)) return Reject("anchor_mismatch");
        }
        return new(p.GetProperty("id").GetString(), true, sourceDigest, "observed_profile_runtime_unverified");
    }
}
