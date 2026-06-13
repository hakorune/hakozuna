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
HZ6_MIDPAGE_RUN_BYTES=262144
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2=1
HZ6_LINUX_MMAP_RETAIN_L1=1
HZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_L1=1
HZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=128
HZ6_ROUTE_TOMBSTONE_COMPACT_L1=1
HZ6_ROUTE_HASH_XOR_FOLD_L1=1
HZ6_ROUTE_LINEAR_WRAP_L1=1
HZ6_ROUTE_LOOP_CARRY_L1=1
```

## Current Read

The preload lane is a real Ubuntu performance lane, but it remains separate
from the direct HZ6 API and Windows selected-family rows.

Latest selected shape:

| Row | Selected read |
| --- | ---: |
| `16..256` focused cross | `hz6 53.883M` vs `mimalloc 52.656M` |
| `16..4096` focused cross | `hz6 34.559M` vs `mimalloc 6.027M` |
| `1024..4096` external-map repeat-7 | `32.011M` |
| `4096..16384` external-map repeat-7 | `19.903M` |

Important caveat:

```text
tcmalloc remains much faster on the broad preload rows.  Do not present this
lane as a universal allocator win; present it as the current Ubuntu HZ6
LD_PRELOAD selected/default lane.
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

## Selected Controls

Keep these controls available when changing the preload lane:

| Control | Why |
| --- | --- |
| no MidPage active map | Direct control for `HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2`. |
| internal MidPage active map | MidPage-only upper-bound; repeat-7 4096..16384 reached `20.504M`, but balanced locality was worse than external storage. |
| MidPage run 512K | MidPage-specialized source-churn upper-bound; improves 4096..16384 but regresses 16..4096. |
| frontcache 16384 | Boundary for frontcache8192; flat enough not to promote. |
| route table 262144 | Capacity upper-bound; not selected by current evidence. |

## No-Go / Evidence-Only

| Lane | Read |
| --- | --- |
| Toy-map widening to MidPage | Reduced route probes but created too many map misses/collisions and slightly regressed focused repeat; do not widen the Toy active map. |
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
```

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
