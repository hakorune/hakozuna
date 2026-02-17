#ifndef HZ4_CENTRAL_PAGEHEAP_H
#define HZ4_CENTRAL_PAGEHEAP_H

#include "hz4_types.h"
#include "hz4_config.h"
#include "hz4_os.h"
#include <stdio.h>

#if HZ4_CENTRAL_PAGEHEAP

enum {
    HZ4_CPH_NONE = 0,
    HZ4_CPH_QUEUED = 1,
    HZ4_CPH_INFLIGHT = 2
};

// Forward declarations
typedef struct hz4_page_meta hz4_page_meta_t;

// CPH per-sc stack: locked LIFO of page_meta pointers (ABA回避)
typedef struct hz4_cph_sc {
    atomic_flag lock;
    hz4_page_meta_t* top;
} hz4_cph_sc_t;

extern hz4_cph_sc_t g_cph_sc[HZ4_SC_MAX];

#if HZ4_CPH_2TIER
extern hz4_cph_sc_t g_cph_hot_sc[HZ4_SC_MAX];
extern hz4_cph_sc_t g_cph_cold_sc[HZ4_SC_MAX];
extern _Atomic(uint32_t) g_cph_hot_count[HZ4_SC_MAX];
static inline bool hz4_cph_remove_meta_any(hz4_page_meta_t* meta);
#endif

// Push empty page to central heap (meta version)
// Uses meta->cph_next for linking (avoids conflict with dqnext/reuse_next)
static inline void hz4_cph_push_empty_meta(hz4_page_meta_t* meta) {
    uint32_t sc_idx = (uint32_t)meta->sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return;
    }
    // Box boundary: PageQ (segment-local) and CPH (global) must be disjoint.
    // Never push a page into CPH while it is still queued in PageQ; it can later
    // appear in PageQ drain as "cph_queued!=0" and become pathological.
    if (atomic_load_explicit(&meta->queued, memory_order_acquire) != 0) {
        return;
    }
#if HZ4_FAILFAST
    if (((!meta->decommitted) && !HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT) || meta->used_count != 0) {
        fprintf(stderr, "[HZ4_CPH_PUSH_INVALID] meta=%p sc=%u used=%u decommitted=%u initialized=%u\n",
                (void*)meta, (unsigned)meta->sc, (unsigned)meta->used_count,
                (unsigned)meta->decommitted, (unsigned)meta->initialized);
        HZ4_FAIL("CPH push: invalid meta state");
    }
#endif
    uint8_t expected = HZ4_CPH_NONE;
    if (!atomic_compare_exchange_strong_explicit(&meta->cph_queued, &expected, HZ4_CPH_QUEUED,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire)) {
        return;
    }
    hz4_cph_sc_t* sc_stack = &g_cph_sc[sc_idx];

    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    meta->cph_next = sc_stack->top;
    sc_stack->top = meta;
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
}

// Pop empty page from central heap (owner-strict meta version)
static inline hz4_page_meta_t* hz4_cph_pop_empty_meta(uint8_t sc, uint16_t tid) {
    uint32_t sc_idx = (uint32_t)sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return NULL;
    }
    hz4_cph_sc_t* sc_stack = &g_cph_sc[sc_idx];

    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    hz4_page_meta_t* prev = NULL;
    hz4_page_meta_t* cur = sc_stack->top;
    while (cur && cur->owner_tid != tid) {
        prev = cur;
        cur = cur->cph_next;
    }
    if (!cur) {
        atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
        return NULL;
    }
    hz4_page_meta_t* next = cur->cph_next;
    if (prev) {
        prev->cph_next = next;
    } else {
        sc_stack->top = next;
    }
    cur->cph_next = NULL;
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    atomic_store_explicit(&cur->cph_queued, HZ4_CPH_INFLIGHT, memory_order_release);
    return cur;
}

// Remove a specific meta from central heap (safety adopt path)
static inline bool hz4_cph_remove_meta(hz4_page_meta_t* meta) {
#if HZ4_CPH_2TIER
    return hz4_cph_remove_meta_any(meta);
#else
    if (!meta) {
        return false;
    }
    uint32_t sc_idx = (uint32_t)meta->sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return false;
    }
    hz4_cph_sc_t* sc_stack = &g_cph_sc[sc_idx];

    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    hz4_page_meta_t* prev = NULL;
    hz4_page_meta_t* cur = sc_stack->top;
    bool removed = false;
    while (cur) {
        if (cur == meta) {
            hz4_page_meta_t* next = cur->cph_next;
            if (prev) {
                prev->cph_next = next;
            } else {
                sc_stack->top = next;
            }
            cur->cph_next = NULL;
            removed = true;
            break;
        }
        prev = cur;
        cur = cur->cph_next;
    }
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    if (removed) {
        atomic_store_explicit(&meta->cph_queued, HZ4_CPH_INFLIGHT, memory_order_release);
    }
    return removed;
#endif
}

#if HZ4_CPH_2TIER
static inline bool hz4_cph_hot_push_meta(hz4_page_meta_t* meta) {
    uint32_t sc_idx = (uint32_t)meta->sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return false;
    }
    // Box boundary: do not mix PageQ membership with CPH membership.
    if (atomic_load_explicit(&meta->queued, memory_order_acquire) != 0) {
        return false;
    }
    uint8_t expected = HZ4_CPH_NONE;
    if (!atomic_compare_exchange_strong_explicit(&meta->cph_queued, &expected, HZ4_CPH_QUEUED,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire)) {
        return false;
    }
    uint32_t prev = atomic_fetch_add_explicit(&g_cph_hot_count[sc_idx], 1, memory_order_acq_rel);
    if (prev >= HZ4_CPH_HOT_MAX_PAGES) {
        atomic_fetch_sub_explicit(&g_cph_hot_count[sc_idx], 1, memory_order_acq_rel);
        atomic_store_explicit(&meta->cph_queued, HZ4_CPH_NONE, memory_order_release);
        hz4_os_stats_cph_hot_full();
        return false;
    }
    atomic_store_explicit(&meta->cph_state, HZ4_CPH_HOT, memory_order_release);
    hz4_cph_sc_t* sc_stack = &g_cph_hot_sc[sc_idx];
    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    meta->cph_next = sc_stack->top;
    sc_stack->top = meta;
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    hz4_os_stats_cph_hot_push();
    return true;
}

static inline bool hz4_cph_cold_push_meta(hz4_page_meta_t* meta) {
    uint32_t sc_idx = (uint32_t)meta->sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return false;
    }
    // Box boundary: do not mix PageQ membership with CPH membership.
    if (atomic_load_explicit(&meta->queued, memory_order_acquire) != 0) {
        return false;
    }
    uint8_t expected = HZ4_CPH_NONE;
    if (!atomic_compare_exchange_strong_explicit(&meta->cph_queued, &expected, HZ4_CPH_QUEUED,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire)) {
        return false;
    }
    atomic_store_explicit(&meta->cph_state, HZ4_CPH_COLD, memory_order_release);
    hz4_cph_sc_t* sc_stack = &g_cph_cold_sc[sc_idx];
    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    meta->cph_next = sc_stack->top;
    sc_stack->top = meta;
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    hz4_os_stats_cph_cold_push();
    return true;
}

static inline hz4_page_meta_t* hz4_cph_hot_pop_meta(uint8_t sc) {
    uint32_t sc_idx = (uint32_t)sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return NULL;
    }
    hz4_cph_sc_t* sc_stack = &g_cph_hot_sc[sc_idx];
    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    hz4_page_meta_t* cur = sc_stack->top;
    if (cur) {
        sc_stack->top = cur->cph_next;
        cur->cph_next = NULL;
    }
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    if (!cur) return NULL;
    atomic_store_explicit(&cur->cph_queued, HZ4_CPH_INFLIGHT, memory_order_release);
    atomic_fetch_sub_explicit(&g_cph_hot_count[sc_idx], 1, memory_order_acq_rel);
    hz4_os_stats_cph_hot_pop();
    return cur;
}

static inline hz4_page_meta_t* hz4_cph_cold_pop_meta(uint8_t sc) {
    uint32_t sc_idx = (uint32_t)sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return NULL;
    }
    hz4_cph_sc_t* sc_stack = &g_cph_cold_sc[sc_idx];
    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    hz4_page_meta_t* cur = sc_stack->top;
    if (cur) {
        sc_stack->top = cur->cph_next;
        cur->cph_next = NULL;
    }
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    if (!cur) return NULL;
    atomic_store_explicit(&cur->cph_queued, HZ4_CPH_INFLIGHT, memory_order_release);
    hz4_os_stats_cph_cold_pop();
    return cur;
}

// Try to extract a HOT page from CPH for decommit (reclaim path).
// Returns true on successful removal; meta is no longer queued in CPH.
static inline bool hz4_cph_hot_try_extract_for_decommit(hz4_page_meta_t* meta) {
    if (!meta) return false;
    uint8_t state = atomic_load_explicit(&meta->cph_state, memory_order_acquire);
    if (state != HZ4_CPH_HOT) {
        hz4_os_stats_cph_extract_fail_state();
#if !HZ4_CPH_EXTRACT_RELAX_STATE
        return false;
#endif
    }
    uint8_t queued = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
    if (queued != HZ4_CPH_QUEUED && queued != HZ4_CPH_INFLIGHT) {
        hz4_os_stats_cph_extract_fail_queued();
        return false;
    }
    uint32_t sc_idx = (uint32_t)meta->sc;
    if (sc_idx >= HZ4_SC_MAX) {
        return false;
    }
    hz4_cph_sc_t* sc_stack = &g_cph_hot_sc[sc_idx];
    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    hz4_page_meta_t* prev = NULL;
    hz4_page_meta_t* cur = sc_stack->top;
    bool removed = false;
    while (cur) {
        if (cur == meta) {
            hz4_page_meta_t* next = cur->cph_next;
            if (prev) {
                prev->cph_next = next;
            } else {
                sc_stack->top = next;
            }
            cur->cph_next = NULL;
            removed = true;
            break;
        }
        prev = cur;
        cur = cur->cph_next;
    }
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    if (!removed) {
        hz4_os_stats_cph_extract_fail_notfound();
        return false;
    }

    atomic_fetch_sub_explicit(&g_cph_hot_count[sc_idx], 1, memory_order_acq_rel);
    atomic_store_explicit(&meta->cph_queued, HZ4_CPH_NONE, memory_order_release);
    atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_release);
    return true;
}

static inline bool hz4_cph_remove_meta_any(hz4_page_meta_t* meta) {
    if (!meta) return false;
    uint8_t state = atomic_load_explicit(&meta->cph_state, memory_order_acquire);
    uint32_t sc_idx = (uint32_t)meta->sc;
    if (sc_idx >= HZ4_SC_MAX) return false;
    hz4_cph_sc_t* sc_stack = (state == HZ4_CPH_COLD) ? &g_cph_cold_sc[sc_idx] : &g_cph_hot_sc[sc_idx];
    while (atomic_flag_test_and_set_explicit(&sc_stack->lock, memory_order_acquire)) {
    }
    hz4_page_meta_t* prev = NULL;
    hz4_page_meta_t* cur = sc_stack->top;
    bool removed = false;
    while (cur) {
        if (cur == meta) {
            hz4_page_meta_t* next = cur->cph_next;
            if (prev) {
                prev->cph_next = next;
            } else {
                sc_stack->top = next;
            }
            cur->cph_next = NULL;
            removed = true;
            break;
        }
        prev = cur;
        cur = cur->cph_next;
    }
    atomic_flag_clear_explicit(&sc_stack->lock, memory_order_release);
    if (removed) {
        atomic_store_explicit(&meta->cph_queued, HZ4_CPH_INFLIGHT, memory_order_release);
        if (state == HZ4_CPH_HOT) {
            atomic_fetch_sub_explicit(&g_cph_hot_count[sc_idx], 1, memory_order_acq_rel);
        }
    }
    return removed;
}
#endif

// Initialize CPH (startup)
void hz4_cph_init(void);

// Cleanup CPH (shutdown)
void hz4_cph_cleanup(void);

#endif // HZ4_CENTRAL_PAGEHEAP
#endif // HZ4_CENTRAL_PAGEHEAP_H
