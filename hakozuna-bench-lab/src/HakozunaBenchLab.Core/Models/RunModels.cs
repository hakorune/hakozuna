using System.Collections.Immutable;

namespace HakozunaBenchLab.Core.Models;

public enum LaneKind { Speed, Diagnostic, Research }
public enum RunMode { Preview, Verified }

public sealed record AllocatorManifest(
    string Id,
    string Label,
    LaneKind LaneKind,
    string ExecutablePath,
    string? ArtifactSha256 = null,
    string? WorkingDirectory = null);

public sealed record WorkloadPreset(
    string Id,
    string Label,
    string ExecutablePath,
    ImmutableArray<string> Arguments,
    int DefaultRuns = 1,
    string? WorkingDirectory = null);

public sealed record RunPlan(
    AllocatorManifest Allocator,
    WorkloadPreset Workload,
    RunMode Mode,
    int Runs,
    string Platform,
    string RunOrder = "ABBA")
{
    public static RunPlan Create(AllocatorManifest allocator, WorkloadPreset workload,
        RunMode mode, int runs, string platform = "windows-x64")
    {
        ArgumentNullException.ThrowIfNull(allocator);
        ArgumentNullException.ThrowIfNull(workload);
        if (runs <= 0) throw new ArgumentOutOfRangeException(nameof(runs));
        if (mode == RunMode.Verified && allocator.LaneKind != LaneKind.Speed)
            throw new InvalidOperationException("Diagnostic and research lanes cannot enter a Verified speed run.");
        return new RunPlan(allocator, workload, mode, runs, platform);
    }
}

public sealed record ProcessSample(
    int SampleIndex,
    DateTimeOffset StartedUtc,
    TimeSpan Duration,
    int ExitCode,
    bool TimedOut,
    string StandardOutput,
    string StandardError);

public sealed record RunResult(
    RunPlan Plan,
    ImmutableArray<ProcessSample> Samples,
    string? ArtifactSha256,
    DateTimeOffset CreatedUtc)
{
    public int FailureCount => Samples.Count(sample => sample.TimedOut || sample.ExitCode != 0);
}
