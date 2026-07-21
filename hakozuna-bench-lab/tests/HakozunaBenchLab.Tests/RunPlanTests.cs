using HakozunaBenchLab.Core.Models;
using HakozunaBenchLab.Core.Validation;
using HakozunaBenchLab.Agent;

namespace HakozunaBenchLab.Tests;

public sealed class RunPlanTests
{
    [Fact]
    public void VerifiedRejectsDiagnosticLane()
    {
        var allocator = new AllocatorManifest("diag", "Diagnostic", LaneKind.Diagnostic, "bench.exe");
        var workload = new WorkloadPreset("local", "Local", "bench.exe", []);

        Assert.Throws<InvalidOperationException>(() =>
            RunPlan.Create(allocator, workload, RunMode.Verified, 1));
    }

    [Fact]
    public void ValidatorRejectsMalformedSha256()
    {
        var allocator = new AllocatorManifest("hz8", "HZ8", LaneKind.Speed, "bench.exe", "bad");

        Assert.Contains(ManifestValidator.Validate(allocator),
            message => message.Contains("SHA-256", StringComparison.Ordinal));
    }

    [Fact]
    public async Task AgentCapturesSuccessfulChildProcess()
    {
        var allocator = new AllocatorManifest("system", "System", LaneKind.Speed, "dotnet");
        var workload = new WorkloadPreset("version", "Version", "dotnet", ["--version"]);
        var plan = RunPlan.Create(allocator, workload, RunMode.Verified, 1);

        var result = await new BenchmarkAgent().RunAsync(
            plan,
            new AgentOptions(TimeSpan.FromSeconds(10)));

        Assert.Equal(0, result.FailureCount);
        Assert.Single(result.Samples);
        Assert.NotEmpty(result.Samples[0].StandardOutput);
    }
}
