// hz3_s65_medium_reclaim.c - S65-2C Medium Epoch Reclaim implementation
//
// Port of S61 dtor reclaim to epoch tick for medium runs.
// Key difference from S61: Uses boundary API instead of hz3_segment_free_run().

#include "hz3_s65_medium_reclaim.h"

#if HZ3_S65_MEDIUM_RECLAIM

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_segmap.h"
#include "hz3_sc.h"
#include "hz3_release_boundary.h"
#include "hz3_segment_packing.h"
#include "hz3_os_purge.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <errno.h>

// ============================================================================
// TLS Data
// ============================================================================

extern HZ3_TLS Hz3TCache t_hz3_cache;

// ============================================================================
// Stats (global atomics)
// ============================================================================

#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_medium_reclaim_runs = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_pages = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_boundary_calls = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_skip_null_meta = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_skip_bounds = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_skip_collision = 0;

static _Atomic(int) g_s65_medium_atexit_registered = 0;

static void s65_medium_atexit_dump(void) {
    uint64_t runs = atomic_load_explicit(&g_s65_medium_reclaim_runs, memory_order_relaxed);
    uint64_t pages = atomic_load_explicit(&g_s65_medium_reclaim_pages, memory_order_relaxed);
    uint64_t bcalls = atomic_load_explicit(&g_s65_medium_reclaim_boundary_calls, memory_order_relaxed);
    uint64_t skip_meta = atomic_load_explicit(&g_s65_medium_reclaim_skip_null_meta, memory_order_relaxed);
    uint64_t skip_bounds = atomic_load_explicit(&g_s65_medium_reclaim_skip_bounds, memory_order_relaxed);
    uint64_t skip_collision = atomic_load_explicit(&g_s65_medium_reclaim_skip_collision, memory_order_relaxed);

    if (runs > 0 || pages > 0) {
        fprintf(stderr,
                "[HZ3_S65_MEDIUM] reclaim_runs=%lu reclaim_pages=%lu boundary_calls=%lu\n"
                "  skip_null_meta=%lu skip_bounds=%lu skip_collision=%lu\n",
                runs, pages, bcalls, skip_meta, skip_bounds, skip_collision);
    }
}

static void s65_medium_stats_register_atexit(void) {
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&g_s65_medium_atexit_registered,
                                                 &expected, 1,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed)) {
        atexit(s65_medium_atexit_dump);
    }
}
#endif

// ============================================================================
// S80: Medium reclaim runtime gate (env-based)
// ============================================================================

#if HZ3_S80_MEDIUM_RECLAIM_GATE
static _Atomic int g_s80_medium_reclaim_inited = 0;
static _Atomic int g_s80_medium_reclaim_enabled = HZ3_S80_MEDIUM_RECLAIM_DEFAULT;
static _Atomic int g_s80_medium_reclaim_logged = 0;

static int hz3_s80_parse_bool(const char* s, int defval) {
    if (!s || !*s) {
        return defval;
    }
    errno = 0;
    char* end = NULL;
    long v = strtol(s, &end, 0);
    if (errno != 0 || end == s) {
        return defval;
    }
    return (v != 0) ? 1 : 0;
}

static inline int hz3_s80_medium_reclaim_is_enabled(void) {
    if (atomic_load_explicit(&g_s80_medium_reclaim_inited, memory_order_acquire) == 0) {
        int enabled = HZ3_S80_MEDIUM_RECLAIM_DEFAULT;
        const char* s = getenv("HZ3_S80_MEDIUM_RECLAIM");
        enabled = hz3_s80_parse_bool(s, enabled);
        atomic_store_explicit(&g_s80_medium_reclaim_enabled, enabled, memory_order_release);
        atomic_store_explicit(&g_s80_medium_reclaim_inited, 1, memory_order_release);
#if HZ3_S80_MEDIUM_RECLAIM_LOG
        if (!enabled &&
            atomic_exchange_explicit(&g_s80_medium_reclaim_logged, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr, "[HZ3_S80_MEDIUM_RECLAIM] disabled via env\n");
        }
#endif
    }
    return atomic_load_explicit(&g_s80_medium_reclaim_enabled, memory_order_acquire);
}
#endif

// ============================================================================
// Main tick function
// ============================================================================

static void hz3_s65_medium_reclaim_run(uint32_t budget) {
    if (budget == 0) {
        return;
    }
    uint8_t my_shard = t_hz3_cache.my_shard;

    // Stats for this run
    uint32_t reclaim_runs = 0;
    uint32_t reclaim_pages = 0;
    uint32_t boundary_calls = 0;

    enum { HZ3_S65_MEDIUM_RECLAIM_ACTION_RELEASE = 1, HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY = 2 };
    int action = HZ3_S65_MEDIUM_RECLAIM_MODE;
    if (action == 0) {
        // Auto: medium has no safe predicate yet; default to purge_only.
        action = HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY;
    }

#if HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD
    if (action == HZ3_S65_MEDIUM_RECLAIM_ACTION_RELEASE &&
        hz3_shard_live_count(t_hz3_cache.my_shard) > 1) {
        static _Atomic int g_s65_collision_shot = 0;
        if (!HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT ||
            atomic_exchange_explicit(&g_s65_collision_shot, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr,
                    "[HZ3_S65_MEDIUM_RECLAIM] collision_guard=1 action=release->purge_only shard=%u live=%u\n",
                    (unsigned)t_hz3_cache.my_shard,
                    (unsigned)hz3_shard_live_count(t_hz3_cache.my_shard));
        }
        if (HZ3_S65_MEDIUM_RECLAIM_COLLISION_FAILFAST) {
            abort();
        }
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_medium_reclaim_skip_collision, 1, memory_order_relaxed);
#endif
        action = HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY;
    }
#endif

#if HZ3_S80_MEDIUM_RECLAIM_LOG
    if (HZ3_S65_MEDIUM_RECLAIM_MODE == 0) {
        static _Atomic int g_s65_medium_mode_logged = 0;
        if (atomic_exchange_explicit(&g_s65_medium_mode_logged, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr, "[HZ3_S65_MEDIUM_RECLAIM] auto_mode=purge_only (no medium-safe predicate yet; use MODE=1 only for repro)\n");
        }
    }
#endif

    // Drain central bins: sc=7→0 (large to small) for better syscall efficiency
    for (int sc = HZ3_NUM_SC - 1; sc >= 0 && budget > 0; sc--) {
        // purge_only mode: keep runs in central, but move via a local list to avoid re-popping the same nodes.
        void* hold_head = NULL;
        void* hold_tail = NULL;
        uint32_t hold_n = 0;

        void* obj;
        while (budget > 0 && (obj = hz3_central_pop(my_shard, sc)) != NULL) {

            // 1. Object -> Segment base (2MB-aligned)
            uintptr_t ptr = (uintptr_t)obj;
            uintptr_t seg_base = ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);

            // 2. Get segment metadata from segmap
            Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
            if (!meta) {
                // Safety: external allocation or unmapped - skip
#if HZ3_S65_STATS
                atomic_fetch_add_explicit(&g_s65_medium_reclaim_skip_null_meta, 1, memory_order_relaxed);
#endif
                continue;
            }

            // 3. Calculate page index and pages for this size class
            uint32_t page_idx = (uint32_t)((ptr - seg_base) >> HZ3_PAGE_SHIFT);
            uint32_t pages = (uint32_t)hz3_sc_to_pages(sc);

            // Boundary check (Fail-Fast: skip invalid bounds)
            if (page_idx + pages > HZ3_PAGES_PER_SEG) {
#if HZ3_S65_STATS
                atomic_fetch_add_explicit(&g_s65_medium_reclaim_skip_bounds, 1, memory_order_relaxed);
#endif
                continue;
            }

            if (action == HZ3_S65_MEDIUM_RECLAIM_ACTION_RELEASE) {
                // 4. Call boundary API (instead of hz3_segment_free_run)
                // This updates free_bits, free_pages, sc_tag, and enqueues to ledger
                int ret = hz3_release_range_definitely_free_meta(
                    meta, page_idx, pages, HZ3_RELEASE_MEDIUM_RUN_RECLAIM);

                if (ret == 0) {
                    reclaim_runs++;
                    reclaim_pages += pages;
                    boundary_calls++;
                }
            } else {
                // purge_only: keep (tag/bin/owner) intact and return to central.
                // This avoids handing out a run whose pages were reclassified while stale copies exist.
                void* run_addr = (void*)(seg_base + ((uintptr_t)page_idx << HZ3_PAGE_SHIFT));
                size_t run_size = (size_t)pages << HZ3_PAGE_SHIFT;
                (void)hz3_os_madvise_dontneed_checked(run_addr, run_size);

                hz3_obj_set_next(obj, NULL);
                if (!hold_head) {
                    hold_head = obj;
                } else {
                    hz3_obj_set_next(hold_tail, obj);
                }
                hold_tail = obj;
                hold_n++;
            }

            // 5. Update pack pool (keep pack pool consistent)
#if HZ3_S49_SEGMENT_PACKING
            hz3_pack_on_free(my_shard, meta);
#endif

            budget--;
        }

        if (action == HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY && hold_head) {
            hz3_central_push_list(my_shard, sc, hold_head, hold_tail, hold_n);
        }
    }

    // Update global stats
#if HZ3_S65_STATS
    if (reclaim_runs > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_reclaim_runs, reclaim_runs, memory_order_relaxed);
    }
    if (reclaim_pages > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_reclaim_pages, reclaim_pages, memory_order_relaxed);
    }
    if (boundary_calls > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_reclaim_boundary_calls, boundary_calls, memory_order_relaxed);
    }
#endif
}

void hz3_s65_medium_reclaim_tick(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }
#if HZ3_S80_MEDIUM_RECLAIM_GATE
    if (!hz3_s80_medium_reclaim_is_enabled()) {
        return;
    }
#endif
#if HZ3_S65_STATS
    s65_medium_stats_register_atexit();
#endif
    hz3_s65_medium_reclaim_run(HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS);
}

void hz3_s65_medium_reclaim_on_demand(int sc, int want, uint32_t reason) {
    (void)reason;
#if !HZ3_S65_MEDIUM_RECLAIM_DEMAND
    (void)sc;
    (void)want;
    return;
#else
    if (!t_hz3_cache.initialized) {
        return;
    }
#if HZ3_S80_MEDIUM_RECLAIM_GATE
    if (!hz3_s80_medium_reclaim_is_enabled()) {
        return;
    }
#endif
#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_SKIP_REMOTE_HINT
    if (t_hz3_cache.remote_hint) {
        return;
    }
#endif
    if (sc < HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN ||
        sc > HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX) {
        return;
    }
    uint32_t stride = (uint32_t)HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE;
    if (stride > 1) {
        uint32_t miss = t_hz3_cache.stats.central_pop_miss[sc];
        if ((miss % stride) != 0) {
            return;
        }
    }
#if HZ3_S65_STATS
    s65_medium_stats_register_atexit();
#endif
    uint32_t budget = (uint32_t)HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS;
#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_SCALE > 0
    if (want > 0) {
        uint32_t scaled = (uint32_t)want * (uint32_t)HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_SCALE;
        if (scaled > budget) {
            budget = scaled;
        }
    }
#else
    (void)want;
#endif
    hz3_s65_medium_reclaim_run(budget);
#endif
}

#endif  // HZ3_S65_MEDIUM_RECLAIM
