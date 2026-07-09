# HZ11 Windows Span Cache512 Pressure L4

Status: HOLD as Windows pressure evidence. Do not promote over
`hz11-span-cache256` yet.

## Question

The Windows selected row `hz11-span-cache256` is strong in the paper-aligned
`random_mixed` runner, but remains weak in the legacy allocator matrix
`balanced` and `wide_ws` rows. Is the matrix weakness mainly caused by
returned-object / refill pressure that can be solved by increasing
`HZ11_CACHE_CAP` from 256 to 512?

## Scope

Added matrix-only rows:

```text
hz11-span-cache512
hz11-span-cache512-diag
```

The diagnostic row enables:

```text
HZ11_CACHE_CAP=512
HZ11_ENABLE_HOT_COUNTERS=1
HZ11_SPAN_RETURNED_DIAG=1
HZ_BENCH_HZ11_SUMMARY=1
```

The speed row does not enable the diagnostic counters.

## Evidence

Source:

```text
results/hz11-wide-compare/matrix-cache512-probe-r1/20260709_182456_allocator_matrix.md
```

RUNS=1 scout comparison:

| Profile | cache256 ops/s | cache512 ops/s | cache256 RSS | cache512 RSS | Reading |
| --- | ---: | ---: | ---: | ---: | --- |
| balanced | 12.459M | 17.962M | 38320KB | 41260KB | cap512 helps |
| wide_ws | 11.076M | 11.099M | 89508KB | 90332KB | cap512 does not solve |
| larger_sizes | 30.863M | 32.399M | 60452KB | 60768KB | small positive |

Diagnostic cache512 counters:

```text
balanced:
  hz11_malloc=1015808
  hz11_hit=269100
  hz11_refill=746708
  hz11_overflow=1440
  hz11_flush_items=737280
  hz11_returned_push=737280
  hz11_returned_pop_hit=717962
  hz11_returned_pop_miss=28746

wide_ws:
  hz11_malloc=813568
  hz11_hit=46437
  hz11_refill=767131
  hz11_overflow=1470
  hz11_flush_items=752640
  hz11_returned_push=752640
  hz11_returned_pop_hit=654114
  hz11_returned_pop_miss=113017

larger_sizes:
  hz11_malloc=305088
  hz11_hit=47771
  hz11_refill=257317
  hz11_overflow=2231
  hz11_flush_items=254798
  hz11_returned_push=254798
  hz11_returned_pop_hit=240952
  hz11_returned_pop_miss=16365
```

## Interpretation

`cache512` reduces pressure enough to improve the matrix `balanced` row and
slightly improve `larger_sizes`. However, it does not materially improve
`wide_ws`. The wide working-set row still shows refill-dominated behavior:

```text
wide_ws refill / malloc ~= 767131 / 813568
```

This means the remaining weakness is not simply the per-class cache cap. It is
likely a visibility/locality/refill-placement issue in the matrix workload.

## Decision

```text
cache512:
  KEEP as pressure evidence / HOLD.

Do not:
  promote cache512 as the selected Windows row yet.
  replace cache256 based on a single RUNS=1 scout.
  claim that cap tuning solves the Windows matrix weakness.

Next:
  inspect why wide_ws remains refill-dominated.
  compare matrix wide_ws loop shape against random_mixed.
  if needed, add attribution for refill source class / cache miss class.
```

## Class Attribution Follow-up

Source:

```text
results/hz11-wide-compare/matrix-classdiag-r1/20260709_183701_allocator_matrix.md
```

Added diagnostic-only row:

```text
hz11-span-cache512-classdiag
```

This row enables `HZ11_CLASS_DIAG=1`, which prints per-class malloc / hit /
refill / overflow / returned-pop attribution. It is not a speed row.

### balanced

```text
total malloc 1015808
total hit    269100
total refill 746708
```

Top refill classes:

| class | slot | malloc | hit | refill | overflow | returned hit | returned miss | refill share | hit rate |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 7 | 2048 | 511370 | 42335 | 469035 | 913 | 455922 | 13113 | 62.8% | 8.3% |
| 6 | 1024 | 256161 | 55311 | 200850 | 390 | 194183 | 6667 | 26.9% | 21.6% |
| 5 | 512 | 127923 | 55707 | 72216 | 137 | 68117 | 4099 | 9.7% | 43.5% |

### wide_ws

```text
total malloc 813568
total hit    46437
total refill 767131
```

Top refill classes:

| class | slot | malloc | hit | refill | overflow | returned hit | returned miss | refill share | hit rate |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 6 | 1024 | 412688 | 6659 | 406029 | 790 | 345322 | 60707 | 52.9% | 1.6% |
| 5 | 512 | 206262 | 5355 | 200907 | 387 | 170075 | 30832 | 26.2% | 2.6% |
| 4 | 256 | 103578 | 7318 | 96260 | 181 | 79894 | 16366 | 12.5% | 7.1% |
| 3 | 128 | 51537 | 5037 | 46500 | 86 | 38165 | 8335 | 6.1% | 9.8% |

### Reading

The matrix weakness is not spread evenly across all classes. It is dominated by
the larger classes inside the matrix size range:

```text
balanced:
  2048B and 1024B dominate refill.

wide_ws:
  1024B, 512B, 256B, and 128B dominate refill.
```

For `wide_ws`, cache512 still leaves the hot cache nearly cold for the dominant
classes. The returned sink is active, but the front-cache hit rate remains very
low. This points to a refill placement / workload-locality mismatch, not just a
global cap value.

Next candidate should focus on the dominant classes only, for example:

```text
class-aware refill batch / retain policy for 512B and 1024B
or a matrix-specific returned-pop batch path
```

Do not add another global cap ladder before testing a class-aware mechanism.

Follow-up implemented:

```text
HZ11_WINDOWS_CLASS_AWARE_REFILL_BATCH_L1.md
```

That L1 keeps the selected row unchanged and adds an opt-in
`hz11-span-cache512-classbatch` behavior probe. RUNS=1 matrix evidence strongly
supports the class-aware hypothesis; repeat confirmation is still required
before promotion.

## Current Windows Selected Row

Keep:

```text
hz11-span-cache256
```

Reason:

```text
cache256 already has stronger random_mixed evidence.
cache512 is promising for matrix balanced but incomplete for wide_ws.
```
