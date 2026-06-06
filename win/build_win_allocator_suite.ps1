param(
    [string]$VcpkgRoot,
    [switch]$DiagnosticHz6Probes
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $RepoRoot "out_win_suite"

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Hz3Script = Join-Path $RepoRoot "hakozuna\\win\\build_win_bench_compare.ps1"
$Hz4Script = Join-Path $RepoRoot "hakozuna-mt\\win\\build_win_bench_compare.ps1"
$Hz5Root = Join-Path $RepoRoot "hakozuna-hz5"
$Hz6Root = Join-Path $RepoRoot "hakozuna-hz6"
$Hz6Common = Join-Path $Hz6Root "win\\hz6_win_build_common.ps1"
$AppLikeBuildCommon = Join-Path $PSScriptRoot "bench_app_like_allocator_build_common.ps1"

$Hz3Args = @()
$Hz4Args = @()
if ($VcpkgRoot) {
    $Hz3Args += @("-VcpkgRoot", $VcpkgRoot)
    $Hz4Args += @("-VcpkgRoot", $VcpkgRoot)
}

& $Hz3Script @Hz3Args
if ($LASTEXITCODE -ne 0) {
    throw "hz3 build script failed with exit code $LASTEXITCODE"
}

& $Hz4Script @Hz4Args
if ($LASTEXITCODE -ne 0) {
    throw "hz4 build script failed with exit code $LASTEXITCODE"
}

$Hz3OutDir = Join-Path $RepoRoot "hakozuna\out_win_bench"
$Hz4OutDir = Join-Path $RepoRoot "hakozuna-mt\out_win_bench"

$Compiler = Get-Command "clang-cl" -ErrorAction Stop
$CompareSource = Join-Path $PSScriptRoot "bench_allocator_compare.c"

if (Test-Path $Hz5Root) {
    $Hz5IncludeFlags = @(
        "/I$Hz5Root\include",
        "/I$Hz5Root\contract",
        "/I$Hz5Root\core",
        "/I$Hz5Root\fallback",
        "/I$Hz5Root\lowpage",
        "/I$Hz5Root\policy",
        "/I$Hz5Root\route",
        "/I$Hz5Root\smallfront",
        "/I$Hz5Root\wrapper"
    )
    $Hz5LibSources = @(
        "$Hz5Root\contract\hz5_contract.c",
        "$Hz5Root\core\hz5_segment.c",
        "$Hz5Root\core\hz5_run.c",
        "$Hz5Root\core\hz5_owner.c",
        "$Hz5Root\core\hz5_remote.c",
        "$Hz5Root\core\hz5_tcache.c",
        "$Hz5Root\core\hz5_stats.c",
        "$Hz5Root\route\hz5_route.c",
        "$Hz5Root\wrapper\hz5_wrapper.c",
        "$Hz5Root\policy\hz5_policy.c",
        "$Hz5Root\lowpage\hz5_lowpage64.c",
        "$Hz5Root\lowpage\hz5_lowpage64_os.c",
        "$Hz5Root\lowpage\hz5_lowpage64_p43_segment.c"
    )
    $Hz5Output = Join-Path $OutDir "bench_mixed_ws_hz5_policy.exe"
    $Hz5Args = @(
        "/nologo",
        "/O2",
        "/DNDEBUG",
        "/std:c11",
        "/W3",
        "/MD",
        "/DHZ_BENCH_USE_HZ5_POLICY",
        "/DHZ_BENCH_DISABLE_REALLOC=1",
        "/DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN=1"
    )
    $Hz5Args += $Hz5IncludeFlags
    $Hz5Args += $Hz5LibSources
    $Hz5Args += $CompareSource
    $Hz5Args += "psapi.lib"
    $Hz5Args += "/Fe:$Hz5Output"
    Write-Host "[hz5-win] building bench_mixed_ws_hz5_policy.exe"
    & $Compiler.Source @Hz5Args
    if ($LASTEXITCODE -ne 0) {
        throw "HZ5 mixed_ws build failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Warning "HZ5 root not found; skipping HZ5 matrix executable: $Hz5Root"
}

if (Test-Path $Hz6Common) {
    . $Hz6Common
    if (Test-Path $AppLikeBuildCommon) {
        . $AppLikeBuildCommon
    } else {
        throw "HZ6 app-like build helper not found: $AppLikeBuildCommon"
    }
    $Hz6IncludeFlags = Get-Hz6WinIncludeFlags -Hz6Root $Hz6Root -ExtraIncludeRoots @("win")
    $Hz6LibSources = Get-Hz6WinLibSources -Hz6Root $Hz6Root
    if ((Split-Path -Leaf $CompareSource) -eq "bench_allocator_compare.c") {
        $Hz6LibSources += (Join-Path $RepoRoot "win\bench_allocator_compare_hz6_summary.c")
    }
    if ((Split-Path -Leaf $CompareSource) -eq "bench_redis_workload_compare.c") {
        $Hz6LibSources += (Join-Path $RepoRoot "win\bench_redis_hz6_diag.c")
    }
    $Hz6CommonFlags = Get-Hz6WinClangCommonFlags
    $Hz6DiagnosticFlags = @()
    if ($DiagnosticHz6Probes) {
        $Hz6DiagnosticFlags += "/DHZ6_DIAGNOSTIC_PROBES=1"
    }
    $Hz6Profiles = @(
        @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" },
        @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" },
        @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" }
    )
    $Hz6BroadCapacityFlags = Get-Hz6WinBroadCapacityFlags
    $Hz6Route4kCapacityFlags = Get-Hz6WinRoute4kCapacityFlags
    $Hz6LargerLowRssCapacityFlags = Get-Hz6WinLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6SameOwnerFastLargerLowRssCapacityFlags = Get-Hz6WinSameOwnerFastLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags
    $Hz6DirectLocalFreeReuseLargerLowRssCapacityFlags = Get-Hz6WinDirectLocalFreeReuseLargerLowRssFront8kSourceRunDesc8kRoute8kCapacityFlags

    foreach ($profile in $Hz6Profiles) {
        foreach ($variant in @(
            @{ Suffix = ""; ExtraFlags = @() },
            @{ Suffix = "_broad"; ExtraFlags = $Hz6BroadCapacityFlags },
            @{ Suffix = "_route4k"; ExtraFlags = $Hz6Route4kCapacityFlags },
            @{ Suffix = "_largerlowrss"; ExtraFlags = $Hz6LargerLowRssCapacityFlags },
            @{ Suffix = "_sameownerfast_largerlowrss"; ExtraFlags = $Hz6SameOwnerFastLargerLowRssCapacityFlags },
            @{ Suffix = "_directlocalfreereuse_largerlowrss"; ExtraFlags = $Hz6DirectLocalFreeReuseLargerLowRssCapacityFlags }
        )) {
            $output = Join-Path $OutDir ("bench_mixed_ws_hz6_{0}{1}.exe" -f $profile.Name, $variant.Suffix)
            $args = @()
            $args += $Hz6CommonFlags
            $args += $Hz6DiagnosticFlags
            $args += "/DHZ_BENCH_USE_HZ6"
            $args += "/DHZ_BENCH_DISABLE_REALLOC=1"
            $args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
            $args += $variant.ExtraFlags
            $args += $Hz6IncludeFlags
            $args += $Hz6LibSources
            $args += $CompareSource
            $args += "psapi.lib"
            $args += "/Fe:$output"
            Write-Host ("[hz6-win] building {0}" -f (Split-Path -Leaf $output))
            & $Compiler.Source @args
            if ($LASTEXITCODE -ne 0) {
                throw "HZ6 mixed_ws build failed for $($profile.Name)$($variant.Suffix) with exit code $LASTEXITCODE"
            }
        }
    }
} else {
    Write-Warning "HZ6 Windows build helper not found; skipping HZ6 matrix executables: $Hz6Common"
}

$Artifacts = @(
    (Join-Path $Hz3OutDir "bench_mixed_ws_crt.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_hz3.exe"),
    (Join-Path $Hz4OutDir "bench_mixed_ws_hz4.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_mimalloc.exe"),
    (Join-Path $Hz3OutDir "bench_mixed_ws_tcmalloc.exe"),
    (Join-Path $Hz3OutDir "mimalloc.dll"),
    (Join-Path $Hz3OutDir "mimalloc-redirect.dll"),
    (Join-Path $Hz3OutDir "tcmalloc_minimal.dll")
)

foreach ($artifact in $Artifacts) {
    if (Test-Path $artifact) {
        Copy-Item -Force $artifact $OutDir | Out-Null
    }
}

Write-Host "Built suite artifacts in: $OutDir"
