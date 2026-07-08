# HZ11MacroSpeedLaneGateFine128Reclassify-L1

Status: GO for `fine128` as the next opt-in macro candidate. No default
promotion.

## Box

Reclassify the `HZ11MacroSpeedLaneGateWithFine128-L1` result under the documented
current-RSS gate semantics from `HZ11MacroCurrentRSSGateSemantics-L1`.

Boundary:

```text
gate/reclassification only
no allocator policy changes
no new promotion claim for the default path
```

Candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fine128.so
```

Evidence used:

```text
bench_results/hz11_macro_speed_lane_20260708T083138Z/summary.md
bench_results/hz11_macro_current_rss_20260708T085828Z/summary.md
docs/no_go/HZ11_MACRO_SPEED_LANE_FINE128_L1.md
docs/HZ11_MACRO_CURRENT_RSS_GATE_SEMANTICS_L1.md
```

## Macro Gate Result

The original fine128 macro gate was marked MIXED only because current RSS failed
under the old single-sample hard-fail rule.

Rows that already passed:

```text
python_alloc OK 5/5
mstress OK 5/5
larson RSS fix holds
xmalloc_test stays inside the gate shape
cache_scratch stays inside the gate shape
sh6bench max RSS passes the 1.25x guard
sh6bench wall does not regress versus batch32
```

Fine128 macro medians:

| workload | tcmalloc wall | fine128 wall | wall ratio | tcmalloc max RSS KiB | fine128 max RSS KiB | max RSS ratio |
|---|---:|---:|---:|---:|---:|---:|
| python_alloc | 0.032 | 0.032 | 1.000 | 7936 | 6528 | 0.823 |
| mstress | 0.192 | 0.221 | 1.151 | 217088 | 235996 | 1.087 |
| larson | 4.152 | 4.140 | 0.997 | 278912 | 273024 | 0.979 |
| sh6bench | 0.349 | 3.532 | 10.120 | 259584 | 323712 | 1.247 |
| xmalloc_test | 2.047 | 2.027 | 0.990 | 195456 | 18432 | 0.094 |
| cache_scratch | 1.163 | 1.177 | 1.012 | 7296 | 3456 | 0.474 |

Old blocker:

```text
python_alloc current RSS 1.714x tcmalloc
sh6bench current RSS     1.313x tcmalloc
```

## Current-RSS Rule Application

`HZ11MacroCurrentRSSGateSemantics-L1` defines the active rule:

```text
max RSS remains the primary hard RSS promotion guard.
current RSS hard-fails only when focused RUNS>=10 stability evidence also fails
or shows a large stable retained-footprint delta.
```

Focused RUNS=10 5ms sampling cleared both current-RSS misses:

| workload | metric | fine128/tcmalloc |
|---|---|---:|
| python_alloc | focused last RSS | 1.138 |
| python_alloc | focused max RSS | 1.138 |
| sh6bench | focused last RSS | 1.226 |
| sh6bench | focused p95 RSS | 1.167 |
| sh6bench | focused max RSS | 1.226 |

Interpretation:

```text
The old current-RSS failures are not stable retained-footprint regressions.
They are single-sample gate artifacts under rows where max RSS and focused
stabilized current RSS remain inside the guard.
```

## Reclassified Decision

Reclassify:

```text
HZ11MacroSpeedLaneGateWithFine128-L1:
  old verdict: MIXED / no promotion
  new interpretation: macro gate GO for opt-in fine128 candidate under the
  documented current-RSS semantics rule
```

This does not make fine128 the default allocator path.

Fine128 is now the best HZ11 macro candidate because it combines:

```text
thread-exit larson RSS fix
central-cap python_alloc/mstress correctness fix
batch32 sh6bench wall improvement
selective fine128 sh6bench RSS improvement
focused current-RSS stability under the documented rule
```

## Remaining Risk

Sh6bench wall remains far from tcmalloc:

```text
fine128 sh6bench wall ratio: 10.120x tcmalloc
```

This is acceptable only for candidate reclassification because the current HZ11
macro gate allows rows that are inside the wall/RSS shape and preserve the
candidate's intended improvements. It is not a public performance claim.

Remote/mixed evidence is not final yet:

```text
HZ11SelectiveFineclassRange-L1 has RUNS=5 fine128 matrix evidence.
Before promoting fine128 beyond opt-in candidate status, rerun the transfer
promotion matrix with RUNS=10 and fine128 as the candidate.
```

## Next Step

Run final remote/mixed confirmation:

```bash
RUNS=10 THREADS=16 ITERS=100000 \
  hakozuna-hz11/scripts/run_hz11_transfer_promotion_matrix.sh \
    --allocators tcmalloc,hz11-span-transfer,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fine128
```

Decision after that:

```text
If remote/mixed side effects remain acceptable:
  mark fine128 as the recommended opt-in macro speed lane candidate.

If remote/mixed side effects are material:
  keep fine128 as macro/RSS candidate only and retain span-transfer as the
  remote/mixed speed lane.
```
