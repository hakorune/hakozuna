using HakozunaBenchLab.Agent;

namespace HakozunaBenchLab.App;

internal static class RepositoryComparisonCatalog
{
    public static ComparisonRequest CreateRequest(string suite, string profile)
    {
        var root = FindBenchLabRoot()
            ?? throw new InvalidOperationException(
                "allocator-bench-lab was not found. Set HAKOZUNA_BENCHLAB_ROOT.");
        var executable = Environment.GetEnvironmentVariable("HAKOZUNA_BENCHLAB_EXE")
            ?? FindExecutable(root)
            ?? throw new FileNotFoundException(
                "benchlab.exe was not found. Build the allocator-bench-lab release binary first.");
        var output = Path.Combine(root, "results",
            $"gui-comparison-{DateTimeOffset.UtcNow:yyyyMMdd-HHmmss-fff}.json");
        return new ComparisonRequest(executable, root, suite, profile, output);
    }

    private static string? FindBenchLabRoot()
    {
        var candidates = new[]
        {
            Environment.GetEnvironmentVariable("HAKOZUNA_BENCHLAB_ROOT"),
            OperatingSystem.IsWindows() ? @"Z:\TextureVoice_local\git\allocator-bench-lab" : null,
            Environment.CurrentDirectory,
            AppContext.BaseDirectory
        };
        foreach (var candidate in candidates.Where(value => !string.IsNullOrWhiteSpace(value)))
        {
            var directory = new DirectoryInfo(candidate!);
            for (var depth = 0; directory is not null && depth < 8; depth++, directory = directory.Parent)
            {
                if (File.Exists(Path.Combine(directory.FullName, "Cargo.toml")) &&
                    Directory.Exists(Path.Combine(directory.FullName, "suites")))
                    return directory.FullName;
            }
        }
        return null;
    }

    private static string? FindExecutable(string root)
    {
        var candidates = OperatingSystem.IsWindows()
            ? new[] { Path.Combine(root, "target", "release", "benchlab.exe"), Path.Combine(root, "target", "debug", "benchlab.exe") }
            : new[] { Path.Combine(root, "target", "release", "benchlab"), Path.Combine(root, "target", "debug", "benchlab") };
        return candidates.FirstOrDefault(File.Exists);
    }
}
