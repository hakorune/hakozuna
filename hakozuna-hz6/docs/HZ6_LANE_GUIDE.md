# HZ6 Lane Guide

This document is the compact lane map for HZ6 Windows and Linux/Ubuntu
benchmarking. Keep long experiment notes out of this file; use it to answer
"which lane is which?" before running or comparing benchmarks.

For the short selected-row readout, see
[`HZ6_SELECTED_FAMILY_SUMMARY.md`](HZ6_SELECTED_FAMILY_SUMMARY.md).
For repo cleanup rules and the source modularization backlog, see
[`HZ6_REPO_HYGIENE.md`](HZ6_REPO_HYGIENE.md) and
[`HZ6_SOURCE_MODULARIZATION.md`](HZ6_SOURCE_MODULARIZATION.md).
For the Ubuntu LD_PRELOAD selected bundle and controls, see
[`HZ6_UBUNTU_PRELOAD_LANES.md`](HZ6_UBUNTU_PRELOAD_LANES.md).
For the current ElasticCapacity design target, see
[`HZ6_ELASTIC_CAPACITY_PLAN.md`](HZ6_ELASTIC_CAPACITY_PLAN.md).

## Quick Lane Classification

Use this section first.  The detailed tables below keep the evidence ledger, but
these rows answer which lanes should be used, watched, or avoided.

| Class | Lane / family | Use |
| --- | --- | --- |
| Selected profile lane | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Current balanced clean low-RSS row. |
| Selected sibling | `route18` mixedclean sibling | Current wide_ws sibling when wide_ws is the target; do not replace balanced route17/loopcarry by default. |
| Boundary / no-go control | `mixedclean-front16k-sourcerun-desc17k-source2k-route16k-linearwrap-loopcarry` | Route exact lower-bound evidence. It does not exhaust SourceBlock metadata, but exact route registration fails and triggers alloc-fail/starvation aftershock in wide_ws. Do not promote. |
| Boundary / hard no-go | `mixedclean-front16k-sourcerun-desc17k-source2k-route8k-linearwrap-loopcarry` / `...source4k-route8k...` | Route pressure collapse. Route registration failure cascades into SourceBlock cap exhaustion; source4k does not rescue wide_ws. Evidence only. |
| Selected profile lane | `sameownertrustedfree-descavail-noboost-route4k` | Current random_mixed same-owner speed row. SameOwnerTrustedLocalFree-L1 avoids the second owner check after `hz6_free()` already proved local ownership. Repeat-5 guard is safety-clean and beats the old selected lane on small/medium/mixed with flat RSS. |
| Superseded selected control | `sameownerfast-descavail-noboost-route4k` | Former random_mixed selected lane. Keep as the direct control for trusted-free guards and paper history. |
| Selected profile lane | `largerlowrss-front8k-sourcerun-desc8k-route8k` | larger_sizes RSS/speed row. |
| Ubuntu selected/default | `HZ6_ROUTE_LAST_HIT_CACHE_L1=1` | Linux/Ubuntu HZ6 default route shortcut. Caches the last exact route per allocator, validates descriptor pointer/generation/state/source-block liveness before returning VALID, and clears on exact unregister. Focused repeat-5 default-on versus explicit off improves local 8K/64K by +16%..+30%, remote 128K by +27%..+38%, and reuse 128K by +38%..+41%. |
| Ubuntu control-off | `HZ6_ROUTE_LAST_HIT_CACHE_L1=0` | Direct control for LastHitRouteCache-L1. Keep for A/B and regression attribution; route header inline remains no-go. |
| Ubuntu selected/default | `HZ6_TOY_CLASS_ID_FAST_ALLOC_L1=1` | Toy alloc shortcut for Linux/Ubuntu. `hz6_malloc()` already selected `class_id`, so Toy alloc validates against class-id bytes instead of re-running request-size classification. Repeat-10 suspicious-row guard is positive on 512B/1K/2K medians; keep explicit off as control. |
| Ubuntu diagnostic-only | `HZ6_TOY_SMALL_HOTPATH_DIAG_L1=1` / `[HZ6_TOY_SMALL]` | Toy/small hot-path L2 audit. Splits malloc front dispatch, frontcache pop, descriptor activation, free route lookup, owner check, cache push, and active-map bypass. Use only with `HZ6_DIAGNOSTIC_PROBES=1`; not speed-rankable. |
| Ubuntu diagnostic-only/closeout | `ToyActiveMapFreeProbeAudit-L1` | Extends Toy/small diagnostics with Toy/class4 active-map free hit base-slot ratio and probe length. Raw `hz6_midpage_payload_trim_ab_20260615_215547` shows class4 free lookup is already near base-slot optimal on mid-small rows, around `94%` base hits and `1.06..1.07` average probes, so Toy active-map capacity/probe remains a closed default lever. |
| Ubuntu selected/default | `ToyDirectMapTrusted max4` | Linux/Ubuntu default direct-map composition: direct local alloc/reuse/free, trusted local-cache owner, Toy active free-map, trusted active-map owner, and `HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4`. The selected implementation tries transfer reuse before local-cache reuse on transfer-first profiles, preserving R1 transfer semantics. Focused repeat-5 versus macro-off control is broadly positive; keep explicit off as control. |
| Ubuntu functional lane | `hz6` LD_PRELOAD allocator | Linux compare-runner integration through `hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so`. Build with `./hakozuna-hz6/linux/build_hz6_preload.sh` or include `hz6` in `run_linux_bench_compare_matrix.sh --allocators`. See `HZ6_UBUNTU_PRELOAD_LANES.md` for the full bundle, controls, no-go lanes, and raw evidence. |
| Ubuntu LD_PRELOAD default bundle | `build_hz6_preload.sh` default flags | Canonical preload lane: route65k, desc16384, source2048, frontcache4096, Toy active map32768, MidPage run768K, MidPage 32K run2048K, external MidPage active map16K/probe4 with unaligned slots allowed, mask-index, register fast-slot, MidPage alloc descriptor-out, preload-boundary MidPage malloc skip, current-bias free order, realloc in-place, mmap retain, 64K retain stack, ToyFull max128, tombstone compact, xor-fold, linear-wrap, loop-carry. |
| Ubuntu selected/default | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384` in LD_PRELOAD | MidPageActiveMapCap16K-L1. Resume diagnostics showed remaining active-map misses on 4096..16384. Focused repeat-5 held `27.067M` versus pre-promotion default `25.908M`, and cross repeat-3 held `27.752M` with about `+0.6MB` RSS. Cap32K and cap16K probe2/probe8 were weaker, so keep probe4 and promote capacity only. |
| Ubuntu closeout | `MidPageHz4Close-L1` | Previous closeout result. MidPage unaligned active-map plus probe4 moved `4096..16384` to `hz6 31.505M / 117,248 KB` versus `hz4 30.916M / 134,400 KB`; the later resume pass promotes cap16K as the current preload default. |
| Ubuntu selected/default | `HZ6_LINUX_MMAP_RETAIN_L1=1` in LD_PRELOAD | PreloadSourceRetain-L1. Linux mmap SourceLayer caches exact-size released mappings behind retained slots/bytes caps, then `reserve()` reuses them before calling `mmap`. It cuts mixed_ws syscall churn from `mmap 5060 / munmap 4315` to `mmap 2106 / munmap 6` on the 50K strace guard and raises route-131K preload repeat-3 from about 3.05M to about 7.1M ops/s. RSS rises only a few MiB in the first guard. |
| Ubuntu selected/default | `HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1` in LD_PRELOAD | PreloadSourceRetain64kStack-L1. Exact 64KiB retained mappings use an O(1) stack before the generic retained table. This targets ToyFront's 64KiB source blocks, reduces strace futex calls on the 50K guard from `3129` to `872`, and raises the SourceRetain repeat median to about `9.7M ops/s` with roughly flat RSS. |
| Ubuntu selected/default | `HZ6_TOY_FULL_BLOCK_PREFILL_L1=1`, max128 in LD_PRELOAD | ToyFullBlockPrefill-L1. On Toy source miss, the preload build now asks the source-block prefill path to consume more slots from the new 64KiB source block, capped at 128 slots. This cuts mixed_ws Toy source churn from about `9346` to about `2224` and first/live 64K mmap fallback from about `2066` to about `522`; first straight repeat-3 improved from about `10.0M..10.3M` to about `12.3M..12.6M` ops/s with similar RSS. |
| Ubuntu selected/default | `HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1` in LD_PRELOAD | LongRunDegradeAudit-L1. The 1M mixed_ws guard exposed route exact tombstone accumulation: without compact, `route_register_probe_total` jumped to about `29.4B` and maxed at the 131072 table capacity. Normal tombstone compact cuts the 500K diagnostic register probe total to about `3.44M` with max `154`, removing the long-run collapse. Aggressive compact is too costly; keep normal compact only. |
| Ubuntu control | `HZ6_FRONT_CACHE_BIN_CAPACITY=8192` in LD_PRELOAD | Former wide preload default. StaticTableTrim-L1 later promoted frontcache4096 together with route65k/desc16384/source2048, cutting about 20 MiB from selected RSS while preserving or improving selected rows. Keep 8192 as a wide-table control. |
| Ubuntu control/no-go | `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=3072/2560/2048` | FrontcacheShapeAudit-L1 class-specific cap controls. 3072 was safe but did not win; 2560/2048 increased class5 empty pops and slowed 4096..16384. Keep selected cap4096. |
| Ubuntu control/watch | `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | FrontcacheClassStorageTrim-L1. Default-off class-specific backing storage for frontcache entries. It keeps class2/class4 at 4096 and class5 at 3072 while trimming colder classes. Production-shape repeat-7 was mostly neutral/target-positive, but peak RSS barely moved and diagnostic target was weak, so keep off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | MidPageLowWaterRefill-L1. After a miss-boundary MidPage refill/pop, the control prefilled one more run if the class was below a low-water mark. Default and strong thresholds did not pass: stats-off repeat-9 left 4096..16384 flat and regressed 16..256, 16..4096, and 1024..4096. Keep off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1` | MidPage8KBorrow32-L1. Narrowed broad larger-bin borrow to MidPage 8K misses only. Broad borrow remained guard-negative; the narrowed control was guard-safe but target-flat/weak and reported `borrow_success=0` on the selected 4096..16384 stats run. Keep default off. |
| Ubuntu selected/default | `PreloadToyActiveFastFree-L1` | LD_PRELOAD free tries the allocator-local Toy active-map before exact/visible route lookup. The miss path is unchanged, so foreign pointers still fall through to normal route lookup and then real free on MISS. This fixes the wrapper-boundary issue where internal Toy active-map hits could not avoid preload-level route lookup: 16..256 diagnostic route probes dropped from about 2.12M to about 37.7K, and focused cross median moved to `hz6 53.883M` versus `mimalloc 52.656M`. |
| Ubuntu selected/default | `HZ6_MIDPAGE_RUN_BYTES=786432` in LD_PRELOAD | MidPage8KRun768-L1. After current-bias, widening 8K MidPage runs from 256K to 768K passed repeat-15: 16..256 +0.65%, 16..4096 +0.11%, 1024..4096 +1.35%, and 4096..16384 +0.71% with flat RSS. Post-promotion selected repeat-5 reached 46.496M / 94.25 MiB on 4096..16384. |
| Ubuntu selected/default | `HZ6_MIDPAGE_32K_RUN_BYTES=2097152` in LD_PRELOAD | MidPage32KRun2048-L1. After run1536, a 2M 32K run cut source churn again and improved the 4096..16384 target: focused repeat-15 moved `48.278M -> 49.789M`, with small guard costs and 1024..4096 positive. Stats repeat-3 kept fail counters 0 and cut 4096..16384 source_alloc `900 -> 723`. Keep 1536K, 768K, and 512K as direct controls. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_32K_RUN_BYTES=1792K/2304K/2560K/3072K/4096K` | MidPage32KRunFineLadder-L1. Post-run2048 repeat-7 kept 2M as the local peak; larger 2304K+ variants weakened 4096..16384 and 3072K/4096K were clearly worse. Keep as controls only. |
| Ubuntu selected/default | `HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1` in LD_PRELOAD | MidPageActiveMapMaskIndex-L1. The selected active-map capacity is 16384, so register/free probe wrapping can use a mask instead of modulo without changing behavior. Production repeat-15 moved 4096..16384 `49.443M -> 50.231M`, kept 16..4096 flat, and left RSS flat. |
| Ubuntu selected/default | `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1` in LD_PRELOAD | MidPageActiveMapRegisterFastSlot-L1. After mask-index, diagnostic probe attribution showed 4096..16384 register/free hits average about `1.15` probes with `88.3%` base-slot hits. Register fast-slot avoids the bounded loop on empty/same-pointer base-slot registrations. Production repeat-15 moved 4096..16384 `48.584M -> 50.160M`, kept 16..4096 slightly positive, and left RSS flat. |
| Ubuntu selected/default | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1` + external storage in LD_PRELOAD | Dedicated MidPage active free map. Unlike the no-go Toy-map widening, this map is separate, validates descriptor pointer/generation/class/state, and registers MidPage alloc/reuse through exact route descriptors. The selected preload shape uses external storage, capacity 16384, unaligned slots, and probe limit 4. The original cap8192/probe2 repeat-7 moved 1024..4096 from `30.962M` to `32.011M` and 4096..16384 from `18.983M` to `19.903M`; later unaligned/probe4 and cap16K passes are promoted on top. |
| Ubuntu selected/default | `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1` | MidPage alloc returns the activated descriptor to active-map registration, avoiding the post-alloc exact route lookup without changing prefill policy. Confirmation guards keep 16..256, 16..4096, and 1024..4096 flat/positive and move 4096..16384 from about `29.8M` control-off to about `34.8M`, so it is selected for LD_PRELOAD only. |
| Ubuntu selected/default | `HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1` + `HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=1` | Preload-boundary MidPage malloc skip avoids empty transfer-first probes on the MidPage direct-local path without changing core `hz6_malloc()`. The selected shape uses an unlikely size guard and noinline helper. Boundary-off confirmation repeat-15 moved 4096..16384 `33.735M -> 40.056M` and kept 16..256, 16..4096, and 1024..4096 positive. |
| Ubuntu selected/default | `HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | Post-raw-pop Toy active-map free lookup uses a base-slot-first fast path before the bounded loop. Production repeat-15 improved 16..256, 16..4096, and 4096..16384 with flat RSS; stats-on safety kept route/fail counters clean and reduced a target route-after-maps sample from `1396` to `1171`. |
| Ubuntu selected/default | `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1` | Balanced active-count bias for LD_PRELOAD free ordering. When MidPage active entries exceed Toy active entries, `free()` probes MidPage before Toy; otherwise it preserves Toy-first order. Promotion repeat-15 moved 4096..16384 `44.163M -> 45.596M`, kept 16..4096 and 1024..4096 flat/slightly positive, and limited 16..256 to -0.45% with flat RSS. |
| Ubuntu selected/default | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1` | LD_PRELOAD realloc returns the same pointer when requested size fits the current HZ6 usable descriptor bytes. This removes malloc/copy/free round trips for same-class/shrink reallocs. Repeat-5 versus control-off improved all focused rows, including 4096..16384 `19.971M -> 30.118M`. Latest selected cross repeat-3 has HZ6 ahead of mimalloc on all focused rows and slightly ahead of tcmalloc on 4096..16384 speed/RSS; tcmalloc remains much stronger on tiny/mid rows and system remains faster on tiny 16..256. |
| Ubuntu diagnostic-only | `HZ6_PRELOAD_STATS=1` | Prints aggregate `[HZ6_PRELOAD_STATS]`, `[HZ6_PRELOAD_ROUTE_DETAIL]`, `[HZ6_PRELOAD_FRONT_DETAIL]`, `[HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL]`, `[HZ6_PRELOAD_PHASE_STATS]`, `[HZ6_PRELOAD_HOOK_DETAIL]`, `[HZ6_PRELOAD_WRAPPER_DETAIL]`, and `[HZ6_PRELOAD_WRAPPER_SIZE_DETAIL]` lines at process unload across registered thread-local preload allocators. Use to attribute source/route/fail/retain/frontcache/hook pressure plus calloc/realloc/usable-size/aligned-allocation wrapper pressure; do not use for speed ranking. |
| Ubuntu selected API behavior | LD_PRELOAD `malloc_trim(0)` | Explicit quiescent release API. The hook scavenges HZ6 local-free descriptors, flushes Linux mmap retained mappings, then forwards to real libc `malloc_trim` when available. It adds no malloc/free hot-path work. No-stats raw `hz6_midpage_payload_trim_ab_20260615_222345` drops current RSS to the `27..28 MiB` floor on focused/fixed rows while peak RSS remains flat. |
| Ubuntu profile/control | `HZ6_PRELOAD_REAL_ALIGNED_FREE_SKIP_L1=1` | RealAlignedFreeSkip-L1. Records successful real `posix_memalign` / `aligned_alloc` fallback pointers so `free()` can skip HZ6 route lookup and call real free directly. Aligned audit raw `hz6_preload_aligned_wrapper_audit_20260615_224612` moves aligned rows from about `14K` to `7.2M..8.1M ops/s` and removes `free_route_miss_real`. Keep off by default because mixed/fixed guard raw `hz6_midpage_payload_trim_ab_20260615_224657` regressed `fixed_8k` (`42.633M -> 39.876M`). |
| Ubuntu control/no-go | `HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Conservative Toy active-map negative envelope. It can skip impossible Toy probes in principle, but the first repeat-3 was not selected-clean: tiny improved slightly while 1024..4096 and 4096..16384 weakened. Keep off. |
| Ubuntu control/no-go | `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | Preload free tries the MidPage active map before the Toy active map. It improves the MidPage-heavy 4096..16384 row by removing the Toy-miss wall, but badly regresses 16..256, 16..4096, and 1024..4096. Keep off; use only as evidence for future class-aware/free-hint gating. |
| Ubuntu control/no-go | `HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1=1` | Cheap selective MidPage-first preload free gate using MidPage alignment. It behaved like unconditional MidPage-first on Toy-heavy rows because nearly every free pointer passed the alignment gate, so guard regressions were large. Keep off. |
| Ubuntu control-off | `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=0` | Direct control for the selected balanced current-bias preload free order. |
| Ubuntu diagnostic-only/no-go direction | `HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1=1` | Dry-run for a selective MidPage-first preload free gate. The recent min/max envelope covered the MidPage target but produced huge false positives on 16..4096 and 1024..4096, so do not behaviorize this shape. A tighter range/page hint table is needed. |
| Ubuntu diagnostic-only | `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | Dry-run for a tighter selective MidPage-first preload free gate. Successful preload MidPage malloc boundary hits register base/last 4K pages in a small TLS hint table; free() probes by page and reuses the existing `mh_*` hook-detail counters. Capacity32768 proved the classifier is much cleaner than the broad envelope, but the behavior A/B showed the per-free probe cost is too high. Keep as evidence only. |
| Ubuntu control/no-go | `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | Behavior A/B for the page-hint gate. Hinted frees try MidPage active-map before Toy active-map; unhinted frees keep selected Toy-first order. It reduced target Toy probes but regressed all short repeat-5 rows, including 4096..16384 `26.146M -> 25.194M`. Keep off; the all-free hint probe/table overhead is too expensive for this shape. |
| Ubuntu diagnostic/design | `HZ6_PAGE_KIND_FREE_SELECTOR_DRYRUN_L1=1` | Allocator-local page-kind selector populated by active-map registration. It is advisory only: preload free probes it for attribution, then keeps the selected free order and normal descriptor validation. Raw `hz6_page_kind_selector_dryrun_20260616_000420` showed zero wrong Toy/MidPage hit classifications across focused+fixed rows, making it a behavior A/B candidate; dry-run lookup/counter overhead itself is not selected. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | MidPage active-map free lookup tests the base hash slot before the bounded probe loop. After register fast-slot promotion, repeat-15 was target-flat (`50.375M -> 50.408M`) and guard-weak on 16..256 and 1024..4096. Keep off. |
| Ubuntu profile/control DSO | `build_hz6_preload_toy_target.sh` / `run_hz6_preload_toy_target_ab.sh` | Named Toy/mid-small DSO. It builds selected preload plus `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_L1=1,max4096` and `HZ6_PRELOAD_TOY_MALLOC_DIRECT_CLASS_FAST_REUSE_L1=1`, preserving selected default while exposing the mid-small win for workloads that do not need MidPage/fixed guard balance. Fast-reuse DSO raw `hz6_toy_target_fast_reuse_preload_ab_20260616_004700` improved 16..256 by `+30.39%`, 16..4096 by `+15.27%`, 1024..4096 by `+14.90%`, and fixed_4k by `+12.77%`, but still regressed 4096..16384 and fixed_8k slightly. |
| Ubuntu control/no-go | `HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_DRYRUN_L1` / `HZ6_PRELOAD_SOURCE_RUN_ROUTE_AFTER_MAPS_L1` | Source-run route after active-map misses. Raw `hz6_midpage_payload_trim_ab_20260615_221341` showed zero run-route hits and large range-index RSS cost, so keep as evidence only. |
| Ubuntu diagnostic-only | `HZ6_PRELOAD_STATS=per_allocator` | Extends preload stats with one `[HZ6_PRELOAD_ALLOCATOR_STATS]` line per registered thread-local allocator. The first topology audit showed the main allocator is negligible (`source_alloc=4`) and four worker allocators split source churn evenly, so do not chase loader/main isolation next. |
| Ubuntu diagnostic-only | `[HZ6_PRELOAD_PHASE_STATS]` | PreloadPhaseAudit-L1. Emitted when `HZ6_PRELOAD_STATS` is enabled. Splits wrapper calls and ownership decisions across malloc/calloc/free/realloc/malloc_usable_size, including local route-valid frees, visible hits, real fallbacks, and prechecked-free candidates. Use this before changing preload free/realloc behavior or Toy source prefill. |
| Ubuntu diagnostic-only | `[HZ6_PRELOAD_MIDPAGE_CLASS_DETAIL]` | MidPageSourcePressureAudit-L1. Splits MidPage 8K/32K alloc, source-run prefill, active-map register/free hits, and local route fallback. Active-map miss stays aggregate because class is unknown before route fallback. Use this before touching MidPage source-run sizing, prefill policy, or route-register behavior. |
| Ubuntu diagnostic-only | MidPage register callsite counters | Split active-map registration into direct reuse, front alloc, and route fallback. The descriptor-out selected target row has route fallback at zero, so the next target is register/free code shape rather than exact-route fallback removal. |
| Ubuntu diagnostic-only | MidPage free-cache counters | Split MidPage active-map free cache attempt/success/fail and expose 8K/32K frontcache push/pop-empty counts. The first read showed cache failures are zero, so behavior work must target success-path shape, not recovery. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_32K_RUN_BYTES=128K/192K/224K` | Payload-trim controls. They did not reduce peak RSS materially, increased source allocation on 4096..16384, and slowed the target, so keep them as no-go evidence. |
| Ubuntu control-off/no-go | `HZ6_MIDPAGE_RUN_BYTES=256K/384K/512K/640K` | 8K source-run controls below selected run768. 512K remained target-positive but guard-negative; keep run768 selected. |
| Ubuntu control-off | `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=0` | Direct control for the selected LD_PRELOAD MidPage descriptor-out malloc path. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1=1` | Class-salted MidPage active-map index. It did not reduce diagnostic collision/miss pressure and regressed a guard row in the first A/B, so keep off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1=1` | MidPage prefill/direct-pop retry to avoid post-prefill exact route lookup. It severely regressed 4096..16384 and raised RSS, so keep off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | MidPage front cache-only retry after successful prefill_run. It avoids a second transfer-first probe, but repeat-15 regressed 16..256 by -7.11% and 4096..16384 by -2.08%, so keep off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Conservative MidPage active-map min/max negative filter. It helps Toy/tiny by skipping impossible MidPage probes, but target 4096..16384 does not hit the envelope skip path and is not improved. Keep off by default. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` | Same-class active-map overwrite victim selection. Dry-run found alternate victims, but behavior regressed target and 1024..4096 guards, so keep off. |
| Ubuntu control/no-go | `HZ6_LINUX_MMAP_RETAIN_TLS_L1=1` in LD_PRELOAD | TLS retained front over SourceRetain. It did not reduce mmap count and regressed repeat-3 to about `6.6M..7.0M`; keep default off. |
| Ubuntu control/no-go | `HZ6_SOURCE_RUN_REUSE_L1=1` in LD_PRELOAD | Reduces Toy source allocation count strongly, but reusable-run scan/activation cost dominates mixed_ws and drops repeat-3 to about `1.6M..2.1M`; keep default off for preload. |
| Ubuntu control/no-go | `HZ6_TOY_SOURCE_BLOCK_BYTES=128K/256K` in LD_PRELOAD | Larger Toy source blocks with matching `HZ6_SOURCE_RUN_MAX_SLOTS` were tested after retain counters. They did not reduce source_alloc, raised RSS, and regressed mixed_ws throughput; keep the default 64KiB Toy source block. |
| Ubuntu control/no-go | `HZ6_ROUTE_PACKED_META_L1=1` in LD_PRELOAD | Route-register probe-loop footprint candidate. With route table 131072 it lowers mixed_ws RSS by a few MiB, but repeat-3 is slower and less stable than the unpacked 131072 default, so keep it as control evidence only. |
| Ubuntu control-off | `HZ6_PRELOAD_FAST_FREE_L1=0` | Default for the LD_PRELOAD lane. `HZ6_PRELOAD_FAST_FREE_L1=1` reuses preload's prechecked route in HZ6 free dispatch, but repeat-3 did not improve mixed_ws and perf stayed dominated by `hz6_route_register_exact`, so keep the shortcut as candidate/control only. |
| Ubuntu control/no-go | `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | MidPage-only version of preload fast-free. It removes a second route path for local MidPage frees and gave a small 4096..16384 bump in short repeat, but code-shape/tiny guard cost regressed 16..256, so keep default off. |
| Ubuntu control/no-go | `HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1=1` | Re-arms the MidPage active map from preload's local route-valid result before calling `hz6_free()`. Diagnostic counters moved in the intended direction, but throughput was flat/small; keep as evidence-only. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1=1` | Alternative 4K-granularity MidPage active-map index. It improved some non-target guard rows but regressed the target 4096..16384 row, so the selected default remains the existing 8K-shift index. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1=1` | Collision policy that keeps existing entries when the bounded probe range is full. It regressed the target MidPage row before and after descriptor-out, so keep current overwrite-on-full behavior. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1` | Current-bias-gated free fast-slot tried to apply base-slot-first lookup only when MidPage active entries exceed Toy active entries. The added branch was not free: repeat-7 weakened tiny and target rows. Keep off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` | Trusted local-free MidPage activation source-block-check skip. It is a plausible direct-reuse shortcut, but focused repeat-5 did not improve the target or tiny guard, so keep default off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | Direct trusted cache push from MidPage active-map free hit. It bypasses the generic cache helper, but the correct focused repeat was flat/negative on target and tiny guard, so keep default off. |
| Ubuntu control/no-go | `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | MidPage direct-local reuse skips transfer-first before the local frontcache pop. It is strongly target-positive, but the tested helper shape regressed non-MidPage guard rows; keep off until guard isolation is solved. |
| Ubuntu control/no-go | MidPage noinline transfer-skip helper | Guard-isolated helper attempts preserved the target win but still regressed small/non-target rows beyond the promotion gate. Keep as target DSO/control, not selected default. |
| Ubuntu no-go/control | MidPage preclassified malloc shape | Direct 4097..32768 classification can improve the target row, but the tested malloc code-shape change regressed the tiny guard too much. Do not promote without a design that isolates small rows. |
| Ubuntu control-off | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=0` | Direct control for the selected preload realloc in-place shortcut. Keep for A/B if future realloc semantics or size-class behavior changes. |
| Ubuntu no-go/control | `ToyDirectMapTrusted max3` | Boundary attempt to avoid 16K fallout by limiting direct local cache to class <=3. It loses 4K/8K and still does not create a clean 16K guard. |
| Ubuntu no-go/control | `HZ6_LARGE_SPAN_TRUSTED_LOCAL_FREE_L1=1` | LargeSpan local-free owner recheck bypass after `hz6_free()` proves local ownership. Diagnostic confirms the owner_equal call can be removed, but focused repeat-5 after LastHitRouteCache-L1 is mixed and regresses key local/reuse rows. Keep default off. |
| Redis candidate-control | `redislowrss-sourcerun-desc8k-route8k` | Current Redis-like low-RSS candidate-control. Use for Redis long/paper-style rows when the goal is completion and low RSS, not broad HZ6 promotion. |
| Redis behavior evidence | `redislowrss-sourcerun-desc8k-route8k-tombcompact` | Conservative RouteTombstoneCompact-L1 Redis route-churn evidence. Helps RANDOM/LPUSH in some rows, but does not cleanly win SET/GET/LPOP. |
| Redis boundary controls | `redislowrss-sourcerun-desc8k-route8k-tombcompact-aggr1024` / `...aggr2048` | Aggressive tombstone threshold probes. They prove the half-cap threshold can suppress compaction on 8K route tables, but first refreshed Redis long run did not produce a clean winner and RSS rises. Keep as controls, not defaults. |
| Redis behavior evidence | `redislowrss-sourcerun-desc8k-route8k-condtombdry` / `...condtombcompact` | Conditional tombstone compact witness. Dry-run cleanly projected LPUSH/RANDOM only; behavior compacts only LPUSH/RANDOM and keeps safety clean, but repeat-3 still loses row geomean to `aggr1024` and loses GET/LPOP enough to block selection. |
| Redis no-go/control | `redislowrss-sourcerun-desc8k-route8k-retainedoverflow` / `...slotlookup` | RetainedOverflow and SlotLookup are useful Redis mechanism witnesses. Neither replaces the candidate-control today; do not compose them into selected lanes without fresh evidence. |
| Candidate-watch/control | `sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k` | Mechanism evidence for the small/mid same-owner fixed-size win. Positive across 256B..16K, but weaker as a single simple lane than DirectLocalFreeReuse in the latest repeat-10. Keep as a close control, not broad/default. |
| Boundary / no-go control | `directlocalsmall8k-sameownerlarge-largerlowrss-front8k-sourcerun-desc8k-route8k` | Class-gated hybrid: DirectLocalFreeReuse for class <= 4 and SameOwnerFast for class >= 5. Run-1/focused refresh is safety-clean and gives a small 2K witness, but loses the 8K shape versus `directlocalfreereuse-small8k` and does not fix 16K. Keep as selected-small boundary evidence; do not promote or add another static size hybrid without fresh hot-path attribution. |
| Selected-small candidate-watch | `sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | Current SourceBlockRoute dynmap selected-small evidence row. Earlier repeat-3 was broadly positive, but the 2026-06-06 follow-up showed the shape is workload-sensitive: 8K/16K can improve while 4K/balanced/larger_sizes can wobble. Keep as candidate-watch/evidence; do not broad-default without a fresh repeat-5/10 selected-family guard. |
| Candidate-watch/control | `directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | Former selected-small simple lane. HZ6-only repeat-10 was positive across 256B..16K versus LargerLowRSS; keep as the simple baseline/control for SourceBlockRoute dynmap. |
| Diagnostic-only | `toysmallhotpathdiag-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | Selected-small candidate plus `HZ6_TOY_SMALL_HOTPATH_DIAG_L1`. Use only with `-DiagnosticHz6Probes` to attribute Toy/small malloc/free hot-path work; not speed-rankable and not selected-family wiring. |
| Diagnostic-only | `toysmallhotpathdiag-sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | Same Toy/small hot-path attribution over the SourceBlockRoute dynmap candidate. Added to verify whether dynmap leaves Toy/small route work after the directlocal diagnostic. Use only with `-DiagnosticHz6Probes`. |
| Candidate-watch/control | `sourceblockroute-behavior-dynmap-notoy-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | SourceBlockRoute dynmap with Toy/small excluded from late range-index registration. Safety-clean and useful to prove Toy fallback isolation, but the 2026-06-07 A/B did not beat DirectLocalFreeReuse cleanly. Keep as control evidence, not selected wiring. |
| Candidate-watch/control | `toysmallactivemap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | Toy/small active free-map over DirectLocalFreeReuse. It keeps a bounded per-allocator active pointer map so same-owner active frees can bypass exact-route lookup and fall back to route lookup on miss/stale/cache-fail. The current-gated version avoids probing the map when it is empty, so non-Toy rows do not pay full map lookup cost. Latest repeat-3 is safety-clean and improves 256B/512B/1K/2K/4K/8K, but 16K regresses and the lane adds hot-path state, so keep as control evidence rather than default. |
| Candidate-watch/control | `toysmallactivemap-sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | Toy/small active free-map layered over the current SourceBlockRoute dynmap selected-small lane. Repeat-10 is safety-clean but regresses 512B and 16K enough to miss promotion: keep as control evidence that same-owner active-map bypass can help some Toy/small rows, not as selected wiring. |
| Candidate-watch/control | `directlocalfreereuse-small8k-largerlowrss-front8k-sourcerun-desc8k-route8k` | Size-gated DirectLocalFreeReuse control. It sets `HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4`, so 256B..8K can use direct local free/alloc/reuse while 16K/32K fall back. Useful evidence, not a universal winner. |
| Candidate-watch/control | `sourceblockroute-behavior-dynmap-small8k-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | SourceBlockRoute dynmap with late range-index registration and `HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS=4`. Safety-clean repeat-3 proved class-gated late registration works, but the 2026-06-07 boundary refresh lost to dynmap/direct on 4K/8K/16K. Keep as control/no-go evidence, not a selected-small rescue gate. |
| Candidate-watch/control | `smallrunroute-behavior-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` / `smallrunroute-behavior-min512-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | TinyRun/SmallRun route evidence. Safety-clean and proves source-run slot route reconstruction can replace exact-route lookup on Toy/small slots. The latest repeat-5 does not beat the selected dynmap row broadly; min512 mostly helps as a 1K/4K clue and pays lower-gate fallback cost on 256B. Keep as evidence/control, not selected/default. |
| Candidate-watch/control | `smallrunroute-behavior-range64k-toyonly-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | SmallRunRoute with 64 KiB range-index granularity and Toy-only late range registration. It cuts 2K range-index probe pressure (`1,174,976/max129 -> 342,992/max2` in diagnostic) and wins 256B/512B/1K in the focused repeat-3, but 2K/4K/8K/16K still do not form a broad selected-small replacement. Keep as Toy-low control/evidence. |
| Candidate-watch/control | `smallrunroute-behavior-range64k-toyarmed-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | SmallRunRoute range64k/toyonly plus an allocator-local armed count. It skips the range-index lookup until at least one eligible Toy source-run range is registered, avoiding the empty-table probe on pure non-Toy rows without a second prefilter table. Focused repeat-3 improves 256B/512B/4K/8K versus toyonly, but 16K regresses, so keep as candidate/control only. |
| Candidate-watch/control | `smallrunroute-behavior-range64k-toyarmed-slotmax1k-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k` | SmallRunRoute range64k/toyarmed with `HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES=1024`. It intentionally registers only Toy 256B/512B/1K source-runs and leaves 2K/4K/8K/16K on the DirectLocalFreeReuse/exact-route fallback. The post-route-closeout repeat-5 makes it a 4K clue/control, not a selected replacement: winners split across dynmap/direct/sameownerfast/slotmax1k. |
| Boundary / no-go control | `directlocaltrusted-largerlowrss-front8k-sourcerun-desc8k-route8k` | Trusted local-cache owner shortcut over DirectLocalFreeReuse. Safety-clean smoke, but loses to DirectLocalFreeReuse on 512B/2K/8K and only wins 16K. Do not select or legacy-wire. |
| Boundary / no-go control | `directlocalpacked-largerlowrss-front8k-sourcerun-desc8k-route8k` | FrontCachePackedMeta-L1 over DirectLocalFreeReuse. Safety-clean smoke, but loses to DirectLocalFreeReuse on 512B/2K/8K/16K. Keep FrontCachePacked for Larson RSS rows, not selected-small speed. |
| Candidate-watch/control | `directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k` | DirectLocalFreeReuse plus exact-first free route lookup. Repeat-10 ties DirectLocalFreeReuse on average and slightly improves min delta, but loses 1K/2K/8K. Keep as route-pressure control; do not select yet. |
| LargeSpan family seed | `mixed_ws large_slice_128k,large_slice_256k,large_slice_512k,large_slice_1m + speed-route4k` | Narrow 128K..1M LargeSpan class verification. Use for LargeSpan backend safety/coverage, not broad speed ranking. |
| LargeDirect coverage seed | `mixed_ws large_direct_slice_2m,large_direct_slice_4m,large_direct_slice_8m + speed-route4k` | Narrow >1M..8M direct-release verification. Use for coverage/low-RSS safety evidence, not throughput ranking. |
| Candidate-control | `largedirectretain16m-largerlowrss-front8k-sourcerun-desc8k-route8k` / legacy `hz6-*-largedirectretain16m-largerlowrss` | Practical LargeDirectRetain cap after the repeat-3 cap ladder and single-run cross-allocator slice. 16M removes source churn for 2M/4M/8M, keeps 512K/1M guard source counts unchanged, and wins 512K/2M/8M in the follow-up slice while staying low RSS. |
| Candidate-watch/control | `largedirectretain32m-largerlowrss-front8k-sourcerun-desc8k-route8k` / legacy `hz6-*-largedirectretain32m-largerlowrss` | Direct-large retained reuse for >1MiB objects. Repeat-3 turns 2M/4M/8M from direct OS source churn into practical retained reuse (`source_alloc` drops to `16/12/8`) while safety stays clean. Legacy runner connection is wired and reproduces the direct-large win. Keep as a >1MiB LargeDirect control, not a broad LargeSpan/default selected lane until cross-allocator `large_slices` guard confirms 512K/1M stability. |
| Preset/control | `win/run_win_hz6_selected_family.ps1 -LargeDirectRetainControl` | Convenience preset for the LargeDirectRetain cap ladder. It compares LargerLowRSS base, 8M default retain, 16M, and 32M on 2M/4M/8M direct slices and includes 512K/1M LargeSpan guards. This preset is intentionally outside `-SelectedFamily`. |
| Selected Larson low-RSS sibling | `...frontcachepacked-sourceblockpacked-source10k-route192k-run512` | Current Larson minimum-RSS sibling. Use for current HZ6 Larson low-RSS comparisons. |
| ElasticCapacity candidate-watch | `...elasticdescroute-desc16k-...source10k-route16k-run512` | Best ElasticCapacity RSS/throughput shape so far. Watch, but do not broad-promote. |
| ElasticCapacity component/control | `...elasticdescsource-route-desc16k-...source64-route16k-run512` | Lower RSS source-block depot evidence; speed is lower than ElasticDescriptorRouteOverflow. |
| Diagnostic-only | `...localizedryrun...`, `...runlocalitydryrun...`, `...depotrunmeta...`, `...slotownerdryrun...`, `...slotownersparse...`, `...slotownerconsumerdryrun...`, `...ownerequalcallsite-dryrun...`, `...freelocalownerpredicate-dryrun...`, `[HZ6_ELASTIC_PROJECTION]`, `[HZ6_MAIN_WARMUP_CAPACITY]`, `[HZ6_ELASTIC_OVERFLOW_PROJECTION]` | Never use as production speed-ranking rows. |
| Selected Larson Elastic low-RSS sibling | `...depotownerdirect-directfree-trustedlocalcache...` | Best source-depot speed/RSS shape so far. It composes DepotOwnerDirect with direct local-cache free and trusted-owner local-cache push, avoiding the second owner check after `free()` already proved local ownership. Repeat-3 improves every main/worker 1k/4k/10k guard row over DepotOwnerDirect (`avg +2.60%`, min `+1.34%`) with essentially unchanged RSS. Selected as a Larson/Elastic low-RSS sibling, not a broad HZ6 default. |
| Candidate-watch/control | `...depotownerdirect...` | Previous source-depot selected sibling. Keep as the clean baseline/control for DirectFreeTrustedLocalCache; it remains the simplest depot-owner direct shape. |
| Boundary / no-go control | `...depotownerdirect-trustedlocalcache...` | TrustedLocalCache without DirectFree mostly affects local-cache activation, not the intended free-side owner check. Repeat-3 is safety-clean but mixed and does not beat DepotOwnerDirect broadly. |
| Boundary / no-go | `...slotownerlogical...` | Safety-clean behavior no-go/control. Sparse generation-checked positive owner match is valid, but probing sparse metadata at every `owner_equal()` entry costs more than it saves. |
| Boundary / no-go | `source2k`, `source8k`, `elasticproj-local1k`, whole-SourceBlock localize | Keep as evidence only. These rows explain capacity or ownership limits. |

## Selected-Small Wiring Policy

The current selected-small candidate is fixed as:

```text
speed + sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
```

Use it in `win/run_win_hz6_selected_family.ps1 -SelectedFamily` and in the
legacy cross-allocator matrix when a selected-small HZ6 row is needed. This is
the selected-small candidate row, not a broad/default allocator profile.
Keep the
following rows out of selected-family and legacy cross-allocator tables unless a
later repeat explicitly promotes them:

```text
directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
directlocaltrusted-largerlowrss-front8k-sourcerun-desc8k-route8k
directlocalpacked-largerlowrss-front8k-sourcerun-desc8k-route8k
directlocalexact-largerlowrss-front8k-sourcerun-desc8k-route8k
```

These rows are still valuable HZ6-only controls. They answer whether owner
checks, packed front-cache metadata, or exact-first free routing explain the
remaining small fixed-size gap. So far they do not cleanly replace
DirectLocalFreeReuse, and they should stay in the capacity matrix rather than
paper-facing cross-allocator rows.

Small-object design note:

```text
The SourceBlockRoute / active-map / notoy small-knob track is closed for now.
ToyFront remains the route-safe reference front.  SmallRunRoute-L1 now exists
as the separate TinyRunRoute evidence lane requested by this note. It is
safety-clean, but the current signal is narrow and does not replace the
selected dynmap row. Do not promote another SourceBlockRoute, Toy-active-map,
or SmallRunRoute toggle into selected-small without a fresh repeat matrix and a
clear multi-row win.

The slotmax1k SmallRunRoute lane is the current narrow follow-up. It is not a
free-time size selector: it only decides which source-runs register into the
range index. HZ6 maps 2K and 4K Toy requests to the same 4K slot class, and
MidPage 8K also uses class 4, so this lane deliberately tests only the clean
256B/512B/1K signal instead of pretending a static class gate can isolate 2K.
```

MidPage/Toy source placement note:

```text
MidPage 8K/32K prefill now uses the same shared source-block helper as Toy:
  hz6_midpage_prefill_run()
    -> hz6_front_prefill_source_block_kind()

This is a structural cleanup for the small/mid transition. It does not promote
a new speed lane by itself; use repeat matrices before changing selected-family
rows.
```

Fixed confirmation:

```text
hz6-midpage-sourceblock-unified-repeat3:
  8K LargerLowRSS:             55.476M / 25,984 KB
  8K DirectLocalFreeReuse:     63.667M / 25,920 KB
  16K LargerLowRSS:            50.723M / 17,648 KB
  16K DirectLocalFreeReuse:    52.147M / 17,648 KB

  safety:
    route_invalid=0
    route_miss=0
    alloc_fail=0
    route_register_fail=0
    source-run rollback/safety counters=0
```

## Current Fixed Read

```text
HZ6 selected-small / small-mid:
  FIXED:
    sourceblockroute-behavior-dynmap-directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
      as selected-small candidate row
    directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k
      as simple baseline/control
    MidPage source-block prefill unified through
      hz6_front_prefill_source_block_kind()

  DO NOT PROMOTE:
    trusted/packed/exact DirectLocal variants
    directlocalfreereuse-small8k as a default
    per-size hybrid rows

  NEXT:
    run repeat-5/10 and cross-allocator selected-small refresh before using
    SourceBlockRoute dynmap as a paper-facing broad claim.  For further
    256B..2K work, inspect Toy/small hot-path attribution first; do not add
    another direct-local micro-knob without a new pressure signal.

HZ6 LargeSpan / LargeDirect:
  FIXED:
    LargeSpan class table with 128K, 256K, 512K, and 1M classes
    bytes-aware CentralSpanPool budget
    LargeDirect direct-release coverage for >1M..8M

  CURRENT READ:
    large_slice_128k, large_slice_256k, large_slice_512k, and large_slice_1m
    are clean under speed-route4k after ForceBuild. Treat this as backend
    coverage/safety evidence, not a broad allocator ranking row.
    large_direct_slice_2m, large_direct_slice_4m, and large_direct_slice_8m
    are also clean, but intentionally slow because they do OS direct
    alloc/release instead of retained reuse.
    `win/run_win_allocator_matrix.ps1 -Profiles large_slices` now includes
    these 256K/512K/1M and direct 2M/4M/8M rows for cross-allocator comparison.
    For a quick HZ6-only connection check, pass `-Allocators hz6-speed-route4k`
    instead of running the full allocator set. Use `-BenchTimeoutSeconds` for
    exploratory large rows so a stuck row cannot keep spawning children. Use
    `-ForceBuild` after HZ6 source changes; the legacy matrix otherwise reuses
    existing `out_win_suite` artifacts when they are present.
    The 2026-06-06 selected cross-allocator `large_slices` run is archived under
    `docs/benchmarks/windows/paper/hz6_legacy_large_slices_selected_20260606/`
    and summarized in `HZ6_CROSS_ALLOCATOR_COMPARISON.md`.
    `hz6-*-largerlowrss` is now wired into the legacy allocator matrix as the
    selected 4K..16K/LargerSizes HZ6 row. Use it when checking whether the
    4K/8K/16K fixed-size gap is a real algorithmic gap or just a route4k
    low-capacity control artifact.
    `sameownerfast-largerlowrss-front8k-sourcerun-desc8k-route8k` is a
    candidate-watch follow-up for 256B..16K same-owner fixed-size rows.
    Repeat-10 is positive across the set:
    +23.2% 256B, +19.5% 512B, +20.0% 1K, +12.9% 2K, +5.2% 4K,
    +13.0% 8K, and +8.6% 16K, with flat RSS. 2K is noisy and 4K/16K are
    smaller wins, so do not default-promote yet. Decomposition shows the win
    is not free-side only: `directlocalfreereuse-largerlowrss-front8k-...`
    is the stronger controlled decomposition row in most rows, while
    SameOwnerFast remains a close mechanism/control lane. Full 256B..16K
    repeat-5 best rows are DirectLocalFreeReuse for 256B/1K/2K/4K/16K and
    SameOwnerFast for 512B/8K. Legacy matrix connectivity is also verified,
    but the single-run legacy read regresses at 16K for DirectLocalFreeReuse
    while SameOwnerFast stays positive. The current read is local alloc/reuse
    activation cost, not source capacity; promotion needs a size gate or more
    repeat evidence. Latest HZ6-only repeat-10 makes full DirectLocalFreeReuse
    the best simple candidate: avg +19.8%, min +14.2% across 256B..16K.
    It is not the best row at every size, but it avoids the overfit risk of a
    per-size hybrid. A size-gated control,
    `directlocalfreereuse-small8k-largerlowrss-front8k-...`, is now wired. It
    uses `HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS=4` to allow Toy/class-4 paths up to
    8K while excluding 16K/32K MidPage class 5. Repeat-5 and legacy single-run
    show it is useful, especially around 512B/1K/8K, but it is not a universal
    winner. Do not build an overfit per-size hybrid until repeat evidence gives
    a clean rule.

  NEXT:
    stop large/direct coverage expansion for now.
    LargeDirectRetain16M already provides the practical >1MiB retained-reuse
    control, and 32M remains the upper-bound row. Do not add another
    direct-large recycle/cap lane unless a new >8M workload becomes the target.

  Ubuntu hot-path tuning:
    Front registry dispatch is now a direct branch path rather than a repeated
    front-array scan, large-span central push uses the compile-time caps
    directly, and Large128 local free avoids re-decoding the entry class id
    just before frontcache push. This is an implementation-speed improvement
    only; the selected HZ6 lane map above stays unchanged.

HZ6 Larson / ElasticCapacity:
  KEEP:
    selected source10k packed lane as the current minimum-RSS sibling
    ElasticDescriptorRouteOverflow-L1 as candidate-watch
    ElasticDescriptorSourceRouteOverflow-L1 as lower-RSS source-depot evidence

  STOP:
    whole-SourceBlock localize behavior
    another isolated route-only / descriptor-only / source-only cap knob
    broad slot-local storage-owner override

  NEXT if continuing this track:
    DepotSlotTransferScoped-L1 proved sparse slot recording is safe when it
    stays scoped to transfer reuse.
    DepotDescriptorRehomeDryRun-L1 shows most transfer-reused depot
    descriptors are run-matched and local descriptor capacity is available.
    First add a RouteExactDescriptorReplace / route-replace dry-run to prove
    old route descriptor/generation/class match and safe current-route commit.
    Only after that should DepotDescriptorRehome-L1 clone/rehome descriptors at
    transfer reuse. Preserve generation during rehome; do not revive general
    owner_source storage override.

  DO NOT MIX:
    diagnostic counters or dry-run lanes into production speed tables
```

## Larson / ElasticCapacity Lane Status

| Role | Lane / diagnostic | Status |
| --- | --- | --- |
| Current Larson minimum-RSS sibling | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512` | Selected HZ6 Larson low-RSS sibling. Full10k repeat-3: `44.864M / 412280 KB`, safety clean. Guard rows worker10k, main/worker4k, and main/worker1k are clean. Not a broad promotion/default lane. |
| Source-cap backup | `...source12k-route192k-run512` | Backup/control if source10k shows repeat variance or broader cap pressure. Run-1: `43.910M / 417332 KB`, safety clean. |
| Source-cap no-go controls | `...source8k...`, `...source2k...` | Warmup SourceBlock exhaustion. These prove the cross-owner main-warmup source-block transient is above 8k even though the final worker steady state is tiny. |
| ElasticProjection-L1 | `[HZ6_ELASTIC_PROJECTION]` diagnostic | Fixed diagnostic evidence. It shows large static-table upside but also that final local high-water alone is not enough for main-warmup sizing. |
| ElasticHighWater-L1 | `descriptor_live_max`, `source_block_active_max`, `frontcache_total_max` diagnostics | Fixed diagnostic evidence. In the selected source10k lane, final worker high-water is modest: descriptor `400`, source block `25`, frontcache `401`, route occupied `5425`. |
| MainWarmupCapacity-L1 | `[HZ6_MAIN_WARMUP_CAPACITY]` diagnostic | Fixed diagnostic evidence. Main-warmup temporarily owns descriptor `160048`, route `170051`, source block `10003`, frontcache `48`; after worker handoff the steady-state worker maxima are tiny. |
| ElasticOverflowProjection-L0 | `[HZ6_ELASTIC_OVERFLOW_PROJECTION]` diagnostic | Fixed diagnostic evidence. Full10k with final high-water local caps projects overflow descriptor `159024`, route `153667`, source block `9939`, frontcache `0`, transfer `0`. This confirms descriptor/route/source must move together. |
| ElasticRouteOverflow-L1 | `...elasticroute-desc160k-front4k-...source10k-route16k-run512` | RSS candidate-control, not promotion. It shrinks local route to `16k` and uses shared exact/envelope route overflow. Smoke is clean; full10k run-1: `41.723M / 335132 KB`, safety clean. This proves route overflow can work and cuts RSS, but speed is below source10k control, so descriptor/source overflow must be designed next. |
| ElasticDescriptorRouteOverflow-L1 | `...elasticdescroute-desc16k-front4k-...source10k-route16k-run512` | Strong RSS candidate-watch after the depot storage fast path. It shrinks local descriptor and route to `16k`, uses a shared descriptor depot and shared route exact/envelope overflow. Before storagefast it was `33.184M / 246824 KB`; after depot descriptors resolve storage through SourceBlock side metadata, diagnostic full10k is `40.951M / 246912 KB`. Non-diagnostic repeat-3 guard matrix is safety-clean: main10k `42.770M / 246880 KB`, worker10k `46.060M / 237056 KB`, main1k `56.686M / 115460 KB`, worker1k `57.326M / 115272 KB`, main4k `55.410M / 162072 KB`, worker4k `51.050M / 156040 KB`. Keep as the current ElasticCapacity best RSS shape; not broad promotion until source overflow/unified drain is designed. |
| ElasticDescriptorSourceRouteOverflow-L1 | `...elasticdescsource-route-desc16k-front4k-...source64-route16k-run512` | Lower-RSS component/control, not promotion. It keeps descriptor and route local caps at `16k`, trims local SourceBlock cap to `64`, and adds a shared SourceBlock depot. Diagnostic full10k is `41.516M / 225212 KB`, non-diagnostic full10k is `41.733M / 227852 KB`, and smoke main1k is `56.362M / 106048 KB`; safety is clean with `source_block_exhausted=0`, `alloc_fail=0`, `descriptor_exhausted=0`, and `route_register_fail=0`. Main-warmup confirms local `source_block_used=64` while active SourceBlock high-water reaches `10003`. This proves the source-block depot can absorb the warmup spike and cut RSS below ElasticDescriptorRouteOverflow, but speed is lower, so keep it as source-depot evidence/control. |
| SourceBlockLocalizeDryRun-L1 | `...elasticdescsource-route-localizedryrun-...source64-route16k-run512` | Diagnostic no-go/control for whole-SourceBlock localize. Transfer reuse sees depot blocks with storage-owner mismatch, but `would_move=0` because every localize probe is blocked by shared SourceBlock ref-count. This says the next design should not move whole SourceBlocks to a worker; use slot-level ownership or storage-locality policy instead. |
| SourceRunLocalityDryRun-L1 | `...elasticdescsource-route-runlocalitydryrun-...source64-route16k-run512` | Diagnostic evidence/control for slot-level/source-run ownership. Full10k run-1 is safety clean at `42.733M / 225168 KB`. It reports `elastic_source_run_locality_probe=79485`, `storage_mismatch=79485`, `run_miss=79485`, and inferred `slot_match=would_rehome_slot=79485`. Read: the current source-depot lane does not carry source-run metadata, but every probed transfer object is physically slot-localizable by block/offset/bytes. This supports a future slot-level owner/locality behavior, not whole-block movement. |
| SourceRunMetadataOnDepot-L1 | `...elasticdescsource-route-depotrunmeta-...source64-route16k-run4096` | Metadata-only prerequisite/control for slot-level locality. It keeps `HZ6_SOURCE_RUN_REUSE_L1` off and adds source-run metadata to elastic source-depot blocks. Full10k run-1 is safety clean at `42.148M / 230032 KB`; the prior `run_miss=79485` drops to `0`, while `slot_match=would_rehome_slot=79485`. Main-warmup shows depot metadata activity: `init=9939`, `mark=159024`, all mismatch counters `0`. This closes the C prerequisite from the consult; next is sparse SlotOwnerLocalityDryRun-L1, not production promotion. |
| SlotOwnerLocalityDryRun-L1 | `...elasticdescsource-route-slotownerdryrun-...source64-route16k-run4096` | Diagnostic-only A0 evidence for sparse per-slot owner/locality metadata. Full10k run-1 is safety clean at `43.174M / 230020 KB`; `elastic_slot_owner_locality_probe=79485`, `storage_mismatch=79485`, `run_miss=0`, `slot_match=79485`, `owner_match=79485`, `would_set_owner=would_hit_owner=79485`, and mismatch counters `0`. This says slot-level locality is plausible without moving whole SourceBlocks. Not behavior/promotion yet. |
| SlotOwnerSparseMeta-L1 | `...elasticdescsource-route-slotownersparse-...source64-route16k-run4096` | Diagnostic metadata feasibility lane. Full10k run-1 is safety clean at `43.039M / 233124 KB`; sparse table `lookup=79485`, `insert=79485`, `hit=0`, `update=0`, `collision=62673`, `full=0`. It proves capacity is enough for the observed transfer slots, but this workload does not show same-slot hits. Keep as side-metadata evidence, not performance behavior. |
| DescriptorDepotOwnerDirectFastPath-L1 | `...elasticdescsource-route-depotownerdirect-...source64-route16k-run512` | Previous selected Larson Elastic low-RSS sibling after SlotOwnerSparseMeta-L1. Depot descriptors read/write the shared depot owner table before OwnerSourceSideMeta-L2 storage lookup. Repeat-3 guard versus the packed source10k sibling: average `+0.52%`, min `-1.81%`, about `187-199 MB` lower RSS, safety clean. Keep as the clean source-depot control for DirectFreeTrustedLocalCache; not broad default. |
| DepotOwnerDirectDirectFreeTrustedLocalCache | `...elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-...source64-route16k-run512` | Selected Larson Elastic low-RSS sibling. It layers `HZ6_LOCAL_CACHE_DIRECT_FREE_L1` and `HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1` onto DepotOwnerDirect so local-owner frees avoid a second local-cache owner check. Repeat-3 versus DepotOwnerDirect improves every main/worker 1k/4k/10k row (`avg +2.60%`, min `+1.34%`) with essentially unchanged RSS and safety clean. |
| SlotOwnerConsumerDryRun-L1 | `...elasticdescsource-route-slotownerconsumerdryrun-...source64-route16k-run4096` | Diagnostic-only consumer evidence. Full10k run-1 is safety clean at `36.691M / 233556 KB`; `consumer_probe=687695410`, `hit=687536440`, `would_skip_l2=687536440`, `false_positive=0`, `stale_generation=0`, `fallback=158970`. This validates a narrow logical-owner fast-path experiment, but the dry-run counter volume is not speed-rankable. |
| SlotOwnerLogicalOwnerFastPath-L1 | `...elasticdescsource-route-slotownerlogical-...source64-route16k-run4096` | Safety-clean behavior no-go/control. Non-diagnostic full10k run-1: `38.494M / 239484 KB`; diagnostic full10k: `logical_probe=717941382`, `hit=717746510`, `stale_generation=0`, `owner_mismatch=0`, but speed is below depotownerdirect. The contract is valid, but broad sparse probing at every owner-equality site is too expensive. |
| OwnerEqualCallsiteDryRun-L1 | `...elasticdescsource-route-depotownerdirect-ownerequalcallsite-dryrun-...source64-route16k-run512` | Diagnostic-only callsite attribution on top of DepotOwnerDirectFastPath-L1. Full10k run-1 is safety clean at `42.434M / 224612 KB`; `owner_equal_site_free=424522978`, `owner_equal_site_local_cache=424449034`, all remote/visible/transfer/same-owner sites `0`, and `owner_equal_site_unknown=0`. This says broad slot-owner probing failed because free/local-cache dominates, so future behavior needs a cheaper predicate or narrower free/local-cache gate. |
| FreeLocalCacheOwnerPredicate-L0 | `...elasticdescsource-route-depotownerdirect-freelocalownerpredicate-dryrun-...source64-route16k-run512` | Diagnostic-only predicate witness. Full10k run-1 is safety clean at `41.049M / 224628 KB`; `flc_owner_predicate_probe=821934976`, `depot_descriptor=693768653`, `foreign_descriptor=774457636`, `source_block=821934976`, `source_block_shared=698397445`, `source_run_active=0`. This supports a depot-descriptor-only owner equality shortcut; do not gate on source_run_active here. |
| DepotDescriptorOwnerEqualFastPath-L1 | `...elasticdescsource-route-depotownerdirect-depotownerequal-...source64-route16k-run512` | Safety-clean behavior no-go/control. Non-diagnostic full10k run-1: `40.531M / 227708 KB`; diagnostic full10k reports `probe=824802008`, `hit=696139014`, `fallback=128591183`. This is slower than DepotOwnerDirectFastPath-L1 because descriptor_owner already does the depot-direct owner read; the extra free/local-cache branch costs more than it saves. |
| UnifiedDepotDrainDryRun-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdraindryrun-...source64-route16k-run4096` | Diagnostic-only unified drain/localize witness. Full10k run-1 is safety clean at `43.101M / 235420 KB`; `probe=79485`, `storage_mismatch=79485`, `run_match=79485`, `ref_shared=79485`, `owner_match=79485`, `would_slot_localize=79485`, and `would_block_whole_localize=79485`. Read: whole-block localize stays no-go, but slot-level depot drain/localize has a strong witness. |
| DepotSlotLocalize-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotslotlocalize-...source64-route16k-run4096` | Behavior no-go/control. The slot-local storage table is active, but non-diagnostic full10k reports `44.658M / 240636 KB` with `route_invalid=125` and `remote_free_transfer_fail=125`. Diagnostic full10k shows heavy use (`attempt=success=30733367`, `storage_hit=401643367`, `storage_miss=4465070`), but broad owner-source storage override is not fail-closed enough. |
| DepotSlotTransferScoped-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotslottransfer-...source64-route16k-run4096` | Safe transfer-scoped control/evidence. It records depot transfer-reuse slots in the sparse slot-owner table but does not let `owner_source_side_meta_storage()` return slot-local storage. Full10k non-diagnostic is safety-clean at `41.596M`; diagnostic reports `attempt=success=79485`, `sparse_insert=79485`, `sparse_hit=79485`, `storage_hit=0`, `route_invalid=0`, and `remote_free_transfer_fail=0`. Read: sparse slot recording is safe when scoped to transfer reuse; the prior DepotSlotLocalize failures came from broad storage-owner override. |
| DepotDescriptorRehomeDryRun-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehomedry-...source64-route16k-run4096` | Diagnostic-only descriptor locality witness on top of DepotSlotTransferScoped-L1. Full10k diagnostic is safety-clean at `43.040M`; `transfer_reuse_hit=80000`, `depot_descriptor=71811`, `run_match=71811`, `local_descriptor_available=71811`, `no_local_descriptor=0`, and `would_rehome=71811`. Read: descriptor clone/rehome is plausible at transfer reuse, but behavior must implement fail-closed route exact replacement and depot descriptor rollback/release. |
| DepotDescriptorRouteReplaceDryRun-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotroutereplacedry-...source64-route16k-run4096` | Diagnostic-only route-swap preflight on top of DepotDescriptorRehomeDryRun-L1. Full10k diagnostic is safety-clean at `41.332M`; `depot_descriptor=71811`, `run_match=71811`, `old_route_found=71811`, descriptor/generation/front/class matches are all `71811`, `current_route_same=71811`, `current_route_conflict=0`, and `would_commit=71811`. Read: the current allocator already owns an exact route that points to the old depot descriptor, so the next behavior should replace that exact route descriptor in place, not unregister-first. |
| DepotDescriptorRehome-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-...source64-route16k-run4096` | Behavior evidence, not promotion yet. Uses exact-route descriptor replacement to materialize eligible depot transfer-reuse descriptors into consumer-local descriptors. Full10k non-diagnostic is safety-clean at `44.853M`; diagnostic is safety-clean at `43.616M` with `attempt=80000`, `success=71811`, `ineligible=8189`, and all fail/rollback counters `0`. Watch item: final diagnostic `descriptor_used=77883`, so the next question is descriptor retention/bounding, not another route locality knob. |
| DepotDescriptorRehomeCapFree-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-capfree-...source64-route16k-run4096` | Control/no-go for the simple frontcache cap hypothesis. It combines DepotDescriptorRehome-L1 with `HZ6_FRONTCACHE_CAP_ON_FREE=1`. Full10k non-diagnostic is safety-clean at `43.602M`; diagnostic is safety-clean at `42.119M` with `success=71811` and all fail/rollback counters `0`. `descriptor_used` remains `77883`, `active_descriptors` is about `72327`, and `frontcache_total` is only `6072`, so the retention watch item is live consumer-local materialization, not cold frontcache backlog. |
| DepotDescriptorRehomeBudget2048-L1 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-depotdescrehome-budget2048-...source64-route16k-run4096` | Candidate-control for bounded descriptor materialization. It keeps DepotDescriptorRehome-L1 but limits rehome successes to `2048` per allocator. Initial diagnostic was safety-clean at `40.961M` with `success=30483`, `budget_denied=41328`, all fail/rollback counters `0`, and final `descriptor_used=36539`. Repeat/guard after the depot run-meta zero-count guard is safety clean: T16 main median `44.043M`, explicit main10k `44.034M`, worker10k `45.404M`, and worker/main 1k/4k guards also pass. Broad follow-up: random_mixed smoke is safety-clean, and mixed_ws balanced passes. The mixed_ws wide_ws access violation was converted to fail-closed behavior by source-block activation/release guards, but it still reports non-zero `route_invalid`/`route_miss`; keep as Larson bounded source-depot evidence, not broad/default promotion. |
| DirectFreeTrustedLocalCache + DepotDescriptorRehomeBudget2048 | `...elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-...source64-route16k-run4096` | Composition guard/control. It combines the selected Elastic DirectFreeTrustedLocalCache row with bounded descriptor materialization. Run-1 guard is safety-clean, but main10k loses to selected DFTLC (`39.525M / 234696 KB` versus `42.369M / 227444 KB`) while RSS rises. Worker/small rows can improve, but the critical cross-owner main10k row misses promotion. Keep as evidence that DFTLC and RehomeBudget do not compose into a better selected lane. |
| DirectFreeTrustedLocalCache + RehomeBudget intersection dry-run | `...elasticdescsource-route-depotrunmeta-depotownerdirect-directfree-trustedlocalcache-depotdescrehome-budget2048-intersectiondryrun-...source64-route16k-run4096` | Diagnostic-only composition attribution. It does not change behavior beyond the existing DFTLC + budget2048 lane; it counts direct-free hits/fails and transfer/rehome eligibility/success/budget-denial in the same run. Latest diagnostic: main1k is DFTLC-only (`directfree_hit=53158057`, `rehome_success=0`, `rehome_ineligible=8000`); main10k has partial overlap (`directfree_hit=362995404`, `transfer_depot=71811`, `rehome_success=30483`, `budget_denied=41328`). Keep as evidence that static budgeted rehome does not cleanly compose with selected DFTLC. Do not speed-rank this row. |
| DirectFreeTrustedLocalCache + RehomeBudget boundary | `...directfree-trustedlocalcache-depotdescrehome-budget512/1024/2048-...source64-route16k-run4096` | Boundary/control. The small-budget guard is safety-clean, but budget512/budget1024 still raise main10k RSS versus selected DFTLC (`240676/240684 KB` versus `224732 KB`) and do not provide a selected replacement. They can improve isolated small rows, so keep them as budget-boundary evidence; do not continue with another simple budget value without a new conditional policy. |
| Next design fallback | `ElasticCapacity-L1 descriptor clone/rehome at transfer reuse` | Close the slot-owner logical / depot-owner-equal shortcut family and broad slot-local storage override as evidence for now. The next useful behavior should consume the transfer-scoped slot witness by cloning/re-homing eligible depot descriptors into the consumer allocator, not by moving whole SourceBlocks or overriding general owner_source storage. |

## Current Recommendation

| Profile family | Selected HZ6 profile | Selected capacity lane | Why this lane now |
| --- | --- | --- | --- |
| balanced clean low-RSS | `rss` | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Selected mixed_ws clean low-RSS row. LinearWrap-L1 preserves linear probing semantics, and LoopCarry-L1 keeps the probe index loop-carried in hot exact route paths. Repeat-3: balanced `67.462M / 110888 KB`, safety clean. |
| wide_ws clean low-RSS | `rss` | `mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry` | Same selected mixed_ws row also improves wide_ws in the loopcarry repeat-3: `22.674M / 140320 KB`, safety clean and still lower RSS than the old route18 sibling. |
| balanced / wide_ws pressure evidence | `rss` | `descavail-noboost-route4k` | Very fast and very low-RSS, but not safety-clean for paper/default claims: it completes by hitting large `alloc_fail` / source-block exhaustion counts. Keep it as pressure evidence only. |
| mixed_ws route-capacity boundary | `rss + control` | `mixedclean-front16k-sourcerun-desc17k-source2k-route8k/16k-linearwrap-loopcarry` | Boundary evidence for the selected mixedclean route size. Route4k is no-go. Route8k is clean for balanced/larger but collapses wide_ws with source-block exhaustion. Source4k/route8k does not rescue wide_ws. Route16k with descriptor17k is clean for balanced/larger but still leaves wide_ws `alloc_fail=6943`; route17k remains the selected clean boundary. |
| random_mixed same-owner speed | `strict` | `sameownerfast-descavail-noboost-route4k` | Selected same-owner fast lane: `HZ6_SAME_OWNER_FAST_L1` + descriptor availability, promoted from the A-ladder. |
| larger_sizes RSS/speed | `speed` or `rss` | `largerlowrss-front8k-sourcerun-desc8k-route8k` | Best larger_sizes lane; needs larger front retention, not more descriptor-failure cleanup. |
| Larson cross-owner full 10k | `speed` | `ownerlocalityfast-rsscap-2-desc160k` | Full Larson cross-owner throughput/RSS balance lane; appcap-class throughput with sub-1GB peak RSS. |
| Larson cross-owner low RSS | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-source16k-route192k` | Clean route-capacity control. It keeps the thindesc/source16k shape and trims route capacity to 192K; repeat-3 is safety-clean at about `44.610M / 628844 KB`. |
| Larson cross-owner lowest-RSS balance sibling | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512` | Current selected lowest-RSS balance sibling. It combines routebytes16 with StorageOwner16 ownerless descriptors and OwnerSourceSideMeta-L2 source-block storage hints. Same-run full-10k repeat-3: routebytes16 control `40.750M / 449128 KB`, L2 `40.754M / 439912 KB`, safety clean. Worker-warmup run=1: routebytes16 `40.126M / 448948 KB`, L2 `40.787M / 439740 KB`. |
| Larson frontcache packed meta component | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512` | Lower-RSS component candidate/control, not broad throughput promotion. `HZ6_FRONTCACHE_PACKED_META_L1` shrinks `Hz6FrontCacheEntry` from 32 to 24 bytes by packing bytes-minus-one, class id, and descriptor-cold-governor detached flag into a 32-bit meta word. SourceBlockPacked closeout matrix: `41.131M / 430692 KB`, safety clean. |
| Larson sourceblock packed flags component | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-sourceblockpacked-source16k-route192k-run512` | Lower-RSS SourceBlock metadata component candidate/control over OwnerSourceSideMeta-L2. `HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1` packs SourceBlock `source_kind / active / route_registered / run_active` into one flag word while keeping `source_release` and OwnerSourceSideMeta-L2 inline. Repeat-3 closeout: `41.070M / 435304 KB`, safety clean. |
| Larson combined packed minimum-RSS candidate | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source10k-route192k-run512` | Current minimum-RSS candidate over the combined packed lane. It keeps descriptor160k/route192k/front4k but trims SourceBlock capacity from source16k to source10k. Repeat-3 main10k: `44.864M / 412280 KB`, safety clean; guard rows worker10k, main/worker4k, and main/worker1k are also safety clean. Source8k/source2k fail warmup with SourceBlock exhaustion. Keep as the current minimum-RSS sibling/candidate, not broad throughput promotion. |
| Larson combined packed source-cap backup | `speed` | `ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source12k-route192k-run512` | Safety backup / boundary control for the source10k trim. Run-1: `43.910M / 417332 KB`, safety clean. Use if source10k repeat variance or broader rows show cap pressure. |
| Larson combined packed source-cap no-go controls | `speed + diagnostic` | `source2k` / `source8k` variants of the combined packed lane | No-go/control. Source2k fails warmup with `source_block_fail_active_max=2048`; source8k fails with `source_block_fail_active_max=8192`. The cross-owner main-warmup transient SourceBlock pressure is above 8k even though final active SourceBlock count is tiny. |
| Larson Elastic front1k selected lower-RSS sibling | `speed` | `ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front1k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512` | Selected lower-RSS sibling for the Elastic DFTLC family. Frontcache-boundary repeat-3 across main/worker 1k/4k/10k saves about 20.7 MiB RSS on every row, wins 4/6 speed rows, and keeps safety clean; the largest observed speed regression is main10k `-3.06%`. Keep the front4k DFTLC row as the speed-balance control and front2k as boundary evidence/control. |
| Larson ElasticProjection local-only boundary | `speed + diagnostic` | `ownerlocalityfast-rsscap-2-elasticproj-local1k-route16k-source64-front1k-packed` | No-go/control. It tests the raw final-snapshot local cap projection. Both main/worker 1k fail during warmup with descriptor/source-block exhaustion, proving final snapshot utilization is not enough for warmup live-set sizing. MainWarmupCapacity-L1 later confirms why: main-warmup can transiently own descriptor `160048`, route `170051`, and source block `10003`. |
| Larson ElasticProjection live-set boundary | `speed + diagnostic` | `ownerlocalityfast-rsscap-2-elasticproj-live2k-route16k-source128-front1k-packed` | Boundary evidence. Worker-warmup 1k is clean at `55.585M / 60952 KB`; main-warmup 1k still fails because the main allocator seeds cross-owner live sets. Keep as evidence for ElasticCapacity-L1 shared descriptor/route/source overflow, not as a promotion lane. |
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
# Small fixed-size selected candidate only.
.\win\run_win_hz6_selected_family.ps1 `
  -SelectedSmallFixed `
  -Runs 3 `
  -TimeoutSeconds 120 `
  -ContinueOnFailure
```

2026-06-06 connectivity note:
`-SelectedFamily -Runs 1` completes with `selected-small-fixed` included.
Treat those smoke outputs as runner validation only; use repeat evidence or
archived paper rows for comparison tables.

`-SelectedSmallFixed` runs `mixed_ws large_slice_256..large_slice_16k` with
`speed + directlocalfreereuse-largerlowrss-front8k-sourcerun-desc8k-route8k`.
Use it as the selected-small runner connection check; use the documented
HZ6-only repeat-10 for the current performance read.

```powershell
# Larson full-10k selected lane plus low-RSS siblings.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonCrossOwnerSelected `
  -Runs 3 `
  -TimeoutSeconds 240 `
  -ContinueOnFailure
```

`-LarsonElasticLowRssSelected` now runs the Elastic DFTLC front4k
speed-balance control and the front1k selected lower-RSS sibling together.
Use this preset for frontcache-boundary follow-up checks instead of spelling out
the long capacity lane names by hand.

```powershell
# Elastic frontcache boundary guard: front4k / front2k / front1k across
# main/worker 1k/4k/10k.
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonElasticFrontcacheGuard `
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
# Diagnostic-only residual RSS audit for the Larson low-RSS family.
# This preset enables HZ6_DIAGNOSTIC_PROBES and should not be used as a
# production speed-ranking row. It also emits capacity utilization and elastic
# projection rows when the underlying runner captures [HZ6_CAPACITY_UTIL] and
# [HZ6_ELASTIC_PROJECTION].
.\win\run_win_hz6_selected_family.ps1 `
  -LarsonRssResidualAudit `
  -Runs 1 `
  -TimeoutSeconds 90 `
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
  strict + sameownertrustedfree-descavail-noboost-route4k

random-sameowner-trustedfree-guard:
  random_mixed small / medium / mixed
  strict + sameownerfast-descavail-noboost-route4k
  strict + sameownertrustedfree-descavail-noboost-route4k
  Use through `run_win_hz6_selected_family.ps1 -RandomSameOwnerTrustedFreeGuard`.

selected-larger-lowrss:
  mixed_ws larger_sizes
  speed/rss + largerlowrss-front8k-sourcerun-desc8k-route8k

larson-cross-owner-selected:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-desc160k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-source16k-route192k-run512
  speed + ownerlocalityfast-rsscap-2-desc160k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-source16k-route192k-run512

larson-elastic-lowrss-selected:
  larson_t16_main_10k
  speed + ownerlocalityfast-rsscap-2-elasticdescsource-route-depotownerdirect-directfree-trustedlocalcache-desc16k-front4k-thindesc-nobackptr-noroutebackptr-dir192k-routepacked-routebytes16-storageowner16-ownersourcel2-frontcachepacked-sourceblockpacked-source64-route16k-run512
  status:
    selected Larson/Elastic low-RSS sibling. It keeps DepotOwnerDirect's low
    RSS shape and repeat-3 improves every main/worker 1k/4k/10k row over
    DepotOwnerDirect, averaging +2.60% speed with essentially unchanged RSS.

larson-rss-residual-audit:
  larson_t16_main_10k
  diagnostic-only
  compares:
    OwnerSourceSideMeta-L2
    FrontCachePackedMeta-L1
    SourceBlockPackedFlags-L1
    combined packed
  output:
    [HZ6_MEMORY_ATTR]
    [HZ6_RSS_RESIDUAL]
    [HZ6_CAPACITY_UTIL]
    [HZ6_MAIN_WARMUP_CAPACITY]
    [HZ6_ELASTIC_PROJECTION]
    [HZ6_METADATA_SLIM]
    HZ6 RSS residual audit table in the generated markdown
    HZ6 capacity utilization audit table in the generated markdown
    HZ6 main-warmup capacity audit table in the generated markdown
    HZ6 elastic capacity projection table in the generated markdown
    per-worker high-water/max usage and projected local_cap_2x for
      elastic-capacity design
  status:
    attribution/evidence only, not a production speed-ranking row.

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

## Detailed References

```text
Recommended lane sets and capacity lane dictionary:
  HZ6_LANE_GUIDE_CAPACITY_REFERENCE.md

Focused mechanism lanes, frozen no-go lanes, benchmark family read, commands,
and lane rules:
  HZ6_LANE_GUIDE_MECHANISM_REFERENCE.md
```
