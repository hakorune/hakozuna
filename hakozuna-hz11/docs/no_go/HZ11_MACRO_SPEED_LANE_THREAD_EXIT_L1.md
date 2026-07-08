# HZ11MacroSpeedLaneGateWithThreadExit-L1

Status: measured.
Verdict: NO-GO for macro speed-lane promotion.

This box re-runs the macro promotion gate after
`HZ11CurrentSpanPoolThreadExit-L1`. The question is whether the thread-exit
current-span pool fixes the macro lane broadly enough to promote a new HZ11
speed lane.

## Boundary

```text
Candidate:
  libhz11_span_transfer_thread_exit.so
  HZ11_CURRENT_SPAN_THREAD_EXIT=1

Compare:
  tcmalloc
  hz11-span-transfer
  hz11-thread-exit

Runner:
  scripts/run_hz11_macro_speed_lane_gate.sh

Command:
  scripts/run_hz11_macro_speed_lane_gate.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit \
    --candidate hz11-thread-exit \
    --skip-span-soa-check \
    --runs 5

Output:
  bench_results/hz11_macro_speed_lane_20260708T001420Z/summary.md
```

No default lane changes are made. The macro runner now accepts
`hz11-thread-exit` as an allocator and can gate a selected HZ11 candidate.

## Results

RUNS=5.

| Workload | tcmalloc wall | transfer wall | thread-exit wall | thread-exit/tcmalloc wall | thread-exit/tcmalloc max RSS | thread-exit status |
|---|---:|---:|---:|---:|---:|---|
| python_alloc | 0.032 | NA | NA | NA | NA | FAIL:5 |
| mstress | 0.191 | NA | NA | NA | NA | FAIL:5 |
| larson | 4.145 | 4.180 | 4.132 | 0.997 | 0.979 | OK:5 |
| sh6bench | 0.347 | 4.602 | 4.567 | 13.161 | 1.307 | OK:5 |
| xmalloc_test | 2.041 | 2.017 | 2.024 | 0.992 | 0.080 | OK:5 |
| cache_scratch | 1.173 | 1.182 | 1.188 | 1.013 | 0.464 | OK:5 |

Gate checks:

```text
correctness: FAIL
  python_alloc FAIL:5
  mstress FAIL:5

RSS: FAIL
  sh6bench max RSS 1.307x tcmalloc
  sh6bench current RSS 1.315x tcmalloc

wall: FAIL for macro promotion
  sh6bench wall 13.161x tcmalloc
```

Positive results:

```text
larson:
  thread-exit fixes the previous RSS failure:
    transfer RSS 653952 KiB
    thread-exit RSS 273280 KiB
    tcmalloc RSS 279168 KiB
  thread-exit wall is near tcmalloc and slightly faster than transfer.

xmalloc_test:
  thread-exit remains near tcmalloc wall with much lower RSS.

cache_scratch:
  near tcmalloc wall with lower RSS.
```

The thread-exit pool counters confirm the larson fix is still active:

```text
larson:
  span_create 19..24
  current_span_pool drop=0
```

## Decision

NO-GO for macro promotion.

`HZ11CurrentSpanPoolThreadExit-L1` fixed the larson thread-churn RSS root cause,
but it does not fix the previous correctness rows and it does not address
sh6bench. Keep `libhz11_span_transfer_thread_exit.so` as an opt-in diagnostic /
candidate lane only.

Next work should return to the unresolved central policy problem:

```text
python_alloc / mstress:
  central capacity / span-return policy still fails fast.

sh6bench:
  transfer path remains far slower than tcmalloc and exceeds the RSS guard.
  The previous attribution showed span-return's extra wall time came from
  metadata-lock traffic, not sweep/O(n).
```
