# HZ8 MT Lane x Remote% Snapshot

Status: HZ8 paper/supporting measurement snapshot.

This document fixes the HZ8 values that were missing from the MT lane x remote%
comparison. It keeps the claim modest:

```text
HZ8 is a balanced low-RSS allocator with practical throughput.
Do not claim universal tcmalloc replacement.
```

## Measurement Sources

```text
main_r0 / main_r50 / main_r90 / guard_r0:
  hakozuna-hz8/bench_results/20260630T215340Z_hz8_mt_matrix/summary.md

cross128_r90:
  hakozuna-hz8/bench_results/20260701T000000Z_cross128_full/summary.md
```

## Table

Median ops/s, `RUNS=10`, `T=16`.

| Lane | HZ8 median ops/s | HZ8 p25 ops/s | HZ8 p75 ops/s | HZ8 median post RSS | HZ8 median peak RSS | n_ok | n_fail |
|---|---:|---:|---:|---:|---:|---:|---:|
| `main_r0` | 107.633M | 105.742M | 111.029M | 9.00 MiB | 9.00 MiB | 10 | 0 |
| `main_r50` | 29.633M | 27.445M | 30.823M | 9.83 MiB | 25.50 MiB | 10 | 0 |
| `main_r90` | 20.610M | 19.097M | 22.070M | 9.93 MiB | 29.88 MiB | 10 | 0 |
| `guard_r0` | 224.750M | 198.357M | 233.275M | 7.94 MiB | 7.94 MiB | 10 | 0 |
| `cross128_r90` | 37.342k | 37.127k | 37.405k | 151.40 MiB | 196.85 MiB | 10 | 0 |

## Interpretation

```text
main_r0:
  HZ8 is practically useful and stays low-RSS.

main_r50 / main_r90:
  HZ8 remains balanced under remote pressure, but tcmalloc is still faster.

guard_r0:
  HZ8 stays strong on the tight local guard lane.

cross128_r90:
  HZ8 completes the full cross128 row; use the full row rather than the
  earlier probe when comparing against other allocators.
```
