# HZ11MacroSpeedLaneGateWithThreadExitCap-L1

Status: measured.
Verdict: NO-GO for macro speed-lane promotion.

This box re-runs the full macro gate after
`HZ11CentralPolicyCorrectness-L1`. The candidate combines:

```text
HZ11_CURRENT_SPAN_THREAD_EXIT=1
HZ11_CENTRAL_CAP=65536
```

The question is whether the candidate clears the entrance correctness rows,
keeps the larson RSS fix, and removes the remaining macro blockers.

## Boundary

```text
Candidate:
  libhz11_span_transfer_thread_exit_cap.so

Compare:
  tcmalloc
  hz11-span-transfer
  hz11-thread-exit
  hz11-thread-exit-cap

Runner:
  scripts/run_hz11_macro_speed_lane_gate.sh

Command:
  scripts/run_hz11_macro_speed_lane_gate.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit,hz11-thread-exit-cap \
    --candidate hz11-thread-exit-cap \
    --skip-span-soa-check \
    --runs 5

Output:
  bench_results/hz11_macro_speed_lane_20260708T003638Z/summary.md
```

No default lane changes are made.

## Results

RUNS=5.

| Workload | tcmalloc wall | candidate wall | candidate/tcmalloc wall | candidate/tcmalloc max RSS | candidate status |
|---|---:|---:|---:|---:|---|
| python_alloc | 0.033 | 0.032 | 0.970 | 0.735 | OK:5 |
| mstress | 0.192 | 0.221 | 1.151 | 1.125 | OK:5 |
| larson | 4.152 | 4.139 | 0.997 | 0.979 | OK:5 |
| xmalloc_test | 2.052 | 2.021 | 0.985 | 0.083 | OK:5 |
| cache_scratch | 1.176 | 1.188 | 1.010 | 0.456 | OK:5 |
| sh6bench | 0.355 | 4.561 | 12.848 | 1.358 | OK:5 |

Good:

```text
python_alloc and mstress now pass 5/5.
larson RSS fix holds:
  transfer: 653824 KiB
  thread-exit-cap: 273152 KiB
  tcmalloc: 278912 KiB
xmalloc_test and cache_scratch remain within the gate shape with much lower RSS.
```

Bad:

```text
sh6bench remains a macro blocker:
  wall:    12.848x tcmalloc
  max RSS: 1.358x tcmalloc
  current RSS: 1.306x tcmalloc
```

The generic summary verdict is `MIXED` because the candidate has no
crashes/timeouts and passes several rows. For macro promotion, this is a
NO-GO because the decision rule requires sh6bench wall/RSS to stop blocking.

## Decision

NO-GO for macro promotion.

The candidate clears the previous correctness entrance failures and preserves
the larson current-span fix. The remaining primary blocker is sh6bench. The
next optimization box should target the measured sh6bench cause:

```text
HZ11SpanReturnMetadataBatch-L1
  remove or batch per-object metadata locking in span-return, or otherwise
  reduce sh6bench transfer/central path cost without regressing RSS.
```

Keep `libhz11_span_transfer_thread_exit_cap.so` as an opt-in candidate lane
only. A fixed 65536 central cap is still not a final default central policy.
