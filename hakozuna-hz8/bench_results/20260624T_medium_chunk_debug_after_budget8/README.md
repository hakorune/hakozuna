# Medium Chunk Debug After Budget8

```text
build:
  debug / bench attribution / H8_MEDIUM_CHUNK_CARVE

row:
  medium r50
  threads=16
  iters=20000
  size=4097..65536
  remote_pct=50
  interleaved=1
  runs=3
```

## Result

```text
throughput:
  median 4.244M ops/s
  steady 4.541M ops/s

memory:
  peak RSS 32.2MiB
  minor_median 6972
  minor_faults_per_op 0.021788

chunk:
  chunk_create 4
  chunk_alloc 923
  reserved_bytes 64MiB
  used_bytes 57.7MiB
```

## Residual Split

```text
budget_reject:
  4594

madvise:
  6283

madvise_ms:
  78.032

slot_ms:
  287.686

collect_ms:
  337.315

lease_ms:
  154.465

qpush_ms:
  39.446
```

## Decision

Chunk carving is still HOLD as default.  It removes one-run mmap pressure but
reintroduces medium r50 fault/decommit churn through resident-budget pressure
and more run creation.  Continue on the per-run mmap default and attack the
structural 64K one-slot geometry / size-policy lane before reopening chunk.
