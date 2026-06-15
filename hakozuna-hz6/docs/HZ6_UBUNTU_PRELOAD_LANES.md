# HZ6 Ubuntu LD_PRELOAD Lanes

This document is the compact lane ledger for the Linux/Ubuntu `hz6`
LD_PRELOAD allocator. Keep long experimental notes in the root
`current_task.md`; keep Windows selected-family rows in
`HZ6_SELECTED_FAMILY_SUMMARY.md`.

## Selected Bundle

Build:

```bash
./hakozuna-hz6/linux/build_hz6_preload.sh
```

Authoritative selected flags:

```text
hakozuna-hz6/linux/hz6_preload_flags.sh
```

This document mirrors the selected bundle for review. Build and A/B scripts
should source the shared flag file and use key-based define replacement for
controls.

Output:

```text
hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so
```

Default flags selected by `build_hz6_preload.sh`:

```text
HZ6_ROUTE_TABLE_CAPACITY=65536
HZ6_OBJECT_DESCRIPTOR_CAPACITY=16384
HZ6_SOURCE_BLOCK_CAPACITY=2048
HZ6_FRONT_CACHE_BIN_CAPACITY=4096
HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=32768
HZ6_TOY_SOURCE_BLOCK_BYTES=65536
HZ6_MIDPAGE_RUN_BYTES=786432
HZ6_MIDPAGE_32K_RUN_BYTES=1572864
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1
HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1
HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=1
HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1
HZ6_LINUX_MMAP_RETAIN_L1=1
HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=128
HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1
HZ6_ROUTE_HASH_XOR_FOLD_L1=1
HZ6_ROUTE_LINEAR_WRAP_L1=1
HZ6_ROUTE_LOOP_CARRY_L1=1
```

The preload build script also explicitly keeps these no-go/control lanes off
unless `HZ6_EXTRA_CFLAGS` overrides them for an A/B run:

```text
HZ6_LINUX_MMAP_RETAIN_TLS_L1=0
HZ6_SOURCE_RUN_REUSE_L1=0
HZ6_ROUTE_PACKED_META_L1=0
HZ6_PRELOAD_FAST_FREE_L1=0
```

## Current Read

The preload lane is a real Ubuntu performance lane, but it remains separate
from the direct HZ6 API and Windows selected-family rows.

Latest selected-default focused guards, after current-bias, 8K run768, and
32K run1536:

| Row | Selected read |
| --- | ---: |
| `16..256` HZ6-only repeat-5 | `57.985M / 30.38 MiB` |
| `16..4096` HZ6-only repeat-5 | `41.868M / 79.75 MiB` |
| `1024..4096` HZ6-only repeat-5 | `40.253M / 91.12 MiB` |
| `4096..16384` HZ6-only repeat-5 | `48.563M / 94.50 MiB` |

Latest cross-allocator refresh after 32K run1536 promotion, repeat-3,
`bench_mixed_ws_crt`:

| Row | hz6 | mimalloc | tcmalloc | system | hz6 peak KB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `16..256` | `56.806M` | `53.272M` | `255.572M` | `105.393M` | `31,104` |
| `16..4096` | `40.578M` | `7.084M` | `98.825M` | `17.043M` | `81,664` |
| `1024..4096` | `38.725M` | `5.568M` | `92.349M` | `9.745M` | `93,184` |
| `4096..16384` | `45.283M` | `1.323M` | `44.034M` | `2.943M` | `96,640` |

Important caveat:

```text
tcmalloc remains much faster on tiny and mixed small rows. On 4096..16384, HZ6
now edges tcmalloc on speed while keeping lower RSS and better ops-per-MiB.
HZ6 is clearly ahead of mimalloc on the selected mixed_ws preload rows, but HZ3
remains the higher speed/RSS frontier.
```

Follow-up:

```text
MidPage 8K run768 and 32K run1536 are now selected. The current selected lane
keeps the static-table RSS cut and reaches 48.563M / 94.50 MiB on the HZ6-only
4096..16384 repeat-5, with full-cross repeat-3 still ahead of tcmalloc. Keep
32K run768 and run512 as direct controls.
```

Current follow-up read:

| Lane | Status | Read |
| --- | --- | --- |
| MidPage active-map deeper probe | no-go direction | Miss attribution showed `found_elsewhere=0`; misses are not probe-limit misses. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | watch/control | Re-tested after descriptor-out: target improves, but `16..256` repeat-7 is still slightly weaker. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | watch/control | Helps Toy/tiny by skipping impossible MidPage probes, but target has `addr_envelope_skip=0`. Keep off. |
| Next likely lane | diagnostic/design | Follow `HZ6_UBUNTU_MIDPAGE_NEXT_DESIGN.md`: first add transfer-probe attribution, then isolate any target-specialized transfer-skip lane from the selected balanced DSO. |

HZ3/HZ4 comparison read:

| Row | hz6 | hz3 | hz4 | Read |
| --- | ---: | ---: | ---: | --- |
| `16..256` | `56.028M` | `265.824M` | `224.971M` | HZ3/HZ4 are still architecture-fast here. |
| `16..4096` | `36.843M` | `101.277M` | `57.714M` | HZ6 is about `0.64x` HZ4. |
| `1024..4096` | `35.131M` | `91.406M` | `49.979M` | HZ6 is about `0.70x` HZ4. |
| `4096..16384` old default | `29.409M` | `74.802M` | `31.089M` | HZ6 was about `0.95x` HZ4 and had the clearest close target. |
| `4096..16384` MidPage unaligned/probe4 | `31.505M` | n/a | `30.916M` | HZ6 now edges HZ4 while keeping lower RSS. |

Architecture read:

```text
HZ3/HZ4:
  page/segment metadata first
  thin TLS/tcache local path
  class/owner recovery without HZ6 route descriptors in the common path

HZ6:
  explicit RouteLayer / descriptor / SourceLayer / FrontCache contracts
  stronger VALID / INVALID / MISS separation
  better scoped RSS and safety boundaries, but more hot-path work

Near-term target:
  close/beat HZ4 on 4096..16384 while keeping HZ6 RSS below HZ4.
  MidPageActiveMapUnaligned-L2 + probe4 reaches this on the latest guard:
    hz6 31.505M / 117,248 KB
    hz4 30.916M / 134,400 KB

Longer-term target:
  design a local-page/run metadata fast lane if HZ6 must chase HZ3/HZ4 tiny
  object rows. Do not blur that with the current descriptor-first default lane.
```

## Promotion History

| Stage | Decision | Evidence |
| --- | --- | --- |
| Route capacity | `HZ6_ROUTE_TABLE_CAPACITY=131072` | Initial preload lane moved from about `0.236M` to about `3.05M`. |
| Source retain | `HZ6_LINUX_MMAP_RETAIN_L1=1` | Repeat-3 moved to about `7.0M..7.3M`; strace showed mmap/munmap churn reduction. |
| 64K retain stack | `HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1` | Repeat median moved to about `9.7M`; targets Toy 64KiB source blocks. |
| Toy full prefill | `HZ6_TOY_FULL_BLOCK_PREFILL_L1=1`, max128 | First guard moved to about `12.3M..12.6M`; Toy source churn dropped. |
| Tombstone compact | `HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1` | Removed the 1M long-run cliff; non-aggressive compact only. |
| Frontcache 8192 | `HZ6_FRONT_CACHE_BIN_CAPACITY=8192` | Focused 1M median moved to about `34.2M`; overflow unregister/tombstone churn removed. |
| Toy active fast free | `PreloadToyActiveFastFree-L1` | 16..256 route probes dropped from about `2.12M` to about `37.7K`; focused cross edged past mimalloc. |
| MidPage run 768K | `HZ6_MIDPAGE_RUN_BYTES=786432` | Current selected 8K MidPage run size after current-bias. Earlier run256K moved 4096..16384 from `16.549M` to `19.394M`; run768K later passed repeat-15 and selected repeat-5. |
| External MidPage active map | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1`, external, cap8192/probe2 | Repeat-7 versus no-map moved 1024..4096 from `30.962M` to `32.011M` and 4096..16384 from `18.983M` to `19.903M`. |
| Realloc in-place | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1` | Repeat-5 versus control-off moved 16..256 `50.810M -> 55.313M`, 16..4096 `33.867M -> 36.556M`, 1024..4096 `31.473M -> 34.678M`, and 4096..16384 `19.971M -> 30.118M`. |
| MidPage unaligned/probe4 | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1`, probe4 | 4096..16384 active-map hits moved from `3,321` to `915,393`; repeat-5 HZ4 guard reached `hz6 31.505M` vs `hz4 30.916M` with lower HZ6 RSS. |
| MidPage active-map cap16K | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384`, probe4 | Resume diagnostic showed MidPage active-map misses on 4096..16384. Focused repeat-5 held `27.067M` versus pre-promotion default `25.908M`; cross repeat-3 held `27.752M` with about `+0.6MB` RSS. Cap32K and cap16K probe2/probe8 were weaker. |
| MidPage alloc descriptor-out | `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1` | MidPage alloc returns the activated descriptor to active-map registration, avoiding the post-alloc exact route lookup without changing prefill policy. Focused repeat-5 moved 4096..16384 median `30.004M -> 34.600M`, 16..4096 `41.126M -> 41.897M`, and 16..256 `55.985M -> 56.595M`; 1024..4096 was essentially flat. |
| MidPage miss audit after cap16K | `HZ6_PRELOAD_STATS=per_allocator` | Cap16K diagnostic still showed MidPage `free_hit=397136`, `free_miss=19254`, and `free_local_route_valid=9633` on the 200K 4096..16384 guard. The remaining miss path is real, but preload-boundary shortcuts must pass tiny guards. |
| MidPageSourcePressureAudit-L1 | diagnostic-only | Next lane. Split MidPage 8K/32K alloc, run prefill, active-map register/free hits, and preload local route fallback before changing source-run or route-register behavior. Active-map miss remains aggregate because the class is not known until route fallback. Active-map internals are closed for now. |
| MidPage source pressure read | diagnostic-only | 4096..16384 is 32K dominated: `midpage_32k_alloc_call=271026`, `midpage_8k_alloc_call=352`, `midpage_32k_prefill_run_call=2769`, `midpage_8k_prefill_run_call=352` on the 200K guard. |
| MidPage route-register class split | diagnostic-only | Source-run-slot route registration is visible now: on the 500K 4096..16384 guard, `route_register_reason_source_run_slot=17320`, split as `8K=5760` and `32K=11224`. This is much smaller than the 1M-class active-map register/free path, so do not chase source-run-slot route registration first. |
| MidPage active-map miss attribution | diagnostic-only | On selected 500K 4096..16384, MidPage active-map misses were not probe-limit misses: `free_miss=3102`, `probe_empty=2658`, `probe_occupied=444`, `found_elsewhere=0`. Deeper free probing is therefore not the next lever. |
| MidPage active-map lifetime audit | diagnostic-only | On selected 500K 4096..16384, `register_overwrite=1287` while route fallback was `8K=472`, `32K=815`. Overwrite explains much of the remaining fallback shape, but preserving old entries is not automatically better. |
| MidPage same-class victim dry-run | diagnostic/control | Same-class alternate victims exist (`same_class_alt=924` on a diagnostic 4096..16384 run), but behavior `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` regressed target median `34.597M -> 34.169M`; do not promote. |
| MidPage register callsite audit | diagnostic-only | With descriptor-out selected, 500K 4096..16384 showed `midpage_active_map_register_direct=333540`, `front_alloc=666979`, and `route_fallback=0`. The remaining target pressure is normal active-map registration, not post-alloc exact route fallback. |
| MidPage trusted activation skip | control/no-go | `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` skips the source-block bounds check on trusted local-free MidPage reuse. Focused repeat-5 was not better: 4096..16384 `35.114M -> 34.891M`, 16..256 `57.131M -> 56.538M`; keep off. |
| MidPage preclassified malloc | no-go direction | Perf showed malloc classification cost on 4096..16384, but preclassifying MidPage in the hot malloc shape regressed tiny guard badly (`16..256 55.463M -> 40.188M`) despite target gain. Do not keep the code-shape change. |
| MidPage free-cache audit | diagnostic/control | Selected 500K 4096..16384 had `midpage_active_map_free_cache_attempt=998685`, `success=998685`, `fail=0`; 8K/32K frontcache pushes were `339583/678023`. Failure is not the issue; only a thinner success path could help. |
| `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | control/no-go | Direct trusted cache push after active-map validation was flat/negative on the correct repeat-5: 4096..16384 `34.657M -> 34.477M`, 1024..4096 `40.039M -> 40.290M`, 16..256 `56.214M -> 55.622M`. Keep off. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | control/no-go | Skipping empty transfer-first probes for MidPage direct-local reuse is target-positive but guard-sensitive. Helper-shape repeat-7 moved 4096..16384 `34.804M -> 38.944M`, but regressed 16..4096 `41.947M -> 40.394M` and 1024..4096 `40.552M -> 39.062M`; keep off. |
| TransferProbeAudit-L1 | diagnostic-only | Selected diagnostic 500K showed 4096..16384 has `333720` MidPage direct transfer probes and every probe was empty; 16..4096 had only `63` empty probes. The target witness is real, but selected default still needs guard isolation. |
| MidPage target DSO | control alias | `./hakozuna-hz6/linux/build_hz6_preload_midpage_target.sh` builds `out/linux/hz6_preload_midpage_target/libhakozuna_hz6_preload.so` with the selected outer-guard noinline MidPage malloc boundary. Use when a named MidPage-target DSO is useful. |
| MidPage guard-isolated transfer skip | control/no-go | noinline and noinline+unlikely helper shapes kept the 4096..16384 win (`~39.4M`), but still regressed 16..256, 16..4096, and 1024..4096 beyond the promotion gate. Keep as target DSO/control only. |
| MidPage preload-boundary malloc skip | selected/default | Wrapper-level MidPage malloc boundary keeps selected `hz6_malloc()` shape clean. The first direct boundary shape improved target but regressed guards; the promoted outer-guard noinline shape passed repeat-15. Confirmation against explicit boundary-off control: 4096..16384 `33.735M -> 40.056M`, 16..256 `55.957M -> 56.539M`, 16..4096 `40.969M -> 41.339M`, 1024..4096 `39.059M -> 39.953M`. |
| Static table trim | selected/default | `HZ6_ROUTE_TABLE_CAPACITY=65536`, `HZ6_OBJECT_DESCRIPTOR_CAPACITY=16384`, `HZ6_SOURCE_BLOCK_CAPACITY=2048`, and `HZ6_FRONT_CACHE_BIN_CAPACITY=4096` reduce allocator-local fixed table RSS. Confirm repeat-5 without stats moved 16..4096 `41.519M / 100.62 MiB -> 43.581M / 79.75 MiB`, 1024..4096 `39.966M / 111.75 MiB -> 41.849M / 91.00 MiB`, and 4096..16384 `40.863M / 115.25 MiB -> 42.904M / 94.38 MiB`; no route/descriptor/source failures in the repeat-3 safety lane. |
| MidPage 32K run1536 | selected/default | `HZ6_MIDPAGE_32K_RUN_BYTES=1572864` reduces 32K source churn after run768. Focused repeat-15 moved 4096..16384 `46.078M -> 47.991M` with small guard costs under 1%; stats repeat-3 kept fail counters 0 and cut 4096..16384 source_alloc `1599 -> 900`. Keep run768 and run512 as direct controls. |
| MidPage 32K run768 | control | Previous selected `HZ6_MIDPAGE_32K_RUN_BYTES=786432`; keep as direct control for run1536. |
| MidPage supply/map resume | diagnostic/control | After run768, selected diagnostic shows 4096..16384 has only about `2.2K` free route fallbacks for `1M` frees; free path is no longer the main wall. Remaining pressure is MidPage supply/frontcache shape: `midpage_source_alloc=649`, split as `8K alloc_call=180` and `32K alloc_call=469`. |
| MidPage 8K run768 | selected/default | `HZ6_MIDPAGE_RUN_BYTES=786432` reduces 8K source-run churn after current-bias. Repeat-15 versus run256K kept guards flat/positive and moved 4096..16384 +0.71%; post-promotion selected repeat-5 reached 46.496M / 94.25 MiB on 4096..16384 with clean safety stats. Keep 256K and 512K as direct controls. |
| `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | control/no-go | MidPage front retries local frontcache only after successful prefill_run, skipping a second transfer-first probe. Repeat-15 was not selected-safe: 16..256 regressed -7.11% and 4096..16384 regressed -2.08%. Keep off. |
| MidPage active-map cap/probe widening | control/no-go | `cap32K/64K` and `probe8` remove most remaining route-after-map fallbacks, but larger/hotter maps regress target speed and raise RSS. Example focused repeat-5: cap32K/probe4 cut 4096..16384 `route_after_maps ~2199 -> 124`, but target speed fell and RSS rose. Keep cap16K/probe4 selected. |
| MidPage low-water refill | control/no-go | Default-off behavior control. After a MidPage cache-miss refill successfully returns an object, optionally prefill one more run if that class remains below a small low-water mark. Stats-off repeat-9 did not pass; strong 8K=256/32K=128 was target-flat and guard-negative. Keep off. |
| Frontcache class-max attribution | diagnostic-only | `HZ6_PRELOAD_FRONTCACHE_CLASS_DETAIL` prints class-level push/pop-empty/max occupancy. FrontcacheShapeAudit repeat-3 showed class4 reaches cap4096 on 1024..4096, while class5 reaches about 2832 on 4096..16384. This supports future lazy/cold storage design, but not a broad cap shrink. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | control/watch, not selected | Default-off class-specific frontcache backing storage trim. It shrinks allocator-local frontcache entry arrays by class while preserving class2/class4 4096 storage and class5 3072 storage. It is buildable and mostly speed-neutral/target-positive in production-shape repeat-7, but peak RSS barely moved and diagnostic target was weak. Keep as a storage-layout control, not selected. |
| Preload hook path attribution | diagnostic-only | `HZ6_PRELOAD_HOOK_DETAIL` splits free() hook flow across Toy active-map attempt/hit/miss, MidPage active-map attempt/hit/miss, route-after-map lookup, local/visible route valid, invalid, and real fallback. First short read showed 4096..16384 pays mostly Toy active-map misses before MidPage hits. |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | control/no-go | Unconditionally trying MidPage active-map before Toy active-map recovers part of the 4096..16384 Toy-miss wall, but the guard cost is too high: tiny/Toy rows regress materially. Keep off in selected default; use as evidence for a future class-aware/free-hint gate. |
| `HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1=1` | control/no-go | Cheap selective MidPage-first free gate using MidPage active-map alignment. It behaved like unconditional MidPage-first because Toy-heavy rows also passed the alignment gate; guard regressions were large. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1` | selected/default | Cheap selective MidPage-first free gate using allocator-local active-map current counts. If MidPage active entries exceed Toy active entries, free tries MidPage first. Promotion repeat-15 kept 16..4096 and 1024..4096 flat/slightly positive, limited 16..256 to -0.45%, and improved 4096..16384 by +3.24% with flat RSS. |
| `HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1=1` | diagnostic-only/no-go direction | Dry-run for selective MidPage-first free ordering. The recent min/max envelope covered 4096..16384 well, but false positives were huge on 16..4096 and 1024..4096. Behavior remains Toy-first; future work needs a tighter range/page hint table. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | diagnostic-only | Tighter selective MidPage-first free dry-run. Preload MidPage malloc boundary hits register base/last 4K pages in a small TLS hint table; free() probes the table and reports through the existing `mh_*` hook-detail counters. Capacity32768 was much cleaner than the broad envelope, but the behavior A/B showed the per-free probe cost is too high. Keep as evidence only. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | control/no-go | Behavior A/B for the page-hint gate. Hinted frees use MidPage-first ordering; unhinted frees preserve selected Toy-first ordering. It reduced 4096..16384 Toy active-map attempts, but short repeat-5 regressed every focused row, so the per-free hint probe/table overhead is not selected-safe. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | control/no-go | Base-slot-first MidPage active-map free lookup. It avoids a new free classifier and only changes the existing probe code shape, but short repeat-5 was target-negative (`4096..16384 26.295M -> 25.422M`) and guard-flat/weak. Keep off. |

Closeout details for the selective MidPage-first free lanes are in:

```text
HZ6_UBUNTU_PRELOAD_FREE_HINT_CLOSEOUT.md
HZ6_UBUNTU_PRELOAD_FREE_ORDER_CLOSEOUT.md
```

## Selected Controls

Keep these controls available when changing the preload lane:

| Control | Why |
| --- | --- |
| no MidPage active map | Direct control for `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2`. |
| internal MidPage active map | MidPage-only upper-bound; repeat-7 4096..16384 reached `20.504M`, but balanced locality was worse than external storage. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=786432` | Previous selected 32K run size and direct control for run1536. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=524288` | Earlier selected 32K run size and direct control below run768. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=262144` | Earlier selected 32K run size and direct control for run512. |
| `HZ6_MIDPAGE_RUN_BYTES=262144` | Previous selected 8K run size and direct control for run768. |
| `HZ6_MIDPAGE_RUN_BYTES=524288` | 8K source-run widening control. It lowers source_alloc on 4096..16384 and remains target-positive after current-bias, but did not pass guard gates. |
| `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | MidPage low-water refill behavior control. Default thresholds 8K=128/32K=64 and strong thresholds 8K=256/32K=128 did not pass selected gates. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=0` | Direct control for MidPage active-map 8K-alignment gating. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=8192` | Previous balanced default and direct control for the selected cap16K promotion. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=2` | Balanced control for the selected probe4 MidPage active map. Probe2 is slightly better on 1024..4096 but weaker on the HZ4-close 4096..16384 target. |
| `HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=0` | Direct control for the selected MidPage descriptor-out malloc path. |
| frontcache 8192/16384 | Wide frontcache controls after static table trim. 8192 is the previous selected capacity; 16384 was flat enough not to promote. |
| `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=3072` | Class-specific cap control. Safe in the first FrontcacheShapeAudit repeat-3 but did not improve speed/RSS enough to promote. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | Class-specific frontcache backing-storage trim control. It tests RSS from storage shape rather than logical bin caps. |
| route table 262144 | Capacity upper-bound; not selected by current evidence. |
| `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=0` | Direct control for preload realloc in-place. |
| `wide_l0` static tables | Previous preload table capacities: route 131072, descriptor 32768, source blocks 4096, frontcache bin 8192. Keep as direct RSS/safety control for the selected trim. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=0` | Direct control for the selected balanced current-bias free ordering. |

MidPage 32K run-size closeout details are in
`HZ6_UBUNTU_MIDPAGE_32K_RUN_CLOSEOUT.md`.

## No-Go / Evidence-Only

| Lane | Read |
| --- | --- |
| Toy-map widening to MidPage | Reduced route probes but created too many map misses/collisions and slightly regressed focused repeat; do not widen the Toy active map. |
| Toy active-map capacity 65536/16384 | Both capacity directions regressed Toy high rows in focused repeat; keep the selected 32768 default. |
| Toy active-map probe8 | Roughly flat on 1024..4096 and weaker on 16..4096; keep probe4. |
| MidPage active-map cap32K | Regressed both 1024..4096 and 4096..16384 in the resume repeat-3; too large for current locality. |
| MidPage active-map cap16K probe2/probe8 | Both were weaker than cap16K/probe4 in focused repeat-3. |
| `HZ6_PRELOAD_MIDPAGE_ROUTE_REARM_L1=1` | Re-arming the MidPage active map after preload local route-valid shifted hits slightly but produced only a small/flat 4096..16384 change; keep as evidence-only. |
| `HZ6_PRELOAD_MIDPAGE_FAST_FREE_L1=1` | MidPage-only prechecked free improved 4096..16384 median in a short focused repeat (`27.855M control -> 28.094M`) but regressed the tiny 16..256 guard (`57.9M control -> 55.0M`); keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1=1` | 4K-granularity index helped some Toy/high guards but regressed the target 4096..16384 row to about `24.8M`; keep the selected 8K-shift index. |
| `HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1=1` | Preserving existing entries on full probe looked plausible, but target 4096..16384 regressed before and after descriptor-out. Latest retest moved `35.281M -> 34.100M`; keep current base-slot overwrite policy. |
| `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1` | Toy-style register fast-slot worsened guards and target rows; 4096..16384 fell to about `24.2M`. Keep the current bounded loop shape for MidPage. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=128K/192K/224K` | Smaller 32K runs looked like payload-trim candidates, but RSS stayed flat while source allocation rose and 4096..16384 slowed. Keep as no-go evidence. |
| `HZ6_MIDPAGE_RUN_BYTES=384K/512K/640K` | 8K run widening controls below selected run768. 512K remained target-positive but guard-negative; 384K/640K are evidence-only. |
| `HZ6_MIDPAGE_PREFILL_CACHE_ONLY_REUSE_L1=1` | Post-prefill cache-only reuse skipped a second transfer-first probe, but repeat-15 regressed both tiny and target rows. Keep off. |
| `HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1` | Eagerly prefill one more MidPage run after miss-boundary reuse. It adds source/refill work and did not improve stats-off selected balance. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=32768/65536` and probe8 | Larger maps/probes remove remaining route fallback but lose speed/RSS to map hotness. Keep selected cap16K/probe4. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=2` | Current-bias target upper-bound. 4096..16384 can improve more than 1x, but 16..4096 regresses too much. Keep off. |
| `HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Toy active-map negative envelope. It is conservative and buildable, but the first repeat-3 did not pass promotion: tiny was slightly positive while 1024..4096 and 4096..16384 were weak. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | Unconditional preload free order swap. Target-positive but guard-negative; keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | Page-table dry-run for selective MidPage-first ordering. Use before any behavior gate; it must avoid the false-positive wall observed in the broad recent-envelope dry-run. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | Page-hinted MidPage-first behavior control. It is narrower than unconditional MidPage-first but still slower in short repeat-5; keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1` | MidPage active-map free lookup fast-slot control. No new classifier, but not faster in focused repeat; keep off. |
| `HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY=2560/2048` | Class5 cap shrink increased class5 empty pops and slowed 4096..16384 in FrontcacheShapeAudit-L1. Keep off. |
| `HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY=3072` with 32K cap3072 | Broad MidPage cap shrink regressed 1024..4096 badly because class4 can genuinely reach cap4096. Keep off. |
| `HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1=1` | Class-specific storage trim did not produce a meaningful peak RSS reduction in the focused preload rows. Production-shape repeat-7 was acceptable but not enough for default; diagnostic repeat-1 was target-weak. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1=1` | Class-salted MidPage active-map index did not reduce collision pressure in diagnostic (`register_collision 369139 -> 373449`, `free_miss 3632 -> 4624`) and regressed 1024..4096 in the first A/B. Keep off. |
| `HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1=1` | Avoiding post-prefill exact route lookup by prefill/direct-pop retry is too disruptive: first repeat-3 dropped 4096..16384 from about `29.5M` to about `0.52M` and raised RSS. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1=1` | Conservative min/max negative filter skips Toy/tiny MidPage-map probes (`addr_envelope_skip=366` on 16..256 diagnostic) and improved non-target guards in first repeat-3, but did not help the 4096..16384 target (`addr_envelope_skip=0`, slight target regression). Keep as control, not selected. |
| `HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1=1` | Prefer same-class entry as overwrite victim when the probe window is full. Dry-run found candidates, but behavior regressed 1024..4096 and 4096..16384 in the first repeat-3. Keep off. |
| `HZ6_MIDPAGE_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1=1` | Trusted local-free MidPage activation can skip source-block bounds validation safely in principle, but first focused repeat-5 did not improve target or tiny guard. Keep off. |
| `HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1=1` | Direct MidPage active-map free success cache path. It removes the generic cache helper call but did not improve the target and regressed tiny guard, so keep off. |
| `HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1` | MidPage direct-local reuse skips the transfer-first probe. It is a useful target witness, but the tested helper shape regressed non-MidPage guards enough to block selection. |
| `build_hz6_preload_midpage_target.sh` | Named MidPage-target DSO alias for the selected preload-boundary transfer-skip shape. |
| `build_hz6_preload_midpage_boundary_control.sh` | Explicit boundary-off control DSO for confirming the selected preload-boundary transfer-skip shape. |
| `run_hz6_preload_midpage_boundary_ab.sh` | Repeat runner for selected default versus boundary-off control on `4096..16384`, `16..256`, `16..4096`, and `1024..4096`. |
| `run_hz6_ubuntu_selected_balance_matrix.sh` | Cross-allocator speed/RSS balance matrix for selected HZ6 versus system/HZ3/HZ4/HZ5/mimalloc/tcmalloc. |
| `build_hz6_preload_diag.sh` | Diagnostic preload build wrapper with `HZ6_DIAGNOSTIC_PROBES=1`; use for attribution, not selected speed ranking. |
| `run_hz6_midpage_rss_audit.sh` | Diagnostic RSS attribution runner for `16..4096`, `1024..4096`, and `4096..16384`. |
| `run_hz6_midpage_supply_map_ab.sh` | Focused A/B runner for 8K run widening and MidPage active-map capacity/probe controls after run768. Use default `--stats` for attribution; use `--no-stats` for production-shape speed/RSS ranking. |
| `run_hz6_midpage_payload_trim_ab.sh` | Focused A/B runner for 32K run-size payload/supply controls. Supports `--variants`, `--include-tiny`, `--stats`, and `--no-stats`; use `--no-stats` for speed/RSS promotion gates and `--stats` for fail/source attribution. |
| `run_hz6_preload_free_order_ab.sh` | Focused A/B runner for preload free-order controls: selected, unconditional MidPage-first, aligned-first, current-bias 1x/2x/4x, and delta64. |
| `run_hz6_static_table_trim_ab.sh` | Builds selected trim and wide-table controls, then compares speed/RSS plus failure counters. |
| `run_hz6_midpage_payload_trim_ab.sh` | Builds MidPage 32K run-size controls and compares selected speed/RSS plus source/failure counters. |
| `run_hz6_frontcache_shape_ab.sh` | Builds selected, class-specific MidPage frontcache cap controls, and storage-trim control. Use default `--stats --diagnostics` for attribution; use `--no-stats --no-diagnostics` for production-shape speed/RSS ranking. |
| MidPage noinline/branch-isolated transfer skip | Still guard-sensitive. Branch/layout isolation did not make it selected-safe, so do not add it to `build_hz6_preload.sh`. |
| MidPage preclassified malloc shape | Direct 4097..32768 MidPage classification improved target in short repeat, but disturbed `16..256` too much. Avoid broad malloc code-shape changes unless small guards are isolated first. |
| active-map slot-index/code-shape helper | No selected-row win; changing this header shape can disturb MidPage/Toy preload layout, so keep the existing body. |
| `HZ6_LINUX_MMAP_RETAIN_TLS_L1=1` | Did not reduce mmap count and regressed repeat-3. |
| `HZ6_SOURCE_RUN_REUSE_L1=1` | Reduced source allocation count but reusable-run scan/activation cost dominated. |
| Toy source blocks 128K/256K | Raised RSS and regressed throughput. |
| `HZ6_ROUTE_PACKED_META_L1=1` | Lowered RSS slightly but was slower/less stable than unpacked route metadata. |
| aggressive route tombstone compact | Too costly; keep normal compact only. |
| `HZ6_PRELOAD_FAST_FREE_L1=1` | Prechecked-route reuse did not improve mixed_ws; route registration remained dominant. |

## Diagnostic Use

Use stats only for attribution, not speed ranking:

```bash
HZ6_PRELOAD_STATS=1 \
LD_PRELOAD=$PWD/hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so \
./bench/out/linux/x86_64/bench_mixed_ws_crt 4 500000 4096 4096 16384
```

Useful lines:

```text
[HZ6_PRELOAD_STATS]
[HZ6_PRELOAD_ROUTE_DETAIL]
[HZ6_PRELOAD_FRONT_DETAIL]
[HZ6_PRELOAD_PHASE_STATS]
[HZ6_PRELOAD_SIZE_STATS]
```

`[HZ6_PRELOAD_SIZE_STATS]` is the 1024..4096 boundary audit line. It is
diagnostic-only and splits malloc/realloc requests into `<=1024`,
`1025..4096`, `4097..16384`, and `>16384` buckets, plus owned old-size
realloc buckets and realloc copy calls. Use it with the existing route/source
and phase lines before changing Toy/MidPage boundary behavior.

Use diagnostic builds when detailed probe counters are needed:

```bash
./hakozuna-hz6/linux/build_hz6_preload_diag.sh
```

Use the MidPage RSS audit runner to split selected peak RSS pressure:

```bash
./hakozuna-hz6/linux/run_hz6_midpage_rss_audit.sh --iters 200000
```

The diagnostic preload emits one additional attribution line:

```text
[HZ6_PRELOAD_MEMORY_ATTR]
```

Important caveat:

```text
preload_attributed_bytes is an attribution estimate, not exact RSS.
source_block_payload_bytes is logical source backing capacity and can exceed
resident pages. Use the line to choose the next lane, not as a replacement for
peak_kb from the benchmark.
```

## Raw Evidence

Recent raw evidence directories:

```text
private/raw-results/linux/hz6_midpage_rss_audit_20260614_164214
private/raw-results/linux/hz6_static_table_trim_ab_20260614_164920
private/raw-results/linux/hz6_static_table_trim_confirm_20260614_165003
private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_165226
private/raw-results/linux/hz6_midpage_payload_trim_ab_20260614_194352
private/raw-results/linux/hz6_midpage_run512_confirm_20260614_194437
private/raw-results/linux/hz6_ubuntu_selected_balance_20260614_162527
private/raw-results/linux/hz6_preload_activefast_cross_20260613
private/raw-results/linux/hz6_preload_midrun_ladder_r3_20260613
private/raw-results/linux/hz6_preload_midrun_default256_guard_20260613
private/raw-results/linux/hz6_preload_midmap_route_cap_small_r3_20260613
private/raw-results/linux/hz6_preload_midmap_selected_guard_20260613
private/raw-results/linux/hz6_preload_midmap_external_control_r7_20260613
private/raw-results/linux/hz6_preload_midmap_external_diag_20260613
private/raw-results/linux/hz6_preload_realloc_audit_20260613
private/raw-results/linux/hz6_preload_realloc_inplace_ab_r5_20260613
private/raw-results/linux/hz6_preload_realloc_cross_r5_20260613
private/raw-results/linux/hz6_preload_vs_hz3_hz4_r5_20260613
private/raw-results/linux/hz6_midpage_hz4close_diag_20260613
private/raw-results/linux/hz6_midpage_unaligned_ab_r5_20260613
private/raw-results/linux/hz6_midpage_probe4_ab_r5_20260613
private/raw-results/linux/hz6_midpage_probe4_hz4_guard_r5_20260613
private/raw-results/linux/hz6_toy_high_alloc_path_diag_20260613
private/raw-results/linux/hz6_toy_active_map_tune_r5_20260613
private/raw-results/linux/hz6_toy_active_map_cap16_r5_20260613
private/raw-results/linux/hz6_clean_rebuild_default_matrix_guard_r5_20260613
private/raw-results/linux/hz6_preload_1024_4096_boundary_audit_20260613
private/raw-results/linux/hz6_toy_preclassified_malloc_r5_20260613
private/raw-results/linux/hz6_toy_active_map_register_fastslot_r5_20260613
private/raw-results/linux/hz6_toy_active_map_register_fastslot_matrix_r5_20260613
private/raw-results/linux/hz6_direct_local_reuse_rawpop_r5_20260613
private/raw-results/linux/hz6_toy_active_map_free_fastslot_r5_20260613
private/raw-results/linux/hz6_preload_selected_cross_fastslot_r5_20260613
private/raw-results/linux/hz6_toy_trusted_activate_skip_r5_20260613
private/raw-results/linux/hz6_toy_trusted_activate_skip_focus_r9_20260613
private/raw-results/linux/hz6_preload_selected_cross_actskip_r5_20260613
private/raw-results/linux/hz6_toy_run_local_meta_diag_20260613
private/raw-results/linux/hz6_smallrun_route_behavior_toy_r5_20260613
```

Storage rule:

```text
private/raw-results/linux:
  raw local evidence, not paper-facing curated output

HZ6_UBUNTU_PRELOAD_LANES.md:
  selected/default/control/no-go summary only

current_task.md:
  detailed chronological experiment ledger
```
