// ============================================================================
// S58: TCache Decay Box - tcache -> central -> segment reclaim
// ============================================================================
//
// Purpose:
// - S57-A/B showed "tcache holds freed memory" is the root cause of RSS gap
// - S58-1: tcache -> central via dynamic bin_target (reuse existing hz3_bin_trim)
// - S58-2: central -> segment via hz3_segment_free_run (budgeted)
//
// Design:
// - Medium bins only (HZ3_NUM_SC = 16) to minimize tiny performance risk
// - Epoch boundary only (no hot path cost)
// - Overage observation: len > max_len triggers shrink after MAX_OVERAGES
// - Central reclaim: pop from central, return to segment (free_pages increase)

#define _GNU_SOURCE

#include "hz3_tcache_decay.h"

#if HZ3_S58_TCACHE_DECAY

#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_sc.h"
#include "hz3_owner_lease.h"
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S60_PURGE_RANGE_QUEUE
#include "hz3_purge_range_queue.h"
#endif
#include "hz3_owner_excl.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#if HZ3_BIN_LAZY_COUNT && HZ3_S123_S58_OVERAGE_BOUNDED
static inline int hz3_bin_has_more_than(void* head, uint16_t max_len) {
    uint16_t n = 0;
    void* p = head;
    while (p && n <= max_len) {
        p = hz3_obj_get_next(p);
        n++;
    }
    return n > max_len;
}
#endif

// ============================================================================
// S58 Statistics (global atomic, atexit one-shot)
// ============================================================================

#if HZ3_S58_STATS
static _Atomic uint64_t g_s58_overage_shrinks = 0;
static _Atomic uint64_t g_s58_reclaim_objs = 0;
static _Atomic uint64_t g_s58_reclaim_pages = 0;
static _Atomic uint64_t g_s58_adjust_calls = 0;
static _Atomic uint64_t g_s58_reclaim_calls = 0;
static pthread_once_t g_s58_atexit_once = PTHREAD_ONCE_INIT;

static void hz3_s58_atexit_dump(void) {
    fprintf(stderr, "[HZ3_S58_TCACHE_DECAY] adjust_calls=%lu overage_shrinks=%lu "
            "reclaim_calls=%lu reclaim_objs=%lu reclaim_pages=%lu\n",
            (unsigned long)atomic_load(&g_s58_adjust_calls),
            (unsigned long)atomic_load(&g_s58_overage_shrinks),
            (unsigned long)atomic_load(&g_s58_reclaim_calls),
            (unsigned long)atomic_load(&g_s58_reclaim_objs),
            (unsigned long)atomic_load(&g_s58_reclaim_pages));
}

static void hz3_s58_atexit_register_impl(void) {
    atexit(hz3_s58_atexit_dump);
}

void hz3_s58_stats_register_atexit(void) {
    pthread_once(&g_s58_atexit_once, hz3_s58_atexit_register_impl);
}
#endif  // HZ3_S58_STATS

// ============================================================================
// S58: Initialize decay fields in TLS
// ============================================================================

void hz3_s58_init(void) {
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        t_hz3_cache.s58_max_len[sc] = HZ3_S58_INITIAL_MAX_LEN;
        t_hz3_cache.s58_overage[sc] = 0;
        t_hz3_cache.s58_lowwater[sc] = 0;  // will be set on first epoch
    }
}

// ============================================================================
// S58-1: Adjust bin targets (epoch boundary)
// ============================================================================
//
// Called at epoch before existing hz3_bin_trim() runs.
// - Observes current len vs max_len (overage detection)
// - Shrinks max_len after MAX_OVERAGES consecutive overages
// - Sets bin_target = lowwater / 2 (floor = refill_batch)

void hz3_s58_adjust_targets(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S58_STATS
    atomic_fetch_add(&g_s58_adjust_calls, 1);
#endif

    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        // Get current bin length
        uint32_t bin_idx = hz3_bin_index_medium(sc);
        uint16_t len = 0;
        uint16_t max_len = t_hz3_cache.s58_max_len[sc];

#if HZ3_TCACHE_SOA_LOCAL
        // SoA layout: count is in local_count[]
        len = (uint16_t)t_hz3_cache.local_count[bin_idx];
#elif HZ3_BIN_LAZY_COUNT
#if HZ3_S123_S58_OVERAGE_BOUNDED
        void* head = t_hz3_cache.local_bins[bin_idx].head;
        if (hz3_bin_has_more_than(head, max_len)) {
            len = (uint16_t)(max_len + 1);
        } else {
            // Bounded walk (<= max_len) for lowwater tracking.
            Hz3Bin tmp_bin = { .head = head, .count = 0 };
            len = hz3_bin_count_walk(&tmp_bin, max_len);
        }
#else
        // Lazy count: walk the list (event-only, acceptable overhead)
        Hz3Bin tmp_bin = { .head = t_hz3_cache.local_bins[bin_idx].head, .count = 0 };
        len = hz3_bin_count_walk(&tmp_bin, 256);  // cap at 256 to bound walk
#endif
#else
        // AoS with count
        Hz3Bin* bin = &t_hz3_cache.local_bins[bin_idx];
        len = (uint16_t)bin->count;
#endif

        // 1. Overage detection (len > max_len)
        if (len > max_len) {
            t_hz3_cache.s58_overage[sc]++;
            if (t_hz3_cache.s58_overage[sc] >= HZ3_S58_MAX_OVERAGES) {
                // Shrink max_len by one batch
                uint8_t batch = HZ3_REFILL_BATCH[sc];
                if (max_len > batch) {
                    t_hz3_cache.s58_max_len[sc] = max_len - batch;
                }
                t_hz3_cache.s58_overage[sc] = 0;
#if HZ3_S58_STATS
                atomic_fetch_add(&g_s58_overage_shrinks, 1);
#endif
            }
        } else {
            // Reset overage counter on non-overage
            t_hz3_cache.s58_overage[sc] = 0;
        }

        // 2. Update lowwater
        if (len < t_hz3_cache.s58_lowwater[sc] || t_hz3_cache.s58_lowwater[sc] == 0) {
            t_hz3_cache.s58_lowwater[sc] = len;
        }

        // 3. Adjust bin_target = lowwater / 2 (floor = refill_batch)
        uint16_t new_target = t_hz3_cache.s58_lowwater[sc] / 2;
        uint8_t batch = HZ3_REFILL_BATCH[sc];
        if (new_target < batch) {
            new_target = batch;  // floor at refill_batch
        }
        if (new_target > 255) {
            new_target = 255;  // cap at uint8_t max
        }
#if HZ3_S136_HOTSC_ONLY
        // S136 archived/NO-GO: stubbed (no-op).
        (void)batch;
#endif
        t_hz3_cache.knobs.bin_target[sc] = (uint8_t)new_target;

        // 4. Reset lowwater for next epoch interval
        t_hz3_cache.s58_lowwater[sc] = len;
    }
}

// ============================================================================
// S58-2: Central -> Segment reclaim (epoch boundary, budgeted)
// ============================================================================
//
// Called at epoch after hz3_bin_trim() runs.
// - Pops objects from my_shard's central bins
// - Returns them to segment via hz3_segment_free_run()
// - Budgeted: stops after HZ3_S58_RECLAIM_BUDGET objects

void hz3_s58_central_reclaim(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S58_STATS
    atomic_fetch_add(&g_s58_reclaim_calls, 1);
#endif

    uint8_t my_shard = t_hz3_cache.my_shard;

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_try_acquire(my_shard);
    if (!lease_token.active) {
        return;
    }

    // OwnerExcl: Protect segment metadata from collision race
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(my_shard);

    uint32_t budget = HZ3_S58_RECLAIM_BUDGET;
    uint32_t total_objs = 0;
    uint32_t total_pages = 0;

    // Iterate over medium size classes (sc=0..7)
    for (int sc = 0; sc < HZ3_NUM_SC && budget > 0; sc++) {
        void* obj;
        while (budget > 0 && (obj = hz3_central_pop(my_shard, sc)) != NULL) {
            // Object -> Segment/Page reversal (O(1))
            uintptr_t ptr = (uintptr_t)obj;
            uintptr_t seg_base = ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);

            // Get segment metadata from segmap
            Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
            if (!meta) {
                // Safety: external allocation or unmapped - skip
                continue;
            }

            // Calculate page index and pages for this size class
            uint32_t page_idx = (uint32_t)((ptr - seg_base) >> HZ3_PAGE_SHIFT);
            uint32_t pages = (uint32_t)hz3_sc_to_pages(sc);

            // Boundary check
            if (page_idx + pages > HZ3_PAGES_PER_SEG) {
                // Safety: invalid run bounds - skip
                continue;
            }

            // Return run to segment (marks pages as free, increments free_pages)
            hz3_segment_free_run(meta, page_idx, pages);

#if HZ3_S60_PURGE_RANGE_QUEUE
            // S60: Queue freed range for epoch madvise
            hz3_s60_queue_range((void*)seg_base, (uint16_t)page_idx, (uint16_t)pages);
#endif

#if HZ3_S49_SEGMENT_PACKING
            // S49: Update pack pool after free_run (may increase pack_max_fit)
            hz3_pack_on_free(my_shard, meta);
#endif

            total_objs++;
            total_pages += pages;
            budget--;
        }
    }

#if HZ3_S58_STATS
    if (total_objs > 0) {
        atomic_fetch_add(&g_s58_reclaim_objs, total_objs);
        atomic_fetch_add(&g_s58_reclaim_pages, total_pages);
    }
#endif

    hz3_owner_excl_release(excl_token);
    hz3_owner_lease_release(lease_token);
}

#endif  // HZ3_S58_TCACHE_DECAY
