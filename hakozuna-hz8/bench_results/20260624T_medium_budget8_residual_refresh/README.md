# Medium Budget8 Residual Refresh

```text
build:
  debug / bench attribution

row:
  medium r50
  threads=16
  iters=20000
  size=4097..65536
  remote_pct=50
  interleaved=1
  runs=5
```

## Result

```text
throughput:
  median 6.708M ops/s
  steady 6.994M ops/s
  p25 5.250M ops/s

memory:
  peak RSS 24.1MiB
  minor_median 2329
  minor_faults_per_op 0.007278

residency:
  budget_reject 10
  madvise 1711
  madvise_ms 11.209
  resident_peak 32MiB
```

## Residual Split

```text
slot_ms:
  506.843

collect_ms:
  521.949

lease_ms:
  254.360

lock_ms:
  210.140

qpush_ms:
  68.660

claim_ms:
  44.570
```

## Collect Shape

```text
collect_runs_per_call:
  7.102

collect_slots_per_run:
  1.235

class slots/run:
  8K   1.511
  16K  1.879
  32K  1.650
  64K  1.000

64K class:
  pub 426698
  qpush 426698
  collect_run 426698
  collect_slot 426698
```

## Interpretation

Budget8 removes the old madvise/fault churn as the primary bottleneck.  The
remaining medium r50 cost is protocol and geometry: slot mutation, collect work,
lease, and the 64K one-slot run shape.  Additional queue micro-tuning is not the
next target because qpush time is smaller than slot/collect/lease, and 64K class
has structural one-push-per-free behavior.
