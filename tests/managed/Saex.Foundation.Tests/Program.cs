using System.Buffers.Binary;
using System.Diagnostics;
using System.Reflection.PortableExecutable;
using System.Text;
using System.Text.Json.Nodes;
using Saex.Contracts;
using Saex.Tools;

if (args.Length != 2)
{
    Console.Error.WriteLine("Usage: Saex.Foundation.Tests <repository-root> <native-probe-path>");
    return 2;
}
var root = Path.GetFullPath(args[0]);
var native = Path.GetFullPath(args[1]);
var sample = File.ReadAllText(Path.Combine(root, "samples/foundation/world-plan.json"));
var entity = new EntityRef(new(0x0102030405060708UL), new(2), new(3), new(ulong.MaxValue));
var tests = new List<(string Name, Func<Task> Run)>();
void Test(string name, Action action) => tests.Add((name, () => { action(); return Task.CompletedTask; }));
void Require(bool condition, string detail) { if (!condition) throw new InvalidOperationException(detail); }
PlanValidation Change(Action<JsonNode> modify)
{
    var node = JsonNode.Parse(sample)!; modify(node);
    return WorldPlanValidator.Validate(Encoding.UTF8.GetBytes(node.ToJsonString()));
}
void Has(PlanValidation result, string code) => Require(!result.Valid && result.Diagnostics.Any(x => x.Code == code), code);

Test("metadata plan order is stable and never production eligible", () =>
{
    var result = WorldPlanValidator.Validate(Encoding.UTF8.GetBytes(sample));
    Require(result.Valid && !result.ProductionEligible, "eligibility boundary");
    Require(result.ResourceOrder.SequenceEqual(["saex.foundation", "sample.harbor"]), "dependency order");
    Require(result.AssetOrder.SequenceEqual(["sample:crate/collision", "sample:crate/prefab"]), "asset order");
    var reverse = Change(node =>
    {
        var items = node["resources"]!.AsArray(); var first = items[0]!.DeepClone();
        var second = items[1]!.DeepClone(); items.Clear(); items.Add(second); items.Add(first);
    });
    Require(reverse.ResourceOrder.SequenceEqual(result.ResourceOrder), "input order affected result");
});
Test("duplicate service provider rejected", () => Has(Change(n => n["resources"]![1]!["provides"] = new JsonArray("world.state")), "provider_conflict"));
Test("missing service provider rejected", () => Has(Change(n => n["resources"]![1]!["requires"] = new JsonArray("missing.service")), "missing_provider"));
Test("field mutator conflict rejected", () => Has(Change(n => n["resources"]![1]!["mutates"] = new JsonArray("entity.identity")), "mutator_conflict"));
Test("resource dependency cycle rejected", () => Has(Change(n => n["resources"]![0]!["dependencies"] = new JsonArray("sample.harbor")), "dependency_cycle"));
Test("missing resource dependency rejected", () => Has(Change(n => n["resources"]![1]!["dependencies"] = new JsonArray("missing.resource")), "missing_dependency"));
Test("asset dependency cycle rejected", () => Has(Change(n => n["assets"]![0]!["dependencies"] = new JsonArray("sample:crate/prefab")), "dependency_cycle"));
Test("missing collision dependency rejected", () => Has(Change(n => n["assets"]![1]!["dependencies"] = new JsonArray("sample:missing")), "missing_asset"));
Test("indirect critical dependency cannot be optional", () => Has(Change(n =>
{
    n["assets"]![0]!["gameplayCritical"] = false;
    n["assets"]![0]!["optionalDownload"] = true;
}), "critical_optional_asset"));
Test("missing declared capability rejected", () => Has(Change(n => n["availableCapabilities"] = new JsonArray()), "missing_capability"));
Test("recovery hierarchy rejected", () => Has(Change(n => n["recovery"]!["maxActorOperations"] = 129), "invalid_recovery_budget"));
Test("recovery timeout cap rejected", () => Has(Change(n => n["recovery"]!["syncTimeoutMilliseconds"] = 30001), "invalid_recovery_budget"));
Test("schema mismatch rejected", () => Has(Change(n => n["schemaVersion"] = 2), "unsupported_schema"));
Test("unknown required JSON field rejected", () => Has(Change(n => n["silentlyIgnored"] = true), "invalid_document"));
Test("missing required JSON field rejected", () => Has(Change(n => n.AsObject().Remove("recovery")), "invalid_document"));
Test("null definition collection rejected", () => Has(Change(n => n["resources"] = null), "invalid_document"));
Test("null array element rejected", () => Has(Change(n => n["resources"]![0] = null), "null_element"));
Test("null dependency name rejected without exception", () => Has(Change(n => n["resources"]![1]!["dependencies"] = new JsonArray((JsonNode?)null)), "invalid_names"));
Test("duplicate JSON key rejected", () => Has(WorldPlanValidator.Validate(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"schemaVersion\":1}")), "invalid_document"));
Test("oversized JSON rejected before parsing", () => Has(WorldPlanValidator.Validate(new byte[CheckedJson.MaxBytes + 1]), "invalid_document"));
Test("asset traversal rejected", () => Has(Change(n => n["assets"]![0]!["id"] = "sample:../escape"), "invalid_asset_id"));
Test("typed fixture rejects all prefixes and corrupt version", () =>
{
    var frame = FixtureCodec.Encode(entity);
    Require(frame[12] == 8 && frame[19] == 1, "endianness");
    Require(FixtureCodec.Decode(frame) == entity, "roundtrip");
    for (var i = 0; i < frame.Length; i++)
    {
        try { FixtureCodec.Decode(frame.AsSpan(0, i)); throw new InvalidOperationException("truncated accepted"); }
        catch (InvalidDataException) { }
    }
    frame[4] = 2;
    try { FixtureCodec.Decode(frame); throw new InvalidOperationException("wrong version accepted"); }
    catch (InvalidDataException) { }
});
Test("PE inspector rejects broken images and never approves managed x86", () =>
{
    var temp = Path.Combine(Path.GetTempPath(), "saex-pe-fixture-" + Guid.NewGuid().ToString("N") + ".exe");
    try
    {
        File.WriteAllBytes(temp, new byte[128]);
        try { EngineInspector.Inspect(temp); throw new InvalidOperationException("broken PE accepted"); }
        catch (Exception e) when (e is BadImageFormatException or InvalidDataException) { }
        var own = EngineInspector.Inspect(typeof(FixtureCodec).Assembly.Location);
        Require(!own.CanAttach && !own.RecognizedProfile && !own.NativeX86Executable, "managed PE approved");
        Require(!own.FileProfileMatched && own.ObservedProfileId is null, "managed image matched observation");
        var profileBytes = File.ReadAllBytes(Path.Combine(root, "contracts/engine/observed-profile.json"));
        Require(own.ProfileSourceDigest == Convert.ToHexStringLower(System.Security.Cryptography.SHA256.HashData(profileBytes)),
            "embedded profile differs from native generator source");
    }
    finally { File.Delete(temp); }
});

Test("native PE with unknown hash never receives an observed profile", () =>
{
    var b = new byte[1024];
    void U16(int at, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(at), value);
    void U32(int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(at), value);
    U16(0, 0x5a4d); U32(60, 64); U32(64, 0x4550); U16(68, 0x14c); U16(70, 1);
    U16(84, 224); U16(86, 0x102); U16(88, 0x10b); U32(104, 4096); U32(116, 0x400000);
    U32(120, 4096); U32(124, 512); U32(144, 8192); U32(148, 512); U32(180, 16);
    b[312] = (byte)'.'; b[313] = (byte)'x'; U32(320, 512); U32(324, 4096);
    U32(328, 512); U32(332, 512); U32(348, 0x60000020);
    var temp = Path.Combine(Path.GetTempPath(), "saex-unknown-pe-" + Guid.NewGuid().ToString("N") + ".exe");
    try
    {
        File.WriteAllBytes(temp, b);
        var result = EngineInspector.Inspect(temp);
        Require(result.NativeX86Executable && !result.FileProfileMatched && result.ObservedProfileId is null &&
            !result.CanAttach && !result.RecognizedProfile && result.Reason == "unknown_fingerprint", "unknown profile allowed");
    }
    finally { File.Delete(temp); }
});

tests.Add(("C# to native process full-width EntityRef roundtrip", async () =>
{
    var frame = FixtureCodec.Encode(entity);
    var result = await RunProbe(native, frame);
    Require(result.ExitCode == 0 && result.Stdout.SequenceEqual(frame), "native roundtrip mismatch: " + result.Stderr);
    Require(FixtureCodec.Decode(result.Stdout) == entity, "decoded native result mismatch");
}));
tests.Add(("native process rejects malformed/trailing frames without output", async () =>
{
    var good = FixtureCodec.Encode(entity);
    var wrongVersion = (byte[])good.Clone(); wrongVersion[4] = 2;
    var huge = good[..12]; BinaryPrimitives.WriteUInt32LittleEndian(huge.AsSpan(8), uint.MaxValue);
    var zero = (byte[])good.Clone(); zero.AsSpan(12, 8).Clear();
    foreach (var payload in new[] { Array.Empty<byte>(), good[..10], good[..43], wrongVersion, huge, zero, good.Concat(new byte[] { 1 }).ToArray() })
    {
        var result = await RunProbe(native, payload);
        Require(result.ExitCode != 0 && result.Stdout.Length == 0, "malformed input yielded accepted state");
    }
}));

StartupTests.Register(Test);
LinkageTests.Register(Test);

var failures = 0;
foreach (var (name, run) in tests)
{
    try { await run(); Console.WriteLine("PASS " + name); }
    catch (Exception error) { failures++; Console.Error.WriteLine("FAIL " + name + ": " + error.Message); }
}
Console.WriteLine($"{tests.Count - failures}/{tests.Count} managed/integration tests passed");
return failures == 0 ? 0 : 1;

static async Task<(int ExitCode, byte[] Stdout, string Stderr)> RunProbe(string path, byte[] frame)
{
    using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(5));
    using var process = new Process
    {
        StartInfo = new ProcessStartInfo(path)
        {
            UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardInput = true, RedirectStandardOutput = true, RedirectStandardError = true
        }
    };
    process.Start();
    try
    {
        var output = ReadBounded(process.StandardOutput.BaseStream, 64, timeout.Token);
        var error = ReadBounded(process.StandardError.BaseStream, 4096, timeout.Token);
        try { await process.StandardInput.BaseStream.WriteAsync(frame, timeout.Token); }
        catch (IOException) { /* Rejection may close stdin before its body is consumed. */ }
        process.StandardInput.Close();
        await process.WaitForExitAsync(timeout.Token);
        return (process.ExitCode, await output, Encoding.UTF8.GetString(await error));
    }
    finally
    {
        if (!process.HasExited) { process.Kill(entireProcessTree: true); await process.WaitForExitAsync(); }
    }
}

static async Task<byte[]> ReadBounded(Stream stream, int limit, CancellationToken cancellation)
{
    using var output = new MemoryStream();
    var buffer = new byte[128];
    int count;
    while ((count = await stream.ReadAsync(buffer, cancellation)) != 0)
    {
        if (output.Length + count > limit) throw new InvalidDataException("process_output_limit");
        output.Write(buffer, 0, count);
    }
    return output.ToArray();
}
