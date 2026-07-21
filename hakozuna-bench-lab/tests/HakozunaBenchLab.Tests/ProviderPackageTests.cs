using System.Collections.Immutable;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using HakozunaBenchLab.Agent;
using HakozunaBenchLab.Core.Models;
using HakozunaBenchLab.Core.Providers;

namespace HakozunaBenchLab.Tests;

public sealed class ProviderPackageTests
{
    [Fact]
    public async Task InstallerVerifiesAndInstallsVersionSideBySide()
    {
        var root = CreateTestDirectory();
        try
        {
            var package = CreateValidPackage(root);
            var providerRoot = Path.Combine(root, "providers");

            var installed = await new ProviderPackageInstaller().InstallAsync(
                package, providerRoot, new ProviderInstallOptions("windows", "x64"));

            Assert.Equal("demo", installed.Manifest.Id);
            Assert.Equal("1.0.0", installed.Manifest.Version);
            Assert.True(File.Exists(Path.Combine(installed.InstallDirectory, "artifacts", "bench.exe")));
            await Assert.ThrowsAsync<IOException>(() => new ProviderPackageInstaller().InstallAsync(
                package, providerRoot, new ProviderInstallOptions("windows", "x64")));
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public async Task InstallerRejectsPathTraversal()
    {
        var root = CreateTestDirectory();
        try
        {
            var package = Path.Combine(root, "unsafe.hbl-provider.zip");
            using (var archive = ZipFile.Open(package, ZipArchiveMode.Create))
            {
                var entry = archive.CreateEntry("../escape.txt");
                await using var writer = new StreamWriter(entry.Open());
                await writer.WriteAsync("escape");
            }

            await Assert.ThrowsAsync<InvalidDataException>(() => new ProviderPackageInstaller().InstallAsync(
                package, Path.Combine(root, "providers"), new ProviderInstallOptions("windows", "x64")));
            Assert.False(File.Exists(Path.Combine(root, "escape.txt")));
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public async Task InstallerRejectsArtifactHashMismatch()
    {
        var root = CreateTestDirectory();
        try
        {
            var package = CreateValidPackage(root, declaredHash: new string('0', 64));

            var error = await Assert.ThrowsAsync<InvalidDataException>(() =>
                new ProviderPackageInstaller().InstallAsync(
                    package, Path.Combine(root, "providers"), new ProviderInstallOptions("windows", "x64")));

            Assert.Contains("hash mismatch", error.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public async Task InstallerRejectsWrongPlatform()
    {
        var root = CreateTestDirectory();
        try
        {
            var package = CreateValidPackage(root);

            var error = await Assert.ThrowsAsync<InvalidDataException>(() =>
                new ProviderPackageInstaller().InstallAsync(
                    package, Path.Combine(root, "providers"), new ProviderInstallOptions("macos", "arm64")));

            Assert.Contains("expected macos-arm64", error.Message, StringComparison.OrdinalIgnoreCase);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    private static string CreateValidPackage(string root, string? declaredHash = null)
    {
        var package = Path.Combine(root, "demo.hbl-provider.zip");
        var artifactBytes = Encoding.UTF8.GetBytes("benchmark artifact");
        var artifactHash = Convert.ToHexString(SHA256.HashData(artifactBytes)).ToLowerInvariant();
        var manifest = new ProviderManifest(
            1, "demo", "Demo Allocator", "1.0.0", "windows", "x64", LaneKind.Speed,
            [new ProviderArtifact("local", ProviderArtifactKind.BenchmarkExecutable,
                "artifacts/bench.exe", declaredHash ?? artifactHash, "local-mixed")],
            [new ProviderEnvironmentProfile("default", "Default",
                ImmutableDictionary<string, string>.Empty.Add("DEMO_PROFILE", "default"))]);

        using var archive = ZipFile.Open(package, ZipArchiveMode.Create);
        var manifestEntry = archive.CreateEntry("provider.json");
        using (var writer = new StreamWriter(manifestEntry.Open()))
            writer.Write(ProviderManifestSerializer.Serialize(manifest));
        var artifactEntry = archive.CreateEntry("artifacts/bench.exe");
        using (var output = artifactEntry.Open()) output.Write(artifactBytes);
        return package;
    }

    private static string CreateTestDirectory()
    {
        var path = Path.Combine(Path.GetTempPath(), $"hbl-provider-test-{Guid.NewGuid():N}");
        Directory.CreateDirectory(path);
        return path;
    }
}
