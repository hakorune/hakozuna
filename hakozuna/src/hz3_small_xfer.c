#include "hz3_small_xfer.h"

#if HZ3_S42_SMALL_XFER

#include "hz3_list_invariants.h"
#include "hz3_small_v2.h"
#include "hz3_tcache.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HZ3_XFER_DEBUG
#define HZ3_XFER_DEBUG 0
#endif

// =============================================================================
// HZ3_XFER_STRUCT_FAILFAST: Detect slot/count corruption (NOT list corruption)
// =============================================================================
#ifndef HZ3_XFER_STRUCT_FAILFAST
#define HZ3_XFER_STRUCT_FAILFAST 0
#endif

#if HZ3_XFER_STRUCT_FAILFAST

// Reasonable upper bound for slot->n (should never exceed batch * 4 or so)
#ifndef HZ3_XFER_SLOT_N_MAX
#define HZ3_XFER_SLOT_N_MAX 256
#endif

static inline void hz3_xfer_struct_fail(const char* where, const char* msg,
                                        uint8_t owner, int sc, uint32_t val) {
    fprintf(stderr, "[HZ3_XFER_STRUCT_FAILFAST] %s: %s (owner=%u sc=%d val=%u)\n",
            where, msg, owner, sc, val);
    abort();
}

// Check bin->count bounds (call while holding lock OR outside to detect races)
#define XFER_CHECK_BIN_COUNT(where, bin, owner, sc) do { \
    if ((bin)->count > HZ3_S42_SMALL_XFER_SLOTS) { \
        hz3_xfer_struct_fail(where, "bin->count OOB", owner, sc, (bin)->count); \
    } \
} while(0)

// Check slot->n bounds before use
#define XFER_CHECK_SLOT_N(where, slot_n, owner, sc) do { \
    if ((slot_n) > HZ3_XFER_SLOT_N_MAX) { \
        hz3_xfer_struct_fail(where, "slot->n exceeds max", owner, sc, (slot_n)); \
    } \
} while(0)

// Check remaining_n doesn't underflow (got should never exceed chain.n)
#define XFER_CHECK_REMAINING(where, chain_n, got, owner, sc) do { \
    if ((uint32_t)(got) > (chain_n)) { \
        hz3_xfer_struct_fail(where, "remaining_n underflow (got > chain.n)", \
                             owner, sc, (got)); \
    } \
} while(0)

#else
#define XFER_CHECK_BIN_COUNT(where, bin, owner, sc) ((void)0)
#define XFER_CHECK_SLOT_N(where, slot_n, owner, sc) ((void)0)
#define XFER_CHECK_REMAINING(where, chain_n, got, owner, sc) ((void)0)
#endif

#if HZ3_XFER_DEBUG
static inline void hz3_xfer_debug_fail(const char* where, const char* msg) {
    fprintf(stderr, "[HZ3_XFER_DEBUG] %s: %s\n", where, msg);
    abort();
}

static inline void hz3_xfer_debug_check_list(const char* where,
                                             void* head,
                                             void* tail,
                                             uint32_t expected_n) {
    if (!head || !tail || expected_n == 0) {
        hz3_xfer_debug_fail(where, "null head/tail or zero count");
    }

    void* cur = head;
    void* last = NULL;
    uint32_t count = 0;
    while (cur && count <= expected_n) {
        last = cur;
        cur = hz3_obj_get_next(cur);
        count++;
    }

    if (count != expected_n) {
        hz3_xfer_debug_fail(where, "count mismatch or cycle");
    }
    if (cur != NULL) {
        hz3_xfer_debug_fail(where, "list longer than expected");
    }
    if (last != tail) {
        hz3_xfer_debug_fail(where, "tail mismatch");
    }
    if (hz3_obj_get_next(tail) != NULL) {
        hz3_xfer_debug_fail(where, "tail->next not NULL");
    }
}

#define XFER_CHECK_LIST(where, head, tail, n) \
    hz3_xfer_debug_check_list(where, head, tail, n)
#else
#define XFER_CHECK_LIST(where, head, tail, n) ((void)0)
#endif

// Global transfer cache array
static Hz3SmallXferBin g_hz3_small_xfer[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];
static pthread_once_t g_hz3_small_xfer_once = PTHREAD_ONCE_INIT;

static void hz3_small_xfer_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
            Hz3SmallXferBin* bin = &g_hz3_small_xfer[shard][sc];
#if HZ3_S142_XFER_LOCKFREE
            atomic_store_explicit(&bin->head, NULL, memory_order_relaxed);
            atomic_store_explicit(&bin->count, 0, memory_order_relaxed);
#else
            pthread_mutex_init(&bin->lock, NULL);
            bin->count = 0;
            memset(bin->slots, 0, sizeof(bin->slots));
#endif
        }
    }
}

void hz3_small_xfer_init(void) {
    pthread_once(&g_hz3_small_xfer_once, hz3_small_xfer_do_init);
}

void hz3_small_xfer_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

#if HZ3_LIST_FAILFAST
    hz3_list_failfast("xfer_push:entry", owner, sc, head, tail, n);
#endif
    XFER_CHECK_LIST("push_list", head, tail, n);
    hz3_small_xfer_init();

    Hz3SmallXferBin* bin = &g_hz3_small_xfer[owner][sc];

#if HZ3_S142_XFER_LOCKFREE
    // S142-B: Lock-free MPSC push using CAS
    // Pattern from hz3_central_bin: always re-link tail->next on CAS retry
    void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
    do {
        hz3_obj_set_next(tail, old_head);  // Must update on every retry
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, head,
        memory_order_release, memory_order_relaxed));

    // Count update (approximate, relaxed ordering OK for stats)
    atomic_fetch_add_explicit(&bin->count, n, memory_order_relaxed);

#else  // !HZ3_S142_XFER_LOCKFREE
    pthread_mutex_lock(&bin->lock);

    // Struct failfast: check count bounds inside lock
    XFER_CHECK_BIN_COUNT("push:in_lock", bin, owner, sc);

    if (bin->count < HZ3_S42_SMALL_XFER_SLOTS) {
        // Store entire list as one slot
        Hz3BatchChain* slot = &bin->slots[bin->count];
        slot->head = head;
        slot->tail = tail;
        slot->n = n;
        bin->count++;
        pthread_mutex_unlock(&bin->lock);
        return;
    }

    pthread_mutex_unlock(&bin->lock);

    // Overflow: fall back to central
    hz3_small_v2_central_push_list(owner, sc, head, tail, n);
#endif  // HZ3_S142_XFER_LOCKFREE
}

// S111: Push a single object (n==1 fastpath)
int hz3_small_xfer_push_one(uint8_t owner, int sc, void* obj) {
    if (!obj) {
        return 0;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

    hz3_small_xfer_init();

    Hz3SmallXferBin* bin = &g_hz3_small_xfer[owner][sc];

#if HZ3_S142_XFER_LOCKFREE
    // S142-B: Lock-free MPSC push for single object
    void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
    do {
        hz3_obj_set_next(obj, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, obj,
        memory_order_release, memory_order_relaxed));

    atomic_fetch_add_explicit(&bin->count, 1, memory_order_relaxed);
    return 1;  // always succeeds (no capacity limit)

#else  // !HZ3_S142_XFER_LOCKFREE
    pthread_mutex_lock(&bin->lock);

    if (bin->count < HZ3_S42_SMALL_XFER_SLOTS) {
        Hz3BatchChain* slot = &bin->slots[bin->count];
        slot->head = obj;
        slot->tail = obj;
        slot->n = 1;
        bin->count++;
        pthread_mutex_unlock(&bin->lock);
        return 1;  // success
    }

    pthread_mutex_unlock(&bin->lock);
    return 0;  // full - caller should fall back to central
#endif  // HZ3_S142_XFER_LOCKFREE
}

int hz3_small_xfer_pop_batch(uint8_t shard, int sc, void** out, int want) {
    if (want <= 0) {
        return 0;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

    hz3_small_xfer_init();

    Hz3SmallXferBin* bin = &g_hz3_small_xfer[shard][sc];

#if HZ3_S142_XFER_LOCKFREE
    // S142-B: Lock-free MPSC pop using atomic_exchange + push-back remainder
    // MPSC confirmed: only my_shard owner calls pop (single consumer per shard)

    // Step 1: Atomic exchange to drain entire list
    void* list = atomic_exchange_explicit(&bin->head, NULL, memory_order_acquire);
    if (!list) {
        return 0;
    }

    // Step 2: Take up to 'want' objects from list
    int got = 0;
    void* cur = list;
    while (cur && got < want) {
        void* next = hz3_obj_get_next(cur);
        out[got++] = cur;
        cur = next;
    }

    // Step 3: Push back remainder if any
    void* remainder = cur;  // First node not taken
    if (remainder) {
        // Find tail of remainder for push-back
        uint32_t remainder_n = 0;
        void* rtail = remainder;
        while (rtail) {
            remainder_n++;
            void* rnext = hz3_obj_get_next(rtail);
            if (!rnext) break;
            rtail = rnext;
        }

        // Push remainder back using CAS
        void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
        do {
            hz3_obj_set_next(rtail, old_head);
        } while (!atomic_compare_exchange_weak_explicit(
            &bin->head, &old_head, remainder,
            memory_order_release, memory_order_relaxed));

        // Restore remainder count
        atomic_fetch_add_explicit(&bin->count, remainder_n, memory_order_relaxed);
    }

    // Step 4: Decrement count for objects we took
    uint32_t old_count = atomic_fetch_sub_explicit(&bin->count, (uint32_t)got, memory_order_relaxed);
#if HZ3_S142_FAILFAST
    if (old_count < (uint32_t)got) {
        fprintf(stderr, "[S142_XFER_FAILFAST] count underflow: old=%u got=%d\n", old_count, got);
        abort();
    }
#else
    (void)old_count;
#endif

    return got;

#else  // !HZ3_S142_XFER_LOCKFREE
    pthread_mutex_lock(&bin->lock);

    // Struct failfast: check count bounds inside lock
    XFER_CHECK_BIN_COUNT("pop:in_lock", bin, shard, sc);

    if (bin->count == 0) {
        pthread_mutex_unlock(&bin->lock);
        return 0;
    }

    // Pop one list slot
    bin->count--;
    Hz3BatchChain chain = bin->slots[bin->count];
    pthread_mutex_unlock(&bin->lock);

    // Struct failfast: check slot->n bounds after reading
    XFER_CHECK_SLOT_N("pop:slot_n", chain.n, shard, sc);

    // Unpack the list into out[]
    int got = 0;
    void* cur = chain.head;
    XFER_CHECK_LIST("pop_batch:entry", chain.head, chain.tail, chain.n);
    while (cur && got < want) {
        if (cur == chain.tail) {
            out[got++] = cur;
            break;
        }
#if HZ3_S42_XFER_POP_PREFETCH && (defined(__GNUC__) || defined(__clang__))
        void* next = hz3_obj_get_next(cur);
        if (next) {
            __builtin_prefetch(next, 0, 1);
#if HZ3_S42_XFER_POP_PREFETCH_DIST >= 2
            void* next2 = hz3_obj_get_next(next);
            if (next2) __builtin_prefetch(next2, 0, 1);
#endif
        }
        out[got++] = cur;
        cur = next;
#else
        out[got++] = cur;
        cur = hz3_obj_get_next(cur);
#endif
    }

    // Struct failfast: check got <= chain.n (underflow prevention)
    XFER_CHECK_REMAINING("pop:remaining", chain.n, got, shard, sc);

    // If there are remaining items in the list, push them back
    uint32_t remaining_n = chain.n - (uint32_t)got;
    if (remaining_n > 0 && cur) {
#if HZ3_LIST_FAILFAST
        hz3_list_failfast("xfer_pop:remain", shard, sc, cur, chain.tail, remaining_n);
#endif
        XFER_CHECK_LIST("pop_batch:remain", cur, chain.tail, remaining_n);
        // Push back to xfer (or central if full)
        hz3_small_xfer_push_list(shard, sc, cur, chain.tail, remaining_n);
    }

    return got;
#endif  // HZ3_S142_XFER_LOCKFREE
}

// S71: Check if xfer has pending objects for this shard/sc
// Used by retire scan to skip SC if objects are in transit
int hz3_small_xfer_has_pending(uint8_t shard, int sc) {
    hz3_small_xfer_init();

    Hz3SmallXferBin* bin = &g_hz3_small_xfer[shard][sc];
#if HZ3_S142_XFER_LOCKFREE
    // S142-B: Lock-free check (approximate, no lock)
    return atomic_load_explicit(&bin->head, memory_order_relaxed) != NULL;
#else
    pthread_mutex_lock(&bin->lock);
    int has = (bin->count > 0);
    pthread_mutex_unlock(&bin->lock);
    return has;
#endif
}

#endif  // HZ3_S42_SMALL_XFER
