# HZ11 Macro Speed Lane Gate L1

Status: measured.
Verdict: NO-GO for macro speed-lane promotion.

## Purpose

Verify whether `libhz11_span_transfer.so` is a real HZ11 macro speed-lane
candidate, not only a `bench_matrix_malloc` remote/mixed winner.

This box is measurement only. It does not change allocator hot paths and does
not default HZ11.

## Script

```bash
RUNS=5 ./hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh
```

Output:

```text
bench_results/hz11_macro_speed_lane_<timestamp>/
  README.log
  samples.csv
  summary.md
  per-run stdout/stderr/log files
```

## Allocators

```text
glibc
tcmalloc
mimalloc if available
hz11-span-soa
hz11-span-transfer
```

Missing optional allocator libraries are recorded as `SKIP` rows instead of
failing the whole matrix.

## Workloads

Minimum rows:

```text
python_alloc
larson
xmalloc_test
sh6bench if available
```

Optional local rows, run when the existing binaries are found:

```text
cache_scratch
mstress
```

The script uses existing local binaries only. It does not fetch dependencies.

## RSS Sampling

The script samples `/proc/<pid>/status` while the workload process is alive:

```text
VmHWM -> max_rss_kb
VmRSS -> current_rss_kb, using the last observed value before exit
```

Fast-exiting workloads may report `current_rss_kb=NA`. `ru_maxrss` is not used
as post/current RSS.

## GO Conditions

```text
Correctness:
  no hz11-span-transfer crashes or timeouts

Transfer vs span-soa:
  transfer wall time <= span-soa * 1.05 on at least 3/4 available rows

Transfer vs tcmalloc:
  at least one macro row wins on wall time, OR
  wall time <= tcmalloc * 1.25 while max RSS is clearly lower

RSS:
  transfer max RSS <= tcmalloc * 1.25 on every completed row
  transfer current RSS <= tcmalloc * 1.25 where current RSS is measurable

Transfer counters:
  multi-thread/churn rows should show xfer_hit > 0, or the result must explain
  why transfer is not expected to fire
```

## NO-GO Conditions

```text
Any correctness crash in hz11-span-transfer
xmalloc_test or larson hangs or aborts
RSS exceeds tcmalloc * 1.25 on multiple rows
macro wall time regresses by >10% vs span-soa on most rows
transfer cache is unused in all workloads where it should be used
```

## Result

Run:

```bash
RUNS=5 ./hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh
```

Output:

```text
bench_results/hz11_macro_speed_lane_20260707T222023Z/
```

Conditions:

```text
RUNS=5
allocators=glibc,tcmalloc,mimalloc,hz11-span-soa,hz11-span-transfer
workloads=python_alloc,larson,xmalloc_test,sh6bench,cache_scratch,mstress
```

Summary:

| Workload | tcmalloc wall | span-soa wall | span-transfer wall | transfer/tcmalloc wall | transfer/tcmalloc max RSS | transfer status |
|---|---:|---:|---:|---:|---:|---|
| python_alloc | 0.032s | 0.032s | NA | NA | NA | FAIL 5/5, rc=134 |
| larson | 4.147s | 4.181s | 4.177s | 1.007 | 2.344 | OK 5/5 |
| xmalloc_test | 2.054s | 2.021s | 2.025s | 0.986 | 0.083 | OK 5/5 |
| sh6bench | 0.356s | 29.886s* | 4.645s | 13.048 | 1.336 | OK 5/5 |
| cache_scratch | 1.174s | 1.173s | 1.177s | 1.003 | 0.439 | OK 5/5 |
| mstress | 0.189s | 0.243s | NA | NA | NA | FAIL 5/5, rc=134 |

`*` `hz11-span-soa` sh6bench had 1 OK run and 4 timeouts; transfer completed all
5 runs, but remained far slower than tcmalloc and exceeded the tcmalloc RSS
guard.

Gate:

| Check | Result | Detail |
|---|---|---|
| No transfer crashes/timeouts | FAIL | `python_alloc` 5/5 abort, `mstress` 5/5 abort |
| Transfer within 5% of span-soa on >=3/4 available rows | PASS | 4/4 rows within 1.05x |
| tcmalloc macro win or <=1.25x with lower RSS | PASS | wins `xmalloc_test`; lower-RSS near-parity `cache_scratch`, `xmalloc_test` |
| max RSS <= tcmalloc * 1.25 | FAIL | `larson=2.344`, `sh6bench=1.336` |
| current RSS <= tcmalloc * 1.25 | FAIL | `larson=2.344`, `sh6bench=1.393` |
| transfer counters fire where expected | PASS | `larson`, `xmalloc_test`, `sh6bench`; no insert on `cache_scratch` |

Important transfer counters:

```text
larson:
  xfer_hit=1,970
  xfer_insert=31,520

xmalloc_test:
  xfer_hit=51,575,793
  xfer_insert=825,254,000

sh6bench:
  xfer_hit=52,545,492
  xfer_insert=840,739,024

cache_scratch:
  xfer_insert=0, so transfer reuse is not expected on this shape
```

Interpretation:

```text
NO-GO for macro speed-lane promotion.

The transfer lane remains valuable as a remote/mixed microbench lane, and it is
excellent on xmalloc_test RSS and wall time. It is not yet a macro speed lane:
python_alloc and mstress abort, while larson/sh6bench exceed the tcmalloc RSS
guard. Do not strengthen public claims or move toward default/productization
from this result.
```

Abort attribution:

```text
python_alloc:
  gdb confirms SIGABRT in hz11_central_stack_insert_range()
  class_id=2
  caller: hz11_thread_cache_flush_class -> hz11_thread_cache_push_overflow_slow

mstress:
  gdb confirms SIGABRT in hz11_central_stack_insert_range()
  class_id=0
  caller: hz11_thread_cache_flush_class -> hz11_thread_cache_push_overflow_slow
```

This is the intended fail-fast path for L1 central stack capacity overflow, not
a silent heap corruption. The next box should treat it as a central policy/cap
failure: the bounded object stack is too small or the lane needs a real
CentralFreeList/span-return policy before macro promotion.

Next box:

```text
HZ11MacroFailureAttribution-L1:
  diagnose python_alloc/mstress aborts first, then larson/sh6bench RSS.
  Keep transfer cap/batch tuning deferred until correctness is stable.
```
