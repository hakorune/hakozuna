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
#include <inttypes.h>

// ============================================================================
// TLS Data
// ============================================================================

extern HZ3_TLS Hz3TCache t_hz3_cache;

#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_medium_purged_route_hot_runs = 0;
static _Atomic(uint64_t) g_s65_medium_purged_route_cold_runs = 0;
static _Atomic(uint64_t) g_s65_medium_purged_route_split_batches = 0;

static inline void s65_note_purged_route(uint32_t hot_n, uint32_t cold_n) {
    if (hot_n > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purged_route_hot_runs,
                                  hot_n,
                                  memory_order_relaxed);
    }
    if (cold_n > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purged_route_cold_runs,
                                  cold_n,
                                  memory_order_relaxed);
    }
    if (hot_n > 0 && cold_n > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purged_route_split_batches,
                                  1,
                                  memory_order_relaxed);
    }
}
#else
#define s65_note_purged_route(hot_n, cold_n) do { (void)(hot_n); (void)(cold_n); } while (0)
#endif

static void s65_push_purged_hold_list(int shard,
                                      int sc,
                                      void* head,
                                      void* tail,
                                      uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }

#if HZ3_S65_CENTRAL_COLD_ENABLE && HZ3_S65_CENTRAL_COLD_PARK_PURGED
    uint32_t hot_n = 0;

#if HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS > 0
    uint32_t hot_count = hz3_central_count_snapshot(shard, sc);
    if (hot_count < (uint32_t)HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS) {
        uint32_t need = (uint32_t)HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS - hot_count;
        hot_n = (need < n) ? need : n;
    }
#endif

    if (hot_n == 0) {
        s65_note_purged_route(0, n);
        hz3_central_cold_push_list(shard, sc, head, tail, n);
        return;
    }

    if (hot_n >= n) {
        s65_note_purged_route(n, 0);
        hz3_central_push_list(shard, sc, head, tail, n);
        return;
    }

    void* hot_head = head;
    void* hot_tail = head;
    for (uint32_t i = 1; i < hot_n; i++) {
        hot_tail = hz3_obj_get_next(hot_tail);
    }

    void* cold_head = hz3_obj_get_next(hot_tail);
    hz3_obj_set_next(hot_tail, NULL);

    s65_note_purged_route(hot_n, n - hot_n);
    hz3_central_push_list(shard, sc, hot_head, hot_tail, hot_n);
    hz3_central_cold_push_list(shard, sc, cold_head, tail, n - hot_n);
#else
    s65_note_purged_route(n, 0);
    hz3_central_push_list(shard, sc, head, tail, n);
#endif
}

// ============================================================================
// Stats (global atomics)
// ============================================================================

#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_medium_reclaim_runs = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_pages = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_boundary_calls = 0;
static _Atomic(uint64_t) g_s65_medium_purge_attempt_runs = 0;
static _Atomic(uint64_t) g_s65_medium_purge_attempt_pages = 0;
static _Atomic(uint64_t) g_s65_medium_purge_madvise_calls = 0;
static _Atomic(uint64_t) g_s65_medium_purge_madvise_fail = 0;
static _Atomic(uint64_t) g_s65_medium_purge_sc_runs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_medium_purge_sc_pages[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_medium_reclaim_skip_null_meta = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_skip_bounds = 0;
static _Atomic(uint64_t) g_s65_medium_reclaim_skip_collision = 0;

static _Atomic(int) g_s65_medium_atexit_registered = 0;

static void s65_medium_atexit_dump(void) {
    uint64_t runs = atomic_load_explicit(&g_s65_medium_reclaim_runs, memory_order_relaxed);
    uint64_t pages = atomic_load_explicit(&g_s65_medium_reclaim_pages, memory_order_relaxed);
    uint64_t bcalls = atomic_load_explicit(&g_s65_medium_reclaim_boundary_calls, memory_order_relaxed);
    uint64_t purge_runs = atomic_load_explicit(&g_s65_medium_purge_attempt_runs, memory_order_relaxed);
    uint64_t purge_pages = atomic_load_explicit(&g_s65_medium_purge_attempt_pages, memory_order_relaxed);
    uint64_t purge_calls = atomic_load_explicit(&g_s65_medium_purge_madvise_calls, memory_order_relaxed);
    uint64_t purge_fail = atomic_load_explicit(&g_s65_medium_purge_madvise_fail, memory_order_relaxed);
    uint64_t skip_meta = atomic_load_explicit(&g_s65_medium_reclaim_skip_null_meta, memory_order_relaxed);
    uint64_t skip_bounds = atomic_load_explicit(&g_s65_medium_reclaim_skip_bounds, memory_order_relaxed);
    uint64_t skip_collision = atomic_load_explicit(&g_s65_medium_reclaim_skip_collision, memory_order_relaxed);
    uint64_t route_hot = atomic_load_explicit(&g_s65_medium_purged_route_hot_runs, memory_order_relaxed);
    uint64_t route_cold = atomic_load_explicit(&g_s65_medium_purged_route_cold_runs, memory_order_relaxed);
    uint64_t route_split = atomic_load_explicit(&g_s65_medium_purged_route_split_batches, memory_order_relaxed);

    if (runs > 0 || pages > 0 || purge_runs > 0 || purge_pages > 0 ||
        purge_calls > 0 || purge_fail > 0 || skip_meta > 0 ||
        skip_bounds > 0 || skip_collision > 0 || route_hot > 0 ||
        route_cold > 0 || route_split > 0) {
        fprintf(stderr,
                "[HZ3_S65_MEDIUM] release_runs=%" PRIu64 " release_pages=%" PRIu64 " boundary_calls=%" PRIu64 "\n"
                "  purge_attempt_runs=%" PRIu64 " purge_attempt_pages=%" PRIu64 " madvise_calls=%" PRIu64 " madvise_fail=%" PRIu64 "\n"
                "  skip_null_meta=%" PRIu64 " skip_bounds=%" PRIu64 " skip_collision=%" PRIu64 "\n"
                "  purged_route_hot=%" PRIu64 " purged_route_cold=%" PRIu64 " purged_route_split_batches=%" PRIu64
                " hot_reserve_runs=%d\n",
                runs, pages, bcalls,
                purge_runs, purge_pages, purge_calls, purge_fail,
                skip_meta, skip_bounds, skip_collision,
                route_hot, route_cold, route_split,
                HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS);
        fprintf(stderr, "  purge_by_sc:");
        int printed = 0;
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            uint64_t sc_runs = atomic_load_explicit(&g_s65_medium_purge_sc_runs[sc], memory_order_relaxed);
            uint64_t sc_pages = atomic_load_explicit(&g_s65_medium_purge_sc_pages[sc], memory_order_relaxed);
            if (sc_runs > 0 || sc_pages > 0) {
                fprintf(stderr, " sc%d=%" PRIu64 "r/%" PRIu64 "p", sc, sc_runs, sc_pages);
                printed = 1;
            }
        }
        if (!printed) {
            fprintf(stderr, " none");
        }
        fprintf(stderr, "\n");
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

static void hz3_s65_medium_reclaim_run_for_shard(uint8_t target_shard, int only_sc, uint32_t budget) {
    if (budget == 0) {
        return;
    }
    if (target_shard >= HZ3_NUM_SHARDS) {
        return;
    }
    if (only_sc >= HZ3_NUM_SC) {
        return;
    }

#if HZ3_S65_STATS
    // Stats for this run.
    uint32_t reclaim_runs = 0;
    uint32_t reclaim_pages = 0;
    uint32_t boundary_calls = 0;
    uint32_t purge_attempt_runs = 0;
    uint32_t purge_attempt_pages = 0;
    uint32_t purge_madvise_calls = 0;
    uint32_t purge_madvise_fail = 0;
    uint32_t purge_sc_runs[HZ3_NUM_SC] = {0};
    uint32_t purge_sc_pages[HZ3_NUM_SC] = {0};
#endif

    enum { HZ3_S65_MEDIUM_RECLAIM_ACTION_RELEASE = 1, HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY = 2 };
    int action = HZ3_S65_MEDIUM_RECLAIM_MODE;
    if (action == 0) {
        // Auto: medium has no safe predicate yet; default to purge_only.
        action = HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY;
    }

#if HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD
    if (action == HZ3_S65_MEDIUM_RECLAIM_ACTION_RELEASE &&
        hz3_shard_live_count(target_shard) > 1) {
        static _Atomic int g_s65_collision_shot = 0;
        if (!HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT ||
            atomic_exchange_explicit(&g_s65_collision_shot, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr,
                    "[HZ3_S65_MEDIUM_RECLAIM] collision_guard=1 action=release->purge_only shard=%u live=%u\n",
                    (unsigned)target_shard,
                    (unsigned)hz3_shard_live_count(target_shard));
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

    // Drain central bins: sc=7→0 (large to small) for better syscall efficiency.
    // Targeted owner cleanup can restrict this to one size class.
    int sc_start = (only_sc >= 0) ? only_sc : (HZ3_NUM_SC - 1);
    int sc_end = (only_sc >= 0) ? only_sc : 0;
    for (int sc = sc_start; sc >= sc_end && budget > 0; sc--) {
        // purge_only mode: keep runs in central, but move via a local list to avoid re-popping the same nodes.
        void* hold_head = NULL;
        void* hold_tail = NULL;
        uint32_t hold_n = 0;

        void* obj;
        while (budget > 0 && (obj = hz3_central_pop(target_shard, sc)) != NULL) {

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

#if HZ3_S65_STATS
                if (ret == 0) {
                    reclaim_runs++;
                    reclaim_pages += pages;
                    boundary_calls++;
                }
#else
                (void)ret;
#endif
            } else {
                // purge_only: keep (tag/bin/owner) intact and return to central.
                // This avoids handing out a run whose pages were reclassified while stale copies exist.
                void* run_addr = (void*)(seg_base + ((uintptr_t)page_idx << HZ3_PAGE_SHIFT));
                size_t run_size = (size_t)pages << HZ3_PAGE_SHIFT;
                int ret = hz3_os_madvise_dontneed_checked(run_addr, run_size);
#if HZ3_S65_STATS
                purge_attempt_runs++;
                purge_attempt_pages += pages;
                purge_madvise_calls++;
                purge_sc_runs[sc]++;
                purge_sc_pages[sc] += pages;
                if (ret != 0) {
                    purge_madvise_fail++;
                }
#else
                (void)ret;
#endif

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
            hz3_pack_on_free(target_shard, meta);
#endif

            budget--;
        }

        if (action == HZ3_S65_MEDIUM_RECLAIM_ACTION_PURGE_ONLY && hold_head) {
            s65_push_purged_hold_list(target_shard, sc, hold_head, hold_tail, hold_n);
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
    if (purge_attempt_runs > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purge_attempt_runs, purge_attempt_runs, memory_order_relaxed);
    }
    if (purge_attempt_pages > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purge_attempt_pages, purge_attempt_pages, memory_order_relaxed);
    }
    if (purge_madvise_calls > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purge_madvise_calls, purge_madvise_calls, memory_order_relaxed);
    }
    if (purge_madvise_fail > 0) {
        atomic_fetch_add_explicit(&g_s65_medium_purge_madvise_fail, purge_madvise_fail, memory_order_relaxed);
    }
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        if (purge_sc_runs[sc] > 0) {
            atomic_fetch_add_explicit(&g_s65_medium_purge_sc_runs[sc], purge_sc_runs[sc], memory_order_relaxed);
        }
        if (purge_sc_pages[sc] > 0) {
            atomic_fetch_add_explicit(&g_s65_medium_purge_sc_pages[sc], purge_sc_pages[sc], memory_order_relaxed);
        }
    }
#endif
}

static void hz3_s65_medium_reclaim_run(uint32_t budget) {
    hz3_s65_medium_reclaim_run_for_shard(t_hz3_cache.my_shard, -1, budget);
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

void hz3_s65_medium_reclaim_shard_sc(uint8_t shard, int sc, uint32_t budget, uint32_t reason) {
    (void)reason;
    if (sc < 0 || sc >= HZ3_NUM_SC) {
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
    hz3_s65_medium_reclaim_run_for_shard(shard, sc, budget);
}

#endif  // HZ3_S65_MEDIUM_RECLAIM
