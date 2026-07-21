using System.Collections.Immutable;
using HakozunaBenchLab.Core.Models;

namespace HakozunaBenchLab.Core.Providers;

public enum ProviderArtifactKind
{
    BenchmarkExecutable,
    AllocatorLibrary,
    Auxiliary
}

public sealed record ProviderArtifact(
    string Id,
    ProviderArtifactKind Kind,
    string RelativePath,
    string Sha256,
    string? WorkloadId = null);

public sealed record ProviderEnvironmentProfile(
    string Id,
    string Label,
    ImmutableDictionary<string, string> Variables);

public sealed record ProviderManifest(
    int SchemaVersion,
    string Id,
    string Label,
    string Version,
    string Platform,
    string Architecture,
    LaneKind LaneKind,
    ImmutableArray<ProviderArtifact> Artifacts,
    ImmutableArray<ProviderEnvironmentProfile> EnvironmentProfiles,
    string? BuildCommit = null,
    string? Description = null);

public sealed record InstalledProvider(ProviderManifest Manifest, string InstallDirectory);
