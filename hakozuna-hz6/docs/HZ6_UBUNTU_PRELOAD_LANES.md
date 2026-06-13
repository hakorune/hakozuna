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

Output:

```text
hakozuna-hz6/out/linux/hz6_preload/libhakozuna_hz6_preload.so
```

Default flags selected by `build_hz6_preload.sh`:

```text
HZ6_ROUTE_TABLE_CAPACITY=131072
HZ6_OBJECT_DESCRIPTOR_CAPACITY=32768
HZ6_SOURCE_BLOCK_CAPACITY=4096
HZ6_FRONT_CACHE_BIN_CAPACITY=8192
HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=32768
HZ6_TOY_SOURCE_BLOCK_BYTES=65536
HZ6_MIDPAGE_RUN_BYTES=262144
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
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

Latest selected shape:

| Row | Selected read |
| --- | ---: |
| `16..256` cap16K cross repeat-3 | `hz6 57.671M` vs `mimalloc 53.099M` |
| `16..4096` cap16K cross repeat-3 | `hz6 39.382M` vs `mimalloc 5.873M` |
| `1024..4096` cap16K cross repeat-3 | `hz6 38.497M` vs `mimalloc 4.858M` |
| `4096..16384` cap16K cross repeat-3 | `hz6 27.752M` vs `mimalloc 1.282M` |
| `4096..16384` cap16K focused repeat-5 | `27.067M` vs pre-promotion default `25.908M` |
| `1024..4096` external-map repeat-7 | `32.011M` |
| `4096..16384` external-map repeat-7 | `19.903M` |
| `1024..4096` realloc-in-place repeat-5 | `34.678M` |
| `4096..16384` realloc-in-place repeat-5 | `30.118M` |

Latest cross-allocator refresh, repeat-3, `bench_mixed_ws_crt`:

| Row | hz6 | mimalloc | tcmalloc | system | hz6 peak KB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `16..256` | `57.671M` | `53.099M` | `255.517M` | `98.761M` | `56,576` |
| `16..4096` | `39.382M` | `5.873M` | `85.177M` | `8.729M` | `156,672` |
| `1024..4096` | `38.497M` | `4.858M` | `86.491M` | `6.328M` | `181,120` |
| `4096..16384` | `27.752M` | `1.282M` | `42.086M` | `8.132M` | `185,472` |

Important caveat:

```text
tcmalloc remains much faster on these preload rows, and system malloc is still
faster on the tiny 16..256 row.  HZ6 is now clearly ahead of mimalloc on the
selected mixed_ws preload rows, but this is not a universal allocator win.
Present it as the current Ubuntu HZ6 LD_PRELOAD selected/default lane.
```

Current lane decisions:

| Lane | Status | Read |
| --- | --- | --- |
| `HZ6_TOY_ACTIVE_MAP_REGISTER_FAST_SLOT_L1` | selected | Large Toy-row win; keep default on. |
| `HZ6_TOY_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1` | candidate/default | Small `1024..4096` win; watch `16..1024`. |
| `HZ6_TOY_PRECLASSIFIED_MALLOC_L1` | no-go | Direct preclassification worsened layout/throughput. |
| `HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1` | no-go | Wrapper bypass regressed target rows. |
| `HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1` | no-go | Neutral target, broad Toy wobble. |
| `SmallRunRouteBehaviorToy-L1` | no-go | Dry-run clean, support machinery too expensive. |

`HZ6_TOY_PRECLASSIFIED_MALLOC_L1` exists as a gated control/no-go candidate,
not as selected default. It bypasses generic front-registry classification for
`size <= 4096`, but the focused `1024..4096` preload A/B regressed versus the
registry path. Keep default off unless a broader repeat reverses that result.

`HZ6_TOY_ACTIVE_MAP_REGISTER_FAST_SLOT_L1` is the current Toy high candidate.
It only short-circuits active-map registration when the hashed base slot is
empty or already matches the pointer; collision cases keep the prior bounded
probe path. Focused repeat showed the main `1024..4096` preload row improving
from `34.566M` to `39.798M ops/s`, with clean stats guards. Keep watching the
`4096..16384` guard row because it was roughly flat/slightly down in the first
short repeat.

`HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1` is a no-go control. It bypassed the
allocator frontcache-pop wrapper only in non-diagnostic direct-reuse builds, but
repeat-5 regressed `1024..4096` from `39.699M` to `36.341M ops/s`. Keep it off.

`HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1` is a no-go control. It checks the hashed
base slot before the Toy active-map free probe loop, but repeat-5 was neutral
on `1024..4096` and regressed `16..4096`; keep it off.

`HZ6_TOY_TRUSTED_ACTIVATE_SKIP_SOURCE_BLOCK_CHECK_L1` is a Toy-high
candidate/default. It skips the repeated source-block active/bounds validation
only after local frontcache state/ptr/generation checks and only for
Toy-sized descriptors. Repeat-9 showed `1024..4096` improving `1.0065x`; keep
watching the small `16..1024` wobble (`0.9955x`).

`[HZ6_PRELOAD_RUNMETA_DETAIL]` is the ToyRunLocalMeta diagnostic line. Use it
only with diagnostic builds that enable source-run reuse, source-block range
index, slot descriptor map, and `HZ6_SMALL_RUN_ROUTE_DRYRUN_L1`; it estimates
whether run-local metadata can replace exact route / descriptor source-block
work on Toy source-run frees.

First runmeta diagnostic: `1024..4096` and `16..4096` had
`smallrun_would_valid / smallrun_route_attempt = 0.9994` with
`smallrun_false_positive=0`. `4096..16384` had only `0.0001`, so any behavior
must stay Toy/source-run gated.

`SmallRunRouteBehaviorToy-L1` is no-go with the current source-run route stack.
Although dry-run was clean, enabling source-run reuse + descriptor map + range
index dropped `1024..4096` into the `35M ops/s` band; behavior on was slightly
worse than support-only. Do not promote this path without a cheaper
active-map-adjacent metadata design.

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
| MidPage run 256K | `HZ6_MIDPAGE_RUN_BYTES=262144` | 4096..16384 repeat-3 moved from `16.549M` to `19.394M` with flat RSS. |
| External MidPage active map | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1`, external, cap8192/probe2 | Repeat-7 versus no-map moved 1024..4096 from `30.962M` to `32.011M` and 4096..16384 from `18.983M` to `19.903M`. |
| Realloc in-place | `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=1` | Repeat-5 versus control-off moved 16..256 `50.810M -> 55.313M`, 16..4096 `33.867M -> 36.556M`, 1024..4096 `31.473M -> 34.678M`, and 4096..16384 `19.971M -> 30.118M`. |
| MidPage unaligned/probe4 | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1`, probe4 | 4096..16384 active-map hits moved from `3,321` to `915,393`; repeat-5 HZ4 guard reached `hz6 31.505M` vs `hz4 30.916M` with lower HZ6 RSS. |
| MidPage active-map cap16K | `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384`, probe4 | Resume diagnostic showed MidPage active-map misses on 4096..16384. Focused repeat-5 held `27.067M` versus pre-promotion default `25.908M`; cross repeat-3 held `27.752M` with about `+0.6MB` RSS. Cap32K and cap16K probe2/probe8 were weaker. |
| MidPage miss audit after cap16K | `HZ6_PRELOAD_STATS=per_allocator` | Cap16K diagnostic still showed MidPage `free_hit=397136`, `free_miss=19254`, and `free_local_route_valid=9633` on the 200K 4096..16384 guard. The remaining miss path is real, but preload-boundary shortcuts must pass tiny guards. |
| MidPageSourcePressureAudit-L1 | diagnostic-only | Next lane. Split MidPage 8K/32K alloc, run prefill, active-map register/free hits, and preload local route fallback before changing source-run or route-register behavior. Active-map miss remains aggregate because the class is not known until route fallback. Active-map internals are closed for now. |
| MidPage source pressure read | diagnostic-only | 4096..16384 is 32K dominated: `midpage_32k_alloc_call=271026`, `midpage_8k_alloc_call=352`, `midpage_32k_prefill_run_call=2769`, `midpage_8k_prefill_run_call=352` on the 200K guard. |
| MidPage route-register class split | diagnostic-only | Source-run-slot route registration is visible now: on the 500K 4096..16384 guard, `route_register_reason_source_run_slot=17320`, split as `8K=5760` and `32K=11224`. This is much smaller than the 1M-class active-map register/free path, so do not chase source-run-slot route registration first. |

## Selected Controls

Keep these controls available when changing the preload lane:

| Control | Why |
| --- | --- |
| no MidPage active map | Direct control for `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2`. |
| internal MidPage active map | MidPage-only upper-bound; repeat-7 4096..16384 reached `20.504M`, but balanced locality was worse than external storage. |
| MidPage run 512K | MidPage-specialized source-churn upper-bound; improves 4096..16384 but regresses 16..4096. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=0` | Direct control for MidPage active-map 8K-alignment gating. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=8192` | Previous balanced default and direct control for the selected cap16K promotion. |
| `HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=2` | Balanced control for the selected probe4 MidPage active map. Probe2 is slightly better on 1024..4096 but weaker on the HZ4-close 4096..16384 target. |
| frontcache 16384 | Boundary for frontcache8192; flat enough not to promote. |
| route table 262144 | Capacity upper-bound; not selected by current evidence. |
| `HZ6_PRELOAD_REALLOC_IN_PLACE_L1=0` | Direct control for preload realloc in-place. |

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
| `HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1=1` | Preserving existing entries on full probe looked plausible, but target 4096..16384 regressed to about `24.7M` and diagnostic free miss increased slightly; keep current base-slot overwrite policy. |
| `HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1` | Toy-style register fast-slot worsened guards and target rows; 4096..16384 fell to about `24.2M`. Keep the current bounded loop shape for MidPage. |
| `HZ6_MIDPAGE_32K_RUN_BYTES=524288` | Cut source pressure (`source_alloc 3189 -> 1806`, `midpage_32k_prefill_run_call 2769 -> 1386`) but regressed 4096..16384 to about `24.5M`; source churn is real, but thicker 32K runs lose locality. |
| `HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1=1` | Class-salted MidPage active-map index did not reduce collision pressure in diagnostic (`register_collision 369139 -> 373449`, `free_miss 3632 -> 4624`) and regressed 1024..4096 in the first A/B. Keep off. |
| `HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1=1` | Avoiding post-prefill exact route lookup by prefill/direct-pop retry is too disruptive: first repeat-3 dropped 4096..16384 from about `29.5M` to about `0.52M` and raised RSS. Keep off. |
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
OUT_DIR=$PWD/hakozuna-hz6/out/linux/hz6_preload_diag \
HZ6_EXTRA_CFLAGS='-DHZ6_DIAGNOSTIC_PROBES=1 -DHZ6_TOY_SMALL_HOTPATH_DIAG_L1=1' \
./hakozuna-hz6/linux/build_hz6_preload.sh
```

## Raw Evidence

Recent raw evidence directories:

```text
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
