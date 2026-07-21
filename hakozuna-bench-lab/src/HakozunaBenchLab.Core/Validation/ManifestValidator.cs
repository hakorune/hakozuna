using HakozunaBenchLab.Core.Models;

namespace HakozunaBenchLab.Core.Validation;

public static class ManifestValidator
{
    public static IReadOnlyList<string> Validate(AllocatorManifest manifest)
    {
        ArgumentNullException.ThrowIfNull(manifest);
        var errors = new List<string>();
        if (string.IsNullOrWhiteSpace(manifest.Id)) errors.Add("Allocator id is required.");
        if (string.IsNullOrWhiteSpace(manifest.Label)) errors.Add("Allocator label is required.");
        if (!Enum.IsDefined(manifest.LaneKind)) errors.Add("Allocator lane kind is invalid.");
        if (string.IsNullOrWhiteSpace(manifest.ExecutablePath)) errors.Add("Allocator executable path is required.");
        if (string.IsNullOrWhiteSpace(manifest.Version)) errors.Add("Allocator version is required.");
        if (!string.IsNullOrWhiteSpace(manifest.ArtifactSha256) &&
            (manifest.ArtifactSha256.Length != 64 || !manifest.ArtifactSha256.All(Uri.IsHexDigit)))
            errors.Add("Artifact SHA-256 must contain 64 hexadecimal characters.");
        return errors;
    }

    public static IReadOnlyList<string> Validate(WorkloadPreset preset)
    {
        ArgumentNullException.ThrowIfNull(preset);
        var errors = new List<string>();
        if (string.IsNullOrWhiteSpace(preset.Id)) errors.Add("Workload id is required.");
        if (string.IsNullOrWhiteSpace(preset.Label)) errors.Add("Workload label is required.");
        if (string.IsNullOrWhiteSpace(preset.ExecutablePath)) errors.Add("Workload executable path is required.");
        if (string.IsNullOrWhiteSpace(preset.RunnerVersion)) errors.Add("Runner version is required.");
        if (!string.IsNullOrWhiteSpace(preset.RunnerSha256) &&
            (preset.RunnerSha256.Length != 64 || !preset.RunnerSha256.All(Uri.IsHexDigit)))
            errors.Add("Runner SHA-256 must contain 64 hexadecimal characters.");
        if (preset.DefaultRuns <= 0) errors.Add("Default run count must be positive.");
        return errors;
    }
}
