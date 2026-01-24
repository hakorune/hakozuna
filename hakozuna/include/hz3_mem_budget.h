#pragma once

// ============================================================================
// S45: Memory Budget Box - Segment-level reclaim on arena exhaustion
// ============================================================================
//
// When arena_alloc() fails (all slots used), this module scans for fully-free
// segments (free_pages == HZ3_PAGES_PER_SEG) and releases them back to the OS,
// freeing arena slots for reuse.
//
// Phase 1: Segment-level reclaim only
// - No run-level tracking (deferred to Phase 2)
// - Minimal hot path impact (reclaim only on failure)
// - scale lane only (HZ3_MEM_BUDGET_ENABLE=1)
//

#include "hz3_config.h"
#include <stddef.h>
#include <stdint.h>

// S45-FOCUS: Progress info for multi-pass termination (shared by both lanes)
typedef struct {
    uint16_t sampled;       // number of objects sampled from central
    uint16_t freed_pages;   // pages returned to segment (may not reach 512)
    size_t   freed_bytes;   // HZ3_SEG_SIZE if segment released, 0 otherwise
} Hz3FocusProgress;

#if HZ3_MEM_BUDGET_ENABLE

// Phase 1: arena_alloc() calls this on failure. Scans arena for fully-free
// segments and releases them. Returns total bytes reclaimed.
size_t hz3_mem_budget_reclaim_segments(size_t target_bytes);

// S55-2B: Budgeted segment scan (epoch-safe, O(scan_slots_budget) fixed cost)
// - Scans up to scan_slots_budget arena slots from cursor (round-robin)
// - Returns fully-free segments to OS
// - Returns total bytes reclaimed
// - Safe for epoch: no full arena scan, predictable cost
size_t hz3_mem_budget_reclaim_segments_budget(size_t target_bytes, uint32_t scan_slots_budget);

// Phase 2: Pop medium runs from central pool and return them to their segments
// via hz3_segment_free_run(). This increases segment free_pages, enabling
// Phase 1 reclaim. Returns pages reclaimed.
//
// CRITICAL: my_shard のみ操作。他 shard は触らない。
// Reason: hz3_segment_free_run() はロックを持たず、free_bits/free_pages を
// 直接変更する。他スレッドの segment を触るとデータレースになる。
size_t hz3_mem_budget_reclaim_medium_runs(void);

// S55-3: Budgeted medium runs reclaim with subrelease
// - Pops up to max_reclaim_pages from central pool (my_shard only)
// - Returns runs to segment via hz3_segment_free_run()
// - Applies madvise(MADV_DONTNEED) to reclaimed runs
// - Returns: pages actually reclaimed and released to OS
size_t hz3_mem_budget_reclaim_medium_runs_subrelease(size_t max_reclaim_pages);

// NOTE: S55-3B delayed purge is archived; no delayed purge API in mainline.

// Emergency flush (event-only): push current thread's medium bins/inbox to central.
// Used right before reclaim to make central-pop possible on arena exhaustion.
void hz3_mem_budget_emergency_flush(void);

// S45-FOCUS: Focused reclaim - concentrate on ONE segment to reach free_pages==512.
// Samples objects from central, groups by seg_base, picks best (most pages_sum + owner match).
// Returns progress info for multi-pass termination decision.
Hz3FocusProgress hz3_mem_budget_reclaim_focus_one_segment(void);

#else

// Stubs for fast lane (HZ3_MEM_BUDGET_ENABLE=0)
static inline size_t hz3_mem_budget_reclaim_segments(size_t target_bytes) {
    (void)target_bytes;
    return 0;
}

static inline size_t hz3_mem_budget_reclaim_medium_runs(void) {
    return 0;
}

static inline void hz3_mem_budget_emergency_flush(void) {
}

static inline Hz3FocusProgress hz3_mem_budget_reclaim_focus_one_segment(void) {
    return (Hz3FocusProgress){0, 0, 0};
}

#endif
