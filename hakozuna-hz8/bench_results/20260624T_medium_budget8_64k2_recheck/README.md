# Medium Budget8 64K2 Recheck

```text
baseline:
  q64-run64k

candidate:
  q64-run64k2

runs:
  release R5 quick
```

## Note

The first files without `_seq` were accidentally run concurrently and are not
valid A/B evidence.  Use only the `_seq` files below.

## Sequential Results

```text
medium r50 baseline:
  median 13.578M ops/s
  p25 3.250M ops/s
  peak RSS 46.6MiB
  minor_median 8851

medium r50 64k2:
  median 3.507M ops/s
  p25 2.966M ops/s
  peak RSS 37.8MiB
  minor_median 822103

small remote90 baseline:
  median 54.440M ops/s
  p25 54.163M ops/s
  peak RSS 25.0MiB

small remote90 64k2:
  median 54.486M ops/s
  p25 54.357M ops/s
  peak RSS 25.3MiB
```

## Decision

Do not promote q64-run64k2.  Frozen small remains clean, but medium r50 is still
too sensitive to page-fault / arena variance and the candidate did not improve
the sequential quick median.  Keep q64-run64k as default and treat geometry as
blocked on the next arena/fault-lane evidence, not on queue micro-tuning.
