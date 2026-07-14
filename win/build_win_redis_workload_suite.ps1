param(
    [string]$VcpkgRoot,
    [switch]$IncludeHz8Research,
    [string[]]$RequestedHz8Variants
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_redis"
$ObjDir = Join-Path $OutDir "obj"
$SuiteBuild = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"
$ModernBuildCommon = Join-Path $PSScriptRoot "bench_app_like_allocator_build_common.ps1"

New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force $ObjDir | Out-Null

$Cc = "clang-cl"
if (-not (Get-Command $Cc -ErrorAction SilentlyContinue)) {
    throw "clang-cl not found in PATH."
}

function Invoke-Checked {
    param(
        [string]$Exe,
        [string[]]$ArgList
    )
    & $Exe @ArgList | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "$Exe failed with exit code $LASTEXITCODE"
    }
}

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}

. $ModernBuildCommon

if (-not $RequestedHz8Variants) {
    & $SuiteBuild -IncludeHz8Research:$IncludeHz8Research
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

$VcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"

$BenchSrc = Join-Path $RepoRoot "win\bench_redis_workload_compare.c"
$BenchHz6Diag = Join-Path $RepoRoot "win\bench_redis_hz6_diag.c"
$Hz3Dir = Join-Path $RepoRoot "hakozuna"
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$Hz8Dir = Join-Path $RepoRoot "hakozuna-hz8"
$Hz3Lib = Join-Path $Hz3Dir "out_win\hz3_win.lib"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\hz4_win.lib"
$Hz8Sources = Get-ChildItem (Join-Path $Hz8Dir "src") -Filter "*.c" |
    ForEach-Object { $_.FullName }

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

Write-Host "Building: redis workload (CRT baseline)"
$BenchCrtOut = Join-Path $OutDir "bench_redis_workload_crt.exe"
Invoke-Checked $Cc ($BaseFlags + @($BenchSrc, "/link", "/out:$BenchCrtOut"))

if (Test-Path $Hz3Lib) {
    Write-Host "Building: redis workload (hz3)"
    $BenchHz3Out = Join-Path $OutDir "bench_redis_workload_hz3.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ3=1", $BenchSrc, $BenchHz6Diag, $Hz3Lib, "/link", "/out:$BenchHz3Out"))
} else {
    Write-Warning "hz3_win.lib not found; skipping hz3 redis workload bench."
}

$Hz4ApiObj = Join-Path $ObjDir "hz4_win_api_redis.obj"
$Hz4Flags = $BaseFlags + @(
    "/D__thread=__declspec(thread)",
    "/DHZ4_TLS_DIRECT=0",
    "/DHZ4_PAGE_META_SEPARATE=0",
    "/DHZ4_RSSRETURN=0",
    "/DHZ4_MID_PAGE_SUPPLY_RESV_BOX=0",
    "/DHZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1",
    "/DHZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1"
)
if ((Test-Path $Hz4Lib) -and (Test-Path (Join-Path $Hz4Dir "win\hz4_win_api.c"))) {
    Invoke-Checked $Cc ($Hz4Flags + @("/c", (Join-Path $Hz4Dir "win\hz4_win_api.c"), "/Fo$Hz4ApiObj"))
    Write-Host "Building: redis workload (hz4)"
    $BenchHz4Out = Join-Path $OutDir "bench_redis_workload_hz4.exe"
    Invoke-Checked $Cc ($Hz4Flags + @("/DHZ_BENCH_USE_HZ4=1", $BenchSrc, $BenchHz6Diag, $Hz4ApiObj, $Hz4Lib, "/link", "/out:$BenchHz4Out"))
} else {
    Write-Warning "hz4_win.lib not found; skipping hz4 redis workload bench."
}

$MiHeader = Join-Path $VcpkgInclude "mimalloc.h"
$MiLib = Join-Path $VcpkgLib "mimalloc.dll.lib"
if ((Test-Path $MiHeader) -and (Test-Path $MiLib)) {
    Write-Host "Building: redis workload (mimalloc)"
    $BenchMiOut = Join-Path $OutDir "bench_redis_workload_mimalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_MIMALLOC=1", $BenchSrc, $BenchHz6Diag, $MiLib, "/link", "/out:$BenchMiOut"))
} else {
    Write-Warning "mimalloc not found in $VcpkgRoot; skipping mimalloc redis workload bench."
}

$TcHeader = Join-Path $VcpkgInclude "gperftools\tcmalloc.h"
$TcLib = Join-Path $VcpkgLib "tcmalloc_minimal.lib"
if ((Test-Path $TcHeader) -and (Test-Path $TcLib)) {
    Write-Host "Building: redis workload (tcmalloc)"
    $BenchTcOut = Join-Path $OutDir "bench_redis_workload_tcmalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_TCMALLOC=1", $BenchSrc, $BenchHz6Diag, $TcLib, "/link", "/out:$BenchTcOut"))
} else {
    Write-Warning "tcmalloc not found in $VcpkgRoot; skipping tcmalloc redis workload bench."
}

Invoke-AppLikeHz5BenchBuild `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutputPath (Join-Path $OutDir "bench_redis_workload_hz5_policy.exe")

$Hz8Flags = @(
    "/I$Hz8Dir\include", "/I$Hz8Dir\src",
    "/DHZ_BENCH_USE_HZ8=1",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1"
)
$Hz8PreTransitionDefaultFlags = @(
    "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
    "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
    "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
    "/DH8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1=1",
    "/DH8_MEDIUM_PAGE_ENTRY_BOUNDARY_L1=1"
)
$Hz8PreMediumTransitionDefaultFlags = $Hz8PreTransitionDefaultFlags + @(
    "/DH8_SMALL_TRANSITION_INVENTORY_L1=1"
)
$Hz8DefaultFlags = $Hz8PreMediumTransitionDefaultFlags + @(
    "/DH8_MEDIUM_TRANSITION_INVENTORY_L1=1"
)
$hz8Variants = @(
    @{ Name = "hz8"; Output = "bench_redis_workload_hz8.exe"; ExtraFlags = $Hz8DefaultFlags },
    @{ Name = "hz8-pre-transition-rollback"; Output = "bench_redis_workload_hz8_pre_transition.exe"; ExtraFlags = $Hz8PreTransitionDefaultFlags },
    @{
        Name = "hz8-small-partial-transition-only"
        Output = "bench_redis_workload_hz8_small_partial_transition_only.exe"
        ExtraFlags = $Hz8PreTransitionDefaultFlags + @(
            "/DH8_SMALL_PARTIAL_TRANSITION_DEPOT_L1=1",
            "/DH8_SMALL_PARTIAL_TRANSITION_ONLY_L1B=1"
        )
    },
    @{
        Name = "hz8-small-transition-inventory"
        Output = "bench_redis_workload_hz8_small_transition_inventory.exe"
        ExtraFlags = $Hz8PreMediumTransitionDefaultFlags
    },
    @{
        Name = "hz8-medium-transition-inventory"
        Output = "bench_redis_workload_hz8_medium_transition_inventory.exe"
        ExtraFlags = $Hz8DefaultFlags
    },
    @{ Name = "hz8-v2-rollback"; Output = "bench_redis_workload_hz8_v2.exe"; ExtraFlags = @() },
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
)
if (-not $IncludeHz8Research) {
    $hz8Variants = @($hz8Variants | Where-Object { $_.Name -eq "hz8" })
}
if ($RequestedHz8Variants -and $RequestedHz8Variants.Count -gt 0) {
    $wanted = @($RequestedHz8Variants | ForEach-Object { $_ -split ',' } |
        ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $hz8Variants = @($hz8Variants | Where-Object { $wanted -contains $_.Name })
    if ($hz8Variants.Count -ne $wanted.Count) {
        throw "Unknown or duplicate HZ8 variant in: $($wanted -join ', ')"
    }
}
foreach ($variant in $hz8Variants) {
    Write-Host "Building: redis workload ($($variant.Name))"
    $output = Join-Path $OutDir $variant.Output
    Invoke-Checked $Cc ($BaseFlags + $Hz8Flags + $variant.ExtraFlags + $Hz8Sources + @($BenchSrc, "/link", "/out:$output"))
}

if ($RequestedHz8Variants) {
    Write-Host "Built focused HZ8 Redis artifacts in: $OutDir"
    return
}

Invoke-AppLikeHz6BenchBuilds `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutDir $OutDir `
    -OutputPrefix "bench_redis_workload" `
    -IncludeControlCapacity

$RuntimeDlls = @(
    (Join-Path $VcpkgBin "mimalloc.dll"),
    (Join-Path $VcpkgBin "mimalloc-redirect.dll"),
    (Join-Path $VcpkgBin "tcmalloc_minimal.dll")
)

foreach ($dll in $RuntimeDlls) {
    if (Test-Path $dll) {
        Copy-Item -Force $dll $OutDir | Out-Null
    }
}

Write-Host "Built Redis workload artifacts in: $OutDir"
