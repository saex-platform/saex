using System.Text.Json;

namespace Saex.Tools;

public static class Program
{
    public static int Main(string[] args)
    {
        var linkage = args.Length == 5 && args[0] == "engine" && args[1] == "linkage";
        if (!linkage && (args.Length != 3 || args[0] is not ("plan" or "engine") ||
            (args[0] == "plan" && args[1] != "validate") || (args[0] == "engine" && args[1] is not ("inspect" or "startup"))))
        {
            Console.Error.WriteLine("Usage: Saex.Tools plan validate <plan.json> | engine inspect|startup <gta_sa.exe> | engine linkage <consumer> <imported-module> <candidate.dll>");
            return 2;
        }
        try
        {
            if (linkage)
            {
                var report = PeLinkageInspector.Compare(args[2], args[3], args[4]);
                Print(report);
                return report.StaticSymbolsComplete ? 3 : 1;
            }
            if (args[0] == "plan")
            {
                var result = WorldPlanValidator.Validate(CheckedJson.ReadFile(args[2]));
                Print(result);
                return result.Valid ? 0 : 1;
            }
            if (args[1] == "startup")
            {
                var inventory = NativeStartupInspector.Inspect(args[2]);
                Print(inventory);
                return inventory.Snapshot.MetadataComplete ? 3 : 1;
            }
            var inspection = EngineInspector.Inspect(args[2]);
            Print(inspection);
            // Inspection success is not support. Exit 3 makes the unverified gate explicit.
            return inspection.CanAttach ? 0 : 3;
        }
        catch (Exception error) when (error is InvalidDataException or IOException or UnauthorizedAccessException or BadImageFormatException or ArgumentException or OverflowException)
        {
            Print(new { error = "input_rejected", detail = error.Message });
            return 1;
        }
    }

    private static void Print<T>(T value) => Console.WriteLine(JsonSerializer.Serialize(value,
        new JsonSerializerOptions { PropertyNamingPolicy = JsonNamingPolicy.CamelCase, WriteIndented = true }));
}
