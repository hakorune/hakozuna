using System.Collections.Immutable;
using HakozunaBenchLab.Core.Models;

namespace HakozunaBenchLab.App;

internal static class RepositoryBenchCatalog
{
    public static RunPlan CreatePreviewPlan(string allocatorId, string workloadId)
    {
        var repositoryRoot = FindRepositoryRoot()
            ?? throw new InvalidOperationException(
                "Hakozuna sources were not found. Set HAKOZUNA_REPO_ROOT or import a provider pack.");
        var executableName = allocatorId switch
        {
            "system" => "bench_mixed_ws_crt.exe",
            "HZ8" => "bench_mixed_ws_hz8.exe",
            _ => throw new NotSupportedException(
                $"{allocatorId} is not installed yet. Import its provider pack from Providers.")
        };
        var executablePath = Path.Combine(repositoryRoot, "out_win_suite", executableName);
        if (!File.Exists(executablePath))
            throw new FileNotFoundException(
                $"Required {allocatorId} benchmark artifact is missing. Build the Windows allocator suite first.",
                executablePath);

        var arguments = workloadId switch
        {
            // Keep Preview short, but long enough for process-level peak RSS sampling.
            "Local Mixed" => ImmutableArray.Create("1", "2000000", "256", "16", "4096", "0"),
            "Medium Mixed" => ImmutableArray.Create("1", "1500000", "256", "4097", "8192", "0"),
            "Remote 90" => throw new NotSupportedException(
                "Remote 90 needs the dedicated MT remote runner adapter, which is not connected yet."),
            "RSS Turnover" => throw new NotSupportedException(
                "RSS Turnover needs its dedicated runner adapter, which is not connected yet."),
            _ => throw new NotSupportedException($"Unknown workload: {workloadId}")
        };

        var allocator = new AllocatorManifest(
            allocatorId.ToLowerInvariant(), allocatorId, LaneKind.Speed, executablePath,
            Version: "development", WorkingDirectory: Path.GetDirectoryName(executablePath));
        var workload = new WorkloadPreset(
            workloadId.ToLowerInvariant().Replace(' ', '-'), workloadId, executablePath,
            arguments, RunnerVersion: "bench_allocator_compare/development",
            WorkingDirectory: Path.GetDirectoryName(executablePath));
        return RunPlan.Create(allocator, workload, RunMode.Preview, runs: 1);
    }

    private static string? FindRepositoryRoot()
    {
        var candidates = new[]
        {
            System.Environment.GetEnvironmentVariable("HAKOZUNA_REPO_ROOT"),
            OperatingSystem.IsWindows() ? @"Y:\hakozuna_repo" : null,
            System.Environment.CurrentDirectory,
            AppContext.BaseDirectory
        };
        foreach (var candidate in candidates.Where(candidate => !string.IsNullOrWhiteSpace(candidate)))
        {
            var directory = new DirectoryInfo(candidate!);
            for (var depth = 0; directory is not null && depth < 8; depth++, directory = directory.Parent)
            {
                if (File.Exists(Path.Combine(directory.FullName, "win", "bench_allocator_compare.c")))
                    return directory.FullName;
            }
        }

        return null;
    }
}
