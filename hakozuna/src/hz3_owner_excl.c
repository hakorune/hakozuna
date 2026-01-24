// hz3_owner_excl.c - Owner Exclusive Lock Box (CollisionGuardBox)

#include "hz3_owner_excl.h"

#if HZ3_OWNER_EXCL_ENABLE

#include <stdatomic.h>
#include <stdio.h>

// Use platform abstractions from hz3_platform.h (hz3_lock_t, hz3_once_t, etc.)

// hz3_lock_trylock wrapper (not in platform.h, provide here)
#if defined(_WIN32)
static inline int hz3_lock_trylock(hz3_lock_t* lock) {
    return TryAcquireSRWLockExclusive(lock) ? 0 : 1;  // 0=success, 1=busy
}
#else
static inline int hz3_lock_trylock(hz3_lock_t* lock) {
    return pthread_mutex_trylock(lock);  // 0=success, non-zero=busy
}
#endif

// Per-shard mutex for collision-time exclusion
static hz3_lock_t g_owner_excl[HZ3_NUM_SHARDS];
static hz3_once_t g_owner_excl_once = HZ3_ONCE_INIT;

// TLS re-entry guard (depth counter per shard)
static HZ3_TLS uint8_t t_owner_excl_depth[HZ3_NUM_SHARDS] = {0};

// Stats (atomic counters for diagnostics)
static _Atomic uint64_t g_owner_excl_contended = 0;
static _Atomic uint64_t g_owner_excl_guarded_ops = 0;

#if HZ3_OWNER_EXCL_SHOT
static _Atomic int g_owner_excl_shot_fired = 0;
#endif

static void hz3_owner_excl_init_once(void) {
    for (int i = 0; i < HZ3_NUM_SHARDS; i++) {
        hz3_lock_init(&g_owner_excl[i]);
    }
}

Hz3OwnerExclToken hz3_owner_excl_acquire(uint8_t owner) {
    Hz3OwnerExclToken token = { .owner = owner, .locked = 0, .active = 1 };

    if (owner >= HZ3_NUM_SHARDS) {
        token.active = 0;
        return token;  // Invalid owner
    }

    // Re-entry check: already holding lock for this shard
    if (t_owner_excl_depth[owner] > 0) {
        t_owner_excl_depth[owner]++;
        return token;  // Skip lock, already inside critical section
    }

    uint32_t live = hz3_shard_live_count(owner);
    if (!HZ3_OWNER_EXCL_ALWAYS && live <= 1) {
        return token;  // No collision, skip lock
    }

    // Collision detected - need lock
    hz3_once(&g_owner_excl_once, hz3_owner_excl_init_once);
    hz3_lock_acquire(&g_owner_excl[owner]);
    token.locked = 1;
    t_owner_excl_depth[owner] = 1;  // Mark as holding lock

    // Track stats
    atomic_fetch_add_explicit(&g_owner_excl_contended, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_owner_excl_guarded_ops, 1, memory_order_relaxed);

#if HZ3_OWNER_EXCL_SHOT
    if (atomic_exchange_explicit(&g_owner_excl_shot_fired, 1, memory_order_relaxed) == 0) {
        fprintf(stderr, "[HZ3_OWNER_EXCL] first_contention owner=%u live=%u\n",
                (unsigned)owner, (unsigned)live);
    }
#endif

    return token;
}

Hz3OwnerExclToken hz3_owner_excl_try_acquire(uint8_t owner) {
    Hz3OwnerExclToken token = { .owner = owner, .locked = 0, .active = 1 };

    if (owner >= HZ3_NUM_SHARDS) {
        token.active = 0;
        return token;  // Invalid owner
    }

    // Re-entry check: already holding lock for this shard
    if (t_owner_excl_depth[owner] > 0) {
        t_owner_excl_depth[owner]++;
        return token;  // Skip lock, already inside critical section
    }

    uint32_t live = hz3_shard_live_count(owner);
    if (!HZ3_OWNER_EXCL_ALWAYS && live <= 1) {
        return token;  // No collision, skip lock
    }

    hz3_once(&g_owner_excl_once, hz3_owner_excl_init_once);
    if (hz3_lock_trylock(&g_owner_excl[owner]) != 0) {
        token.active = 0;
        return token;
    }

    token.locked = 1;
    t_owner_excl_depth[owner] = 1;

    atomic_fetch_add_explicit(&g_owner_excl_guarded_ops, 1, memory_order_relaxed);

    // Contention metric: try_acquire failure isn't counted here; keep it simple.
#if HZ3_OWNER_EXCL_SHOT
    if (atomic_exchange_explicit(&g_owner_excl_shot_fired, 1, memory_order_relaxed) == 0) {
        fprintf(stderr, "[HZ3_OWNER_EXCL] first_contention owner=%u live=%u\n",
                (unsigned)owner, (unsigned)live);
    }
#endif

    return token;
}

void hz3_owner_excl_release(Hz3OwnerExclToken token) {
    if (!token.active) {
        return;
    }
    if (token.owner >= HZ3_NUM_SHARDS) {
        return;
    }

    // Re-entry: decrement depth, only unlock at outermost level
    if (t_owner_excl_depth[token.owner] > 1) {
        t_owner_excl_depth[token.owner]--;
        return;  // Still inside outer critical section
    }

    if (token.locked) {
        t_owner_excl_depth[token.owner] = 0;
        hz3_lock_release(&g_owner_excl[token.owner]);
    }
}

// Get stats (for atexit diagnostics)
void hz3_owner_excl_get_stats(uint64_t* contended, uint64_t* guarded_ops) {
    if (contended) {
        *contended = atomic_load_explicit(&g_owner_excl_contended, memory_order_relaxed);
    }
    if (guarded_ops) {
        *guarded_ops = atomic_load_explicit(&g_owner_excl_guarded_ops, memory_order_relaxed);
    }
}

#else  // !HZ3_OWNER_EXCL_ENABLE

// Stub implementations when disabled
Hz3OwnerExclToken hz3_owner_excl_acquire(uint8_t owner) {
    return (Hz3OwnerExclToken){ .owner = owner, .locked = 0, .active = 1 };
}

Hz3OwnerExclToken hz3_owner_excl_try_acquire(uint8_t owner) {
    return (Hz3OwnerExclToken){ .owner = owner, .locked = 0, .active = 1 };
}

void hz3_owner_excl_release(Hz3OwnerExclToken token) {
    (void)token;
}

void hz3_owner_excl_get_stats(uint64_t* contended, uint64_t* guarded_ops) {
    if (contended) *contended = 0;
    if (guarded_ops) *guarded_ops = 0;
}

#endif  // HZ3_OWNER_EXCL_ENABLE
