using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Saex.Tools;

internal static class LinkageTests
{
    private const int Opt = 88, Table = 312;
    private static int Raw(uint rva) => checked((int)rva - 0x2000 + 1024);
    private static void U16(byte[] b, int at, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(at), value);
    private static void U32(byte[] b, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(at), value);
    private static void Field(byte[] b, uint rva, uint value) => U32(b, Raw(rva), value);
    private static void Text(byte[] b, uint rva, string value) => Encoding.ASCII.GetBytes(value + "\0").CopyTo(b, Raw(rva));
    private static void Dir(byte[] b, int index, uint rva, uint size)
    {
        U32(b, Opt + 96 + index * 8, rva); U32(b, Opt + 100 + index * 8, size);
    }
    private static byte[] Fixture(bool dll)
    {
        var b = new byte[1024 + 0x20000];
        U16(b, 0, 0x5a4d); U32(b, 60, 64); U32(b, 64, 0x4550); U16(b, 68, 0x14c); U16(b, 70, 2);
        U16(b, 84, 224); U16(b, 86, dll ? (ushort)0x2102 : (ushort)0x102);
        U16(b, Opt, 0x10b); U32(b, Opt + 16, 0x1000); U32(b, Opt + 28, 0x400000);
        U32(b, Opt + 32, 4096); U32(b, Opt + 36, 512); U32(b, Opt + 56, 0x22000); U32(b, Opt + 60, 512); U32(b, Opt + 92, 16);
        Encoding.ASCII.GetBytes(".text").CopyTo(b, Table);
        U32(b, Table + 8, 512); U32(b, Table + 12, 4096); U32(b, Table + 16, 512); U32(b, Table + 20, 512); U32(b, Table + 36, 0x60000020);
        Encoding.ASCII.GetBytes(".rdata").CopyTo(b, Table + 40);
        U32(b, Table + 48, 0x20000); U32(b, Table + 52, 8192); U32(b, Table + 56, 0x20000); U32(b, Table + 60, 1024); U32(b, Table + 76, 0x40000040);
        b[512] = 0xc3;
        if (dll)
        {
            Dir(b, 0, 0x10000, 0x1000);
            Field(b, 0x10010, 7); Field(b, 0x10014, 2); Field(b, 0x10018, 1);
            Field(b, 0x1001c, 0x10100); Field(b, 0x10020, 0x10200); Field(b, 0x10024, 0x10300);
            Field(b, 0x10100, 0x1000); Field(b, 0x10104, 0x1001);
            Field(b, 0x10200, 0x10400); Text(b, 0x10400, "Fn");
        }
        else
        {
            Dir(b, 1, 0x2000, 40); Field(b, 0x2000, 0x4000); Field(b, 0x200c, 0x2100); Field(b, 0x2010, 0x9000);
            Text(b, 0x2100, "TARGET.dll"); Text(b, 0x3002, "Fn");
            Field(b, 0x4000, 0x3000); Field(b, 0x9000, 0x3000);
        }
        return b;
    }
    private sealed class Workspace : IDisposable
    {
        public string Root { get; } = Path.Combine(Path.GetTempPath(), "saex linkage " + Guid.NewGuid().ToString("N"));
        public Workspace() => Directory.CreateDirectory(Root);
        public string Write(string name, byte[] bytes)
        {
            var path = Path.Combine(Root, name); File.WriteAllBytes(path, bytes); return path;
        }
        public void Dispose()
        {
            foreach (var file in Directory.EnumerateFiles(Root)) File.Delete(file);
            Directory.Delete(Root);
        }
    }
    private static void Require(bool ok, string why)
    {
        if (!ok) throw new InvalidOperationException(why);
    }
    private static NativeLinkageReport Compare(byte[] consumer, byte[] candidate, string module = "target.dll")
    {
        using var w = new Workspace();
        return PeLinkageInspector.Compare(w.Write("consumer.exe", consumer), module, w.Write("different-name.dll", candidate));
    }
    private static void Reject(byte[] bytes, string reason)
    {
        using var w = new Workspace(); var path = w.Write("fixture.bin", bytes);
        try { PeLinkageInspector.Inspect(path); throw new InvalidOperationException("accepted " + reason); }
        catch (InvalidDataException e) { Require(e.Message == reason, "expected " + reason + "; got " + e.Message); }
    }

    public static void Register(Action<string, Action> test)
    {
        test("linkage names and ordinal imports bind to an explicit candidate without runtime permission", () =>
        {
            var b = Fixture(false); Field(b, 0x4004, 0x80000008); Field(b, 0x9004, 0x80000008);
            var r = Compare(b, Fixture(true), "TARGET.DLL");
            Require(r.StaticSymbolsComplete && r.Bindings.Length == 2 && r.Bindings[0].Export!.Ordinal == 7 &&
                r.Bindings[1].Export is { Ordinal: 8 } && r.Candidate.Exports[1].Names.Length == 0 && r.Candidate.Name == "different-name.dll", "bindings");
            Require(!r.CanAttach && !r.CanInitialize && !r.RuntimeResolutionVerified && !r.CallingConventionVerified && !r.DynamicLoadsEnumerated, "authority");
        });
        test("linkage names are case sensitive and hints never substitute a missing name", () =>
        {
            var b = Fixture(false); Text(b, 0x3002, "fn");
            var r = Compare(b, Fixture(true));
            Require(!r.StaticSymbolsComplete && r.Bindings[0].Status == "missing_export", "case/hint fallback");
        });
        test("linkage ordinal and named holes remain missing exports", () =>
        {
            var d = Fixture(true); Field(d, 0x10100, 0);
            Require(Compare(Fixture(false), d).Bindings[0].Status == "missing_export", "named hole");
            var b = Fixture(false); Field(b, 0x4000, 0x80000007);
            Require(Compare(b, d).Bindings[0].Status == "missing_export", "ordinal hole");
            Field(b, 0x4000, 0x8000ffff);
            Require(Compare(b, Fixture(true)).Bindings[0].Status == "missing_export", "ordinal beyond EAT");
        });
        test("linkage sorted aliases may exceed the number of export slots", () =>
        {
            var d = Fixture(true); Field(d, 0x10014, 1); Field(d, 0x10018, 2);
            Field(d, 0x10204, 0x10480); Text(d, 0x10480, "Other");
            var b = Fixture(false); Text(b, 0x3002, "Other");
            var r = Compare(b, d);
            Require(r.StaticSymbolsComplete && r.Candidate.Exports[0].Names.SequenceEqual(["Fn", "Other"]), "aliases");
            using var json = JsonDocument.Parse(JsonSerializer.Serialize(r));
            Require(!json.RootElement.GetProperty("Bindings")[0].GetProperty("Export").TryGetProperty("Names", out _), "aliases multiplied per binding");
        });
        test("linkage RVA delay imports preserve their separate kind", () =>
        {
            var b = Fixture(false); Dir(b, 1, 0, 0); Dir(b, 13, 0x2000, 64); b.AsSpan(Raw(0x2000), 64).Clear();
            Field(b, 0x2000, 1); Field(b, 0x2004, 0x2100); Field(b, 0x200c, 0x9000); Field(b, 0x2010, 0x4000);
            var r = Compare(b, Fixture(true));
            Require(r.StaticSymbolsComplete && r.Bindings[0].Import.Kind == "delay", "delay import lost");
            Field(b, 0x2000, 0); Field(b, 0x2004, 0x402100);
            Reject(b, "linkage_legacy_delay_unsupported");
        });
        test("linkage unbound IAT fallback is allowed but bound IAT is not interpreted as names", () =>
        {
            var b = Fixture(false); Field(b, 0x2000, 0);
            Require(Compare(b, Fixture(true)).StaticSymbolsComplete, "unbound fallback");
            Field(b, 0x2004, 1); Reject(b, "linkage_bound_iat_without_lookup");
            Field(b, 0x2000, 0x4000); Field(b, 0x9000, 0x77770000);
            Require(Compare(b, Fixture(true)).StaticSymbolsComplete, "valid lookup ignored for bound IAT");
        });
        test("linkage rejects missing and out of image thunk tables", () =>
        {
            var b = Fixture(false); Field(b, 0x2010, 0); Reject(b, "linkage_missing_thunk_table");
            b = Fixture(false); Field(b, 0x2000, 0x22000); Reject(b, "startup_rva_bounds");
            b = Fixture(false); Field(b, 0x2010, 0x22000); Reject(b, "startup_rva_bounds");
        });
        test("linkage rejects ordinal reserved bits and ordinal zero", () =>
        {
            var b = Fixture(false); Field(b, 0x4000, 0x80010001); Reject(b, "linkage_ordinal_reserved_bits");
            Field(b, 0x4000, 0x80000000); Reject(b, "linkage_ordinal_zero");
        });
        test("linkage rejects empty non ASCII and unterminated symbol names", () =>
        {
            var b = Fixture(false); b[Raw(0x3002)] = 0; Reject(b, "linkage_empty_symbol");
            b[Raw(0x3002)] = 255; Reject(b, "linkage_symbol_ascii");
            b.AsSpan(Raw(0x3002), 512).Fill((byte)'a'); Reject(b, "linkage_symbol_length");
        });
        test("linkage bounds total imports and does not accept missing terminators", () =>
        {
            var b = Fixture(false);
            for (uint i = 0; i <= PeLinkageInspector.MaxSymbols; i++)
            {
                Field(b, 0x4000 + i * 4, 0x80000007); Field(b, 0x9000 + i * 4, 0x80000007);
            }
            Reject(b, "linkage_import_symbol_limit");
            Field(b, 0x4000 + 4096 * 4, 0); Field(b, 0x9000 + 4096 * 4, 0);
            Require(Compare(b, Fixture(true)).Bindings.Length == 4096, "exact limit rejected");
        });
        test("linkage import budget is shared by normal and delay descriptors", () =>
        {
            var b = Fixture(false);
            for (uint i = 0; i < 4096; i++) { Field(b, 0x4000 + i * 4, 0x80000007); Field(b, 0x9000 + i * 4, 0x80000007); }
            Dir(b, 13, 0xe000, 64); Field(b, 0xe000, 1); Field(b, 0xe004, 0x2100);
            Field(b, 0xe00c, 0xf100); Field(b, 0xe010, 0xf000); Field(b, 0xf000, 0x3000); Field(b, 0xf100, 0x3000);
            Reject(b, "linkage_import_symbol_limit");
        });
        test("linkage rejects premature or mismatched IAT terminators", () =>
        {
            var b = Fixture(false); Field(b, 0x9000, 0); Reject(b, "linkage_iat_early_termination");
            b = Fixture(false); Field(b, 0x9004, 1); Reject(b, "linkage_iat_termination");
        });
        test("linkage export counts indexes and ordinal arithmetic are bounded", () =>
        {
            var d = Fixture(true); Field(d, 0x10014, 4097); Reject(d, "linkage_export_symbol_limit");
            d = Fixture(true); Field(d, 0x10018, 4097); Reject(d, "linkage_export_symbol_limit");
            d = Fixture(true); U16(d, Raw(0x10300), 2); Reject(d, "linkage_export_name_index");
            d = Fixture(true); Field(d, 0x10010, uint.MaxValue); Reject(d, "linkage_export_ordinal_overflow");
            d = Fixture(true); Field(d, 0x10014, 0); Reject(d, "linkage_export_name_without_table");
        });
        test("linkage rejects unsorted and duplicate export names", () =>
        {
            foreach (var name in new[] { "AA", "Fn" })
            {
                var d = Fixture(true); Field(d, 0x10018, 2); Field(d, 0x10204, 0x10480); Text(d, 0x10480, name);
                Reject(d, "linkage_export_name_order");
            }
        });
        test("linkage validates export tables and target ranges", () =>
        {
            var d = Fixture(true); Field(d, 0x1001c, 0x21ffc); Reject(d, "startup_rva_bounds");
            d = Fixture(true); Field(d, 0x10100, 0x22000); Reject(d, "linkage_export_target_bounds");
            d = Fixture(true); Field(d, 0x10100, 0x100); Reject(d, "linkage_export_target_bounds");
        });
        test("linkage data exports and zero filled data remain metadata without call permission", () =>
        {
            var d = Fixture(true); Field(d, 0x10100, 0x21fff);
            U32(d, Table + 56, 0x1f000); d = d[..(1024 + 0x1f000)];
            var r = Compare(Fixture(false), d);
            Require(r.StaticSymbolsComplete && r.Bindings[0].Export!.Kind == "data_section" && !r.CallingConventionVerified, "data export");
        });
        test("linkage forwarders remain unresolved and cannot read past export directory", () =>
        {
            foreach (var name in new[] { "other.Fn", "other.#7" })
            {
                var d = Fixture(true); Field(d, 0x10100, 0x10800); Text(d, 0x10800, name);
                var r = Compare(Fixture(false), d);
                Require(!r.StaticSymbolsComplete && r.Bindings[0].Status == "forwarder_unresolved" && r.Bindings[0].Export!.Forwarder == name, "forwarder followed");
            }
            var broken = Fixture(true); Field(broken, 0x10100, 0x10fff); broken[Raw(0x10fff)] = (byte)'a';
            Reject(broken, "linkage_string_bounds");
        });
        test("linkage missing module and missing export directory cannot pass vacuously", () =>
        {
            Require(Compare(Fixture(false), Fixture(true), "unknown.dll").Issues.SequenceEqual(["requested_module_not_imported"]), "absent module");
            var d = Fixture(true); Dir(d, 0, 0, 0);
            Require(Compare(Fixture(false), d).Bindings[0].Status == "missing_export", "absent exports");
        });
        test("linkage rejects path shaped module names and executable candidates", () =>
        {
            foreach (var module in new[] { "../target.dll", "C:\\target.dll", "target.", "" })
            {
                try { Compare(Fixture(false), Fixture(true), module); throw new InvalidOperationException("unsafe module accepted"); }
                catch (InvalidDataException e) { Require(e.Message == "linkage_requested_module", e.Message); }
            }
            try { Compare(Fixture(false), Fixture(false)); throw new InvalidOperationException("exe candidate accepted"); }
            catch (InvalidDataException e) { Require(e.Message == "startup_native_x86_kind", e.Message); }
        });
        test("linkage retains native x86 and file bounds gates", () =>
        {
            var d = Fixture(true); U16(d, 68, 0x8664); Reject(d, "startup_native_x86_kind");
            Reject(Fixture(true)[..^1], "startup_section_bounds");
            using var w = new Workspace(); var path = w.Write("large.dll", []);
            using (var file = File.OpenWrite(path)) file.SetLength(PeStartupInspector.MaxModuleBytes + 1L);
            try { PeLinkageInspector.Inspect(path); throw new InvalidOperationException("large file accepted"); }
            catch (InvalidDataException e) { Require(e.Message == "startup_module_size", e.Message); }
        });
        test("linkage stable hashes use observed bytes and inspection preserves both files", () =>
        {
            using var w = new Workspace(); var b = Fixture(false); var d = Fixture(true);
            var exe = w.Write("consumer.exe", b); var dll = w.Write("target.dll", d);
            var first = PeLinkageInspector.Compare(exe, "target.dll", dll);
            var second = PeLinkageInspector.Compare(exe, "target.dll", dll);
            Require(first.Consumer.Sha256 == second.Consumer.Sha256 && first.Candidate.Sha256 == second.Candidate.Sha256 &&
                File.ReadAllBytes(exe).SequenceEqual(b) && File.ReadAllBytes(dll).SequenceEqual(d), "hash stability/file mutation");
            d[513] = 1; w.Write("target.dll", d);
            Require(PeLinkageInspector.Compare(exe, "target.dll", dll).Candidate.Sha256 != first.Candidate.Sha256, "changed bytes kept hash");
        });
        test("linkage CLI makes diagnostic success incomplete input and usage distinct", () =>
        {
            using var w = new Workspace(); var exe = w.Write("consumer.exe", Fixture(false)); var dll = w.Write("target.dll", Fixture(true));
            var saved = Console.Out; var savedError = Console.Error; using var output = new StringWriter();
            try
            {
                Console.SetOut(output); Console.SetError(output);
                Require(Saex.Tools.Program.Main(["engine", "linkage", exe, "target.dll", dll]) == 3, "success exit");
                using (var json = JsonDocument.Parse(output.ToString()))
                    Require(json.RootElement.GetProperty("staticSymbolsComplete").GetBoolean() && !json.RootElement.GetProperty("canInitialize").GetBoolean(), "CLI boundary");
                output.GetStringBuilder().Clear();
                Require(Saex.Tools.Program.Main(["engine", "linkage", exe, "missing.dll", dll]) == 1, "incomplete exit");
                w.Write("target.dll", []); output.GetStringBuilder().Clear();
                Require(Saex.Tools.Program.Main(["engine", "linkage", exe, "target.dll", dll]) == 1, "input exit");
                using (var json = JsonDocument.Parse(output.ToString())) Require(json.RootElement.GetProperty("error").GetString() == "input_rejected", "structured rejection");
                Require(Saex.Tools.Program.Main(["engine", "linkage", exe]) == 2, "usage exit");
            }
            finally { Console.SetOut(saved); Console.SetError(savedError); }
        });
    }
}
