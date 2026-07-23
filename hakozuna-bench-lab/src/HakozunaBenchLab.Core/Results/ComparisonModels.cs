using System.Text.Json.Serialization;

namespace HakozunaBenchLab.Core.Results;

public sealed record ComparisonScorecard(
    [property: JsonPropertyName("schema")] string Schema,
    [property: JsonPropertyName("suite_names")] IReadOnlyList<string> SuiteNames,
    [property: JsonPropertyName("profile")] string Profile,
    [property: JsonPropertyName("generated_unix_ms")] long GeneratedUnixMs,
    [property: JsonPropertyName("recommended_allocator")] string? RecommendedAllocator,
    [property: JsonPropertyName("rows")] IReadOnlyList<ComparisonRow> Rows,
    [property: JsonPropertyName("allocator_summaries")] IReadOnlyList<AllocatorSummary> AllocatorSummaries);

public sealed record ComparisonRow(
    [property: JsonPropertyName("allocator")] string Allocator,
    [property: JsonPropertyName("workload")] string Workload,
    [property: JsonPropertyName("ops_per_sec")] double OpsPerSecond,
    [property: JsonPropertyName("peak_rss_bytes")] long PeakRssBytes,
    [property: JsonPropertyName("final_rss_bytes")] long FinalRssBytes,
    [property: JsonPropertyName("speed_score")] double SpeedScore,
    [property: JsonPropertyName("memory_score")] double? MemoryScore,
    [property: JsonPropertyName("profile_score")] double? ProfileScore,
    [property: JsonPropertyName("complete")] bool Complete);

public sealed record AllocatorSummary(
    [property: JsonPropertyName("allocator")] string Allocator,
    [property: JsonPropertyName("workload_count")] int WorkloadCount,
    [property: JsonPropertyName("complete_workload_count")] int CompleteWorkloadCount,
    [property: JsonPropertyName("average_speed_score")] double AverageSpeedScore,
    [property: JsonPropertyName("average_memory_score")] double? AverageMemoryScore,
    [property: JsonPropertyName("average_profile_score")] double? AverageProfileScore);
