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
| Larson T16 full 10k lowest RSS | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512` | 44.580M | 472,176 | clean selected sibling candidate |
| Larson T16 full 10k minimum RSS control | `speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | 41.107M | 469,868 | clean lowest-RSS control |

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
- `docs/benchmarks/windows/paper/hz6_route_linearwrap_l1_guard/`
- `docs/benchmarks/windows/paper/hz6_route_loopcarry_l1_repeat/`

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
| Larson descriptor layout | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-source16k-route192k-run512` | Descriptor no-backptr L1 removes the per-descriptor allocator pointer and passes allocator explicitly through lifecycle helpers. Repeat-3 clean at `40.710M / 476784 KB`; diagnostic entry size is `48 -> 40` bytes and descriptor table bytes are `127926272 -> 106954752`. Strong keep / comparison control for the selected dir192k lowest-RSS sibling. |
| Larson descriptor L2 dry-run | diagnostic only | Owner packing to 16-bit does not shrink the no-backptr descriptor: `descriptor_owner16_hot_entry_bytes=40`, savings `0`. Ownerless hot descriptor projects `32` bytes and about `20971520` bytes additional table savings versus no-backptr. Next candidate is side-owner / ownerless hot descriptor metadata, not owner16 packing. |
| Larson side-owner16 L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-sideowner16-source16k-route192k-run512` | No-go / evidence. It reaches a 32-byte hot descriptor entry and lowers descriptor-table bytes to `96468992`, but the allocator-local side-owner table breaks cross-owner lifecycle: `route_invalid=11739`, `remote_free_transfer_fail=11739`, and `lifecycle_foreign_free_invalid=11739`. Keep no-backptr selected; a future side-owner design must be owner-source-aware. |
| Larson descriptor-source diagnostic | diagnostic only | Confirms why side-owner16 is unsafe: no-backptr run512 is safety-clean while `descriptor_source_route_allocator_mismatch` is about `447.5M`. Route rehome makes route owner and descriptor-storage owner diverge heavily, so owner side metadata cannot be keyed by current or route allocator alone. |
| Larson descriptor-storage diagnostic | diagnostic only | Confirms the storage owner is discoverable but often foreign to both the route allocator and current allocator. In the 2026-06-04 recheck, no-backptr stayed safety-clean with `descriptor_storage_miss=0`, while `descriptor_storage_route_allocator_mismatch=420.2M` and `descriptor_storage_current_allocator_mismatch=420.3M`. Sideowner16 still produced `route_invalid=11626` and `remote_free_transfer_fail=11626`, so no-backptr remains selected. |
| Larson directory capacity L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-dir192k-source16k-route192k-run512` | Selected lowest-RSS sibling candidate. Repeat-3 clean at `44.580M / 472176 KB`, reducing owner-locality/shared-directory bytes from `18874368` to `14155776`. Same-run no-backptr control is `45.310M / 476788 KB`; dir192k trades about 1.6% speed for about 4.6 MB lower peak RSS. `dir128k` and `dir96k` are no-go controls: lower RSS but owner locality misses and full-table probes appear. |
| Larson SourceBlock no-route-backptr L1 | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-source16k-route192k-run512` | Clean minimum-RSS sibling/control. It removes only the SourceBlock route-backend back-pointer and passes allocator explicitly to SourceBlock release/unregister helpers. Repeat-3: `41.107M / 469868 KB`, `source_block_entry_bytes=136`, safety clean. Keep as lowest-RSS control; do not replace the dir192k/no-backptr selected sibling when throughput/RSS balance matters. |
| Larson lowest-RSS preset check | `larson-cross-owner-lowest-rss` | Default check now includes front4k, route192k, no-backptr route192k-run512, and dir192k no-backptr route192k-run512. Latest guard before dir192k promotion: front4k `42.460M / 716340 KB`, route192k `44.583M / 628848 KB`, no-backptr `42.324M / 476868 KB`, all safety clean. |
| Larson over-retention control | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source32k` | Passes but over-retains RSS; no promotion. |

## Current Read

```text
HZ6 is now a profile-family allocator:
  mixed balanced/wide:
    clean low-RSS selected rows exist.
    desc17/route17 remains the balanced lower-RSS row.
    desc17/route18 is now the selected wide_ws sibling: it keeps descriptor and
    transfer capacity at desc17 while adding only route slots, lifting wide_ws
    with a very small RSS increase.

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
    `dir192k` repeat-3 stays safety-clean, saves about 4.6 MB versus
    no-backptr, and is now the selected lowest-RSS sibling candidate. The
    cost is about -1.6% throughput versus the same-run no-backptr control.
    SourceBlockNoRouteBackptr-L1 removes the SourceBlock route-backend
    back-pointer and creates an even lower-RSS control at about 470 MB, but
    this row is a minimum-RSS sibling rather than the throughput/RSS selected
    replacement.
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
      keep dir192k/no-backptr as the selected throughput/RSS sibling and keep
      no-route-backptr/dir192k as the minimum-RSS control. Side-owner16 L1
      proves the 32-byte entry is possible, but allocator-local owner side
      metadata is unsafe for cross-owner lifecycle. The next descriptor RSS
      attempt needs owner-source-aware side metadata or a broader route /
      descriptor ownership representation rewrite. Static route trimming is
      closed unless the route representation changes; static descriptor-cap
      trimming is nearly exhausted at desc158k, static directory trimming below
      dir192k is no-go under the current index representation, and SourceBlock
      route-backptr slimming is now done.
```
