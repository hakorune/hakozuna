$ErrorActionPreference = "Stop"

function Get-Hz5WinBenchIncludeFlags {
    param([Parameter(Mandatory = $true)][string]$Hz5Root)

    @(
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
}

function Get-Hz5WinBenchLibSources {
    param([Parameter(Mandatory = $true)][string]$Hz5Root)

    @(
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
}

function Get-Hz5WinBenchFlags {
    @(
        "/DHZ_BENCH_USE_HZ5_POLICY",
        "/DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN=1"
    )
}

function Get-Hz6WinBroadCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)4096)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)1024)"
    )
}

function Get-Hz6WinAppLikeCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)262144)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)262144)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)262144)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)32768)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)65536)"
    )
}

function Invoke-AppLikeHz5BenchBuild {
    param(
        [Parameter(Mandatory = $true)][string]$Compiler,
        [Parameter(Mandatory = $true)][string[]]$BaseFlags,
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$BenchSrc,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $hz5Root = Join-Path $RepoRoot "hakozuna-hz5"
    if (-not (Test-Path $hz5Root)) {
        Write-Warning "HZ5 root not found; skipping $(Split-Path -Leaf $OutputPath): $hz5Root"
        return
    }

    $args = @()
    $args += $BaseFlags
    $args += (Get-Hz5WinBenchFlags)
    $args += (Get-Hz5WinBenchIncludeFlags -Hz5Root $hz5Root)
    $args += (Get-Hz5WinBenchLibSources -Hz5Root $hz5Root)
    $args += $BenchSrc
    $args += "/Fe:$OutputPath"
    Write-Host "[hz5-win] building $(Split-Path -Leaf $OutputPath)"
    & $Compiler @args
    if ($LASTEXITCODE -ne 0) {
        throw "HZ5 app-like bench build failed for $(Split-Path -Leaf $OutputPath) with exit code $LASTEXITCODE"
    }
}

function Invoke-AppLikeHz6BenchBuilds {
    param(
        [Parameter(Mandatory = $true)][string]$Compiler,
        [Parameter(Mandatory = $true)][string[]]$BaseFlags,
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$BenchSrc,
        [Parameter(Mandatory = $true)][string]$OutDir,
        [Parameter(Mandatory = $true)][string]$OutputPrefix
    )

    $hz6Root = Join-Path $RepoRoot "hakozuna-hz6"
    $hz6Common = Join-Path $hz6Root "win\hz6_win_build_common.ps1"
    if (-not (Test-Path $hz6Common)) {
        Write-Warning "HZ6 build helper not found; skipping $OutputPrefix HZ6 rows: $hz6Common"
        return
    }

    . $hz6Common
    $includeFlags = Get-Hz6WinIncludeFlags -Hz6Root $hz6Root -ExtraIncludeRoots @("win")
    $libSources = Get-Hz6WinLibSources -Hz6Root $hz6Root
    $commonFlags = Get-Hz6WinClangCommonFlags
    $profiles = @(
        @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" },
        @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" },
        @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" }
    )
    $broadFlags = Get-Hz6WinBroadCapacityFlags
    $appLikeFlags = Get-Hz6WinAppLikeCapacityFlags

    foreach ($profile in $profiles) {
        foreach ($variant in @(
            @{ Suffix = ""; ExtraFlags = @() },
            @{ Suffix = "_broad"; ExtraFlags = $broadFlags },
            @{ Suffix = "_appcap"; ExtraFlags = $appLikeFlags }
        )) {
            $output = Join-Path $OutDir ("{0}_hz6_{1}{2}.exe" -f $OutputPrefix, $profile.Name, $variant.Suffix)
            $args = @()
            $args += $commonFlags
            $args += "/DHZ_BENCH_USE_HZ6"
            $args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
            $args += $variant.ExtraFlags
            $args += $includeFlags
            $args += $libSources
            $args += $BenchSrc
            $args += "/Fe:$output"
            Write-Host "[hz6-win] building $(Split-Path -Leaf $output)"
            & $Compiler @args
            if ($LASTEXITCODE -ne 0) {
                throw "HZ6 app-like bench build failed for $(Split-Path -Leaf $output) with exit code $LASTEXITCODE"
            }
        }
    }
}
