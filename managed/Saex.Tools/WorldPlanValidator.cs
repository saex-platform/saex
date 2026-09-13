using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace Saex.Tools;

public sealed record Diagnostic(string Code, string Path, string Message);
public sealed record PlanValidation(bool Valid, bool ProductionEligible, string SourceDigest,
    IReadOnlyList<string> ResourceOrder, IReadOnlyList<string> AssetOrder, IReadOnlyList<Diagnostic> Diagnostics);

public sealed record ResourceInput(string Id, string ArtifactDigest, string[] Dependencies,
    string[] Provides, string[] Requires, string[] Mutates);
public sealed record AssetInput(string Id, string ArtifactDigest, string[] Dependencies,
    bool GameplayCritical, bool OptionalDownload);
public sealed record RecoveryInput(int MaxActorOperations, int MaxResourceOperations,
    int MaxWorldOperations, int MaxProcessOperations, int MaxQueuedBytes,
    int MaxBaselineBytes, int SyncTimeoutMilliseconds, int MaxSyncRestarts, int MaxWorkerRestarts);
public sealed record WorldPlanInput(int SchemaVersion, string WorldId, ResourceInput[] Resources,
    AssetInput[] Assets, string[] RequiredCapabilities, string[] AvailableCapabilities, RecoveryInput Recovery);

// D1 metadata subset. Actual artifacts, engine evidence, semantics and runtime admission are later gates.
public static class WorldPlanValidator
{
    private static readonly JsonSerializerOptions Options = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
        RespectRequiredConstructorParameters = true,
        RespectNullableAnnotations = true,
        MaxDepth = 32
    };

    public static PlanValidation Validate(ReadOnlyMemory<byte> bytes)
    {
        var issues = new List<Diagnostic>();
        void Error(string code, string path, string message) => issues.Add(new(code, path, message));
        WorldPlanInput input;
        try
        {
            using var document = CheckedJson.Parse(bytes);
            input = document.Deserialize<WorldPlanInput>(Options) ?? throw new InvalidDataException("null_plan");
        }
        catch (Exception exception) when (exception is JsonException or InvalidDataException or NotSupportedException)
        {
            return new(false, false, "", [], [], [new("invalid_document", "$", exception.Message)]);
        }
        var digest = Convert.ToHexStringLower(SHA256.HashData(bytes.Span));
        if (input.SchemaVersion != 1) Error("unsupported_schema", "schemaVersion", "D1 foundation accepts version 1 only.");
        if (!Identifier(input.WorldId)) Error("invalid_identifier", "worldId", "Use a bounded lower-case identifier.");
        if (input.Resources.Length > 64 || input.Assets.Length > 256 ||
            input.RequiredCapabilities.Length > 128 || input.AvailableCapabilities.Length > 128)
            return new(false, false, digest, [], [], [new("collection_limit", "$", "Plan cardinality exceeded.")]);
        // Nullable annotations do not validate collection elements during deserialization.
        if (input.Resources.Any(x => x is null) || input.Assets.Any(x => x is null))
            return new(false, false, digest, [], [], [new("null_element", "$", "Null definition is not allowed.")]);

        var resources = new Dictionary<string, ResourceInput>(StringComparer.Ordinal);
        var services = new Dictionary<string, string>(StringComparer.Ordinal);
        var mutators = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var resource in input.Resources)
        {
            var path = "resources/" + resource.Id;
            if (!Identifier(resource.Id)) { Error("invalid_identifier", path, "Invalid resource identifier."); continue; }
            if (!resources.TryAdd(resource.Id, resource)) Error("duplicate_resource", path, "Resource IDs must be unique.");
            if (!Digest(resource.ArtifactDigest)) Error("invalid_digest", path, "Expected lower-case SHA-256.");
            CheckNames(resource.Dependencies, path + "/dependencies", Error);
            CheckNames(resource.Requires, path + "/requires", Error);
            CheckNames(resource.Provides, path + "/provides", Error);
            CheckNames(resource.Mutates, path + "/mutates", Error);
            foreach (var service in resource.Provides.Where(Identifier))
                if (!services.TryAdd(service, resource.Id)) Error("provider_conflict", path + "/provides", service);
            foreach (var field in resource.Mutates.Where(Identifier))
                if (!mutators.TryAdd(field, resource.Id)) Error("mutator_conflict", path + "/mutates", field);
        }
        var edges = resources.ToDictionary(x => x.Key, _ => new HashSet<string>(StringComparer.Ordinal), StringComparer.Ordinal);
        foreach (var (id, resource) in resources)
        {
            foreach (var dependency in resource.Dependencies.Where(Identifier))
                if (!resources.ContainsKey(dependency)) Error("missing_dependency", "resources/" + id, dependency);
                else edges[id].Add(dependency);
            foreach (var service in resource.Requires.Where(Identifier))
                if (!services.TryGetValue(service, out var provider)) Error("missing_provider", "resources/" + id, service);
                else if (provider != id) edges[id].Add(provider);
        }

        var assets = new Dictionary<string, AssetInput>(StringComparer.Ordinal);
        foreach (var asset in input.Assets)
        {
            var path = "assets/" + asset.Id;
            if (!AssetIdentifier(asset.Id)) { Error("invalid_asset_id", path, "Invalid namespaced AssetRef."); continue; }
            if (!assets.TryAdd(asset.Id, asset)) Error("duplicate_asset", path, "AssetRef must be unique.");
            if (!Digest(asset.ArtifactDigest)) Error("invalid_digest", path, "Expected lower-case SHA-256.");
            if (asset.GameplayCritical && asset.OptionalDownload)
                Error("critical_optional_asset", path, "Gameplay closure cannot be optional.");
            if (asset.Dependencies.Length > 256 || asset.Dependencies.Any(x => !AssetIdentifier(x)))
                Error("invalid_dependencies", path, "Invalid/bounded dependency list required.");
            if (asset.Dependencies.Distinct(StringComparer.Ordinal).Count() != asset.Dependencies.Length)
                Error("duplicate_dependency", path, "Repeated dependency.");
        }
        var assetEdges = assets.ToDictionary(x => x.Key, _ => new HashSet<string>(StringComparer.Ordinal), StringComparer.Ordinal);
        foreach (var (id, asset) in assets)
            foreach (var dependency in asset.Dependencies.Where(AssetIdentifier))
                if (!assets.ContainsKey(dependency)) Error("missing_asset", "assets/" + id, dependency);
                else assetEdges[id].Add(dependency);

        // Requiredness propagates through dependency closure, not only direct metadata.
        var critical = new HashSet<string>(StringComparer.Ordinal);
        var pending = new Queue<string>(assets.Where(x => x.Value.GameplayCritical).Select(x => x.Key));
        while (pending.TryDequeue(out var id))
        {
            if (!critical.Add(id)) continue;
            if (assets[id].OptionalDownload) Error("critical_optional_asset", "assets/" + id, "Required by gameplay closure.");
            foreach (var dependency in assetEdges[id]) pending.Enqueue(dependency);
        }
        CheckNames(input.RequiredCapabilities, "requiredCapabilities", Error);
        CheckNames(input.AvailableCapabilities, "availableCapabilities", Error);
        var available = new HashSet<string>(input.AvailableCapabilities.Where(Identifier), StringComparer.Ordinal);
        foreach (var capability in input.RequiredCapabilities.Where(Identifier))
            if (!available.Contains(capability)) Error("missing_capability", "requiredCapabilities", capability);
        var budget = input.Recovery;
        if (budget.MaxActorOperations <= 0 || budget.MaxActorOperations > budget.MaxResourceOperations ||
            budget.MaxResourceOperations > budget.MaxWorldOperations || budget.MaxWorldOperations > budget.MaxProcessOperations ||
            budget.MaxActorOperations > 32 || budget.MaxResourceOperations > 128 ||
            budget.MaxWorldOperations > 1024 || budget.MaxProcessOperations > 4096 ||
            budget.MaxQueuedBytes is <= 0 or > 8 * 1024 * 1024 || budget.MaxBaselineBytes is <= 0 or > 8 * 1024 * 1024 ||
            budget.SyncTimeoutMilliseconds is <= 0 or > 30_000 || budget.MaxSyncRestarts is < 0 or > 2 ||
            budget.MaxWorkerRestarts is < 0 or > 3)
            Error("invalid_recovery_budget", "recovery", "D1 bounded profile or hierarchy violated.");

        var order = Sort(edges, "resources", Error);
        var assetOrder = Sort(assetEdges, "assets", Error);
        return new(issues.Count == 0, false, digest, order, assetOrder, issues);
    }

    private static bool Identifier(string? value) => value is { Length: > 0 and <= 128 } &&
        Regex.IsMatch(value, "^[a-z0-9][a-z0-9._-]*$") && !value.Contains("..");
    private static bool AssetIdentifier(string? value) => value is { Length: > 0 and <= 192 } &&
        Regex.IsMatch(value, "^[a-z0-9][a-z0-9._-]*:[a-z0-9][a-z0-9._/-]*$") &&
        !value.Contains("..") && !value.Contains("//") && !value.EndsWith('/');
    private static bool Digest(string? value) => value is { Length: 64 } && Regex.IsMatch(value, "^[0-9a-f]{64}$");

    private static void CheckNames(string[] names, string path, Action<string, string, string> error)
    {
        if (names.Length > 128 || names.Any(x => !Identifier(x))) error("invalid_names", path, "Invalid/bounded name list required.");
        if (names.Distinct(StringComparer.Ordinal).Count() != names.Length) error("duplicate_name", path, "Repeated identifier.");
    }

    private static string[] Sort(Dictionary<string, HashSet<string>> edges, string path, Action<string, string, string> error)
    {
        var result = new List<string>();
        var done = new HashSet<string>(StringComparer.Ordinal);
        while (result.Count < edges.Count)
        {
            var ready = edges.Keys.Where(id => !done.Contains(id) && edges[id].All(done.Contains))
                .Order(StringComparer.Ordinal).ToArray();
            if (ready.Length == 0)
            {
                error("dependency_cycle", path, string.Join(", ", edges.Keys.Where(x => !done.Contains(x)).Order(StringComparer.Ordinal)));
                break;
            }
            foreach (var id in ready) { done.Add(id); result.Add(id); }
        }
        return result.ToArray();
    }
}
