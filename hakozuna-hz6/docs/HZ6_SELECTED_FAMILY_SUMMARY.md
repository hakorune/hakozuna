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
| random_mixed small | `strict + sameownertrustedfree-descavail-noboost-route4k` | 44.923M | 4,636 | clean selected; repeat-5 beats former sameownerfast control by +4.1% with flat RSS |
| random_mixed medium | `strict + sameownertrustedfree-descavail-noboost-route4k` | 44.256M | 4,636 | clean selected; repeat-5 beats former sameownerfast control by +5.0% with flat RSS |
| random_mixed mixed | `strict + sameownertrustedfree-descavail-noboost-route4k` | 41.798M | 4,632 | clean selected; repeat-5 beats former sameownerfast control by +4.5% with flat RSS |
| mixed_ws fixed 256B..16K | `speed + sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | candidate-watch: earlier repeat-3 was broadly positive; 2026-06-06 follow-up showed the shape is workload-sensitive, with 8K/16K wins but 4K/balanced/larger_sizes wobble depending on run | RSS generally higher than DirectLocalFreeReuse | selected-small evidence, not broad/default |
| selected-small hybrid boundary | `speed + directlocalsmall8k-sameownerlarge-largerlowrss-front8k-sourcerun-desc8k-route8k` | class-gated DirectLocalFreeReuse <=8K plus SameOwnerFast >=16K control; focused refresh only gives a 2K witness and loses 8K/16K shape | RSS flat | no-go/control, not selected |
| mixed_ws larger_sizes speed | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 26.404M | 71,040 | clean selected |
| mixed_ws larger_sizes rss | `speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k` | 27.178M | 71,012 | clean selected |
| Larson T16 full 10k throughput/RSS | `speed + ownerlocalityfast-rsscap-2-desc160k` | 44.754M | 808,488 | clean selected |
| Larson T16 full 10k lower RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k` | 45.092M | 716,324 | clean selected sibling |
| Larson T16 full 10k routebytes16 control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-source16k-route192k-run512` | 40.750M | 449,128 | clean superseded control |
| Larson T16 full 10k lowest-RSS balance sibling | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | 40.754M | 439,912 | clean selected balance sibling |
| Larson T16 full 10k FrontCachePacked component | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512` | 41.131M | 430,692 | clean lower-RSS component candidate/control in the SourceBlockPacked closeout matrix |
| Larson T16 full 10k SourceBlock packed candidate | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512` | 41.070M | 435,304 | clean lower-RSS candidate; `source_block_entry_bytes` projects/observes `144 -> 128`, lower RSS than L2 but not lower than FrontCachePacked |
| Larson T16 full 10k packed minimum RSS candidate | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512` | 44.864M | 412,280 | current packed minimum-RSS sibling; source10k repeat-3 safety-clean, source8k/source2k are warmup no-go |
| Larson T16 full 10k Elastic speed-balance sibling | `speed + ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | repeat-3 main10k refresh: 42.930M | 224,720 | clean speed-balance control; DirectFreeTrustedLocalCache improves every main/worker 1k/4k/10k guard row over DepotOwnerDirect (`avg +2.60%`, min `+1.34%`) with essentially unchanged RSS |
| Larson T16 full 10k Elastic lower-RSS front1k sibling | `speed + ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front1k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | latest repeat-3 main10k guard: 41.448M | 204,092 | clean selected lower-RSS sibling. Frontcache-boundary repeat-3 across main/worker 1k/4k/10k saves about 20.7 MiB RSS on every row, wins speed on 4/6 rows, and keeps the largest observed regression to main10k `-3.06%`. Keep front4k as the speed-balance control; front2k remains evidence/control. |
| Larson T16 full 10k minimum RSS control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | 41.107M | 469,868 | clean superseded control |

Selected-small note:
`sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k`
is kept as the selected-small candidate-watch row for 256B..16K fixed-size
checks. The former DirectLocalFreeReuse lane remains the simple
control/baseline and the conservative fallback if a paper-facing selected-small
row needs maximum stability.
The `sourceblockroute-behavior-dynmap-small8k-*` follow-up proves class-gated
late range-index registration works, but it still regresses 4K/balanced enough
to remain control evidence rather than selected wiring.  The 2026-06-07
boundary refresh confirms that reading: dynmap-small8k lost to dynmap/direct on
4K/8K/16K, so it is not a selected-small rescue gate.
MidPage 8K/32K prefill now uses the shared SourceBlock helper, so the small/mid
boundary is structurally unified with Toy; this does not create a new promoted
speed lane by itself. `directlocaltrusted-*`, `directlocalpacked-*`,
`directlocalexact-*`, and the former DirectLocalFreeReuse selected-small lane are
HZ6-only research/control rows. They remain useful for attribution, but are not
selected-family rows unless a later repeat re-promotes them.
The 2026-06-07 Toy-small / SourceBlockRoute closeout keeps selected-small as
candidate-watch rather than a universal tiny-object claim.  SmallRunRoute-L1 /
TinyRunRoute was added after that closeout and is safety-clean, but the repeat-5
refresh keeps it as mechanism evidence: it is mostly a 1K/4K clue and does not
replace the selected dynmap row.  If HZ6 later targets 256B..2K speed directly,
continue with SmallRunFront/TinyRunRoute attribution while keeping ToyFront as
the route-safe reference implementation.
The follow-up `smallrunroute-behavior-range64k-toyonly-*` row is a cleaner
Toy-low control: 64 KiB range-index granularity matches Toy source blocks and
Toy-only late registration avoids returning SmallRunRoute VALID for non-Toy
runs. It improves 256B/512B/1K in the focused repeat-3, but still loses enough
2K/4K/8K/16K shape to remain control evidence rather than a selected-small
replacement.
The `smallrunroute-behavior-range64k-toyarmed-*` control removes empty-table
range-index probes before any eligible Toy range is registered. It improves
256B/512B/4K/8K versus toyonly in the focused repeat-3, but the 16K row
regresses, so it remains candidate/control evidence rather than selected
wiring.
The `smallrunroute-behavior-range64k-toyarmed-slotmax1k-*` follow-up narrows
SmallRunRoute registration to Toy source-run slots up to 1K.  It is a focused
low-class control, not a selected-small replacement: Toy 2K and 4K share the
same 4K physical slot class, so static registration policy cannot rescue 2K
without also taxing 4K.
The post-route-closeout selected-small repeat-5 keeps this conclusion:
`dynmap` wins 256B/1K/8K, `directlocalfreereuse` wins 512B/16K,
`sameownerfast` wins 2K, and `slotmax1k` wins 4K; all rows are safety-clean.
This is useful mechanism evidence, but not a single-lane promotion signal.

LargeDirect note:
`largedirectretain16m-largerlowrss-front8k-sourcerun-desc8k-route8k` is the
current practical >1MiB direct-large candidate/control, not a broad selected
default.  It retains exact-size direct-large objects up to 16 MiB and keeps the
generic LargeSpan <=1MiB central-pool path unchanged.  The cap ladder shows 8M
is too small for 4M/8M (`source_alloc` remains high), while 16M removes churn
for 2M/4M/8M and 32M is mainly an upper-bound/control.  Legacy rows
`hz6-*-largedirectretain16m-largerlowrss` and
`hz6-*-largedirectretain32m-largerlowrss` are wired in the legacy matrix.  Use
16M as the practical LargeDirect candidate/control and 32M as upper-bound
evidence; neither is a broad selected default because the generic LargeSpan
<=1MiB path remains separate.
The 2026-06-07 single-run cross-allocator slice now confirms 16M as the cleaner
candidate-control: it wins 512K/2M/8M, stays close on 1M/4M, and keeps direct
large RSS around 9 MiB.  32M remains an upper-bound/control row rather than the
default assumption.

Redis note:
Redis-like rows are intentionally outside the selected-family table.  The
current Redis candidate-control is
`redislowrss-sourcerun-desc8k-route8k`; use it when the benchmark goal is
Redis-like completion/low-RSS evidence.  `...tombcompact` remains conservative
RouteTombstoneCompact evidence.  `...tombcompact-aggr1024` and
`...tombcompact-aggr2048` are threshold-boundary controls only: they prove the
half-cap threshold can suppress compaction on 8K route tables, but the first
refreshed Redis long run did not produce a clean winner and RSS rises.  Do not
promote Redis tombstone threshold knobs into selected-family rows without a
fresh repeat showing SET/GET/LPUSH/RANDOM and RSS improve together.

## Active ElasticCapacity Rows

These rows are not broad selected defaults. They are the current source-depot /
ElasticCapacity working set used to reduce Larson RSS while preserving the
cross-owner contract.

| Role | Lane | Median ops/s | Peak KB | Status |
| --- | --- | ---: | ---: | --- |
| Elastic descriptor+route depot | `speed + ...elasticdescroute-desc16k-...source10k-route16k-run512` | 42.770M | 246,880 | candidate-watch; best descriptor+route depot RSS/throughput shape before source-depot work |
| Elastic descriptor+source+route depot | `speed + ...elasticdescsource-route-desc16k-...source64-route16k-run512` | 41.733M | 227,852 | lower-RSS source-depot component/control |
| Depot owner direct fast path | `speed + ...elasticdescsource-route-depotownerdirect-...source64-route16k-run512` | guard repeat-3 main10k: 39.833M | 224,592 | previous selected Larson/Elastic low-RSS sibling; now the clean control for DirectFreeTrustedLocalCache |
| Depot owner direct + DirectFreeTrustedLocalCache | `speed + ...elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-...source64-route16k-run512` | repeat-3 main10k: 43.75M | 224,724 | selected Larson/Elastic low-RSS sibling; safety clean, `avg +2.60%` and min `+1.34%` vs DepotOwnerDirect over main/worker 1k/4k/10k |
| Depot owner direct + TrustedLocalCache only | `speed + ...elasticdescsource-route-depotownerdirect-trustedlocalcache-...source64-route16k-run512` | repeat-3 main10k: 41.63M | 224,712 | boundary/control; without DirectFree it does not remove the intended free-side second owner check and does not broadly beat DepotOwnerDirect |
| Slot owner consumer dry-run | `speed + diagnostic + ...elasticdescsource-route-slotownerconsumerdryrun-...source64-route16k-run4096` | 36.691M | 233,556 | diagnostic-only; `would_skip_l2=687536440`, `false_positive=0`; not speed-rankable |
| Slot owner logical fast path | `speed + ...elasticdescsource-route-slotownerlogical-...source64-route16k-run4096` | 38.494M | 239,484 | safety-clean behavior no-go/control; broad sparse probing is too expensive |
| Owner-equal callsite dry-run | `speed + diagnostic + ...elasticdescsource-route-depotownerdirect-ownerequalcallsite-dryrun-...source64-route16k-run512` | 42.434M | 224,612 | diagnostic-only; owner_equal pressure is free/local-cache dominated |
| Free/local owner predicate dry-run | `speed + diagnostic + ...elasticdescsource-route-depotownerdirect-freelocalownerpredicate-dryrun-...source64-route16k-run512` | 41.049M | 224,628 | diagnostic-only; free/local-cache owner_equal pressure is mostly depot/source-block backed |
| Depot descriptor owner-equal fast path | `speed + ...elasticdescsource-route-depotownerdirect-depotownerequal-...source64-route16k-run512` | 40.531M | 227,708 | safety-clean behavior no-go/control; earlier depot-owner branch is slower than depotownerdirect |
| Unified depot drain dry-run | `speed + diagnostic + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdraindryrun-...source64-route16k-run4096` | 43.101M | 235,420 | diagnostic-only; whole-block localize remains blocked, but every probed transfer slot is run-matched and slot-localizable |
| Depot slot localize | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotslotlocalize-...source64-route16k-run4096` | 44.658M | 240,636 | behavior no-go/control; slot-local storage hits heavily but leaks `route_invalid=125` / `remote_free_transfer_fail=125` |
| Depot slot transfer scoped | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotslottransfer-...source64-route16k-run4096` | 41.596M | n/a | safe transfer-scoped control/evidence; sparse slot recording works without broad storage override, `route_invalid=0` / `remote_free_transfer_fail=0` |
| Depot descriptor rehome dry-run | `speed + diagnostic + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehomedry-...source64-route16k-run4096` | 43.040M | n/a | diagnostic-only; `would_rehome=71811` with local descriptor availability and safety clean |
| Depot descriptor route-replace dry-run | `speed + diagnostic + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotroutereplacedry-...source64-route16k-run4096` | 41.332M | n/a | diagnostic-only; `would_commit=71811`, `current_route_same=71811`, no mismatch/conflict, safety clean |
| Depot descriptor rehome | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-...source64-route16k-run4096` | 44.853M | n/a | behavior evidence; `success=71811`, safety clean, but descriptor retention grows so not promotion yet |
| Depot descriptor rehome + capfree | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-capfree-...source64-route16k-run4096` | 43.602M | n/a | control/no-go for simple frontcache cap; safety clean but `descriptor_used` stays `77883`, showing retention is live consumer-local materialization rather than cold frontcache backlog |
| Depot descriptor rehome budget2048 | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-...source64-route16k-run4096` | 44.034M main10k / 45.404M worker10k | 217888 KB worker10k | Larson bounded rehome candidate-control; repeat/guard safety clean after depot run-meta zero-count guard, diagnostic `descriptor_used` drops to `36539`; broad smoke keeps it out of default promotion. The former mixed_ws wide_ws `0xc0000005` is now fail-closed by source-block activation/release guards, but the row still reports `route_invalid`/`route_miss`, so it remains no-go for broad selected defaults. |
| Depot owner direct + DirectFreeTrustedLocalCache + rehome budget2048 | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-...source64-route16k-run4096` | 39.525M main10k / 45.404M worker10k | 234696 KB main10k | composition guard/control; safety clean, but it loses the critical main10k row versus selected DFTLC (`42.369M / 227444 KB`) while RSS rises. Keep as evidence that bounded descriptor rehome does not compose into a better selected Elastic low-RSS lane. |
| Depot owner direct + DirectFreeTrustedLocalCache + rehome budget512/1024 | `speed + ...elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget512/1024-...source64-route16k-run4096` | 39.805M / 39.888M main10k | 240676 / 240684 KB main10k | budget-boundary control; safety clean, but smaller budgets do not solve the composition. Main10k RSS remains above selected DFTLC (`224732 KB` in the boundary guard), so the current selected Elastic low-RSS sibling stays DirectFreeTrustedLocalCache without rehome budget. |

Source:
- `docs/benchmarks/windows/paper/hz6_selected_family/sourceblockroute-dynmap-selected-small-20260606/`
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
- `docs/benchmarks/windows/paper/hz6_selected_family/larson-elastic-directfree-trustedlocalcache-repeat3/`
- `docs/benchmarks/windows/paper/hz6_selected_family/large-direct-retain32m-directpush-20260606/`
- `docs/benchmarks/windows/paper/hz6_large_direct_retain_cap_ladder_20260606/`
- `docs/benchmarks/windows/paper/hz6_large_direct_retain16m_crossalloc_slice_20260607/`
- `docs/benchmarks/windows/paper/hz6_slot_owner_consumer_dryrun_full10k/`
- `docs/benchmarks/windows/paper/hz6_owner_equal_callsite_dryrun_full10k/`
- `docs/benchmarks/windows/paper/hz6_flc_owner_predicate_dryrun_full10k/`
- `docs/benchmarks/windows/paper/hz6_depot_owner_equal_fastpath_full10k/`
- `docs/benchmarks/windows/paper/hz6_depot_owner_equal_fastpath_diag_full10k/`
- `docs/benchmarks/windows/paper/hz6_elastic_depot_runmeta_drain_dryrun_full10k/`
- `docs/benchmarks/windows/paper/hz6_elastic_depot_slot_localize_full10k/`
- `docs/benchmarks/windows/paper/hz6_elastic_depot_slot_localize_diag_full10k/`

## Evidence Rows

## Runner Smoke

```text
2026-06-06 selected-family wrapper smoke:
  win/run_win_hz6_selected_family.ps1 -SelectedFamily -Runs 1

  Completed preset groups:
    selected-mixed-lowrss
    selected-random-sameowner
    selected-small-fixed
    selected-larger-lowrss
    larson-cross-owner-selected

  Safety scan:
    route_invalid = 0
    route_miss = 0
    alloc_fail = 0
    descriptor_exhausted = 0
    route_register_fail = 0
    source_block_exhausted = 0
    remote_free_transfer_fail = 0
    lifecycle_foreign_free_invalid = 0

  Note:
    This is a runner connectivity smoke.  Do not use the run-1 smoke values as
    paper medians.
```

| Family | Lane | Read |
| --- | --- | --- |
| mixed_ws balanced/wide_ws pressure | `rss + descavail-noboost-route4k` | Very fast and very low RSS, but not clean: high `alloc_fail` / source-block exhaustion. Keep as pressure evidence only. |
| mixed_ws wide-speed sibling | `rss + mixedclean-front16k-sourcerun-desc20k-source2k-route20k` | Clean sibling: wide_ws `21.498M / 142676 KB`, but higher RSS than desc17. |
| mixed_ws route-only wide_ws L1 | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route18k` | Previous selected wide_ws sibling. Keeps descriptor/transfer/source/frontcache at desc17 but raises route capacity to 18K. Repeat-3: balanced `64.923M / 111248 KB`, wide_ws `22.184M / 140456 KB`. Superseded by route17-linearwrap in the latest guard. |
| mixed_ws LinearWrap-L1 | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap` | Previous selected mixed_ws row after route hash/probe cleanup. Guard repeat-3: balanced `69.821M / 110836 KB`, wide_ws `22.964M / 140280 KB`, larger_sizes guard `33.720M / 77992 KB`; safety clean. Superseded by LoopCarry-L1 in same-run repeat-3. |
| mixed_ws LoopCarry-L1 | `rss + mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Selected mixed_ws row. It preserves the linearwrap route semantics and carries the exact-route probe index through the loop. Repeat-3 versus linearwrap: balanced `67.462M / 110888 KB`, wide_ws `22.674M / 140320 KB`, larger_sizes `34.032M / 78008 KB`; safety clean. |
| mixed_ws desc16 boundary | `mixedclean-front16k-sourcerun-desc16k-*` | No-go for wide_ws. Transfer2304/2560 does not remove `alloc_fail=6943`; desc17 is the current clean lower bound. |
| mixed_ws route16/route8 boundary | `mixedclean-front16k-sourcerun-desc17k-source2k-route16k/route8k-linearwrap-loopcarry` | No-go controls. Route16k keeps SourceBlock capacity clean but fails exact route registration (`route_register_fail=20833`) and triggers starvation aftershock; route8k is a hard route-pressure collapse that cascades into SourceBlock exhaustion. Route17k remains the selected safety boundary. |
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
| Larson DepotOwnerDirectFastPath-L1 | `ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | Previous selected Larson/Elastic low-RSS sibling and current clean control. Repeat-3 guard vs packed source10k is safety-clean, average `+0.52%` speed across main/worker 1k/4k/10k, and about `187-199 MiB` lower RSS. Diagnostic A/B cuts `owner_source_side_meta_l2_lookup` from `1547776055` to `489480577`. Not broad default. |
| Larson DepotOwnerDirectDirectFreeTrustedLocalCache | `ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | Selected Larson/Elastic low-RSS sibling. Repeat-3 vs DepotOwnerDirect improves every main/worker 1k/4k/10k row, `avg +2.60%`, min `+1.34%`, RSS essentially unchanged, safety clean. |
| Larson SlotOwnerConsumerDryRun-L1 | `ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerconsumerdryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096` | Diagnostic-only consumer evidence. Full10k run-1: `36.691M / 233556 KB`, safety clean, `would_skip_l2=687536440`, `false_positive=0`, `stale_generation=0`. This justifies a narrow logical-owner fast-path experiment, but the dry-run counter volume is not speed-rankable. |
| Larson SlotOwnerLogicalOwnerFastPath-L1 | `ownerlocalityfast-rsscap-2-elasticdescsource-route-slotownerlogical-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run4096` | Safety-clean behavior no-go/control. Non-diagnostic full10k: `38.494M / 239484 KB`; diagnostic full10k reports `logical_hit=717746510`, `stale_generation=0`, `owner_mismatch=0`, but broad sparse probing at every owner-equality entry loses speed versus depotownerdirect. |
| Larson OwnerEqualCallsiteDryRun-L1 | `ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-ownerequalcallsite-dryrun-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | Diagnostic-only callsite attribution. Full10k run-1: `42.434M / 224612 KB`, safety clean. `owner_equal_site_free=424522978`, `owner_equal_site_local_cache=424449034`, all remote/visible/transfer/same-owner callsites `0`, and `unknown=0`. This fixes the next design target: do not probe sparse slot-owner metadata at every owner_equal; use a cheaper free/local-cache state predicate first. |
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
    SameOwnerTrustedLocalFree-L1 is now selected: it avoids proving local
    owner twice on same-owner free and repeat-5 improves small/medium/mixed
    over the former sameownerfast row without safety/RSS cost.

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
    ElasticCapacity source-depot work is now split into speed-balance and
    lower-RSS selected siblings:
      front4k DFTLC remains the speed-balance control.
      front1k DFTLC is the selected lower-RSS sibling after the repeat-3
      main/worker 1k/4k/10k guard.
      front2k remains boundary evidence/control only.
    DepotOwnerDirectFastPath-L1 is now a superseded clean control.  The
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
Current selected rows are stable enough for comparison tables.  Do not mix
pressure/no-go/diagnostic lanes into those tables.

1. Keep selected profile rows fixed unless a same-run guard replaces them:
   mixed_ws:
     route17-linearwrap-loopcarry
   random_mixed:
     sameownertrustedfree-descavail-noboost-route4k
   larger_sizes:
     largerlowrss-front8k-sourcerun-desc8k-route8k
   Larson minimum-RSS sibling:
     frontcachepacked-sourceblockpacked-source10k-route192k-run512

2. Keep ElasticCapacity out of broad defaults, but include the clean
   DepotOwnerDirectDirectFreeTrustedLocalCache row as the selected
   Larson/Elastic low-RSS sibling:
   current selected Elastic low-RSS sibling:
     DepotOwnerDirectDirectFreeTrustedLocalCache
     repeat-3 main10k 43.75M / 224724 KB
     average +2.60% speed vs DepotOwnerDirect across main/worker 1k/4k/10k
     essentially unchanged RSS versus DepotOwnerDirect
   current lower-RSS evidence/control:
     ElasticDescriptorSourceRouteOverflow-L1
   current no-go/control:
     SlotOwnerLogicalOwnerFastPath-L1 in broad form

3. Completed ElasticCapacity free/local-cache attack:
   DirectFreeTrustedLocalCache-L1.
   OwnerEqualCallsiteDryRun-L1 shows owner_equal pressure is dominated by
   free/local-cache:
     owner_equal_site_free=424522978
     owner_equal_site_local_cache=424449034
     remote/visible/transfer/same-owner callsites=0

   Therefore:
     do not sparse-probe slot-owner metadata at every owner_equal() entry.
     do not revive whole-SourceBlock localize.
     do not add another isolated static cap knob.

   FreeLocalCacheOwnerPredicate-L0 was added first as a strong witness:
       probe=821934976
       depot_descriptor=693768653
       foreign_descriptor=774457636
       source_block=821934976
       source_run_active=0

   Behavior result:
     DepotOwnerDirectDirectFreeTrustedLocalCache is now selected for the
     Larson/Elastic low-RSS sibling. It avoids the second owner read only after
     hz6_free() has already proven local ownership, rather than adding another
     broad owner_equal branch.

4. DepotDescriptorOwnerEqualFastPath-L1 was tested:
   non-diagnostic full10k:
     40.531M / 227708 KB
   diagnostic full10k:
     probe=824802008
     hit=696139014
     fallback=128591183

   Result:
     safety-clean no-go/control.
     It is slower than DepotOwnerDirectFastPath-L1 because descriptor_owner()
     already performs the depot-direct owner read before the L2 path.

5. Next:
   UnifiedDepotDrainDryRun-L1 was added on the depot-runmeta +
   depotownerdirect lane.  Full10k diagnostic:
     43.101M / 235420 KB
     elastic_depot_drain_probe=79485
     storage_mismatch=79485
     run_match=79485
     ref_shared=79485
     owner_match=79485
     would_slot_localize=79485
     would_block_whole_localize=79485

   Read:
     close the slot-owner logical / depot-owner-equal shortcut family as
     evidence for now.  Whole-block localize is still blocked by shared
     SourceBlocks, but slot-level depot drain/localize now has a strong,
     safety-clean witness.  The next behavior candidate should localize
     slot ownership/storage state, not move entire SourceBlocks and not add
     another owner_equal branch.

6. DepotSlotLocalize-L1 was tested:
   non-diagnostic full10k:
     44.658M / 240636 KB
     route_invalid=125
     remote_free_transfer_fail=125
   diagnostic full10k:
     36.191M / 240656 KB
     attempt=success=30733367
     storage_hit=401643367
     storage_miss=4465070
     storage_stale=0

   Result:
     no-go/control.  Slot-local storage is real, but returning it through the
     general owner_source storage path is not fail-closed enough.  The next
     design must keep slot-localization scoped to transfer reuse or a
     descriptor-local path that cannot leak stale storage into normal free.
```
