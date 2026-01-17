#pragma once

#include "hz3_types.h"

// ============================================================================
// Day 6: Epoch - periodic maintenance (event-only, not on hot path)
// Triggers: hz3_alloc_slow(), hz3_outbox_flush(), hz3_tcache_destructor()
// ============================================================================

// TLS cache (defined in hz3_tcache.c)
extern __thread Hz3TCache t_hz3_cache;

// Called from destructor or forced cleanup - always runs
void hz3_epoch_force(void);

// Epoch interval (slow path calls between epoch runs)
#ifndef HZ3_EPOCH_INTERVAL
#define HZ3_EPOCH_INTERVAL 1024
#endif

#if ((HZ3_EPOCH_INTERVAL & (HZ3_EPOCH_INTERVAL - 1)) == 0)
#define HZ3_EPOCH_MASK (HZ3_EPOCH_INTERVAL - 1)
#endif

// Called from slow path - checks if epoch is due (event-only)
static inline void hz3_epoch_maybe(void) {
#if defined(HZ3_EPOCH_MASK)
    if (__builtin_expect(((++t_hz3_cache.epoch_counter) & HZ3_EPOCH_MASK) != 0, 1)) {
        return;
    }
#else
    if (++t_hz3_cache.epoch_counter < HZ3_EPOCH_INTERVAL) {
        return;
    }
    t_hz3_cache.epoch_counter = 0;
#endif
    hz3_epoch_force();
}
