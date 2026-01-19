# S147: Reverse Mailbox (Pull-based Remote Free)

## Status: Research Box (Pending A/B Test)

## Background

S146-B (CAS Batch Push) was NO-GO:
- CAS itself is lightweight (failure rate 0.002%)
- Link construction overhead exceeded CAS reduction benefit

perf analysis results:
- `hz3_owner_stash_push_one`: 16.38% of cycles
- `lock cmpxchg`: 96.95% of push_one time (memory latency)

The bottleneck is **memory latency** in the atomic operation, not the CAS retry logic.

## Objective

Switch from Push型 to Pull型:
- **Push型 (current)**: Producer → CAS → Owner's stash (atomic on every object)
- **Pull型 (S147)**: Producer → TLS outbox → Owner polls (atomic only at threshold)

## Design

### Current (Push)
```
Producer: CAS push to owner's stash    ← atomic per object
Owner:    pop from own stash
```

### Proposed (Pull = Reverse Mailbox)
```
Producer: write to MY TLS outbox       ← no atomic
          atomic_store(notify, 1)      ← only at threshold
Owner:    scan notified outboxes
          atomic_exchange(items)       ← batch collection
```

### Performance Model

| Operation | Current | S147 |
|-----------|---------|------|
| Producer atomic | CAS per object | 1 store per batch |
| Owner scan | none | O(threads) |
| Contention | Producer-Producer | none |

### Memory Layout

**Optimized design**: `outbox[owner]` not `outbox[owner][sc]`

The naive `outbox[owner][sc]` layout explodes:
- 63 shards × 128 sc × 16 items × 8B = **1MB per thread**

Optimized:
- 63 shards × (16 entries × 16B + overhead) = **~17KB per thread**
- Entry contains (ptr, sc) so sc routing happens at drain time

### Concurrency Protocol

```
notify flag states:
  0 = empty / producer can write
  1 = has items / owner may drain

Producer:
  if (notify == 1) → fallback to owner_stash
  write item
  if (count >= threshold) → notify = 1

Owner:
  for each remote outbox where notify == 1:
    notify = 0 (via exchange, gains exclusive access)
    read items
    clear count
```

This avoids data races:
- Producer doesn't write while Owner reads (notify==1 blocks Producer)
- Owner doesn't read stale data (exchange gives acquire semantics)

## Files

| File | Purpose |
|------|---------|
| `hz3_s147_reverse_mailbox.h` | Feature flags, structs, API |
| `hz3_s147_reverse_mailbox.c` | Implementation |
| `hz3_s147_patch.inc` | Mainline integration guide |
| `README.md` | This document |

## Integration

See `hz3_s147_patch.inc` for detailed integration instructions.

Summary of required patches:
1. `hz3_config_scale.h`: Add S147 flags
2. `hz3_types.h`: Add TLS fields (s147_outbox, s147_tid)
3. `hz3_tcache.c`: Thread registration/cleanup
4. `hz3_owner_stash_push.inc`: Producer hook (try S147 first)
5. `hz3_owner_stash_pop.inc`: Owner hook (pull from S147 first)
6. `Makefile`: Include S147 source

## A/B Test

### Build

```bash
# A: baseline (S147 OFF)
make -C hakozuna/hz3 clean all_ldpreload_scale

# B: S147 ON
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S147_REVERSE_MAILBOX=1 \
    -DHZ3_S147_TID_FIELD=1 -DHZ3_S147_OUTBOX_FIELD=1'
```

### Benchmark

```bash
# Target workload: T=32/R=90
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  32 2500000 400 16 2048 90 262144

# Regression check: R=0
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  32 2500000 400 16 2048 0 262144
```

### Success Criteria

| Metric | Target |
|--------|--------|
| Throughput (R=90) | +5% or better |
| R=0 regression | ±5% |
| push_one cycles | significant reduction |

## Risk Analysis

1. **Thread scan overhead**: O(threads) scan per pop
   - Mitigation: Only process notified outboxes (quick empty check)

2. **Latency increase**: Items delayed until threshold
   - Mitigation: Flush on thread exit, tuneable threshold

3. **Memory increase**: ~17KB per thread
   - Acceptable given current TLS usage

4. **SC filtering**: Drain takes all sc, must route non-matching
   - Solution: Push non-matching to owner_stash during drain

## Expected Outcome

If successful:
- Producer side: No CAS per object, only threshold store
- Throughput: +5-15% on R=90% workloads
- Fixed cost: Minimal impact (scan is O(threads) but fast)

If NO-GO:
- Thread scan overhead exceeds CAS savings
- Archive and document findings

---

## Results: NO-GO

### S147-0 Results (2025-01-19)

Initial implementation with O(threads) scan + notify flag:
- T=32/R=90: **-50%** regression (notify race + scan overhead)
- Abandoned due to fundamental design flaw in scan model

### S147-2 Results (2025-01-19)

Improved version with 2-buffer + pending bitset:

| Condition | Baseline | S147-2 | Delta |
|-----------|----------|--------|-------|
| T=4 R=0 | 243.9M | 232.6M | **-4.6%** |
| T=4 R=90 | 106.9M | 95.4M | **-10.8%** |
| T=32 R=90 | 146-266M | 64M | **-50%+** |

### Failure Analysis

1. **Rate mismatch**: Producer fill rate >> Owner drain rate
   - Producers fill buffers faster than owners can drain
   - Steady state: both buffers always full

2. **Fallback rate 47%**: Even with 2-buffer design, both buffers fill quickly
   - Falls back to `owner_stash_push_one` (original CAS path)
   - Defeats the purpose of the optimization

3. **Scan cost at T=32**: Pending bitset scan overhead dominates
   - O(threads) scan per pop becomes significant
   - Memory latency in scan loop

### Root Cause

The "reverse mailbox" model fundamentally assumes:
- Producer writes are bursty and owner polls are frequent
- Reality: owner polls are rate-limited by workload, producers are continuous

This creates a steady-state where outboxes are always full, forcing fallback.

### Verdict: NO-GO

- S147-0: Abandoned (design flaw)
- S147-2: NO-GO (rate mismatch, -10% at T=4/R=90)
- Code removed from mainline, archived here
