#pragma once

#include "hz3_types.h"
#include <stdatomic.h>

// ============================================================================
// Inbox: MPSC stack for remote free
// Day 4: producer threads push via CAS, owner thread drains via atomic_exchange
// ============================================================================

typedef struct {
    _Atomic(void*) head;
    _Atomic uint32_t count;  // approximate count (for stats)
} Hz3Inbox;

// Global inbox pool: [owner_shard][sc]
extern Hz3Inbox g_hz3_inbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];

// ============================================================================
// Inbox API
// ============================================================================

// Push a linked list to inbox (thread-safe, CAS-based)
// head→tail in FIFO order, n = number of objects
void hz3_inbox_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);

// Drain inbox to local bin (called by owner thread)
// Returns first object (for immediate use), remaining pushed to bin
// Respects HZ3_BIN_HARD_CAP limit
// Day 5: Overflow goes to central (not back to inbox)
void* hz3_inbox_drain(uint8_t shard, int sc, Hz3Bin* bin);

// Drain inbox directly to central (event-only).
// Used by memory budget reclaim to make central-pop possible.
uint32_t hz3_inbox_drain_to_central(uint8_t shard, int sc);

// Initialize inbox pool (thread-safe via pthread_once)
void hz3_inbox_init(void);
