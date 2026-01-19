# PHASE_HZ3_S57-A: Segment Decay Trim (Fully-Free Segment Return)

## Status: ❌ NO-GO (steady-state RSS)

**Date**: 2026-01-08
**Track**: hz3
**Previous**: S56 NO-GO, S55-3B NO-GO
**Goal**: mstress mean RSS -10% vs baseline
**Result**: -0.32% (noise level)
**Commit**: `848267d88`

---

## Results (2026-01-08)

### RSS Measurement (mstress T=10, L=100, I=100, RUNS=10)

| Metric | Baseline (S57-A OFF) | Treatment (S57-A ON) | Change |
|--------|---------------------|---------------------|--------|
| Median peak RSS | 927,334 KB | 924,336 KB | **-0.32%** |

**Verdict**: ❌ NO-GO for -10% target

### SSOT Regression Check (RUNS=5)

| Benchmark | hz3 (ops/s) | Status |
|-----------|-------------|--------|
| small | ~104M | ✅ No regression |
| medium | ~107M | ✅ No regression |
| mixed | ~100M | ✅ No regression |

**Verdict**: ✅ OK (no throughput regression)

### Root Cause Analysis

S57-A only triggers when segments become **completely** empty (`free_pages == PAGES_PER_SEG`).
In steady-state workloads like mstress:

1. Allocations continuously land on existing segments
2. Segments never become fully free
3. Therefore, nothing gets munmap'd

This is the fundamental limitation of "fully-free segment decay" in steady-state workloads.

### Conclusion

S57-A provides value for:
- Burst workloads that produce fully-free segments
- Shutdown cleanup
- Memory pressure recovery

But **NOT** for steady-state RSS reduction (which requires S57-B: Partial Subrelease).

### Next Steps

1. **Keep S57-A as default OFF**: Opt-in for specific workloads
2. **Proceed to S57-B**: Use madvise(DONTNEED) on contiguous free page ranges within segments

---

## Background

### S56 NO-GO Root Cause

> "選び方を変えても、返さなければRSSは下がらない"
>
> Pack pool は「既に開いた segment の墓場」を回しているだけ。
> OS に返す行為（munmap）が発生しない限り、RSS は下がらない。

### S57 Insight

**tcmalloc/jemalloc/mimalloc が RSS を下げられる根本**:
- steady-state でも少しずつ返す（alloc_failed のときだけではない）
- tick で budget 付きで返す

**S57-A**: Fully-free segment を grace period 後に munmap

---

## Design

### 1. Data Structure

**Hz3SegMeta 追加フィールド** (`hz3_types.h`):

```c
#if HZ3_S57_DECAY_TRIM
    struct Hz3SegMeta* decay_next;  // decay queue linked list
    uint32_t decay_epoch;           // epoch when segment became fully free
    uint8_t  decay_state;           // HZ3_SEG_ACTIVE / HZ3_SEG_DECAYING
#endif
```

**Per-shard decay queue head** (`hz3_segment_decay.c`):

```c
static Hz3SegMeta* g_decay_head[HZ3_NUM_SHARDS];
static Hz3SegMeta* g_decay_tail[HZ3_NUM_SHARDS];  // O(1) enqueue
```

### 2. State Machine

```
ACTIVE ─────────────────────────────────────────────> (in use)
   │
   │ free_run() && free_pages == PAGES_PER_SEG
   │
   ▼
DECAYING ──────────────────────────────────────────> (in decay queue)
   │
   │ decay_tick() && epoch - decay_epoch >= GRACE_EPOCHS
   │
   ▼
(munmap) ─────────────────────────────────────────> (returned to OS)
```

### 3. API

**New file**: `hakozuna/hz3/src/hz3_segment_decay.c`

```c
// Enqueue a fully-free segment for decay
// Called from hz3_segment_free_run() when free_pages == PAGES_PER_SEG
void hz3_segment_decay_enqueue(Hz3SegMeta* meta);

// Epoch tick: process decay queue, munmap expired segments
// Called from hz3_epoch_force()
void hz3_segment_decay_tick(void);

// Safety check: can this segment be munmapped?
// Returns true if all conditions met
static bool hz3_segment_can_munmap(Hz3SegMeta* meta);

// Re-activate a decaying segment (if reused before grace period)
// Called from hz3_segment_alloc_run() if segment is DECAYING
bool hz3_segment_decay_dequeue(Hz3SegMeta* meta);
```

### 4. Integration Points

**4.1. hz3_segment.c: hz3_segment_free_run()**

```c
void hz3_segment_free_run(Hz3SegMeta* meta, size_t start_page, size_t pages) {
    // ... existing code ...

#if HZ3_S57_DECAY_TRIM
    // S57-A: Enqueue for decay if segment is now fully free
    if (meta->free_pages == HZ3_PAGES_PER_SEG) {
        hz3_segment_decay_enqueue(meta);
    }
#endif
}
```

**4.2. hz3_epoch.c: hz3_epoch_force()**

```c
#if HZ3_S57_DECAY_TRIM
    // S57-A: Segment decay (return fully-free segments to OS)
    hz3_segment_decay_tick();
#endif
```

**4.3. hz3_tcache.c: segment reuse path**

When allocating a new run, check if we can dequeue from decay queue first:

```c
// In hz3_alloc_slow() or run allocation path
#if HZ3_S57_DECAY_TRIM
    // Try to reuse decaying segment before allocating new
    Hz3SegMeta* reused = hz3_segment_decay_try_reuse(my_shard, pages_needed);
    if (reused) {
        // Use reused segment
    }
#endif
```

### 5. Config Flags (`hz3_config.h`)

```c
// S57-A: Segment Decay Trim
#ifndef HZ3_S57_DECAY_TRIM
#define HZ3_S57_DECAY_TRIM 0  // default OFF
#endif

#ifndef HZ3_S57_DECAY_GRACE_EPOCHS
#define HZ3_S57_DECAY_GRACE_EPOCHS 8  // wait 8 epochs before munmap
#endif

#ifndef HZ3_S57_DECAY_BUDGET_SEGS
#define HZ3_S57_DECAY_BUDGET_SEGS 4  // max segments to munmap per tick per shard
#endif
```

### 6. Safety Conditions (CRITICAL)

**hz3_segment_can_munmap()** must ensure:

1. `free_pages == HZ3_PAGES_PER_SEG` (all pages free)
2. `decay_state == HZ3_SEG_DECAYING` (in decay queue)
3. `epoch_now - decay_epoch >= HZ3_S57_DECAY_GRACE_EPOCHS` (grace period passed)

**Grace period rationale**:
- inbox/outbox may have in-flight pointers to objects in segment
- grace period ensures all in-flight operations complete
- 8 epochs (~8 slow path calls) is conservative first estimate

**Note**: `remote_ref_count` is NOT required because:
- hz3 uses owner-shard model: segment owner handles all frees
- remote frees go through inbox/outbox, drained in epoch
- grace period covers inbox drain completion

### 7. Stats (Observability)

```c
// Stats counters (atomic, event-only)
static _Atomic uint64_t g_s57_enqueued = 0;       // segments entered decay queue
static _Atomic uint64_t g_s57_trimmed = 0;        // segments munmapped
static _Atomic uint64_t g_s57_reused = 0;         // segments reused from decay queue
static _Atomic uint64_t g_s57_skipped_young = 0;  // grace period not passed

// atexit handler
static void hz3_s57_atexit(void) {
    fprintf(stderr, "[HZ3_S57_STATS] enqueued=%lu trimmed=%lu reused=%lu skipped_young=%lu\n",
            atomic_load(&g_s57_enqueued),
            atomic_load(&g_s57_trimmed),
            atomic_load(&g_s57_reused),
            atomic_load(&g_s57_skipped_young));
}
```

---

## Implementation Order

1. **hz3_config.h**: Add S57 flags
2. **hz3_types.h**: Add decay fields to Hz3SegMeta
3. **hz3_segment_decay.h**: Header (API declarations)
4. **hz3_segment_decay.c**: Implementation
5. **hz3_segment.c**: Integration (hz3_segment_free_run)
6. **hz3_epoch.c**: Integration (hz3_epoch_force)
7. **Makefile**: Add hz3_segment_decay.c to build

---

## GO Conditions

### Measurement Protocol

| Parameter | Value |
|-----------|-------|
| Benchmark | mstress |
| Threads | 10 |
| Load | 100 (100%) |
| Iterations | 100 |
| RUNS | 10 |

### Success Criteria

- mstress mean RSS: **-10% or better** vs baseline
- mstress p95 RSS: **-5% or better** vs baseline (secondary)
- SSOT small/medium/mixed: **±2%** vs baseline (no regression)

---

## Files Changed

| File | Change |
|------|--------|
| `hakozuna/hz3/include/hz3_config.h` | Add S57 flags |
| `hakozuna/hz3/include/hz3_types.h` | Add decay fields to Hz3SegMeta |
| `hakozuna/hz3/include/hz3_segment_decay.h` | New header |
| `hakozuna/hz3/src/hz3_segment_decay.c` | New implementation |
| `hakozuna/hz3/src/hz3_segment.c` | Integration in free_run |
| `hakozuna/hz3/src/hz3_epoch.c` | Integration in epoch_force |
| `hakozuna/hz3/Makefile` | Add hz3_segment_decay.o |

---

## Next: S57-B (Partial Subrelease)

After S57-A is GO, proceed to S57-B:
- madvise(DONTNEED) on cold segment contiguous free ranges
- For segments that aren't fully free but have large free gaps
