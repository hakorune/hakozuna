using System.Collections;
using System.Collections.Immutable;

namespace HakozunaBenchLab.Core.Environment;

public static class EnvironmentPolicy
{
    private static readonly string[] RelevantPrefixes =
    [
        "H8_", "HAKOZUNA_", "BENCH_", "MIMALLOC_", "TCMALLOC_", "MALLOC_",
        "COMPlus_", "DOTNET_"
    ];

    private static readonly string[] RelevantNames =
    [
        "LD_PRELOAD", "DYLD_INSERT_LIBRARIES", "DYLD_LIBRARY_PATH",
        "PROCESSOR_IDENTIFIER", "NUMBER_OF_PROCESSORS", "OS"
    ];

    private static readonly string[] SensitiveParts =
    [
        "TOKEN", "SECRET", "PASSWORD", "PASSWD", "AUTH", "COOKIE",
        "CREDENTIAL", "PRIVATE_KEY", "API_KEY"
    ];

    public static ImmutableDictionary<string, string> MergeOverrides(
        params ImmutableDictionary<string, string>?[] sources)
    {
        var merged = ImmutableDictionary.CreateBuilder<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var source in sources)
        {
            if (source is null) continue;
            foreach (var pair in source) merged[pair.Key] = pair.Value;
        }
        return merged.ToImmutable();
    }

    public static ImmutableDictionary<string, string> CaptureForReport(
        IReadOnlyDictionary<string, string> overrides)
    {
        var captured = ImmutableDictionary.CreateBuilder<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (DictionaryEntry entry in System.Environment.GetEnvironmentVariables())
        {
            var name = entry.Key?.ToString();
            if (string.IsNullOrWhiteSpace(name) || !IsRelevant(name)) continue;
            captured[name] = Redact(name, entry.Value?.ToString() ?? string.Empty);
        }
        foreach (var pair in overrides) captured[pair.Key] = Redact(pair.Key, pair.Value);
        return captured.ToImmutable();
    }

    public static bool IsSensitive(string name) =>
        SensitiveParts.Any(part => name.Contains(part, StringComparison.OrdinalIgnoreCase));

    private static bool IsRelevant(string name) =>
        RelevantNames.Contains(name, StringComparer.OrdinalIgnoreCase) ||
        RelevantPrefixes.Any(prefix => name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase));

    private static string Redact(string name, string value) => IsSensitive(name) ? "<redacted>" : value;
}
