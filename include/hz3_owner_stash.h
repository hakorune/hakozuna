#pragma once

// S44: Owner Shared Stash (MPSC per-owner/per-sc)
//
// Purpose:
// - Remote free objects accumulate in owner's stash (no mutex, atomic push)
// - Alloc miss pops from own stash before hitting central (mutex avoidance)
//
// Insertion order (alloc miss):
//   xfer (S42) -> owner_stash (S44) -> central -> page
//
// Design:
// - MPSC: multiple producers (remote free), single consumer (alloc miss)
// - Atomic head (either object list or packet list, compile-time selected)
// - Scale lane only (requires HZ3_REMOTE_STASH_SPARSE)

#include "hz3_config.h"
#include <stdint.h>
#include <stdatomic.h>

#if HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE

// MPSC bin (per owner, per sc)
typedef struct {
    _Atomic(void*)    head;   // MPSC list head
    _Atomic(uint32_t) count;  // approximate count
} Hz3OwnerStashBin;

// Push a list to owner's stash.
// If count >= HZ3_S44_STASH_MAX_OBJS, returns 0 (caller should fallback).
// Returns 1 on success.
int hz3_owner_stash_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);

// S111: Push a single object to owner's stash (n==1 fastpath).
// Simpler than push_list - no tail manipulation.
// Returns 1 on success, 0 on overflow.
int hz3_owner_stash_push_one(uint8_t owner, int sc, void* obj);

// Pop up to `want` objects from owner's stash.
// Returns number of objects placed in out[].
// Uses atomic_exchange to drain, returns leftovers to head.
int hz3_owner_stash_pop_batch(uint8_t owner, int sc, void** out, int want);

// Get approximate count (for overflow check before push).
uint32_t hz3_owner_stash_count(uint8_t owner, int sc);

// Initialize the owner stash (called once via pthread_once).
void hz3_owner_stash_init(void);

// S46: Flush owner stash to central with budget (for pressure box)
// Flushes up to budget_per_sc objects per size class from owner's stash to central.
// Used during arena pressure to release objects for reclaim.
void hz3_owner_stash_flush_to_central_budget(uint8_t owner, uint32_t budget_per_sc);

// S71: Check if stash has pending objects for this owner/sc
// Used by retire scan to skip SC if objects are in transit
int hz3_owner_stash_has_pending(uint8_t owner, int sc);

// S121-D: Flush all page packets for this thread (NO-GO, archived)
void hz3_s121d_packet_flush_all_extern(void);

// S121-M: Flush pending pageq batch for this thread
void hz3_s121m_flush_pending_extern(void);

#else

// Stubs when disabled
static inline int hz3_owner_stash_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    (void)owner; (void)sc; (void)head; (void)tail; (void)n;
    return 0;  // always fallback
}

static inline int hz3_owner_stash_push_one(uint8_t owner, int sc, void* obj) {
    (void)owner; (void)sc; (void)obj;
    return 0;  // always fallback
}

static inline int hz3_owner_stash_pop_batch(uint8_t owner, int sc, void** out, int want) {
    (void)owner; (void)sc; (void)out; (void)want;
    return 0;
}

static inline uint32_t hz3_owner_stash_count(uint8_t owner, int sc) {
    (void)owner; (void)sc;
    return 0;
}

static inline void hz3_owner_stash_init(void) {}

static inline void hz3_owner_stash_flush_to_central_budget(uint8_t owner, uint32_t budget_per_sc) {
    (void)owner; (void)budget_per_sc;
}

static inline int hz3_owner_stash_has_pending(uint8_t owner, int sc) {
    (void)owner; (void)sc;
    return 0;  // no stash → no pending
}

static inline void hz3_s121d_packet_flush_all_extern(void) {}

static inline void hz3_s121m_flush_pending_extern(void) {}

static inline void hz3_owner_pagelist_append(int sc, void* page) {
    (void)sc; (void)page;
}

#endif  // HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE
