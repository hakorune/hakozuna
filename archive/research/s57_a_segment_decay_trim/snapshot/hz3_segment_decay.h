#pragma once

// S57-A: Segment Decay Trim
// Return fully-free segments to OS via munmap after grace period

#include "hz3_config.h"
#include <stdbool.h>

#if HZ3_S57_DECAY_TRIM

#include "hz3_types.h"

// Enqueue a fully-free segment for decay.
// Called from hz3_segment_free_run() when free_pages == PAGES_PER_SEG.
// Thread-safety: Must be called by segment owner only.
void hz3_segment_decay_enqueue(Hz3SegMeta* meta);

// Epoch tick: process decay queue, munmap expired segments.
// Called from hz3_epoch_force().
// Thread-safety: Processes only my_shard's queue.
void hz3_segment_decay_tick(void);

// Try to reuse a decaying segment for allocation.
// Returns segment if found and dequeued, NULL otherwise.
// Called from slow alloc path before allocating new segment.
// Thread-safety: Must be called by segment owner only.
Hz3SegMeta* hz3_segment_decay_try_reuse(uint8_t owner, uint16_t pages_needed);

// Dequeue a specific segment from decay queue (if in queue).
// Used when segment is about to be reused.
// Returns true if dequeued, false if not in queue.
// Thread-safety: Must be called by segment owner only.
bool hz3_segment_decay_dequeue(Hz3SegMeta* meta);

// Get current epoch counter (for decay timing)
uint32_t hz3_decay_epoch_now(void);

#endif  // HZ3_S57_DECAY_TRIM
