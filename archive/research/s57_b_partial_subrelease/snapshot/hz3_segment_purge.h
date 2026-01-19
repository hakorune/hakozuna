// hakozuna/hz3/include/hz3_segment_purge.h
//
// S57-B: Partial Subrelease API
// Return contiguous free page ranges to OS via madvise(DONTNEED)
//
// Design:
// - Boundary: hz3_segment_purge_tick() called from hz3_epoch_force()
// - Candidate: pack pool segments, cold (last_touch_epoch expired), not active
// - Purge: free_bits & ~purged_bits → contiguous ranges → madvise(DONTNEED)

#pragma once

#include "hz3_config.h"

#if HZ3_S57_PARTIAL_PURGE

#include "hz3_types.h"
#include <stdint.h>

// ============================================================================
// API: Epoch Tick Entry Point
// ============================================================================

// Process pack pool segments and purge cold free ranges.
// Called from hz3_epoch_force() (event-only, budgeted).
void hz3_segment_purge_tick(void);

// ============================================================================
// API: Touch Tracking (called from alloc_run / free_run)
// ============================================================================

// Update last_touch_epoch on segment access.
// Should be inlined into alloc_run / free_run for minimal overhead.
static inline void hz3_segment_touch(Hz3SegMeta* meta, uint32_t epoch) {
    if (meta) {
        meta->last_touch_epoch = epoch;
    }
}

// Clear purged_bits for the given range (pages were re-allocated).
// Called from alloc_run to mark pages as "need re-purge after next free".
void hz3_segment_clear_purged_range(Hz3SegMeta* meta, uint16_t start_page, uint16_t pages);

// ============================================================================
// API: Initialization (called from hz3_segment_init)
// ============================================================================

// Initialize S57-B fields in segment metadata.
void hz3_segment_purge_init_meta(Hz3SegMeta* meta);

#endif  // HZ3_S57_PARTIAL_PURGE
