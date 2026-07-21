namespace HakozunaBenchLab.Agent;

internal sealed class ExecutableStager : IDisposable
{
    private readonly string _directory;

    private ExecutableStager(string directory, string executablePath)
    {
        _directory = directory;
        ExecutablePath = executablePath;
    }

    public string ExecutablePath { get; }
    public string WorkingDirectory => _directory;

    public static ExecutableStager Create(string sourceExecutable)
    {
        if (!File.Exists(sourceExecutable))
            throw new FileNotFoundException("Benchmark executable was not found.", sourceExecutable);

        var sourceDirectory = Path.GetDirectoryName(sourceExecutable)
            ?? throw new InvalidDataException("Benchmark executable has no parent directory.");
        var stageDirectory = Path.Combine(Path.GetTempPath(),
            $"hakozuna-bench-lab-run-{Guid.NewGuid():N}");
        Directory.CreateDirectory(stageDirectory);

        try
        {
            // Stage direct sidecars so a network or protected source directory never executes in place.
            foreach (var sourcePath in Directory.EnumerateFiles(sourceDirectory))
            {
                var extension = Path.GetExtension(sourcePath);
                if (!string.Equals(sourcePath, sourceExecutable, StringComparison.OrdinalIgnoreCase) &&
                    !string.Equals(extension, ".dll", StringComparison.OrdinalIgnoreCase) &&
                    !string.Equals(extension, ".json", StringComparison.OrdinalIgnoreCase) &&
                    !string.Equals(extension, ".config", StringComparison.OrdinalIgnoreCase) &&
                    !string.Equals(extension, ".manifest", StringComparison.OrdinalIgnoreCase))
                    continue;
                File.Copy(sourcePath, Path.Combine(stageDirectory, Path.GetFileName(sourcePath)), overwrite: false);
            }

            return new ExecutableStager(stageDirectory,
                Path.Combine(stageDirectory, Path.GetFileName(sourceExecutable)));
        }
        catch
        {
            TryDelete(stageDirectory);
            throw;
        }
    }

    public void Dispose() => TryDelete(_directory);

    private static void TryDelete(string directory)
    {
        try
        {
            if (Directory.Exists(directory)) Directory.Delete(directory, recursive: true);
        }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
    }
}
