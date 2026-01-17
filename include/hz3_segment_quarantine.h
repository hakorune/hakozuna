#pragma once

// ============================================================================
// S47: Segment Quarantine
// ============================================================================
//
// Deterministic drain: focus on ONE segment until it becomes fully free.
// - Scan: sample from central, pick segment with most reclaimable pages
// - Quarantine: draining_seg doesn't allocate new runs
// - Drain: only return runs belonging to draining_seg
// - TTL: release draining_seg if it doesn't become free within MAX_EPOCHS

#include "hz3_config.h"
#include "hz3_types.h"

#if HZ3_S47_SEGMENT_QUARANTINE

// ----------------------------------------------------------------------------
// Draining state (per shard)
// ----------------------------------------------------------------------------
// draining_seg: segment being drained (0 = none)
// draining_epoch: epoch when draining started (for TTL)

// Get current draining segment for shard (0 = none)
void* hz3_s47_get_draining_seg(uint8_t shard);

// Check if segment is currently being drained
int hz3_s47_is_draining(uint8_t shard, void* seg_base);

// ----------------------------------------------------------------------------
// Compact operations (called from arena_alloc)
// ----------------------------------------------------------------------------

// Soft compact: called when headroom threshold reached
// - Scan central, select best segment, start draining
// - Single drain pass
void hz3_s47_compact_soft(uint8_t shard);

// Hard compact: called when slot allocation failed
// - More aggressive scan/drain budget
// - Multiple drain passes
void hz3_s47_compact_hard(uint8_t shard);

// ----------------------------------------------------------------------------
// Drain operation (called from pressure handler)
// ----------------------------------------------------------------------------

// Drain runs from central to draining_seg
// Returns number of pages freed
uint32_t hz3_s47_drain(uint8_t shard, uint32_t budget);

// ----------------------------------------------------------------------------
// TTL management
// ----------------------------------------------------------------------------

// Check and release draining_seg if TTL expired
void hz3_s47_check_ttl(uint8_t shard);

// ----------------------------------------------------------------------------
// S47-PolicyBox: adaptive parameter adjustment
// ----------------------------------------------------------------------------

// Policy modes
typedef enum {
    HZ3_S47_POLICY_FROZEN = 0,   // Fixed parameters (compile-time values)
    HZ3_S47_POLICY_OBSERVE = 1,  // Log events only (no adjustment)
    HZ3_S47_POLICY_LEARN = 2     // Adaptive adjustment
} Hz3S47PolicyMode;

// Initialize policy (pthread_once safe)
void hz3_s47_policy_init(void);

// Event handlers (called from arena_alloc)
void hz3_s47_policy_on_alloc_full(void);    // slot allocation failed
void hz3_s47_policy_on_alloc_failed(void);  // final allocation failed
void hz3_s47_policy_on_wait_success(void);  // succeeded after bounded wait

// Parameter getters (thread-safe, atomic reads)
uint32_t hz3_s47_policy_get_headroom_slots(void);
uint32_t hz3_s47_policy_get_panic_wait_us(void);
uint32_t hz3_s47_policy_get_drain_passes_hard(void);

// ----------------------------------------------------------------------------
// S47-4: Snapshot metrics (for triage)
// ----------------------------------------------------------------------------

// Get max_potential observed in scan_and_select
uint32_t hz3_s47_get_max_potential(void);

// Get pinned_candidates count (near-full but hopeless)
uint32_t hz3_s47_get_pinned_candidates(void);

// Get avoid_hits count
uint32_t hz3_s47_get_avoid_hits(void);

// Get avoid_used count (per shard, returns total)
uint32_t hz3_s47_get_avoid_used(void);

#endif  // HZ3_S47_SEGMENT_QUARANTINE
