#pragma once

#include "hz3_config.h"
#include <stdint.h>

typedef struct {
    uint8_t owner;
    uint8_t locked;
    uint8_t active;
} Hz3OwnerLeaseToken;

Hz3OwnerLeaseToken hz3_owner_lease_acquire(uint8_t owner);
Hz3OwnerLeaseToken hz3_owner_lease_try_acquire(uint8_t owner);
void hz3_owner_lease_release(Hz3OwnerLeaseToken token);

#if HZ3_OWNER_LEASE_STATS
void hz3_owner_lease_stats_register_atexit(void);
#endif

#define HZ3_OWNER_LEASE_GUARD(owner) \
    for (Hz3OwnerLeaseToken _t = hz3_owner_lease_acquire(owner); _t.active; \
         hz3_owner_lease_release(_t), _t.active = 0)

#define HZ3_OWNER_LEASE_TRY_GUARD(owner) \
    for (Hz3OwnerLeaseToken _t = hz3_owner_lease_try_acquire(owner); _t.active; \
         hz3_owner_lease_release(_t), _t.active = 0)
