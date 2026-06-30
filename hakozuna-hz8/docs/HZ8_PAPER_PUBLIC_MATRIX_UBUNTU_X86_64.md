# HZ8 Paper Data Snapshot

Status: paper input / public matrix snapshot.

This document is the paper-facing Ubuntu/Linux x86_64 benchmark snapshot for
the current HZ8 release line. It keeps the claim modest:

```text
HZ8 is a balanced low-RSS allocator with practical throughput.
Do not claim universal tcmalloc replacement.
```

## Naming note

The public matrix runner keeps `hz8_keeprefill` as an explicit compatibility
alias. In this repository snapshot, the current runner also notes that `hz8`
already includes KeepRefill. For strict version-lineage text, cite the frozen
HZ8 v1.1 RC record and the separate KeepRefill control record:

```text
HZ8 v1.1 baseline:
  hakozuna-hz8/docs/HZ8_MEDIUM_RUN_V1_1_RC.md

KeepRefill control record:
  hakozuna-hz8/docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
```

## Environment

```text
allocator_behavior_sha:
  e6307d8beb71de92156290b028a2ce98ee67827c

matrix record:
  hakozuna-hz8/bench_results/20260630T191745Z_hz8_keeprefill_public_matrix/

date_utc:
  2026-06-30T19:17:45Z

platform:
  Ubuntu 22.04.5 LTS
  Linux 6.8.0-90-generic
  x86_64

cpu:
  AMD Ryzen 7 5825U with Radeon Graphics
  16 CPUs

compiler:
  gcc 11.4.0
  clang 14.0.0
  ld 2.38

allocator DSOs:
  MIMALLOC_SO=/mnt/workdisk/public_share/hakmem/allocators/mimalloc/libmimalloc.so
  TCMALLOC_SO=/mnt/workdisk/public_share/hakmem/allocators/tcmalloc/libtcmalloc_minimal.so
```

## Exact Command

```bash
TMPDIR=/mnt/workdisk/public_share/hakozuna_repo/hakozuna-hz8/bench/tmp \
MIMALLOC_SO=/mnt/workdisk/public_share/hakmem/allocators/mimalloc/libmimalloc.so \
TCMALLOC_SO=/mnt/workdisk/public_share/hakmem/allocators/tcmalloc/libtcmalloc_minimal.so \
RUNS=10 THREADS=16 ITERS=50000 \
ALLOCATORS=hz8,hz8_keeprefill,mimalloc,tcmalloc,system \
ROWS=small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50,guard_local0,main_local0,medium_local0 \
bash hakozuna-hz8/scripts/run_hz8_keeprefill_public_matrix.sh
```

## Public Matrix

All combinations in this snapshot have `n_ok = 10` and `n_fail = 0`.

### small_interleaved_remote90

| Allocator | median ops/s | p25 ops/s | p75 ops/s | peak RSS median | post RSS median | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `hz8` | 6.691M | 6.495M | 6.767M | 590.31 MiB | 5.21 MiB | 10 | 0 |
| `hz8_keeprefill` | 12.023M | 11.909M | 12.176M | 28.81 MiB | 2.91 MiB | 10 | 0 |
| `mimalloc` | 10.960M | 7.186M | 11.593M | 51.88 MiB | 50.98 MiB | 10 | 0 |
| `tcmalloc` | 23.900M | 23.674M | 24.243M | 33.06 MiB | 32.94 MiB | 10 | 0 |
| `system` | 5.928M | 5.805M | 6.019M | 16.25 MiB | 3.96 MiB | 10 | 0 |

### main_interleaved_r90

| Allocator | median ops/s | p25 ops/s | p75 ops/s | peak RSS median | post RSS median | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `hz8` | 5.792M | 5.551M | 6.070M | 70.06 MiB | 4.48 MiB | 10 | 0 |
| `hz8_keeprefill` | 6.048M | 5.483M | 6.390M | 67.31 MiB | 4.57 MiB | 10 | 0 |
| `mimalloc` | 4.715M | 3.412M | 6.005M | 192.34 MiB | 183.12 MiB | 10 | 0 |
| `tcmalloc` | 12.178M | 11.641M | 12.747M | 90.38 MiB | 90.31 MiB | 10 | 0 |
| `system` | 2.804M | 2.666M | 2.931M | 85.95 MiB | 3.93 MiB | 10 | 0 |

### medium_interleaved_r50

| Allocator | median ops/s | p25 ops/s | p75 ops/s | peak RSS median | post RSS median | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `hz8` | 8.226M | 7.691M | 8.532M | 70.19 MiB | 3.72 MiB | 10 | 0 |
| `hz8_keeprefill` | 8.128M | 7.816M | 8.842M | 61.00 MiB | 3.81 MiB | 10 | 0 |
| `mimalloc` | 4.151M | 3.350M | 4.820M | 181.92 MiB | 162.54 MiB | 10 | 0 |
| `tcmalloc` | 15.870M | 15.538M | 16.295M | 79.19 MiB | 79.06 MiB | 10 | 0 |
| `system` | 1.960M | 1.941M | 1.976M | 41.75 MiB | 3.26 MiB | 10 | 0 |

### guard_local0

| Allocator | median ops/s | p25 ops/s | p75 ops/s | peak RSS median | post RSS median | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `hz8` | 186.011M | 176.951M | 195.403M | 2.25 MiB | 2.12 MiB | 10 | 0 |
| `hz8_keeprefill` | 188.108M | 179.814M | 196.301M | 2.25 MiB | 2.12 MiB | 10 | 0 |
| `mimalloc` | 93.938M | 29.380M | 132.487M | 7.78 MiB | 7.71 MiB | 10 | 0 |
| `tcmalloc` | 266.772M | 246.456M | 270.506M | 7.12 MiB | 7.00 MiB | 10 | 0 |
| `system` | 143.426M | 135.885M | 146.139M | 1.62 MiB | 1.50 MiB | 10 | 0 |

### main_local0

| Allocator | median ops/s | p25 ops/s | p75 ops/s | peak RSS median | post RSS median | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `hz8` | 92.142M | 88.848M | 95.049M | 3.50 MiB | 3.31 MiB | 10 | 0 |
| `hz8_keeprefill` | 95.441M | 92.258M | 100.176M | 3.44 MiB | 3.31 MiB | 10 | 0 |
| `mimalloc` | 26.474M | 23.758M | 29.075M | 67.28 MiB | 67.25 MiB | 10 | 0 |
| `tcmalloc` | 272.563M | 262.035M | 287.547M | 9.00 MiB | 8.88 MiB | 10 | 0 |
| `system` | 118.141M | 117.406M | 119.709M | 1.88 MiB | 1.75 MiB | 10 | 0 |

### medium_local0

| Allocator | median ops/s | p25 ops/s | p75 ops/s | peak RSS median | post RSS median | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `hz8` | 84.990M | 83.724M | 91.634M | 2.94 MiB | 2.75 MiB | 10 | 0 |
| `hz8_keeprefill` | 87.564M | 81.665M | 93.203M | 3.06 MiB | 2.87 MiB | 10 | 0 |
| `mimalloc` | 27.224M | 24.698M | 30.933M | 54.44 MiB | 54.22 MiB | 10 | 0 |
| `tcmalloc` | 277.076M | 262.989M | 297.366M | 9.12 MiB | 9.06 MiB | 10 | 0 |
| `system` | 116.179M | 113.268M | 117.942M | 1.81 MiB | 1.69 MiB | 10 | 0 |

## KeepRefill Comparison Note

The compatibility alias in this snapshot shows the expected remote-heavy
improvement over the same snapshot's baseline label:

```text
small_interleaved_remote90:
  hz8             6.691M / peak 590.31 MiB
  hz8_keeprefill  12.023M / peak 28.81 MiB

main_interleaved_r90:
  hz8             5.792M / peak 70.06 MiB
  hz8_keeprefill  6.048M / peak 67.31 MiB

medium_interleaved_r50:
  hz8             8.226M / peak 70.19 MiB
  hz8_keeprefill  8.128M / peak 61.00 MiB
```

For the paper, keep the claim modest:

```text
KeepRefill removes the prior remote-heavy cliff and keeps RSS low on the
reported remote-heavy rows.
```

If you need a strict v1.1 baseline citation, use the frozen RC record:

```text
hakozuna-hz8/docs/HZ8_MEDIUM_RUN_V1_1_RC.md
```

If you need the direct KeepRefill cliff-control record, use:

```text
hakozuna-hz8/docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
```

## Paper Claim

Use this wording:

```text
HZ8 is a balanced low-RSS allocator with practical throughput.
It emphasizes fail-closed ownership, bounded retained-empty residency, and
cross-thread free correctness. The HZ8-v2 KeepRefill default removes the prior
remote-heavy cliff while staying far below mimalloc RSS on the reported
remote-heavy rows.
```

Do not use these claims:

```text
HZ8 universally beats tcmalloc.
HZ8 is the fastest allocator on every local-only row.
HZ8 replaces all earlier HZ lines as evidence.
Windows HZ8 has Linux parity.
```

## Supporting MT Lane

If you need the missing MT lane x remote% values for HZ8 itself, use:

```text
hakozuna-hz8/docs/HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md
```
