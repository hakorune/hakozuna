# Allocator Line Integrated Snapshot With HZ10 Bump Default

Status: current public summary snapshot after `HZ10BumpDefaultGate-L1`.

This table keeps the non-HZ10 columns from the same-run integrated matrix
`20260707_allocator_line_integrated_hz3_hz4_hz8_hz10_r10`, and updates the HZ10
column to the promoted default lazy page-init lane from
`20260707_hz10_bump_init_l0_r10` (`hz10_bump`, now the default HZ10 behavior).
Use this as the current README-facing snapshot. Use the source result
directories for raw samples.

## Conditions

```text
date_utc=2026-07-07
base_matrix=docs/benchmarks/20260707_allocator_line_integrated_hz3_hz4_hz8_hz10_r10/
hz10_default_update=docs/benchmarks/20260707_hz10_bump_init_l0_r10/
RUNS=10
THREADS=16
ITERS=50000
rows=guard_local0,main_local0,main_interleaved_r50,main_interleaved_r90,small_interleaved_remote90,medium_interleaved_r50
```

## Median Ops/s

| row | hz3 | hz4 | hz8 | hz10 default | mimalloc | tcmalloc |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `guard_local0` | 156.85M | 49.01M | 207.05M | 240.26M | 88.23M | 354.64M |
| `main_local0` | 149.31M | 28.82M | 117.94M | 208.94M | 21.32M | 367.54M |
| `main_interleaved_r50` | 16.86M | 12.28M | 10.84M | 21.68M | 5.63M | 22.22M |
| `main_interleaved_r90` | 10.20M | 9.75M | 7.04M | 13.13M | 4.28M | 13.90M |
| `small_interleaved_remote90` | 12.95M | 11.13M | 14.70M | 15.44M | 13.19M | 26.54M |
| `medium_interleaved_r50` | 15.43M | 8.68M | 9.84M | 19.24M | 4.20M | 16.76M |

## Post RSS

| row | hz3 | hz4 | hz8 | hz10 default | mimalloc | tcmalloc |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `guard_local0` | 12.35MiB | 131.11MiB | 2.00MiB | 3.25MiB | 13.33MiB | 7.00MiB |
| `main_local0` | 15.11MiB | 169.99MiB | 3.50MiB | 4.88MiB | 67.89MiB | 9.00MiB |
| `main_interleaved_r50` | 163.77MiB | 258.10MiB | 4.33MiB | 64.00MiB | 190.12MiB | 68.50MiB |
| `main_interleaved_r90` | 205.82MiB | 275.23MiB | 4.62MiB | 71.06MiB | 234.37MiB | 82.88MiB |
| `small_interleaved_remote90` | 130.92MiB | 144.87MiB | 2.95MiB | 35.06MiB | 56.82MiB | 31.62MiB |
| `medium_interleaved_r50` | 148.37MiB | 237.98MiB | 3.83MiB | 65.06MiB | 191.08MiB | 85.12MiB |

## Interpretation

- HZ10 default is now materially stronger than the old HZ10 no-bump column on
  local fixed-cost rows: `guard_local0` and `main_local0` now exceed HZ8
  throughput while keeping RSS close to HZ8 and below tcmalloc/mimalloc on
  `main_local0`.
- HZ8 remains the public recommended balanced low-RSS line: it still has the
  lowest HZ post RSS on every row in this table.
- tcmalloc remains the raw-throughput leader on most local/main rows. Hakozuna's
  public positioning should stay RSS discipline, fail-closed ownership, and
  explicit recovery boundaries rather than universal throughput replacement.
- HZ10 is the active speed/RSS-aware research line. It is now a much stronger
  comparator on local and main interleaved rows, but its remote/interleaved RSS
  is still tens of MiB.
