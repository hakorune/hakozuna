# HZ6 Lane Guide

This document is the compact lane map for HZ6 Windows benchmarking. Keep long
experiment notes in `current_task.md`; use this file to answer "which lane is
which?" before running or comparing benchmarks.

For the short selected-row readout, see
[`HZ6_SELECTED_FAMILY_SUMMARY.md`](HZ6_SELECTED_FAMILY_SUMMARY.md).
For repo cleanup rules and the source modularization backlog, see
[`HZ6_REPO_HYGIENE.md`](HZ6_REPO_HYGIENE.md) and
[`HZ6_SOURCE_MODULARIZATION.md`](HZ6_SOURCE_MODULARIZATION.md).

## Current Recommendation

| Profile family | Selected HZ6 profile | Selected capacity lane | Why this lane now |
| --- | --- | --- | --- |
| balanced clean low-RSS | `rss` | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Selected mixed_ws clean low-RSS row. LinearWrap-L1 preserves linear probing semantics, and LoopCarry-L1 keeps the probe index loop-carried in hot exact route paths. Repeat-3: balanced `67.462M / 110888 KB`, safety clean. |
| wide_ws clean low-RSS | `rss` | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Same selected mixed_ws row also improves wide_ws in the loopcarry repeat-3: `22.674M / 140320 KB`, safety clean and still lower RSS than the old route18 sibling. |
| balanced / wide_ws pressure evidence | `rss` | `descavail-noboost-route4k` | Very fast and very low-RSS, but not safety-clean for paper/default claims: it completes by hitting large `alloc_fail` / source-block exhaustion counts. Keep it as pressure evidence only. |
| random_mixed same-owner speed | `strict` | `sameownerfast-descavail-noboost-route4k` | Selected same-owner fast lane: `HZ6_SAME_OWNER_FAST_L1` + descriptor availability, promoted from the A-ladder. |
| larger_sizes RSS/speed | `speed` or `rss` | `largerlowrss-front8k-sourcerun-desc8k-route8k` | Best larger_sizes lane; needs larger front retention, not more descriptor-failure cleanup. |
| Larson cross-owner full 10k | `speed` | `ownerlocalityfast-rsscap-2-desc160k` | Full Larson cross-owner throughput/RSS balance lane; appcap-class throughput with sub-1GB peak RSS. |
| Larson cross-owner low RSS | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k` | Clean route-capacity control. It keeps the thindesc/source16k shape and trims route capacity to 192K; repeat-3 is safety-clean at about `44.610M / 628844 KB`. |
| Larson cross-owner lowest-RSS balance sibling | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | Current selected lowest-RSS balance sibling. It combines routebytes16 with StorageOwner16 ownerless descriptors and OwnerSourceSideMeta-L2 source-block storage hints. Same-run full-10k repeat-3: routebytes16 control `40.750M / 449128 KB`, L2 `40.754M / 439912 KB`, safety clean. Worker-warmup run=1: routebytes16 `40.126M / 448948 KB`, L2 `40.787M / 439740 KB`. |
| Larson frontcache packed meta component | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512` | Lower-RSS component candidate/control, not broad throughput promotion. `HZ6_FRONTCACHE_PACKED_META_L1` shrinks `Hz6FrontCacheEntry` from 32 to 24 bytes by packing bytes-minus-one, class id, and descriptor-cold-governor detached flag into a 32-bit meta word. SourceBlockPacked closeout matrix: `41.131M / 430692 KB`, safety clean. |
| Larson sourceblock packed flags component | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512` | Lower-RSS SourceBlock metadata component candidate/control over OwnerSourceSideMeta-L2. `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1` packs SourceBlock `source_kind / active / route_registered / run_active` into one flag word while keeping `source_release` and OwnerSourceSideMeta-L2 inline. Repeat-3 closeout: `41.070M / 435304 KB`, safety clean. |
| Larson combined packed minimum-RSS candidate | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512` | Clean minimum-RSS candidate over OwnerSourceSideMeta-L2. It combines `HZ6_FRONTCACHE_PACKED_META_L1` and `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1`. Repeat-3 full 10k: `40.837M / 426084 KB`, safety clean. Lower RSS than FrontCachePacked alone (`41.131M / 430692 KB`) and SourceBlockPacked alone (`41.070M / 435304 KB`). Keep as minimum-RSS sibling/candidate, not broad throughput promotion. |
| Larson owner-source side metadata dry-run | `speed + diagnostic` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512` | Diagnostic-only follow-up to the routebytes16 comparison-control lane. Full 10k run=1: `46.202M / 449164 KB`, safety clean, `owner_source_side_meta_foreign=871979714`, `miss=0`, `probe_max=1`. The next owner-side design must be O(1), not StorageOwner16 scan-based. |
| Larson routebytes16 control | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512` | Superseded clean control. Same-run full-10k repeat-3 against OwnerSourceSideMeta-L2: routebytes16 `40.750M / 449128 KB`, L2 `40.754M / 439912 KB`. Keep as the main comparison control for L2. |
| Larson cross-owner routepacked control | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512` | RoutePackedMeta-L1 comparison control. Repeat-3 historical full 10k was `47.616M / 456048 KB`; same-run L2 A/B repeat-3 is `45.079M / 456040 KB`, safety clean. Superseded first by routebytes16 as the route-entry comparison control, then by OwnerSourceSideMeta-L2 for selected lowest-RSS comparisons. |
| Larson cross-owner RSS-first descriptor evidence | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512` | StorageOwner16-L1 evidence/control. It is safety-clean and reaches `42.024M / 444520 KB`, but does not replace routepacked because the RSS gain costs about 12% throughput. |
| Larson cross-owner minimum RSS control | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | Clean SourceBlockNoRouteBackptr-L1 control. It removes the SourceBlock route-backend back-pointer and reaches `41.107M / 469868 KB`; superseded by routepacked for both speed and RSS, but useful as an isolation control. |
| Larson cross-owner no-backptr control | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512` | Comparison control for dir192k. It keeps route192k/run512 and removes the descriptor allocator back-pointer; repeat-3 is safety-clean at `40.710M / 476784 KB`, and same-run repeat-3 against dir192k is `45.310M / 476788 KB`. |
| Larson cross-owner run512 control | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512` | Previous selected lowest-RSS sibling/control. Repeat-3 is safety-clean at `48.512M / 499820 KB`; same-run no-backptr cuts another about 23 MB. |
| Larson descriptor boundary | `speed` | `ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512` | Clean tiny-RSS sibling/control after run512. Repeat-3 is `40.400M / 498080 KB`; desc156k and below are warmup no-go from `descriptor_exhausted=3` / `alloc_fail=1`. |
| perf-recovery upper-bound | `strict` / `speed` / `rss` | `ownerlocalityfast-appcap` | Upper-bound / completion control only; too much RSS for default use. |

For a cross-allocator side-by-side summary using past data only, see
[`HZ6_CROSS_ALLOCATOR_COMPARISON.md`](HZ6_CROSS_ALLOCATOR_COMPARISON.md).

## Selected-Family Runner Presets

Use `win/run_win_hz6_selected_family.ps1` when the goal is to measure HZ6 as a
profile family instead of opening another broad capacity sweep. The wrapper
delegates to `win/run_win_hz6_capacity_matrix.ps1` and keeps selected lanes
separate by output subdirectory.

```powershell
.\win\run_win_hz6_selected_family.ps1 -ListPresets
```

```powershell
# Paper/development selected-family slice.
.\win\run_win_hz6_selected_family.ps1 `
  -SelectedFamily `
  -Runs 3 `
  -TimeoutSeconds 240 `
  -ContinueOnFailure
```

```powershell
# Larson full-10k selected lane plus low-RSS siblings.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonCrossOwnerSelected `
  -Runs 3 `
  -TimeoutSeconds 240 `
  -ContinueOnFailure
```

```powershell
# Narrow low-RSS sibling check: front4k / route192k / no-backptr / routepacked.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonCrossOwnerLowestRss `
  -Runs 3 `
  -TimeoutSeconds 240 `
  -ContinueOnFailure
```

```powershell
# Source-block recovery check after thindesc full-10k warmup failure.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonThinDescSourceCap `
  -Runs 1 `
  -TimeoutSeconds 300 `
  -ContinueOnFailure
```

```powershell
# SourceBlockMetaSlim-L1 run bitmap ladder on the selected route192k lane.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonSourceRunMetaSlim `
  -Runs 1 `
  -TimeoutSeconds 300 `
  -ContinueOnFailure

# Route-capacity re-check after run512; evidence/no-go boundary only.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonRun512RouteSlim `
  -Runs 1 `
  -TimeoutSeconds 300 `
  -DiagnosticHz6Probes `
  -ContinueOnFailure

# Descriptor-capacity re-check after run512; evidence/no-go boundary only.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonRun512DescSlim `
  -Runs 1 `
  -TimeoutSeconds 300 `
  -DiagnosticHz6Probes `
  -ContinueOnFailure

# Descriptor-layout re-check after run512; no-backptr L1 versus baseline.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonRun512DescriptorLayout `
  -Runs 3 `
  -TimeoutSeconds 300 `
  -ContinueOnFailure
```

Preset intent:

```text
selected-mixed-lowrss:
  mixed_ws balanced / wide_ws
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry
  status:
    LinearWrap-L1 keeps the desc17/route17 RSS shape while improving route
    arithmetic. LoopCarry-L1 is the current selected refinement because it
    improves balanced, wide_ws, and larger_sizes against linearwrap in the
    same repeat-3 comparison.

selected-mixed-pressure:
  mixed_ws balanced / wide_ws
  rss + descavail-noboost-route4k
  status:
    pressure row only after the 2026-06-03 repeat-3 because alloc_fail and
    source_block_exhausted are intentionally large under this low-capacity
    shape.

selected-random-sameowner:
  random_mixed small / medium / mixed
  strict + sameownerfast-descavail-noboost-route4k

selected-larger-lowrss:
  mixed_ws larger_sizes
  speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k

larson-cross-owner-selected:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512

selected-family-guard:
  short mixed_ws smoke/control guard before a longer selected-family run
  includes:
    route4k control
    mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry
    descavail-noboost-route4k pressure evidence
    largerlowrss-front8k-sourcerun-desc8k-route8k larger_sizes selected row

larson-thindesc-sourcecap:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k
  use only as a source-block recovery experiment after thindesc full-10k
  warmup failure

larson-sourcerun-metaslim:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
  run512 is the previous selected lowest-RSS sibling/control after the
  SourceBlockMetaSlim-L1 repeat-3 clean result; no-backptr run512 superseded
  it as a descriptor-layout control. dir192k/no-backptr is now a
  directory-capacity comparison control, no-route-backptr/dir192k is an
  isolation control, and routebytes16/routepacked/no-routebackptr/dir192k is
  the route-entry comparison control; OwnerSourceSideMeta-L2 superseded it as
  the selected lowest-RSS balance sibling. FrontCachePackedMeta-L1 and
  SourceBlockPackedFlags-L1 are component lower-RSS controls/candidates, and
  the combined packed lane is the current minimum-RSS candidate/sibling.
  Run2048/run1024 remain SourceBlockMetaSlim-L1 controls.

larson-run512-routeslim:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512
  status:
    evidence/no-go boundary only. Route192k-run512 stays clean; route160k-run512
    and route128k-run512 saturate during warmup (`route_register_fail=3`,
    `alloc_fail=1`). Do not promote these route-trim siblings.

larson-run512-descslim:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512
  status:
    evidence/no-go boundary only. Desc158k is clean and saves only about
    1.7MB median peak versus desc160k; desc156k and below fail warmup with
    descriptor exhaustion. Do not continue static descriptor-capacity trimming
    under the current representation.

larson-run512-descriptorlayout:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512
  status:
    descriptor layout boundary. No-backptr removes the allocator pointer from
    `Hz6ObjectDescriptor` and requires descriptor lifecycle helpers to receive
    allocator explicitly. Repeat-3 is clean at `40.710M / 476784 KB` versus the
    run512 baseline `40.498M / 499812 KB`; keep as the descriptor-layout
    comparison control before the dir192k/routepacked refinements. Sideowner16
    is no-go/evidence only: it reaches a 32-byte hot descriptor but reports
    `route_invalid=11739`, `remote_free_transfer_fail=11739`, and
    `lifecycle_foreign_free_invalid=11739` because allocator-local side-owner
    metadata is not owner-source-aware.

larson-nobackptr-selected-guard:
  larson-cross-owner-lowest-rss preset one-run check after preset promotion.
  The selected preset now runs:
    front4k:
      42.460M / 716340 KB
    route192k:
      44.583M / 628848 KB
    no-backptr route192k-run512:
      42.324M / 476868 KB
  all safety clean. Use this as pre-routepacked wiring evidence that
  selected-family runners exercised no-backptr before the dir192k and
  routepacked promotions.

larson-descriptor-layout-l2-dryrun-clean:
  diagnostic-only descriptor layout projection on the run512/no-backptr pair.
  It adds no behavior change. Read:
    owner16 hot descriptor:
      40 bytes, no extra savings versus no-backptr
    ownerless hot descriptor:
      32 bytes, projected extra savings about 20 MiB versus no-backptr
  Therefore the next descriptor RSS candidate is side-owner / ownerless hot
  descriptor metadata, not 16-bit owner packing alone.
```

```text
Windows profile family:
  balanced / wide_ws low-RSS pressure row:
    HZ6 profile:
      rss
    capacity lane:
      descavail-noboost-route4k
    caveat:
      fast/low-RSS but not safety-clean under the selected-family repeat.
      Do not use as the clean default or paper row without the alloc-fail
      caveat.

  balanced / wide_ws clean low-RSS:
    HZ6 profile:
      rss
    balanced capacity lane:
      mixedclean-front16k-sourcerun-desc17k-source2k-route17k
    wide_ws capacity lane:
      mixedclean-front16k-sourcerun-desc17k-source2k-route18k
    speed sibling/control:
      mixedclean-front16k-sourcerun-desc18k-source2k-route18k
      mixedclean-front16k-sourcerun-desc20k-source2k-route20k
      mixedclean-front16k-sourcerun-desc32k-source2k-route32k
      mixedclean-front16k-sourcerun-desc32k-source4k-route32k
    read:
      selected clean mixed rows. Desc17/route17 is the balanced lower-RSS row.
      Desc17/route18 is the selected wide_ws sibling: route capacity is raised
      without raising descriptor or transfer capacity. Desc16 remains wide_ws
      no-go even when transfer capacity is widened. Desc18/20/24/32 remain
      controls for broader capacity / speed shape.

  random_mixed same-owner speed:
    HZ6 profile:
      strict
    capacity lane:
      sameownerfast-descavail-noboost-route4k

  larger_sizes-rss-speed:
    HZ6 profiles:
      speed or rss
    capacity lane:
      largerlowrss-front8k-sourcerun-desc8k-route8k
    close candidate-control:
      largerlowrss-front6k-sourcerun-desc8k-route8k

  Larson cross-owner full 10k:
    HZ6 profile:
      speed
    capacity lane:
      ownerlocalityfast-rsscap-2-desc160k
    stable near-capacity sibling:
      ownerlocalityfast-rsscap-2-desc192k
    selected lower-RSS sibling:
      ownerlocalityfast-rsscap-2-desc160k-front4k
    selected low-RSS sibling:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512
    previous run512 control:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
    descriptor-capacity boundary control:
      ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512
    route192k source16k control:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
    source16k route-capacity control:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
    route-capacity no-go controls:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k-run512
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k-run512
    descriptor-capacity no-go controls:
      ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512
      ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512
      ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512
      ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512
      ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512
    lower-RSS / lower-throughput source-cap control:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k
    source-block over-retention control:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k
    compact/moderate thindesc evidence:
      ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
      full 10k no-go until source-block capacity / retention is fixed
    stable controls:
      ownerlocalityfast-rsscap-1
      ownerlocalityfast-rsscap-2
    compact/moderate live-set evidence:
      ownerlocalityfast-rsscap-4

  perf-recovery upper-bound:
    ownerlocalityfast-appcap

  redis-evidence:
    redislowrss-sourcerun-desc8k-route8k
    redislowrss-sourcerun-desc8k-route8k-tombcompact

Primary controls:
  route4k
  noboost-route4k
  descavail-noboost-route4k
  mixedclean-front16k-sourcerun-desc17k-source2k-route17k

Low-capacity / low-RSS baseline:
  control

Redis-like candidate-control:
  redislowrss-route4k
  redislowrss-slim-route4k

Capacity / completion control:
  appcap

Route-lifecycle diagnostic:
  visiblefirst-appcap
  negativefilter-appcap
  sharedir-appcap
  sharedirfirst-appcap
  ownerlocality-appcap
  ownerlocalityfast-appcap
  ownerlocalityfast-rsscap-1
  ownerlocalityfast-rsscap-2
  ownerlocalityfast-rsscap-2-desc192k
  ownerlocalityfast-rsscap-2-desc160k
  ownerlocalityfast-rsscap-2-desc160k-front4k
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
  ownerlocalityfast-rsscap-2-desc160k-route128k
  ownerlocalityfast-rsscap-2-desc160k-source2k
  ownerlocalityfast-rsscap-2-desc144k
  ownerlocalityfast-rsscap-3
  ownerlocalityfast-rsscap-4
  ownerlocalityfast-widecap-1
  ownerlocalityfast-widecap-2
  ownerlocalityfast-widecap-3
  ownerlocalityfast-widecap-4

Evidence-only source-run controls:
  sourcerun-route4k
  sourcerun-sameclass-route4k

Descriptor lifecycle prototype:
  descavail-noboost-route4k
  sameownerfast-descavail-noboost-route4k

Same-owner A-ladder evidence:
  directlocalfree-descavail-noboost-route4k
  directlocalalloc-descavail-noboost-route4k
  directlocalreuse-descavail-noboost-route4k
  directlocalfreealloc-descavail-noboost-route4k
  directlocalfreereuse-descavail-noboost-route4k

Descriptor lifecycle evidence:
  descriptorless-route4k
  descriptorreserve-route4k
  descriptorcold-route4k
  descriptorcoldgov-route4k

Frozen no-go controls:
  spill-route4k
  borrow-route4k
  cap-route4k
  sourcerun-reclaim-route4k
```

## Recommended Lane Sets

Use these sets before opening another capacity sweep. They keep HZ6 as a
profile family and avoid accidentally promoting an evidence lane outside the
row where it was measured.

```text
balanced / wide_ws clean low-RSS:
  HZ6 profile:
    rss
  capacity lane:
    mixedclean-front16k-sourcerun-desc17k-source2k-route17k
  pressure evidence, not selected:
    descavail-noboost-route4k

random_mixed same-owner speed:
  HZ6 profile:
    strict
  capacity lane:
    sameownerfast-descavail-noboost-route4k

larger_sizes RSS/speed:
  HZ6 profile:
    speed for throughput, rss for lower RSS
  capacity lane:
    largerlowrss-front8k-sourcerun-desc8k-route8k
  tighter-retention candidate-control:
    largerlowrss-front6k-sourcerun-desc8k-route8k

Larson cross-owner full 10k:
  HZ6 profile:
    speed
  capacity lane:
    ownerlocalityfast-rsscap-2-desc160k
  stable near-capacity sibling:
    ownerlocalityfast-rsscap-2-desc192k
  selected lower-RSS sibling:
    ownerlocalityfast-rsscap-2-desc160k-front4k
    repeat-3 full 10k clean; use when about -1.3% throughput is acceptable for
    about 90MB lower peak RSS versus desc160k
  current selected low-RSS sibling:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512
    repeat-3 full 10k clean at about 40.754M ops/s and 439912 KB peak RSS.
    It combines dir192k, descriptor no-backptr, SourceBlock no-route-backptr,
    RoutePackedMeta-L1, RoutePackedMeta-L2 routebytes16, StorageOwner16, and
    OwnerSourceSideMeta-L2.
  lower-RSS candidate/sibling:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512
    direct closeout is about 44.831M ops/s and 430708 KB peak RSS; use only
    when about 9 MiB lower RSS is worth the measured throughput tradeoff.
  routebytes16 comparison control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512
    repeat-3 full 10k clean at about 48.367M ops/s and 449144 KB peak RSS.
  directory-capacity control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512
    repeat-3 full 10k clean at about 44.580M ops/s and 472176 KB peak RSS.
    It trims shared-route-directory / owner-locality capacity from 262K to
    192K while keeping route capacity at 192K.
  descriptor no-backptr comparison control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512
    repeat-3 full 10k clean; original no-backptr median is about 40.710M ops/s
    and 476784 KB peak RSS. Same-run control versus dir192k is about 45.310M
    ops/s and 476788 KB peak RSS.
  directory-capacity no-go controls:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir128k-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir96k-source16k-route192k-run512
    one-run safety counters stay clean, but owner-locality misses and huge
    full-table probes appear; speed regresses too much for selection.
  previous run512 control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
    repeat-3 full 10k clean; current median is about 48.512M ops/s and
    499820 KB peak RSS. Keep it as the descriptor no-backptr comparison
    baseline/control.
  descriptor-capacity boundary control:
    ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512
    repeat-3 full 10k clean at about 40.400M ops/s and 498080 KB peak RSS.
    Keep it as a tiny-RSS sibling/control, not a default replacement, because
    the RSS win versus desc160k is small and the next lower descriptor caps
    fail warmup.
  descriptor side-owner no-go:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512
    one-run diagnostic reaches `46.490M / 475672 KB` and a 32-byte hot
    descriptor entry, but it is not safety-clean. Keep it as evidence that
    side owner metadata needs the descriptor's owning allocator, not the
    current allocator.
  route192k control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k
    repeat-3 clean at about 44.610M ops/s and 628844 KB peak RSS.
  source16k baseline control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k
    repeat-3 full 10k clean; keep as the route-capacity control for the
    route192k metadata-slim result.
  lower-RSS / lower-throughput source-cap control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k
    repeat-3 full 10k clean; current median is about 44.308M ops/s and
    623084 KB peak RSS. Keep as a lowest-RSS control, not the selected
    throughput/RSS sibling.
  source-cap interpolation control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source14k
    run-1 full 10k clean at about 44.471M ops/s and 644836 KB peak RSS.
  route-capacity boundary controls:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route224k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route128k
    route224k is clean control; route160k/128k fail warmup and define the
    current no-go lower bound.
  descriptor-capacity boundary controls:
    ownerlocalityfast-rsscap-2-desc156k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc152k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc148k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc144k-front4k-thindesc-source16k-route192k-run512
    ownerlocalityfast-rsscap-2-desc128k-front4k-thindesc-source16k-route192k-run512
    desc156k and below fail warmup with `descriptor_exhausted=3` and
    `alloc_fail=1`; static descriptor capacity cuts are closed under the
    current descriptor representation.
  source-run metadata ladder:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run2048
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run1024
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512
    run512 is the previous selected lowest-RSS sibling/control. Run2048/run1024
    remain source-run metadata controls; no-backptr run512 is the descriptor
    layout control, dir192k/no-backptr is a directory-capacity control,
    routepacked/no-routebackptr/dir192k is the L1 control, and
    routebytes16 is the clean comparison-control sibling; OwnerSourceSideMeta-L2
    is the current selected low-RSS balance sibling, with FrontCachePackedMeta-L1
    as the lower-RSS candidate/sibling.
  source-block over-retention control:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k
    passes, but raises peak RSS and is not selected
  compact/moderate thindesc evidence:
    ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc
    post-pack 4k repeat-3 keeps safety clean, but full 10k fails during warmup
    with source_block_exhausted=257; do not use as a paper-facing full-10k
    claim yet
  no-go static-table controls:
    ownerlocalityfast-rsscap-2-desc160k-route128k
    ownerlocalityfast-rsscap-2-desc160k-source2k
  stable controls:
    ownerlocalityfast-rsscap-1
    ownerlocalityfast-rsscap-2
  compact/moderate live-set only:
    ownerlocalityfast-rsscap-4

performance upper-bound / completion control:
  ownerlocalityfast-appcap

generic safety/control baseline:
  route4k

capacity completion control:
  appcap

Redis evidence only:
  redislowrss-sourcerun-desc8k-route8k
  redislowrss-sourcerun-desc8k-route8k-tombcompact
  redislowrss-sourcerun-desc8k-route8k-slotlookup
```

Recommended comparison matrix for Windows HZ6 mixed profiles:

```powershell
.\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -BenchmarkProfiles balanced,wide_ws,larger_sizes `
  -Hz6Profiles strict,speed,rss `
  -CapacityLanes noboost-route4k,mixedclean-front16k-sourcerun-desc17k-source2k-route17k,descavail-noboost-route4k,sameownerfast-descavail-noboost-route4k,largerlowrss-front8k-sourcerun-desc8k-route8k,largerlowrss-front6k-sourcerun-desc8k-route8k,ownerlocalityfast-rsscap-4,ownerlocalityfast-appcap
```

Next Windows focus:

```text
Profile-family read:
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k is the
  current balanced / wide_ws clean low-RSS selected lane.
  rss + descavail-noboost-route4k is still useful as a very fast low-RSS
  pressure row, but it is not paper/default clean because it relies on large
  allocation-failure/source-block-exhaustion counts.
  strict + sameownerfast-descavail-noboost-route4k is the current
  random_mixed same-owner speed candidate-control.
  largerlowrss-front8k-sourcerun-desc8k-route8k is the current larger_sizes
  RSS/speed candidate-control.
  ownerlocalityfast-rsscap-2-desc160k is the current full Larson cross-owner
  candidate-control; ownerlocalityfast-rsscap-2-desc192k is the stable
  near-capacity sibling.
  ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k is the
  selected low-RSS Larson sibling after the source-block recovery run.
  ownerlocalityfast-rsscap-3/4 are too tight for full 10k Larson and should be
  used only for compact/moderate live-set evidence.
  ownerlocalityfast-rsscap-4 is now a historical larger_sizes high-RSS
  comparison control.
  ownerlocalityfast-appcap is the perf-recovery upper-bound / completion
  control, not the default.
  Redis lanes are frozen as evidence-only.

Wide / mixed profiles:
  noboost-route4k
  descavail-noboost-route4k
  sameownerfast-descavail-noboost-route4k
  largerlowrss-front8k-sourcerun-desc8k-route8k
  ownerlocalityfast-rsscap-4

Next experiment:
  do not start another static capacity sweep until the profile-family matrix
  above is enough for the paper/dev comparison table.

Do not:
  reopen Redis capacity tuning as the next broad HZ6 track
  treat ownerlocalityfast-appcap as promotion while peak working set remains
  appcap-sized
```

`ownerlocalityfast-rsscap-*` is the perf-recovery RSS reduction family, not an
existing promotion lane. The first larger_sizes D-lite read shows that the row
is `256..8192` mid/source-block pressure, not the LargeSpan front:
`large_span_central_push/pop/source_alloc` stay at zero.

```text
ownerlocalityfast-rsscap-1:
  transfer/frontcache trim only.
  Performance is preserved, but peak is essentially unchanged.
  Keep as no-op evidence.

ownerlocalityfast-rsscap-2:
  rsscap-1 plus source-block capacity 8192.
  larger_sizes run1 drops peak by roughly 57 MiB versus appcap and preserves
  or improves throughput with safety counters clean.
  Promising evidence; superseded by rsscap-3 if guard rows stay clean.

ownerlocalityfast-rsscap-3:
  rsscap-2 plus descriptor capacity 131072.
  larger_sizes run1 drops peak again to about 191..196 MiB with clean safety
  counters. Strong evidence; superseded by rsscap-4 on larger_sizes if guard
  rows are acceptable.

ownerlocalityfast-rsscap-4:
  rsscap-3 plus route capacity 131072.
  larger_sizes run1 and repeat-3 drop peak again to about 166..172 MiB and
  keep safety counters clean. Current strongest larger_sizes RSSCap
  candidate-control, but not broad promotion: wide_ws guard regresses versus
  ownerlocalityfast-appcap.

ownerlocalityfast-widecap-1:
  appcap descriptor / route / source-block capacity, with transfer/frontcache
  kept wide at 32768. Wide_ws restores appcap-class speed, but peak remains
  appcap-sized. Keep as wide hot-cache evidence.

ownerlocalityfast-widecap-2:
  appcap descriptor / route / source-block capacity, with transfer/frontcache
  at 16384. Wide_ws still restores most appcap-class speed with appcap-sized
  peak. Use as the narrow hot-cache baseline for further wide_ws RSS trims.

ownerlocalityfast-widecap-3:
  widecap-2 plus source-block capacity 8192. Wide_ws keeps speed near
  widecap-2 while dropping peak by roughly 110 MiB in speed/rss profiles.
  Current source-block trim evidence for wide_ws.

ownerlocalityfast-widecap-4:
  widecap-3 plus descriptor capacity 131072. Wide_ws drops peak further
  while keeping clean safety counters. Wide_ws repeat-3 keeps or improves
  throughput versus appcap / widecap-2 and lowers peak to about 351..773 MiB
  depending on strict/speed/rss profile. Balanced guard is acceptable.
  Larger_sizes guard is weaker than rsscap-4, so this is the current wide_ws
  RSS/speed candidate-control, not a broad promotion.
```

`route4k` is the current HZ6 Windows lane to use first when checking whether
HZ6 remains low-RSS while avoiding tiny-route-table artifacts. It is not a
promotion claim. It is the cleanest candidate-control shape so far.

`appcap` proves whether a workload can complete when HZ6 gets very large
descriptor / route / transfer / source-block / front-cache capacities. It is a
diagnostic capacity control, not the default production shape.

`visiblefirst-appcap` is a diagnostic-only route-lifecycle upper-bound test.
It uses appcap capacity plus `HZ6_VISIBLE_FIRST_FREE_L1=1` and must be built
with `HZ6_DIAGNOSTIC_PROBES=1`. Use it to measure whether skipping expensive
worker-local route MISS scans can recover Larson main-warmup; do not use it as
a production or paper-facing lane.

## Capacity Lanes

```text
default:
  Tiny R1 smoke/control shape. Useful for build sanity, not performance claims.

broad:
  Historical broad-capacity control. Often faster than tiny lanes, but can
  raise RSS substantially.

control:
  Low-capacity baseline: small descriptor / route / transfer / source-block /
  front-cache limits. Useful for seeing how low-RSS HZ6 behaves under pressure.

route4k:
  control plus a 4096-entry route table. This isolates route capacity while
  keeping other capacities near control values.

noboost-route4k:
  route4k plus `HZ6_SOURCE_ADMISSION_NO_STARVATION_BOOST=1`. This keeps the
  low-capacity shape but prevents alloc-fail pressure from increasing source
  refill batch size. Latest repeat-3 strongly improves balanced and wide_ws
  while preserving larger_sizes, and random_mixed repeat-3 is neutral to
  positive. Treat it as the current Windows low-capacity candidate-control
  lane.

redislowrss-route4k:
  Redis-like low-RSS candidate-control. It keeps the route table at 4096,
  combines descriptor capacity 4096 with source-block capacity 512, keeps
  frontcache/transfer modest, and disables starvation source-refill boost.
  L0 redis_short showed descriptor capacity is the main collapse point and
  descriptor+source-block capacity gives a better Redis SET/LPUSH/RANDOM shape
  without moving all the way to appcap. Mixed_ws guard strongly regresses, so
  keep this lane Redis-like only; do not use it as a general HZ6 primary lane.

redislowrss-slim-route4k:
  Redis-like slim candidate-control. It keeps the same 4096 route table and
  starvation-boost disable, but trims descriptor capacity to 2048 and
  source-block capacity to 256. Use this as the next narrow Redis experiment
  after `redislowrss-route4k` if we want a smaller RSS footprint or less
  retention pressure while keeping Redis-like completion behavior. Latest L0
  checks made it the better Redis-like balance than `redislowrss-route4k`,
  while mixed_ws/random_mixed still remain guards rather than adoption targets.
  The staged `redis_long` row completed on this lane at 29.0 MiB peak working
  set while `noboost-route4k` timed out, so treat this as BURST_SUPPLY
  upper-shape evidence for the future control plane rather than as a new
  general default. Diagnostic builds now also surface ControlPlane-R1 dry-run
  counters on Redis rows, which is the bridge to the next behavior-gated
  experiment.

appcap:
  High-capacity application-like control. Use it to separate capacity failure
  from policy failure. Do not treat it as the HZ6 default lane.

visiblefirst-appcap:
  Diagnostic-only appcap variant. In non-strict `free()`, visible/shared route
  lookup is tried before local route lookup; visible MISS falls back to local
  lookup so local INVALID is not converted to MISS. This is an upper-bound
  probe for cross-owner warmup, not a promotion lane. It is now treated as
  no-go evidence rather than a production candidate.

negativefilter-appcap:
  Diagnostic-only appcap variant. It uses a conservative local owned-range hint
  to skip local route lookup only when the pointer is definitely not local.
  Diagnostic builds shadow-verify the skip and record false-skip counters.
  The source-block-only hint is now no-go as an optimization lane because
  route rehome can create local exact routes without local source-block
  ownership. Keep this lane as evidence that the next design needs rehome-aware
  owner ranges or a shared route directory.

sharedir-appcap:
  Diagnostic-only appcap variant. It publishes exact routes into a process-wide
  shared route directory and probes it only after local MISS. Behavior is
  unchanged. Use it to measure whether a shared directory could avoid the
  worker-local route MISS scan in cross-owner warmup.

sharedirfirst-appcap:
  Experimental behavior variant. After the allocator has observed a foreign
  visibility hit, it tries the shared directory first and reconstructs exact
  route results from the directory. Compact/moderate main-warmup can recover
  strongly, but 10k-chunk stress main-warmup currently times out. Keep it
  evidence-only until large live-set scaling is fixed.

ownerlocality-appcap:
  Diagnostic-only hint/backend lane. It uses a cheap owner-locality index to
  decide whether a pointer is definitely foreign, then consults the shared
  route directory backend for foreign exact lookup before falling back to the
  ordinary local route path. Use it to measure whether a low-cost locality hint
  can prune worker-local MISS scans without turning source-block ownership into
  the only truth source. The Larson runner now releases each worker live set
  before worker allocator teardown, so main-warmup no longer adds an accidental
  post-measurement owner-death cleanup stress on top of the timed cross-owner
  handoff lane.

ownerlocalityfast-appcap:
  Non-diagnostic owner-locality behavior lane. It uses the same owner-locality
  index and shared-directory exact backend as `ownerlocality-appcap`, but does
  not force `HZ6_DIAGNOSTIC_PROBES=1`. Use it after the diagnostic lane has
  shown clean counters to check whether the route-lifecycle fix is still fast
  without diagnostic probe overhead. Repeat-3 now makes this the preferred
  candidate-control lane for the fast Larson owner-locality comparison rows.

ownerlocalityfast-rsscap-1:
  Non-diagnostic owner-locality behavior lane with appcap descriptor / route /
  source-block capacity, but reduced transfer and frontcache capacity. Use it
  as the current full 10k Larson cross-owner candidate-control. Latest run1
  matches `ownerlocalityfast-appcap` throughput while cutting peak from
  roughly 2.82GB to roughly 1.23GB. It is also evidence that larger_sizes peak
  is not primarily transfer/frontcache capacity.

ownerlocalityfast-rsscap-2:
  Non-diagnostic owner-locality behavior lane with reduced transfer,
  frontcache, and source-block capacity. Current larger_sizes run1 keeps or
  improves throughput while cutting peak working set by about 57 MiB versus
  ownerlocalityfast-appcap. In full 10k Larson it is the lower-RSS sibling of
  rsscap-1: lower peak, slightly lower throughput, safety clean. Treat as
  source-block trim evidence; rsscap-3 is the stronger follow-up only when
  descriptor trim is safe for the target row.

ownerlocalityfast-rsscap-2-desc192k:
  Descriptor-boundary probe for full 10k Larson cross-owner. Same shape as
  rsscap-2, but descriptor capacity is 196608 instead of 262144. Use it to
  test whether rsscap-2 peak can be reduced without crossing the rsscap-3
  warmup-failure boundary. Repeat-3 makes it the stable near-capacity sibling:
  about 43.679M ops/s and 974,296 KB peak, with safety counters clean.

ownerlocalityfast-rsscap-2-desc160k:
  Descriptor-boundary probe for full 10k Larson cross-owner. Same shape as
  rsscap-2, but descriptor capacity is 163840. It is intentionally closer to
  the failing rsscap-3 descriptor budget; treat failures as boundary evidence,
  not as a route/source behavior verdict. Repeat-3 promotes it to the current
  full 10k Larson candidate-control: corrected median is about 43.721M ops/s
  and 928,228 KB peak, with safety counters clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k:
  ThinDescriptor/source16k Larson low-RSS baseline. It completes full-10k and
  remains a useful control, but MetadataSlim-L1 showed route capacity can be
  reduced further without losing completion.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k:
  Clean route-capacity control. Same as thindesc-source16k, but route capacity
  is trimmed from 262144 to 196608. Latest repeat-3:
  `44.610M / 628844 KB`, safety clean. SourceBlockMetaSlim-L1 superseded it
  as the lowest-RSS sibling with route192k-run512. Route160k and route128k are
  no-go boundary controls: both fail warmup from route-table saturation.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512:
  Previous Larson lowest-RSS sibling/control. Same route capacity as route192k,
  but `HZ6_SOURCE_RUN_MAX_SLOTS` is reduced to 512. Repeat-3:
  `48.512M / 499820 KB`, safety clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512:
  Larson descriptor-layout comparison control. Same route/run512 shape, but
  `HZ6_DESCRIPTOR_NO_BACKPTR_L1=1` removes the allocator back-pointer from
  `Hz6ObjectDescriptor`. Repeat-3:
  `40.710M / 476784 KB`, safety clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512:
  Larson directory-capacity comparison control. Same no-backptr route/run512
  shape, but `HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY` is trimmed to 196608 so the
  owner-locality/shared-directory index is smaller. It is superseded by the
  routebytes16 comparison-control sibling and the later OwnerSourceSideMeta-L2
  selected sibling. Repeat-3:
  `44.580M / 472176 KB`, safety clean. Same-run no-backptr control is
  `45.310M / 476788 KB`, so dir192k trades about -1.6% throughput for about
  4.6 MB lower peak RSS. Dir128k/dir96k are no-go controls because
  owner-locality misses and full-table probes appear.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512:
  SourceBlockNoRouteBackptr-L1 minimum-RSS Larson control. Same no-backptr /
  dir192k / route192k / run512 shape, but `HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1`
  removes the SourceBlock route-backend pointer and relies on allocator-explicit
  SourceBlock release/unregister. Repeat-3:
  `41.107M / 469868 KB`, safety clean, `source_block_entry_bytes=136`.
  Keep it as a clean isolation control; routebytes16/routepacked supersedes it
  for selected lowest-RSS comparisons.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512:
  RoutePackedMeta-L1 comparison control. Same descriptor no-backptr /
  SourceBlock no-route-backptr / dir192k / route192k / run512 shape, plus
  `HZ6_ROUTE_PACKED_META_L1=1`. It packs route entry front/class/state into a
  32-bit meta word and moves bytes to a side array. Repeat-3 full 10k:
  `47.616M / 456048 KB`, safety clean. The 1k diagnostic smoke reports
  `route_entry_bytes=24`, `route_invalid=0`, `route_miss=0`, and
  `route_register_fail=0`. Superseded by RoutePackedMeta-L2 routebytes16 for
  the selected Larson lowest-RSS sibling.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512:
  Superseded Larson lowest-RSS control. It keeps the RoutePackedMeta-L1
  shape and stores route bytes as `uint16_t(bytes - 1)` behind the route
  accessor boundary. The L2 dry-run showed raw uint16 overflow only at 64KiB
  source-block envelopes (`route_bytes16_overflow=147`,
  `route_bytes16_max=65536`), while biased16 was clean
  (`route_bytes16_minus1_overflow=0`, `route_bytes16_minus1_zero=0`,
  `route_bytes16_minus1_max_stored=65535`). Same-run full-10k A/B repeat-3:
  routepacked `45.079M / 456040 KB`, routebytes16 `48.367M / 449144 KB`,
  safety clean. Superseded by OwnerSourceSideMeta-L2 after the 2026-06-05
  repeat-3.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512:
  OwnerSourceSideMeta-L1 dry-run. Same routebytes16 comparison-control lane, plus
  `HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1` in diagnostic builds only. It projects
  whether descriptor owner side metadata can be made owner-source-aware without
  changing behavior. Full 10k run=1:
  `46.202M / 449164 KB`, safety clean,
  `owner_source_side_meta_foreign=871979714`,
  `owner_source_side_meta_miss=0`, `owner_source_side_meta_probe_max=1`.
  Read: the problem is lookup frequency, not depth. Do not promote
  StorageOwner16 directly; design an O(1) owner-source side metadata map.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512:
  Current Larson lowest-RSS selected sibling. It combines routebytes16 with
  StorageOwner16 ownerless descriptors and stores an O(1) descriptor-storage
  hint on each SourceBlock. Full 10k non-diagnostic repeat-3:
  routebytes16 comparison control `40.750M / 449128 KB`, L2
  `40.754M / 439912 KB`. Worker-warmup 10k run=1: routebytes16
  `40.126M / 448948 KB`, L2 `40.787M / 439740 KB`.
  Full 10k diagnostic dry-run comparison:
  `owner_source_side_meta_l2_hit=1253200849`,
  `owner_source_side_meta_l2_miss_no_block=0`,
  `owner_source_side_meta_l2_miss_inactive=0`,
  `owner_source_side_meta_l2_storage_mismatch=0`, safety clean.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512:
  Lowest-RSS candidate/sibling over OwnerSourceSideMeta-L2. It adds
  `HZ6_FRONTCACHE_PACKED_META_L1=1`, shrinking `Hz6FrontCacheEntry` from
  32 to 24 bytes by packing bytes-minus-one, class id, and the descriptor-cold
  governor detached flag into a 32-bit meta word. Direct full-10k closeout:
  OwnerSourceSideMeta-L2 baseline `46.467M / 439912 KB`, FrontCachePacked
  `44.831M / 430708 KB`, safety clean. Keep as the lower-RSS sibling/candidate,
  not as the selected throughput/RSS balance lane.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512:
  SourceBlockPackedFlags-L1 lower-RSS candidate over OwnerSourceSideMeta-L2.
  It adds `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1`, packing SourceBlock
  `source_kind`, `active`, `route_registered`, and `run_active` into one
  state flag word. It keeps `source_release`, OwnerSourceSideMeta-L2, and
  source-run bitmap/slot metadata inline. Build/smoke is clean; diagnostic
  sizeof check for the selected routepacked/routebytes16/L2 shape shows
  `source_block_entry_bytes=144 -> 128`. Repeat-3 closeout:
  `41.070M / 435304 KB`, safety clean. Keep as a component lower-RSS
  candidate/control; the combined packed lane is the current minimum-RSS
  candidate.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source16k-route192k-run512:
  Combined packed minimum-RSS candidate over OwnerSourceSideMeta-L2. It enables
  both `HZ6_FRONTCACHE_PACKED_META_L1=1` and
  `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1=1`. The two optimizations target separate
  metadata tables, so the RSS savings compose cleanly in the smoke/repeat.
  Repeat-3 full 10k: `40.837M / 426084 KB`, safety clean. This is lower RSS
  than FrontCachePacked alone (`41.131M / 430692 KB`) and SourceBlockPacked
  alone (`41.070M / 435304 KB`) at similar throughput. Keep as the current
  Larson minimum-RSS candidate/sibling, not as a broad throughput promotion.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2dryrun-source16k-route192k-run512:
  Validation lane for OwnerSourceSideMeta-L2. Same L2 behavior plus
  `HZ6_OWNER_SOURCE_SIDE_META_DRYRUN=1`, so it compares the O(1) source-block
  storage hint against the old scan-based storage-owner result. Use only for
  mismatch/safety validation, not speed ranking.

ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512:
  StorageOwner16-L1 RSS-first descriptor side metadata evidence/control. It
  removes owner from the hot descriptor and stores packed owner slot/generation
  in the descriptor storage allocator's side table. This fixes the old
  allocator-local side-owner16 safety failure. Repeat-3 full 10k:
  `42.024M / 444520 KB`, safety clean. Keep as evidence/control; routepacked
  remains selected because storageowner16 saves about 11.5 MB RSS but loses
  about 12% throughput.

ownerlocalityfast-rsscap-2-desc144k:
  Descriptor-boundary probe for full 10k Larson cross-owner. Same shape as
  rsscap-2, but descriptor capacity is 147456. It is the narrow probe between
  selected desc160k and failing rsscap-3/131k. Use only to test whether the
  descriptor budget can be tightened one more step; failures are boundary
  evidence, not a route/source behavior verdict. First full 10k Larson run
  fails warmup allocation with `alloc_fail=1` and source-block exhaustion, so
  keep it as no-go/boundary evidence. Diagnostic read confirms the descriptor
  table is effectively full of active objects; because T=16 and chunks=10000
  imply roughly 160k live objects, desc160k is close to the
  one-descriptor-per-live-object lower bound.

ownerlocalityfast-rsscap-3:
  Non-diagnostic owner-locality behavior lane with reduced transfer,
  frontcache, source-block, and descriptor capacity. Current larger_sizes run1
  keeps safety counters clean and lowers peak to about 191..196 MiB while
  preserving appcap-class throughput. It fails full 10k Larson warmup, so treat
  it as RSSCap evidence only, not as the cross-owner full-stress lane.

ownerlocalityfast-rsscap-4:
  Non-diagnostic owner-locality behavior lane with reduced transfer,
  frontcache, source-block, descriptor, and route capacity. Current
  larger_sizes run1 lowers peak to about 166..172 MiB and improves throughput
  versus rsscap-3, with safety counters clean. Treat as the current
  larger_sizes RSSCap candidate-control only; wide_ws guard regresses versus
  ownerlocalityfast-appcap. It is excellent for compact/moderate Larson
  live-set checks, but too tight for full 10k Larson warmup.

directlocalfree-ownerlocalityfast-rsscap-4:
  ownerlocalityfast-rsscap-4 plus DirectLocalFree-L1. Use this to test whether
  the same-owner free-path improvement also helps the larger_sizes selected
  lane without changing its RSS-capacity shape. This is a selected-family
  control lane, not a default replacement. Repeat-3 is mixed: larger_sizes and
  some rss rows improve, but balanced strict and wide_ws speed regress.

ownerlocalityfast-widecap-1:
  Non-diagnostic owner-locality behavior lane for wide_ws. It keeps appcap
  descriptor / route / source-block capacity, but sets transfer/frontcache to
  32768. It shows that wide_ws needs a larger hot cache than rsscap-1.

ownerlocalityfast-widecap-2:
  Same as widecap-1, but transfer/frontcache are 16384. It preserves
  appcap-like source allocation behavior for wide_ws and is the baseline for
  trimming source-block / descriptor capacity without reverting to rsscap-1.

ownerlocalityfast-widecap-3:
  widecap-2 plus source-block capacity 8192. Current wide_ws run1 cuts peak
  versus widecap-2 while keeping clean safety counters and largely preserving
  throughput.

ownerlocalityfast-widecap-4:
  widecap-3 plus descriptor capacity 131072. Current wide_ws run1 cuts peak
  further and keeps safety counters clean. Repeat-3 confirms the wide_ws shape:
  speed/rss improve versus appcap while peak drops substantially. Balanced is
  acceptable, but larger_sizes remains worse than rsscap-4. Treat as wide_ws
  candidate-control evidence, not a universal ownerlocalityfast replacement.

directlocalfree-ownerlocalityfast-widecap-4:
  ownerlocalityfast-widecap-4 plus DirectLocalFree-L1. Use this to test whether
  the same-owner free-path improvement lifts the wide_ws selected lane while
  preserving its existing RSS profile. First selected-family read regressed
  wide_ws, so keep it as no-go/control evidence unless a later class/front gate
  reopens the question.
```

## Focused Mechanism Lanes

```text
localexactfree-noboost-route4k:
  noboost-route4k plus LocalExactFirstFree-L1. Same low-RSS capacity shape,
  but free/free_remote try exact route lookup before the full route lookup.
  Exact MISS still falls back to the normal full lookup, so invalid-range and
  foreign/fallback safety semantics are preserved. Use as a random_mixed
  same-owner hot-path probe, not as a default lane. First random_mixed run
  confirms full lookup probes are removed on exact-valid frees, but normal
  throughput is not materially better, so keep it as mechanism evidence.

directlocalfree-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1. Local-owner TOY/MIDPAGE/LOCAL2P
  frees bypass front lookup / function-pointer dispatch / wrapper validation
  and call descriptor-to-frontcache directly. LARGE and all remote/foreign
  paths remain on the normal front contract. Use as a narrow same-owner
  free-path overhead probe. Repeat-3 strongly improves random_mixed and
  modestly improves mixed_ws balanced / wide_ws / larger_sizes with flat RSS.
  Treat as candidate-control evidence, but keep it named explicitly because it
  bypasses the generic front contract for selected local-owner fronts.

descavail-noboost-route4k:
  noboost-route4k plus DescriptorAvailCount-L1. It keeps the same low-RSS
  capacity shape but avoids full descriptor-table scans once the allocator
  knows no descriptor is available. Repeat-3 strongly improves mixed_ws
  balanced and wide_ws and keeps larger_sizes slightly positive, with route
  safety counters clean. Random_mixed is mostly neutral, so this is a
  descriptor-failure cost candidate-control rather than the primary
  same-owner hot-path lane.

directlocalfree-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1 and DescriptorAvailCount-L1. This
  explicit composition is strongest in random_mixed and edges out descavail in
  mixed_ws wide_ws/larger_sizes, but it loses to descavail alone in mixed_ws
  balanced. Keep it as a named candidate-control lane, not a silent replacement
  for either parent mechanism.

sameownerfast-descavail-noboost-route4k:
  Selected random_mixed same-owner lane. This is the cleaned-up contract flag:
  HZ6_SAME_OWNER_FAST_L1 plus DescriptorAvailCount-L1 on the noboost-route4k
  capacity shape. It encodes the strong A-ladder composition
  (DirectLocalFree-L1 + DirectLocalAlloc-L1 + DirectLocalReuse-L1) without
  forcing benchmark scripts to carry a pile of mechanism flags. Use this name
  for current comparisons; keep the longer directlocal* names as
  evidence/control lanes.

directlocalalloc-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalAlloc-L1 and DescriptorAvailCount-L1. This is
  a random_mixed ablation lane: TOY/MIDPAGE/LOCAL2P malloc tries the existing
  cached/transfer reuse helper before the generic front function-pointer path.
  LARGE and cross-owner paths stay on the normal front contract.

directlocalreuse-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalAlloc-L1, DirectLocalReuse-L1, and
  DescriptorAvailCount-L1. This is the narrower local-cache reuse ablation:
  malloc only activates materialized LOCAL_FREE frontcache descriptors before
  falling back to the normal front path. Descriptorless materialization and
  transfer reuse are intentionally excluded from the direct probe.

directlocalfreealloc-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1, DirectLocalAlloc-L1, and
  DescriptorAvailCount-L1. This composition tests whether the existing
  free-path win and the new alloc dispatch-bypass compose on random_mixed.
  Keep it as A-ladder evidence until repeat data justifies a shared
  same-owner front fast path.

directlocalfreereuse-descavail-noboost-route4k:
  noboost-route4k plus DirectLocalFree-L1, DirectLocalAlloc-L1,
  DirectLocalReuse-L1, and DescriptorAvailCount-L1. This closes the A-ladder:
  free goes through the direct local cache helper and malloc first tries only
  materialized LOCAL_FREE frontcache descriptors for TOY/MIDPAGE/LOCAL2P.
  Repeat-3 random_mixed gives the cleanest broad same-owner signal, so use it
  as B-design input. It is still not the selected balanced/wide_ws low-RSS
  default because rss + descavail remains stronger there.

largerlowrss-front8k-sourcerun-desc8k-route8k:
  Larger_sizes-targeted low-RSS lane: descriptor 8K, route 8K, source-block
  512, frontcache 8K, and SourceRunReuse-L1. It keeps allocation failures at
  zero and reduces source/route churn by retaining larger object slots long
  enough to avoid constant source recreation. Repeat-3 larger_sizes beats
  ownerlocalityfast-rsscap-4 while using less than half the peak RSS. Do not
  use it as a universal mixed_ws lane: wide_ws guard regresses badly and
  balanced uses much more RSS than descavail.

largerlowrss-front6k-sourcerun-desc8k-route8k:
  Tighter larger_sizes front-retention candidate-control: descriptor 8K, route
  8K, source-block 512, frontcache 6144, and SourceRunReuse-L1. Repeat-3
  larger_sizes is effectively tied with front8k while keeping the same 72MB
  peak band. Promotion check across large_slice rows did not justify replacing
  front8k: front6k is good for larger_sizes itself, but front8k covers 4K/8K
  and 32K slice rows more consistently. Keep front6k as close candidate-control.

largerlowrss-front4k-sourcerun-desc8k-route8k:
  Front-retention lower-bound control: descriptor 8K, route 8K, source-block
  512, frontcache 4096, and SourceRunReuse-L1. Run1 larger_sizes speed drops
  sharply, so keep it as no-go evidence that front retention below 6K is too
  small for this profile.

mixedclean-front8k-sourcerun-desc16k-source2k-route16k:
  Mixed_ws clean-lane boundary probe: frontcache 8K, descriptor 16K,
  source-block 2K, route 16K, and SourceRunReuse-L1. Balanced is clean but
  below the selected source2k/source4k rows; wide_ws still reports allocation
  failures. Keep as lower-capacity evidence.

mixedclean-front16k-sourcerun-desc16k-source2k-route16k:
  Mixed_ws clean-lane boundary probe with frontcache 16K but descriptor/route
  still 16K. It improves wide_ws substantially versus front8K but still leaves
  allocation failures. It shows wide_ws needs more than the 16K
  descriptor/route band.

mixedclean-front16k-sourcerun-desc16k-transfer2304-source2k-route16k:
  Transfer-isolation control for the desc16 no-go. Increasing transfer cache
  from 2048 to 2304 does not remove the wide_ws allocation failures, so the
  desc16 failure is not just a too-small transfer cache.

mixedclean-front16k-sourcerun-desc16k-transfer2560-source2k-route16k:
  Wider transfer-isolation control for the desc16 no-go. It still reports the
  same wide_ws allocation-failure count as the base desc16 lane, so keep it as
  no-go evidence.

mixedclean-front16k-sourcerun-desc17k-source2k-route17k:
  Selected mixed_ws balanced clean low-RSS lane: frontcache 16K, descriptor 17K,
  source-block 2K, route 17K, and SourceRunReuse-L1. Repeat-3 balanced and
  wide_ws are safety-clean, with the lowest RSS among the clean boundary
  lanes. Boundary scan: balanced `64.117M / 110976 KB`, wide_ws
  `21.119M / 140256 KB`. Selected-family refresh:
  balanced `55.504M / 110780 KB`, wide_ws `19.978M / 140236 KB`.
  Route-only repeat control: balanced `66.308M / 110940 KB`, wide_ws
  `20.155M / 140172 KB`.

mixedclean-front16k-sourcerun-desc17k-source2k-route18k:
  Selected wide_ws sibling. Same descriptor, transfer, source-block, and
  frontcache capacities as desc17/route17, but route capacity is raised from
  17408 to 18432. Repeat-3:
  balanced `64.923M / 111248 KB`, wide_ws `22.184M / 140456 KB`, safety clean.
  It lifts wide_ws about 10% versus the same-run desc17/route17 control with
  about +284 KB peak RSS. Route20 reduces probes further but does not improve
  throughput enough to justify the extra RSS.

mixedclean-front16k-sourcerun-desc18k-source2k-route18k:
  Clean boundary control above the selected desc17 lane. Repeat-3 is
  safety-clean, but it uses slightly more RSS and does not improve wide_ws
  enough to replace desc17: balanced `64.979M / 111524 KB`, wide_ws
  `20.398M / 140860 KB`.

mixedclean-front16k-sourcerun-desc20k-source2k-route20k:
  Clean wide-speed sibling/control. Repeat-3 gives a little more wide_ws speed
  than desc17 but with more RSS: balanced `59.888M / 113076 KB`, wide_ws
  `21.498M / 142676 KB`. Keep desc17 as the low-RSS selected lane.

mixedclean-front16k-sourcerun-desc22k-source2k-route22k:
  Boundary control between desc20 and desc24. Run-1 was clean but repeat did
  not beat desc17/20 enough to justify promotion.

mixedclean-front16k-sourcerun-desc24k-source2k-route24k:
  Previous mixed_ws clean low-RSS lane / extra-capacity control: frontcache
  16K, descriptor 24K, source-block 2K, route 24K, and SourceRunReuse-L1.
  Superseded by desc17 after the desc17 repeat-3 lowered RSS further while
  staying safety-clean.

mixedclean-front16k-sourcerun-desc32k-source2k-route32k:
  Previous mixed_ws clean low-RSS lane / extra-capacity control: frontcache
  16K, descriptor 32K, source-block 2K, route 32K, and SourceRunReuse-L1.
  Desc24 and then desc17 superseded it with lower RSS and comparable or better
  repeat-3 speed, so keep this as a control.

mixedclean-front16k-sourcerun-desc32k-source3k-route32k:
  Source-capacity midpoint control between source2K and source4K. Run-1 stayed
  safety-clean, but it did not improve wide_ws enough to justify the extra RSS:
  balanced `60.420M / 127648 KB`, wide_ws `20.113M / 156504 KB`. Keep as
  no-promotion evidence.

mixedclean-front16k-sourcerun-desc32k-source4k-route32k:
  Speed/control sibling for mixed_ws clean rows: same front/descriptor/route
  shape as the old desc32/source2K lane but source-block capacity 4K.
  Repeat-3 is safety-clean and can be faster in balanced rows, but uses much
  more peak RSS than desc17, so keep it as speed/control only.

mixedclean-front8k-sourcerun-desc32k-source4k-route32k:
  Tightening control for the clean mixed lane. Descriptor/route/source are
  wide enough, but frontcache 8K makes wide_ws much slower. Keep as evidence
  that clean wide_ws needs frontcache 16K, not just descriptor/route capacity.

mixedclean-front12k-sourcerun-desc32k-source4k-route32k:
  Frontcache tightening control between 8K and 16K. It can keep wide_ws clean,
  but source2K/front16K is the cleaner RSS default after repeat-3.

desc4k-route4k:
  route4k plus descriptor capacity 4096. Descriptor-pressure probe only.

source512-route4k:
  route4k plus source-block capacity 512. Source-block-pressure probe only.

desc4k-source512-route4k:
  route4k plus descriptor 4096 and source-block 512. Combined pressure probe.

redislowrss-route4k:
  noboost plus descriptor 4096 and source-block 512. This is the named
  Redis-like low-RSS candidate-control lane after redis_short L0 showed
  `desc4k-source512-route4k` avoids Redis allocation-failure spam at a much
  lower peak working set than appcap. Evidence-only outside Redis-like rows:
  mixed_ws guard shows the capacity shape over-retains and slows broad mixed
  profiles. Redis long diagnostics now show this lane is the stronger
  completion/LPUSH shape than the slim lane.

redislowrss-sourcerun-route4k:
  redislowrss-route4k plus SourceRunReuse-L1. This is a narrow follow-up for
  Redis-like rows where redislowrss-route4k completes but still reports
  source_block_exhausted/source_prefill_fallback in RANDOM. It tests whether
  reusing physical slots inside existing source blocks can reduce Redis source
  pressure without widening capacity again. Latest redis_long run strongly
  improved all Redis patterns and lowered peak working set, but mixed_ws guard
  still keeps this outside general promotion.

redislowrss-sourcerun-route8k:
  redislowrss-sourcerun-route4k with route capacity 8192. Evidence lane for
  paper-aligned Redis LPUSH, where route4k showed route_register_fail. Route8k
  removes route register failure but exposes descriptor pressure, so it is not
  the final paper-row candidate.

redislowrss-sourcerun-desc8k-route8k:
  redislowrss-sourcerun with descriptor 8192 and route 8192. Current
  paper-aligned Redis candidate-control: removes route/descriptor/source-block
  exhaustion in the checked diagnostic row and keeps peak working set far below
  appcap while restoring LPUSH into a usable range. Keep this Redis-specific;
  do not promote it as the general mixed_ws lane.

redislowrss-sourcerun-desc8k-route8k-tombcompact:
  redislowrss-sourcerun-desc8k-route8k plus RouteTombstoneCompact-L1. Narrow
  Redis RANDOM route-churn behavior lane. It compacts exact-route tombstones
  after overflow unregisters cross a threshold, without changing descriptor or
  source retention policy. Use it to test whether RANDOM's full-table
  tombstone probes are the remaining bottleneck before attempting retained
  overflow or source-run slot lookup. First paper-row repeat-3 roughly doubles
  RANDOM and modestly improves LPUSH with a small peak increase, but SET/GET
  are slightly lower, so keep it Redis-specific.

redislowrss-sourcerun-desc8k-route8k-retainedoverflow:
  redislowrss-sourcerun-desc8k-route8k plus RouteRetainedOverflow-L1. When a
  LOCAL_FREE object cannot enter frontcache, it is retained in the bounded
  transfer cache as TRANSFER_FREE instead of immediately unregistering its
  exact route and releasing its descriptor/source. This tests whether Redis
  SET/LPUSH/RANDOM are losing time to overflow unregister churn before we add a
  larger retained-overflow data structure. First diagnostic row is no-go as a
  Redis fix: transfer retention succeeds mechanically but final
  frontcache-overflow unregisters and RANDOM tombstone full-probes remain.

redislowrss-sourcerun-desc8k-route8k-slotlookup:
  redislowrss-sourcerun-desc8k-route8k plus SourceRunSlotLookup-L1. It keeps
  the same source-run reuse contract but prefers the reusable block with the
  most free slots instead of the first reusable block found. Narrow lookup
  policy probe for Redis paper rows after tombstones and retained overflow have
  already been measured. Repeat-3 showed it is a useful source-run witness but
  not a promotion lane: SET is slightly better, RANDOM is roughly neutral, GET
  and peak are a bit worse.

redislowrss-slim-route4k:
  noboost plus descriptor 2048 and source-block 256. This is the slimmer Redis
  follow-up lane. Use it only if we need to reduce peak / retention further
  than `redislowrss-route4k`. Redis long diagnostics showed this can collapse
  LPUSH through descriptor/source-block exhaustion, so keep it as a control
  rather than the current Redis candidate.

front1k-desc4k-source512-route4k:
  route4k plus descriptor/source/front-cache widening. Reproduces broad-like
  behavior more than it solves the low-RSS lane. Evidence/control only.

sourcerun-route4k:
  Source-run slot reuse evidence lane. It proves reusable-run mechanics can
  reduce some source pressure, but it did not solve balanced mixed rows.

sourcerun-sameclass-route4k:
  Narrower same-class source-run reclaim evidence lane. Safer than broad donor
  reclaim and mildly positive on the latest rows, but still not a promotion
  lane.

descriptorless-route4k:
  Descriptor lifecycle prototype. Source-block cached slots can drop their
  descriptor while staying in frontcache; reuse materializes a fresh descriptor
  and exact route. This lane includes source-run metadata because descriptorless
  cached slots need a physical slot owner after the descriptor is detached. Use
  it to test whether cached slots should stop pinning descriptor capacity. The
  current L1 is evidence/control only: it preserves route safety in the latest
  checks, but descriptor materialization can still fail under a full descriptor
  table, so it is not a promotion lane.

descriptorreserve-route4k:
  Descriptor materialization-reserve prototype. Extends descriptorless-route4k
  by reserving the detached descriptor for the same cached physical slot, so
  reuse can materialize without a fresh descriptor-table search. Evidence only:
  it removes descriptorless materialization failures in the latest checks, but
  it does not solve balanced / wide_ws and can reduce the descriptor pool
  available to normal allocation.

descriptorcold-route4k:
  Selective descriptorless prototype. Only over-soft-cap frontcache bins detach
  descriptors, reducing the broad descriptorless failure loop. Evidence only:
  it has a small balanced / wide_ws signal, but larger_sizes regresses, so the
  simple soft-cap gate is not a promotion policy.

descriptorcoldgov-route4k:
  Class/pressure-aware descriptor governor prototype. Descriptorless is treated
  as cold-cache descriptor compression, not as a hot reuse path. The first L1
  gates detach to small/mixed classes, limits detached entries, and admits
  materialization only when a descriptor is available. Latest repeat-3 is
  promising on balanced and preserves larger_sizes, but wide_ws is still weak,
  so keep it as candidate-control evidence rather than default promotion.

descriptorcoldgov-widews-route4k:
  Budget-expanded descriptor governor variant for wide_ws probing. It keeps the
  same class gate as descriptorcoldgov-route4k but raises the detach budget to
  test whether wide_ws is limited by detached-slot budget saturation rather
  than by the class gate itself. Latest repeat-3 did not improve wide_ws and
  badly regressed balanced, so keep it as no-go/control evidence.
```

## Frozen No-Go Lanes

```text
spill-route4k:
  No-go. Frontcache spill increased source allocation and RSS.

borrow-route4k:
  No-go. Borrowing moved some source-block pressure but did not produce a clean
  throughput/RSS improvement.

cap-route4k:
  No-go. Local cap behavior slowed the lane and worsened RSS.

sourcerun-reclaim-route4k:
  No-go. Broad source-run donor reclaim caused route churn / probe explosions.
```

Keep these lanes buildable as controls, but do not use them as the next
optimization target unless a new diagnostic specifically reopens the question.

## Benchmark Family Read

```text
mixed_ws / random_mixed:
  Best for capacity-shape and low-RSS tradeoff checks.
  Latest selected-lane random_mixed run shows capacity is not the blocker:
  `noboost-route4k` is roughly as fast as widecap/rsscap/appcap while using
  far less RSS. Treat `noboost-route4k` as the random_mixed low-RSS control
  and attack same-owner hot-path overhead before adding more capacity lanes.

Larson worker-warmup:
  Same-owner small-object control. This is the lane for hot-path / toy-small
  behavior without cross-owner handoff stress.

Larson main-warmup:
  Cross-owner handoff stress. Treat failures here as route visibility /
  remote-free / transfer ownership evidence, not as a pure hot-path verdict.
  For full 10k T16, use `speed + ownerlocalityfast-rsscap-2-desc160k` as the
  current candidate-control and `speed + ownerlocalityfast-rsscap-2-desc192k`
  as the stable near-capacity sibling. Use rsscap-3/4 only for compact/moderate
  live-set checks.

Redis workload:
  App-like pattern control. Useful for detecting whether HZ6 capacity changes
  actually survive more realistic access mixes. Current route4k/noboost rows
  need a focused shorter profile before they are useful for candidate ranking,
  because the default Redis-like row can timeout while emitting many allocation
  failure stats lines. Use `redis_tiny`, `redis_short`, `redis_medium`, and
  `redis_long` before returning to `redis_workload`, so collapse phases are
  visible before the paper-sized row times out.
```

## Commands

Build a focused HZ6 capacity suite:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\build_win_hz6_capacity_suite.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap
```

List the rows without running them:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -ListOnly `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,sourcerun-sameclass-route4k
```

Run a quick one-pass capacity check:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families mixed_ws `
  -Hz6Profiles strict `
  -CapacityLanes control,route4k,appcap `
  -Runs 1
```

Run a focused Redis-like timeout triage row:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families redis `
  -Hz6Profiles rss `
  -CapacityLanes route4k,noboost-route4k `
  -BenchmarkProfiles redis_short `
  -Runs 1 `
  -SkipBuild
```

Run staged Redis-like pressure rows:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\run_win_hz6_capacity_matrix.ps1 `
  -Families redis `
  -Hz6Profiles rss `
  -CapacityLanes noboost-route4k,redislowrss-slim-route4k `
  -BenchmarkProfiles redis_short,redis_medium,redis_long `
  -Runs 1 `
  -SkipBuild `
  -ContinueOnFailure
```

## Lane Rules

- Speed / paper lanes must not include diagnostic atomics or trace-only probes.
- Research lanes must have explicit names and must not silently replace
  `route4k`.
- `appcap` is a capacity/completion control, not a production default.
- A lane is not promoted from one good row. It needs RSS, safety, repeat
  stability, and guard behavior.
- Owned-looking invalid pointers must never be converted into foreign fallback.
- Do not put OS queries such as `VirtualQuery` on hot free paths.
