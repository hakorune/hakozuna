#define _GNU_SOURCE

#include "hz3_inbox.h"
#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_stash_guard.h"

#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Global inbox pool (zero-initialized)
Hz3Inbox g_hz3_inbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];

// Day 5: pthread_once for thread-safe initialization
static pthread_once_t g_hz3_inbox_once = PTHREAD_ONCE_INIT;

static void hz3_inbox_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
            atomic_store_explicit(&inbox->head, NULL, memory_order_relaxed);
            atomic_store_explicit(&inbox->count, 0, memory_order_relaxed);
        }
    }
}

void hz3_inbox_init(void) {
    pthread_once(&g_hz3_inbox_once, hz3_inbox_do_init);
}

void hz3_inbox_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    if (owner >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail) return;

#if HZ3_S82_STASH_GUARD
    {
        uint32_t expect_bin = (uint32_t)hz3_bin_index_medium(sc);
        hz3_s82_stash_guard_list("inbox_push_list", head, expect_bin, HZ3_S82_STASH_GUARD_MAX_WALK);
    }
#endif

    Hz3Inbox* inbox = &g_hz3_inbox[owner][sc];

    // CAS loop to push list atomically
    void* old_head;
    do {
        old_head = atomic_load_explicit(&inbox->head, memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head, &old_head, head,
        memory_order_release, memory_order_relaxed));

    atomic_fetch_add_explicit(&inbox->count, n, memory_order_relaxed);
}

void hz3_inbox_push_remaining(uint8_t shard, int sc, void* obj) {
    if (shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!obj) return;

    // Find tail and count
    void* head = obj;
    void* tail = obj;
    uint32_t n = 1;
    void* next = hz3_obj_get_next(obj);
    while (next) {
        tail = next;
        n++;
        next = hz3_obj_get_next(next);
    }

    hz3_inbox_push_list(shard, sc, head, tail, n);
}

void* hz3_inbox_drain(uint8_t shard, int sc, Hz3Bin* bin) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];

    // Fast path: empty check without RMW
    void* head = atomic_load_explicit(&inbox->head, memory_order_acquire);
    if (!head) return NULL;

    // Swap entire list (atomic_exchange is cheaper than CAS loop for drain)
    head = atomic_exchange_explicit(&inbox->head, NULL, memory_order_acq_rel);
    if (!head) return NULL;

    // Reset count (approximate)
    atomic_store_explicit(&inbox->count, 0, memory_order_relaxed);

    // First object is returned immediately for use
    void* first = head;
    void* obj = hz3_obj_get_next(head);

    // Day 5: Build overflow list for central (don't re-push to inbox)
    void* overflow_head = NULL;
    void* overflow_tail = NULL;
    uint32_t overflow_n = 0;

    // Push to local bin, respecting HZ3_BIN_HARD_CAP
#if HZ3_BIN_LAZY_COUNT && HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY
    if (!hz3_bin_is_empty(bin)) {
#if HZ3_S123_INBOX_DRAIN_FAILFAST
        fprintf(stderr, "[HZ3_S123_INBOX] bin_not_empty sc=%d head=%p\n", sc, hz3_bin_head(bin));
        abort();
#endif
        // Conservative fallback: treat as full to avoid overfilling.
        // Keeps bounded behavior without a full walk.
    }
    uint16_t bin_len = hz3_bin_is_empty(bin) ? 0 : HZ3_BIN_HARD_CAP;
#elif HZ3_BIN_LAZY_COUNT
    uint16_t bin_len = hz3_bin_count_walk(bin, HZ3_BIN_HARD_CAP);
#endif
    while (obj) {
        void* next = hz3_obj_get_next(obj);
#if HZ3_BIN_LAZY_COUNT
        if (bin_len < HZ3_BIN_HARD_CAP) {
            hz3_bin_push(bin, obj);
            bin_len++;
        } else {
#else
        if (hz3_bin_count_get(bin) < HZ3_BIN_HARD_CAP) {
            hz3_bin_push(bin, obj);
        } else {
#endif
            // Add to overflow list
            if (!overflow_head) {
                overflow_head = obj;
            } else {
                hz3_obj_set_next(overflow_tail, obj);
            }
            overflow_tail = obj;
            overflow_n++;
        }
        obj = next;
    }

    // Day 5: Send overflow to central (NOT back to inbox)
    if (overflow_head) {
        hz3_obj_set_next(overflow_tail, NULL);
        hz3_central_push_list(shard, sc, overflow_head, overflow_tail, overflow_n);
    }

    return first;
}

uint32_t hz3_inbox_drain_to_central(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    void* head = atomic_exchange_explicit(&inbox->head, NULL, memory_order_acq_rel);
    if (!head) {
        return 0;
    }

    atomic_store_explicit(&inbox->count, 0, memory_order_relaxed);

    void* tail = head;
    uint32_t n = 1;
    void* obj = hz3_obj_get_next(head);
    while (obj) {
        tail = obj;
        n++;
        obj = hz3_obj_get_next(obj);
    }

    hz3_central_push_list(shard, sc, head, tail, n);
    return n;
}
