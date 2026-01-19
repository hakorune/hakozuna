# S146-B OverflowBatchBox (NO-GO)

## Goal
Reduce owner_stash `push_one` CAS volume in T=32/R=90 by batching overflow
pushes into a per-TLS buffer and flushing via `push_list` (1 CAS for N objs).

## Design (archived)
- Trigger: remote ring overflow path, `n==1` fastpath.
- Buffer: per-sc TLS buffer with owner guard.
- Flush: on full or owner mismatch, link list and call `owner_stash_push_list`.
- A/B flags: `HZ3_S146_OVERFLOW_BATCH=1`, `HZ3_S146_OVERFLOW_BATCH_N=8/16`.

See `s146_overflow_batch.inc` for the archived snippet.

## Results (T=32 / R=90, ring_slots=262144, RUNS=10, ITERS=2.5M)

| Config | Median ops/s | vs baseline |
|--------|--------------|-------------|
| baseline | ~226M | - |
| S146-B N=8 | ~205M | -9% |
| S146-B N=16 | ~141M | -38% |

R=0 check:
- N=8: ~-4.5% (noise range)
- N=16: ~+1.6%

## Conclusion
NO-GO. The batching overhead (list construction + owner guard checks) outweighs
CAS reduction in the hot `n==1` path. Keep archived and OFF by default.

## References
- `hakozuna/hz3/docs/PHASE_HZ3_S146_OWNER_STASH_BOTTLENECK_ANALYSIS.md`
