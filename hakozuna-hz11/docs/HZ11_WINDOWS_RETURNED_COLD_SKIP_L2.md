# HZ11 Windows ReturnedColdSkip L2

Status: KEEP as Windows matrix evidence / candidate-watch. Do not promote as the
selected general Windows row.

## Question

After `classbatch16`, the Windows matrix still showed a tcmalloc gap on
`balanced`, `wide_ws`, and `larger_sizes`. The missing attribution was not just
per-class refill volume, but the refill subpath:

```text
front-cache empty
  -> returned sink lookup
  -> current span bump
  -> new span / sys fallback
```

`HZ11_MATRIX_ATTRIB_DIAG=1` was added as a diagnostic-only row to split that
path without putting atomics into speed lanes.

## New Diagnostic Rows

```text
hz11-span-cache256-matrixattrib
hz11-span-cache512-matrixattrib
hz11-span-cache512-classbatch16-matrixattrib
hz11-span-cache512-classbatch16-coldskip-matrixattrib
```

These rows enable:

```text
HZ11_ENABLE_HOT_COUNTERS=1
HZ11_SPAN_RETURNED_DIAG=1
HZ11_MATRIX_ATTRIB_DIAG=1
HZ_BENCH_HZ11_SUMMARY=1
```

They print per-class lines:

```text
[HZ11_MATRIX_ATTRIB]
  refill_samples
  returned_one_hit / miss
  returned_batch_call / items / miss
  current_hit
  span_new
  sys_fallback
```

Diagnostic rows are not speed-ranking rows.

## Observation

Source:

```text
results/hz11-wide-compare/matrix-attrib-r1/20260709_212919_allocator_matrix.md
```

The main signal:

```text
classbatch16 still has many:
  returned_batch_miss -> current_hit
```

Examples from the R1 attribution:

```text
wide_ws class 1024:
  returned_batch_call = 86293
  returned_batch_miss = 64792
  current_hit         = 63776

larger_sizes class 8192:
  returned_batch_call = 17321
  returned_batch_miss = 8223
  current_hit         = 7194
```

This means many refill attempts pay the returned-sink lock even when the sink is
empty and the current span can satisfy the allocation.

## Behavior Probe

Added:

```text
hz11-span-cache512-classbatch16-coldskip
```

Build flags:

```text
HZ11_CACHE_CAP=512
HZ11_RETURNED_REFILL_BATCH=1
HZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4
HZ11_RETURNED_REFILL_BATCH_COUNT=16
HZ11_RETURNED_REFILL_COLD_SKIP=1
HZ11_RETURNED_REFILL_COLD_SKIP_BUDGET=8
```

Policy:

```text
if returned-pop recently missed
and current span has slots:
  skip a small number of returned-sink lock attempts
  bump from current directly

on cache overflow/flush:
  clear the skip hint for that class
```

This keeps the hint bounded and avoids permanently ignoring returned objects.

## Matrix Results

Source:

```text
results/hz11-wide-compare/matrix-coldskip-repeat3/
```

RUNS=3 median:

| Profile | classbatch16 ops/s | coldskip ops/s | classbatch16 RSS | coldskip RSS | Reading |
| --- | ---: | ---: | ---: | ---: | --- |
| balanced | 30.591M | 30.135M | 39760KB | 38628KB | speed neutral/slightly lower, RSS lower |
| wide_ws | 22.585M | 26.770M | 88732KB | 74512KB | clear speed and RSS win |
| larger_sizes | 44.455M | 43.682M | 59472KB | 60468KB | slight speed/RSS loss |

R1 had a stronger balanced signal, but repeat-3 makes the stable claim narrower:

```text
GO:
  wide_ws / returned-empty-lock evidence
  RSS reduction signal on wide_ws

HOLD:
  balanced

NO:
  default selected-row promotion
```

Follow-up source:

```text
results/hz11-wide-compare/matrix-coldskip-repeat5-loop/
```

RUNS=5 median:

| Profile | cache256 ops/s | classbatch16 ops/s | coldskip ops/s | classbatch16 RSS | coldskip RSS | Reading |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| balanced | 13.167M | 27.908M | 27.690M | 40232KB | 39960KB | neutral/slightly lower speed, small RSS win |
| wide_ws | 10.809M | 21.490M | 22.075M | 85996KB | 77148KB | modest speed win, clear RSS win |
| larger_sizes | 31.893M | 40.355M | 40.812M | 60404KB | 59444KB | neutral/slightly better |

The repeat-5 loop confirms the safer interpretation:

```text
KEEP:
  coldskip is better shaped than plain classbatch16.
  It reduces returned-empty lock churn where wide_ws pressure is visible.

DO NOT PROMOTE:
  tcmalloc remains much faster on these matrix rows.
  coldskip is not a selected general Windows row.
```

## Random Mixed Check

Source:

```text
results/hz11-wide-compare/random-mixed-coldskip-r3/20260709_213729_paper_random_mixed_windows.md
```

RUNS=3 median:

| Profile | cache256 ops/s | classbatch16 ops/s | coldskip ops/s | tcmalloc ops/s | Reading |
| --- | ---: | ---: | ---: | ---: | --- |
| small | 158.100M | 157.439M | 158.267M | 124.274M | no regression |
| medium | 155.067M | 152.915M | 154.331M | 154.407M | coldskip recovers most classbatch16 loss |
| mixed | 154.947M | 152.265M | 154.246M | 151.151M | coldskip recovers most classbatch16 loss |

This makes `coldskip` better shaped than plain `classbatch16`, but it still does
not justify replacing `hz11-span-cache256` as the selected general row.

## Decision

```text
HZ11_MATRIX_ATTRIB_DIAG:
  KEEP as diagnostic-only instrumentation.
  Do not mix with speed rows.

ReturnedColdSkip-L2:
  KEEP as Windows matrix evidence / candidate-watch.
  It explains and partly fixes returned-empty lock churn.

selected Windows row:
  unchanged: hz11-span-cache256

next if continuing Windows matrix work:
  do not tune skip budget blindly.
  if wide_ws matters, compare a profile-scoped matrix lane:
    cache512 + classbatch16 + coldskip
  against cache256 and tcmalloc with repeat-5.
```
