using System.Text.Json;
using System.Text.Json.Serialization;

namespace HakozunaBenchLab.Core.Providers;

public static class ProviderManifestSerializer
{
    private static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.Web)
    {
        AllowTrailingCommas = false,
        PropertyNameCaseInsensitive = false,
        WriteIndented = true,
        Converters = { new JsonStringEnumConverter(JsonNamingPolicy.CamelCase) }
    };

    public static async Task<ProviderManifest> LoadAsync(
        string path,
        CancellationToken cancellationToken = default)
    {
        await using var stream = File.OpenRead(path);
        return await JsonSerializer.DeserializeAsync<ProviderManifest>(stream, Options, cancellationToken)
            ?? throw new InvalidDataException("Provider manifest is empty.");
    }

    public static string Serialize(ProviderManifest manifest) =>
        JsonSerializer.Serialize(manifest, Options);
}
