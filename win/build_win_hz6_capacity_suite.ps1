param(
    [string[]]$Families,
    [string[]]$Hz6Profiles,
    [string[]]$CapacityLanes,
    [string]$OutDirName,
    [switch]$DiagnosticHz6Probes
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutDirName) {
    $OutDirName = if ($DiagnosticHz6Probes) { "out_win_hz6_capacity_diag" } else { "out_win_hz6_capacity" }
}
$OutDir = Join-Path $RepoRoot $OutDirName
$ModernBuildCommon = Join-Path $PSScriptRoot "bench_app_like_allocator_build_common.ps1"

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Cc = "clang-cl"
if (-not (Get-Command $Cc -ErrorAction SilentlyContinue)) {
    throw "clang-cl not found in PATH."
}

. $ModernBuildCommon

function Split-List {
    param([string[]]$Values)
    $items = @()
    foreach ($value in @($Values)) {
        foreach ($part in @($value -split ',' | ForEach-Object { $_.Trim().ToLowerInvariant() } | Where-Object { $_ -ne "" })) {
            $items += $part
        }
    }
    $items
}

$selectedFamilies = Split-List -Values $Families
if (-not $selectedFamilies -or $selectedFamilies.Count -eq 0) {
    $selectedFamilies = @("mixed_ws", "random_mixed", "larson", "redis")
}

$Hz3Dir = Join-Path $RepoRoot "hakozuna"
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$BaseFlags = @(
    "/nologo",
    "/O2",
    "/DNDEBUG",
    "/std:c11",
    "/W3",
    "/MD",
    "/I$PSScriptRoot",
    "/I$Hz3Dir\include",
    "/I$Hz3Dir\win\include",
    "/I$Hz4Dir\core",
    "/I$Hz4Dir\include",
    "/I$Hz4Dir\win"
)

$familyMap = @{
    "mixed_ws" = @{
        Source = (Join-Path $RepoRoot "win\bench_allocator_compare.c")
        Prefix = "bench_mixed_ws"
        ExtraFlags = @("/DHZ_BENCH_DISABLE_REALLOC=1", "psapi.lib")
    }
    "random_mixed" = @{
        Source = (Join-Path $RepoRoot "win\bench_random_mixed_compare.c")
        Prefix = "bench_random_mixed"
        ExtraFlags = @()
    }
    "larson" = @{
        Source = (Join-Path $RepoRoot "win\bench_larson_compare.c")
        Prefix = "bench_larson"
        ExtraFlags = @()
    }
    "redis" = @{
        Source = (Join-Path $RepoRoot "win\bench_redis_workload_compare.c")
        Prefix = "bench_redis_workload"
        ExtraFlags = @()
    }
}

foreach ($family in $selectedFamilies) {
    if (-not $familyMap.ContainsKey($family)) {
        throw "Unknown benchmark family: $family"
    }
    $config = $familyMap[$family]
    Write-Host "[hz6-only] building family=$family"
    Invoke-AppLikeHz6BenchBuilds `
        -Compiler $Cc `
        -BaseFlags ($BaseFlags + $config.ExtraFlags) `
        -RepoRoot $RepoRoot `
        -BenchSrc $config.Source `
        -OutDir $OutDir `
        -OutputPrefix $config.Prefix `
        -Hz6Profiles $Hz6Profiles `
        -CapacityLanes $CapacityLanes `
        -IncludeControlCapacity `
        -DiagnosticHz6Probes:$DiagnosticHz6Probes
}

Write-Host "Built HZ6-only capacity artifacts in: $OutDir"
