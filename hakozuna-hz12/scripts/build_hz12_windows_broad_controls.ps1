param(
    [string]$OutDirName = "out_win_broad"
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz12Root
$OutDir = Join-Path $Hz12Root $OutDirName
$RandomBench = Join-Path $RepoRoot "win\bench_random_mixed_compare.c"
$MixedBench = Join-Path $RepoRoot "win\bench_allocator_compare.c"
$XownerBench = Join-Path $RepoRoot "win\bench_xowner_pipeline.c"
$OwnerChurnBench = Join-Path $RepoRoot "win\bench_hz12_owner_churn.c"
$Sources = @(
    (Join-Path $Hz12Root "src\hz12_size_class.c"),
    (Join-Path $Hz12Root "src\hz12_sys_alloc.c"),
    (Join-Path $Hz12Root "src\hz12_thread_cache.c"),
    (Join-Path $Hz12Root "src\hz12_thread_cache_diag.c"),
    (Join-Path $Hz12Root "src\hz12_public_entry.c"),
    (Join-Path $Hz12Root "src\hz12_span.c"),
    (Join-Path $Hz12Root "src\hz12_live_footprint.c")
)
$Shadow = Join-Path $Hz12Root "src\hz12_shadow.c"
$Inbox = Join-Path $Hz12Root "src\hz12_inbox.c"
$FlushOwnerRoute = Join-Path $Hz12Root "src\hz12_flush_owner_route.c"
$Common = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/DHZ_BENCH_USE_HZ12=1", "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/I$(Join-Path $Hz12Root 'src')"
)

foreach ($path in @($RandomBench, $MixedBench) + $Sources) {
    if (-not (Test-Path $path)) { throw "Missing HZ12 broad source: $path" }
}
New-Item -ItemType Directory -Force $OutDir | Out-Null

function Invoke-Build([string]$Bench, [string]$Output) {
    $args = $Common + @($Bench) + $Sources + @(
        "psapi.lib", "/link", "/out:$Output"
    )
    & clang-cl @args
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

function Invoke-OwnerMapBuild([string]$Bench, [string]$Output) {
    $args = $Common + @(
        "/DHZ_BENCH_HZ12_OWNER_MAP_CONTROL=1",
        "/DHZ12_SHADOW_OWNER_FAST_LOAD=1",
        "/DHZ12_SHADOW_DIAG_COUNTERS=0",
        $Bench, $Shadow
    ) + $Sources + @(
        "psapi.lib", "/link", "/out:$Output"
    )
    & clang-cl @args
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

function Invoke-AllocMapBuild([string]$Bench, [string]$Output) {
    $args = $Common + @(
        "/DHZ_BENCH_HZ12_ALLOC_OWNER_MAP_CONTROL=1",
        "/DHZ12_SHADOW_OWNER_FAST_LOAD=1",
        "/DHZ12_SHADOW_DIAG_COUNTERS=0",
        $Bench, $Shadow
    ) + $Sources + @(
        "psapi.lib", "/link", "/out:$Output"
    )
    & clang-cl @args
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

function Invoke-FlushRouteBuild([string]$Bench, [string]$Output) {
    $args = $Common + @(
        "/DHZ12_FLUSH_OWNER_ROUTE=1",
        "/DHZ12_SHADOW_OWNER_FAST_LOAD=1",
        "/DHZ12_SHADOW_DIAG_COUNTERS=0",
        "/DHZ12_INBOX_DIAG_COUNTERS=0",
        "/DHZ12_INBOX_ACCOUNTING=0",
        $Bench, $Shadow, $FlushOwnerRoute
    ) + $Sources + @(
        "psapi.lib", "/link", "/out:$Output"
    )
    & clang-cl @args
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

function Invoke-FlushRouteInertBuild([string]$Bench, [string]$Output) {
    $args = $Common + @(
        "/DHZ12_FLUSH_OWNER_ROUTE=1",
        "/DHZ12_FLUSH_OWNER_ROUTE_INERT=1",
        "/DHZ12_SHADOW_OWNER_FAST_LOAD=1",
        "/DHZ12_SHADOW_DIAG_COUNTERS=0",
        "/DHZ12_INBOX_DIAG_COUNTERS=0",
        "/DHZ12_INBOX_ACCOUNTING=0",
        $Bench, $Shadow, $FlushOwnerRoute
    ) + $Sources + @(
        "psapi.lib", "/link", "/out:$Output"
    )
    & clang-cl @args
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

function Invoke-ColdSpanOwnerBuild([string]$Bench, [string]$Output) {
    $args = $Common + @(
        "/DHZ12_FLUSH_OWNER_ROUTE=1",
        "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
        "/DHZ12_SHADOW_OWNER_FAST_LOAD=1",
        "/DHZ12_SHADOW_DIAG_COUNTERS=0",
        $Bench, $Shadow, $FlushOwnerRoute
    ) + $Sources + @(
        "psapi.lib", "/link", "/out:$Output"
    )
    & clang-cl @args
    if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
}

Invoke-Build $RandomBench (Join-Path $OutDir "bench_random_mixed_hz12_core.exe")
Invoke-Build $MixedBench (Join-Path $OutDir "bench_mixed_ws_hz12_core.exe")
Invoke-OwnerMapBuild $RandomBench (Join-Path $OutDir "bench_random_mixed_hz12_ownermap.exe")
Invoke-OwnerMapBuild $MixedBench (Join-Path $OutDir "bench_mixed_ws_hz12_ownermap.exe")
Invoke-AllocMapBuild $RandomBench (Join-Path $OutDir "bench_random_mixed_hz12_allocmap.exe")
Invoke-AllocMapBuild $MixedBench (Join-Path $OutDir "bench_mixed_ws_hz12_allocmap.exe")
Invoke-FlushRouteBuild $RandomBench (Join-Path $OutDir "bench_random_mixed_hz12_flushroute.exe")
Invoke-FlushRouteBuild $MixedBench (Join-Path $OutDir "bench_mixed_ws_hz12_flushroute.exe")
Invoke-FlushRouteBuild $XownerBench (Join-Path $OutDir "bench_xowner_hz12_flushroute.exe")
Invoke-FlushRouteInertBuild $RandomBench (Join-Path $OutDir "bench_random_mixed_hz12_flushroute_inert.exe")
Invoke-FlushRouteInertBuild $MixedBench (Join-Path $OutDir "bench_mixed_ws_hz12_flushroute_inert.exe")
Invoke-ColdSpanOwnerBuild $RandomBench (Join-Path $OutDir "bench_random_mixed_hz12_coldspanowner.exe")
Invoke-ColdSpanOwnerBuild $MixedBench (Join-Path $OutDir "bench_mixed_ws_hz12_coldspanowner.exe")
Invoke-ColdSpanOwnerBuild $XownerBench (Join-Path $OutDir "bench_xowner_hz12_coldspanowner.exe")
Invoke-ColdSpanOwnerBuild $OwnerChurnBench (Join-Path $OutDir "hz12_owner_churn_smoke.exe")
Write-Host "Built HZ12 Windows broad controls in: $OutDir"
