using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Saex.Tools;

internal static class StartupTests
{
    private const int Opt = 88, Table = 312;
    private const uint ImageBase = 0x400000;
    private static int Raw(uint rva) => checked((int)rva - 0x2000 + 1024);
    private static void U16(byte[] b, int at, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(at), value);
    private static void U32(byte[] b, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(at), value);
    private static void Dir(byte[] b, int index, uint rva, uint size)
    {
        U32(b, Opt + 96 + index * 8, rva); U32(b, Opt + 100 + index * 8, size);
    }
    private static byte[] Fixture(bool dll = false, params string[] imports)
    {
        var b = new byte[1024 + 0x6000];
        U16(b, 0, 0x5a4d); U32(b, 60, 64); U32(b, 64, 0x4550); U16(b, 68, 0x14c); U16(b, 70, 2);
        U16(b, 84, 224); U16(b, 86, dll ? (ushort)0x2102 : (ushort)0x102);
        U16(b, Opt, 0x10b); U32(b, Opt + 16, 0x1000); U32(b, Opt + 28, ImageBase);
        U32(b, Opt + 32, 4096); U32(b, Opt + 36, 512); U32(b, Opt + 56, 0x8000); U32(b, Opt + 60, 512); U32(b, Opt + 92, 16);
        Encoding.ASCII.GetBytes(".text").CopyTo(b, Table);
        U32(b, Table + 8, 512); U32(b, Table + 12, 4096); U32(b, Table + 16, 512); U32(b, Table + 20, 512); U32(b, Table + 36, 0x60000020);
        Encoding.ASCII.GetBytes(".rdata").CopyTo(b, Table + 40);
        U32(b, Table + 48, 0x6000); U32(b, Table + 52, 8192); U32(b, Table + 56, 0x6000); U32(b, Table + 60, 1024); U32(b, Table + 76, 0x40000040);
        b[512] = 0xc3;
        if (imports.Length != 0)
        {
            Dir(b, 1, 0x2000, checked((uint)(20 * (imports.Length + 1))));
            for (var i = 0; i < imports.Length; i++)
            {
                var name = checked(0x3000U + (uint)i * 64);
                U32(b, Raw(0x2000) + i * 20 + 12, name);
                Encoding.ASCII.GetBytes(imports[i] + "\0").CopyTo(b, Raw(name));
            }
        }
        return b;
    }
    private static void AddTls(byte[] b, params uint[] callbacks)
    {
        Dir(b, 9, 0x6000, 24); U32(b, Raw(0x6000) + 8, ImageBase + 0x6200); U32(b, Raw(0x6000) + 12, ImageBase + 0x6100);
        for (var i = 0; i < callbacks.Length; i++) U32(b, Raw(0x6100) + 4 * i, ImageBase + callbacks[i]);
    }
    private static void AddDelay(byte[] b, string name, bool rvaBased)
    {
        Dir(b, 13, 0x5000, 64); U32(b, Raw(0x5000), rvaBased ? 1U : 0U);
        U32(b, Raw(0x5000) + 4, rvaBased ? 0x5200U : ImageBase + 0x5200);
        Encoding.ASCII.GetBytes(name + "\0").CopyTo(b, Raw(0x5200));
    }
    private sealed class Workspace : IDisposable
    {
        public string Root { get; } = Path.Combine(Path.GetTempPath(), "saex startup " + Guid.NewGuid().ToString("N"));
        public Workspace() => Directory.CreateDirectory(Root);
        public string Write(string name, byte[] bytes)
        {
            var path = Path.Combine(Root, name); File.WriteAllBytes(path, bytes); return path;
        }
        public void Dispose()
        {
            // Delete only explicitly created files in this unique test directory; no recursive shell cleanup.
            foreach (var file in Directory.EnumerateFiles(Root)) File.Delete(file);
            foreach (var child in Directory.EnumerateDirectories(Root)) Directory.Delete(child);
            Directory.Delete(Root);
        }
    }
    private static void Require(bool ok, string why)
    {
        if (!ok) throw new InvalidOperationException(why);
    }
    private static void Reject(byte[] bytes, string reason, bool dll = false)
    {
        using var workspace = new Workspace();
        var path = workspace.Write(dll ? "fixture.dll" : "gta_sa.exe", bytes);
        try { PeStartupInspector.Inspect(path, dll); throw new InvalidOperationException("accepted " + reason); }
        catch (InvalidDataException error) { Require(error.Message == reason, "expected " + reason + "; got " + error.Message); }
    }
    public static void Register(Action<string, Action> test)
    {
        test("startup metadata preserves engine observation boundary", () =>
        {
            using var workspace = new Workspace(); var bytes = Fixture(false, "KERNEL32.dll");
            var path = workspace.Write("gta_sa.exe", bytes); var result = PeStartupInspector.Inspect(path, false);
            Require(result.Imports.SequenceEqual(["kernel32.dll"]) && result.EntryRva == 0x1000 && !result.Tls.Present &&
                result.Engine is { FileProfileMatched: false, CanAttach: false, Reason: "unknown_fingerprint" }, "metadata/profile boundary");
            Require(File.ReadAllBytes(path).SequenceEqual(bytes), "file changed");
        });
        test("startup DLL with no entry point is represented", () =>
        {
            using var workspace = new Workspace(); var b = Fixture(true); U32(b, Opt + 16, 0);
            var result = PeStartupInspector.Inspect(workspace.Write("data.dll", b), true);
            Require(result.IsDll && result.EntryRva == 0 && result.Engine is null, "data DLL");
        });
        test("startup rejects DLL presented as executable", () => Reject(Fixture(true), "startup_native_x86_kind"));
        test("startup rejects executable presented as DLL", () => Reject(Fixture(), "startup_native_x86_kind", true));
        test("startup rejects wrong machine", () => { var b = Fixture(); U16(b, 68, 0x8664); Reject(b, "startup_native_x86_kind"); });
        test("startup rejects nonexecuting entry section", () => { var b = Fixture(); U32(b, Table + 36, 0x40000040); Reject(b, "startup_target_not_executable"); });
        test("startup rejects zero executable entry", () => { var b = Fixture(); U32(b, Opt + 16, 0); Reject(b, "startup_entry_zero"); });
        test("startup rejects overlapping sections", () => { var b = Fixture(); U32(b, Table + 52, 4096); Reject(b, "startup_section_overlap"); });
        test("startup records noncanonical raw padding without loader permission", () =>
        {
            using var workspace = new Workspace(); var b = Fixture(true); U32(b, Table + 56, 0x6000 - 16); b = b[..^16];
            var exe = workspace.Write("gta_sa.exe", Fixture()); workspace.Write("packed.dll", b);
            var snapshot = NativeStartupInspector.Inspect(exe).Snapshot;
            Require(snapshot.MetadataComplete && snapshot.LocalModules[0].LayoutNotes.SequenceEqual(["section_raw_size_unaligned:1"]) &&
                !snapshot.CanAdvanceToLoader, "layout deviation became permission or was hidden");
        });
        test("startup rejects half present data directory", () => { var b = Fixture(); Dir(b, 1, 0x2000, 0); Reject(b, "startup_directory_bounds"); });
        test("startup rejects import directory outside image", () => { var b = Fixture(); Dir(b, 1, 0x7ff0, 40); Reject(b, "startup_rva_bounds"); });
        test("startup rejects unterminated imports", () => { var b = Fixture(false, "one.dll"); Dir(b, 1, 0x2000, 20); Reject(b, "startup_import_termination"); });
        test("startup rejects import traversal and duplicate case aliases", () =>
        {
            foreach (var name in new[] { "../evil.dll", "C:\\evil.dll", "evil/next.dll", ".", "trailing." }) Reject(Fixture(false, name), "startup_import_name");
            Reject(Fixture(false, "Same.dll", "same.DLL"), "startup_duplicate_import");
        });
        test("startup import string and descriptor limits", () =>
        {
            var b = Fixture(false, "a.dll"); b.AsSpan(Raw(0x3000), 128).Fill((byte)'a'); Reject(b, "startup_import_name_limit");
            Reject(Fixture(false, Enumerable.Range(0, 129).Select(i => $"module{i}.dll").ToArray()), "startup_import_limit");
        });
        test("startup delay imports accept RVA and legacy VA names", () =>
        {
            using var workspace = new Workspace();
            foreach (var rva in new[] { true, false })
            {
                var b = Fixture(); AddDelay(b, "LATE.dll", rva);
                Require(PeStartupInspector.Inspect(workspace.Write("delay.exe", b), false).DelayImports.SequenceEqual(["late.dll"]), "delay names");
            }
        });
        test("startup rejects unknown delay attributes", () => { var b = Fixture(); AddDelay(b, "late.dll", true); U32(b, Raw(0x5000), 2); Reject(b, "startup_delay_attributes"); });
        test("startup rejects underflow delay VA", () => { var b = Fixture(); AddDelay(b, "late.dll", false); U32(b, Raw(0x5000) + 4, 1); Reject(b, "startup_va_bounds"); });
        test("startup TLS callback addresses are observed without execution", () =>
        {
            using var workspace = new Workspace(); var b = Fixture(true); AddTls(b, 0x1000, 0x1001);
            var tls = PeStartupInspector.Inspect(workspace.Write("tls.dll", b), true).Tls;
            Require(tls.Present && tls.CallbackRvas.SequenceEqual([0x1000U, 0x1001U]), "TLS callback RVAs");
        });
        test("startup empty TLS list differs from absent TLS directory", () =>
        {
            using var workspace = new Workspace(); var b = Fixture(true); AddTls(b);
            Require(PeStartupInspector.Inspect(workspace.Write("tls.dll", b), true).Tls is { Present: true, CallbackRvas.Length: 0 }, "empty TLS");
        });
        test("startup rejects TLS target in data", () => { var b = Fixture(true); AddTls(b, 0x2000); Reject(b, "startup_target_not_executable", true); });
        test("startup rejects TLS callback outside image", () => { var b = Fixture(true); AddTls(b, 0x8000); Reject(b, "startup_va_bounds", true); });
        test("startup TLS callback count is bounded", () => { var b = Fixture(true); AddTls(b, Enumerable.Repeat(0x1000U, 33).ToArray()); Reject(b, "startup_tls_callback_limit", true); });
        test("startup rejects TLS data overflow", () => { var b = Fixture(true); AddTls(b); U32(b, Raw(0x6000) + 16, uint.MaxValue); Reject(b, "startup_tls_data_bounds", true); });
        test("startup rejects directories in zero-filled section tails", () =>
        {
            var b = Fixture(); U32(b, Table + 56, 512); b = b[..1536]; Dir(b, 1, 0x2200, 40); Reject(b, "startup_rva_not_file_backed");
        });
        test("startup local dependency graph distinguishes candidates from runtime modules", () =>
        {
            using var workspace = new Workspace(); var exe = workspace.Write("gta_sa.exe", Fixture(false, "Proxy.dll", "kernel32.dll"));
            workspace.Write("Proxy.dll", Fixture(true, "helper.dll")); workspace.Write("helper.dll", Fixture(true, "proxy.dll"));
            workspace.Write("plugin.asi", Fixture(true)); Directory.CreateDirectory(Path.Combine(workspace.Root, "modloader"));
            var result = NativeStartupInspector.Inspect(exe); var snapshot = result.Snapshot;
            Require(snapshot.MetadataComplete && !snapshot.CanAttach && !snapshot.CanAdvanceToLoader && !snapshot.RuntimeResolutionVerified &&
                !snapshot.DynamicLoadsEnumerated && snapshot.LocalModules.Length == 3, "startup authority");
            Require(snapshot.Imports.Any(e => e.From == "gta_sa.exe" && e.LocalCandidate == "proxy.dll") &&
                snapshot.Imports.Any(e => e.RequestedName == "kernel32.dll" && e.LocalCandidate is null) &&
                snapshot.Imports.Any(e => e.From == "helper.dll" && e.LocalCandidate == "proxy.dll"), "candidate graph/cycle");
            Require(snapshot.ExtensionMarkers.SequenceEqual(["modloader", "plugin.asi"]), "extension markers");
            Require(NativeStartupInspector.Inspect(exe).InventoryDigest == result.InventoryDigest, "unstable digest");
            var changed = Fixture(true, "proxy.dll"); changed[513] = 1; workspace.Write("helper.dll", changed);
            Require(NativeStartupInspector.Inspect(exe).InventoryDigest != result.InventoryDigest, "changed DLL retained inventory identity");
        });
        test("startup corrupt local DLL remains an explicit incomplete issue", () =>
        {
            using var workspace = new Workspace(); var exe = workspace.Write("gta_sa.exe", Fixture(false, "broken.dll"));
            workspace.Write("broken.dll", new byte[128]); var result = NativeStartupInspector.Inspect(exe).Snapshot;
            Require(!result.MetadataComplete && !result.CanAdvanceToLoader && result.Issues.Length == 1 &&
                result.Issues[0].Module == "broken.dll" && result.Imports[0].LocalCandidate == "broken.dll", "corrupt candidate hidden");
        });
        test("startup module count rejects excessive inventory", () =>
        {
            using var workspace = new Workspace(); var exe = workspace.Write("gta_sa.exe", Fixture());
            for (var i = 0; i <= NativeStartupInspector.MaxModules; i++) workspace.Write($"module{i}.dll", []);
            try { NativeStartupInspector.Inspect(exe); throw new InvalidOperationException("module count accepted"); }
            catch (InvalidDataException e) { Require(e.Message == "startup_module_count_limit", e.Message); }
        });
        test("startup oversized module is rejected before parsing", () =>
        {
            using var workspace = new Workspace(); var file = workspace.Write("large.dll", []);
            using (var stream = File.OpenWrite(file)) stream.SetLength(PeStartupInspector.MaxModuleBytes + 1L);
            try { PeStartupInspector.Inspect(file, true); throw new InvalidOperationException("oversized module accepted"); }
            catch (InvalidDataException e) { Require(e.Message == "startup_module_size", e.Message); }
        });
        test("startup aggregate budget charges even malformed modules", () =>
        {
            using var workspace = new Workspace(); var exe = workspace.Write("gta_sa.exe", Fixture());
            for (var i = 0; i < 4; i++)
            {
                var file = workspace.Write($"invalid{i}.dll", []);
                using var stream = File.OpenWrite(file); stream.SetLength(PeStartupInspector.MaxModuleBytes);
            }
            try { NativeStartupInspector.Inspect(exe); throw new InvalidOperationException("aggregate byte limit accepted"); }
            catch (InvalidDataException e) { Require(e.Message == "startup_total_byte_limit", e.Message); }
        });
        test("startup CLI returns diagnostic exit without an activation path", () =>
        {
            using var workspace = new Workspace(); var exe = workspace.Write("gta_sa.exe", Fixture());
            var saved = Console.Out; using var output = new StringWriter();
            try
            {
                Console.SetOut(output); Require(Saex.Tools.Program.Main(["engine", "startup", exe]) == 3, "startup exit");
                using var json = JsonDocument.Parse(output.ToString());
                Require(!json.RootElement.GetProperty("snapshot").GetProperty("canAdvanceToLoader").GetBoolean(), "CLI granted loader");
                workspace.Write("invalid.dll", new byte[128]); output.GetStringBuilder().Clear();
                Require(Saex.Tools.Program.Main(["engine", "startup", exe]) == 1, "incomplete CLI passed");
            }
            finally { Console.SetOut(saved); }
        });
        test("engine CLIs return structured input rejection for invalid metadata", () =>
        {
            using var workspace = new Workspace(); var exe = workspace.Write("gta_sa.exe", new byte[16]);
            var saved = Console.Out; using var output = new StringWriter();
            try
            {
                Console.SetOut(output);
                foreach (var command in new[] { "inspect", "startup" })
                {
                    output.GetStringBuilder().Clear();
                    Require(Saex.Tools.Program.Main(["engine", command, exe]) == 1, "invalid input escaped CLI");
                    using var json = JsonDocument.Parse(output.ToString());
                    Require(json.RootElement.GetProperty("error").GetString() == "input_rejected", "input error shape");
                }
            }
            finally { Console.SetOut(saved); }
        });
    }
}
