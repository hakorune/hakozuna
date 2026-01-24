#define _GNU_SOURCE

#include "hz3_owner_lease.h"
#include "hz3_tcache.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <dlfcn.h>
#include <sys/syscall.h>

#if HZ3_OWNER_LEASE_ENABLE

// Lease encoding: [holder_tid:32][expire_us:32]
static _Atomic uint64_t g_owner_lease[HZ3_NUM_SHARDS];
static HZ3_TLS uint8_t g_owner_lease_depth[HZ3_NUM_SHARDS];
static HZ3_TLS uint32_t g_owner_lease_start_us[HZ3_NUM_SHARDS];
static HZ3_TLS uintptr_t g_owner_lease_start_ra[HZ3_NUM_SHARDS];

#if HZ3_OWNER_LEASE_STATS
static _Atomic uint64_t g_lease_acquire_ok = 0;
static _Atomic uint64_t g_lease_acquire_fail = 0;
static _Atomic uint64_t g_lease_steal = 0;
static _Atomic uint64_t g_lease_hold_us_max = 0;
static _Atomic uint64_t g_lease_hold_us_max_ra = 0;
static _Atomic uint64_t g_lease_hold_us_max_owner = 0;
static pthread_once_t g_lease_atexit_once = PTHREAD_ONCE_INIT;
#endif

static inline uint32_t hz3_owner_lease_tid32(void) {
    return (uint32_t)syscall(SYS_gettid);
}

static inline uint32_t hz3_owner_lease_now_us32(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_us = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    return (uint32_t)now_us;
}

static inline uint64_t hz3_owner_lease_encode(uint32_t tid, uint32_t exp_us) {
    return ((uint64_t)tid << 32) | (uint64_t)exp_us;
}

static inline uint32_t hz3_owner_lease_tid(uint64_t val) {
    return (uint32_t)(val >> 32);
}

static inline uint32_t hz3_owner_lease_exp(uint64_t val) {
    return (uint32_t)val;
}

static inline int hz3_owner_lease_is_expired(uint64_t val, uint32_t now_us) {
    if (val == 0) {
        return 1;
    }
    uint32_t exp = hz3_owner_lease_exp(val);
    return ((int32_t)(now_us - exp) >= 0);
}

#if HZ3_OWNER_LEASE_STATS
static void hz3_owner_lease_atexit_dump(void) {
    uint64_t ok = atomic_load_explicit(&g_lease_acquire_ok, memory_order_relaxed);
    uint64_t fail = atomic_load_explicit(&g_lease_acquire_fail, memory_order_relaxed);
    uint64_t steal = atomic_load_explicit(&g_lease_steal, memory_order_relaxed);
    uint64_t hold_max = atomic_load_explicit(&g_lease_hold_us_max, memory_order_relaxed);
    uint64_t max_ra = atomic_load_explicit(&g_lease_hold_us_max_ra, memory_order_relaxed);
    uint64_t max_owner = atomic_load_explicit(&g_lease_hold_us_max_owner, memory_order_relaxed);

    const char* so_name = "";
    void* so_base = NULL;
    unsigned long off = 0;
    if (max_ra != 0) {
        Dl_info info;
        if (dladdr((void*)max_ra, &info) != 0) {
            so_name = info.dli_fname ? info.dli_fname : "";
            so_base = info.dli_fbase;
            if (so_base) {
                off = (unsigned long)((uintptr_t)max_ra - (uintptr_t)so_base);
            }
        }
    }

    fprintf(stderr,
            "[HZ3_OWNER_LEASE] ok=%lu fail=%lu steal=%lu hold_us_max=%lu max_owner=%lu ra=0x%lx base=%p off=0x%lx so=%s\n",
            (unsigned long)ok, (unsigned long)fail,
            (unsigned long)steal, (unsigned long)hold_max,
            (unsigned long)max_owner,
            (unsigned long)max_ra,
            so_base,
            off,
            so_name);
}

static void hz3_owner_lease_register_atexit(void) {
    atexit(hz3_owner_lease_atexit_dump);
}

void hz3_owner_lease_stats_register_atexit(void) {
    pthread_once(&g_lease_atexit_once, hz3_owner_lease_register_atexit);
}
#endif

static Hz3OwnerLeaseToken hz3_owner_lease_try_internal(uint8_t owner, int blocking, void* ra0) {
    Hz3OwnerLeaseToken token = (Hz3OwnerLeaseToken){ .owner = owner, .locked = 0, .active = 1 };

    if (owner >= HZ3_NUM_SHARDS) {
        token.active = 0;
        return token;
    }

#if HZ3_OWNER_LEASE_STATS
    hz3_owner_lease_stats_register_atexit();
#endif

    // Fast path: no collision, skip lease.
    if (hz3_shard_live_count(owner) <= 1) {
        return token;
    }

    // Re-entrant: same thread already holds lease for this owner.
    if (g_owner_lease_depth[owner] > 0) {
        g_owner_lease_depth[owner]++;
        token.locked = 1;
        // Refresh expiry (best-effort).
        uint32_t tid = hz3_owner_lease_tid32();
        uint32_t now = hz3_owner_lease_now_us32();
        uint64_t new_val = hz3_owner_lease_encode(tid, now + HZ3_OWNER_LEASE_TTL_US);
        atomic_store_explicit(&g_owner_lease[owner], new_val, memory_order_release);
        return token;
    }

    for (;;) {
        uint32_t tid = hz3_owner_lease_tid32();
        uint32_t now = hz3_owner_lease_now_us32();
        uint64_t old = atomic_load_explicit(&g_owner_lease[owner], memory_order_acquire);
        uint32_t old_tid = hz3_owner_lease_tid(old);

        // Same thread holds lease but depth was 0 (shouldn't happen, but be safe).
        if (old != 0 && old_tid == tid) {
            uint64_t new_val = hz3_owner_lease_encode(tid, now + HZ3_OWNER_LEASE_TTL_US);
            atomic_store_explicit(&g_owner_lease[owner], new_val, memory_order_release);
            g_owner_lease_depth[owner] = 1;
            g_owner_lease_start_us[owner] = now;
            g_owner_lease_start_ra[owner] = (uintptr_t)ra0;
            token.locked = 1;
#if HZ3_OWNER_LEASE_STATS
            atomic_fetch_add_explicit(&g_lease_acquire_ok, 1, memory_order_relaxed);
#endif
            return token;
        }

        // Empty or expired lease -> try to acquire.
        if (hz3_owner_lease_is_expired(old, now)) {
            uint64_t new_val = hz3_owner_lease_encode(tid, now + HZ3_OWNER_LEASE_TTL_US);
            if (atomic_compare_exchange_strong_explicit(&g_owner_lease[owner], &old, new_val,
                    memory_order_acq_rel, memory_order_acquire)) {
                g_owner_lease_depth[owner] = 1;
                g_owner_lease_start_us[owner] = now;
                g_owner_lease_start_ra[owner] = (uintptr_t)ra0;
                token.locked = 1;
#if HZ3_OWNER_LEASE_STATS
                atomic_fetch_add_explicit(&g_lease_acquire_ok, 1, memory_order_relaxed);
                if (old != 0) {
                    atomic_fetch_add_explicit(&g_lease_steal, 1, memory_order_relaxed);
                }
#endif
                return token;
            }
        }

        if (!blocking) {
            token.active = 0;
#if HZ3_OWNER_LEASE_STATS
            atomic_fetch_add_explicit(&g_lease_acquire_fail, 1, memory_order_relaxed);
#endif
            return token;
        }
        sched_yield();
    }
}

__attribute__((noinline)) Hz3OwnerLeaseToken hz3_owner_lease_acquire(uint8_t owner) {
    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
    return hz3_owner_lease_try_internal(owner, 1, ra0);
}

__attribute__((noinline)) Hz3OwnerLeaseToken hz3_owner_lease_try_acquire(uint8_t owner) {
    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
    return hz3_owner_lease_try_internal(owner, 0, ra0);
}

void hz3_owner_lease_release(Hz3OwnerLeaseToken token) {
    if (!token.locked) {
        return;
    }
    uint8_t owner = token.owner;
    if (owner >= HZ3_NUM_SHARDS) {
        return;
    }

    uint8_t depth = g_owner_lease_depth[owner];
    if (depth == 0) {
        return;
    }
    depth--;
    g_owner_lease_depth[owner] = depth;
    if (depth != 0) {
        return;
    }

#if HZ3_OWNER_LEASE_STATS
    uint32_t now = hz3_owner_lease_now_us32();
    uint32_t start = g_owner_lease_start_us[owner];
    uint32_t delta = (uint32_t)(now - start);
    uint64_t prev = atomic_load_explicit(&g_lease_hold_us_max, memory_order_relaxed);
    while (delta > prev) {
        if (atomic_compare_exchange_weak_explicit(&g_lease_hold_us_max, &prev, delta,
                memory_order_relaxed, memory_order_relaxed)) {
            atomic_store_explicit(&g_lease_hold_us_max_owner, (uint64_t)owner, memory_order_relaxed);
            atomic_store_explicit(&g_lease_hold_us_max_ra, (uint64_t)g_owner_lease_start_ra[owner], memory_order_relaxed);
            break;
        }
    }
#endif

    atomic_store_explicit(&g_owner_lease[owner], 0, memory_order_release);
}

#else  // !HZ3_OWNER_LEASE_ENABLE

Hz3OwnerLeaseToken hz3_owner_lease_acquire(uint8_t owner) {
    return (Hz3OwnerLeaseToken){ .owner = owner, .locked = 0, .active = 1 };
}

Hz3OwnerLeaseToken hz3_owner_lease_try_acquire(uint8_t owner) {
    return (Hz3OwnerLeaseToken){ .owner = owner, .locked = 0, .active = 1 };
}

void hz3_owner_lease_release(Hz3OwnerLeaseToken token) {
    (void)token;
}

#if HZ3_OWNER_LEASE_STATS
void hz3_owner_lease_stats_register_atexit(void) {}
#endif

#endif
