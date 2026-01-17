#pragma once

// ============================================================================
// S60: PurgeRangeQueueBox
// ============================================================================
//
// Captures freed page ranges from S58 reclaim and applies budgeted madvise
// at epoch boundary. Unlike S57-B2 (segment scan), S60 queues the exact
// ranges that S58 returns to segments, ensuring full coverage.
//
// Safety:
// - No carryover: queue is drained completely each epoch (drop if over budget)
// - Arena bounds check before madvise
// - Budget limits: both call count and page count

#include "hz3_config.h"
#include <stdint.h>

#if HZ3_S60_PURGE_RANGE_QUEUE

// Queue a freed page range for later madvise.
// Called by S58 reclaim after hz3_segment_free_run().
// Thread-safe: TLS queue, no locking needed.
void hz3_s60_queue_range(void* seg_base, uint16_t page_idx, uint16_t pages);

// Drain queued ranges and apply madvise (budgeted).
// Called once per epoch at hz3_epoch_force() tail.
// Order: S58 reclaim -> S60 purge (SSOT).
void hz3_s60_purge_tick(void);

#endif  // HZ3_S60_PURGE_RANGE_QUEUE
