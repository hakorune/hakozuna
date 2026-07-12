param()

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_redis"
$Hz8Dir = Join-Path $RepoRoot "hakozuna-hz8"
$BenchSrc = Join-Path $RepoRoot "win\bench_redis_workload_compare.c"
$Cc = Get-Command "clang-cl" -ErrorAction Stop

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Sources = Get-ChildItem (Join-Path $Hz8Dir "src") -Filter "*.c" |
    ForEach-Object { $_.FullName }
$Flags = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$PSScriptRoot", "/I$Hz8Dir\include", "/I$Hz8Dir\src",
    "/DHZ_BENCH_USE_HZ8=1",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1"
)

foreach ($variant in @(
    @{ Name = "hz8-v2"; Output = "bench_redis_workload_hz8_v2.exe"; ExtraFlags = @() },
    @{
        Name = "hz8-r3-page8k-integrated"
        Output = "bench_redis_workload_hz8_page8k_r3.exe"
        ExtraFlags = @(
            "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
            "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
            "/DHZ_BENCH_DISABLE_REALLOC=1"
        )
    },
    @{
        Name = "hz8-r3-page8k-target-dispatch"
        Output = "bench_redis_workload_hz8_page8k_target_dispatch.exe"
        ExtraFlags = @(
            "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
            "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
            "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
            "/DHZ_BENCH_DISABLE_REALLOC=1"
        )
    }
)) {
    $output = Join-Path $OutDir $variant.Output
    Write-Host "Building: Redis-like R3 gate ($($variant.Name))"
    & $Cc.Source @Flags @($variant.ExtraFlags) @Sources $BenchSrc "/Fe:$output"
    if ($LASTEXITCODE -ne 0) {
        throw "HZ8 Redis-like build failed for $($variant.Name): $LASTEXITCODE"
    }
}

Write-Host "Built focused HZ8 Redis-like gate artifacts in: $OutDir"
