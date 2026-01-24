#pragma once
// hz3_release_ledger.h - S65 ReleaseLedgerBox
//
// TLS ring buffer with range coalescing for deferred madvise(DONTNEED).
// Called by hz3_release_range_definitely_free() when HZ3_S65_RELEASE_LEDGER=1.

#include "hz3_config.h"
#include "hz3_release_boundary.h"
#include <stdint.h>
#include <stdbool.h>

#if HZ3_S65_RELEASE_LEDGER

// Power-of-2 enforcement for ring buffer
_Static_assert((HZ3_S65_LEDGER_SIZE & (HZ3_S65_LEDGER_SIZE - 1)) == 0,
               "HZ3_S65_LEDGER_SIZE must be power of 2");
#define HZ3_S65_LEDGER_MASK (HZ3_S65_LEDGER_SIZE - 1)

// ============================================================================
// Data Structures
// ============================================================================

// Ledger entry (20 bytes, packed to 24 with alignment)
typedef struct {
    void*    seg_base;       // segment base address
    uint32_t page_idx;       // page index in segment
    uint16_t pages;          // consecutive pages
    uint16_t reason;         // Hz3ReleaseReason
    uint32_t retire_epoch;   // epoch when enqueued
} Hz3S65ReleaseEntry;

// Per-TLS ring buffer (no cross-thread sharing)
typedef struct {
    Hz3S65ReleaseEntry entries[HZ3_S65_LEDGER_SIZE];
    uint16_t head;           // next slot to read
    uint16_t tail;           // next slot to write
    // Stats
    uint64_t enqueued;       // total enqueued
    uint64_t coalesced;      // total coalesce merges
    uint64_t purged_pages;   // total pages purged
    uint64_t madvise_calls;  // total madvise calls
    uint64_t dropped;        // overflow dropped
    uint64_t idle_ticks;     // ticks in idle mode
    uint64_t busy_ticks;     // ticks in busy mode
} Hz3S65ReleaseLedger;

// ============================================================================
// Idle/Busy Gate Helper
// ============================================================================

#if HZ3_S65_IDLE_GATE
#include "hz3_segment_packing.h"

// Returns true if system is idle (no pressure)
static inline bool hz3_s65_is_idle(void) {
    return !hz3_pack_pressure_active();
}
#else
static inline bool hz3_s65_is_idle(void) {
    return true;  // Always idle when gate disabled
}
#endif

// ============================================================================
// API
// ============================================================================

// Enqueue a released range for deferred purge.
// Called by hz3_release_range_definitely_free().
// Attempts to coalesce with adjacent entries in same segment.
void hz3_s65_ledger_enqueue(void* seg_base, uint32_t page_idx, uint32_t pages, Hz3ReleaseReason reason);

// Process ledger and purge aged entries.
// Called from hz3_epoch_force() after S64 work.
// delay_epochs: how many epochs to wait before purging
// budget_pages: max pages to purge this tick
void hz3_s65_ledger_purge_tick(uint32_t delay_epochs, uint32_t budget_pages);

// Convenience wrapper using idle/busy gate
void hz3_s65_ledger_purge_tick_gated(void);

// Stats dump (atexit)
void hz3_s65_ledger_dump_stats(void);

// Flush entire TLS ledger (ignore delay, drain all entries).
// Called from tcache destructor to ensure worker threads drain before exit.
void hz3_s65_ledger_flush_all(void);

#else  // !HZ3_S65_RELEASE_LEDGER

// Stubs when disabled
static inline void hz3_s65_ledger_enqueue(void* seg_base, uint32_t page_idx, uint32_t pages, Hz3ReleaseReason reason) {
    (void)seg_base; (void)page_idx; (void)pages; (void)reason;
}

static inline void hz3_s65_ledger_purge_tick(uint32_t delay_epochs, uint32_t budget_pages) {
    (void)delay_epochs; (void)budget_pages;
}

static inline void hz3_s65_ledger_purge_tick_gated(void) {}

static inline void hz3_s65_ledger_dump_stats(void) {}

static inline void hz3_s65_ledger_flush_all(void) {}

#endif  // HZ3_S65_RELEASE_LEDGER
