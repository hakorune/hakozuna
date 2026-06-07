param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [int]$TimeoutSeconds = 180,
    [switch]$SkipBuild,
    [switch]$ForceBuild,
    [switch]$DiagnosticHz6Probes,
    [switch]$ContinueOnFailure,
    [switch]$SelectedFamily,
    [switch]$SelectedFamilyGuard,
    [switch]$RandomSameOwnerTrustedFreeGuard,
    [switch]$SelectedSmallFixed,
    [switch]$SelectedSmallFixedHybridExactUpper,
    [switch]$LargeDirectRetainControl,
    [switch]$LarsonCrossOwnerSelected,
    [switch]$LarsonElasticLowRssSelected,
    [switch]$LarsonElasticFrontcacheGuard,
    [switch]$LarsonElasticRehomeBudgetGuard,
    [switch]$LarsonCrossOwnerLowestRss,
    [switch]$LarsonMetadataSlim,
    [switch]$LarsonSourceRunMetaSlim,
    [switch]$LarsonRssResidualAudit,
    [switch]$LarsonRun512RouteSlim,
    [switch]$LarsonRun512DescSlim,
    [switch]$LarsonRun512DescriptorLayout,
    [switch]$LarsonThinDescSourceCap,
    [switch]$ListOnly,
    [switch]$ListPresets
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$MatrixScript = Join-Path $PSScriptRoot "run_win_hz6_capacity_matrix.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper\hz6_selected_family"
}

function New-Preset {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$Families,
        [Parameter(Mandatory = $true)][string[]]$BenchmarkProfiles,
        [Parameter(Mandatory = $true)][string[]]$Hz6Profiles,
        [Parameter(Mandatory = $true)][string[]]$CapacityLanes,
        [string]$Note = ""
    )

    @{
        Name = $Name
        Families = $Families
        BenchmarkProfiles = $BenchmarkProfiles
        Hz6Profiles = $Hz6Profiles
        CapacityLanes = $CapacityLanes
        Note = $Note
    }
}

$presetMap = [ordered]@{
    "selected-mixed-lowrss" = New-Preset `
        -Name "selected-mixed-lowrss" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("balanced", "wide_ws") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry") `
        -Note "selected mixed_ws clean low-RSS row: desc17/route17 with linear wrap plus loop-carried route probing"

    "selected-mixed-pressure" = New-Preset `
        -Name "selected-mixed-pressure" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("balanced", "wide_ws") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("descavail-noboost-route4k") `
        -Note "balanced / wide_ws low-RSS pressure row; fast but not safety-clean"

    "selected-random-sameowner" = New-Preset `
        -Name "selected-random-sameowner" `
        -Families @("random_mixed") `
        -BenchmarkProfiles @("small", "medium", "mixed") `
        -Hz6Profiles @("strict") `
        -CapacityLanes @("sameownertrustedfree-descavail-noboost-route4k") `
        -Note "selected random_mixed same-owner speed lane with trusted local free"

    "random-sameowner-trustedfree-guard" = New-Preset `
        -Name "random-sameowner-trustedfree-guard" `
        -Families @("random_mixed") `
        -BenchmarkProfiles @("small", "medium", "mixed") `
        -Hz6Profiles @("strict") `
        -CapacityLanes @("sameownerfast-descavail-noboost-route4k", "sameownertrustedfree-descavail-noboost-route4k") `
        -Note "guard selected same-owner fast against SameOwnerTrustedLocalFree-L1 candidate"

    "selected-larger-lowrss" = New-Preset `
        -Name "selected-larger-lowrss" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("larger_sizes") `
        -Hz6Profiles @("speed", "rss") `
        -CapacityLanes @("largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "selected larger_sizes RSS/speed lane"

    "selected-small-fixed" = New-Preset `
        -Name "selected-small-fixed" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("large_slice_256", "large_slice_512", "large_slice_1k", "large_slice_2k", "large_slice_4k", "large_slice_8k", "large_slice_16k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "selected-small candidate: SourceBlockRoute dynmap over DirectLocalFreeReuse; repeat-3 improves balanced/wide/larger/4K/16K with small 8K regression"

    "selected-small-fixed-hybrid-lower" = New-Preset `
        -Name "selected-small-fixed-hybrid-lower" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("large_slice_256", "large_slice_512", "large_slice_1k", "large_slice_2k", "large_slice_4k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "selected-small hybrid control lower rows: keep DirectLocalFreeReuse for 256B..4K"

    "selected-small-fixed-hybrid-upper-exact" = New-Preset `
        -Name "selected-small-fixed-hybrid-upper-exact" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("large_slice_8k", "large_slice_16k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "selected-small hybrid control upper rows: DirectLocalExact for 8K..16K only; not a single-binary default"

    "large-direct-retain-control" = New-Preset `
        -Name "large-direct-retain-control" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("large_direct_slice_2m", "large_direct_slice_4m", "large_direct_slice_8m", "large_slice_1m", "large_slice_512k") `
        -Hz6Profiles @("speed", "rss") `
        -CapacityLanes @("largerlowrss-front8k-sourcerun-desc8k-route8k", "largedirectretain-largerlowrss-front8k-sourcerun-desc8k-route8k", "largedirectretain16m-largerlowrss-front8k-sourcerun-desc8k-route8k", "largedirectretain32m-largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "LargeDirectRetain cap ladder: compare base, 8M default, 16M, and 32M retained reuse while guarding 512K/1M LargeSpan rows"

    "larson-cross-owner-selected" = New-Preset `
        -Name "larson-cross-owner-selected" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k", "ownerlocalityfast-rsscap-2-desc160k-front4k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512") `
        -Note "Larson full 10k selected lane plus OwnerSourceSideMeta-L2 lowest-RSS sibling, FrontCachePackedMeta-L1, SourceBlockPackedFlags-L1, and combined packed lower-RSS candidates"

    "larson-elastic-lowrss-selected" = New-Preset `
        -Name "larson-elastic-lowrss-selected" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front1k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512") `
        -Note "Larson ElasticCapacity low-RSS siblings: front4k speed-balance control plus front1k selected lower-RSS sibling; front1k repeat-3 guard saves about 20.7 MiB on every main/worker 1k/4k/10k row with small speed variance and safety clean"

    "larson-elastic-frontcache-guard" = New-Preset `
        -Name "larson-elastic-frontcache-guard" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k", "larson_t16_worker_10k", "larson_t16_main_4k", "larson_t16_worker_4k", "larson_t16_main_1k", "larson_t16_worker_1k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front2k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front1k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512") `
        -Note "Larson Elastic frontcache-boundary guard: front4k speed-balance control, front2k evidence/control, and front1k selected lower-RSS sibling across main/worker 1k/4k/10k"

    "larson-elastic-rehomebudget-guard" = New-Preset `
        -Name "larson-elastic-rehomebudget-guard" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k", "larson_t16_worker_10k", "larson_t16_main_4k", "larson_t16_worker_4k", "larson_t16_main_1k", "larson_t16_worker_1k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget512-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget1024-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096", "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096") `
        -Note "Larson ElasticCapacity composition guard: selected DirectFree/TrustedLocalCache versus bounded DepotDescriptorRehomeBudget2048 and DFTLC+rehome budget 512/1024/2048 candidate-controls"

    "larson-cross-owner-lowest-rss" = New-Preset `
        -Name "larson-cross-owner-lowest-rss" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512") `
        -Note "front4k versus route192k, no-backptr, dir192k/source-block controls, routepacked, routebytes16 control, storageowner16 evidence, OwnerSourceSideMeta-L2 selected sibling, FrontCachePackedMeta-L1, SourceBlockPackedFlags-L1, and combined packed lower-RSS candidates"

    "larson-rss-residual-audit" = New-Preset `
        -Name "larson-rss-residual-audit" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512") `
        -Note "diagnostic-only residual RSS audit: L2 balance sibling, FrontCachePacked, SourceBlockPacked, and combined packed minimum-RSS candidate"

    "larson-thindesc-sourcecap" = New-Preset `
        -Name "larson-thindesc-sourcecap" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k") `
        -Note "thindesc source-block capacity recovery check after full-10k warmup failure"

    "larson-metadata-slim" = New-Preset `
        -Name "larson-metadata-slim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k") `
        -Note "MetadataSlim-L1 route-table capacity ladder for the selected thindesc-source16k Larson lane"

    "larson-sourcerun-metaslim" = New-Preset `
        -Name "larson-sourcerun-metaslim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512") `
        -Note "SourceBlockMetaSlim-L1 run-slot bitmap ladder on top of the selected route192k Larson lane"

    "larson-run512-routeslim" = New-Preset `
        -Name "larson-run512-routeslim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512") `
        -Note "Route-table capacity re-check after the selected run512 source-run metadata slim lane"

    "larson-run512-descslim" = New-Preset `
        -Name "larson-run512-descslim" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512") `
        -Note "Descriptor-table capacity re-check after route192k-run512 closed the static route trim path"

    "larson-run512-descriptorlayout" = New-Preset `
        -Name "larson-run512-descriptorlayout" `
        -Families @("larson") `
        -BenchmarkProfiles @("larson_t16_main_10k") `
        -Hz6Profiles @("speed") `
        -CapacityLanes @("ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512", "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512") `
        -Note "Descriptor/source-block layout: baseline, no-backptr, source-block no-route-backptr, routepacked, routebytes16, storageowner16, and side-owner16 controls"

    "selected-family-guard" = New-Preset `
        -Name "selected-family-guard" `
        -Families @("mixed_ws") `
        -BenchmarkProfiles @("smoke", "balanced", "wide_ws", "larger_sizes") `
        -Hz6Profiles @("rss") `
        -CapacityLanes @("route4k", "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry", "descavail-noboost-route4k", "largerlowrss-front8k-sourcerun-desc8k-route8k") `
        -Note "short guard/control slice before a broader selected-family run"
}

function Write-PresetList {
    foreach ($preset in $presetMap.Values) {
        Write-Host ("{0}: {1}" -f $preset.Name, $preset.Note)
        Write-Host ("  Families: {0}" -f ($preset.Families -join ","))
        Write-Host ("  BenchmarkProfiles: {0}" -f ($preset.BenchmarkProfiles -join ","))
        Write-Host ("  Hz6Profiles: {0}" -f ($preset.Hz6Profiles -join ","))
        Write-Host ("  CapacityLanes: {0}" -f ($preset.CapacityLanes -join ","))
    }
}

if ($ListPresets) {
    Write-PresetList
    return
}

$selectedPresetNames = New-Object System.Collections.Generic.List[string]

if ($LarsonCrossOwnerLowestRss) {
    [void]$selectedPresetNames.Add("larson-cross-owner-lowest-rss")
}

if ($LarsonThinDescSourceCap) {
    [void]$selectedPresetNames.Add("larson-thindesc-sourcecap")
}

if ($LarsonMetadataSlim) {
    [void]$selectedPresetNames.Add("larson-metadata-slim")
}

if ($LarsonSourceRunMetaSlim) {
    [void]$selectedPresetNames.Add("larson-sourcerun-metaslim")
}

if ($LarsonRssResidualAudit) {
    [void]$selectedPresetNames.Add("larson-rss-residual-audit")
    $DiagnosticHz6Probes = $true
}

if ($LarsonRun512RouteSlim) {
    [void]$selectedPresetNames.Add("larson-run512-routeslim")
}

if ($LarsonRun512DescSlim) {
    [void]$selectedPresetNames.Add("larson-run512-descslim")
}

if ($LarsonRun512DescriptorLayout) {
    [void]$selectedPresetNames.Add("larson-run512-descriptorlayout")
}

if ($LarsonCrossOwnerSelected) {
    [void]$selectedPresetNames.Add("larson-cross-owner-selected")
}

if ($LarsonElasticLowRssSelected) {
    [void]$selectedPresetNames.Add("larson-elastic-lowrss-selected")
}

if ($LarsonElasticFrontcacheGuard) {
    [void]$selectedPresetNames.Add("larson-elastic-frontcache-guard")
}

if ($LarsonElasticRehomeBudgetGuard) {
    [void]$selectedPresetNames.Add("larson-elastic-rehomebudget-guard")
}

if ($SelectedFamilyGuard) {
    [void]$selectedPresetNames.Add("selected-family-guard")
}

if ($RandomSameOwnerTrustedFreeGuard) {
    [void]$selectedPresetNames.Add("random-sameowner-trustedfree-guard")
}

if ($SelectedSmallFixed) {
    [void]$selectedPresetNames.Add("selected-small-fixed")
}

if ($SelectedSmallFixedHybridExactUpper) {
    [void]$selectedPresetNames.Add("selected-small-fixed-hybrid-lower")
    [void]$selectedPresetNames.Add("selected-small-fixed-hybrid-upper-exact")
}

if ($LargeDirectRetainControl) {
    [void]$selectedPresetNames.Add("large-direct-retain-control")
}

if ($SelectedFamily) {
    foreach ($name in @(
        "selected-mixed-lowrss",
        "selected-random-sameowner",
        "selected-small-fixed",
        "selected-larger-lowrss",
        "larson-cross-owner-selected",
        "larson-elastic-lowrss-selected"
    )) {
        [void]$selectedPresetNames.Add($name)
    }
}

if ($selectedPresetNames.Count -eq 0) {
    [void]$selectedPresetNames.Add("selected-family-guard")
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null

$seen = @{}
foreach ($presetName in $selectedPresetNames) {
    if ($seen.ContainsKey($presetName)) {
        continue
    }
    $seen[$presetName] = $true
    if (-not $presetMap.Contains($presetName)) {
        throw "Unknown selected-family preset: $presetName"
    }

    $preset = $presetMap[$presetName]
    $presetOutputDir = Join-Path $OutputDir $preset.Name
    $matrixArgs = @{
        OutputDir = $presetOutputDir
        Runs = $Runs
        Families = $preset.Families
        BenchmarkProfiles = $preset.BenchmarkProfiles
        Hz6Profiles = $preset.Hz6Profiles
        CapacityLanes = $preset.CapacityLanes
        TimeoutSeconds = $TimeoutSeconds
        SkipBuild = [bool]$SkipBuild
        ForceBuild = [bool]$ForceBuild
        DiagnosticHz6Probes = [bool]$DiagnosticHz6Probes
        ContinueOnFailure = [bool]$ContinueOnFailure
        ListOnly = [bool]$ListOnly
    }

    Write-Host ("[selected-family] {0}: {1}" -f $preset.Name, $preset.Note)
    & $MatrixScript @matrixArgs
    if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        throw "run_win_hz6_capacity_matrix.ps1 failed for preset $($preset.Name) with exit code $LASTEXITCODE"
    }
}
