# Medium Collect Breakdown

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
throughput median: 2.088M ops/s
steady median:     2.136M ops/s
minor faults/op:   0.530219

medium_collect_breakdown:
  state_ms:         16.282
  pending_clear_ms: 19.922
  mask_ms:          16.225
  empty_ms:       1046.737

medium_residual_budget:
  collect_ms: 1273.963
  madvise_ms: 1682.263
  slot_ms:     937.194
  lease_ms:    142.612
```

## Interpretation

Collect slot-state mutation and pending clear are not the main collector cost
in this sample.  The dominant collector sub-bucket is empty transition handling,
which correlates with budget rejects, madvise, and minor faults.

The next behavior box should target resident-budget / empty-transition policy
or geometry.  Do not spend the next box on pending clear or slot-state store
micro-optimization.
