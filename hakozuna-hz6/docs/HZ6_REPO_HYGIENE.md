# HZ6 Repo Hygiene

HZ6 now has enough lanes and evidence that the repository needs a stable
reading order. This file is the cleanup map: use it before adding another
benchmark lane or source-level experiment.

## Reading Order

| Purpose | File |
| --- | --- |
| Short paper-facing selected rows | `HZ6_SELECTED_FAMILY_SUMMARY.md` |
| Which lane is which | `HZ6_LANE_GUIDE.md` |
| Cross-allocator comparison table | `HZ6_CROSS_ALLOCATOR_COMPARISON.md` |
| ElasticCapacity source-depot track | `HZ6_ELASTIC_CAPACITY_PLAN.md` |
| Windows lane registry cleanup | `HZ6_WINDOWS_LANE_REGISTRY.md` |
| Source/module cleanup plan | `HZ6_SOURCE_MODULARIZATION.md` |
| Long experiment ledger | `current_task.md` |

Rule:

```text
Use current_task.md as a ledger, not as the primary orientation document.
If a decision is stable, summarize it in the selected summary or lane guide.
```

## Lane Status Vocabulary

Use these terms consistently in docs and scripts.

| Status | Meaning |
| --- | --- |
| selected | Clean row used for current HZ6 family comparisons. |
| selected sibling | Clean alternate row with a specific tradeoff, usually lower RSS or higher speed. |
| control | Useful comparison row, not promoted. |
| evidence | Mechanism result that explains a design point. |
| pressure | Intentionally stressed/unsafe-capacity row; never a paper default without caveat. |
| no-go | Frozen failed lane; keep only when it explains a boundary. |

## Current Selected Lanes

The current selected family is:

```text
mixed balanced/wide:
  rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k

random_mixed:
  strict + sameownerfast-descavail-noboost-route4k

larger_sizes:
  speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k

Larson throughput:
  speed + ownerlocalityfast-rsscap-2-desc160k

Larson lower RSS:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k

Larson lowest-RSS balance sibling:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512

Larson lower-RSS component candidates:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512

Larson packed minimum-RSS sibling:
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512

Larson Elastic low-RSS sibling:
  speed + ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512

ElasticCapacity bounded descriptor rehome candidate-control:
  speed + ownerlocalityfast-rsscap-2-elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096

ElasticCapacity diagnostic-only:
  speed + diagnostic + ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerconsumerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096
```

The side-owner16 descriptor layout lane is not selected. It is buildable as
no-go evidence only: allocator-local owner side metadata reaches a 32-byte hot
descriptor entry, but it breaks cross-owner lifecycle safety counters.
StorageOwner16 is buildable as RSS-first evidence/control: descriptor side
metadata becomes safety-clean when keyed by descriptor storage ownership, but
it does not replace routepacked because the RSS gain costs too much throughput.
OwnerSourceSideMeta-L2 is the selected Larson lowest-RSS balance sibling.
FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 are lower-RSS component
controls/candidates. The combined packed source10k lane is the current clean
packed minimum-RSS candidate after repeat-3 (`44.864M / 412280 KB`), but it
remains a minimum-RSS sibling/candidate rather than a broad throughput
promotion. DepotOwnerDirectDirectFreeTrustedLocalCache is the selected
Larson/Elastic low-RSS sibling after the 2026-06-06 repeat-3 guard: it is
safety-clean, improves every main/worker 1k/4k/10k row over DepotOwnerDirect
(`avg +2.60%`, min `+1.34%`), and keeps essentially the same peak RSS.
DepotOwnerDirectFastPath-L1 remains the clean source-depot control. It is not a
broad HZ6 default. DepotDescriptorRehomeBudget2048-L1
is the current bounded descriptor rehome candidate-control (`44.919M`, safety
clean, diagnostic `descriptor_used=36539` versus full rehome `77883`) and should
be validated with repeat/guard before any promotion. SlotOwnerConsumerDryRun-L1
is diagnostic-only and should not enter production speed-ranking tables.

Do not add new lanes to the selected family unless they pass:

```text
alloc_fail = 0
descriptor_exhausted = 0
route_register_fail = 0
source_block_exhausted = 0
```

## Cleanup Backlog

### P0: Keep Orientation Docs Small

Do:

```text
HZ6_SELECTED_FAMILY_SUMMARY.md:
  selected rows and latest numbers only

HZ6_LANE_GUIDE.md:
  lane dictionary and runner commands

HZ6_ELASTIC_CAPACITY_PLAN.md:
  source-depot / ElasticCapacity working-set decisions

current_task.md:
  historical ledger and detailed experimental notes
```

Do not:

```text
copy every benchmark table into every doc
leave superseded lanes near the top without a status label
```

### P1: Script Lane Registry

Current issue:

```text
win/run_win_hz6_capacity_matrix.ps1 has lane suffixes.
win/bench_app_like_allocator_build_common.ps1 has build flags and suffixes.
win/run_win_hz6_selected_family.ps1 has selected presets.
```

This is workable, but new lane names are easy to mistype.

Near-term rule:

```text
Add a lane in both capacity_matrix and build_common in the same commit.
Add selected-family presets only after the lane has a clean result.
Keep no-go lanes available only when they define a boundary.
Keep diagnostic-only lanes behind -DiagnosticHz6Probes and explicit diagnostic
lane names.
```

Longer-term cleanup:

```text
Create a single PowerShell lane manifest for:
  name
  suffix
  status
  family
  notes

Then let build and matrix scripts consume it.
```

See `HZ6_WINDOWS_LANE_REGISTRY.md` for the target shape and migration order.

### P2: Source Metadata Split

Current Larson RSS read:

```text
route192k is the clean static route lower bound.
route160k/128k fail warmup because route capacity saturates.
route192k-run512 is a previous selected lowest-RSS sibling/control.
route192k-run512 + descriptor no-backptr is the descriptor-layout comparison
control.
dir192k + descriptor no-backptr is a directory-capacity control.
dir192k + descriptor no-backptr + SourceBlock no-route-backptr is a clean
isolation control.
routepacked + dir192k + both no-backptr lanes is the RoutePackedMeta-L1
control.
routebytes16 + routepacked + dir192k + both no-backptr lanes is the current
route-entry comparison control.
routebytes16 + routepacked + dir192k + both no-backptr lanes + StorageOwner16
and OwnerSourceSideMeta-L2 is the selected lowest-RSS balance sibling.
FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 over OwnerSourceSideMeta-L2
are component lower-RSS controls/candidates.
FrontCachePackedMeta-L1 + SourceBlockPackedFlags-L1 over OwnerSourceSideMeta-L2
is the current clean minimum-RSS candidate/sibling, not the balance selection.
```

So the next RSS reduction should not be another route-capacity cut or another
blind source-run slot cut.

Next source cleanup target:

```text
Descriptor layout:
  guard no-backptr as the descriptor-layout comparison control
  guard dir192k/no-backptr as the directory-capacity comparison control
  guard routepacked/no-routebackptr/dir192k as the L1 control
  guard routebytes16/routepacked/no-routebackptr/dir192k as the clean
  route-entry comparison control
  guard OwnerSourceSideMeta-L2 over routebytes16/routepacked/no-routebackptr/
  dir192k as the selected low-RSS sibling
  guard FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 over
  OwnerSourceSideMeta-L2 as component lower-RSS controls/candidates
  guard the combined packed lane as the current minimum-RSS candidate/sibling
  do not promote allocator-local side-owner16
  keep storageowner16 as RSS-first evidence/control unless its lookup cost is
  reduced
  consider new side metadata only if the owner source is explicit

SourceBlock metadata layout:
  split or compress run bitmap / run metadata only after descriptor layout
  guard work
  keep route/source lifetime semantics unchanged
  keep safety counters clean
```

See `HZ6_SOURCE_MODULARIZATION.md`.

## Commit Hygiene

Prefer small commits with one of these scopes:

```text
Docs:
  lane/status/current-task cleanup only

Scripts:
  runner or lane registry changes only

Source:
  allocator implementation changes only

Results:
  benchmark markdown files only, usually paired with docs
```

Generated logs should usually stay out of commits unless a log is the result.
