#pragma once

// S42: Small Transfer Cache
//
// Absorbs remote-free small lists before they hit the central (mutex).
// Reduces lock contention for T=32/R=50/16-2048 workloads.
//
// S142-B: Lock-free MPSC variant (CAS push, atomic_exchange pop)
// - Push: CAS loop to prepend list (multi-producer)
// - Pop: atomic_exchange drain + push-back remainder (single-consumer per shard)

#include "hz3_config.h"
#include "hz3_config_scale.h"  // for HZ3_S142_XFER_LOCKFREE
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

#if HZ3_S42_SMALL_XFER

// List unit for transfer cache (single lock per push/pop)
typedef struct {
    void*    head;
    void*    tail;
    uint32_t n;
} Hz3BatchChain;

// Transfer cache bin (per shard, per sc)
#if HZ3_S142_XFER_LOCKFREE
// S142-B: Lock-free MPSC variant (direct linked list, no slot array)
typedef struct {
    _Atomic(void*)    head;      // head of linked list (objects linked via hz3_obj_get_next)
    _Atomic(uint32_t) count;     // approximate count (relaxed ordering OK for stats)
} Hz3SmallXferBin;
#else
// Original mutex + slot array variant
typedef struct {
    pthread_mutex_t lock;
    uint16_t count;  // number of used slots
    Hz3BatchChain slots[HZ3_S42_SMALL_XFER_SLOTS];
} Hz3SmallXferBin;
#endif

// Push a list to the transfer cache.
// If full, falls back to central.
void hz3_small_xfer_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);

// S111: Push a single object to the transfer cache (n==1 fastpath).
// Returns 1 on success, 0 if full (caller should fall back to central).
int hz3_small_xfer_push_one(uint8_t owner, int sc, void* obj);

// Pop up to `want` objects from the transfer cache.
// Returns number of objects placed in out[].
// Does NOT fall back to central (caller should handle).
int hz3_small_xfer_pop_batch(uint8_t shard, int sc, void** out, int want);

// Initialize the transfer cache (called once via pthread_once)
void hz3_small_xfer_init(void);

// S71: Check if xfer has pending objects for this shard/sc
// Used by retire scan to skip SC if objects are in transit
int hz3_small_xfer_has_pending(uint8_t shard, int sc);

#endif  // HZ3_S42_SMALL_XFER
