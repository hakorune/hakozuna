# Medium Budget8 Default

```text
change:
  H8_MEDIUM_RESIDENT_BUDGET_CLASSES 4 -> 8

budget:
  16MiB -> 32MiB global resident empty cap
```

## Release Quick

```text
medium r50 R5:
  median 10.170M ops/s
  steady 10.453M ops/s
  peak RSS 58.9MiB
  minor_median 99813

small remote90 R5:
  first sample median 32.837M ops/s
  repeat median 54.081M ops/s
  repeat peak RSS 26.7MiB
```

## Frozen Small Check

```text
small local0 R10:
  median 251.226M ops/s
  steady 364.927M ops/s
  peak RSS 4.7MiB
  minor_median 153

small remote90 R10:
  median 40.305M ops/s
  steady 43.174M ops/s
  peak RSS 99.0MiB
  minor_median 10713
```

## Debug R3

```text
medium r50:
  median 6.574M ops/s
  budget_reject 0
  madvise 1000
  madvise_ms 6.702
  minor_faults_per_op 0.010722

collect breakdown:
  state_ms 14.593
  pending_clear_ms 18.368
  mask_ms 14.107
  empty_ms 80.680
```

## Decision

Promote budget8 as the MediumRun default candidate.  The collect breakdown
showed empty transition / decommit churn as the dominant bucket; increasing the
fixed resident budget removes budget rejects and collapses `empty_ms` without
changing remote protocol.

The first small remote90 sample was low, but immediate repeat and R10 recovered
to the frozen small gate.  The local0 R10 also remains above the v0 local gate,
though this host run had wide tail variance.
