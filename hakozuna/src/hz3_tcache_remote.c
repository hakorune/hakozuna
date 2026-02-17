// hz3_tcache_remote.c - Outbox and Remote Stash operations
// Extracted from hz3_tcache.c for modularization
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_inbox.h"
#include "hz3_owner_stash.h"
#include "hz3_stash_guard.h"
#include "hz3_dtor_stats.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HZ3_LIST_FAILFAST
#define HZ3_LIST_FAILFAST 0
#endif

#ifndef HZ3_CENTRAL_DEBUG
#define HZ3_CENTRAL_DEBUG 0
#endif

#ifndef HZ3_XFER_DEBUG
#define HZ3_XFER_DEBUG 0
#endif

#if HZ3_REMOTE_STASH_DUP_FAILFAST
enum { HZ3_REMOTE_STASH_DUP_TABLE_SIZE = 2048 };
_Static_assert((HZ3_REMOTE_STASH_DUP_TABLE_SIZE & (HZ3_REMOTE_STASH_DUP_TABLE_SIZE - 1)) == 0,
               "HZ3_REMOTE_STASH_DUP_TABLE_SIZE must be power of 2");

static inline void hz3_remote_stash_dup_init(uintptr_t* table) {
    memset(table, 0, sizeof(uintptr_t) * (size_t)HZ3_REMOTE_STASH_DUP_TABLE_SIZE);
}

static inline void hz3_remote_stash_dup_check(uintptr_t* table, void* ptr,
                                              uint8_t dst, uint32_t bin,
                                              const char* where) {
    uintptr_t key = (uintptr_t)ptr;
    if (!key) {
        return;
    }
    uint32_t mask = (uint32_t)HZ3_REMOTE_STASH_DUP_TABLE_SIZE - 1u;
    uint32_t idx = (uint32_t)(((key >> 4) * 2654435761u) & mask);
    for (uint32_t probe = 0; probe < (uint32_t)HZ3_REMOTE_STASH_DUP_TABLE_SIZE; probe++) {
        uintptr_t cur = table[idx];
        if (cur == 0) {
            table[idx] = key;
            return;
        }
        if (cur == key) {
            fprintf(stderr,
                    "[HZ3_REMOTE_STASH_DUP_FAILFAST] where=%s dst=%u bin=%u ptr=%p\n",
                    (where ? where : "?"), (unsigned)dst, (unsigned)bin, ptr);
            abort();
        }
        idx = (idx + 1u) & mask;
    }
    fprintf(stderr, "[HZ3_REMOTE_STASH_DUP_FAILFAST] table_full where=%s\n",
            (where ? where : "?"));
    abort();
}
#else
#define hz3_remote_stash_dup_init(table) ((void)0)
#define hz3_remote_stash_dup_check(table, ptr, dst, bin, where) ((void)0)
#endif

#if HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL && (HZ3_LIST_FAILFAST || HZ3_CENTRAL_DEBUG || HZ3_XFER_DEBUG)
#error "HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL requires list debug checks OFF (HZ3_LIST_FAILFAST/HZ3_CENTRAL_DEBUG/HZ3_XFER_DEBUG)"
#endif

static inline void hz3_stat_update_max_u32(_Atomic(uint32_t)* stat, uint32_t val) {
    uint32_t cur = atomic_load_explicit(stat, memory_order_relaxed);
    while (val > cur) {
        if (atomic_compare_exchange_weak_explicit(
                stat, &cur, val, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

// =============================================================================
// S97: RemoteStashFlush SSOT (atexit one-shot)
// =============================================================================
#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
HZ3_DTOR_STATS_BEGIN(S97);
HZ3_DTOR_STAT(g_s97_flush_budget_calls);
HZ3_DTOR_STAT(g_s97_flush_budget_entries_total);
HZ3_DTOR_STAT(g_s97_flush_budget_groups_total);
HZ3_DTOR_STAT(g_s97_flush_budget_distinct_keys_total);
HZ3_DTOR_STAT(g_s97_flush_budget_potential_merge_calls_total);
HZ3_DTOR_STAT(g_s97_flush_budget_saved_calls_total);
HZ3_DTOR_STAT(g_s97_flush_budget_n_max);
HZ3_DTOR_STAT(g_s97_flush_budget_n_gt1_calls_total);
HZ3_DTOR_STAT(g_s97_flush_budget_n_gt1_entries_total);
HZ3_DTOR_STAT(g_s97_flush_budget_small_groups);
HZ3_DTOR_STAT(g_s97_flush_budget_sub4k_groups);
HZ3_DTOR_STAT(g_s97_flush_budget_medium_groups);
HZ3_DTOR_STAT(g_s97_flush_budget_selfdst_groups);
HZ3_DTOR_ATEXIT_FLAG(g_s97);

static inline int hz3_s97_seen_insert(uint16_t* keys, uint16_t key) {
    // Small fixed-size open addressing set: keys[i]==0xFFFF means empty.
    // Returns 1 if inserted new, 0 if already present.
    const uint32_t mask = 512u - 1u;
    uint32_t idx = ((uint32_t)key * 2654435761u) & mask;
    for (uint32_t probe = 0; probe < 512u; probe++) {
        uint16_t cur = keys[idx];
        if (cur == 0xFFFFu) {
            keys[idx] = key;
            return 1;
        }
        if (cur == key) {
            return 0;
        }
        idx = (idx + 1u) & mask;
    }
    return 0;
}

static void hz3_s97_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_calls);
    uint32_t entries = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_entries_total);
    uint32_t groups = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_groups_total);
    uint32_t distinct = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_distinct_keys_total);
    uint32_t merge_calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_potential_merge_calls_total);
    uint32_t saved_calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_saved_calls_total);
    uint32_t nmax = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_n_max);
    uint32_t ngt1_calls = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_n_gt1_calls_total);
    uint32_t ngt1_entries = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_n_gt1_entries_total);
    uint32_t small_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_small_groups);
    uint32_t sub4k_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_sub4k_groups);
    uint32_t med_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_medium_groups);
    uint32_t selfdst_g = HZ3_DTOR_STAT_LOAD(g_s97_flush_budget_selfdst_groups);

    fprintf(stderr,
            "[HZ3_S97_REMOTE] bucket=%u skip_tail_null=%u shards=%u bin_total=%u max_keys=%u "
            "calls=%u entries=%u groups=%u distinct=%u "
            "potential_merge_calls=%u saved_calls=%u nmax=%u n_gt1_calls=%u n_gt1_entries=%u "
            "small_groups=%u sub4k_groups=%u medium_groups=%u selfdst_groups=%u\n",
            (unsigned)HZ3_S97_REMOTE_STASH_BUCKET,
            (unsigned)HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL,
            (unsigned)HZ3_NUM_SHARDS,
            (unsigned)HZ3_BIN_TOTAL,
            (unsigned)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS,
            calls, entries, groups, distinct, merge_calls,
            saved_calls, nmax, ngt1_calls, ngt1_entries,
            small_g, sub4k_g, med_g, selfdst_g);
}

#define S97_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s97, hz3_s97_atexit_dump)
#define S97_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S97_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S97_STAT_MAX(name, val) hz3_stat_update_max_u32(&(name), (uint32_t)(val))
#else
#define S97_STAT_REGISTER() ((void)0)
#define S97_STAT_INC(name) ((void)0)
#define S97_STAT_ADD(name, val) ((void)0)
#define S97_STAT_MAX(name, val) ((void)0)
#endif  // HZ3_S97_REMOTE_STASH_FLUSH_STATS

// =============================================================================
// S196: RemoteDispatchPathStatsBox (atexit one-shot, observe-only)
// =============================================================================
#if HZ3_S196_REMOTE_DISPATCH_STATS
HZ3_DTOR_STATS_BEGIN(S196);
HZ3_DTOR_STAT(g_s196_dispatch_small_calls);
HZ3_DTOR_STAT(g_s196_dispatch_small_objs);
HZ3_DTOR_STAT(g_s196_dispatch_sub4k_calls);
HZ3_DTOR_STAT(g_s196_dispatch_sub4k_objs);
HZ3_DTOR_STAT(g_s196_dispatch_medium_calls);
HZ3_DTOR_STAT(g_s196_dispatch_medium_objs);
HZ3_DTOR_STAT(g_s196_direct_n1_small_calls);
static _Atomic(uint32_t) g_s196_medium_sc_calls[HZ3_NUM_SC];
static _Atomic(uint32_t) g_s196_medium_sc_objs[HZ3_NUM_SC];
HZ3_DTOR_ATEXIT_FLAG(g_s196);

static void hz3_s196_atexit_dump(void) {
    uint32_t small_calls = HZ3_DTOR_STAT_LOAD(g_s196_dispatch_small_calls);
    uint32_t small_objs = HZ3_DTOR_STAT_LOAD(g_s196_dispatch_small_objs);
    uint32_t sub4k_calls = HZ3_DTOR_STAT_LOAD(g_s196_dispatch_sub4k_calls);
    uint32_t sub4k_objs = HZ3_DTOR_STAT_LOAD(g_s196_dispatch_sub4k_objs);
    uint32_t medium_calls = HZ3_DTOR_STAT_LOAD(g_s196_dispatch_medium_calls);
    uint32_t medium_objs = HZ3_DTOR_STAT_LOAD(g_s196_dispatch_medium_objs);
    uint32_t direct_small = HZ3_DTOR_STAT_LOAD(g_s196_direct_n1_small_calls);

    fprintf(stderr,
            "[HZ3_S196_REMOTE_DISPATCH] small_calls=%u small_objs=%u "
            "sub4k_calls=%u sub4k_objs=%u medium_calls=%u medium_objs=%u "
            "direct_n1_small_calls=%u\n",
            small_calls, small_objs, sub4k_calls, sub4k_objs,
            medium_calls, medium_objs, direct_small);

    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint32_t sc_calls =
            atomic_load_explicit(&g_s196_medium_sc_calls[sc], memory_order_relaxed);
        uint32_t sc_objs =
            atomic_load_explicit(&g_s196_medium_sc_objs[sc], memory_order_relaxed);
        if (sc_calls == 0 && sc_objs == 0) {
            continue;
        }
        fprintf(stderr, "[HZ3_S196_REMOTE_DISPATCH_SC] sc=%u calls=%u objs=%u\n",
                sc, sc_calls, sc_objs);
    }
}

#define S196_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s196, hz3_s196_atexit_dump)
#define S196_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S196_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S196_MEDIUM_SC_ADD(sc, n) do { \
    uint32_t s196_sc = (uint32_t)(sc); \
    if (s196_sc < (uint32_t)HZ3_NUM_SC) { \
        atomic_fetch_add_explicit(&g_s196_medium_sc_calls[s196_sc], 1, memory_order_relaxed); \
        atomic_fetch_add_explicit(&g_s196_medium_sc_objs[s196_sc], (uint32_t)(n), memory_order_relaxed); \
    } \
} while (0)
#else
#define S196_STAT_REGISTER() ((void)0)
#define S196_STAT_INC(name) ((void)0)
#define S196_STAT_ADD(name, val) ((void)0)
#define S196_MEDIUM_SC_ADD(sc, n) ((void)0)
#endif  // HZ3_S196_REMOTE_DISPATCH_STATS

// =============================================================================
// S203: Medium Remote Counters (A/B measurement)
// =============================================================================
// =============================================================================
// S203: Medium Remote Counters (A/B measurement) - TLS Version
// =============================================================================
#if HZ3_S203_COUNTERS
HZ3_DTOR_STATS_BEGIN(S203);
// Thread-local counters (no atomic overhead)
static _Thread_local uint64_t t_s203_medium_dispatch_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_objs = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_inbox_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_inbox_objs = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_objs = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_s209_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_s209_objs = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_s210_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_s210_objs = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_s230_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_central_s230_objs = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_mailbox_calls = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_to_mailbox_objs = 0;
static _Thread_local uint64_t t_s203_s236_mb_push_attempts = 0;
static _Thread_local uint64_t t_s203_s236_mb_push_hits = 0;
static _Thread_local uint64_t t_s203_s236_mb_push_full_fallbacks = 0;
static _Thread_local uint64_t t_s203_s236_mb_pop_attempts = 0;
static _Thread_local uint64_t t_s203_s236_mb_pop_hits = 0;
static _Thread_local uint64_t t_s203_s236c_batch_push_attempts = 0;
static _Thread_local uint64_t t_s203_s236c_batch_cache_hits = 0;
static _Thread_local uint64_t t_s203_s236c_batch_cache_misses = 0;
static _Thread_local uint64_t t_s203_s236c_batch_flush_msgs = 0;
static _Thread_local uint64_t t_s203_s236c_batch_flush_objs = 0;
static _Thread_local uint64_t t_s203_s236c_pressure_bypass = 0;
// Hist: 1, 2-3, 4-7, 8-15, 16+
static _Thread_local uint64_t t_s203_medium_dispatch_n_hist_1 = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_n_hist_2_3 = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_n_hist_4_7 = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_n_hist_8_15 = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_n_hist_16_plus = 0;
static _Thread_local uint64_t t_s203_medium_dispatch_sc_calls[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_medium_dispatch_sc_objs[HZ3_NUM_SC];

#if HZ3_S204_LARSON_DIAG
static _Thread_local uint64_t t_s204_medium_dispatch_shard_calls[HZ3_NUM_SHARDS];
static _Atomic uint64_t g_s204_medium_dispatch_shard_calls[HZ3_NUM_SHARDS];
#endif

// Global aggregators (atomic)
HZ3_DTOR_STAT(g_s203_medium_dispatch_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_objs);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_inbox_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_inbox_objs);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_objs);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_s209_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_s209_objs);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_s210_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_s210_objs);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_s230_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_central_s230_objs);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_mailbox_calls);
HZ3_DTOR_STAT(g_s203_medium_dispatch_to_mailbox_objs);
HZ3_DTOR_STAT(g_s203_s236_mb_push_attempts);
HZ3_DTOR_STAT(g_s203_s236_mb_push_hits);
HZ3_DTOR_STAT(g_s203_s236_mb_push_full_fallbacks);
HZ3_DTOR_STAT(g_s203_s236_mb_pop_attempts);
HZ3_DTOR_STAT(g_s203_s236_mb_pop_hits);
HZ3_DTOR_STAT(g_s203_s236c_batch_push_attempts);
HZ3_DTOR_STAT(g_s203_s236c_batch_cache_hits);
HZ3_DTOR_STAT(g_s203_s236c_batch_cache_misses);
HZ3_DTOR_STAT(g_s203_s236c_batch_flush_msgs);
HZ3_DTOR_STAT(g_s203_s236c_batch_flush_objs);
HZ3_DTOR_STAT(g_s203_s236c_pressure_bypass);

HZ3_DTOR_STAT(g_s203_medium_dispatch_n_hist_1);
HZ3_DTOR_STAT(g_s203_medium_dispatch_n_hist_2_3);
HZ3_DTOR_STAT(g_s203_medium_dispatch_n_hist_4_7);
HZ3_DTOR_STAT(g_s203_medium_dispatch_n_hist_8_15);
HZ3_DTOR_STAT(g_s203_medium_dispatch_n_hist_16_plus); 
static _Atomic uint64_t g_s203_medium_dispatch_sc_calls[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_medium_dispatch_sc_objs[HZ3_NUM_SC];

HZ3_DTOR_ATEXIT_FLAG(g_s203);

// Flush TLS to Global (called from thread dtor)
void hz3_s203_remote_flush_tls(void) {
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_calls, t_s203_medium_dispatch_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_objs, t_s203_medium_dispatch_objs);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_inbox_calls, t_s203_medium_dispatch_to_inbox_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_inbox_objs, t_s203_medium_dispatch_to_inbox_objs);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_calls, t_s203_medium_dispatch_to_central_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_objs, t_s203_medium_dispatch_to_central_objs);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_s209_calls, t_s203_medium_dispatch_to_central_s209_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_s209_objs, t_s203_medium_dispatch_to_central_s209_objs);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_s210_calls, t_s203_medium_dispatch_to_central_s210_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_s210_objs, t_s203_medium_dispatch_to_central_s210_objs);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_s230_calls, t_s203_medium_dispatch_to_central_s230_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_central_s230_objs, t_s203_medium_dispatch_to_central_s230_objs);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_mailbox_calls, t_s203_medium_dispatch_to_mailbox_calls);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_to_mailbox_objs, t_s203_medium_dispatch_to_mailbox_objs);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mb_push_attempts, t_s203_s236_mb_push_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mb_push_hits, t_s203_s236_mb_push_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mb_push_full_fallbacks, t_s203_s236_mb_push_full_fallbacks);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mb_pop_attempts, t_s203_s236_mb_pop_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s236_mb_pop_hits, t_s203_s236_mb_pop_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236c_batch_push_attempts, t_s203_s236c_batch_push_attempts);
    HZ3_DTOR_STAT_ADD(g_s203_s236c_batch_cache_hits, t_s203_s236c_batch_cache_hits);
    HZ3_DTOR_STAT_ADD(g_s203_s236c_batch_cache_misses, t_s203_s236c_batch_cache_misses);
    HZ3_DTOR_STAT_ADD(g_s203_s236c_batch_flush_msgs, t_s203_s236c_batch_flush_msgs);
    HZ3_DTOR_STAT_ADD(g_s203_s236c_batch_flush_objs, t_s203_s236c_batch_flush_objs);
    HZ3_DTOR_STAT_ADD(g_s203_s236c_pressure_bypass, t_s203_s236c_pressure_bypass);
    
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_n_hist_1, t_s203_medium_dispatch_n_hist_1);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_n_hist_2_3, t_s203_medium_dispatch_n_hist_2_3);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_n_hist_4_7, t_s203_medium_dispatch_n_hist_4_7);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_n_hist_8_15, t_s203_medium_dispatch_n_hist_8_15);
    HZ3_DTOR_STAT_ADD(g_s203_medium_dispatch_n_hist_16_plus, t_s203_medium_dispatch_n_hist_16_plus);
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s203_medium_dispatch_sc_calls[sc],
                                  t_s203_medium_dispatch_sc_calls[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_medium_dispatch_sc_objs[sc],
                                  t_s203_medium_dispatch_sc_objs[sc],
                                  memory_order_relaxed);
    }
#if HZ3_S204_LARSON_DIAG
    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        atomic_fetch_add_explicit(&g_s204_medium_dispatch_shard_calls[i],
                                  t_s204_medium_dispatch_shard_calls[i],
                                  memory_order_relaxed);
    }
#endif
}

static void hz3_s203_atexit_dump(void) {
    fprintf(stderr, "[HZ3_S203] medium_dispatch_calls=%u medium_dispatch_objs=%u "
            "to_inbox_calls=%u to_inbox_objs=%u to_central_calls=%u to_central_objs=%u "
            "to_mailbox_calls=%u to_mailbox_objs=%u "
            "to_central_s209_calls=%u to_central_s209_objs=%u "
            "to_central_s210_calls=%u to_central_s210_objs=%u "
            "to_central_s230_calls=%u to_central_s230_objs=%u\n",
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_objs),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_inbox_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_inbox_objs),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_objs),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_mailbox_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_mailbox_objs),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_s209_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_s209_objs),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_s210_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_s210_objs),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_s230_calls),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_to_central_s230_objs));

    if (HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_push_attempts) > 0 ||
        HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_pop_attempts) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236] mb_push_attempts=%u mb_push_hits=%u "
                "mb_push_full_fallbacks=%u mb_pop_attempts=%u mb_pop_hits=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_push_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_push_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_push_full_fallbacks),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_pop_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s236_mb_pop_hits));
    }
    if (HZ3_DTOR_STAT_LOAD(g_s203_s236c_batch_push_attempts) > 0) {
        fprintf(stderr,
                "[HZ3_S203_S236C] batch_push_attempts=%u cache_hits=%u "
                "cache_misses=%u flush_msgs=%u flush_objs=%u pressure_bypass=%u\n",
                HZ3_DTOR_STAT_LOAD(g_s203_s236c_batch_push_attempts),
                HZ3_DTOR_STAT_LOAD(g_s203_s236c_batch_cache_hits),
                HZ3_DTOR_STAT_LOAD(g_s203_s236c_batch_cache_misses),
                HZ3_DTOR_STAT_LOAD(g_s203_s236c_batch_flush_msgs),
                HZ3_DTOR_STAT_LOAD(g_s203_s236c_batch_flush_objs),
                HZ3_DTOR_STAT_LOAD(g_s203_s236c_pressure_bypass));
    }
            
    fprintf(stderr, "[HZ3_S203] medium_dispatch_n_hist: 1=%u 2..3=%u 4..7=%u 8..15=%u 16+=%u\n",
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_n_hist_1),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_n_hist_2_3),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_n_hist_4_7),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_n_hist_8_15),
            HZ3_DTOR_STAT_LOAD(g_s203_medium_dispatch_n_hist_16_plus));
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t calls =
            atomic_load_explicit(&g_s203_medium_dispatch_sc_calls[sc], memory_order_relaxed);
        uint64_t objs =
            atomic_load_explicit(&g_s203_medium_dispatch_sc_objs[sc], memory_order_relaxed);
        if (calls == 0 && objs == 0) {
            continue;
        }
        fprintf(stderr, "[HZ3_S203_REMOTE_SC] sc=%u calls=%llu objs=%llu\n",
                sc, (unsigned long long)calls, (unsigned long long)objs);
    }
#if HZ3_S204_LARSON_DIAG
    fprintf(stderr, "[HZ3_S204_SHARD_DIST] ");
    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        uint64_t c = atomic_load_explicit(&g_s204_medium_dispatch_shard_calls[i], memory_order_relaxed);
        if (c > 0) fprintf(stderr, "%u=%llu ", i, (unsigned long long)c);
    }
    fprintf(stderr, "\n");
#endif
}

#define S203_STAT_REGISTER() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s203, hz3_s203_atexit_dump)
#define S203_STAT_INC(name) (t_s203_##name++)
#define S203_STAT_ADD(name, val) (t_s203_##name += (val))
// Deprecated hist macro
#define S203_HIST_INC(n) ((void)0)
#if HZ3_S204_LARSON_DIAG
#define S204_SHARD_INC(shard) do { \
    if ((shard) < HZ3_NUM_SHARDS) { \
        t_s204_medium_dispatch_shard_calls[(shard)]++; \
    } \
} while(0)
#else
#define S204_SHARD_INC(shard) ((void)0)
#endif
void hz3_s203_remote_register_once(void) { S203_STAT_REGISTER(); }
#else
#define S203_STAT_REGISTER() ((void)0)
#define S203_STAT_INC(name) ((void)0)
#define S203_STAT_ADD(name, val) ((void)0)
void hz3_s203_remote_register_once(void) {}
#endif

// ============================================================================
// S236-A: Medium mailbox (singleton remote publish -> alloc-fast pop)
// ============================================================================
#if HZ3_S236_MEDIUM_MAILBOX
static _Atomic(void*) g_s236_medium_mailbox[HZ3_NUM_SHARDS][HZ3_NUM_SC][HZ3_S236_MAILBOX_SLOTS];
#endif

int hz3_s236_medium_mailbox_try_push(uint8_t dst, int sc, void* obj) {
#if !HZ3_S236_MEDIUM_MAILBOX
    (void)dst;
    (void)sc;
    (void)obj;
    return 0;
#else
    if (!obj) {
        return 0;
    }
    if (dst >= HZ3_NUM_SHARDS) {
        return 0;
    }
    if (sc < HZ3_S236_SC_MIN || sc > HZ3_S236_SC_MAX) {
        return 0;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }

    S203_STAT_INC(s236_mb_push_attempts);
    for (uint32_t slot = 0; slot < (uint32_t)HZ3_S236_MAILBOX_SLOTS; slot++) {
        void* expected = NULL;
        if (atomic_compare_exchange_strong_explicit(
                &g_s236_medium_mailbox[dst][sc][slot], &expected, obj,
                memory_order_release, memory_order_relaxed)) {
            S203_STAT_INC(s236_mb_push_hits);
            return 1;
        }
    }
    S203_STAT_INC(s236_mb_push_full_fallbacks);
    return 0;
#endif
}

void* hz3_s236_medium_mailbox_try_pop(int sc) {
#if !HZ3_S236_MEDIUM_MAILBOX
    (void)sc;
    return NULL;
#else
    if (sc < HZ3_S236_SC_MIN || sc > HZ3_S236_SC_MAX) {
        return NULL;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return NULL;
    }
    uint8_t my = t_hz3_cache.my_shard;
    if (my >= HZ3_NUM_SHARDS) {
        return NULL;
    }

    S203_STAT_INC(s236_mb_pop_attempts);
    for (uint32_t slot = 0; slot < (uint32_t)HZ3_S236_MAILBOX_SLOTS; slot++) {
        void* obj = atomic_exchange_explicit(&g_s236_medium_mailbox[my][sc][slot], NULL,
                                             memory_order_acq_rel);
        if (obj) {
            S203_STAT_INC(s236_mb_pop_hits);
            return obj;
        }
    }
    return NULL;
#endif
}

// =============================================================================
// S128: RemoteStash Defer-MinRun (ARCHIVED / NO-GO)
// =============================================================================
// The full implementation was removed from this file to keep the hot boundary
// maintainable. See:
// - hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md
// - hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/s128_defer_minrun.inc

// ============================================================================
// Day 4: Outbox operations
// ============================================================================

#if !HZ3_REMOTE_STASH_SPARSE
// Flush outbox to owner's inbox (FIFO order)
void hz3_outbox_flush(uint8_t owner, int sc) {
    Hz3OutboxBin* ob = &t_hz3_cache.outbox[owner][sc];
    if (ob->count == 0) return;

    // Build FIFO linked list: slots[0] -> slots[1] -> ... -> slots[count-1]
    void* head = ob->slots[0];
    for (uint8_t i = 0; i < ob->count - 1; i++) {
        hz3_obj_set_next(ob->slots[i], ob->slots[i + 1]);
    }
    hz3_obj_set_next(ob->slots[ob->count - 1], NULL);
    void* tail = ob->slots[ob->count - 1];

    // Push to inbox
    hz3_inbox_push_list(owner, sc, head, tail, ob->count);
    ob->count = 0;
}

// Push to outbox, flush if full
void hz3_outbox_push(uint8_t owner, int sc, void* obj) {
    Hz3OutboxBin* ob = &t_hz3_cache.outbox[owner][sc];
    ob->slots[ob->count++] = obj;

    if (ob->count >= HZ3_OUTBOX_SIZE) {
        hz3_outbox_flush(owner, sc);
    }
}
#endif  // !HZ3_REMOTE_STASH_SPARSE

#include "hz3_tcache_remote_stash.inc"
// ============================================================================
// S17: dst/bin direct remote bank flush (event-only, dense bank only)
// ============================================================================

#if HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE

#if HZ3_TCACHE_SOA_BANK
// S40: SoA version - detach from bank_head/bank_count
static void hz3_dstbin_detach_soa(uint8_t dst, int bin_idx, void** head_out, void** tail_out, uint32_t* n_out) {
    void* head = t_hz3_cache.bank_head[dst][bin_idx];
    if (!head) {
        *head_out = NULL;
        *tail_out = NULL;
        *n_out = 0;
        return;
    }
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    t_hz3_cache.bank_head[dst][bin_idx] = NULL;
#if !HZ3_BIN_LAZY_COUNT
    t_hz3_cache.bank_count[dst][bin_idx] = 0;
#endif
    *head_out = head;
    *tail_out = tail;
    *n_out = n;
}

// S40: SoA version - flush one bin
void hz3_dstbin_flush_one(uint8_t dst, int bin) {
    if (!t_hz3_cache.bank_head[dst][bin]) return;

    void* head;
    void* tail;
    uint32_t n;
    hz3_dstbin_detach_soa(dst, bin, &head, &tail, &n);
    if (!head) return;

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
    } else {
        int sc = bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}

#else  // !HZ3_TCACHE_SOA_BANK

static void hz3_dstbin_detach(Hz3Bin* bin, void** head_out, void** tail_out, uint32_t* n_out) {
    void* head = hz3_bin_head(bin);
    if (!head) {
        *head_out = NULL;
        *tail_out = NULL;
        *n_out = 0;
        return;
    }
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    hz3_bin_clear(bin);
    *head_out = head;
    *tail_out = tail;
    *n_out = n;
}

// S24: Helper to flush one bin (shared by budget/all)
void hz3_dstbin_flush_one(uint8_t dst, int bin) {
    Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
    if (hz3_bin_is_empty(b)) return;

    void* head;
    void* tail;
    uint32_t n;
    hz3_dstbin_detach(b, &head, &tail, &n);
    if (!head) return;

    if (bin < HZ3_SMALL_NUM_SC) {
        hz3_small_v2_push_remote_list(dst, bin, head, tail, n);
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        int sc = bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
    } else {
        int sc = bin - HZ3_MEDIUM_BIN_BASE;
        hz3_inbox_push_list(dst, sc, head, tail, n);
    }
}
#endif  // HZ3_TCACHE_SOA_BANK

#endif  // HZ3_PTAG_DSTBIN_ENABLE && !HZ3_REMOTE_STASH_SPARSE

// ============================================================================
// S24-1/S41: Remote flush entry points (work for both sparse and dense)
// ============================================================================

#if HZ3_PTAG_DSTBIN_ENABLE
// S190-v3: targeted remote flush by medium size-class.
void hz3_dstbin_flush_remote_sc_budget(uint32_t sc, uint32_t budget_bins) {
    if (budget_bins == 0) return;

    uint32_t target_bin32 = (uint32_t)HZ3_MEDIUM_BIN_BASE + sc;
    if (target_bin32 >= (uint32_t)HZ3_BIN_TOTAL) {
        hz3_dstbin_flush_remote_budget(budget_bins);
        return;
    }

    uint8_t target_bin = (uint8_t)target_bin32;
#if HZ3_REMOTE_STASH_SPARSE
    // Sparse mode keeps a ring; only fire budget flush when target bin is observed.
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t t = ring->tail;
    uint16_t h = ring->head;
    uint32_t scanned = 0;
    int found_target = 0;
    while (t != h && scanned < budget_bins) {
        if (ring->ring[t].bin == target_bin) {
            found_target = 1;
            break;
        }
        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        scanned++;
    }
    if (!found_target) {
        return;
    }
    hz3_remote_stash_flush_budget_impl(budget_bins);
#if HZ3_S173_DSTBIN_DEMAND_GATE
    uint16_t remaining = (uint16_t)((ring->head - ring->tail) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    if (remaining > (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        remaining = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP;
    }
    t_hz3_cache.remote_flush_credit = remaining;
#endif
#else
    // Dense mode: scan only owner shards for the selected bin.
    uint8_t my_shard = t_hz3_cache.my_shard;
    uint8_t dst = t_hz3_cache.remote_dst_cursor;
    uint8_t start_dst = dst;
    uint32_t scanned = 0;
    int wrapped = 0;

    while (scanned < budget_bins && !wrapped) {
        if (dst == my_shard) {
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            if (dst == start_dst) break;
            continue;
        }

#if HZ3_TCACHE_SOA_BANK
        void* head = t_hz3_cache.bank_head[dst][target_bin];
#else
        Hz3Bin* b = hz3_tcache_get_bank_bin(dst, (int)target_bin);
        void* head = b->head;
#endif
        scanned++;
        if (head) {
            hz3_dstbin_flush_one(dst, (int)target_bin);
            t_hz3_cache.remote_hint = 1;
        }

        dst = (dst + 1) % HZ3_NUM_SHARDS;
        if (dst == start_dst) wrapped = 1;
    }

    t_hz3_cache.remote_dst_cursor = dst;
#endif
}

// S24-1: Budgeted flush with round-robin cursor
void hz3_dstbin_flush_remote_budget(uint32_t budget_bins) {
#if HZ3_REMOTE_STASH_SPARSE
    // S41: For sparse ring, budget_bins means "number of ring entries"
    //      (not "number of bins to scan" as in dense bank)
    hz3_remote_stash_flush_budget_impl(budget_bins);
#if HZ3_S173_DSTBIN_DEMAND_GATE
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t remaining = (uint16_t)((ring->head - ring->tail) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    if (remaining > (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        remaining = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP;
    }
    t_hz3_cache.remote_flush_credit = remaining;
#endif
#else
    // Dense bank implementation
    uint8_t my_shard = t_hz3_cache.my_shard;
    uint8_t dst = t_hz3_cache.remote_dst_cursor;
    uint16_t bin = t_hz3_cache.remote_bin_cursor;
    uint32_t scanned = 0;
    uint8_t start_dst = dst;
    int wrapped = 0;

    while (scanned < budget_bins && !wrapped) {
        // Skip my_shard
        if (dst == my_shard) {
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            bin = 0;
            if (dst == start_dst) break;
            continue;
        }

#if HZ3_TCACHE_SOA_BANK
        void* head = t_hz3_cache.bank_head[dst][bin];
#else
        Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
        void* head = b->head;
#endif
        scanned++;
        if (head) {
            hz3_dstbin_flush_one(dst, bin);
            // Found remote -> set hint (budgeted can only raise, never lower)
            t_hz3_cache.remote_hint = 1;
        }

        // Advance cursor
        bin++;
        if (bin >= HZ3_BIN_TOTAL) {
            bin = 0;
            dst = (dst + 1) % HZ3_NUM_SHARDS;
            // Check for wrap-around
            if (dst == start_dst) wrapped = 1;
        }
    }

    // Save cursor for next call
    t_hz3_cache.remote_dst_cursor = dst;
    t_hz3_cache.remote_bin_cursor = bin;
#endif
}

// S24-1: Full flush (all bins, for epoch/destructor)
void hz3_dstbin_flush_remote_all(void) {
#if HZ3_REMOTE_STASH_SPARSE
    hz3_remote_stash_flush_all_impl();
#if HZ3_S173_DSTBIN_DEMAND_GATE
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t remaining = (uint16_t)((ring->head - ring->tail) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    if (remaining > (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        remaining = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP;
    }
    t_hz3_cache.remote_flush_credit = remaining;
#endif
#else
    // Dense bank implementation
    uint8_t my_shard = t_hz3_cache.my_shard;
    int found_any = 0;

    for (uint8_t dst = 0; dst < HZ3_NUM_SHARDS; dst++) {
        if (dst == my_shard) continue;
        for (int bin = 0; bin < HZ3_BIN_TOTAL; bin++) {
#if HZ3_TCACHE_SOA_BANK
            if (t_hz3_cache.bank_head[dst][bin]) {
                hz3_dstbin_flush_one(dst, bin);
                found_any = 1;
            }
#else
            Hz3Bin* b = hz3_tcache_get_bank_bin(dst, bin);
            if (b->head) {
                hz3_dstbin_flush_one(dst, bin);
                found_any = 1;
            }
#endif
        }
    }

    // Full flush confirms hint: 0 if nothing found, 1 if any found
    t_hz3_cache.remote_hint = found_any ? 1 : 0;
    // Reset cursor after full flush
    t_hz3_cache.remote_dst_cursor = 0;
    t_hz3_cache.remote_bin_cursor = 0;
#endif
}
#endif  // HZ3_PTAG_DSTBIN_ENABLE
