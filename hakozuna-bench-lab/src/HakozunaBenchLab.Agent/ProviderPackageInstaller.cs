using System.IO.Compression;
using System.Security.Cryptography;
using HakozunaBenchLab.Core.Providers;

namespace HakozunaBenchLab.Agent;

public sealed record ProviderInstallOptions(
    string Platform,
    string Architecture,
    long MaxUncompressedBytes = 2L * 1024 * 1024 * 1024,
    int MaxEntries = 4096);

public sealed class ProviderPackageInstaller
{
    public async Task<InstalledProvider> InstallAsync(
        string packagePath,
        string providerRoot,
        ProviderInstallOptions options,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(packagePath);
        ArgumentException.ThrowIfNullOrWhiteSpace(providerRoot);
        ArgumentNullException.ThrowIfNull(options);
        if (!File.Exists(packagePath)) throw new FileNotFoundException("Provider package was not found.", packagePath);
        if (options.MaxEntries <= 0 || options.MaxUncompressedBytes <= 0)
            throw new ArgumentOutOfRangeException(nameof(options));

        Directory.CreateDirectory(providerRoot);
        var staging = Path.Combine(providerRoot, $".staging-{Guid.NewGuid():N}");
        Directory.CreateDirectory(staging);
        var installed = false;

        try
        {
            await ExtractCheckedAsync(packagePath, staging, options, cancellationToken);
            var manifestPath = Path.Combine(staging, "provider.json");
            if (!File.Exists(manifestPath)) throw new InvalidDataException("Provider package has no root provider.json.");

            var manifest = await ProviderManifestSerializer.LoadAsync(manifestPath, cancellationToken);
            var errors = ProviderValidator.Validate(manifest);
            if (errors.Count > 0) throw new InvalidDataException(string.Join(Environment.NewLine, errors));
            if (!string.Equals(manifest.Platform, options.Platform, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(manifest.Architecture, options.Architecture, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException(
                    $"Provider targets {manifest.Platform}-{manifest.Architecture}, expected {options.Platform}-{options.Architecture}.");

            foreach (var artifact in manifest.Artifacts)
            {
                var artifactPath = ResolveInside(staging, artifact.RelativePath);
                if (!File.Exists(artifactPath)) throw new InvalidDataException($"Provider artifact is missing: {artifact.RelativePath}");
                var actualHash = await ComputeSha256Async(artifactPath, cancellationToken);
                if (!string.Equals(actualHash, artifact.Sha256, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException($"Provider artifact hash mismatch: {artifact.RelativePath}");
            }

            var destination = Path.Combine(providerRoot, manifest.Id, manifest.Version);
            if (Directory.Exists(destination))
                throw new IOException($"Provider version is already installed: {manifest.Id}/{manifest.Version}");
            Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
            Directory.Move(staging, destination);
            installed = true;
            return new InstalledProvider(manifest, destination);
        }
        finally
        {
            if (!installed) TryDeleteDirectory(staging);
        }
    }

    private static async Task ExtractCheckedAsync(
        string packagePath,
        string staging,
        ProviderInstallOptions options,
        CancellationToken cancellationToken)
    {
        using var archive = ZipFile.OpenRead(packagePath);
        if (archive.Entries.Count > options.MaxEntries) throw new InvalidDataException("Provider package has too many entries.");
        long totalBytes = 0;
        foreach (var entry in archive.Entries)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (IsSymbolicLink(entry)) throw new InvalidDataException($"Provider package contains a symbolic link: {entry.FullName}");
            totalBytes = checked(totalBytes + entry.Length);
            if (totalBytes > options.MaxUncompressedBytes) throw new InvalidDataException("Provider package exceeds the extraction size limit.");

            var outputPath = ResolveInside(staging, entry.FullName);
            if (string.IsNullOrEmpty(entry.Name))
            {
                Directory.CreateDirectory(outputPath);
                continue;
            }

            Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
            await using var input = entry.Open();
            await using var output = new FileStream(outputPath, FileMode.CreateNew, FileAccess.Write, FileShare.None);
            await input.CopyToAsync(output, cancellationToken);
        }
    }

    private static string ResolveInside(string root, string relativePath)
    {
        if (!ProviderValidator.IsSafeRelativePath(relativePath))
            throw new InvalidDataException($"Unsafe provider package path: {relativePath}");
        var fullRoot = Path.GetFullPath(root) + Path.DirectorySeparatorChar;
        var fullPath = Path.GetFullPath(Path.Combine(root, relativePath));
        if (!fullPath.StartsWith(fullRoot, OperatingSystem.IsWindows()
                ? StringComparison.OrdinalIgnoreCase
                : StringComparison.Ordinal))
            throw new InvalidDataException($"Provider path escapes the package root: {relativePath}");
        return fullPath;
    }

    private static bool IsSymbolicLink(ZipArchiveEntry entry) =>
        ((entry.ExternalAttributes >> 16) & 0xF000) == 0xA000;

    private static async Task<string> ComputeSha256Async(string path, CancellationToken cancellationToken)
    {
        await using var stream = File.OpenRead(path);
        var hash = await SHA256.HashDataAsync(stream, cancellationToken);
        return Convert.ToHexString(hash).ToLowerInvariant();
    }

    private static void TryDeleteDirectory(string path)
    {
        try
        {
            if (Directory.Exists(path)) Directory.Delete(path, recursive: true);
        }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
    }
}
