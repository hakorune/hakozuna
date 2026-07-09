# HZ11 Windows Class-Aware Refill Batch L1

Status: CLOSED as Windows matrix behavior evidence. Do not promote as the
general selected Windows row.

## Question

The `cache512` pressure probe showed that the Windows matrix weakness is not a
global cache-cap problem. The remaining `balanced` and `wide_ws` rows are
refill-dominated in a few classes:

```text
balanced:
  2048B, 1024B, 512B

wide_ws:
  1024B, 512B, 256B, 128B
```

Can HZ11 reduce returned-sink mutex/refill churn by popping returned objects in
a small class-aware batch and seeding the thread cache, without changing the
selected `hz11-span-cache256` row?

## Scope

Added opt-in matrix rows:

```text
hz11-span-cache512-classbatch
hz11-span-cache512-classbatch-diag
```

The behavior row enables:

```text
HZ11_CACHE_CAP=512
HZ11_RETURNED_REFILL_BATCH=1
HZ11_RETURNED_REFILL_BATCH_MIN_CLASS=4
HZ11_RETURNED_REFILL_BATCH_COUNT=32
```

The diagnostic row additionally enables:

```text
HZ11_ENABLE_HOT_COUNTERS=1
HZ11_SPAN_RETURNED_DIAG=1
HZ11_CLASS_DIAG=1
HZ_BENCH_HZ11_SUMMARY=1
```

Default builds keep `HZ11_RETURNED_REFILL_BATCH=0`; this experiment does not
change existing HZ11 rows.

## Evidence

Source:

```text
results/hz11-wide-compare/matrix-classbatch-r1/20260709_194815_allocator_matrix.md
```

RUNS=1 scout comparison:

| Profile | cache512 ops/s | classbatch ops/s | speedup | cache512 RSS | classbatch RSS | Reading |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| balanced | 13.922M | 34.099M | 2.45x | 41060KB | 43080KB | strong speed signal, modest RSS cost |
| wide_ws | 10.803M | 24.157M | 2.24x | 90104KB | 85988KB | strong speed signal and lower RSS |
| larger_sizes | 33.722M | 46.246M | 1.37x | 59900KB | 57980KB | positive signal and lower RSS |

Diagnostic row:

```text
balanced:
  hz11_malloc=1015808
  hz11_hit=962084
  hz11_refill=53724
  hz11_overflow=1570
  hz11_returned_pop_hit=783179
  hz11_returned_pop_miss=28928

wide_ws:
  hz11_malloc=813568
  hz11_hit=629181
  hz11_refill=184387
  hz11_overflow=1478
  hz11_returned_pop_hit=655459
  hz11_returned_pop_miss=116045

larger_sizes:
  hz11_malloc=305088
  hz11_hit=281039
  hz11_refill=24049
  hz11_overflow=2184
  hz11_returned_pop_hit=246543
  hz11_returned_pop_miss=16329
```

Compared with the prior `cache512-classdiag` row, the key shift is:

```text
balanced:
  hit    269100 -> 962084
  refill 746708 -> 53724

wide_ws:
  hit    46437  -> 629181
  refill 767131 -> 184387

larger_sizes:
  hit    47771  -> 281039
  refill 257317 -> 24049
```

## Interpretation

The matrix weakness was not solved by a larger cap alone because each refill
only consumed one object from the returned sink. In wide working-set rows, this
left the dominant classes almost permanently cold in the front cache.

Class-aware returned-pop batching changes the placement:

```text
returned sink -> batch pop -> one allocation result + rest seeded into class cache
```

That turns repeated returned-sink lookups into class-local hits and sharply
reduces refill volume. The effect is strongest exactly where classdiag predicted
it should be: `balanced` and `wide_ws`.

## Decision

```text
classbatch:
  KEEP as strong Windows matrix behavior evidence / opt-in specialist.

selected row:
  unchanged for now.

promote:
  NO for default/general selected row.
```

## Confirmation

Matrix repeat source:

```text
results/hz11-wide-compare/matrix-classbatch-repeat3/
```

RUNS=3 median:

| Profile | cache512 ops/s | classbatch ops/s | speedup | cache512 RSS | classbatch RSS | Reading |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| balanced | 17.590M | 33.273M | 1.89x | 40744KB | 38264KB | repeat-stable win |
| wide_ws | 11.069M | 24.614M | 2.22x | 86000KB | 87996KB | repeat-stable win, modest RSS cost |
| larger_sizes | 34.177M | 45.150M | 1.32x | 59968KB | 59728KB | repeat-stable win |

Random mixed confirmation source:

```text
results/hz11-wide-compare/random-mixed-classbatch-r3/20260709_195506_paper_random_mixed_windows.md
```

RUNS=3 median:

| Profile | cache256 ops/s | cache512 ops/s | classbatch ops/s | Reading |
| --- | ---: | ---: | ---: | --- |
| small | 156.751M | 156.450M | 156.091M | no material regression |
| medium | 155.439M | 155.568M | 149.514M | classbatch regresses ~3.8% vs cache256 |
| mixed | 155.218M | 154.981M | 148.009M | classbatch regresses ~4.6% vs cache256 |

This makes the promotion boundary clear:

```text
GO:
  matrix balanced / wide_ws / larger_sizes opt-in specialist.

NO-GO:
  replace hz11-span-cache256 as the general selected Windows row.
```

## Next Gate

Before any broader promotion:

```text
1. Compare against:
   hz11-span-cache256
   hz11-span-cache512
   hz11-span-cache512-classbatch

2. Acceptance:
   balanced and wide_ws remain materially faster
   larger_sizes does not regress
   random_mixed selected rows do not regress materially
   RSS remains near cache512 or lower
```

The current L1 fails the random_mixed general-row acceptance, so keep it as a
profile-specific row rather than a default.

## Narrow L2 Follow-up

The broad L1 was too aggressive for random_mixed medium/mixed, so two narrower
L2 variants were added:

```text
hz11-span-cache512-classbatch16
  class >= 4
  batch count 16

hz11-span-cache512-classbatch16-4-7
  class 4..7 only
  batch count 16
```

Sources:

```text
results/hz11-wide-compare/matrix-classbatch-narrow-r3/
results/hz11-wide-compare/matrix-classbatch-4-7-r3/
results/hz11-wide-compare/random-mixed-classbatch-narrow-r3/
results/hz11-wide-compare/random-mixed-classbatch-4-7-r3/
```

Matrix RUNS=3 median:

| Profile | cache512 | classbatch16 | classbatch16-4-7 | Reading |
| --- | ---: | ---: | ---: | --- |
| balanced | 14.575M | 36.207M | 38.658M | both strong |
| wide_ws | 13.606M | 22.133M | 23.986M | both strong |
| larger_sizes | 33.616M | 42.714M | 29.938M | 4-7 loses larger_sizes |

Random mixed RUNS=3 median:

| Profile | cache256 | classbatch16 | classbatch16-4-7 | Reading |
| --- | ---: | ---: | ---: | --- |
| small | 157.501M | 158.034M | 157.784M | both ok |
| medium | 155.526M | 153.651M | 155.742M | 16 has small regression; 4-7 ok |
| mixed | 155.861M | 153.764M | 155.727M | 16 has small regression; 4-7 ok |

Decision:

```text
classbatch32:
  matrix specialist only.
  Do not promote as general row.

classbatch16:
  candidate-watch.
  It keeps matrix wins including larger_sizes, but costs about 1-1.3% on
  random_mixed medium/mixed in this scout.

classbatch16-4-7:
  balanced/wide_ws specialist.
  It preserves random_mixed, but loses larger_sizes.

selected row:
  keep hz11-span-cache256 until classbatch16 earns broader repeat evidence.
```

Next natural experiment:

```text
pressure-gated classbatch16:
  enable batch refill only after a class shows repeated returned-pop misses or
  low hit rate, instead of always batching the class.
```

## Pressure Gate L2

Pressure-gated variants were added to test whether classbatch could be enabled
only after class-local returned-pop pressure appears:

```text
hz11-span-cache512-classbatch16-pressure
  threshold 4

hz11-span-cache512-classbatch16-pressure1
  threshold 1
```

Sources:

```text
results/hz11-wide-compare/matrix-classbatch-pressure-r3/
results/hz11-wide-compare/random-mixed-classbatch-pressure-r3/
results/hz11-wide-compare/matrix-classbatch-pressure1-r3/
results/hz11-wide-compare/random-mixed-classbatch-pressure1-r3/
```

Matrix RUNS=3 median:

| Profile | cache512 | classbatch16 | pressure4 | pressure1 | Reading |
| --- | ---: | ---: | ---: | ---: | --- |
| balanced | 16.046M | 37.013M | 16.476M | 18.647M | pressure gate loses the matrix win |
| wide_ws | 11.442M | 22.523M | 12.973M | 14.795M | pressure gate loses the matrix win |
| larger_sizes | 32.507M | 43.429M | 33.214M | 33.763M | pressure gate loses the matrix win |

Random mixed RUNS=3 median:

| Profile | cache256 | classbatch16 | pressure4 | pressure1 | Reading |
| --- | ---: | ---: | ---: | ---: | --- |
| small | 157.103M | 157.604M | 157.064M | 157.793M | all ok |
| medium | 155.835M | 153.172M | 155.250M | 155.911M | pressure protects medium |
| mixed | 155.250M | 153.534M | 155.851M | 154.873M | pressure mostly protects mixed |

Decision:

```text
pressure-gated classbatch:
  NO-GO as implemented.

Reason:
  It protects random_mixed by delaying batch refill, but that delay also removes
  the matrix balanced/wide_ws/larger_sizes improvement. The pressure signal is
  too late for the workload shape.

Keep:
  classbatch16 as candidate-watch / matrix helper.
```

Next, if this track continues, should not be another simple threshold. A better
candidate would need a class/workload-visible signal available before the refill
miss loop, or a profile-scoped matrix specialist row.
