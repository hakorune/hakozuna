#pragma once

#include "hz3_types.h"
#include <stdatomic.h>
#include <stdbool.h>

// ============================================================================
// Inbox: MPSC stack for remote free
// Day 4: producer threads push via CAS, owner thread drains via atomic_exchange
// ============================================================================

typedef struct {
    _Atomic(void*) head[HZ3_S195_MEDIUM_INBOX_SHARDS];
    _Atomic uint32_t count[HZ3_S195_MEDIUM_INBOX_SHARDS];  // approximate count
    _Atomic uint32_t seq;                                   // push sequence
    _Atomic uint32_t drain_seq;                             // non-empty drain sequence
} Hz3Inbox;

typedef struct {
    _Atomic(void*) head[HZ3_S195_MEDIUM_INBOX_SHARDS];
    _Atomic uint32_t count[HZ3_S195_MEDIUM_INBOX_SHARDS];  // approximate count
    _Atomic uint32_t seq;                                   // push sequence
    _Atomic uint32_t drain_seq;                             // non-empty drain sequence
} Hz3InboxMixed;

// Global inbox pool: [owner_shard][sc]
extern Hz3Inbox g_hz3_inbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];
// Global mixed inbox pool: [owner_shard]
extern Hz3InboxMixed g_hz3_inbox_medium_mixed[HZ3_NUM_SHARDS];

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

// Hint-gated drain (opt-in): returns NULL quickly when inbox appears empty.
void* hz3_inbox_drain_if_nonempty(uint8_t shard, int sc, Hz3Bin* bin);

// Event-gated drain (S200):
// - Returns NULL when no new inbox push is observed since *last_seq.
// - Updates *last_seq when an event is observed.
void* hz3_inbox_drain_if_seq_advanced(uint8_t shard, int sc, Hz3Bin* bin,
                                      uint32_t* last_seq);

// Drain inbox directly to central (event-only).
// Used by memory budget reclaim to make central-pop possible.
uint32_t hz3_inbox_drain_to_central(uint8_t shard, int sc);

// Initialize inbox pool (thread-safe via pthread_once)
void hz3_inbox_init(void);

// S204: Peek check for diagnostics (non-destructive)
// Returns true if inbox appears non-empty.
bool hz3_inbox_peek_check(uint8_t shard, int sc);

// Approximate queued object count (sum of lane counters).
uint32_t hz3_inbox_count_approx(uint8_t shard, int sc);

// Load non-empty drain sequence for owner/shard+sc.
uint32_t hz3_inbox_drain_seq_load(uint8_t shard, int sc);

// S206B: mixed medium inbox (dst-only path)
void hz3_inbox_medium_mixed_push_list(uint8_t owner, void* head, void* tail, uint32_t n);
void* hz3_inbox_medium_mixed_drain(uint8_t shard, int sc, Hz3Bin* bin, uint32_t budget_objs);
