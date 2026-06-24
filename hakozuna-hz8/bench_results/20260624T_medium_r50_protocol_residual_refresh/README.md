# Medium R50 Protocol Residual Refresh

```text
bench: h8_bench debug/audit
runs: 3
threads: 16
iters: 20000
row: 4097..65536 remote_pct=50 interleaved=1
geometry: q64-run64k
arena: per-run-mmap
```

## Result

```text
throughput median: 6.653M ops/s
steady median:     6.900M ops/s
minor faults/op:   0.006078

medium_residual_budget:
  slot_ms:    460.179
  collect_ms: 228.818
  lease_ms:   148.318
  lock_ms:     85.865
  qpush_ms:    40.499
  claim_ms:    26.221
  madvise_ms:   4.088

medium_collect_detail:
  accepted: 479945
  rejected: 0
  reject_ratio: 0

medium_remote_class_density:
  8K:  1.376 slots/run
  16K: 1.727 slots/run
  32K: 1.611 slots/run
  64K: 1.000 slots/run
```

## Interpretation

`budget_reject=0`, low minor faults, and low `madvise_ms` show the current
medium r50 residual is no longer mmap/fault churn.  The remaining visible
cost is slot/free bookkeeping plus owner collect/lease, with 64K class density
still structurally limited to one slot per collected run.

Fresh chunk A/B improves `main_interleaved_remote90` but regresses medium r50,
so the next allocator behavior box should return to protocol/slot residual
instead of defaulting chunk arena.
