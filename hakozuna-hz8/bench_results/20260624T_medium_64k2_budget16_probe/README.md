# Medium 64K2 Budget16 Probe

```text
build:
  H8_MEDIUM_RESIDENT_BUDGET_CLASSES=16u

baseline:
  q64-run64k

candidate:
  q64-run64k2

row:
  release R5 quick
```

## Result

```text
medium r50 baseline:
  median 13.939M ops/s
  steady 14.467M ops/s
  peak RSS 30.2MiB
  minor_median 6731

medium r50 64k2:
  median 18.774M ops/s
  steady 20.096M ops/s
  peak RSS 57.7MiB
  minor_median 8333

small remote90 baseline:
  median 53.712M ops/s
  peak RSS 24.1MiB

small remote90 64k2:
  median 53.937M ops/s
  peak RSS 25.8MiB
```

## Interpretation

The earlier q64-run64k2 failure was primarily resident-budget pressure.  A
128KiB two-slot 64K run consumes twice the empty-retention charge, so the
budget8 cap under-retains it.  With budget16, q64-run64k2 improves medium r50 by
about 35% while keeping small remote90 stable.

This is not yet a default promotion because the resident cap doubles again, but
it is the strongest current MediumRun candidate.  Next step is a formal paired
gate for the combined policy: budget16 + q64-run64k2.
