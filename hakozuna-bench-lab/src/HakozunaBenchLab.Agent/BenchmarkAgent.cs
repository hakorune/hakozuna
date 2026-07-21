using System.Collections.Immutable;
using System.Diagnostics;
using HakozunaBenchLab.Core.Models;

namespace HakozunaBenchLab.Agent;

public sealed record AgentOptions(TimeSpan Timeout, bool CaptureOutput = true);

public sealed class BenchmarkAgent
{
    public async Task<RunResult> RunAsync(RunPlan plan, AgentOptions options,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(plan);
        ArgumentNullException.ThrowIfNull(options);
        if (options.Timeout <= TimeSpan.Zero) throw new ArgumentOutOfRangeException(nameof(options));

        var samples = ImmutableArray.CreateBuilder<ProcessSample>(plan.Runs);
        for (var index = 0; index < plan.Runs; index++)
            samples.Add(await RunOneAsync(plan, index + 1, options, cancellationToken));

        return new RunResult(plan, samples.ToImmutable(), plan.Allocator.ArtifactSha256, DateTimeOffset.UtcNow);
    }

    private static async Task<ProcessSample> RunOneAsync(RunPlan plan, int sampleIndex,
        AgentOptions options, CancellationToken cancellationToken)
    {
        var started = DateTimeOffset.UtcNow;
        var stopwatch = Stopwatch.StartNew();
        var startInfo = new ProcessStartInfo
        {
            FileName = plan.Workload.ExecutablePath,
            WorkingDirectory = plan.Workload.WorkingDirectory ?? Environment.CurrentDirectory,
            UseShellExecute = false,
            RedirectStandardOutput = options.CaptureOutput,
            RedirectStandardError = options.CaptureOutput,
            CreateNoWindow = true
        };
        foreach (var argument in plan.Workload.Arguments) startInfo.ArgumentList.Add(argument);

        using var process = new Process { StartInfo = startInfo, EnableRaisingEvents = true };
        process.Start();
        var stdoutTask = options.CaptureOutput ? process.StandardOutput.ReadToEndAsync(cancellationToken) : Task.FromResult(string.Empty);
        var stderrTask = options.CaptureOutput ? process.StandardError.ReadToEndAsync(cancellationToken) : Task.FromResult(string.Empty);
        var waitTask = process.WaitForExitAsync(cancellationToken);
        var timeoutTask = Task.Delay(options.Timeout, cancellationToken);
        var completed = await Task.WhenAny(waitTask, timeoutTask);
        var timedOut = completed == timeoutTask;
        if (timedOut) TryKillTree(process);
        await process.WaitForExitAsync(CancellationToken.None);
        var stdout = await stdoutTask;
        var stderr = await stderrTask;
        stopwatch.Stop();
        return new ProcessSample(sampleIndex, started, stopwatch.Elapsed,
            timedOut ? -1 : process.ExitCode, timedOut, stdout, stderr);
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
