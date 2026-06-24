# Medium 64K2 Default Verify

```text
default:
  H8_MEDIUM_RESIDENT_BUDGET_CLASSES=16u
  q64-run64k2
```

## Functional

```text
smoke:
  pass

safety-stress:
  pass
```

## Quick Release

```text
medium r50 R5:
  median 19.043M ops/s
  steady 20.409M ops/s
  peak RSS 67.9MiB
  minor_median 9521

small remote90 R5:
  median 53.313M ops/s
  steady 55.079M ops/s
  peak RSS 24.5MiB
  minor_median 5260
```

## Decision

Promote budget16 + q64-run64k2 as the MediumRun default.  The R10 paired gate
showed strong medium r50 improvement while keeping frozen-small rows within the
working envelope, and the default build now reports `medium_geometry_id=q64-run64k2`.
