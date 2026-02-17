#define _GNU_SOURCE

#include "hz3_inbox.h"
#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_stash_guard.h"
#include "hz3_arena.h"
#include "hz3_segmap.h"
#include "hz3_tag.h"

#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Global inbox pool (zero-initialized)
Hz3Inbox g_hz3_inbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];
Hz3InboxMixed g_hz3_inbox_medium_mixed[HZ3_NUM_SHARDS];

static inline int hz3_s231_fast_sc_enabled(int sc) {
#if HZ3_S231_INBOX_FAST_MPSC
    return (sc >= HZ3_S231_SC_MIN && sc <= HZ3_S231_SC_MAX);
#else
    (void)sc;
    return 0;
#endif
}

// Day 5: pthread_once for thread-safe initialization
static pthread_once_t g_hz3_inbox_once = PTHREAD_ONCE_INIT;

#if HZ3_S203_COUNTERS
// S203: Inbox Counters (Medium) - TLS Version
static _Thread_local uint64_t t_s203_inbox_push_calls = 0;
static _Thread_local uint64_t t_s203_inbox_push_objs = 0;
static _Thread_local uint64_t t_s203_inbox_push_retries = 0;
static _Thread_local uint64_t t_s203_inbox_drain_calls = 0;
static _Thread_local uint64_t t_s203_inbox_drain_nonempty = 0;
static _Thread_local uint64_t t_s203_inbox_drain_objs = 0;
static _Thread_local uint64_t t_s203_inbox_push_sc_objs[HZ3_NUM_SC];
static _Thread_local uint64_t t_s203_inbox_drain_sc_objs[HZ3_NUM_SC];

static _Atomic uint64_t g_s203_inbox_push_calls = 0;
static _Atomic uint64_t g_s203_inbox_push_objs = 0;
static _Atomic uint64_t g_s203_inbox_push_retries = 0;
static _Atomic uint64_t g_s203_inbox_drain_calls = 0;
static _Atomic uint64_t g_s203_inbox_drain_nonempty = 0;
static _Atomic uint64_t g_s203_inbox_drain_objs = 0;
static _Atomic uint64_t g_s203_inbox_push_sc_objs[HZ3_NUM_SC];
static _Atomic uint64_t g_s203_inbox_drain_sc_objs[HZ3_NUM_SC];

#if HZ3_S204_LARSON_DIAG
extern _Thread_local bool t_s204_in_dtor;
static _Thread_local uint64_t t_s204_inbox_push_dtor_sc_objs[HZ3_NUM_SC];
static _Atomic uint64_t g_s204_inbox_push_dtor_sc_objs[HZ3_NUM_SC];
#endif

static pthread_once_t g_s203_inbox_stats_once = PTHREAD_ONCE_INIT;

// Flush TLS to Global (called from thread dtor)
void hz3_s203_inbox_flush_tls(void) {
    atomic_fetch_add_explicit(&g_s203_inbox_push_calls, t_s203_inbox_push_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_push_objs, t_s203_inbox_push_objs, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_push_retries, t_s203_inbox_push_retries, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_drain_calls, t_s203_inbox_drain_calls, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_drain_nonempty, t_s203_inbox_drain_nonempty, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s203_inbox_drain_objs, t_s203_inbox_drain_objs, memory_order_relaxed);
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s203_inbox_push_sc_objs[sc],
                                  t_s203_inbox_push_sc_objs[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s203_inbox_drain_sc_objs[sc],
                                  t_s203_inbox_drain_sc_objs[sc],
                                  memory_order_relaxed);
#if HZ3_S204_LARSON_DIAG
        atomic_fetch_add_explicit(&g_s204_inbox_push_dtor_sc_objs[sc],
                                  t_s204_inbox_push_dtor_sc_objs[sc],
                                  memory_order_relaxed);
#endif
    }
}

static void hz3_s203_inbox_stats_dump(void) {
    fprintf(stderr,
            "[HZ3_S203] inbox_push_calls=%llu inbox_push_objs=%llu inbox_push_retries=%llu "
            "inbox_drain_calls=%llu inbox_drain_nonempty=%llu inbox_drain_objs=%llu\n",
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_push_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_push_objs, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_push_retries, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_drain_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_drain_nonempty, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s203_inbox_drain_objs, memory_order_relaxed));
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t push_objs =
            atomic_load_explicit(&g_s203_inbox_push_sc_objs[sc], memory_order_relaxed);
        uint64_t drain_objs =
            atomic_load_explicit(&g_s203_inbox_drain_sc_objs[sc], memory_order_relaxed);
        if (push_objs == 0 && drain_objs == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S203_INBOX_SC] sc=%u push_objs=%llu drain_objs=%llu\n",
                sc,
                (unsigned long long)push_objs,
                (unsigned long long)drain_objs);
#if HZ3_S204_LARSON_DIAG
        uint64_t dtor_push =
            atomic_load_explicit(&g_s204_inbox_push_dtor_sc_objs[sc], memory_order_relaxed);
        if (dtor_push > 0) {
             fprintf(stderr,
                "[HZ3_S204_INBOX_SC] sc=%u push_dtor_objs=%llu\n",
                sc, (unsigned long long)dtor_push);
        }
#endif
    }
}

static void hz3_s203_inbox_stats_register(void) {
    atexit(hz3_s203_inbox_stats_dump);
}

static void hz3_s203_inbox_stats_register_once(void) {
    pthread_once(&g_s203_inbox_stats_once, hz3_s203_inbox_stats_register);
}

bool hz3_inbox_peek_check(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return false;
    if (sc < 0 || sc >= HZ3_NUM_SC) return false;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t lane_max = (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    if (hz3_s231_fast_sc_enabled(sc)) {
        lane_max = 1;
    }
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        if (atomic_load_explicit(&inbox->head[lane], memory_order_relaxed) != NULL) {
            return true;
        }
    }
    return false;
}

#else
static inline void hz3_s203_inbox_stats_register_once(void) {}
bool hz3_inbox_peek_check(uint8_t shard, int sc) { (void)shard; (void)sc; return false; }
void hz3_s203_inbox_flush_tls(void) {}
#endif

uint32_t hz3_inbox_count_approx(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    if (hz3_s231_fast_sc_enabled(sc)) {
        return atomic_load_explicit(&inbox->count[0], memory_order_relaxed);
    }
    uint32_t total = 0;
    for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
        total += atomic_load_explicit(&inbox->count[lane], memory_order_relaxed);
    }
    return total;
}

uint32_t hz3_inbox_drain_seq_load(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    return atomic_load_explicit(&g_hz3_inbox[shard][sc].drain_seq, memory_order_acquire);
}


#if HZ3_S197_MEDIUM_INBOX_CAS_STATS
static _Atomic uint64_t g_s197_push_calls = 0;
static _Atomic uint64_t g_s197_push_objs = 0;
static _Atomic uint64_t g_s197_push_retry_calls = 0;
static _Atomic uint64_t g_s197_push_retries = 0;
static _Atomic uint64_t g_s197_push_max_retry = 0;
static _Atomic uint64_t g_s197_push_retry_hist_0 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_1 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_2_3 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_4_7 = 0;
static _Atomic uint64_t g_s197_push_retry_hist_8p = 0;
static _Atomic uint64_t g_s197_drain_calls = 0;
static _Atomic uint64_t g_s197_drain_nonempty = 0;
static _Atomic uint64_t g_s197_drain_objs = 0;
static _Atomic uint64_t g_s197_push_sc_calls[HZ3_NUM_SC];
static _Atomic uint64_t g_s197_push_sc_objs[HZ3_NUM_SC];
static _Atomic uint64_t g_s197_push_sc_retries[HZ3_NUM_SC];
static _Atomic uint64_t g_s197_drain_sc_objs[HZ3_NUM_SC];
static pthread_once_t g_s197_stats_once = PTHREAD_ONCE_INIT;

static void hz3_s197_stats_max_retry_update(uint64_t retries) {
    uint64_t cur = atomic_load_explicit(&g_s197_push_max_retry, memory_order_relaxed);
    while (retries > cur &&
           !atomic_compare_exchange_weak_explicit(&g_s197_push_max_retry, &cur, retries,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed)) {
    }
}

static void hz3_s197_stats_dump(void) {
    uint64_t push_calls =
        atomic_load_explicit(&g_s197_push_calls, memory_order_relaxed);
    uint64_t push_retries =
        atomic_load_explicit(&g_s197_push_retries, memory_order_relaxed);
    uint64_t retry_calls =
        atomic_load_explicit(&g_s197_push_retry_calls, memory_order_relaxed);
    uint64_t retry_max =
        atomic_load_explicit(&g_s197_push_max_retry, memory_order_relaxed);
    uint64_t drain_calls =
        atomic_load_explicit(&g_s197_drain_calls, memory_order_relaxed);
    uint64_t drain_nonempty =
        atomic_load_explicit(&g_s197_drain_nonempty, memory_order_relaxed);
    uint64_t drain_objs =
        atomic_load_explicit(&g_s197_drain_objs, memory_order_relaxed);
    double retry_per_call = (push_calls > 0)
                                ? ((double)push_retries / (double)push_calls)
                                : 0.0;
    double retry_call_pct = (push_calls > 0)
                                ? (100.0 * (double)retry_calls / (double)push_calls)
                                : 0.0;
    fprintf(stderr,
            "[HZ3_S197_INBOX_CAS] push_calls=%llu push_objs=%llu retry_calls=%llu "
            "push_retries=%llu retry_per_call=%.4f retry_call_pct=%.2f max_retry=%llu "
            "hist0=%llu hist1=%llu hist2_3=%llu hist4_7=%llu hist8p=%llu "
            "drain_calls=%llu drain_nonempty=%llu drain_objs=%llu\n",
            (unsigned long long)push_calls,
            (unsigned long long)atomic_load_explicit(&g_s197_push_objs, memory_order_relaxed),
            (unsigned long long)retry_calls,
            (unsigned long long)push_retries,
            retry_per_call,
            retry_call_pct,
            (unsigned long long)retry_max,
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_0, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_1, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_2_3, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_4_7, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s197_push_retry_hist_8p, memory_order_relaxed),
            (unsigned long long)drain_calls,
            (unsigned long long)drain_nonempty,
            (unsigned long long)drain_objs);

    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t sc_calls =
            atomic_load_explicit(&g_s197_push_sc_calls[sc], memory_order_relaxed);
        uint64_t sc_objs =
            atomic_load_explicit(&g_s197_push_sc_objs[sc], memory_order_relaxed);
        uint64_t sc_retries =
            atomic_load_explicit(&g_s197_push_sc_retries[sc], memory_order_relaxed);
        uint64_t sc_drain_objs =
            atomic_load_explicit(&g_s197_drain_sc_objs[sc], memory_order_relaxed);
        if (sc_calls == 0 && sc_objs == 0 && sc_retries == 0 && sc_drain_objs == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S197_INBOX_CAS_SC] sc=%u push_calls=%llu push_objs=%llu "
                "push_retries=%llu drain_objs=%llu\n",
                sc,
                (unsigned long long)sc_calls,
                (unsigned long long)sc_objs,
                (unsigned long long)sc_retries,
                (unsigned long long)sc_drain_objs);
    }
}

static void hz3_s197_stats_register(void) {
    atexit(hz3_s197_stats_dump);
}

static inline void hz3_s197_stats_register_once(void) {
    pthread_once(&g_s197_stats_once, hz3_s197_stats_register);
}

static inline void hz3_s197_stats_on_push(int sc, uint32_t n, uint64_t retries) {
    atomic_fetch_add_explicit(&g_s197_push_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s197_push_objs, n, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s197_push_retries, retries, memory_order_relaxed);
    if (retries > 0) {
        atomic_fetch_add_explicit(&g_s197_push_retry_calls, 1, memory_order_relaxed);
    }
    if (retries == 0) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_0, 1, memory_order_relaxed);
    } else if (retries == 1) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_1, 1, memory_order_relaxed);
    } else if (retries <= 3) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_2_3, 1, memory_order_relaxed);
    } else if (retries <= 7) {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_4_7, 1, memory_order_relaxed);
    } else {
        atomic_fetch_add_explicit(&g_s197_push_retry_hist_8p, 1, memory_order_relaxed);
    }
    if (sc >= 0 && sc < HZ3_NUM_SC) {
        atomic_fetch_add_explicit(&g_s197_push_sc_calls[sc], 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s197_push_sc_objs[sc], n, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s197_push_sc_retries[sc], retries, memory_order_relaxed);
    }
    hz3_s197_stats_max_retry_update(retries);
}

static inline void hz3_s197_stats_on_drain(int sc, uint64_t drained_objs, int nonempty) {
    atomic_fetch_add_explicit(&g_s197_drain_calls, 1, memory_order_relaxed);
    if (nonempty) {
        atomic_fetch_add_explicit(&g_s197_drain_nonempty, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s197_drain_objs, drained_objs, memory_order_relaxed);
        if (sc >= 0 && sc < HZ3_NUM_SC) {
            atomic_fetch_add_explicit(&g_s197_drain_sc_objs[sc], drained_objs, memory_order_relaxed);
        }
    }
}
#else
static inline void hz3_s197_stats_register_once(void) {}
static inline void hz3_s197_stats_on_push(int sc, uint32_t n, uint64_t retries) {
    (void)sc;
    (void)n;
    (void)retries;
}
static inline void hz3_s197_stats_on_drain(int sc, uint64_t drained_objs, int nonempty) {
    (void)sc;
    (void)drained_objs;
    (void)nonempty;
}
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS
static _Atomic uint64_t g_s195_push_calls = 0;
static _Atomic uint64_t g_s195_push_objs = 0;
static _Atomic uint64_t g_s195_push_retries = 0;
static _Atomic uint64_t g_s195_push_max_retry = 0;
static _Atomic uint64_t g_s195_drain_calls = 0;
static _Atomic uint64_t g_s195_drain_nonempty = 0;
static _Atomic uint64_t g_s195_drain_objs = 0;
static _Atomic uint64_t g_s195_lane_pushes[HZ3_S195_MEDIUM_INBOX_SHARDS];
static pthread_once_t g_s195_stats_once = PTHREAD_ONCE_INIT;

static void hz3_s195_stats_max_retry_update(uint64_t retries) {
    uint64_t cur = atomic_load_explicit(&g_s195_push_max_retry, memory_order_relaxed);
    while (retries > cur &&
           !atomic_compare_exchange_weak_explicit(&g_s195_push_max_retry, &cur, retries,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed)) {
    }
}

static void hz3_s195_stats_dump(void) {
    fprintf(stderr,
            "[hz3] S195 inbox stats: push_calls=%llu push_objs=%llu push_retries=%llu "
            "max_retry=%llu drain_calls=%llu drain_nonempty=%llu drain_objs=%llu shards=%u\n",
            (unsigned long long)atomic_load_explicit(&g_s195_push_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_push_objs, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_push_retries, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_push_max_retry, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_drain_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_drain_nonempty, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s195_drain_objs, memory_order_relaxed),
            (unsigned)HZ3_S195_MEDIUM_INBOX_SHARDS);
    fprintf(stderr, "[hz3] S195 inbox lane_pushes:");
    for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
        fprintf(stderr, " [%u]=%llu", (unsigned)lane,
                (unsigned long long)atomic_load_explicit(&g_s195_lane_pushes[lane], memory_order_relaxed));
    }
    fprintf(stderr, "\n");
}

static void hz3_s195_stats_register(void) {
    atexit(hz3_s195_stats_dump);
}
#endif

static inline uint32_t hz3_inbox_lane_select(uint8_t owner, int sc, void* head, void* tail) {
#if HZ3_S195_MEDIUM_INBOX_SHARDS == 1
    (void)owner;
    (void)sc;
    (void)head;
    (void)tail;
    return 0;
#else
    uintptr_t x = (uintptr_t)head;
    x ^= ((uintptr_t)tail >> 4);
    x ^= ((uintptr_t)owner << 16);
    x ^= ((uintptr_t)(uint32_t)sc << 8);
    x ^= (x >> 33);
    x *= 0xff51afd7ed558ccdULL;
    x ^= (x >> 33);
    return (uint32_t)x & (HZ3_S195_MEDIUM_INBOX_SHARDS - 1u);
#endif
}

static void hz3_inbox_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
            for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
                atomic_store_explicit(&inbox->head[lane], NULL, memory_order_relaxed);
                atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);
            }
            atomic_store_explicit(&inbox->seq, 0, memory_order_relaxed);
            atomic_store_explicit(&inbox->drain_seq, 0, memory_order_relaxed);
        }
        Hz3InboxMixed* mixed = &g_hz3_inbox_medium_mixed[shard];
        for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
            atomic_store_explicit(&mixed->head[lane], NULL, memory_order_relaxed);
            atomic_store_explicit(&mixed->count[lane], 0, memory_order_relaxed);
        }
        atomic_store_explicit(&mixed->seq, 0, memory_order_relaxed);
        atomic_store_explicit(&mixed->drain_seq, 0, memory_order_relaxed);
    }
}

void hz3_inbox_init(void) {
    pthread_once(&g_hz3_inbox_once, hz3_inbox_do_init);
#if HZ3_S195_MEDIUM_INBOX_STATS
    pthread_once(&g_s195_stats_once, hz3_s195_stats_register);
#endif
#if HZ3_S203_COUNTERS
    hz3_s203_inbox_stats_register_once();
#endif
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
    uint32_t lane = hz3_s231_fast_sc_enabled(sc) ? 0u : hz3_inbox_lane_select(owner, sc, head, tail);
    hz3_s197_stats_register_once();

    // CAS loop to push list atomically
    void* old_head;
#if HZ3_S195_MEDIUM_INBOX_STATS || HZ3_S197_MEDIUM_INBOX_CAS_STATS || HZ3_S203_COUNTERS
    uint64_t attempts = 0;
    do {
        attempts++;
        old_head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head[lane], &old_head, head,
        memory_order_release, memory_order_relaxed));

#if HZ3_S203_COUNTERS
    t_s203_inbox_push_calls++;
    t_s203_inbox_push_objs += n;
    t_s203_inbox_push_sc_objs[sc] += n;
    if (attempts > 1) {
        t_s203_inbox_push_retries += (attempts - 1);
    }
#if HZ3_S204_LARSON_DIAG
    if (t_s204_in_dtor) {
        t_s204_inbox_push_dtor_sc_objs[sc] += n;
    }
#endif
#endif
#else
    do {
        old_head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head[lane], &old_head, head,
        memory_order_release, memory_order_relaxed));
#endif



    atomic_fetch_add_explicit(&inbox->count[lane], n, memory_order_relaxed);
#if !(HZ3_S231_INBOX_FAST_MPSC && HZ3_S231_SKIP_PUSH_SEQ)
    atomic_fetch_add_explicit(&inbox->seq, 1, memory_order_release);
#else
    if (!hz3_s231_fast_sc_enabled(sc)) {
        atomic_fetch_add_explicit(&inbox->seq, 1, memory_order_release);
    }
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS || HZ3_S197_MEDIUM_INBOX_CAS_STATS
    uint64_t retries = (attempts > 0) ? (attempts - 1) : 0;
    hz3_s197_stats_on_push(sc, n, retries);
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS
    atomic_fetch_add_explicit(&g_s195_push_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_push_objs, n, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_push_retries, retries, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_lane_pushes[lane], 1, memory_order_relaxed);
    hz3_s195_stats_max_retry_update(retries);
#endif
}

void hz3_inbox_medium_mixed_push_list(uint8_t owner, void* head, void* tail, uint32_t n) {
    if (owner >= HZ3_NUM_SHARDS) return;
    if (!head || !tail || n == 0) return;

    Hz3InboxMixed* inbox = &g_hz3_inbox_medium_mixed[owner];
    uint32_t lane = hz3_inbox_lane_select(owner, -1, head, tail);

    void* old_head;
    do {
        old_head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head[lane], &old_head, head,
        memory_order_release, memory_order_relaxed));

    atomic_fetch_add_explicit(&inbox->count[lane], n, memory_order_relaxed);
    atomic_fetch_add_explicit(&inbox->seq, 1, memory_order_release);
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

void* hz3_inbox_drain_if_nonempty(uint8_t shard, int sc, Hz3Bin* bin) {
#if HZ3_S198_INBOX_DRAIN_HINT_GATE
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        if (atomic_load_explicit(&inbox->head[lane], memory_order_relaxed) != NULL) {
            return hz3_inbox_drain(shard, sc, bin);
        }
    }
    return NULL;
#else
    return hz3_inbox_drain(shard, sc, bin);
#endif
}

void* hz3_inbox_drain_if_seq_advanced(uint8_t shard, int sc, Hz3Bin* bin,
                                      uint32_t* last_seq) {
#if HZ3_S200_INBOX_SEQ_GATE
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin || !last_seq) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t seq_now = atomic_load_explicit(&inbox->seq, memory_order_acquire);
    if (seq_now == *last_seq) {
        return NULL;
    }
    *last_seq = seq_now;

#if HZ3_S200_INBOX_SEQ_STRICT
    return hz3_inbox_drain(shard, sc, bin);
#else
    return hz3_inbox_drain_if_nonempty(shard, sc, bin);
#endif
#else
    (void)last_seq;
    return hz3_inbox_drain_if_nonempty(shard, sc, bin);
#endif
}

void* hz3_inbox_drain(uint8_t shard, int sc, Hz3Bin* bin) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    hz3_s197_stats_register_once();
#if HZ3_S195_MEDIUM_INBOX_STATS
    atomic_fetch_add_explicit(&g_s195_drain_calls, 1, memory_order_relaxed);
#endif
#if HZ3_S203_COUNTERS
    t_s203_inbox_drain_calls++;
#endif
    void* first = NULL;
    void* obj = NULL;
    uint64_t drained_objs = 0;

    // Day 5: Build overflow list for central (don't re-push to inbox)
    void* overflow_head = NULL;
    void* overflow_tail = NULL;
    uint32_t overflow_n = 0;

#if HZ3_S201_MEDIUM_INBOX_SPLICE
    // Helper for S201-lite: count contiguous chain and capture tail.
    // Keeps writes minimal by appending/splicing whole chains.
    #define HZ3_INBOX_CHAIN_COUNT(_head, _tail_out, _n_out) \
        do {                                                 \
            void* _cur = (_head);                           \
            void* _tail = _cur;                             \
            uint32_t _n = 0;                                \
            while (_cur) {                                  \
                _tail = _cur;                               \
                _n++;                                       \
                _cur = hz3_obj_get_next(_cur);              \
            }                                               \
            *(_tail_out) = _tail;                           \
            *(_n_out) = _n;                                 \
        } while (0)
#endif

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
    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        // Fast path per lane: skip exchange when lane is empty.
        void* head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        if (!head) {
            continue;
        }
        head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) {
            continue;
        }
        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        if (!first) {
            first = head;
            obj = hz3_obj_get_next(head);
            drained_objs++;
        } else {
            obj = head;
        }

#if HZ3_S201_MEDIUM_INBOX_SPLICE
        if (!obj) {
            continue;
        }

        uint32_t cap_left = 0;
#if HZ3_BIN_LAZY_COUNT
        if (bin_len < HZ3_BIN_HARD_CAP) {
            cap_left = (uint32_t)(HZ3_BIN_HARD_CAP - bin_len);
        }
#else
        {
            Hz3BinCount cur_count = hz3_bin_count_get(bin);
            if (cur_count < HZ3_BIN_HARD_CAP) {
                cap_left = (uint32_t)(HZ3_BIN_HARD_CAP - cur_count);
            }
        }
#endif

        if (cap_left == 0) {
            void* rem_tail = NULL;
            uint32_t rem_n = 0;
            HZ3_INBOX_CHAIN_COUNT(obj, &rem_tail, &rem_n);
            if (!overflow_head) {
                overflow_head = obj;
            } else {
                hz3_obj_set_next(overflow_tail, obj);
            }
            overflow_tail = rem_tail;
            overflow_n += rem_n;
            drained_objs += rem_n;
            continue;
        }

        void* take_head = obj;
        void* take_tail = NULL;
        uint32_t take_n = 0;
        while (obj && take_n < cap_left) {
            take_tail = obj;
            take_n++;
            obj = hz3_obj_get_next(obj);
        }

        if (take_n > 0) {
            hz3_bin_prepend_list(bin, take_head, take_tail, take_n);
#if HZ3_BIN_LAZY_COUNT
            bin_len += (uint16_t)take_n;
#endif
            drained_objs += take_n;
        }

        if (obj) {
            void* rem_tail = NULL;
            uint32_t rem_n = 0;
            HZ3_INBOX_CHAIN_COUNT(obj, &rem_tail, &rem_n);
            if (!overflow_head) {
                overflow_head = obj;
            } else {
                hz3_obj_set_next(overflow_tail, obj);
            }
            overflow_tail = rem_tail;
            overflow_n += rem_n;
            drained_objs += rem_n;
        }
#else
        while (obj) {
            void* next = hz3_obj_get_next(obj);
            drained_objs++;
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
#endif
    }

    if (!first) {
        hz3_s197_stats_on_drain(sc, 0, 0);
        return NULL;
    }

    // Day 5: Send overflow to central (NOT back to inbox)
    if (overflow_head) {
        hz3_obj_set_next(overflow_tail, NULL);
        hz3_central_push_list(shard, sc, overflow_head, overflow_tail, overflow_n);
    }

#if HZ3_S195_MEDIUM_INBOX_STATS
    atomic_fetch_add_explicit(&g_s195_drain_nonempty, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_drain_objs, drained_objs, memory_order_relaxed);
#endif
#if HZ3_S203_COUNTERS
    t_s203_inbox_drain_nonempty++;
    t_s203_inbox_drain_objs += drained_objs;
    t_s203_inbox_drain_sc_objs[sc] += drained_objs;
#endif
    atomic_fetch_add_explicit(&inbox->drain_seq, 1, memory_order_release);
    hz3_s197_stats_on_drain(sc, drained_objs, 1);

#if HZ3_S201_MEDIUM_INBOX_SPLICE
    #undef HZ3_INBOX_CHAIN_COUNT
#endif

    return first;
}

static inline int hz3_inbox_medium_sc_from_obj(void* obj, int* sc_out) {
    uint32_t page_idx = 0;
    if (hz3_arena_page_index_fast(obj, &page_idx)) {
        uint32_t tag32 = hz3_pagetag32_load(page_idx);
        if (tag32 != 0) {
            uint32_t bin = hz3_pagetag32_bin(tag32);
            if (bin >= (uint32_t)HZ3_MEDIUM_BIN_BASE &&
                bin < (uint32_t)(HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
                *sc_out = (int)(bin - (uint32_t)HZ3_MEDIUM_BIN_BASE);
                return 1;
            }
        }
    }

    Hz3SegMeta* meta = hz3_segmap_get(obj);
    if (!meta) {
        return 0;
    }
    uintptr_t seg_base = (uintptr_t)obj & ~((uintptr_t)HZ3_SEG_SIZE - 1u);
    uint32_t seg_page_idx = (uint32_t)(((uintptr_t)obj - seg_base) >> HZ3_PAGE_SHIFT);
    if (seg_page_idx >= (uint32_t)HZ3_PAGES_PER_SEG) {
        return 0;
    }
    uint16_t tag = meta->sc_tag[seg_page_idx];
    if (hz3_tag_kind(tag) != HZ3_TAG_KIND_SMALL) {
        return 0;
    }
    int sc = hz3_tag_sc(tag);
    if (sc < 0 || sc >= HZ3_NUM_SC) {
        return 0;
    }
    *sc_out = sc;
    return 1;
}

void* hz3_inbox_medium_mixed_drain(uint8_t shard, int sc, Hz3Bin* bin, uint32_t budget_objs) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin || budget_objs == 0) return NULL;

    Hz3InboxMixed* inbox = &g_hz3_inbox_medium_mixed[shard];
    void* first = NULL;
    uint32_t drained_any = 0;

    void* reroute_head[HZ3_NUM_SC];
    void* reroute_tail[HZ3_NUM_SC];
    uint32_t reroute_n[HZ3_NUM_SC];
    memset(reroute_head, 0, sizeof(reroute_head));
    memset(reroute_tail, 0, sizeof(reroute_tail));
    memset(reroute_n, 0, sizeof(reroute_n));

#if HZ3_BIN_LAZY_COUNT && HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY
    uint16_t bin_len = hz3_bin_is_empty(bin) ? 0 : HZ3_BIN_HARD_CAP;
#elif HZ3_BIN_LAZY_COUNT
    uint16_t bin_len = hz3_bin_count_walk(bin, HZ3_BIN_HARD_CAP);
#endif

    for (uint32_t lane = 0; lane < (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS; lane++) {
        void* head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        if (!head) {
            continue;
        }
        head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) {
            continue;
        }
        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        void* obj = head;
        while (obj && budget_objs > 0) {
            void* next = hz3_obj_get_next(obj);
            hz3_obj_set_next(obj, NULL);
            budget_objs--;

            int obj_sc = sc;
            if (!hz3_inbox_medium_sc_from_obj(obj, &obj_sc)) {
#if HZ3_S206B_MIXED_DECODE_FAILFAST
                fprintf(stderr, "[HZ3_S206B_FAILFAST] mixed_decode_fail ptr=%p shard=%u sc=%d\n",
                        obj, (unsigned)shard, sc);
                abort();
#endif
                obj_sc = sc;
            }

            if (obj_sc == sc) {
                if (!first) {
                    first = obj;
                } else {
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
                        if (!reroute_head[sc]) {
                            reroute_head[sc] = obj;
                        } else {
                            hz3_obj_set_next(reroute_tail[sc], obj);
                        }
                        reroute_tail[sc] = obj;
                        reroute_n[sc]++;
                    }
                }
            } else {
                if (!reroute_head[obj_sc]) {
                    reroute_head[obj_sc] = obj;
                } else {
                    hz3_obj_set_next(reroute_tail[obj_sc], obj);
                }
                reroute_tail[obj_sc] = obj;
                reroute_n[obj_sc]++;
            }
            drained_any++;
            obj = next;
        }

        if (obj) {
            void* rem_head = obj;
            void* rem_tail = obj;
            uint32_t rem_n = 1;
            while ((obj = hz3_obj_get_next(obj)) != NULL) {
                rem_tail = obj;
                rem_n++;
            }
            hz3_inbox_medium_mixed_push_list(shard, rem_head, rem_tail, rem_n);
            break;
        }
    }

    for (int s = 0; s < HZ3_NUM_SC; s++) {
        if (!reroute_head[s]) {
            continue;
        }
        if (s == sc) {
            hz3_central_push_list(shard, s, reroute_head[s], reroute_tail[s], reroute_n[s]);
        } else {
            hz3_inbox_push_list(shard, s, reroute_head[s], reroute_tail[s], reroute_n[s]);
        }
    }

    if (drained_any > 0) {
        atomic_fetch_add_explicit(&inbox->drain_seq, 1, memory_order_release);
    }
    return first;
}

uint32_t hz3_inbox_drain_to_central(uint8_t shard, int sc) {
    if (shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t total_n = 0;

    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        void* head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) {
            continue;
        }

        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        void* tail = head;
        uint32_t n = 1;
        void* obj = hz3_obj_get_next(head);
        while (obj) {
            tail = obj;
            n++;
            obj = hz3_obj_get_next(obj);
        }

        hz3_central_push_list(shard, sc, head, tail, n);
        total_n += n;
    }

    return total_n;
}
