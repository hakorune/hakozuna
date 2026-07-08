# HZ11MacroCurrentRSSGateSemantics-L1

Status: GO for gate semantics. No allocator policy change. No macro promotion.

## Box

Decide how the macro gate should interpret current RSS before promoting
`fine128`.

Boundary:

```text
diagnostics and gate semantics only
no allocator policy changes
no macro promotion claim
```

## Context

`HZ11MacroSpeedLaneGateWithFine128-L1` was MIXED:

```text
bench_results/hz11_macro_speed_lane_20260708T083138Z/summary.md
```

The allocator-shape checks passed:

```text
python_alloc OK 5/5
mstress OK 5/5
larson RSS fix holds
xmalloc_test/cache_scratch stay inside the gate shape
sh6bench max RSS passes: 323712 KiB, 1.247x tcmalloc
sh6bench wall does not regress versus batch32
```

The remaining blocker was current RSS:

```text
python_alloc current RSS 1.714x tcmalloc
sh6bench current RSS     1.313x tcmalloc
```

Prior evidence from `HZ11FineclassPythonAllocCurrentRSS-L1` already showed that
`python_alloc` current RSS can be a sampling artifact on a tiny denominator.

## Diagnostic Runner

Added:

```text
scripts/run_hz11_macro_current_rss_semantics.sh
```

The runner records:

```text
wall
max RSS
last sampled RSS
RSS p50/p75/p95 over time
last/max ratio
raw RSS trace
```

Rows:

```text
python_alloc
sh6bench
```

Allocators:

```text
tcmalloc
hz11-thread-exit-cap-batch32
hz11-thread-exit-cap-batch32-fine128
```

## Runs

The first run reproduced the macro gate's low `python_alloc` surface and showed
why the row needed a dedicated workload-script execution path:

```text
bench_results/hz11_macro_current_rss_20260708T085625Z/summary.md
```

The authoritative run uses a script-file Python workload and 5ms RSS sampling:

```bash
RUNS=10 SAMPLE_SLEEP=0.005 \
  hakozuna-hz11/scripts/run_hz11_macro_current_rss_semantics.sh
```

Output:

```text
bench_results/hz11_macro_current_rss_20260708T085828Z/summary.md
```

## Authoritative Result

`python_alloc`:

| allocator | wall sec | max RSS KiB | last RSS KiB | p95 RSS KiB | last/max | last/tcmalloc | max/tcmalloc |
|---|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 1.019 | 104128 | 104128 | 104128 | 1.000 | 1.000 | 1.000 |
| batch32 | 0.871 | 118628 | 118628 | 118628 | 1.000 | 1.139 | 1.139 |
| fine128 | 0.873 | 118506 | 118512 | 118506 | 1.000 | 1.138 | 1.138 |

`sh6bench`:

| allocator | wall sec | max RSS KiB | last RSS KiB | p95 RSS KiB | last/max | last/tcmalloc | p95/tcmalloc | max/tcmalloc |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| tcmalloc | 0.354 | 263936 | 263616 | 263616 | 0.991 | 1.000 | 1.000 | 1.000 |
| batch32 | 3.607 | 351680 | 351552 | 332480 | 0.999 | 1.334 | 1.261 | 1.332 |
| fine128 | 3.554 | 323520 | 323264 | 307648 | 0.999 | 1.226 | 1.167 | 1.226 |

## Interpretation

`python_alloc`:

```text
The macro gate's low current-RSS denominator is not a stable workload RSS
surface. With the actual workload under 5ms sampling, current RSS equals max RSS
and fine128 is 1.138x tcmalloc, inside the 1.25x guard.
```

`sh6bench`:

```text
The focused run does not reproduce the 1.313x current-RSS failure.
fine128 last RSS is 1.226x tcmalloc and p95 RSS is 1.167x tcmalloc.
last/max is stable near 1.0 for tcmalloc and HZ11 lanes.
```

The current-RSS failure in the macro gate is therefore not sufficient evidence
of a retained-footprint regression. It is a sampling-rule problem: one terminal
sample can move the ratio across the threshold even when focused time-series
sampling and max RSS are inside the guard.

## Gate Rule

Use this rule for HZ11 macro promotion gates:

```text
1. max RSS remains the primary hard RSS promotion guard.
2. current RSS is a hard fail only when a focused RUNS>=10 stability check also
   fails.
3. The stability check should use sampled RSS over time and compare median last
   RSS and/or p95 RSS against tcmalloc.
4. A current-RSS failure is informational, not a promotion blocker, when:
     max RSS passes,
     focused last RSS or p95 RSS passes,
     and there is no large stable post-workload retained-footprint delta.
5. For tiny-denominator rows such as python_alloc, require focused sampling
   before treating current RSS as a hard fail.
```

This is Option B plus Option C from the box definition:

```text
current RSS must pass only with stabilized multi-sample evidence;
current RSS is a hard fail only for stable, large retained-footprint deltas.
```

## Fine128 Consequence

Under this documented rule, the current-RSS blocker from
`HZ11MacroSpeedLaneGateWithFine128-L1` is cleared by focused diagnostics:

```text
python_alloc fine128/tcmalloc:
  macro current RSS 1.714x
  focused last RSS  1.138x
  focused max RSS   1.138x

sh6bench fine128/tcmalloc:
  macro current RSS 1.313x
  focused last RSS  1.226x
  focused p95 RSS   1.167x
  focused max RSS   1.226x
```

This box does not promote fine128. It makes the prior fine128 gate
interpretable under a documented RSS rule. The next step should reclassify or
rerun the fine128 macro gate under this rule, then run final remote/mixed
confirmation if fine128 remains acceptable.
