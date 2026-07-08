# HZ11MacroSpeedLaneGateWithFine128-L1

Status: MIXED. No macro promotion.

Keep `fine128` as an opt-in candidate pending current-RSS gate resolution.

## Box

Run the full macro promotion gate for the selected selective fineclass
candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

Boundary:

```text
gate only
no policy changes
fine128 remains opt-in
global fineclass is comparison context only
```

Compared:

```text
tcmalloc
hz11-thread-exit-cap-batch32
hz11-thread-exit-cap-batch32-fine128
hz11-thread-exit-cap-batch32-fineclass
```

## Run

Command:

```bash
RUNS=5 \
  hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh \
    --allocators tcmalloc,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fine128,hz11-thread-exit-cap-batch32-fineclass \
    --candidate hz11-thread-exit-cap-batch32-fine128 \
    --skip-span-soa-check --runs 5
```

Output:

```text
bench_results/hz11_macro_speed_lane_20260708T083138Z/summary.md
```

Gate verdict:

```text
MIXED
```

## Result

| Workload | tcmalloc wall | fine128 wall | fine128/tcmalloc wall | tcmalloc max RSS KiB | fine128 max RSS KiB | fine128/tcmalloc max RSS | fine128 current RSS KiB |
|---|---:|---:|---:|---:|---:|---:|---:|
| python_alloc | 0.032 | 0.032 | 1.000 | 7936 | 6528 | 0.823 | 3072 |
| mstress | 0.192 | 0.221 | 1.151 | 217088 | 235996 | 1.087 | 177664 |
| larson | 4.152 | 4.140 | 0.997 | 278912 | 273024 | 0.979 | 273024 |
| sh6bench | 0.349 | 3.532 | 10.120 | 259584 | 323712 | 1.247 | 323968 |
| xmalloc_test | 2.047 | 2.027 | 0.990 | 195456 | 18432 | 0.094 | 18432 |
| cache_scratch | 1.163 | 1.177 | 1.012 | 7296 | 3456 | 0.474 | 3328 |

## Checks

Passed:

```text
python_alloc OK 5/5
mstress OK 5/5
larson RSS fix holds: 273024 KiB vs tcmalloc 278912 KiB
xmalloc_test remains a strong RSS/wall win
cache_scratch remains inside the gate shape
sh6bench max RSS is just inside the 1.25x guard: 1.247x
sh6bench wall does not regress versus batch32: 3.532s vs 3.582s
```

Failed:

```text
current RSS gate:
  python_alloc 1.714x tcmalloc current RSS
  sh6bench     1.313x tcmalloc current RSS
```

`python_alloc` current RSS should be interpreted with
`HZ11FineclassPythonAllocCurrentRSS-L1`: the prior fineclass current-RSS miss
was diagnosed as a tiny-denominator sampling artifact, not a steady resident
regression.

The sh6bench current-RSS failure is not the same tiny-denominator issue. The
max-RSS guard passes, but tcmalloc current RSS is sampled lower than its max RSS
while fine128 current RSS tracks its max. That means this gate does not justify
macro promotion until the sh6bench current-RSS rule or sampling behavior is
resolved.

## Decision

Do not promote fine128 to macro candidate/default from this gate.

Keep:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

as the best current sh6bench RSS candidate:

```text
batch32 sh6bench max RSS   351360 KiB
fine128 sh6bench max RSS   323712 KiB
global fineclass max RSS   302080 KiB
```

Next work should focus on the current-RSS gate semantics for sh6bench and
python_alloc, or on a follow-up gate that reports max/current sampling stability
before making a promotion decision.
