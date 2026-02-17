// hz3_tcache.c - Thread-local cache core (init, destructor, slow path)
// Split into modules: hz3_tcache_stats.c, hz3_tcache_remote.c,
//                     hz3_tcache_pressure.c, hz3_tcache_alloc.c
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_eco_mode.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_inbox.h"
#include "hz3_knobs.h"
#include "hz3_epoch.h"
#include "hz3_small.h"
#include "hz3_small_v2.h"
#include "hz3_seg_hdr.h"
#include "hz3_tag.h"
#include "hz3_medium_debug.h"
#include "hz3_watch_ptr.h"
#include "hz3_owner_lease.h"
#include "hz3_owner_stash.h"
#if HZ3_LANE_SPLIT
#include "hz3_lane.h"
#endif

#include <sys/mman.h>
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S58_TCACHE_DECAY
#include "hz3_tcache_decay.h"
#endif
#if HZ3_S61_DTOR_HARD_PURGE
#include "hz3_dtor_hard_purge.h"
#endif
#if HZ3_S62_RETIRE
#include "hz3_s62_retire.h"
#endif
#if HZ3_S62_SUB4K_RETIRE
#include "hz3_s62_sub4k.h"
#endif
#if HZ3_S62_PURGE
#include "hz3_s62_purge.h"
#endif
#if HZ3_S62_OBSERVE
#include "hz3_s62_observe.h"
#endif
#if HZ3_S65_RELEASE_LEDGER
#include "hz3_release_ledger.h"
#endif
#if HZ3_S65_MEDIUM_RECLAIM
#include "hz3_s65_medium_reclaim.h"
#endif

#include <string.h>
#include <stdio.h>
#include "hz3_dtor_stats.h"

// ============================================================================
// S203: Alloc Slow Counters (Medium) - TLS Version
// ============================================================================
#if HZ3_S203_COUNTERS
HZ3_DTOR_STATS_BEGIN(S203_ALLOC);
static _Thread_local uint64_t t_s203_alloc_slow_calls = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_inbox = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_central = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_segment = 0;
static _Thread_local uint64_t t_s203_s206_steal_attempts = 0;
static _Thread_local uint64_t t_s203_s206_steal_hits = 0;
static _Thread_local uint64_t t_s203_s208_reserve_attempts = 0;
static _Thread_local uint64_t t_s203_s208_reserve_hits = 0;
static _Thread_local uint64_t t_s203_s208_central_miss_after_reserve = 0;
static _Thread_local uint64_t t_s203_s223_want_boost_calls = 0;
static _Thread_local uint64_t t_s203_s236_mini_calls = 0;
static _Thread_local uint64_t t_s203_s236_mini_hit_inbox = 0;
static _Thread_local uint64_t t_s203_s236_mini_hit_central = 0;
static _Thread_local uint64_t t_s203_s236_mini_miss = 0;
static _Thread_local uint64_t t_s203_s236e_retry_calls = 0;
static _Thread_local uint64_t t_s203_s236e_retry_hits = 0;
static _Thread_local uint64_t t_s203_s236e_retry_skipped_no_supply = 0;
static _Thread_local uint64_t t_s203_s236f_retry_calls = 0;
static _Thread_local uint64_t t_s203_s236f_retry_hits = 0;
static _Thread_local uint64_t t_s203_s236f_retry_skipped_streak = 0;
static _Thread_local uint64_t t_s203_s236g_hint_checks = 0;
static _Thread_local uint64_t t_s203_s236g_hint_positive = 0;
static _Thread_local uint64_t t_s203_s236g_hint_empty_skips = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_calls = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_hits = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_skip_no_hint = 0;
static _Thread_local uint64_t t_s203_s236i_second_inbox_skip_no_backlog = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_calls = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_inbox_extra_objs = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_central_extra_objs = 0;
static _Thread_local uint64_t t_s203_s236n_bulk_zero_extra = 0;
static _Thread_local uint64_t t_s203_alloc_slow_from_inbox_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_alloc_slow_from_central_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_alloc_slow_from_segment_sc[HZ3_NUM_SC];

HZ3_DTOR_STAT(g_s203_alloc_slow_calls);
HZ3_DTOR_STAT(g_s203_alloc_slow_from_inbox);
HZ3_DTOR_STAT(g_s203_alloc_slow_from_central);
HZ3_DTOR_STAT(g_s203_alloc_slow_from_segment);
HZ3_DTOR_STAT(g_s203_s206_steal_attempts);
HZ3_DTOR_STAT(g_s203_s206_steal_hits);
HZ3_DTOR_STAT(g_s203_s208_reserve_attempts);
HZ3_DTOR_STAT(g_s203_s208_reserve_hits);
HZ3_DTOR_STAT(g_s203_s208_central_miss_after_reserve);
HZ3_DTOR_STAT(g_s203_s223_want_boost_calls);
HZ3_DTOR_STAT(g_s203_s236_mini_calls);
HZ3_DTOR_STAT(g_s203_s236_mini_hit_inbox);
HZ3_DTOR_STAT(g_s203_s236_mini_hit_central);
HZ3_DTOR_STAT(g_s203_s236_mini_miss);
HZ3_DTOR_STAT(g_s203_s236e_retry_calls);
HZ3_DTOR_STAT(g_s203_s236e_retry_hits);
HZ3_DTOR_STAT(g_s203_s236e_retry_skipped_no_supply);
HZ3_DTOR_STAT(g_s203_s236f_retry_calls);
HZ3_DTOR_STAT(g_s203_s236f_retry_hits);
HZ3_DTOR_STAT(g_s203_s236f_retry_skipped_streak);
HZ3_DTOR_STAT(g_s203_s236g_hint_checks);
HZ3_DTOR_STAT(g_s203_s236g_hint_positive);
HZ3_DTOR_STAT(g_s203_s236g_hint_empty_skips);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_calls);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_hits);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_skip_no_hint);
HZ3_DTOR_STAT(g_s203_s236i_second_inbox_skip_no_backlog);
HZ3_DTOR_STAT(g_s203_s236n_bulk_calls);
HZ3_DTOR_STAT(g_s203_s236n_bulk_inbox_extra_objs);
HZ3_DTOR_STAT(g_s203_s236n_bulk_central_extra_objs);
HZ3_DTOR_STAT(g_s203_s236n_bulk_zero_extra);
static _Atomic uint64_t g_s203_alloc_slow_from_inbox_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_alloc_slow_from_central_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_alloc_slow_from_segment_sc[HZ3_NUM_SC];
HZ3_DTOR_ATEXIT_FLAG(g_s203_alloc);

// Extern flush functions from other modules
void hz3_s203_remote_flush_tls(void);
void hz3_s203_remote_register_once(void);
void hz3_s203_inbox_flush_tls(void);
#if HZ3_S204_LARSON_DIAG
static void hz3_s204_flush_tls(void);
#endif

// Flush TLS to Global (called from thread dtor)
void hz3_s203_flush_tls(void) {
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_calls, t_s203_alloc_slow_calls);
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_from_inbox, t_s203_alloc_slow_from_inbox);
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_from_central, t_s203_alloc_slow_from_central);
    HZ3_DTOR_STAT_ADD(g_s203_alloc_slow_from_segment, t_s203_alloc_slow_from_segment);
    HZ3_DTOR_STAT_ADD(g_s203_s206_steal_attempts, t_s203_s206_steal_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s206_steal_hits, t_s203_s206_steal_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s208_reserve_attempts, t_s203_s208_reserve_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s208_reserve_hits, t_s203_s208_reserve_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s208_central_miss_after_reserve,
                      t_s203_s208_central_miss_after_reserve);
    HZ3_DTOR_STAT_ADD(g_s203_s223_want_boost_calls,
                      t_s203_s223_want_boost_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_calls,
                      t_s203_s236_mini_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_hit_inbox,
                      t_s203_s236_mini_hit_inbox);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_hit_central,
                      t_s203_s236_mini_hit_central);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mini_miss,
                      t_s203_s236_mini_miss);
    HZ3_DTOR_STAT_ADD(g_s203_s236e_retry_calls,
                      t_s203_s236e_retry_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236e_retry_hits,
                      t_s203_s236e_retry_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236e_retry_skipped_no_supply,
                      t_s203_s236e_retry_skipped_no_supply);
    HZ3_DTOR_STAT_ADD(g_s203_s236f_retry_calls,
                      t_s203_s236f_retry_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236f_retry_hits,
                      t_s203_s236f_retry_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236f_retry_skipped_streak,
                      t_s203_s236f_retry_skipped_streak);
    HZ3_DTOR_STAT_ADD(g_s203_s236g_hint_checks,
                      t_s203_s236g_hint_checks);
    HZ3_DTOR_STAT_ADD(g_s203_s236g_hint_positive,
                      t_s203_s236g_hint_positive);
    HZ3_DTOR_STAT_ADD(g_s203_s236g_hint_empty_skips,
                      t_s203_s236g_hint_empty_skips);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_calls,
                      t_s203_s236i_second_inbox_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_hits,
                      t_s203_s236i_second_inbox_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_skip_no_hint,
                      t_s203_s236i_second_inbox_skip_no_hint);
    HZ3_DTOR_STAT_ADD(g_s203_s236i_second_inbox_skip_no_backlog,
                      t_s203_s236i_second_inbox_skip_no_backlog);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_calls,
                      t_s203_s236n_bulk_calls);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_inbox_extra_objs,
                      t_s203_s236n_bulk_inbox_extra_objs);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_central_extra_objs,
                      t_s203_s236n_bulk_central_extra_objs);
    HZ3_DTOR_STAT_ADD(g_s203_s236n_bulk_zero_extra,
                      t_s203_s236n_bulk_zero_extra);
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s203_alloc_slow_from_inbox_sc[sc],
                                  t_s203_alloc_slow_from_inbox_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_alloc_slow_from_central_sc[sc],
                                  t_s203_alloc_slow_from_central_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_alloc_slow_from_segment_sc[sc],
                                  t_s203_alloc_slow_from_segment_sc[sc],
                                  memory_order_relaxed);
    }
    
    // Flush other modules too
    hz3_s203_remote_flush_tls();
    hz3_s203_inbox_flush_tls();
#if HZ3_S204_LARSON_DIAG
    hz3_s204_flush_tls();
#endif
}

static void hz3_s203_alloc_atexit_dump(void) {
    uint64_t inbox_band[4] = {0, 0, 0, 0};
    uint64_t central_band[4] = {0, 0, 0, 0};
    uint64_t segment_band[4] = {0, 0, 0, 0};
    const char* band_name[4] = {"le32k", "32k_64k", "64k_128k", "gt128k"};

    fprintf(stderr, "[HZ3_S203] alloc_slow_calls=%u "
            "from_inbox=%u from_central=%u from_segment=%u\n",
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_from_inbox),
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_from_central),
            HZ3_DTOR_STAT_LOAD(g_s203_alloc_slow_from_segment));
    if (HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_attempts) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_hits) > 0) {
        fprintf(stderr, "[HZ3_S203_S206] steal_attempts=%u steal_hits=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s206_steal_hits));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_attempts) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_hits) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s208_central_miss_after_reserve) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S208] reserve_attempts=%u reserve_hits=%u "
                "central_miss_after_reserve=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s208_reserve_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s208_central_miss_after_reserve));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s223_want_boost_calls) > 0) {
        fprintf(stderr, "[HZ3_S203_S223] want_boost_calls=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s223_want_boost_calls));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236B] mini_calls=%u hit_inbox=%u hit_central=%u miss=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_hit_inbox),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_hit_central),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mini_miss));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236E] retry_calls=%u retry_hits=%u retry_skipped_no_supply=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236e_retry_skipped_no_supply));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_calls) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_skipped_streak) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236F] retry_calls=%u retry_hits=%u retry_skipped_streak=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236f_retry_skipped_streak));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_checks) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236G] hint_checks=%u hint_positive=%u hint_empty_skips=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_checks),
                HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_positive),
                HZ3_DTOR_STAT_LOAD(g_s203_s236g_hint_empty_skips));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_calls) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_hint) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_backlog) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236I] second_inbox_calls=%u second_inbox_hits=%u "
                "skip_no_hint=%u skip_no_backlog=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_hint),
                HZ3_DTOR_STAT_LOAD(g_s203_s236i_second_inbox_skip_no_backlog));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_calls) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236N] bulk_calls=%u inbox_extra_objs=%u "
                "central_extra_objs=%u zero_extra=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_calls),
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_inbox_extra_objs),
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_central_extra_objs),
                HZ3_DTOR_STAT_LOAD(g_s203_s236n_bulk_zero_extra));
    }
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t from_inbox =
            atomic_load_explicit(&g_s203_alloc_slow_from_inbox_sc[sc], memory_order_relaxed);
        uint64_t from_central =
            atomic_load_explicit(&g_s203_alloc_slow_from_central_sc[sc], memory_order_relaxed);
        uint64_t from_segment =
            atomic_load_explicit(&g_s203_alloc_slow_from_segment_sc[sc], memory_order_relaxed);
        if (from_inbox == 0 && from_central == 0 && from_segment == 0) {
            continue;
        }
        size_t sz = hz3_sc_to_size((int)sc);
        int band = 3;
        if (sz <= (32u << 10)) {
            band = 0;
        } else if (sz <= (64u << 10)) {
            band = 1;
        } else if (sz <= (128u << 10)) {
            band = 2;
        }
        inbox_band[band] += from_inbox;
        central_band[band] += from_central;
        segment_band[band] += from_segment;
        fprintf(stderr,
                "[HZ3_S203_ALLOC_SC] sc=%u from_inbox=%llu from_central=%llu from_segment=%llu\n",
                sc,
                (unsigned long long)from_inbox,
                (unsigned long long)from_central,
                (unsigned long long)from_segment);
    }
    for (int b = 0; b < 4; b++) {
        if (inbox_band[b] == 0 && central_band[b] == 0 && segment_band[b] == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S203_ALLOC_BAND] band=%s from_inbox=%llu from_central=%llu from_segment=%llu\n",
                band_name[b],
                (unsigned long long)inbox_band[b],
                (unsigned long long)central_band[b],
                (unsigned long long)segment_band[b]);
    }
}
#define S203_ALLOC_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s203_alloc, hz3_s203_alloc_atexit_dump)
#define S203_ALLOC_INC(name) (t_s203_##name++)
#define S203_ALLOC_ADD(name, value) (t_s203_##name += (uint64_t)(value))
#define S203_ALLOC_INC_SC(src, sc) (t_s203_alloc_slow_from_##src##_sc[(sc)]++)
// Hook for thread dtor (need to find where hz3_tcache_fini is defined/used)
#else
#define S203_ALLOC_REGISTER() ((void)0)
#define S203_ALLOC_INC(name) ((void)0)
#define S203_ALLOC_ADD(name, value) ((void)0)
#define S203_ALLOC_INC_SC(src, sc) ((void)0)
void hz3_s203_flush_tls(void) {}
#endif

// ============================================================================
// S204: Larson Diagnostic Instrumentation
// ============================================================================
#if HZ3_S204_LARSON_DIAG
// Flag to indicate thread is in destructor (for split push stats)
_Thread_local bool t_s204_in_dtor = false;

HZ3_DTOR_STATS_BEGIN(S204_ALLOC);
static _Thread_local uint64_t t_s204_alloc_seen_nonempty_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s204_alloc_seen_nonempty_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s204_alloc_seen_nonempty_shard_sc[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Atomic uint64_t g_s204_alloc_seen_nonempty_shard_sc[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static void hz3_s204_flush_tls(void) {
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s204_alloc_seen_nonempty_sc[sc],
                                  t_s204_alloc_seen_nonempty_sc[sc],
                                  memory_order_relaxed);
    }
    for (uint32_t shard = 0; shard < (uint32_t)HZ3_NUM_SHARDS; shard++) {
        for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
            atomic_fetch_add_explicit(&g_s204_alloc_seen_nonempty_shard_sc[shard][sc],
                                      t_s204_alloc_seen_nonempty_shard_sc[shard][sc],
                                      memory_order_relaxed);
        }
    }
}


HZ3_DTOR_ATEXIT_FLAG(g_s204_alloc);

static void hz3_s204_atexit_dump(void) {
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t seen =
            atomic_load_explicit(&g_s204_alloc_seen_nonempty_sc[sc], memory_order_relaxed);
        if (seen > 0) {
            fprintf(stderr, "[HZ3_S204_ALLOC] sc=%u seen_nonempty=%llu\n",
                    sc, (unsigned long long)seen);
        }
    }
    for (uint32_t shard = 0; shard < (uint32_t)HZ3_NUM_SHARDS; shard++) {
        uint64_t shard_total = 0;
        for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
            uint64_t seen =
                atomic_load_explicit(&g_s204_alloc_seen_nonempty_shard_sc[shard][sc],
                                     memory_order_relaxed);
            if (seen > 0) {
                fprintf(stderr,
                        "[HZ3_S204_ALLOC_SHARD_SC] shard=%u sc=%u seen_nonempty=%llu\n",
                        shard, sc, (unsigned long long)seen);
                shard_total += seen;
            }
        }
        if (shard_total > 0) {
            fprintf(stderr, "[HZ3_S204_ALLOC_SHARD] shard=%u seen_nonempty=%llu\n",
                    shard, (unsigned long long)shard_total);
        }
    }
}
#define S204_ALLOC_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s204_alloc, hz3_s204_atexit_dump)
#define S204_ALLOC_INC_SEEN(sc) (t_s204_alloc_seen_nonempty_sc[(sc)]++)
#define S204_ALLOC_INC_SHARD_SC(shard, sc) do {                     \
    uint32_t s204_shard = (uint32_t)(shard);                        \
    uint32_t s204_sc = (uint32_t)(sc);                              \
    if (s204_shard < (uint32_t)HZ3_NUM_SHARDS &&                    \
        s204_sc < (uint32_t)HZ3_NUM_SC) {                           \
        t_s204_alloc_seen_nonempty_shard_sc[s204_shard][s204_sc]++; \
    }                                                                \
} while (0)
#else
#define S204_ALLOC_REGISTER() ((void)0)
#define S204_ALLOC_INC_SEEN(sc) ((void)0)
#define S204_ALLOC_INC_SHARD_SC(shard, sc) ((void)0)
static inline void hz3_s204_flush_tls(void) {}
#endif

// ============================================================================
// S40: Static assertions for SoA + pow2 padding
// ============================================================================
#if HZ3_BIN_PAD_LOG2
_Static_assert((HZ3_BIN_PAD & (HZ3_BIN_PAD - 1)) == 0, "HZ3_BIN_PAD must be power of 2");
_Static_assert(HZ3_BIN_TOTAL <= HZ3_BIN_PAD, "HZ3_BIN_TOTAL must fit in HZ3_BIN_PAD");
#endif

// ============================================================================
// TLS and Global Variables (shared via hz3_tcache_internal.h)
// ============================================================================

// Thread-local cache (zero-initialized by TLS semantics)
HZ3_TLS Hz3TCache t_hz3_cache;
#if HZ3_S200_INBOX_SEQ_GATE
HZ3_TLS uint32_t t_hz3_s200_inbox_seq_seen[HZ3_NUM_SC];
#endif

// Day 4: Shard assignment
_Atomic uint32_t g_shard_counter = 0;

// Detect shard collisions (multiple threads concurrently assigned same shard id).
static _Atomic int g_shard_collision_detected = 0;
static _Atomic uint32_t g_shard_live_count[HZ3_NUM_SHARDS] = {0};
#if HZ3_SHARD_COLLISION_SHOT
_Atomic int g_shard_collision_shot_fired = 0;
#endif

// Day 5: Thread exit cleanup via pthread_key
pthread_key_t g_hz3_tcache_key;
pthread_once_t g_hz3_tcache_key_once = PTHREAD_ONCE_INIT;

#if HZ3_OOM_SHOT
// OOM triage: defined in hz3_tcache_stats.c, declared extern here
extern _Atomic uint64_t g_medium_page_alloc_sc[HZ3_NUM_SC];
#endif

#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
pthread_once_t g_scavenge_obs_atexit_once = PTHREAD_ONCE_INIT;
#endif

#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
pthread_once_t g_retention_obs_atexit_once = PTHREAD_ONCE_INIT;
#endif

#if HZ3_S62_ATEXIT_GATE
// S62-1A: AtExitGate - File-scope guards (follow existing atexit pattern)
pthread_once_t g_s62_atexit_once = PTHREAD_ONCE_INIT;
static _Atomic int g_s62_atexit_executed = 0;
#endif

#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
_Atomic uint64_t g_s56_active_choose_alt = 0;
#endif

#if HZ3_LANE_SPLIT
static inline Hz3Lane* hz3_lane_alloc(void) {
    void* mem = mmap(NULL, HZ3_PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    return (Hz3Lane*)mem;
}

static inline void hz3_lane_free(Hz3Lane* lane) {
    if (!lane) {
        return;
    }
    munmap(lane, HZ3_PAGE_SIZE);
}
#endif

// ============================================================================
// S62-1G: SingleThreadRetireGate - Check if total live threads == 1
// ============================================================================

#if HZ3_S62_SINGLE_THREAD_GATE
static inline int hz3_s62_single_thread_ok(void) {
    uint32_t total = 0;
    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        total += hz3_shard_live_count((uint8_t)i);
    }

    // Defense: total==0 should never happen (executing thread hasn't decremented yet).
    // Treat as unsafe (assume multi-thread) to avoid unexpected S62 execution.
    if (total == 0) {
#if HZ3_S62_SINGLE_THREAD_FAILFAST
        fprintf(stderr, "[HZ3_S62_SINGLE_THREAD_FAILFAST] total_live==0 during destructor (impossible)\n");
        abort();
#endif
        return 0;
    }

    return (total <= 1);
}
#endif  // HZ3_S62_SINGLE_THREAD_GATE

#if HZ3_S62_ATEXIT_GATE
// ============================================================================
// S62-1A: AtExitGate - One-shot atexit handler
// ============================================================================
static void hz3_s62_atexit_handler(void) {
    // One-shot guard: prevent double execution
    int expected = 0;
    if (!atomic_compare_exchange_strong(&g_s62_atexit_executed, &expected, 1)) {
        return;
    }

    // Uninitialized guard: skip if tcache never initialized
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S62_ATEXIT_LOG
    fprintf(stderr, "[HZ3_S62_ATEXIT] S62 cleanup at process exit\n");
#endif

    // Safety check: verify single-thread context
#if HZ3_S62_SINGLE_THREAD_GATE
    int single = hz3_s62_single_thread_ok();
#if HZ3_S62_SINGLE_THREAD_FAILFAST
    if (!single) {
        abort();  // Multi-thread detected: failfast
    }
#endif
    if (!single) {
        return;  // Multi-thread detected: skip S62
    }
#endif

    // Execute S62 sequence: retire → purge
#if HZ3_S62_RETIRE && !HZ3_S62_RETIRE_DISABLE
    hz3_s62_retire();
#endif
#if HZ3_S62_SUB4K_RETIRE && !HZ3_S62_SUB4K_RETIRE_DISABLE
    hz3_s62_sub4k_retire();
#endif
#if HZ3_S62_PURGE && !HZ3_S62_PURGE_DISABLE
    hz3_s62_purge();
#endif
}

static void hz3_s62_atexit_register(void) {
    atexit(hz3_s62_atexit_handler);
}
#endif  // HZ3_S62_ATEXIT_GATE

// ============================================================================
// TLS Destructor
// ============================================================================

static void hz3_tcache_destructor(void* arg) {
    if (!arg) return;
    if (!t_hz3_cache.initialized) return;

#if HZ3_S204_LARSON_DIAG
    t_s204_in_dtor = true;
#endif

#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S12-3C triage: Aggregate thread stats before cleanup
    hz3_s12_v2_stats_aggregate_thread();
#endif
    // Day 6: Force epoch cleanup first
    hz3_epoch_force();

#if HZ3_S203_COUNTERS
    // S203: Flush TLS stats to global aggregators
    hz3_s203_flush_tls();
#endif
#if HZ3_S220_CPU_RRQ && HZ3_S220_CPU_RRQ_STATS
    hz3_s220_rrq_flush_tls();
#endif

// Remote path detection for S65/S62 guard
#ifndef HZ3_S42_SMALL_XFER_DISABLE
#define HZ3_S42_SMALL_XFER_DISABLE 0
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif
#ifndef HZ3_REMOTE_ENABLED
#define HZ3_REMOTE_ENABLED \
    ((HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE) || \
     (HZ3_S44_OWNER_STASH && !HZ3_S44_OWNER_STASH_DISABLE))
#endif

// S65 Remote Guard: Skip S65 ledger flush when remote paths are active
#ifndef HZ3_S65_REMOTE_GUARD
#define HZ3_S65_REMOTE_GUARD 0
#endif
#if !(HZ3_S65_REMOTE_GUARD && HZ3_REMOTE_ENABLED)
#if HZ3_S65_RELEASE_LEDGER
    // S65: Flush TLS ledger (drain all pending entries)
    hz3_s65_ledger_flush_all();
#endif
#endif  // S65_REMOTE_GUARD

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24-1: Full flush on thread exit (event-only)
    hz3_dstbin_flush_remote_all();
#endif

    // S121-D: Flush page packets before flushing other remote state
    hz3_s121d_packet_flush_all_extern();

    // S121-M: Flush pending pageq batch before thread exit
    hz3_s121m_flush_pending_extern();

    // 1. Flush all outboxes (redundant after epoch, but safe)
    for (int owner = 0; owner < HZ3_NUM_SHARDS; owner++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            hz3_outbox_flush((uint8_t)owner, sc);
        }
    }

    // 2. Return bins to central (batch)
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        uint32_t bin_idx = hz3_bin_index_medium(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            // Build list: find tail
            void* tail = head;
            uint32_t n = 1;
            void* obj = hz3_obj_get_next(head);
            while (obj) {
                tail = obj;
                n++;
                obj = hz3_obj_get_next(obj);
            }
            hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            hz3_local_head_clear(bin_idx);
        }
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        if (!hz3_bin_is_empty(bin)) {
            // Build list: find tail
            void* head = hz3_bin_take_all(bin);
            void* tail = head;
            uint32_t n = 1;
            void* obj = hz3_obj_get_next(head);
            while (obj) {
                tail = obj;
                n++;
                obj = hz3_obj_get_next(obj);
            }
            hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
        }
#endif
    }

    // 3. Return small bins to central
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // S40: SoA - directly access local_head for small bins
        uint32_t bin_idx = hz3_bin_index_small(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
#if HZ3_SMALL_V2_ENABLE
            hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
#endif
            hz3_local_head_clear(bin_idx);
        }
#else  // !HZ3_TCACHE_SOA_LOCAL
#if HZ3_SMALL_V2_ENABLE
        hz3_small_v2_bin_flush(sc, hz3_tcache_get_small_bin(sc));
#elif HZ3_SMALL_V1_ENABLE
        hz3_small_bin_flush(sc, hz3_tcache_get_small_bin(sc));
#endif
#endif  // HZ3_TCACHE_SOA_LOCAL
    }

#if HZ3_S145_CENTRAL_LOCAL_CACHE
    // S145-A: Flush TLS central cache back to central pool
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        if (t_hz3_cache.s145_cache_count[sc] > 0) {
            void* head = t_hz3_cache.s145_cache_head[sc];
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            t_hz3_cache.s145_cache_head[sc] = NULL;
            t_hz3_cache.s145_cache_count[sc] = 0;
        }
    }
#endif  // HZ3_S145_CENTRAL_LOCAL_CACHE

#if HZ3_SUB4K_ENABLE
    // 4. Return sub4k bins to central
    for (int sc = 0; sc < HZ3_SUB4K_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // S40: SoA - directly access local_head for sub4k bins
        uint32_t bin_idx = hz3_bin_index_sub4k(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_sub4k_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            hz3_local_head_clear(bin_idx);
        }
#else
        hz3_sub4k_bin_flush(sc, hz3_tcache_get_sub4k_bin(sc));
#endif
    }
#endif

    // =========================================================================
    // S62 Remote Guard: Skip S62 when remote paths (xfer/stash) are enabled
    // =========================================================================
    // Compile-time check: if remote paths are active, S62 may race with
    // objects still in transit, causing memory corruption.
    // Note: HZ3_REMOTE_ENABLED defined earlier in this file (before S65 guard)
    //
    // S62-1G: SingleThreadRetireGate overrides REMOTE_GUARD when total_live==1.
    // This allows RSS reduction in safe single-thread scenarios while maintaining
    // safety in multi-thread environments.

    int s62_allowed = 1;

#if HZ3_S62_REMOTE_GUARD && HZ3_REMOTE_ENABLED
    s62_allowed = 0;  // Remote guard active: block S62 by default
#endif

#if HZ3_S62_SINGLE_THREAD_GATE
    int single = hz3_s62_single_thread_ok();  // total_live <= 1
    if (!s62_allowed && single) {
        s62_allowed = 1;  // single-threadなら remote guard を解除
    }
#if HZ3_S62_SINGLE_THREAD_FAILFAST
    if (!single) {
        uint32_t total = 0;
        for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
            total += hz3_shard_live_count((uint8_t)i);
        }
        fprintf(stderr, "[HZ3_S62_SINGLE_THREAD_FAILFAST] Multi-thread detected (total=%u)\n", total);
        abort();  // multi-thread検出時のみ failfast
    }
#endif
#endif

    if (s62_allowed) {
#if HZ3_S62_RETIRE && !HZ3_S62_RETIRE_DISABLE
        // S62-1: PageRetireBox - retire fully-empty small pages to free_bits
        hz3_s62_retire();
#endif

#if HZ3_S62_SUB4K_RETIRE && !HZ3_S62_SUB4K_RETIRE_DISABLE
        // S62-1b: Sub4kRunRetireBox - retire fully-empty sub4k 2-page runs to free_bits
        hz3_s62_sub4k_retire();
#endif

#if HZ3_S62_PURGE && !HZ3_S62_PURGE_DISABLE
        // S62-2: DtorSmallPagePurgeBox - purge retired small pages
        hz3_s62_purge();
#endif
    }

#if HZ3_S62_OBSERVE && !HZ3_S62_OBSERVE_DISABLE
    // S62-0: OBSERVE - count free_bits purge potential (stats only, no madvise)
    hz3_s62_observe();
#endif

#if HZ3_S61_DTOR_HARD_PURGE
    // S61: Hard purge of central pool (thread-exit, cold path only)
    hz3_s61_dtor_hard_purge();
#endif

    // Note: segment cleanup is deferred to process exit

#if HZ3_LANE_SPLIT
    if (t_hz3_cache.lane) {
        Hz3ShardCore* core = &g_hz3_shards[t_hz3_cache.my_shard];
        if (t_hz3_cache.lane != &core->lane0) {
            hz3_lane_free(t_hz3_cache.lane);
        }
        t_hz3_cache.lane = NULL;
    }
#endif

    // Shard collision tracking: decrement only after flushing all TLS state.
    // If we decrement earlier, S65 reclaim in another thread could release a run
    // while this thread still holds stale pointers in bins.
    {
        uint8_t shard = t_hz3_cache.my_shard;
        if (shard < HZ3_NUM_SHARDS) {
            (void)atomic_fetch_sub_explicit(&g_shard_live_count[shard], 1, memory_order_acq_rel);
        }
    }
}

static void hz3_tcache_create_key(void) {
    pthread_key_create(&g_hz3_tcache_key, hz3_tcache_destructor);
}

// ============================================================================
// TLS Initialization (slow path)
// ============================================================================

void hz3_tcache_ensure_init_slow(void) {
    if (t_hz3_cache.initialized) {
        return;
    }

    // Day 5: Register pthread_key destructor (once per process)
    pthread_once(&g_hz3_tcache_key_once, hz3_tcache_create_key);
    pthread_setspecific(g_hz3_tcache_key, &t_hz3_cache);

#if HZ3_STATS_DUMP && !HZ3_SHIM_FORWARD_ONLY
    // S15: Register atexit stats dump (once per process)
    pthread_once(&g_stats_dump_atexit_once, hz3_stats_register_atexit);
#endif
#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S12-3C triage: Register atexit stats dump (once per process)
    pthread_once(&g_s12_v2_stats_atexit_once, hz3_s12_v2_stats_register_atexit);
#endif
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S187: Register medium-path triage stats dump (once per process)
    pthread_once(&g_medium_path_stats_atexit_once, hz3_medium_path_stats_register_atexit);
#endif
#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S54: Register atexit scavenge observation dump (once per process)
    pthread_once(&g_scavenge_obs_atexit_once, hz3_seg_scavenge_obs_register_atexit);
#endif
#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S55: Register atexit retention observation dump (once per process)
    pthread_once(&g_retention_obs_atexit_once, hz3_retention_obs_register_atexit);
#endif
#if HZ3_S62_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S62-0: Register atexit observation dump (once per process).
    // Best-effort snapshot will run at exit even if thread destructors don't.
    hz3_s62_observe_register_atexit();
#endif
#if HZ3_S62_ATEXIT_GATE && !HZ3_SHIM_FORWARD_ONLY
    // S62-1A: Register atexit S62 cleanup (once per process).
    // Ensures S62 runs at process exit even if thread destructors don't fire.
    pthread_once(&g_s62_atexit_once, hz3_s62_atexit_register);
#endif
#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S56-2: Register atexit active-set stats dump (once per process)
    hz3_s56_active_set_register_atexit();
#endif
#if HZ3_S220_CPU_RRQ && HZ3_S220_CPU_RRQ_STATS
    // S220: Register RRQ counters dump (once per process)
    hz3_s220_rrq_register_once();
#endif
    // 1. Shard assignment FIRST (before any segment creation)
    //
    // Prefer an exclusive shard (live_count==0) to avoid collisions when threads<=shards.
    // If no free shard exists, we must collide (threads > shards).
    uint32_t start = atomic_fetch_add_explicit(&g_shard_counter, 1, memory_order_relaxed);
    uint8_t shard = 0;
    uint32_t prev_live = 0;
    int claimed_exclusive = 0;

    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        shard = (uint8_t)((start + i) % HZ3_NUM_SHARDS);
        uint32_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(&g_shard_live_count[shard], &expected, 1,
                memory_order_acq_rel, memory_order_acquire)) {
            claimed_exclusive = 1;
            prev_live = 0;
            break;
        }
    }

    if (!claimed_exclusive) {
        shard = (uint8_t)(start % HZ3_NUM_SHARDS);
        prev_live = atomic_fetch_add_explicit(&g_shard_live_count[shard], 1, memory_order_acq_rel);
    }

    t_hz3_cache.my_shard = shard;
#if HZ3_TLS_INIT_LOG || HZ3_TLS_INIT_FAILFAST
    {
        static _Atomic(uint32_t) g_tls_init_count = 0;
        static _Atomic(uintptr_t) g_tls_addr_first = 0;
        uint32_t idx = atomic_fetch_add_explicit(&g_tls_init_count, 1, memory_order_relaxed);
        uintptr_t addr = (uintptr_t)&t_hz3_cache;
        if (idx == 0) {
            atomic_store_explicit(&g_tls_addr_first, addr, memory_order_relaxed);
        }
        uintptr_t first = atomic_load_explicit(&g_tls_addr_first, memory_order_relaxed);
        int same_first = (first == addr);
#if HZ3_TLS_INIT_LOG
        fprintf(stderr,
                "[HZ3_TLS_INIT] tid=%lu idx=%u cache=%p shard=%u prev_live=%u start=%u same_first=%d\n",
                hz3_thread_id(), (unsigned)idx, (void*)addr, (unsigned)shard,
                (unsigned)prev_live, (unsigned)start, same_first);
#endif
#if HZ3_TLS_INIT_FAILFAST
        if (idx > 0 && same_first) {
            fprintf(stderr,
                    "[HZ3_TLS_INIT_FAILFAST] shared_tls cache=%p tid=%lu idx=%u\n",
                    (void*)addr, hz3_thread_id(), (unsigned)idx);
            abort();
        }
#endif
    }
#endif
#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Initialize instance selection fields (use shard_counter as unique seed)
    t_hz3_cache.owner_stash_seed = start;  // unique per thread
    t_hz3_cache.owner_stash_rr = 0;        // round-robin cursor starts at 0
#endif
#if HZ3_S121_F_PAGEQ_SHARD
    // S121-F: Hash thread ID for sub-shard selection (use shard_counter as unique seed)
    t_hz3_cache.my_tid_hash = start;
#endif
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = NULL;
    t_hz3_cache.small_current_seg_base = NULL;

    Hz3ShardCore* core = &g_hz3_shards[shard];
    core->lane0.core = core;
    if (prev_live == 0) {
        t_hz3_cache.lane = &core->lane0;
    } else {
        Hz3Lane* lane = hz3_lane_alloc();
        if (lane) {
            lane->core = core;
            lane->placeholder = 0;
            t_hz3_cache.lane = lane;
        } else {
            t_hz3_cache.lane = &core->lane0;
        }
    }
#endif

    // Precise (concurrent) collision detection:
    // - collision means >1 thread concurrently shares the same shard.
    if (prev_live != 0) {
        atomic_store_explicit(&g_shard_collision_detected, 1, memory_order_release);
#if HZ3_SHARD_COLLISION_FAILFAST
        fprintf(stderr, "[HZ3_SHARD_COLLISION_FAILFAST] shards=%u hit_shard=%u counter=%u\n",
            (unsigned)HZ3_NUM_SHARDS, (unsigned)t_hz3_cache.my_shard,
            (unsigned)atomic_load_explicit(&g_shard_counter, memory_order_relaxed));
        abort();
#elif HZ3_SHARD_COLLISION_SHOT
        if (atomic_exchange_explicit(&g_shard_collision_shot_fired, 1, memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_SHARD_COLLISION] shards=%u (threads>shards) example_shard=%u counter=%u\n",
                (unsigned)HZ3_NUM_SHARDS, (unsigned)t_hz3_cache.my_shard,
                (unsigned)atomic_load_explicit(&g_shard_counter, memory_order_relaxed));
        }
#endif
    }

#if HZ3_OWNER_LEASE_STATS
    hz3_owner_lease_stats_register_atexit();
#endif

    // 2. Initialize central pool (thread-safe via pthread_once)
    hz3_central_init();

    // 3. Initialize inbox pool (thread-safe via pthread_once)
    hz3_inbox_init();
#if HZ3_S203_COUNTERS
    // S203: register one-shot atexit dumps out of hot path.
    S203_ALLOC_REGISTER();
    hz3_s203_remote_register_once();
#endif
    S204_ALLOC_REGISTER();

    // 4. Initialize bins and outbox
    // S40-2: LOCAL and BANK can be independently configured
#if HZ3_TCACHE_SOA_LOCAL
    // S40: SoA layout for local bins
    memset(t_hz3_cache.local_head, 0, sizeof(t_hz3_cache.local_head));
    memset(t_hz3_cache.local_count, 0, sizeof(t_hz3_cache.local_count));
#elif HZ3_LOCAL_BINS_SPLIT
    // S33: Unified local_bins (AoS)
    memset(t_hz3_cache.local_bins, 0, sizeof(t_hz3_cache.local_bins));
#else
    // Legacy: separate bins + small_bins (AoS)
    memset(t_hz3_cache.bins, 0, sizeof(t_hz3_cache.bins));
    memset(t_hz3_cache.small_bins, 0, sizeof(t_hz3_cache.small_bins));
#endif

    // === Bank bins (remote free) ===
#if HZ3_REMOTE_STASH_SPARSE
    // S41: Sparse ring initialization
    memset(&t_hz3_cache.remote_stash, 0, sizeof(t_hz3_cache.remote_stash));
#else
#if HZ3_PTAG_DSTBIN_ENABLE
#if HZ3_TCACHE_SOA_BANK
    // S40: SoA layout for bank bins
    memset(t_hz3_cache.bank_head, 0, sizeof(t_hz3_cache.bank_head));
    memset(t_hz3_cache.bank_count, 0, sizeof(t_hz3_cache.bank_count));
#else
    // AoS layout for bank bins
    memset(t_hz3_cache.bank, 0, sizeof(t_hz3_cache.bank));
#endif
#endif
#endif

#if !HZ3_REMOTE_STASH_SPARSE
    memset(t_hz3_cache.outbox, 0, sizeof(t_hz3_cache.outbox));
#endif

#if HZ3_S67_SPILL_ARRAY2
    // S67-2: Initialize spill_count and spill_overflow
    memset(t_hz3_cache.spill_count, 0, sizeof(t_hz3_cache.spill_count));
    memset(t_hz3_cache.spill_overflow, 0, sizeof(t_hz3_cache.spill_overflow));
#elif HZ3_S67_SPILL_ARRAY
    // S67: Initialize spill_count to 0 (spill_array is lazy, count=0 means empty)
    memset(t_hz3_cache.spill_count, 0, sizeof(t_hz3_cache.spill_count));
#elif HZ3_S48_OWNER_STASH_SPILL
    // S48: Initialize stash_spill to NULL
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        t_hz3_cache.stash_spill[sc] = NULL;
    }
#endif
#if HZ3_S94_SPILL_LITE
    // S94: Initialize spill_count and spill_overflow to zero
    // s94_spill_array is lazy (count=0 means empty, no read)
    memset(t_hz3_cache.s94_spill_count, 0, sizeof(t_hz3_cache.s94_spill_count));
    memset(t_hz3_cache.s94_spill_overflow, 0, sizeof(t_hz3_cache.s94_spill_overflow));
#endif
    // S121-E: owner_pages is now GLOBAL (g_owner_pages), not TLS
    // No TLS initialization needed; g_owner_pages is static zero-initialized
#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_PTAG_DSTBIN_TLS
    t_hz3_cache.arena_base =
        atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    t_hz3_cache.page_tag32 = g_hz3_page_tag32;
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
    t_hz3_cache.remote_dst_cursor = 0;
    t_hz3_cache.remote_bin_cursor = 0;
    t_hz3_cache.remote_hint = 0;
    t_hz3_cache.remote_flush_credit = 0;
#endif

#if HZ3_S44_OWNER_STASH
    // S162: Initialize owner stash once here to avoid hot-path init calls.
    hz3_owner_stash_init();
#endif

    // 5. No current segment yet
    t_hz3_cache.current_seg = NULL;
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = NULL;
#endif
#if HZ3_S56_ACTIVE_SET
    t_hz3_cache.active_seg1 = NULL;
#endif
    t_hz3_cache.small_current_seg = NULL;
#if HZ3_LANE_SPLIT
    t_hz3_cache.small_current_seg_base = NULL;
#endif
#if HZ3_LANE_T16_R90_PAGE_REMOTE
    t_hz3_cache.t16_small_seg_count = 0;
    t_hz3_cache.t16_small_seg_cursor = 0;
    memset(t_hz3_cache.t16_small_seg_ids, 0, sizeof(t_hz3_cache.t16_small_seg_ids));
#endif

    // 6. Day 6: Initialize knobs (thread-safe via pthread_once)
    hz3_knobs_init();

    // 7. Day 6: Copy global knobs to TLS snapshot
    t_hz3_cache.knobs = g_hz3_knobs;
    t_hz3_cache.knobs_ver = atomic_load_explicit(&g_hz3_knobs_ver, memory_order_acquire);

    // 8. Day 6: Initialize stats to zero
    memset(&t_hz3_cache.stats, 0, sizeof(t_hz3_cache.stats));

#if HZ3_S58_TCACHE_DECAY
    // 9. S58: Initialize decay tracking fields
    hz3_s58_init();
#if HZ3_S58_STATS
    hz3_s58_stats_register_atexit();
#endif
#endif

    t_hz3_cache.initialized = 1;
}

int hz3_shard_collision_detected(void) {
    return atomic_load_explicit(&g_shard_collision_detected, memory_order_acquire) != 0;
}

uint32_t hz3_shard_live_count(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) {
        return 0;
    }
    return atomic_load_explicit(&g_shard_live_count[shard], memory_order_acquire);
}

#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG || HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
static _Atomic uint32_t g_s209_medium_miss_seq[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#endif

uint32_t hz3_s209_medium_miss_seq_load(uint8_t shard, int sc) {
#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG || HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    if ((uint32_t)shard >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    return atomic_load_explicit(&g_s209_medium_miss_seq[shard][sc], memory_order_relaxed);
#else
    (void)shard;
    (void)sc;
    return 0;
#endif
}

void hz3_s209_medium_miss_seq_note(uint8_t shard, int sc, uint32_t miss_count) {
#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG || HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    if ((uint32_t)shard >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return;
    }
    if (miss_count == 0) {
        return;
    }

#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG && HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    int in_s209_range = (sc >= HZ3_S209_SC_MIN && sc <= HZ3_S209_SC_MAX);
    int in_s210_range = (sc >= HZ3_S210_SC_MIN && sc <= HZ3_S210_SC_MAX);
    if (!in_s209_range && !in_s210_range) {
        return;
    }
    int stride_hit = 0;
    if (in_s209_range &&
        (miss_count % (uint32_t)HZ3_S209_MISS_STRIDE) == 0u) {
        stride_hit = 1;
    }
    if (!stride_hit && in_s210_range &&
        (miss_count % (uint32_t)HZ3_S210_MISS_STRIDE) == 0u) {
        stride_hit = 1;
    }
    if (!stride_hit) {
        return;
    }
#elif HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG
    if (sc < HZ3_S209_SC_MIN || sc > HZ3_S209_SC_MAX) {
        return;
    }
    if ((miss_count % (uint32_t)HZ3_S209_MISS_STRIDE) != 0u) {
        return;
    }
#elif HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    if (sc < HZ3_S210_SC_MIN || sc > HZ3_S210_SC_MAX) {
        return;
    }
    if ((miss_count % (uint32_t)HZ3_S210_MISS_STRIDE) != 0u) {
        return;
    }
#endif

    atomic_fetch_add_explicit(&g_s209_medium_miss_seq[shard][sc], 1, memory_order_relaxed);
#else
    (void)shard;
    (void)sc;
    (void)miss_count;
#endif
}

#include "hz3_tcache_slowpath.inc"
