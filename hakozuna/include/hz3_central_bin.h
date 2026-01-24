#pragma once

// hz3 Generic Central Bin (inline mutex-protected freelist)
//
// Provides a unified implementation for central bins used across:
// - hz3_small.c (Hz3SmallCentralBin)
// - hz3_small_v2.c (Hz3SmallV2CentralBin)
// - hz3_sub4k.c (Hz3Sub4kCentralBin)
//
// All functions are inline for zero-cost abstraction.

#include "hz3_tcache.h"  // for hz3_obj_get_next/set_next
#include "hz3_central_shadow.h"  // S86: shadow map for corruption detection
#include "hz3_arena.h"  // for hz3_arena_contains_fast (debug guards)
#include "hz3_config_scale.h"  // for HZ3_S142_CENTRAL_LOCKFREE
#include "hz3_platform.h"  // for hz3_lock_t (platform abstraction)
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HZ3_CENTRAL_DEBUG
#define HZ3_CENTRAL_DEBUG 0
#endif

#ifndef HZ3_CENTRAL_POP_GUARD
#define HZ3_CENTRAL_POP_GUARD 0
#endif

#ifndef HZ3_CENTRAL_POP_GUARD_FAILFAST
#define HZ3_CENTRAL_POP_GUARD_FAILFAST HZ3_CENTRAL_POP_GUARD
#endif

#ifndef HZ3_CENTRAL_POP_GUARD_SHOT
#define HZ3_CENTRAL_POP_GUARD_SHOT 1
#endif

#if HZ3_CENTRAL_POP_GUARD
#if HZ3_CENTRAL_POP_GUARD_SHOT
static _Atomic int g_hz3_central_pop_guard_shot = 0;
#else
static _Atomic int g_hz3_central_pop_guard_shot __attribute__((unused)) = 0;
#endif

static inline int hz3_central_pop_guard_should_log(void) {
#if HZ3_CENTRAL_POP_GUARD_SHOT
    return (atomic_exchange_explicit(&g_hz3_central_pop_guard_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}
#endif

// Generic central bin structure
#if HZ3_S142_CENTRAL_LOCKFREE
// S142: Lock-free MPSC variant (CAS push, atomic_exchange pop)
typedef struct {
    _Atomic(void*)    head;
    _Atomic(uint32_t) count;
} Hz3CentralBin;
#else
// Original mutex-protected variant (uses platform abstraction)
typedef struct {
    hz3_lock_t lock;
    void*      head;
    uint32_t   count;
} Hz3CentralBin;
#endif

#if HZ3_CENTRAL_DEBUG
static inline void hz3_central_debug_fail(const char* where, const char* msg) {
    fprintf(stderr, "[HZ3_CENTRAL_DEBUG] %s: %s\n", where, msg);
    abort();
}

static inline void hz3_central_debug_check_list(const char* where,
                                                void* head,
                                                void* tail,
                                                uint32_t expected_n) {
    if (!head || !tail || expected_n == 0) {
        hz3_central_debug_fail(where, "null head/tail or zero count");
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
        hz3_central_debug_fail(where, "count mismatch or cycle");
    }
    if (cur != NULL) {
        hz3_central_debug_fail(where, "list longer than expected");
    }
    if (last != tail) {
        hz3_central_debug_fail(where, "tail mismatch");
    }
    if (hz3_obj_get_next(tail) != NULL) {
        hz3_central_debug_fail(where, "tail->next not NULL");
    }
}

static inline void hz3_central_debug_check_self_loop(const char* where,
                                                     void* cur,
                                                     void* next) {
    if (cur == next) {
        hz3_central_debug_fail(where, "self-loop detected");
    }
}

#define CENTRAL_CHECK_LIST(where, head, tail, n) \
    hz3_central_debug_check_list(where, head, tail, n)
#define CENTRAL_CHECK_SELF_LOOP(where, cur, next) \
    hz3_central_debug_check_self_loop(where, cur, next)
#else
#define CENTRAL_CHECK_LIST(where, head, tail, n) ((void)0)
#define CENTRAL_CHECK_SELF_LOOP(where, cur, next) ((void)0)
#endif

// Initialize a central bin (call once per bin)
static inline void hz3_central_bin_init(Hz3CentralBin* bin) {
#if HZ3_S142_CENTRAL_LOCKFREE
    atomic_store_explicit(&bin->head, NULL, memory_order_relaxed);
    atomic_store_explicit(&bin->count, 0, memory_order_relaxed);
#else
    hz3_lock_init(&bin->lock);
    bin->head = NULL;
    bin->count = 0;
#endif
}

// Push a list to central bin.
// Precondition: head, tail are non-NULL, n > 0.
static inline void hz3_central_bin_push_list(Hz3CentralBin* bin,
                                              void* head, void* tail, uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }
    CENTRAL_CHECK_LIST("push_list", head, tail, n);

#if HZ3_S142_CENTRAL_LOCKFREE
    // S142: Lock-free MPSC push using CAS
    // Pattern from hz3_owner_stash: always re-link tail->next on CAS retry
    void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
    do {
        hz3_obj_set_next(tail, old_head);  // Must update on every retry
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, head,
        memory_order_release, memory_order_relaxed));

    // Count update (approximate, relaxed ordering is OK for stats)
    atomic_fetch_add_explicit(&bin->count, n, memory_order_relaxed);
#else
#if HZ3_S86_CENTRAL_SHADOW
    // S86: Get return address at boundary (caller of push_list).
    // Must be captured here before inlining obscures the call site.
    void* s86_ra = __builtin_extract_return_addr(__builtin_return_address(0));
#endif

    hz3_lock_acquire(&bin->lock);
    void* old_head = bin->head;
    hz3_obj_set_next(tail, old_head);
    bin->head = head;
    bin->count += n;

#if HZ3_S86_CENTRAL_SHADOW
    // S86: Record each node's (ptr, next, ra) as stored in central.
    // Important: tail->next is overwritten on push (concatenation), so record after linking.
    {
        void* cur = head;
        while (cur) {
            void* next = hz3_obj_get_next(cur);
            hz3_central_shadow_record(cur, next, s86_ra);
            if (cur == tail) break;
            cur = next;
        }
    }
#endif

    hz3_lock_release(&bin->lock);
#endif  // HZ3_S142_CENTRAL_LOCKFREE
}

// S111: Push a single object to central bin (n==1 fastpath).
static inline void hz3_central_bin_push_one(Hz3CentralBin* bin, void* obj) {
    if (!obj) {
        return;
    }

#if HZ3_S142_CENTRAL_LOCKFREE
    // S142: Lock-free MPSC push for single object
    void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
    do {
        hz3_obj_set_next(obj, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, obj,
        memory_order_release, memory_order_relaxed));

    atomic_fetch_add_explicit(&bin->count, 1, memory_order_relaxed);
#else
#if HZ3_S86_CENTRAL_SHADOW
    void* s86_ra = __builtin_extract_return_addr(__builtin_return_address(0));
#endif

    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(obj, bin->head);
    bin->head = obj;
    bin->count++;

#if HZ3_S86_CENTRAL_SHADOW
    hz3_central_shadow_record(obj, hz3_obj_get_next(obj), s86_ra);
#endif

    hz3_lock_release(&bin->lock);
#endif  // HZ3_S142_CENTRAL_LOCKFREE
}

// Pop up to 'want' objects from central bin.
// Returns number of objects placed in out[].
static inline int hz3_central_bin_pop_batch(Hz3CentralBin* bin,
                                             void** out, int want) {
#if HZ3_S142_CENTRAL_LOCKFREE
    // S142: Lock-free MPSC pop using atomic_exchange + push-back remainder
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
#if HZ3_CENTRAL_POP_GUARD
        {
            uint32_t seg_idx = 0;
            void* seg_base_ptr = NULL;
            if (!hz3_arena_contains_fast(cur, &seg_idx, &seg_base_ptr)) {
                if (hz3_central_pop_guard_should_log()) {
                    fprintf(stderr,
                            "[HZ3_CENTRAL_POP_GUARD] where=s142_pop_batch cur=%p\n", cur);
                }
#if HZ3_CENTRAL_POP_GUARD_FAILFAST
                abort();
#endif
            }
        }
#endif
        void* next = hz3_obj_get_next(cur);
        CENTRAL_CHECK_SELF_LOOP("s142_pop_batch", cur, next);
        out[got++] = cur;
        cur = next;
    }

    // Step 3: Push back remainder if any
    void* remainder = cur;  // First node not taken
    if (remainder) {
        // Find tail of remainder for count calculation
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
        fprintf(stderr, "[S142_FAILFAST] count underflow: old=%u got=%d\n", old_count, got);
        abort();
    }
#else
    (void)old_count;
#endif

    return got;

#else  // !HZ3_S142_CENTRAL_LOCKFREE
    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* cur = bin->head;
#if HZ3_CENTRAL_POP_GUARD
        {
            uint32_t seg_idx = 0;
            void* seg_base_ptr = NULL;
            if (!hz3_arena_contains_fast(cur, &seg_idx, &seg_base_ptr)) {
                if (hz3_central_pop_guard_should_log()) {
                    fprintf(stderr,
                            "[HZ3_CENTRAL_POP_GUARD] where=pop_batch_head_invalid bin=%p cur=%p\n",
                            (void*)bin, cur);
                }
#if HZ3_CENTRAL_POP_GUARD_FAILFAST
                abort();
#endif
            }
        }
#endif
        void* next = hz3_obj_get_next(cur);
        CENTRAL_CHECK_SELF_LOOP("pop_batch", cur, next);
#if HZ3_S86_CENTRAL_SHADOW
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
        // S86: Verify next matches shadow, then remove entry.
        hz3_central_shadow_verify_and_remove(cur, next);
  #else
        // S86: Remove without verification (still prevents false dup-push on reinsert).
        hz3_central_shadow_remove(cur);
  #endif
#endif
        out[got++] = cur;
        bin->head = next;
        bin->count--;
    }
    hz3_lock_release(&bin->lock);
    return got;
#endif  // HZ3_S142_CENTRAL_LOCKFREE
}

// Check if bin is empty (approximate, no lock).
static inline int hz3_central_bin_is_empty(const Hz3CentralBin* bin) {
#if HZ3_S142_CENTRAL_LOCKFREE
    return atomic_load_explicit(&((Hz3CentralBin*)bin)->head, memory_order_relaxed) == NULL;
#else
    return bin->head == NULL;
#endif
}

// Get count (approximate, no lock).
static inline uint32_t hz3_central_bin_count(const Hz3CentralBin* bin) {
#if HZ3_S142_CENTRAL_LOCKFREE
    return atomic_load_explicit(&((Hz3CentralBin*)bin)->count, memory_order_relaxed);
#else
    return bin->count;
#endif
}
