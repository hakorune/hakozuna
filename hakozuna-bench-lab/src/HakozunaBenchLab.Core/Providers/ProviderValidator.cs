using System.Text.RegularExpressions;

namespace HakozunaBenchLab.Core.Providers;

public static partial class ProviderValidator
{
    public static IReadOnlyList<string> Validate(ProviderManifest manifest)
    {
        ArgumentNullException.ThrowIfNull(manifest);
        var errors = new List<string>();
        if (manifest.SchemaVersion != 1) errors.Add("Provider schemaVersion must be 1.");
        if (!IsSafeSegment(manifest.Id)) errors.Add("Provider id is not a safe path segment.");
        if (!IsSafeSegment(manifest.Version)) errors.Add("Provider version is not a safe path segment.");
        if (string.IsNullOrWhiteSpace(manifest.Label)) errors.Add("Provider label is required.");
        if (string.IsNullOrWhiteSpace(manifest.Platform)) errors.Add("Provider platform is required.");
        if (string.IsNullOrWhiteSpace(manifest.Architecture)) errors.Add("Provider architecture is required.");
        if (!Enum.IsDefined(manifest.LaneKind)) errors.Add("Provider lane kind is invalid.");
        if (manifest.Artifacts.IsDefaultOrEmpty) errors.Add("At least one provider artifact is required.");

        var artifactIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var artifact in manifest.Artifacts)
        {
            if (!IsSafeSegment(artifact.Id)) errors.Add($"Artifact id '{artifact.Id}' is invalid.");
            if (!artifactIds.Add(artifact.Id)) errors.Add($"Artifact id '{artifact.Id}' is duplicated.");
            if (!IsSafeRelativePath(artifact.RelativePath))
                errors.Add($"Artifact path '{artifact.RelativePath}' is unsafe.");
            if (!IsSha256(artifact.Sha256)) errors.Add($"Artifact '{artifact.Id}' has an invalid SHA-256.");
            if (artifact.Kind == ProviderArtifactKind.BenchmarkExecutable && string.IsNullOrWhiteSpace(artifact.WorkloadId))
                errors.Add($"Benchmark artifact '{artifact.Id}' requires workloadId.");
        }

        var profileIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var profile in manifest.EnvironmentProfiles)
        {
            if (!IsSafeSegment(profile.Id)) errors.Add($"Environment profile id '{profile.Id}' is invalid.");
            if (!profileIds.Add(profile.Id)) errors.Add($"Environment profile id '{profile.Id}' is duplicated.");
            if (string.IsNullOrWhiteSpace(profile.Label)) errors.Add($"Environment profile '{profile.Id}' requires a label.");
            foreach (var name in profile.Variables.Keys)
                if (!EnvironmentNameRegex().IsMatch(name)) errors.Add($"Environment variable name '{name}' is invalid.");
        }

        return errors;
    }

    public static bool IsSafeRelativePath(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || Path.IsPathRooted(path) || path.Contains(':')) return false;
        var segments = path.Replace('\\', '/').Split('/', StringSplitOptions.RemoveEmptyEntries);
        return segments.Length > 0 && segments.All(segment => segment is not "." and not "..");
    }

    private static bool IsSafeSegment(string value) =>
        !string.IsNullOrWhiteSpace(value) && SafeSegmentRegex().IsMatch(value);

    private static bool IsSha256(string value) => value.Length == 64 && value.All(Uri.IsHexDigit);

    [GeneratedRegex("^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$", RegexOptions.CultureInvariant)]
    private static partial Regex SafeSegmentRegex();

    [GeneratedRegex("^[A-Za-z_][A-Za-z0-9_]{0,127}$", RegexOptions.CultureInvariant)]
    private static partial Regex EnvironmentNameRegex();
}
