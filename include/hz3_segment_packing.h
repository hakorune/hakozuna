#pragma once

#include "hz3_config.h"
#include "hz3_types.h"

#if HZ3_S49_SEGMENT_PACKING

// ============================================================================
// S49: Segment Packing Box
// ============================================================================
//
// Boundary API for existing segment priority allocation.
// All operations are my_shard only (owner == caller's shard).
//
// Design:
// - Per-shard partial pool with pages-bucket structure
// - bucket = pages (1..HZ3_S49_PACK_MAX_PAGES)
// - pack_max_fit tracks max contiguous pages, decremented on failure
// - pack_in_list prevents double insertion

// ============================================================================
// Primary Entry Point (slow path only)
// ============================================================================

// Try to allocate from pack pool (my_shard only)
// Returns: segment meta with space, or NULL if not found
// On failure: decrements pack_max_fit and moves to lower bucket
Hz3SegMeta* hz3_pack_try_alloc(uint8_t owner, size_t npages, uint32_t tries);

// ============================================================================
// Event Handlers (my_shard only, pack_in_list prevents double insertion)
// ============================================================================

// segment_new() completed - add to pack pool
void hz3_pack_on_new(uint8_t owner, Hz3SegMeta* meta);

// segment_alloc_run() completed - update pack pool (remove if full)
void hz3_pack_on_alloc(uint8_t owner, Hz3SegMeta* meta);

// segment_free_run() completed - reset pack_max_fit and re-add to pack pool
void hz3_pack_on_free(uint8_t owner, Hz3SegMeta* meta);

// draining_seg selected - remove from pack pool
void hz3_pack_on_drain(uint8_t owner, Hz3SegMeta* meta);

// avoid list added - remove from pack pool
void hz3_pack_on_avoid(uint8_t owner, Hz3SegMeta* meta);

// avoid TTL expired - re-add to pack pool
void hz3_pack_on_avoid_expire(uint8_t owner, Hz3SegMeta* meta);

// S49-2: Reinsert segment with pre-set pack_max_fit (does NOT overwrite pack_max_fit)
// Used when alloc_run fails and caller has computed max_contig_free
void hz3_pack_reinsert(uint8_t owner, Hz3SegMeta* meta);

// ============================================================================
// Pressure Check Helper
// ============================================================================

// Check if arena pressure is active (existing epoch判定に寄せる)
static inline int hz3_pack_pressure_active(void) {
#if HZ3_ARENA_PRESSURE_BOX
    extern _Atomic(uint32_t) g_hz3_arena_pressure_epoch;
    // epoch が奇数 = pressure 中
    return (atomic_load_explicit(&g_hz3_arena_pressure_epoch,
            memory_order_relaxed) & 1) != 0;
#else
    return 0;
#endif
}

// ============================================================================
// Stats (event-only)
// ============================================================================

uint32_t hz3_pack_get_hits(void);
uint32_t hz3_pack_get_misses(void);

#endif  // HZ3_S49_SEGMENT_PACKING

// ============================================================================
// S54: SegmentPageScavengeBox OBSERVE API (PackBox境界)
// ============================================================================
//
// Rationale:
// - Observe pack pool to determine if segment free pages are RSS reduction target
// - PackBox API maintains g_pack_pool encapsulation (static in .c file)
// - OBSERVE mode: statistics only, no madvise execution
//
// Safety:
// - my_shard only (owner parameter)
// - Read-only traversal of pack pool
// - Event-only (S46 pressure handler), zero hot path cost

#if HZ3_SEG_SCAVENGE_OBSERVE
// Observe pack pool for owner shard (read-only, my_shard only)
// Outputs:
// - out_segments: count of segments in pack pool
// - out_free_pages: total free pages across all segments
// - out_candidate_pages: pages eligible for madvise (contiguous >= MIN_CONTIG_PAGES)
void hz3_pack_observe_owner(uint8_t owner,
                             uint32_t* out_segments,
                             uint32_t* out_free_pages,
                             uint32_t* out_candidate_pages);
#else
// No-op stub when OBSERVE is disabled
static inline void hz3_pack_observe_owner(uint8_t owner,
                                           uint32_t* out_segments,
                                           uint32_t* out_free_pages,
                                           uint32_t* out_candidate_pages) {
    (void)owner; (void)out_segments; (void)out_free_pages; (void)out_candidate_pages;
}
#endif
