# HZ11FineclassPythonAllocCurrentRSS-L1

Status: GO for diagnosis only. No macro promotion.

## Box

Attribute the `python_alloc` current-RSS macro gate miss for the fineclass
candidate:

```text
libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so
```

Boundary:

```text
diagnostics only
do not change fineclass policy in this box
do not promote fineclass to macro default
```

## Inputs

The prior macro gate kept fineclass as NO-GO for macro promotion:

```text
bench_results/hz11_macro_speed_lane_20260708T073737Z/summary.md
docs/no_go/HZ11_MACRO_SPEED_LANE_FINECLASS_L1.md
```

The useful fineclass signal remains sh6bench RSS:

```text
batch32   max RSS 350336 KiB, wall 3.558s
fineclass max RSS 301312 KiB, wall 3.582s
```

The gate blocker was small and only on `python_alloc` current RSS:

```text
tcmalloc current RSS   1792 KiB
fineclass current RSS  2304 KiB
ratio                  1.286x
threshold              1.25x
```

## Runs

Focused `python_alloc` diagnostics were run with `RUNS=10` under:

```text
tcmalloc
hz11-thread-exit-cap-batch32
hz11-thread-exit-cap-batch32-fineclass
hz11-thread-exit-cap-batch32-live-diag
hz11-thread-exit-cap-batch32-fineclass-live-diag
```

Outputs:

```text
bench_results/hz11_fineclass_python_alloc_rss_20260708T075016Z/summary.md
bench_results/hz11_fineclass_python_alloc_rss_20260708T075132Z/summary.md
```

The first run used 5ms sampling. The second used 20ms sampling to match the
macro gate sampling interval.

## RSS Result

5ms sampling:

```text
tcmalloc:
  median max/current RSS    104128 / 104128 KiB

batch32:
  median max/current RSS    118792 / 118792 KiB
  current/tcmalloc          1.141x

fineclass:
  median max/current RSS    117764 / 117764 KiB
  current/tcmalloc          1.131x
```

20ms sampling:

```text
tcmalloc:
  median max/current RSS    104064 / 104064 KiB

batch32:
  median max/current RSS    118790 / 118790 KiB
  current/tcmalloc          1.142x

fineclass:
  median max/current RSS    117850 / 117814 KiB
  current/tcmalloc          1.132x
```

In both focused runs, current RSS tracks max RSS and fineclass is slightly below
batch32. This does not reproduce the macro gate's tiny `python_alloc` current
RSS surface of 1792 KiB versus 2304 KiB.

## Counters

20ms sampling totals across RUNS=10:

```text
span_create:
  batch32     17980
  fineclass   17980

central final count:
  batch32     835520
  fineclass   734400

xfer_insert:
  batch32     5044480
  fineclass   6394240

central_insert:
  batch32     27124480
  fineclass   25585600
```

Live diagnostics:

```text
requested bytes high:
  batch32     84955674
  fineclass   84978054

slot bytes high:
  batch32     114049760
  fineclass   111946176

request-to-slot waste:
  batch32     29094086
  fineclass   26968122
```

Fineclass reduces Python request-to-slot waste by about 2.1 MiB in this focused
workload. It does not reduce total `span_create`, but it also does not create a
new retained-central explanation for the macro current-RSS miss.

## Decision

The 512 KiB macro `python_alloc` current-RSS miss is best treated as a sampling
artifact on a tiny denominator, not as evidence that fineclass worsens the
steady resident footprint.

Reasons:

```text
focused RUNS=10 does not reproduce the low 1792/2304 KiB current-RSS surface
5ms and 20ms focused sampling agree
fineclass current RSS is lower than batch32 in focused sampling
fineclass central final count is lower than batch32
fineclass span_create is equal to batch32
fineclass request-to-slot waste improves slightly
```

This clears the specific fineclass `python_alloc` current-RSS suspicion as a
diagnostic result, but it does not promote fineclass. The macro gate should not
make a default decision from the tiny `python_alloc` current-RSS denominator
without a more stable sampling rule.

## Next

Before another macro promotion attempt, run a dedicated fineclass
remote/mixed tradeoff box. Fineclass remains an opt-in sh6bench RSS candidate
until that tradeoff is measured and the full macro gate is rerun.
