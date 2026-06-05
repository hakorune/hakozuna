# HZ6 Selected Family Summary

This is the short, paper-facing map of the current HZ6 Windows lanes. Use
`HZ6_LANE_GUIDE.md` as the detailed lane dictionary and `current_task.md` as the
experiment ledger.

For cleanup rules and the next source modularization target, see
`HZ6_REPO_HYGIENE.md` and `HZ6_SOURCE_MODULARIZATION.md`.

## Selected Rows

| Family | Selected lane | Median ops/s | Peak KB | Status |
| --- | --- | ---: | ---: | --- |
| mixed_ws balanced | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | 67.462M | 110,888 | clean selected |
| mixed_ws wide_ws | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | 22.674M | 140,320 | clean selected |
| random_mixed small | `strict + sameownerfast-descavail-noboost-route4k` | 45.755M | 4,968 | clean selected |
| random_mixed medium | `strict + sameownerfast-descavail-noboost-route4k` | 42.408M | 4,964 | clean selected |
| random_mixed mixed | `strict + sameownerfast-descavail-noboost-route4k` | 41.306M | 4,964 | clean selected |
| mixed_ws larger_sizes speed | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 26.404M | 71,040 | clean selected |
| mixed_ws larger_sizes rss | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 27.178M | 71,012 | clean selected |
| Larson T16 full 10k throughput/RSS | `speed + ownerlocalityfast-rsscap-2-desc160k` | 44.754M | 808,488 | clean selected |
| Larson T16 full 10k lower RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k` | 45.092M | 716,324 | clean selected sibling |
| Larson T16 full 10k routebytes16 control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512` | 40.750M | 449,128 | clean superseded control |
| Larson T16 full 10k lowest-RSS balance sibling | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | 40.754M | 439,912 | clean selected balance sibling |
| Larson T16 full 10k FrontCachePacked component | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512` | 41.131M | 430,692 | clean lower-RSS component candidate/control in the SourceBlockPacked closeout matrix |
| Larson T16 full 10k SourceBlock packed candidate | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512` | 41.070M | 435,304 | clean lower-RSS candidate; `source_block_entry_bytes` projects/observes `144 -> 128`, lower RSS than L2 but not lower than FrontCachePacked |
| Larson T16 full 10k packed minimum RSS candidate | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512` | 44.864M | 412,280 | current packed minimum-RSS sibling; source10k repeat-3 safety-clean, source8k/source2k are warmup no-go |
| Larson T16 full 10k minimum RSS control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | 41.107M | 469,868 | clean superseded control |

## Active ElasticCapacity Rows

These rows are not broad selected defaults. They are the current source-depot /
ElasticCapacity working set used to reduce Larson RSS while preserving the
cross-owner contract.

| Role | Lane | Median ops/s | Peak KB | Status |
| --- | --- | ---: | ---: | --- |
| Elastic descriptor+route depot | `speed + ...elasticdescroute-desc16k-...source10k-route16k-run512` | 42.770M | 246,880 | candidate-watch; best descriptor+route depot RSS/throughput shape before source-depot work |
| Elastic descriptor+source+route depot | `speed + ...elasticdescsource-route-desc16k-...source64-route16k-run512` | 41.733M | 227,852 | lower-RSS source-depot component/control |
| Depot owner direct fast path | `speed + ...elasticdescsource-route-depotownerdirect-...source64-route16k-run512` | 46.273M | 224,612 | current source-depot candidate-watch; guard rows safety-clean |
| Slot owner consumer dry-run | `speed + diagnostic + ...elasticdescsource-route-slotownerconsumerdryrun-...source64-route16k-run4096` | 36.691M | 233,556 | diagnostic-only; `would_skip_l2=687536440`, `false_positive=0`; not speed-rankable |

Source:
- `docs/benchmarks/windows/paper/hz6_selected_family/selected-family-desc17-refresh/`
- `docs/benchmarks/windows/paper/hz6_selected_family/widews-routeonly-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-metadata-slim-route192-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-sourcerun-metaslim-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-lowest-rss-default-check/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-run512-routeslim-l1/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-run512-desc158-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-run512-descriptorlayout-l1d/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-run512-nobackptr-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-nobackptr-selected-guard/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-descriptor-layout-l2-dryrun-clean/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-directory-cap-l1-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-sourceblock-noroutebackptr-dir192k-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-routepacked-l1-repeat/`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-storageowner16-l1-repeat/`
- `docs/benchmarks/windows/paper/hz6_routepacked_meta_l2_dryrun/`
- `docs/benchmarks/windows/paper/hz6_routepacked_meta_l2_behavior/`
- `docs/benchmarks/windows/paper/hz6_hz5_rss_gap/`
- `docs/benchmarks/windows/paper/hz6_owner_source_side_meta_l2/`
- `docs/benchmarks/windows/paper/hz6_frontcache_packed_l1/`
- `docs/benchmarks/windows/paper/20260605_042827_hz6_capacity_matrix_windows.md`
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-sourceblockpacked-closeout/`
- `docs/benchmarks/windows/paper/20260605_051427_hz6_capacity_matrix_windows.md`
- `docs/benchmarks/windows/paper/hz6_route_linearwrap_l1_guard/`
- `docs/benchmarks/windows/paper/hz6_route_loopcarry_l1_repeat/`
- `docs/benchmarks/windows/paper/hz6_depot_owner_direct_repeat3/`
- `docs/benchmarks/windows/paper/hz6_depot_owner_direct_guard_matrix/`
- `docs/benchmarks/windows/paper/hz6_slot_owner_consumer_dryrun_full10k/`

## Evidence Rows

| Family | Lane | Read |
| --- | --- | --- |
| mixed_ws balanced/wide_ws pressure | `rss + descavail-noboost-route4k` | Very fast and very low RSS, but not clean: high `alloc_fail` / source-block exhaustion. Keep as pressure evidence only. |
| mixed_ws wide-speed sibling | `rss + mixedclean-front16k-sourcerun-desc20k-source2k-route20k` | Clean sibling: wide_ws `21.498M / 142676 KB`, but higher RSS than desc17. |
| mixed_ws route-only wide_ws L1 | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route18k` | Previous selected wide_ws sibling. Keeps descriptor/transfer/source/frontcache at desc17 but raises route capacity to 18K. Repeat-3: balanced `64.923M / 111248 KB`, wide_ws `22.184M / 140456 KB`. Superseded by route17-linearwrap in the latest guard. |
| mixed_ws LinearWrap-L1 | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap` | Previous selected mixed_ws row after route hash/probe cleanup. Guard repeat-3: balanced `69.821M / 110836 KB`, wide_ws `22.964M / 140280 KB`, larger_sizes guard `33.720M / 77992 KB`; safety clean. Superseded by LoopCarry-L1 in same-run repeat-3. |
| mixed_ws LoopCarry-L1 | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Selected mixed_ws row. It preserves the linearwrap route semantics and carries the exact-route probe index through the loop. Repeat-3 versus linearwrap: balanced `67.462M / 110888 KB`, wide_ws `22.674M / 140320 KB`, larger_sizes `34.032M / 78008 KB`; safety clean. |
| mixed_ws desc16 boundary | `mixedclean-front16k-sourcerun-desc16k-*` | No-go for wide_ws. Transfer2304/2560 does not remove `alloc_fail=6943`; desc17 is the current clean lower bound. |
| Larson lower-RSS control | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source12k` | Lower RSS than source16k but lower throughput; useful control, not selected. |
| Larson route boundary | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route160k/128k`, plus `route160k-run512` / `route128k-run512` | No-go: route table saturates during full-10k warmup. Under run512, route160k and route128k still hit `route_register_fail=3` / `alloc_fail=1`, so route192k is the current clean route lower bound. |
| Larson source-run metadata slim | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k-run512` | Previous selected lowest-RSS sibling/control: repeat-3 clean at `48.512M / 499820 KB`. Run1024 is clean control at `44.396M / 518256 KB`. |
| Larson descriptor boundary | `ownerlocalityfast-rsscap-2-desc158k-front4k-thindesc-source16k-route192k-run512` | Clean tiny-RSS sibling: repeat-3 `40.400M / 498080 KB`. Desc156k and below are no-go from `descriptor_exhausted=3` / `alloc_fail=1`, so static descriptor capacity cuts are effectively closed. |
| Larson descriptor layout | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512` | Descriptor no-backptr L1 removes the per-descriptor allocator pointer and passes allocator explicitly through lifecycle helpers. Repeat-3 clean at `40.710M / 476784 KB`; diagnostic entry size is `48 -> 40` bytes and descriptor table bytes are `127926272 -> 106954752`. Strong keep / comparison control before the dir192k, SourceBlock no-route-backptr, and routepacked refinements. |
| Larson descriptor L2 dry-run | diagnostic only | Owner packing to 16-bit does not shrink the no-backptr descriptor: `descriptor_owner16_hot_entry_bytes=40`, savings `0`. Ownerless hot descriptor projects `32` bytes and about `20971520` bytes additional table savings versus no-backptr. Next candidate is side-owner / ownerless hot descriptor metadata, not owner16 packing. |
| Larson side-owner16 L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512` | No-go / evidence. It reaches a 32-byte hot descriptor entry and lowers descriptor-table bytes to `96468992`, but the allocator-local side-owner table breaks cross-owner lifecycle: `route_invalid=11739`, `remote_free_transfer_fail=11739`, and `lifecycle_foreign_free_invalid=11739`. Keep no-backptr as a comparison control; routebytes16 is now a comparison control, OwnerSourceSideMeta-L2 is the selected low-RSS row, and side metadata must be owner-source-aware. |
| Larson descriptor-source diagnostic | diagnostic only | Confirms why side-owner16 is unsafe: no-backptr run512 is safety-clean while `descriptor_source_route_allocator_mismatch` is about `447.5M`. Route rehome makes route owner and descriptor-storage owner diverge heavily, so owner side metadata cannot be keyed by current or route allocator alone. |
| Larson descriptor-storage diagnostic | diagnostic only | Confirms the storage owner is discoverable but often foreign to both the route allocator and current allocator. In the 2026-06-04 recheck, no-backptr stayed safety-clean with `descriptor_storage_miss=0`, while `descriptor_storage_route_allocator_mismatch=420.2M` and `descriptor_storage_current_allocator_mismatch=420.3M`. Sideowner16 still produced `route_invalid=11626` and `remote_free_transfer_fail=11626`, so no-backptr remains a descriptor-layout control; routebytes16 became the clean comparison control that OwnerSourceSideMeta-L2 superseded for lowest RSS. |
| Larson directory capacity L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512` | Directory-capacity control, superseded by RoutePackedMeta-L1 for selected lowest-RSS comparisons. Repeat-3 clean at `44.580M / 472176 KB`, reducing owner-locality/shared-directory bytes from `18874368` to `14155776`. Same-run no-backptr control is `45.310M / 476788 KB`; dir192k trades about 1.6% speed for about 4.6 MB lower peak RSS. `dir128k` and `dir96k` are no-go controls: lower RSS but owner locality misses and full-table probes appear. |
| Larson SourceBlock no-route-backptr L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | Clean minimum-RSS sibling/control, superseded by RoutePackedMeta-L1 for both speed and RSS. It removes only the SourceBlock route-backend back-pointer and passes allocator explicitly to SourceBlock release/unregister helpers. Repeat-3: `41.107M / 469868 KB`, `source_block_entry_bytes=136`, safety clean. |
| Larson RoutePackedMeta L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-source16k-route192k-run512` | Comparison control. It keeps descriptor no-backptr, SourceBlock no-route-backptr, dir192k, source16k, route192k, and run512, then packs route entry front/class/state into a 32-bit meta word and moves bytes to a side array. Repeat-3 full 10k historical: `47.616M / 456048 KB`; same-run L2 A/B repeat-3: `45.079M / 456040 KB`, safety clean. |
| Larson RoutePackedMeta L2 routebytes16 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512` | Superseded clean comparison control. Raw uint16 route bytes overflow only on 64KiB envelopes (`route_bytes16_overflow=147`, `route_bytes16_max=65536`), while biased `uint16_t(bytes - 1)` is clean (`minus1_overflow=0`, `minus1_zero=0`). Same-run full-10k A/B repeat-3: routepacked `45.079M / 456040 KB`, routebytes16 `48.367M / 449144 KB`, safety clean. Same-run L2 promotion repeat-3: routebytes16 control `40.750M / 449128 KB`, OwnerSourceSideMeta-L2 `40.754M / 439912 KB`. |
| Larson OwnerSourceSideMeta L1 dry-run | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-ownersourcedryrun-source16k-route192k-run512` | Diagnostic-only follow-up to the routebytes16 comparison-control lane. Full 10k run=1: `46.202M / 449164 KB`, safety clean, `owner_source_side_meta_foreign=871979714`, `owner_source_side_meta_miss=0`, `owner_source_side_meta_probe_max=1`. It shows the next side-owner design must remove scan-frequency cost rather than deepen lookup. |
| Larson StorageOwner16 L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-storageowner16-noroutebackptr-dir192k-routepacked-source16k-route192k-run512` | RSS-first descriptor side metadata evidence/control. It fixes the allocator-local side-owner16 safety issue by resolving side-owner storage through descriptor storage ownership. Repeat-3 full 10k: `42.024M / 444520 KB`, safety clean. Keep as evidence/control; do not replace RoutePackedMeta-L1 because it saves about 11.5 MB RSS but loses about 12% throughput. |
| Larson OwnerSourceSideMeta L2 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | Current selected lowest-RSS balance sibling. It keeps routebytes16 and StorageOwner16 ownerless descriptors, then stores the descriptor-storage owner hint on each SourceBlock. Repeat-3 full 10k: routebytes16 control `40.750M / 449128 KB`, L2 `40.754M / 439912 KB`. Worker-warmup run=1: routebytes16 `40.126M / 448948 KB`, L2 `40.787M / 439740 KB`. Diagnostic validation: `owner_source_side_meta_l2_hit=1253200849`, all L2 miss/mismatch counters zero, safety clean. |
| Larson SourceBlockPackedFlags L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512` | Lower-RSS SourceBlock metadata component candidate/control on top of OwnerSourceSideMeta-L2. It packs `source_kind`, `active`, `route_registered`, and `run_active` into a SourceBlock flag word while preserving source release and owner-source side metadata. Repeat-3 closeout: `41.070M / 435304 KB`, safety clean. |
| Larson combined packed minimum-RSS candidate | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512` | Current packed minimum-RSS sibling. It combines FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 over OwnerSourceSideMeta-L2, then trims SourceBlock capacity to source10k. Repeat-3 full 10k: `44.864M / 412280 KB`, safety clean. Source12k is backup/control; source8k and source2k are warmup no-go. Keep as minimum-RSS sibling/candidate, not broad throughput promotion. |
| Larson DepotOwnerDirectFastPath-L1 | `ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | ElasticCapacity source-depot candidate-watch. Main10k repeat-3: `46.273M / 224612 KB`, guard rows safety-clean. Diagnostic A/B cuts `owner_source_side_meta_l2_lookup` from `1547776055` to `489480577`. Not broad default yet. |
| Larson SlotOwnerConsumerDryRun-L1 | `ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerconsumerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096` | Diagnostic-only consumer evidence. Full10k run-1: `36.691M / 233556 KB`, safety clean, `would_skip_l2=687536440`, `false_positive=0`, `stale_generation=0`. This justifies a narrow logical-owner fast-path experiment, but the dry-run counter volume is not speed-rankable. |
| Larson lowest-RSS preset check | `larson-cross-owner-lowest-rss` | Default check now includes front4k, route192k, no-backptr route192k-run512, dir192k no-backptr, SourceBlock no-route-backptr, routepacked control, routebytes16 comparison control, OwnerSourceSideMeta-L2 selected balance sibling, FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 component candidates, and the combined packed minimum-RSS candidate. |
| Larson over-retention control | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k` | Passes but over-retains RSS; no promotion. |

## Current Read

```text
HZ6 is now a profile-family allocator:
  mixed balanced/wide:
    clean low-RSS selected rows exist.
    desc17/route17-linearwrap-loopcarry is now selected for both balanced and
    wide_ws. Route18 remains superseded route-capacity evidence/control.

  random_mixed:
    selected same-owner lane is stable and low-RSS,
    but it is not a speed leader versus HZ3/tcmalloc historical rows.

  larger_sizes:
    selected largerlowrss lane is clean and relatively low-RSS.
    The latest selected-family refresh is slower than the earlier isolated
    larger_sizes snapshot, so use the refresh values in paper-facing tables.

  Larson cross-owner:
    full-10k now has clean selected rows,
    and route192k-run512 cut the previous lowest-RSS sibling to about 500 MB.
    Descriptor no-backptr L1 cuts it further to about 477 MB by removing the
    allocator back-pointer from the hot descriptor shape.
    Side-owner16 L1 can shrink the hot entry to 32 bytes, but the first
    allocator-local side-table implementation is no-go because foreign
    descriptor ownership is read from the wrong owner source and creates
    invalid remote-transfer events.
    Descriptor-source diagnostics confirm the design issue: after route
    rehome, route owner and descriptor-storage owner diverge on hundreds of
    millions of frees in the Larson cross-owner row.
    Directory-capacity L1 finds one more low-risk static RSS cut:
    `dir192k` repeat-3 stays safety-clean and saves about 4.6 MB versus
    no-backptr, but is now a directory-capacity control. The cost is about
    -1.6% throughput versus the same-run no-backptr control.
    SourceBlockNoRouteBackptr-L1 removes the SourceBlock route-backend
    back-pointer and creates an even lower-RSS control at about 470 MB.
    RoutePackedMeta-L1 then shrinks the route entry hot payload to 24 bytes
    plus a bytes side array. RoutePackedMeta-L2 stores route bytes as
    `uint16_t(bytes - 1)`, improving the same-run full 10k repeat-3 to
    `48.367M / 449144 KB`; it is now the routebytes16 comparison control.
    OwnerSourceSideMeta-L2 preserves that routebytes16 throughput shape while
    moving the selected Larson lowest-RSS balance sibling to
    `40.754M / 439912 KB`.
    FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 are component
    lower-RSS controls/candidates.  Their combined packed source10k lane is the
    current packed minimum-RSS sibling at `44.864M / 412280 KB`; source8k and
    source2k remain warmup no-go controls.
    ElasticCapacity source-depot work now has a separate candidate-watch:
    DepotOwnerDirectFastPath-L1 at `46.273M / 224612 KB`.  The
    SlotOwnerConsumerDryRun-L1 row is diagnostic-only and only justifies a
    narrow logical-owner fast-path experiment.
    StorageOwner16-L1 is safety-clean RSS-first evidence/control at
    `42.024M / 444520 KB`, but it is not selected because it trades about 12%
    throughput for about 11.5 MB RSS.
    `dir128k` and `dir96k` over-tighten the owner-locality/shared-directory
    tables and regress speed.
    The route table cannot be statically trimmed below route192k under the
    current representation; route160k-run512 and route128k-run512 fail warmup.
    Static descriptor capacity can be trimmed only to desc158k, which saves
    about 1.7 MB in the repeat-3 median; desc156k and below fail warmup.
    Therefore the current RSS direction is descriptor layout/lifecycle, not
    more static capacity cuts.
    RSS is still higher than system/mimalloc/tcmalloc references, but the
    metadata table gap is now much smaller than the previous route192k row.
```

## Next Attack Order

```text
1. Build a compact cross-allocator comparison table using the selected rows.
   Purpose:
     separate clean HZ6 rows from pressure/control rows before paper work.

2. Attack one of two remaining weaknesses:
   A. wide_ws throughput:
      route-only L1 found a clean answer: desc17/route18 keeps desc17
      descriptor/source/frontcache shape and improves wide_ws with about
      +284 KB RSS versus desc17/route17. Next wide_ws work should not be more
      blind descriptor capacity; it should look at route representation or
      class-specific route load.
   B. Larson RSS:
      OwnerSourceSideMeta-L2 over routebytes16/routepacked/no-routebackptr/
      dir192k is the selected lowest-RSS balance sibling.
      FrontCachePackedMeta-L1 and SourceBlockPackedFlags-L1 are component
      lower-RSS controls/candidates, and the combined packed lane is the
      current minimum-RSS candidate/sibling.
      Keep plain
      no-routebackptr/dir192k, routepacked/no-routebackptr/dir192k, and
      routebytes16 as clean controls.
      Side-owner16 L1 proves a 32-byte descriptor hot entry is possible, but
      allocator-local owner side metadata is unsafe for cross-owner lifecycle.
      Further RSS work should not be more static route/descriptor/directory
      trimming; it should inspect the remaining metadata table/payload split
      or move to a broader route / descriptor ownership representation rewrite.
```
