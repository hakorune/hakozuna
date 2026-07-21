using HakozunaBenchLab.Core.Models;
using HakozunaBenchLab.Core.Validation;
using HakozunaBenchLab.Agent;
using HakozunaBenchLab.Core.Environment;
using System.Collections.Immutable;

namespace HakozunaBenchLab.Tests;

public sealed class RunPlanTests
{
    [Fact]
    public void VerifiedRejectsDiagnosticLane()
    {
        var allocator = new AllocatorManifest("diag", "Diagnostic", LaneKind.Diagnostic,
            "bench.exe", "1.0", ArtifactSha256: new string('0', 64));
        var workload = new WorkloadPreset("local", "Local", "bench.exe", [],
            "1.0", new string('1', 64));

        Assert.Throws<InvalidOperationException>(() =>
            RunPlan.Create(allocator, workload, RunMode.Verified, 1));
    }

    [Fact]
    public void ValidatorRejectsMalformedSha256()
    {
        var allocator = new AllocatorManifest("hz8", "HZ8", LaneKind.Speed,
            "bench.exe", "1.0", ArtifactSha256: "bad");

        Assert.Contains(ManifestValidator.Validate(allocator),
            message => message.Contains("SHA-256", StringComparison.Ordinal));
    }

    [Fact]
    public async Task AgentCapturesSuccessfulChildProcess()
    {
        var allocator = new AllocatorManifest("system", "System", LaneKind.Speed,
            "dotnet", "10.0", ArtifactSha256: new string('0', 64));
        var workload = new WorkloadPreset("version", "Version", "dotnet", ["--version"],
            "10.0", new string('1', 64));
        var plan = RunPlan.Create(allocator, workload, RunMode.Verified, 1);

        var result = await new BenchmarkAgent().RunAsync(
            plan,
            new AgentOptions(TimeSpan.FromSeconds(10)));

        Assert.Equal(0, result.FailureCount);
        Assert.Single(result.Samples);
        Assert.NotEmpty(result.Samples[0].StandardOutput);
    }

    [Fact]
    public void EnvironmentCaptureRedactsSecretsAndKeepsAllocatorSettings()
    {
        var overrides = ImmutableDictionary<string, string>.Empty
            .Add("H8_PROFILE", "balanced")
            .Add("H8_API_TOKEN", "do-not-store");

        var captured = EnvironmentPolicy.CaptureForReport(overrides);

        Assert.Equal("balanced", captured["H8_PROFILE"]);
        Assert.Equal("<redacted>", captured["H8_API_TOKEN"]);
    }
}
