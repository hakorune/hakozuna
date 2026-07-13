param(
    [string]$VcpkgRoot,
    [switch]$OnlyHz8,
    [switch]$IncludeHz8Research
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_mt_remote"
$ObjDir = Join-Path $OutDir "obj"
$SuiteBuild = Join-Path $PSScriptRoot "build_win_allocator_suite.ps1"
$ModernBuildCommon = Join-Path $PSScriptRoot "bench_app_like_allocator_build_common.ps1"
$Hz3BuildScript = Join-Path $RepoRoot "hakozuna\win\build_win_min.ps1"

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

if (-not $OnlyHz8) {
    & $SuiteBuild
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_allocator_suite.ps1 failed with exit code $LASTEXITCODE"
    }
}

if (-not $OnlyHz8) {
    $Hz3ExtraDefines = @(
        "HZ3_S97_REMOTE_STASH_BUCKET=2",
        "HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS=256",
        "HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1"
    )
    & $Hz3BuildScript -OutDirName "out_win_mt_remote_hz3" -ExtraDefines $Hz3ExtraDefines | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "build_win_min.ps1 failed for mt_remote hz3 profile with exit code $LASTEXITCODE"
    }
}

$VcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$VcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"
$VcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"

$BenchSrc = Join-Path $RepoRoot "win\bench_random_mixed_mt_remote_compare.c"
$Hz3Dir = Join-Path $RepoRoot "hakozuna"
$Hz4Dir = Join-Path $RepoRoot "hakozuna-mt"
$Hz7Dir = Join-Path $RepoRoot "hakozuna-hz7"
$Hz7Dir = Join-Path $RepoRoot "hz7"
$Hz7V2Dir = Join-Path $Hz7Dir "v2"
$Hz8Dir = Join-Path $RepoRoot "hakozuna-hz8"
$Hz3Lib = Join-Path $Hz3Dir "out_win_mt_remote_hz3\hz3_win.lib"
$Hz4Lib = Join-Path $Hz4Dir "out_win_bench\hz4_win.lib"
$Hz7Source = Join-Path $Hz7Dir "hz7.c"
$Hz7V2Source = Join-Path $Hz7V2Dir "hz7.c"
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
    "/I$Hz4Dir\win",
    "/I$Hz7Dir"
)

$Hz8CommonFlags = @(
    "/I$Hz8Dir\include",
    "/I$Hz8Dir\src",
    "/DHZ_BENCH_USE_HZ8=1",
    "/DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1=1",
    "/DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4",
    "/DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1=1",
    "/DH8_MEDIUM_KEEP_REFILL_EMPTY_L1=1",
    "/DH8_REMOTE_SPAN_LEASE_PUBLISH_L1=1",
    "/DH8_REMOTE_TRANSITION_BACKOFF_L1=1"
)
$Hz8DefaultFlags = @(
    "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
    "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
    "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
    "/DH8_MEDIUM_PAGE_GENERAL_GEOMETRY_L1=1",
    "/DH8_MEDIUM_PAGE_ENTRY_BOUNDARY_L1=1"
)

function Invoke-Hz8MtRemoteBuilds {
    $hz8Variants = @(
        @{ Name = "hz8"; Output = "bench_random_mixed_mt_remote_hz8.exe"; ExtraFlags = $Hz8DefaultFlags },
        @{ Name = "hz8-v2-rollback"; Output = "bench_random_mixed_mt_remote_hz8_v2.exe"; ExtraFlags = @() },
        @{
            Name = "hz8-v2-nomag"
            Output = "bench_random_mixed_mt_remote_hz8_v2_nomag.exe"
            ExtraFlags = @("/DH8_REUSABLE_SPAN_MAGAZINE_L1=0")
        },
        @{
            Name = "hz8-v2-mag32"
            Output = "bench_random_mixed_mt_remote_hz8_v2_mag32.exe"
            ExtraFlags = @("/DH8_REUSABLE_SPAN_MAG_CAP=32")
        },
        @{
            Name = "hz8-v2-mediumlocalfast"
            Output = "bench_random_mixed_mt_remote_hz8_v2_mediumlocalfast.exe"
            ExtraFlags = @("/DH8_MEDIUM_ENABLE_LOCAL_FAST_TIER=1")
        },
        @{
            Name = "hz8-small-partial-depot"
            Output = "bench_random_mixed_mt_remote_hz8_small_partial_depot.exe"
            ExtraFlags = $Hz8DefaultFlags + @(
                "/DH8_SMALL_PARTIAL_TRANSITION_DEPOT_L1=1"
            )
        },
        @{
            Name = "hz8-small-partial-transition-only"
            Output = "bench_random_mixed_mt_remote_hz8_small_partial_transition_only.exe"
            ExtraFlags = $Hz8DefaultFlags + @(
                "/DH8_SMALL_PARTIAL_TRANSITION_DEPOT_L1=1",
                "/DH8_SMALL_PARTIAL_TRANSITION_ONLY_L1B=1"
            )
        },
        @{
            Name = "hz8-medium-pageshadow"
            Output = "bench_random_mixed_mt_remote_hz8_medium_pageshadow.exe"
            ExtraFlags = @("/DH8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0=1")
        },
        @{
            Name = "hz8-r3-page8k-integrated"
            Output = "bench_random_mixed_mt_remote_hz8_medium_page8k_remote.exe"
            ExtraFlags = @(
                "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
                "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
                "/DHZ_BENCH_DISABLE_REALLOC=1"
            )
        },
        @{
            Name = "hz8-r3-page8k-target-dispatch"
            Output = "bench_random_mixed_mt_remote_hz8_medium_page8k_target_dispatch.exe"
            ExtraFlags = @(
                "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
                "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
                "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
                "/DHZ_BENCH_DISABLE_REALLOC=1"
            )
        },
        @{
            Name = "hz8-r3-page8k-target-dispatch-diag"
            Output = "bench_random_mixed_mt_remote_hz8_medium_page8k_target_dispatch_diag.exe"
            ExtraFlags = @(
                "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
                "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
                "/DH8_MEDIUM_PAGE8K_TARGET_DISPATCH_L1=1",
                "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
                "/DHZ_BENCH_DISABLE_REALLOC=1"
            )
        },
        @{
            Name = "hz8-r3-page8k-integrated-diag"
            Output = "bench_random_mixed_mt_remote_hz8_medium_page8k_remote_diag.exe"
            ExtraFlags = @(
                "/DH8_MEDIUM_PAGE8K_REMOTE_L1=1",
                "/DH8_MEDIUM_PAGE8K_REMOTE_BEHAVIOR_L1=1",
                "/DH8_PAGE8K_REMOTE_DIAGNOSTIC=1",
                "/DHZ_BENCH_DISABLE_REALLOC=1"
            )
        },
        @{
            Name = "hz8-v3-adaptive-shadow"
            Output = "bench_random_mixed_mt_remote_hz8_v3_adaptive_shadow.exe"
            ExtraFlags = @("/DH8_ADAPTIVE_TRANSFER_SHADOW_L0=1")
        },
        @{
            Name = "hz8-reclaim-shadow"
            Output = "bench_random_mixed_mt_remote_hz8_reclaim_shadow.exe"
            ExtraFlags = @("/DH8_RECLAIM_ADAPTER_SHADOW_L0=1")
        },
        @{
            Name = "hz8-magazine-tail-shadow"
            Output = "bench_random_mixed_mt_remote_hz8_magazine_tail_shadow.exe"
            ExtraFlags = @("/DH8_MAGAZINE_TAIL_RECLAIM_SHADOW_L0=1")
        },
        @{
            Name = "hz8-small-available4k"
            Output = "bench_random_mixed_mt_remote_hz8_small_available4k.exe"
            ExtraFlags = @("/DH8_SMALL_AVAILABLE_INDEX_L1=1")
        },
        @{
            Name = "hz8-small-available4k-diag"
            Output = "bench_random_mixed_mt_remote_hz8_small_available4k_diag.exe"
            ExtraFlags = @(
                "/DH8_SMALL_AVAILABLE_INDEX_L1=1",
                "/DH8_SMALL_AVAILABLE_INDEX_DIAG=1"
            )
        }
    )
    if (-not $IncludeHz8Research) {
        $hz8Variants = @($hz8Variants | Where-Object { $_.Name -eq "hz8" })
    }
    foreach ($variant in $hz8Variants) {
        Write-Host "Building: mt_remote ($($variant.Name))"
        $output = Join-Path $OutDir $variant.Output
        Invoke-Checked $Cc ($BaseFlags + $Hz8CommonFlags + $variant.ExtraFlags +
            @($BenchSrc) + $Hz8Sources + @("/link", "/out:$output"))
    }
}

if ($OnlyHz8) {
    Invoke-Hz8MtRemoteBuilds
    Write-Host "Built HZ8 MT remote artifacts in: $OutDir"
    return
}

Write-Host "Building: mt_remote (CRT baseline)"
$BenchCrtOut = Join-Path $OutDir "bench_random_mixed_mt_remote_crt.exe"
Invoke-Checked $Cc ($BaseFlags + @($BenchSrc, "/link", "/out:$BenchCrtOut"))

if (Test-Path $Hz3Lib) {
    Write-Host "Building: mt_remote (hz3)"
    $BenchHz3Out = Join-Path $OutDir "bench_random_mixed_mt_remote_hz3.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ3=1", $BenchSrc, $Hz3Lib, "/link", "/out:$BenchHz3Out"))
} else {
    Write-Warning "hz3_win.lib not found; skipping hz3 mt_remote bench."
}

$Hz4ApiObj = Join-Path $ObjDir "hz4_win_api_mt_remote.obj"
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
    Write-Host "Building: mt_remote (hz4)"
    $BenchHz4Out = Join-Path $OutDir "bench_random_mixed_mt_remote_hz4.exe"
    Invoke-Checked $Cc ($Hz4Flags + @("/DHZ_BENCH_USE_HZ4=1", $BenchSrc, $Hz4ApiObj, $Hz4Lib, "/link", "/out:$BenchHz4Out"))
} else {
    Write-Warning "hz4_win.lib not found; skipping hz4 mt_remote bench."
}

$MiHeader = Join-Path $VcpkgInclude "mimalloc.h"
$MiLib = Join-Path $VcpkgLib "mimalloc.dll.lib"
if ((Test-Path $MiHeader) -and (Test-Path $MiLib)) {
    Write-Host "Building: mt_remote (mimalloc)"
    $BenchMiOut = Join-Path $OutDir "bench_random_mixed_mt_remote_mimalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_MIMALLOC=1", $BenchSrc, $MiLib, "/link", "/out:$BenchMiOut"))
} else {
    Write-Warning "mimalloc not found in $VcpkgRoot; skipping mimalloc mt_remote bench."
}

$TcHeader = Join-Path $VcpkgInclude "gperftools\tcmalloc.h"
$TcLib = Join-Path $VcpkgLib "tcmalloc_minimal.lib"
if ((Test-Path $TcHeader) -and (Test-Path $TcLib)) {
    Write-Host "Building: mt_remote (tcmalloc)"
    $BenchTcOut = Join-Path $OutDir "bench_random_mixed_mt_remote_tcmalloc.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/I$VcpkgInclude", "/DHZ_BENCH_USE_TCMALLOC=1", $BenchSrc, $TcLib, "/link", "/out:$BenchTcOut"))
} else {
    Write-Warning "tcmalloc not found in $VcpkgRoot; skipping tcmalloc mt_remote bench."
}

Invoke-AppLikeHz5BenchBuild `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutputPath (Join-Path $OutDir "bench_random_mixed_mt_remote_hz5_policy.exe")

if (Test-Path $Hz7Source) {
    Write-Host "Building: mt_remote (hz7-tinyroute)"
    $BenchHz7Out = Join-Path $OutDir "bench_random_mixed_mt_remote_hz7.exe"
    Invoke-Checked $Cc ($BaseFlags + @("/DHZ_BENCH_USE_HZ7=1", $Hz7Source, $BenchSrc, "/link", "/out:$BenchHz7Out"))
} else {
    Write-Warning "HZ7 source not found; skipping HZ7 mt_remote bench."
}

if (Test-Path $Hz7V2Source) {
    Write-Host "Building: mt_remote (hz7-v2-remote-natural)"
    $BenchHz7V2RemoteNaturalOut = Join-Path $OutDir "bench_random_mixed_mt_remote_hz7_v2_remote_natural.exe"
    $Hz7V2RemoteNaturalFlags = @(
        "/I$Hz7V2Dir",
        "/DHZ_BENCH_USE_HZ7=1",
        "/DH7_REMOTE_NATURAL_PRESET=1"
    )
    Invoke-Checked $Cc ($Hz7V2RemoteNaturalFlags + $BaseFlags + @($Hz7V2Source, $BenchSrc, "/link", "/out:$BenchHz7V2RemoteNaturalOut"))
} else {
    Write-Warning "HZ7 v2 source not found; skipping HZ7 v2 remote-natural mt_remote bench."
}

Invoke-Hz8MtRemoteBuilds

Invoke-AppLikeHz6BenchBuilds `
    -Compiler $Cc `
    -BaseFlags $BaseFlags `
    -RepoRoot $RepoRoot `
    -BenchSrc $BenchSrc `
    -OutDir $OutDir `
    -OutputPrefix "bench_random_mixed_mt_remote"

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

Write-Host "Built MT remote artifacts in: $OutDir"
