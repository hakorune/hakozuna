# PHASE_HZ3_S13: TransferCache + BatchChain Design

## Summary

Goal is to remove O(n) intrusive-list work from the shared path by batching
objects into fixed-size chains and moving them through a TransferCache (TC).
Hot TLS bin push/pop stays unchanged. Only slow/event paths change.

This is a clean, box-theory-friendly replacement: we keep hot boxes intact and
swap shared boxes (central) with a TC + batch-chain interface.

## Goals

- Reduce shared path cost (O(n) list walk -> O(1) batch swap).
- Reduce lock hold time (one lock per batch, not per object).
- Keep hot path unchanged (TLS bin push/pop only).
- Compile-time gating, no ENV, easy A/B.
- Fail-fast on invariant violations (debug only).

## Non-Goals

- No change to hot path branches or per-op stats.
- No new global logging in hot path.
- No redesign of outbox/inbox semantics in this phase.
- No per-CPU or rseq work here.

## Boxes and Boundaries

### Box: BatchChainBox

Represents a fixed-size chain of objects from the same size class.

Data:
```
typedef struct {
    void* head;
    void* tail;
    uint16_t count;  // 1..HZ3_TC_BATCH
} Hz3BatchChain;
```

Rules:
- Chain is intrusive list (next at *(void**)obj).
- Objects in chain must be same class and same owner shard.
- Construction is event-only (bin drain / central pop / segment alloc).
- No per-object operations under TC lock.

API (slow path only):
- `hz3_batch_init(head, tail, count)`
- `hz3_batch_take_one(&chain, &obj)` (optional helper)

### Box: TransferCacheBox (TC)

Per-shard, per-class ring of batch slots.

Data:
```
#define HZ3_TC_SLOTS 32
typedef struct {
    pthread_mutex_t lock;
    uint8_t count;
    uint8_t head;
    uint8_t tail;
    Hz3BatchChain slots[HZ3_TC_SLOTS];
} Hz3TransferCache;
```

Global:
`Hz3TransferCache g_hz3_tc[HZ3_NUM_SHARDS][HZ3_NUM_SC];`

Rules:
- TC operations are O(1) and lock-held briefly.
- TC never touches object contents except to link chain (already done).
- TC is only used in slow/event paths.

API:
- `int hz3_tc_pop(int shard, int sc, Hz3BatchChain* out)`
- `int hz3_tc_push(int shard, int sc, const Hz3BatchChain* in)`
  - Returns 1 on success, 0 if full.

### Box: CentralFallbackBox

Existing central bins remain as the fallback when TC misses or overflows.
Central interface becomes batch-aware:

- `hz3_central_push_list(shard, sc, head, tail, count)`
- `hz3_central_pop_batch(shard, sc, out[], want)`

### Box: SegmentAllocCacheBox (optional, later)

Short-lived segment reuse cache (per-shard) to avoid mmap/madvise churn.
Not required for first TC integration.

## Integration Points (Boundaries)

### Alloc slow path (medium 4KB-32KB)

Order:
1) inbox drain (existing)
2) TC pop (new)
3) central pop batch (existing)
4) segment alloc (existing)

Pseudo:
```
Hz3BatchChain chain;
if (hz3_tc_pop(shard, sc, &chain)) {
    return hz3_chain_to_bin(&chain, bin);
}
got = hz3_central_pop_batch(...);
if (got > 0) { ... }
return hz3_alloc_from_segment(...);
```

### Alloc slow path (small v2)

Order:
1) TC pop (new)
2) central pop batch (existing)
3) page alloc (existing)

### Free (local)

Hot path remains TLS bin push. No change.

### Bin overflow / epoch trim (event-only)

Excess objects are converted into a BatchChain and pushed to TC.
If TC full, push to central.

### Remote free

Outbox/inbox stays as is. When inbox drain fills bin, overflow goes to TC.

## Invariants / Fail-Fast (debug only)

- `chain.count > 0 && chain.count <= HZ3_TC_BATCH`
- `chain.head != NULL && chain.tail != NULL`
- TC slot count never exceeds `HZ3_TC_SLOTS`
- On debug builds, assert that `sc`/`owner` match for all objects in chain.

## Compile-Time Gates

```
#ifndef HZ3_TC_ENABLE
#define HZ3_TC_ENABLE 0
#endif

#ifndef HZ3_TC_SLOTS
#define HZ3_TC_SLOTS 32
#endif

#ifndef HZ3_TC_BATCH
#define HZ3_TC_BATCH HZ3_REFILL_BATCH[sc]
#endif
```

Optional stats (slow-path only):
- `HZ3_TC_POP_HIT / MISS`
- `HZ3_TC_PUSH_OK / FULL`

## A/B Plan (Safe Rollout)

1) **S13-0**: Add data structures and no-op functions, gated OFF.
2) **S13-1**: Integrate TC in slow alloc paths only (no bin overflow yet).
3) **S13-2**: Integrate bin overflow/epoch trim to TC.
4) **S13-3**: Add stats and SSOT runs.

GO/NO-GO:
- GO: small/medium/mixed non-regression and shared path CPU drop.
- NO-GO: regressions or TC full rate too high.

## Testing

- `bench_random_mixed_malloc_args` (small/medium/mixed)
- `bench_random_mixed_mt_remote_malloc` (remote stress)
- `larson_local_hygienic` (4KB-32KB)

## Notes

This design keeps hot path untouched and only replaces shared path structure.
It is designed to sit cleanly on current hz3 boxes with minimal coupling.
