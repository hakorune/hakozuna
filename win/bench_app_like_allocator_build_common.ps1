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

function Get-Hz6WinControlCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)512)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)512)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinRoute4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)512)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinNoBoostRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1"
    $flags
}

function Get-Hz6WinSpillRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_FRONTCACHE_SPILL_ON_DESCRIPTOR_EXHAUSTION=1"
    $flags
}

function Get-Hz6WinBorrowRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_FRONTCACHE_BORROW_LARGER_ON_CLASS_MISS=1"
    $flags
}

function Get-Hz6WinCapRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_FRONTCACHE_CAP_ON_FREE=1"
    $flags
}

function Get-Hz6WinSourceRunRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    $flags
}

function Get-Hz6WinSourceRunReclaimRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceRunRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1=1"
    $flags
}

function Get-Hz6WinSourceRunSameClassReclaimRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceRunRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1=1"
    $flags
}

function Get-Hz6WinDescriptorlessRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSourceRunRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTORLESS_FRONTCACHE_L1=1"
    $flags
}

function Get-Hz6WinDescriptorReserveRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1=1"
    $flags
}

function Get-Hz6WinDescriptorColdRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTORLESS_OVER_CAP_ONLY_L1=1"
    $flags
}

function Get-Hz6WinDescriptorColdGovRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_COLD_GOV_L1=1"
    $flags
}

function Get-Hz6WinDescriptorColdGovWideWsRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $flags += "/DHZ6_DESCRIPTOR_COLD_GOV_L1=1"
    $flags += "/DHZ6_DESCRIPTOR_COLD_GOV_DETACH_BUDGET=((size_t)512)"
    $flags
}

function Get-Hz6WinDesc4kRoute4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)128)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinSource512Route4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)512)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinDesc4kSource512Route4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)"
    )
}

function Get-Hz6WinRedisLowRssRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinDesc4kSource512Route4kCapacityFlags
    $flags += "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunRoute4kCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinRedisLowRssRoute4kCapacityFlags
    $flags += "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    $flags
}

function Get-Hz6WinRedisLowRssSourceRunRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)8192)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)8192)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)512)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1",
        "/DHZ6_SOURCE_RUN_REUSE_L1=1"
    )
}

function Get-Hz6WinRedisLowRssSlimRoute4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)2048)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
        "/DHZ6_SOURCE_BLOCK_CAPACITY=((size_t)256)",
        "/DHZ6_FRONT_CACHE_BIN_CAPACITY=((size_t)256)",
        "/DHZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1"
    )
}

function Get-Hz6WinFront1kDesc4kSource512Route4kCapacityFlags {
    @(
        "/DHZ6_OBJECT_DESCRIPTOR_CAPACITY=((size_t)4096)",
        "/DHZ6_ROUTE_TABLE_CAPACITY=((size_t)4096)",
        "/DHZ6_TRANSFER_CACHE_CAPACITY=((size_t)512)",
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

function Get-Hz6WinVisibleFirstAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_VISIBLE_FIRST_FREE_L1=1"
    $flags
}

function Get-Hz6WinNegativeFilterAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_NEGATIVE_FILTER_L1=1"
    $flags
}

function Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1"
    $flags
}

function Get-Hz6WinSharedRouteDirectoryFirstAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinAppLikeCapacityFlags
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_L1=1"
    $flags += "/DHZ6_SHARED_ROUTE_DIRECTORY_FIRST_L1=1"
    $flags
}

function Get-Hz6WinOwnerLocalityAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags
    $flags += "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    $flags += "/DHZ6_DIAGNOSTIC_PROBES=1"
    $flags
}

function Get-Hz6WinOwnerLocalityFastAppLikeCapacityFlags {
    $flags = @()
    $flags += Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags
    $flags += "/DHZ6_OWNER_LOCALITY_INDEX_L1=1"
    $flags
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
        [Parameter(Mandatory = $true)][string]$OutputPrefix,
        [string[]]$Hz6Profiles,
        [string[]]$CapacityLanes,
        [switch]$DiagnosticHz6Probes,
        [switch]$IncludeControlCapacity
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
    $profileMap = @{
        "strict" = @{ Name = "strict"; Define = "HZ6_PROFILE_STRICT" }
        "speed" = @{ Name = "speed"; Define = "HZ6_PROFILE_SPEED" }
        "rss" = @{ Name = "rss"; Define = "HZ6_PROFILE_RSS" }
    }
    $broadFlags = Get-Hz6WinBroadCapacityFlags
    $controlFlags = Get-Hz6WinControlCapacityFlags
    $route4kFlags = Get-Hz6WinRoute4kCapacityFlags
    $noBoostRoute4kFlags = Get-Hz6WinNoBoostRoute4kCapacityFlags
    $spillRoute4kFlags = Get-Hz6WinSpillRoute4kCapacityFlags
    $borrowRoute4kFlags = Get-Hz6WinBorrowRoute4kCapacityFlags
    $capRoute4kFlags = Get-Hz6WinCapRoute4kCapacityFlags
    $sourceRunRoute4kFlags = Get-Hz6WinSourceRunRoute4kCapacityFlags
    $sourceRunReclaimRoute4kFlags = Get-Hz6WinSourceRunReclaimRoute4kCapacityFlags
    $sourceRunSameClassReclaimRoute4kFlags = Get-Hz6WinSourceRunSameClassReclaimRoute4kCapacityFlags
    $descriptorlessRoute4kFlags = Get-Hz6WinDescriptorlessRoute4kCapacityFlags
    $descriptorReserveRoute4kFlags = Get-Hz6WinDescriptorReserveRoute4kCapacityFlags
    $descriptorColdRoute4kFlags = Get-Hz6WinDescriptorColdRoute4kCapacityFlags
    $descriptorColdGovRoute4kFlags = Get-Hz6WinDescriptorColdGovRoute4kCapacityFlags
    $descriptorColdGovWideWsRoute4kFlags = Get-Hz6WinDescriptorColdGovWideWsRoute4kCapacityFlags
    $desc4kRoute4kFlags = Get-Hz6WinDesc4kRoute4kCapacityFlags
    $source512Route4kFlags = Get-Hz6WinSource512Route4kCapacityFlags
    $desc4kSource512Route4kFlags = Get-Hz6WinDesc4kSource512Route4kCapacityFlags
    $redisLowRssRoute4kFlags = Get-Hz6WinRedisLowRssRoute4kCapacityFlags
    $redisLowRssSourceRunRoute4kFlags = Get-Hz6WinRedisLowRssSourceRunRoute4kCapacityFlags
    $redisLowRssSourceRunRoute8kFlags = Get-Hz6WinRedisLowRssSourceRunRoute8kCapacityFlags
    $redisLowRssSourceRunDesc8kRoute8kFlags = Get-Hz6WinRedisLowRssSourceRunDesc8kRoute8kCapacityFlags
    $redisLowRssSlimRoute4kFlags = Get-Hz6WinRedisLowRssSlimRoute4kCapacityFlags
    $front1kDesc4kSource512Route4kFlags = Get-Hz6WinFront1kDesc4kSource512Route4kCapacityFlags
    $appLikeFlags = Get-Hz6WinAppLikeCapacityFlags
    $visibleFirstAppLikeFlags = Get-Hz6WinVisibleFirstAppLikeCapacityFlags
    $negativeFilterAppLikeFlags = Get-Hz6WinNegativeFilterAppLikeCapacityFlags
    $sharedRouteDirectoryAppLikeFlags = Get-Hz6WinSharedRouteDirectoryAppLikeCapacityFlags
    $sharedRouteDirectoryFirstAppLikeFlags = Get-Hz6WinSharedRouteDirectoryFirstAppLikeCapacityFlags
    $ownerLocalityAppLikeFlags = Get-Hz6WinOwnerLocalityAppLikeCapacityFlags
    $ownerLocalityFastAppLikeFlags = Get-Hz6WinOwnerLocalityFastAppLikeCapacityFlags
    $laneMap = @{
        "default" = @{ Suffix = ""; ExtraFlags = @() }
        "broad" = @{ Suffix = "_broad"; ExtraFlags = $broadFlags }
        "control" = @{ Suffix = "_control"; ExtraFlags = $controlFlags }
        "route4k" = @{ Suffix = "_route4k"; ExtraFlags = $route4kFlags }
        "noboost-route4k" = @{ Suffix = "_noboost_route4k"; ExtraFlags = $noBoostRoute4kFlags }
        "spill-route4k" = @{ Suffix = "_spill_route4k"; ExtraFlags = $spillRoute4kFlags }
        "borrow-route4k" = @{ Suffix = "_borrow_route4k"; ExtraFlags = $borrowRoute4kFlags }
        "cap-route4k" = @{ Suffix = "_cap_route4k"; ExtraFlags = $capRoute4kFlags }
        "sourcerun-route4k" = @{ Suffix = "_sourcerun_route4k"; ExtraFlags = $sourceRunRoute4kFlags }
        "sourcerun-reclaim-route4k" = @{ Suffix = "_sourcerun_reclaim_route4k"; ExtraFlags = $sourceRunReclaimRoute4kFlags }
        "sourcerun-sameclass-route4k" = @{ Suffix = "_sourcerun_sameclass_route4k"; ExtraFlags = $sourceRunSameClassReclaimRoute4kFlags }
        "descriptorless-route4k" = @{ Suffix = "_descriptorless_route4k"; ExtraFlags = $descriptorlessRoute4kFlags }
        "descriptorreserve-route4k" = @{ Suffix = "_descriptorreserve_route4k"; ExtraFlags = $descriptorReserveRoute4kFlags }
        "descriptorcold-route4k" = @{ Suffix = "_descriptorcold_route4k"; ExtraFlags = $descriptorColdRoute4kFlags }
        "descriptorcoldgov-route4k" = @{ Suffix = "_descriptorcoldgov_route4k"; ExtraFlags = $descriptorColdGovRoute4kFlags }
        "descriptorcoldgov-widews-route4k" = @{ Suffix = "_descriptorcoldgov_widews_route4k"; ExtraFlags = $descriptorColdGovWideWsRoute4kFlags }
        "desc4k-route4k" = @{ Suffix = "_desc4k_route4k"; ExtraFlags = $desc4kRoute4kFlags }
        "source512-route4k" = @{ Suffix = "_source512_route4k"; ExtraFlags = $source512Route4kFlags }
        "desc4k-source512-route4k" = @{ Suffix = "_desc4k_source512_route4k"; ExtraFlags = $desc4kSource512Route4kFlags }
        "redislowrss-route4k" = @{ Suffix = "_redislowrss_route4k"; ExtraFlags = $redisLowRssRoute4kFlags }
        "redislowrss-sourcerun-route4k" = @{ Suffix = "_redislowrss_sourcerun_route4k"; ExtraFlags = $redisLowRssSourceRunRoute4kFlags }
        "redislowrss-sourcerun-route8k" = @{ Suffix = "_redislowrss_sourcerun_route8k"; ExtraFlags = $redisLowRssSourceRunRoute8kFlags }
        "redislowrss-sourcerun-desc8k-route8k" = @{ Suffix = "_redislowrss_sourcerun_desc8k_route8k"; ExtraFlags = $redisLowRssSourceRunDesc8kRoute8kFlags }
        "redislowrss-slim-route4k" = @{ Suffix = "_redislowrss_slim_route4k"; ExtraFlags = $redisLowRssSlimRoute4kFlags }
        "front1k-desc4k-source512-route4k" = @{ Suffix = "_front1k_desc4k_source512_route4k"; ExtraFlags = $front1kDesc4kSource512Route4kFlags }
        "appcap" = @{ Suffix = "_appcap"; ExtraFlags = $appLikeFlags }
        "visiblefirst-appcap" = @{ Suffix = "_visiblefirst_appcap"; ExtraFlags = $visibleFirstAppLikeFlags }
        "negativefilter-appcap" = @{ Suffix = "_negativefilter_appcap"; ExtraFlags = $negativeFilterAppLikeFlags }
        "sharedir-appcap" = @{ Suffix = "_sharedir_appcap"; ExtraFlags = $sharedRouteDirectoryAppLikeFlags }
        "sharedirfirst-appcap" = @{ Suffix = "_sharedirfirst_appcap"; ExtraFlags = $sharedRouteDirectoryFirstAppLikeFlags }
        "ownerlocality-appcap" = @{ Suffix = "_ownerlocality_appcap"; ExtraFlags = $ownerLocalityAppLikeFlags }
        "ownerlocalityfast-appcap" = @{ Suffix = "_ownerlocalityfast_appcap"; ExtraFlags = $ownerLocalityFastAppLikeFlags }
    }

    function Split-Hz6BuildList {
        param([string[]]$Values)
        $items = @()
        foreach ($value in @($Values)) {
            foreach ($part in @($value -split ',' | ForEach-Object { $_.Trim().ToLowerInvariant() } | Where-Object { $_ -ne "" })) {
                $items += $part
            }
        }
        $items
    }

    $selectedProfileNames = Split-Hz6BuildList -Values $Hz6Profiles
    if (-not $selectedProfileNames -or $selectedProfileNames.Count -eq 0) {
        $selectedProfileNames = @("strict", "speed", "rss")
    }

    $selectedLaneNames = Split-Hz6BuildList -Values $CapacityLanes
    if (-not $selectedLaneNames -or $selectedLaneNames.Count -eq 0) {
        if ($IncludeControlCapacity) {
            $selectedLaneNames = @("default", "broad", "control", "route4k", "appcap", "ownerlocality-appcap", "ownerlocalityfast-appcap")
        } else {
            $selectedLaneNames = @("default", "broad", "appcap")
        }
    }

    $profiles = @()
    foreach ($profileName in $selectedProfileNames) {
        if (-not $profileMap.ContainsKey($profileName)) {
            throw "Unknown HZ6 profile: $profileName"
        }
        $profiles += $profileMap[$profileName]
    }

    $selectedLanes = @()
    foreach ($laneName in $selectedLaneNames) {
        if (-not $laneMap.ContainsKey($laneName)) {
            throw "Unknown HZ6 capacity lane: $laneName"
        }
        $selectedLanes += $laneMap[$laneName]
    }

    foreach ($profile in $profiles) {
        foreach ($variant in $selectedLanes) {
            $output = Join-Path $OutDir ("{0}_hz6_{1}{2}.exe" -f $OutputPrefix, $profile.Name, $variant.Suffix)
            $args = @()
            $args += $commonFlags
            $args += "/DHZ_BENCH_USE_HZ6"
            $args += ("/DHZ_BENCH_HZ6_PROFILE={0}" -f $profile.Define)
            if ($DiagnosticHz6Probes) {
                $args += "/DHZ6_DIAGNOSTIC_PROBES=1"
            }
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
