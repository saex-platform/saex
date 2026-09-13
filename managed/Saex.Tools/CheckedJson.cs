using System.Text.Json;

namespace Saex.Tools;

public static class CheckedJson
{
    public const int MaxBytes = 1024 * 1024;

    public static byte[] ReadFile(string path)
    {
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        if (stream.Length > MaxBytes) throw new InvalidDataException("json_byte_limit");
        using var buffer = new MemoryStream();
        var chunk = new byte[8192];
        int count;
        while ((count = stream.Read(chunk)) != 0)
        {
            if (buffer.Length + count > MaxBytes) throw new InvalidDataException("json_byte_limit");
            buffer.Write(chunk, 0, count);
        }
        return buffer.ToArray();
    }

    public static JsonDocument Parse(ReadOnlyMemory<byte> bytes)
    {
        if (bytes.Length > MaxBytes) throw new InvalidDataException("json_byte_limit");
        var document = JsonDocument.Parse(bytes, new JsonDocumentOptions { MaxDepth = 32 });
        try
        {
            var remaining = 16384;
            Inspect(document.RootElement, ref remaining);
            return document;
        }
        catch { document.Dispose(); throw; }
    }

    private static void Inspect(JsonElement element, ref int remaining)
    {
        if (--remaining < 0) throw new InvalidDataException("json_node_limit");
        if (element.ValueKind == JsonValueKind.Object)
        {
            var names = new HashSet<string>(StringComparer.Ordinal);
            foreach (var property in element.EnumerateObject())
            {
                if (!names.Add(property.Name)) throw new InvalidDataException("duplicate_json_property: " + property.Name);
                if (property.Name.Length > 128) throw new InvalidDataException("json_name_limit");
                Inspect(property.Value, ref remaining);
            }
        }
        else if (element.ValueKind == JsonValueKind.Array)
            foreach (var item in element.EnumerateArray()) Inspect(item, ref remaining);
    }
}
