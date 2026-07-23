using System.Diagnostics;
using System.Text;
using System.Text.Json;
using HakozunaBenchLab.Core.Results;

namespace HakozunaBenchLab.Agent;

public sealed record ComparisonRequest(
    string ExecutablePath,
    string WorkingDirectory,
    string Suite,
    string Profile,
    string OutputPath);

public sealed record ComparisonAgentOptions(TimeSpan Timeout);

public sealed class ComparisonAgent
{
    public async Task<ComparisonScorecard> RunAsync(
        ComparisonRequest request,
        ComparisonAgentOptions options,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(request);
        ArgumentNullException.ThrowIfNull(options);
        if (options.Timeout <= TimeSpan.Zero)
            throw new ArgumentOutOfRangeException(nameof(options));
        if (!File.Exists(request.ExecutablePath))
            throw new FileNotFoundException("benchlab CLI executable was not found.", request.ExecutablePath);

        var startInfo = new ProcessStartInfo
        {
            FileName = request.ExecutablePath,
            WorkingDirectory = request.WorkingDirectory,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true
        };
        foreach (var argument in new[]
        {
            "compare", "batch", "--suite", request.Suite, "--profile", request.Profile,
            "--output", request.OutputPath
        })
            startInfo.ArgumentList.Add(argument);

        using var process = new Process { StartInfo = startInfo };
        process.Start();
        var stdoutTask = process.StandardOutput.ReadToEndAsync(cancellationToken);
        var stderrTask = process.StandardError.ReadToEndAsync(cancellationToken);
        var waitTask = process.WaitForExitAsync(cancellationToken);
        var timeoutTask = Task.Delay(options.Timeout, cancellationToken);
        var completed = await Task.WhenAny(waitTask, timeoutTask);
        if (completed == timeoutTask)
        {
            TryKillTree(process);
            throw new TimeoutException($"Comparison timed out after {options.Timeout.TotalMinutes:F0} minutes.");
        }

        var stdout = await stdoutTask;
        var stderr = await stderrTask;
        if (process.ExitCode != 0)
        {
            var detail = string.IsNullOrWhiteSpace(stderr) ? stdout : stderr;
            throw new InvalidDataException($"Comparison CLI failed ({process.ExitCode}): {detail.Trim()}");
        }
        if (!File.Exists(request.OutputPath))
            throw new InvalidDataException("Comparison CLI completed without producing a scorecard.");

        await using var stream = File.OpenRead(request.OutputPath);
        var scorecard = await JsonSerializer.DeserializeAsync<ComparisonScorecard>(stream,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true }, cancellationToken);
        return scorecard ?? throw new InvalidDataException("Comparison scorecard was empty.");
    }

    private static void TryKillTree(Process process)
    {
        try
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
        }
        catch (InvalidOperationException)
        {
            // The process exited between the check and Kill.
        }
    }
}
