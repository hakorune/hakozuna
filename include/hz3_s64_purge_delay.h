#pragma once

#include "hz3_config.h"
#include <stdint.h>

#if HZ3_S64_PURGE_DELAY

// ============================================================================
// S64-B: PurgeDelayBox - delayed madvise(DONTNEED) for retired pages
// ============================================================================
//
// Retired pages are enqueued with their retire_epoch.
// Pages are purged when (current_epoch - retire_epoch) >= DELAY_EPOCHS.
// This reduces re-fault churn for recently-retired pages.
//
// Structure: per-TLS ring buffer (no cross-thread sharing)
// Thread boundary: my_shard fixed (TLS-based)

// Purge entry (16 bytes)
typedef struct {
    void*    seg_base;       // segment base address
    uint16_t page_idx;       // page index in segment
    uint16_t pages;          // consecutive pages (1 for small)
    uint32_t retire_epoch;   // epoch when retired
} Hz3S64PurgeEntry;

// Ring buffer for delayed purge
typedef struct {
    Hz3S64PurgeEntry entries[HZ3_S64_PURGE_QUEUE_SIZE];
    uint16_t head;           // next slot to read
    uint16_t tail;           // next slot to write
    uint32_t dropped;        // overflow counter (stats)
    uint32_t enqueued;       // total enqueued (stats)
    uint32_t purged;         // total purged (stats)
} Hz3S64PurgeQueue;

// Enqueue a retired page for delayed purge.
// Called by S64_RetireScanBox when a page is retired.
// Drops oldest if queue is full (ring buffer semantics).
void hz3_s64_purge_delay_enqueue(void* seg_base, uint16_t page_idx, uint16_t pages);

// Process delayed purge queue.
// Called from hz3_epoch_force() after S64_RetireScanBox.
void hz3_s64_purge_delay_tick(void);

#else  // !HZ3_S64_PURGE_DELAY

static inline void hz3_s64_purge_delay_enqueue(void* seg_base, uint16_t page_idx, uint16_t pages) {
    (void)seg_base; (void)page_idx; (void)pages;
}

static inline void hz3_s64_purge_delay_tick(void) {}

#endif  // HZ3_S64_PURGE_DELAY
