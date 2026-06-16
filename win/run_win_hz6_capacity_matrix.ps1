param(
    [string]$OutputDir,
    [int]$Runs = 3,
    [string[]]$Families,
    [string[]]$BenchmarkProfiles,
    [string[]]$Hz6Profiles,
    [string[]]$CapacityLanes,
    [int[]]$ThreadCounts,
    [int]$TimeoutSeconds = 120,
    [switch]$SkipBuild,
    [switch]$ForceBuild,
    [switch]$DiagnosticHz6Probes,
    [switch]$ContinueOnFailure,
    [switch]$ListOnly
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SuiteDirName = if ($DiagnosticHz6Probes) { "out_win_hz6_capacity_diag" } else { "out_win_hz6_capacity" }
$SuiteDir = Join-Path $RepoRoot $SuiteDirName
$BuildScript = Join-Path $PSScriptRoot "build_win_hz6_capacity_suite.ps1"

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "docs\benchmarks\windows\paper"
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
. (Join-Path $scriptDir "run_win_hz6_capacity_matrix_helpers.ps1")
$selectedFamilies = Split-List -Values $Families
if (-not $selectedFamilies -or $selectedFamilies.Count -eq 0) {
    $selectedFamilies = @("mixed_ws", "random_mixed", "larson", "redis")
}

$selectedHz6Profiles = Split-List -Values $Hz6Profiles
if (-not $selectedHz6Profiles -or $selectedHz6Profiles.Count -eq 0) {
    $selectedHz6Profiles = @("strict", "speed", "rss")
}

$selectedLanes = Split-List -Values $CapacityLanes
if (-not $selectedLanes -or $selectedLanes.Count -eq 0) {
    $selectedLanes = @("control", "route4k", "appcap")
}

if (-not $ThreadCounts -or $ThreadCounts.Count -eq 0) {
    $ThreadCounts = @(1, 4, 8, 16)
}

$laneSuffix = @{
    "default" = ""
    "broad" = "_broad"
    "control" = "_control"
    "route4k" = "_route4k"
    "noboost-route4k" = "_noboost_route4k"
    "localexactfree-noboost-route4k" = "_localexactfree_noboost_route4k"
    "directlocalfree-noboost-route4k" = "_directlocalfree_noboost_route4k"
    "descavail-noboost-route4k" = "_descavail_noboost_route4k"
    "directlocalfree-descavail-noboost-route4k" = "_directlocalfree_descavail_noboost_route4k"
    "directlocalalloc-descavail-noboost-route4k" = "_directlocalalloc_descavail_noboost_route4k"
    "directlocalreuse-descavail-noboost-route4k" = "_directlocalreuse_descavail_noboost_route4k"
    "directlocalfreealloc-descavail-noboost-route4k" = "_directlocalfreealloc_descavail_noboost_route4k"
    "directlocalfreereuse-descavail-noboost-route4k" = "_directlocalfreereuse_descavail_noboost_route4k"
    "sameownerfast-descavail-noboost-route4k" = "_sameownerfast_descavail_noboost_route4k"
    "sameownertrustedfree-descavail-noboost-route4k" = "_sameownertrustedfree_descavail_noboost_route4k"
    "spill-route4k" = "_spill_route4k"
    "borrow-route4k" = "_borrow_route4k"
    "cap-route4k" = "_cap_route4k"
    "sourcerun-route4k" = "_sourcerun_route4k"
    "sourcerun-reclaim-route4k" = "_sourcerun_reclaim_route4k"
    "sourcerun-sameclass-route4k" = "_sourcerun_sameclass_route4k"
    "descriptorless-route4k" = "_descriptorless_route4k"
    "descriptorreserve-route4k" = "_descriptorreserve_route4k"
    "descriptorcold-route4k" = "_descriptorcold_route4k"
    "descriptorcoldgov-route4k" = "_descriptorcoldgov_route4k"
    "descriptorcoldgov-widews-route4k" = "_descriptorcoldgov_widews_route4k"
    "desc4k-route4k" = "_desc4k_route4k"
    "source512-route4k" = "_source512_route4k"
    "desc4k-source512-route4k" = "_desc4k_source512_route4k"
    "redislowrss-route4k" = "_redislowrss_route4k"
    "redislowrss-sourcerun-route4k" = "_redislowrss_sourcerun_route4k"
    "redislowrss-sourcerun-route8k" = "_redislowrss_sourcerun_route8k"
    "redislowrss-sourcerun-desc8k-route8k" = "_redislowrss_sourcerun_desc8k_route8k"
    "largerlowrss-sourcerun-desc8k-route8k" = "_largerlowrss_sourcerun_desc8k_route8k"
    "redislowrss-sourcerun-desc8k-route8k-tombcompact" = "_redislowrss_sourcerun_desc8k_route8k_tombcompact"
    "redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr1024" = "_redislowrss_sourcerun_desc8k_route8k_tombcompact_aggr1024"
    "redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr2048" = "_redislowrss_sourcerun_desc8k_route8k_tombcompact_aggr2048"
    "redislowrss-sourcerun-desc8k-route8k-condtombdry" = "_redislowrss_sourcerun_desc8k_route8k_condtombdry"
    "redislowrss-sourcerun-desc8k-route8k-condtombcompact" = "_redislowrss_sourcerun_desc8k_route8k_condtombcompact"
    "redislowrss-sourcerun-desc8k-route8k-retainedoverflow" = "_redislowrss_sourcerun_desc8k_route8k_retainedoverflow"
    "redislowrss-sourcerun-desc8k-route8k-slotlookup" = "_redislowrss_sourcerun_desc8k_route8k_slotlookup"
    "largerlowrss-sourcerun-desc8k-route64k" = "_largerlowrss_sourcerun_desc8k_route64k"
    "largerlowrss-sourcerun-desc8k-source2k-route64k" = "_largerlowrss_sourcerun_desc8k_source2k_route64k"
    "largerlowrss-front8k-sourcerun-desc8k-route8k" = "_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "largedirectretain-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_largedirectretain_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "largedirectretain16m-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_largedirectretain16m_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "largedirectretain32m-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_largedirectretain32m_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sameownerfast_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalsmall8k-sameownerlarge-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalsmall8k_sameownerlarge_largerlowrss_front8k_sourcerun_desc8k_route8k"
    # Selected-small candidate/control family. Keep trusted/packed/exact in
    # HZ6-only capacity runs unless later repeat evidence promotes them.
    "directlocalfree-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalfree_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalalloc-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalalloc_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalfreealloc-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalfreealloc_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "toysmallhotpathdiag-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_toysmallhotpathdiag_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "toysmallactivemap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_toysmallactivemap_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocaltrusted-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocaltrusted_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalpacked-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalpacked_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalexact_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-dryrun-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_dryrun_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-slotmap-dryrun-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_slotmap_dryrun_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-rangeindex-slotmap-dryrun-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_rangeindex_slotmap_dryrun_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-behavior-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_behavior_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_behavior_dynmap_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "toysmallhotpathdiag-sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_toysmallhotpathdiag_sourceblockroute_behavior_dynmap_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-behavior-dynmap-notoy-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_behavior_dynmap_notoy_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-dryrun-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_dryrun_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-behavior-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_behavior_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-behavior-min512-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_behavior_min512_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-behavior-range64k-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_behavior_range64k_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-behavior-range64k-toyonly-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_behavior_range64k_toyonly_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-behavior-range64k-toyarmed-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_behavior_range64k_toyarmed_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "smallrunroute-behavior-range64k-toyarmed-slotmax1k-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_smallrunroute_behavior_range64k_toyarmed_slotmax1k_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "sourceblockroute-behavior-dynmap-small8k-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_sourceblockroute_behavior_dynmap_small8k_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "toysmallactivemap-sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_toysmallactivemap_sourceblockroute_behavior_dynmap_directlocalfreereuse_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "directlocalfreereuse-small8k-largerlowrss-front8k-sourcerun-desc8k-route8k" = "_directlocalfreereuse_small8k_largerlowrss_front8k_sourcerun_desc8k_route8k"
    "largerlowrss-front6k-sourcerun-desc8k-route8k" = "_largerlowrss_front6k_sourcerun_desc8k_route8k"
    "largerlowrss-front4k-sourcerun-desc8k-route8k" = "_largerlowrss_front4k_sourcerun_desc8k_route8k"
    "mixedclean-front8k-sourcerun-desc16k-source2k-route16k" = "_mixedclean_front8k_sourcerun_desc16k_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc16k-source2k-route16k" = "_mixedclean_front16k_sourcerun_desc16k_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc16k-transfer2304-source2k-route16k" = "_mixedclean_front16k_sourcerun_desc16k_transfer2304_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc16k-transfer2560-source2k-route16k" = "_mixedclean_front16k_sourcerun_desc16k_transfer2560_source2k_route16k"
    "mixedclean-front16k-sourcerun-desc24k-source2k-route24k" = "_mixedclean_front16k_sourcerun_desc24k_source2k_route24k"
    "mixedclean-front16k-sourcerun-desc22k-source2k-route22k" = "_mixedclean_front16k_sourcerun_desc22k_source2k_route22k"
    "mixedclean-front16k-sourcerun-desc20k-source2k-route20k" = "_mixedclean_front16k_sourcerun_desc20k_source2k_route20k"
    "mixedclean-front16k-sourcerun-desc19k-source2k-route19k" = "_mixedclean_front16k_sourcerun_desc19k_source2k_route19k"
    "mixedclean-front16k-sourcerun-desc18k-source2k-route18k" = "_mixedclean_front16k_sourcerun_desc18k_source2k_route18k"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route17k" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k_linearwrap"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route17k_linearwrap_loopcarry"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route4k-linearwrap-loopcarry" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route4k_linearwrap_loopcarry"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route8k-linearwrap-loopcarry" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route8k_linearwrap_loopcarry"
    "mixedclean-front16k-sourcerun-desc17k-source4k-route8k-linearwrap-loopcarry" = "_mixedclean_front16k_sourcerun_desc17k_source4k_route8k_linearwrap_loopcarry"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route16k-linearwrap-loopcarry" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route16k_linearwrap_loopcarry"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-doublehash" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_doublehash"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-hashxor" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_hashxor"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route18k-linearwrap" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route18k_linearwrap"
    "mixedclean-front16k-sourcerun-desc17k-source2k-route20k" = "_mixedclean_front16k_sourcerun_desc17k_source2k_route20k"
    "mixedclean-front16k-sourcerun-desc32k-source4k-route32k" = "_mixedclean_front16k_sourcerun_desc32k_source4k_route32k"
    "mixedclean-front16k-sourcerun-desc32k-source3k-route32k" = "_mixedclean_front16k_sourcerun_desc32k_source3k_route32k"
    "mixedclean-front8k-sourcerun-desc32k-source4k-route32k" = "_mixedclean_front8k_sourcerun_desc32k_source4k_route32k"
    "mixedclean-front12k-sourcerun-desc32k-source4k-route32k" = "_mixedclean_front12k_sourcerun_desc32k_source4k_route32k"
    "mixedclean-front16k-sourcerun-desc32k-source2k-route32k" = "_mixedclean_front16k_sourcerun_desc32k_source2k_route32k"
    "redislowrss-slim-route4k" = "_redislowrss_slim_route4k"
    "front1k-desc4k-source512-route4k" = "_front1k_desc4k_source512_route4k"
    "appcap" = "_appcap"
    "visiblefirst-appcap" = "_visiblefirst_appcap"
    "negativefilter-appcap" = "_negativefilter_appcap"
    "sharedir-appcap" = "_sharedir_appcap"
    "sharedirfirst-appcap" = "_sharedirfirst_appcap"
    "ownerlocality-appcap" = "_ownerlocality_appcap"
    "ownerlocalityfast-appcap" = "_ownerlocalityfast_appcap"
    "ownerlocalityfast-rsscap-1" = "_ownerlocalityfast_rsscap_1"
    "ownerlocalityfast-rsscap-2" = "_ownerlocalityfast_rsscap_2"
    "ownerlocalityfast-rsscap-2-desc192k" = "_ownerlocalityfast_rsscap_2_desc192k"
    "ownerlocalityfast-rsscap-2-desc160k" = "_ownerlocalityfast_rsscap_2_desc160k"
    "ownerlocalityfast-rsscap-2-desc160k-route128k" = "_ownerlocalityfast_rsscap_2_desc160k_route128k"
    "ownerlocalityfast-rsscap-2-desc160k-source2k" = "_ownerlocalityfast_rsscap_2_desc160k_source2k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source12k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source14k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route128k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route224k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run2048"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run1024"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir192k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_dir192k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_noroutebackptr_dir192k_routepacked_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_osdry_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_sbp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source2k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s2_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source8k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s8_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source12k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s12_r192_run512"
    "ownerlocalityfast-rsscap-2-elasticproj-local1k-route16k-source64-front1k-packed" = "_ownerloc_rss2_elproj_l1k_r16k_s64_f1k_packed"
    "ownerlocalityfast-rsscap-2-elasticproj-live2k-route16k-source128-front1k-packed" = "_ownerloc_rss2_elproj_l2k_r16k_s128_f1k_packed"
    "ownerlocalityfast-rsscap-2-elasticroute-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512" = "_ownerloc_rss2_elroute_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescroute-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route16k-run512" = "_ownerloc_rss2_eldescroute_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s10_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-localizedryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_locdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-runlocalitydryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_runlocdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_depotrunmeta_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_slotownerdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownersparse-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_slotownersparse_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownerloc_rss2_eldescsrcroute_depotownerdirect_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_depotdirect_trustedlocalcache"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_depotdirect_directfree_trustedlocalcache"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front2k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_depotdirect_dftlc_front2k"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front1k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_depotdirect_dftlc_front1k"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdraindryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdraindry_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotslotlocalize-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotslotlocalize_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotslottransfer-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotslottransfer_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehomedry-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdescrehomedry_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotroutereplacedry-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotroutereplacedry_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdescrehome_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-capfree-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdescrehome_capfree_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdescrehome_budget2048_depotrunmeta_direct"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget512-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdirect_dftlc_depotdescrehome_budget512"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget1024-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdirect_dftlc_depotdescrehome_budget1024"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdirect_dftlc_depotdescrehome_budget2048"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-intersectiondryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_depotdirect_dftlc_depotdescrehome_budget2048_interdry"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerconsumerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_slotownerconsumerdry_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerlogical-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096" = "_ownerloc_rss2_eldescsrcroute_slotownerlogical_d16_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2_fcp_sbp_s64_r16_run4096"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-ownerequalcallsite-dryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_ownereqsite_depotdirect"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-freelocalownerpredicate-dryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_flcownerpred_depotdirect"
    "ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-depotownerequal-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512" = "_depotownereq_depotdirect"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2dryrun-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_nrb_d192_rp_rb16_so16_osl2dry_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512" = "_ownerloc_rss2_d160_f4_thin_nb_so16_nrb_d192_rp_s16_r192_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir128k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir128k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir96k-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_dir96k_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_nobackptr_sideowner16_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc158k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc156k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc152k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc148k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc144k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512" = "_ownerlocalityfast_rsscap_2_desc128k_front4k_thindesc_source16k_route192k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route160k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route160k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route128k_run512"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route96k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route96k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route64k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source16k_route64k"
    "ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k" = "_ownerlocalityfast_rsscap_2_desc160k_front4k_thindesc_source32k"
    "ownerlocalityfast-rsscap-2-desc144k" = "_ownerlocalityfast_rsscap_2_desc144k"
    "ownerlocalityfast-rsscap-3" = "_ownerlocalityfast_rsscap_3"
    "ownerlocalityfast-rsscap-4" = "_ownerlocalityfast_rsscap_4"
    "directlocalfree-ownerlocalityfast-rsscap-4" = "_directlocalfree_ownerlocalityfast_rsscap_4"
    "ownerlocalityfast-widecap-1" = "_ownerlocalityfast_widecap_1"
    "ownerlocalityfast-widecap-2" = "_ownerlocalityfast_widecap_2"
    "ownerlocalityfast-widecap-3" = "_ownerlocalityfast_widecap_3"
    "ownerlocalityfast-widecap-4" = "_ownerlocalityfast_widecap_4"
    "directlocalfree-ownerlocalityfast-widecap-4" = "_directlocalfree_ownerlocalityfast_widecap_4"
}

$familyPrefixes = @{
    "mixed_ws" = "bench_mixed_ws"
    "random_mixed" = "bench_random_mixed"
    "larson" = "bench_larson"
    "redis" = "bench_redis_workload"
}

$Executables = @()
foreach ($family in $selectedFamilies) {
    if (-not $familyPrefixes.ContainsKey($family)) {
        throw "Unknown benchmark family: $family"
    }
    foreach ($profile in $selectedHz6Profiles) {
        foreach ($lane in $selectedLanes) {
            if (-not $laneSuffix.ContainsKey($lane)) {
                throw "Unknown HZ6 capacity lane: $lane"
            }
            $name = "hz6-$profile"
            if ($lane -ne "default") {
                $name += "-$lane"
            }
            $Executables += @{
                Family = $family
                Name = $name
                Path = (Join-Path $SuiteDir ("{0}_hz6_{1}{2}.exe" -f $familyPrefixes[$family], $profile, $laneSuffix[$lane]))
            }
        }
    }
}

if ($ListOnly) {
    $Executables | ForEach-Object { "{0}`t{1}`t{2}" -f $_.Family, $_.Name, $_.Path }
    return
}

if (-not $SkipBuild) {
    $missing = @($Executables | Where-Object { -not (Test-Path $_.Path) })
    if ($ForceBuild -or $missing.Count -gt 0) {
        & $BuildScript `
            -Families $selectedFamilies `
            -Hz6Profiles $selectedHz6Profiles `
            -CapacityLanes $selectedLanes `
            -OutDirName $SuiteDirName `
            -DiagnosticHz6Probes:$DiagnosticHz6Probes
        if ($LASTEXITCODE -ne 0) {
            throw "build_win_hz6_capacity_suite.ps1 failed with exit code $LASTEXITCODE"
        }
    }
}

foreach ($exe in $Executables) {
    if (-not (Test-Path $exe.Path)) {
        throw "Missing HZ6 executable: $($exe.Path)"
    }
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null
$Stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$SummaryPath = Join-Path $OutputDir ($Stamp + "_hz6_capacity_matrix_windows.md")
$RawLogPath = Join-Path $OutputDir ($Stamp + "_hz6_capacity_matrix_windows.log")
$Summary = New-Object System.Collections.Generic.List[string]
$Utf8BomEncoding = New-Object System.Text.UTF8Encoding $true
$RawLogWriter = New-Object System.IO.StreamWriter($RawLogPath, $false, $Utf8BomEncoding)
$RawLogWriter.AutoFlush = $true

$Summary.Add("# Windows HZ6 Capacity Matrix")
$Summary.Add("")
$Summary.Add("Generated: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss zzz"))
$Summary.Add("")
$Summary.Add("- artifacts: [$SuiteDirName]($SuiteDir)")
$Summary.Add(('- families: `{0}`' -f ($selectedFamilies -join ", ")))
$Summary.Add(('- HZ6 profiles: `{0}`' -f ($selectedHz6Profiles -join ", ")))
$Summary.Add(('- capacity lanes: `{0}`' -f ($selectedLanes -join ", ")))
$Summary.Add(('- diagnostic probes: `{0}`' -f ([string]$DiagnosticHz6Probes)))
$Summary.Add("")

$mixedProfiles = @(
    @{ Name = "smoke"; Args = @("2", "10000", "256", "16", "128"); Note = "quick sanity and regression check" },
    @{ Name = "balanced"; Args = @("8", "250000", "4096", "16", "2048"); Note = "larger mixed run for first Windows compare" },
    @{ Name = "wide_ws"; Args = @("8", "200000", "16384", "16", "1024"); Note = "wider working-set pressure" },
    @{ Name = "larger_sizes"; Args = @("4", "150000", "4096", "256", "8192"); Note = "shift toward larger allocations" },
    @{ Name = "large_slice_256"; Args = @("4", "150000", "4096", "256", "256"); Note = "fixed-size large slice: 256 bytes" },
    @{ Name = "large_slice_512"; Args = @("4", "150000", "4096", "512", "512"); Note = "fixed-size large slice: 512 bytes" },
    @{ Name = "large_slice_1k"; Args = @("4", "150000", "4096", "1024", "1024"); Note = "fixed-size large slice: 1 KiB" },
    @{ Name = "large_slice_2k"; Args = @("4", "150000", "4096", "2048", "2048"); Note = "fixed-size large slice: 2 KiB" },
    @{ Name = "large_slice_4k"; Args = @("4", "120000", "2048", "4096", "4096"); Note = "fixed-size large slice: 4 KiB" },
    @{ Name = "large_slice_8k"; Args = @("4", "100000", "1024", "8192", "8192"); Note = "fixed-size large slice: 8 KiB" },
    @{ Name = "large_slice_16k"; Args = @("4", "80000", "512", "16384", "16384"); Note = "fixed-size large slice: 16 KiB" },
    @{ Name = "large_slice_32k"; Args = @("4", "60000", "256", "32768", "32768"); Note = "fixed-size large slice: 32 KiB" },
    @{ Name = "large_slice_64k"; Args = @("4", "50000", "128", "65536", "65536"); Note = "fixed-size large slice: 64 KiB" },
    @{ Name = "large_slice_128k"; Args = @("4", "40000", "64", "131072", "131072"); Note = "fixed-size large slice: 128 KiB" },
    @{ Name = "large_slice_256k"; Args = @("4", "30000", "32", "262144", "262144"); Note = "fixed-size large slice: 256 KiB" },
    @{ Name = "large_slice_512k"; Args = @("4", "20000", "16", "524288", "524288"); Note = "fixed-size large slice: 512 KiB" },
    @{ Name = "large_slice_1m"; Args = @("4", "12000", "8", "1048576", "1048576"); Note = "fixed-size large slice: 1 MiB" },
    @{ Name = "large_direct_slice_2m"; Args = @("4", "8000", "4", "2097152", "2097152"); Note = "fixed-size direct large slice: 2 MiB" },
    @{ Name = "large_direct_slice_4m"; Args = @("4", "5000", "3", "4194304", "4194304"); Note = "fixed-size direct large slice: 4 MiB" },
    @{ Name = "large_direct_slice_8m"; Args = @("4", "3000", "2", "8388608", "8388608"); Note = "fixed-size direct large slice: 8 MiB" },
    @{ Name = "heavy_mixed"; Args = @("8", "5000000", "16384", "16", "4096"); Note = "heavier mixed run with longer timings" }
)
$randomProfiles = @(
    @{ Name = "small"; Args = @("20000000", "400", "16", "2048"); Note = "paper SSOT small range" },
    @{ Name = "medium"; Args = @("20000000", "400", "4096", "32768"); Note = "paper SSOT medium range" },
    @{ Name = "mixed"; Args = @("20000000", "400", "16", "32768"); Note = "paper SSOT mixed range" }
)

. (Join-Path $PSScriptRoot "run_win_hz6_capacity_matrix_tail.ps1")
