using System.Security.Cryptography;
using System.Text.Json;

namespace Saex.Tools;

public sealed record StartupIssue(string Module, string Code);
public sealed record StartupImportEdge(string From, string RequestedName, string Kind, string? LocalCandidate);
public sealed record StartupSnapshot(StartupModule Engine, StartupModule[] LocalModules, StartupImportEdge[] Imports,
    string[] ExtensionMarkers, StartupIssue[] Issues, long ChargedBytes)
{
    public string Scope => "static-native-startup-inventory";
    public int SchemaVersion => 1;
    public bool CanAttach => false;
    public bool CanAdvanceToLoader => false;
    public bool RuntimeResolutionVerified => false;
    public bool DynamicLoadsEnumerated => false;
    public bool MetadataComplete => Issues.Length == 0;
    public string Reason => "startup_closure_runtime_unverified";
}
public sealed record StartupInventory(StartupSnapshot Snapshot, string InventoryDigest);

public static class NativeStartupInspector
{
    public const int MaxDirectoryEntries = 4096;
    public const int MaxModules = 128;
    public const long MaxTotalBytes = 256L * 1024 * 1024;

    internal static void RejectReparsePath(string path)
    {
        var full = Path.GetFullPath(path);
        for (string? current = full; current is not null; current = Path.GetDirectoryName(current))
            if ((File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidDataException("startup_reparse_path");
    }

    public static StartupInventory Inspect(string executable)
    {
        var path = Path.GetFullPath(executable);
        RejectReparsePath(path);
        var root = Path.GetDirectoryName(path)!;
        var budget = new PeStartupInspector.Budget(MaxTotalBytes);
        var engine = PeStartupInspector.Read(path, false, budget);
        var candidates = new SortedDictionary<string, string>(StringComparer.Ordinal);
        var markers = new SortedSet<string>(StringComparer.Ordinal);
        var entries = 0;
        // Top level only. Never traverse game assets or plugin directories.
        foreach (var entry in Directory.EnumerateFileSystemEntries(root))
        {
            if (++entries > MaxDirectoryEntries) throw new InvalidDataException("startup_directory_entry_limit");
            var name = Path.GetFileName(entry).ToLowerInvariant();
            var extension = Path.GetExtension(name);
            if (name is "cleo" or "modloader" or "plugins" or "scripts" ||
                name == Path.GetFileName(path).ToLowerInvariant() + ".local" ||
                name == Path.GetFileName(path).ToLowerInvariant() + ".manifest") markers.Add(name);
            if (extension is not (".dll" or ".asi")) continue;
            if (candidates.Count >= MaxModules) throw new InvalidDataException("startup_module_count_limit");
            if (!candidates.TryAdd(name, entry)) throw new InvalidDataException("startup_case_collision");
            if (extension == ".asi") markers.Add(name);
        }
        var modules = new List<StartupModule>();
        var issues = new List<StartupIssue>();
        foreach (var (name, file) in candidates)
        {
            try { modules.Add(PeStartupInspector.Read(file, true, budget)); }
            catch (Exception e) when (e is InvalidDataException or IOException or UnauthorizedAccessException or BadImageFormatException or OverflowException)
            {
                // Do not silently skip malformed/inaccessible native candidates.
                if (e.Message == "startup_total_byte_limit") throw;
                var code = e is InvalidDataException ? e.Message : e switch
                {
                    BadImageFormatException => "startup_bad_pe",
                    UnauthorizedAccessException => "startup_access_denied",
                    OverflowException => "startup_numeric_overflow",
                    _ => "startup_file_io"
                };
                issues.Add(new(name, code));
            }
        }
        var edges = new List<StartupImportEdge>();
        foreach (var module in new[] { engine }.Concat(modules))
        {
            foreach (var (kind, names) in new[] { ("normal", module.Imports), ("delay", module.DelayImports) })
                foreach (var name in names)
                    edges.Add(new(module.Name.ToLowerInvariant(), name, kind, candidates.ContainsKey(name) ? name : null));
        }
        var snapshot = new StartupSnapshot(engine, modules.ToArray(), edges.ToArray(), markers.ToArray(), issues.ToArray(),
            MaxTotalBytes - budget.Remaining);
        // Versioned deterministic diagnostic digest; no timestamp/absolute user path in identity.
        var digest = Convert.ToHexStringLower(SHA256.HashData(JsonSerializer.SerializeToUtf8Bytes(snapshot)));
        return new(snapshot, digest);
    }
}
