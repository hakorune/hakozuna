# HZ11MacroSpeedLaneGateWithBatch32-L1

Status: measured.
Verdict: NO-GO for macro speed-lane promotion.

This box reruns the macro gate with the sh6bench batch-granularity candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32.so
HZ11_CURRENT_SPAN_THREAD_EXIT=1
HZ11_CENTRAL_CAP=65536
HZ11_TRANSFER_BATCH=32
```

The question is whether `batch32` keeps the previously fixed correctness and
larson rows while improving sh6bench enough to stop blocking macro promotion.

## Boundary

```text
Compare:
  tcmalloc
  hz11-thread-exit-cap
  hz11-thread-exit-cap-batch32

Runner:
  scripts/run_hz11_macro_speed_lane_gate.sh

Command:
  RUNS=5 scripts/run_hz11_macro_speed_lane_gate.sh \
    --allocators tcmalloc,hz11-thread-exit-cap,hz11-thread-exit-cap-batch32 \
    --candidate hz11-thread-exit-cap-batch32 \
    --skip-span-soa-check

Output:
  bench_results/hz11_macro_speed_lane_20260708T061909Z/summary.md
```

No default lane changes are made.

## Results

RUNS=5.

| Workload | tcmalloc wall | thread-exit-cap wall | batch32 wall | batch32/tcmalloc wall | batch32/tcmalloc max RSS | batch32 status |
|---|---:|---:|---:|---:|---:|---|
| python_alloc | 0.032 | 0.032 | 0.032 | 1.000 | 0.735 | OK:5 |
| mstress | 0.188 | 0.219 | 0.221 | 1.176 | 1.130 | OK:5 |
| larson | 4.144 | 4.141 | 4.139 | 0.999 | 0.979 | OK:5 |
| sh6bench | 0.352 | 4.537 | 3.495 | 9.929 | 1.374 | OK:5 |
| xmalloc_test | 2.046 | 2.020 | 2.028 | 0.991 | 0.094 | OK:5 |
| cache_scratch | 1.158 | 1.171 | 1.177 | 1.016 | 0.456 | OK:5 |

Good:

```text
python_alloc and mstress still pass 5/5.
larson RSS fix holds:
  tcmalloc: 278912 KiB
  batch32:  273024 KiB
xmalloc_test remains a wall win with much lower RSS.
cache_scratch stays near parity with lower RSS.
sh6bench wall improves vs thread-exit-cap:
  4.537s -> 3.495s
```

Bad:

```text
sh6bench remains a macro blocker:
  wall:        9.929x tcmalloc
  max RSS:     1.374x tcmalloc
  current RSS: 1.407x tcmalloc
```

The generic summary verdict is `MIXED` because the candidate has no
crashes/timeouts and passes several rows. For macro promotion, this is a NO-GO
because the decision rule requires sh6bench wall/RSS to stop blocking.

## Decision

NO-GO for macro promotion.

`batch32` is a useful candidate improvement with no obvious macro side-effect in
this gate, but it does not solve the sh6bench wall/RSS blocker. Keep
`libhz11_span_transfer_thread_exit_cap_batch32.so` as an opt-in candidate lane
only.

Next work should attribute sh6bench page/span footprint with batch32 in place,
or otherwise explain why the remaining `~1.37x` RSS and `~9.9x` wall gap remain.
