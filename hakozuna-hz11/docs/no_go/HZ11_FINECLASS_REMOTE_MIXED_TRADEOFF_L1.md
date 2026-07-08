# HZ11FineclassRemoteMixedTradeoff-L1

Status: NO-GO for global fineclass as a general remote/mixed candidate.

Keep fineclass as an opt-in sh6bench RSS research lane.

## Box

Decide whether the global fineclass lane is acceptable as the next candidate
lane despite remote/mixed tradeoffs.

Compared:

```text
tcmalloc
hz11-span-transfer
hz11-thread-exit-cap-batch32
hz11-thread-exit-cap-batch32-fineclass
```

Rows:

```text
main_local0
main_r50
main_r90
small_remote90
medium_r50
medium_r90
```

Boundary:

```text
measurement only
no size-class policy change
no macro promotion claim
```

## Run

Command:

```bash
RUNS=10 THREADS=16 ITERS=100000 \
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fineclass
```

Output:

```text
bench_results/hz11_transfer_promotion_20260708T080416Z/summary.md
```

The transfer promotion matrix runner was extended to:

```text
accept hz11-thread-exit-cap-batch32 as an allocator alias
emit span_create in samples and summary
emit extra allocator tradeoff ratios for candidate lanes
avoid span-soa gate checks when span-soa is intentionally not in the allocator set
```

## Summary

Fineclass versus `hz11-span-transfer`:

| Row | ops ratio | post RSS ratio | span_create ratio |
|---|---:|---:|---:|
| main_local0 | 0.893 | 3.800 | 4.105 |
| main_r50 | 0.912 | 1.436 | 1.880 |
| main_r90 | 0.943 | 1.388 | 1.780 |
| small_remote90 | 0.941 | 2.906 | 3.864 |
| medium_r50 | 0.903 | 1.329 | 1.365 |
| medium_r90 | 0.983 | 0.967 | 0.891 |

Fineclass versus `hz11-thread-exit-cap-batch32`:

| Row | ops ratio | post RSS ratio |
|---|---:|---:|
| main_local0 | 1.005 | 3.562 |
| main_r50 | 0.944 | 1.297 |
| main_r90 | 0.989 | 1.231 |
| small_remote90 | 0.948 | 2.394 |
| medium_r50 | 0.969 | 1.071 |
| medium_r90 | 0.991 | 1.031 |

Key raw medians:

```text
main_r50:
  span-transfer 85.23M ops/s, 28.69 MiB post RSS
  batch32       82.34M ops/s, 31.75 MiB post RSS
  fineclass     77.72M ops/s, 41.19 MiB post RSS

small_remote90:
  span-transfer 65.71M ops/s, 7.31 MiB post RSS
  batch32       65.23M ops/s, 8.88 MiB post RSS
  fineclass     61.84M ops/s, 21.25 MiB post RSS

medium_r90:
  span-transfer 52.84M ops/s, 62.38 MiB post RSS
  batch32       52.42M ops/s, 58.50 MiB post RSS
  fineclass     51.94M ops/s, 60.31 MiB post RSS
```

## Interpretation

Global fineclass is not a clean general candidate:

```text
throughput is lower than span-transfer on every matrix row
main_local0/main_r50/medium_r50 lose about 9-11% throughput versus span-transfer
small_remote90 post RSS is 2.9x span-transfer and 2.4x batch32
main_local0 span_create is 4.1x span-transfer
main_r50/main_r90 span_create rises about 1.8-1.9x
```

It still stays below tcmalloc RSS on these rows and remains much faster than
tcmalloc on remote-heavy medium rows, but the tradeoff is not isolated to
sh6bench. The global fineclass map increases footprint and span creation in
remote/mixed rows where sh6bench's class-packing improvement is not being
measured.

## Decision

Do not promote global fineclass as a general macro candidate yet.

Keep:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so
```

as an opt-in sh6bench RSS research lane only.

Next work should test selective fineclass ranges instead of the global
fineclass map. The goal is to preserve the sh6bench RSS win while avoiding the
remote/mixed RSS and span-create expansion seen here.
