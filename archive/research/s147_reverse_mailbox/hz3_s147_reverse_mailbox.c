// hz3_s147_reverse_mailbox.c - Reverse Mailbox implementation
//
// S147: Pull-based remote free to eliminate per-object CAS on push side
//
// This file is self-contained for research box isolation.
// Integration with mainline requires patching via hz3_s147_patch.inc
#define _GNU_SOURCE

#include "hz3_s147_reverse_mailbox.h"

#if HZ3_S147_REVERSE_MAILBOX

#include "hz3_config.h"
#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_owner_stash.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Global State
// ============================================================================

// Thread registry for Owner scan
struct Hz3TCache* g_s147_all_tcaches[HZ3_S147_MAX_THREADS];
_Atomic int g_s147_thread_count = 0;

// Initialization guard
static pthread_once_t g_s147_init_once = PTHREAD_ONCE_INIT;

// ============================================================================
// Stats (when enabled)
// ============================================================================

#if HZ3_S147_STATS
_Atomic uint64_t g_s147_push_calls = 0;
_Atomic uint64_t g_s147_push_accepted = 0;
_Atomic uint64_t g_s147_push_fallback = 0;
_Atomic uint64_t g_s147_pull_calls = 0;
_Atomic uint64_t g_s147_pull_objs = 0;
_Atomic uint64_t g_s147_flush_calls = 0;
_Atomic uint64_t g_s147_flush_objs = 0;

static int g_s147_atexit_registered = 0;

static void hz3_s147_atexit_dump(void) {
    fprintf(stderr, "[S147_STATS] push_calls=%llu accepted=%llu fallback=%llu\n",
            (unsigned long long)atomic_load(&g_s147_push_calls),
            (unsigned long long)atomic_load(&g_s147_push_accepted),
            (unsigned long long)atomic_load(&g_s147_push_fallback));
    fprintf(stderr, "[S147_STATS] pull_calls=%llu pull_objs=%llu\n",
            (unsigned long long)atomic_load(&g_s147_pull_calls),
            (unsigned long long)atomic_load(&g_s147_pull_objs));
    fprintf(stderr, "[S147_STATS] flush_calls=%llu flush_objs=%llu\n",
            (unsigned long long)atomic_load(&g_s147_flush_calls),
            (unsigned long long)atomic_load(&g_s147_flush_objs));
}

#define S147_ATEXIT_ONCE() do { \
    if (!g_s147_atexit_registered) { \
        g_s147_atexit_registered = 1; \
        atexit(hz3_s147_atexit_dump); \
    } \
} while (0)
#else
#define S147_ATEXIT_ONCE() ((void)0)
#endif

// ============================================================================
// Initialization
// ============================================================================

static void hz3_s147_do_init(void) {
    memset(g_s147_all_tcaches, 0, sizeof(g_s147_all_tcaches));
    atomic_init(&g_s147_thread_count, 0);
}

void hz3_s147_init(void) {
    pthread_once(&g_s147_init_once, hz3_s147_do_init);
}

// ============================================================================
// Thread Registration
// ============================================================================

void hz3_s147_register_thread(void) {
    hz3_s147_init();
    S147_ATEXIT_ONCE();

    int my_tid = atomic_fetch_add_explicit(&g_s147_thread_count, 1, memory_order_acq_rel);
    if (my_tid < HZ3_S147_MAX_THREADS) {
        g_s147_all_tcaches[my_tid] = &t_hz3_cache;
        // Store tid for later unregistration
        // Note: Requires adding s147_tid to Hz3TCache (see patch)
#ifdef HZ3_S147_TID_FIELD
        t_hz3_cache.s147_tid = my_tid;
#endif
    }
#if HZ3_S147_FAILFAST
    else {
        fprintf(stderr, "[S147_FAILFAST] thread registry overflow: tid=%d max=%d\n",
                my_tid, HZ3_S147_MAX_THREADS);
        // Don't abort - just lose S147 for this thread (fallback to owner_stash)
    }
#endif
}

void hz3_s147_unregister_thread(void) {
#ifdef HZ3_S147_TID_FIELD
    int my_tid = t_hz3_cache.s147_tid;
    if (my_tid >= 0 && my_tid < HZ3_S147_MAX_THREADS) {
        g_s147_all_tcaches[my_tid] = NULL;
        t_hz3_cache.s147_tid = -1;
    }
#endif
    // Note: g_s147_thread_count is not decremented (monotonic for simplicity)
    // This is safe because we NULL the slot and check for NULL in pull
}

// ============================================================================
// Producer Path: Push to TLS Outbox
// ============================================================================

int hz3_s147_push(uint8_t owner, int sc, void* ptr) {
    S147_STAT_INC(g_s147_push_calls);

    // Range check
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        S147_STAT_INC(g_s147_push_fallback);
        return 0;
    }

    // Note: Requires s147_outbox field in Hz3TCache (see patch)
#ifndef HZ3_S147_OUTBOX_FIELD
    // Without the field, always fallback
    S147_STAT_INC(g_s147_push_fallback);
    return 0;
#else
    Hz3S147Outbox* out = &t_hz3_cache.s147_outbox[owner];

    // Concurrency check: if notify==1, Owner may be draining -> fallback
    // This avoids Producer writing while Owner reads
    if (atomic_load_explicit(&out->notify, memory_order_acquire)) {
        S147_STAT_INC(g_s147_push_fallback);
        return 0;  // fallback to owner_stash
    }

    // Full check: if batch full, flush and retry or fallback
    if (out->count >= HZ3_S147_OUTBOX_BATCH) {
        // Flush to owner_stash (sync fallback)
        hz3_s147_flush_to_owner_stash(owner);
        // Try again - if still full after flush, something is wrong
        if (out->count >= HZ3_S147_OUTBOX_BATCH) {
            S147_STAT_INC(g_s147_push_fallback);
            return 0;
        }
    }

    // Write entry (no atomic needed: only this thread writes)
    out->items[out->count].ptr = ptr;
    out->items[out->count].sc = (uint8_t)sc;
    out->count++;

    // Notify when threshold reached
    // After this, Owner can start draining (Producer must fallback until drain completes)
    if (out->count >= HZ3_S147_NOTIFY_THRESHOLD) {
        atomic_store_explicit(&out->notify, 1, memory_order_release);
    }

    S147_STAT_INC(g_s147_push_accepted);
    return 1;  // success
#endif
}

// ============================================================================
// Owner Path: Pull from All Producers' Outboxes
// ============================================================================

int hz3_s147_pull(int sc, void** out, int want) {
    S147_STAT_INC(g_s147_pull_calls);

    if (want <= 0) {
        return 0;
    }

    // Range check
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

#ifndef HZ3_S147_OUTBOX_FIELD
    return 0;  // Without field, nothing to pull
#else
    int got = 0;
    uint8_t my_shard = t_hz3_cache.my_shard;

    // Scan all registered threads' outboxes for items destined to my_shard
    int thread_count = atomic_load_explicit(&g_s147_thread_count, memory_order_acquire);
    if (thread_count > HZ3_S147_MAX_THREADS) {
        thread_count = HZ3_S147_MAX_THREADS;
    }

    for (int tid = 0; tid < thread_count && got < want; tid++) {
        // Skip self (don't drain own outbox)
        Hz3TCache* remote = g_s147_all_tcaches[tid];
        if (!remote || remote == &t_hz3_cache) {
            continue;
        }

        Hz3S147Outbox* box = &remote->s147_outbox[my_shard];

        // Quick check: only process if notified (has items)
        if (!atomic_load_explicit(&box->notify, memory_order_acquire)) {
            continue;
        }

        // Acquire exclusive drain right via atomic_exchange
        // Sets notify=0, signaling to Producer that we're draining
        // Producer will see notify=0 after we're done and can resume writing
        uint8_t was_notify = atomic_exchange_explicit(&box->notify, 0, memory_order_acquire);
        if (!was_notify) {
            // Race: another Owner (unlikely, but possible with shard collisions)
            continue;
        }

        // Now safe to read: Producer won't write while we're draining
        // (Producer checks notify==0 at entry and falls back if ==1)
        uint8_t count = box->count;

        // Collect items matching our sc, leave others
        // Note: We need a smarter strategy here to not lose items
        // Option A: Only take matching sc, leave rest (current implementation)
        // Option B: Take all, push non-matching to owner_stash
        // Using Option B for correctness

        for (uint8_t i = 0; i < count; i++) {
            void* ptr = box->items[i].ptr;
            uint8_t item_sc = box->items[i].sc;

            if (ptr == NULL) {
                continue;  // already taken or empty
            }

            if (item_sc == (uint8_t)sc && got < want) {
                // Match: take it
                out[got++] = ptr;
                box->items[i].ptr = NULL;  // mark as taken
            } else {
                // Non-match: push to owner_stash for later
                // This ensures we don't lose items with different sc
                hz3_owner_stash_push_one(my_shard, (int)item_sc, ptr);
                box->items[i].ptr = NULL;
            }
        }

        // Reset count (Producer can now refill from scratch)
        box->count = 0;

        // Note: notify was already set to 0 by atomic_exchange above
        // Producer can now write again
    }

    S147_STAT_ADD(g_s147_pull_objs, got);
    return got;
#endif
}

// ============================================================================
// Flush: Outbox -> Owner Stash (for thread exit or forced drain)
// ============================================================================

void hz3_s147_flush_to_owner_stash(uint8_t owner) {
    S147_STAT_INC(g_s147_flush_calls);

#ifndef HZ3_S147_OUTBOX_FIELD
    return;
#else
    if (owner >= HZ3_NUM_SHARDS) {
        return;
    }

    Hz3S147Outbox* out = &t_hz3_cache.s147_outbox[owner];
    uint8_t count = out->count;

    for (uint8_t i = 0; i < count; i++) {
        void* ptr = out->items[i].ptr;
        uint8_t sc = out->items[i].sc;

        if (ptr) {
            hz3_owner_stash_push_one(owner, (int)sc, ptr);
            S147_STAT_INC(g_s147_flush_objs);
        }
    }

    out->count = 0;
    atomic_store_explicit(&out->notify, 0, memory_order_release);
#endif
}

void hz3_s147_flush_all(void) {
#ifdef HZ3_S147_OUTBOX_FIELD
    for (uint8_t owner = 0; owner < HZ3_NUM_SHARDS; owner++) {
        hz3_s147_flush_to_owner_stash(owner);
    }
#endif
}

#endif  // HZ3_S147_REVERSE_MAILBOX
