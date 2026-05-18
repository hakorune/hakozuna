#define _GNU_SOURCE

#include "hz3_central.h"
#include "hz3_arena.h"
#include "hz3_tcache.h"
#include "hz3_central_shadow.h"
#include "hz3_platform.h"
#include "hz3_sc.h"

#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

// Global central pool (zero-initialized)
Hz3CentralBin g_hz3_central[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#if HZ3_S65_CENTRAL_COLD_ENABLE
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
typedef enum Hz3CentralColdState {
    HZ3_CENTRAL_COLD_STATE_READY = 1,
    HZ3_CENTRAL_COLD_STATE_DECOMMITTED = 2,
    HZ3_CENTRAL_COLD_STATE_RECOMMITTING = 3,
    HZ3_CENTRAL_COLD_STATE_DECOMMITTING = 4
} Hz3CentralColdState;

typedef struct Hz3CentralColdNode {
    void* run;
    struct Hz3CentralColdNode* next;
    struct Hz3CentralColdNode* prev;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    uint8_t state;
#endif
} Hz3CentralColdNode;

typedef struct {
    hz3_lock_t lock;
    Hz3CentralColdNode* ready_head;
    Hz3CentralColdNode* ready_tail;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    Hz3CentralColdNode* decommitted_head;
    uint32_t ready_count;
    uint32_t decommitted_count;
    uint32_t inflight_decommitting;
    uint32_t inflight_recommitting;
#endif
    uint32_t count;
} Hz3CentralColdExternalBin;

static Hz3CentralColdExternalBin g_hz3_central_cold_ext[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static Hz3CentralColdNode g_hz3_cold_nodes[HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP];
static Hz3CentralColdNode* g_hz3_cold_free_nodes;
static hz3_lock_t g_hz3_cold_node_lock;
#else
static Hz3CentralBin g_hz3_central_cold[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#endif
#endif

#if HZ3_S65_STATS
#if defined(__GNUC__) || defined(__clang__)
#define HZ3_S65_MAYBE_UNUSED __attribute__((unused))
#else
#define HZ3_S65_MAYBE_UNUSED
#endif

static _Atomic(uint64_t) g_s65_central_push_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_central_pop_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_central_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_central_peak_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_local_rem_add_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_local_rem_pop_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_local_rem_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_local_rem_peak_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_xfer_push_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_xfer_pop_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_xfer_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_xfer_peak_objs[HZ3_NUM_SC];
#if HZ3_S65_CENTRAL_COLD_ENABLE
static _Atomic(uint64_t) g_s65_cold_push_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_pop_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_peak_objs[HZ3_NUM_SC];
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
static _Atomic(uint64_t) g_s65_cold_ext_node_inuse;
static _Atomic(uint64_t) g_s65_cold_ext_node_peak;
static _Atomic(uint64_t) g_s65_cold_ext_node_alloc_fail;
static _Atomic(uint64_t) g_s65_cold_ext_fallback_hot;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
static _Atomic(uint64_t) g_s65_cold_decommit_calls;
static _Atomic(uint64_t) g_s65_cold_decommit_ok;
static _Atomic(uint64_t) g_s65_cold_decommit_fail;
static _Atomic(uint64_t) g_s65_cold_decommit_skip_policy;
static _Atomic(uint64_t) g_s65_cold_decommit_bytes;
static _Atomic(uint64_t) g_s65_cold_decommit_ns_total;
static _Atomic(uint64_t) g_s65_cold_decommit_ns_max;
static _Atomic(uint64_t) g_s65_cold_recommit_calls;
static _Atomic(uint64_t) g_s65_cold_recommit_ok;
static _Atomic(uint64_t) g_s65_cold_recommit_fail;
static _Atomic(uint64_t) g_s65_cold_recommit_bytes;
static _Atomic(uint64_t) g_s65_cold_recommit_ns_total;
static _Atomic(uint64_t) g_s65_cold_recommit_ns_max;
static _Atomic(uint64_t) g_s65_cold_rewarm_batches;
static _Atomic(uint64_t) g_s65_cold_rewarm_nodes;
static _Atomic(uint64_t) g_s65_cold_unlock_batches;
static _Atomic(uint64_t) g_s65_cold_unlock_nodes;
static _Atomic(uint64_t) g_s65_cold_coalesce_input_nodes;
static _Atomic(uint64_t) g_s65_cold_coalesce_ranges;
static _Atomic(uint64_t) g_s65_cold_coalesce_saved_syscalls;
static _Atomic(uint64_t) g_s65_cold_coalesce_max_range_bytes;
static _Atomic(uint64_t) g_s65_cold_decommit_coalesce_batches;
static _Atomic(uint64_t) g_s65_cold_decommit_coalesce_input_nodes;
static _Atomic(uint64_t) g_s65_cold_decommit_coalesce_ranges;
static _Atomic(uint64_t) g_s65_cold_decommit_coalesce_saved_syscalls;
static _Atomic(uint64_t) g_s65_cold_decommit_coalesce_max_range_bytes;
static _Atomic(uint64_t) g_s65_cold_committed_push_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_committed_pop_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_committed_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_committed_peak_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_decommitted_push_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_decommitted_pop_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_decommitted_current_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_decommitted_peak_objs[HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_cold_inflight_decommitting_current;
static _Atomic(uint64_t) g_s65_cold_inflight_decommitting_peak;
static _Atomic(uint64_t) g_s65_cold_inflight_recommitting_current;
static _Atomic(uint64_t) g_s65_cold_inflight_recommitting_peak;
static _Atomic(uint64_t) g_s65_cold_state_mismatch;
static _Atomic(uint64_t) g_s65_cold_ready_pop_calls;
static _Atomic(uint64_t) g_s65_cold_ready_pop_hits;
static _Atomic(uint64_t) g_s65_cold_ready_pop_misses;
static _Atomic(uint64_t) g_s65_cold_ready_pop_objs;
#endif
#endif
#endif
static hz3_once_t g_s65_central_stats_once = HZ3_ONCE_INIT;

static void hz3_s65_stats_update_peak(_Atomic(uint64_t)* peak, uint64_t value) {
    uint64_t old = atomic_load_explicit(peak, memory_order_relaxed);
    while (old < value &&
           !atomic_compare_exchange_weak_explicit(peak, &old, value,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
    }
}

static void hz3_s65_stats_add_current(_Atomic(uint64_t)* current,
                                      _Atomic(uint64_t)* peak,
                                      uint64_t n) {
    if (n == 0) {
        return;
    }
    uint64_t after = atomic_fetch_add_explicit(current, n, memory_order_relaxed) + n;
    hz3_s65_stats_update_peak(peak, after);
}

static void hz3_s65_stats_sub_current(_Atomic(uint64_t)* current, uint64_t n) {
    if (n == 0) {
        return;
    }
    uint64_t old = atomic_load_explicit(current, memory_order_relaxed);
    for (;;) {
        uint64_t next = old > n ? old - n : 0;
        if (atomic_compare_exchange_weak_explicit(current, &old, next,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return;
        }
    }
}

#if HZ3_S65_CENTRAL_COLD_ENABLE && HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
static void hz3_s65_cold_ext_note_node_alloc(void) {
    uint64_t after = atomic_fetch_add_explicit(&g_s65_cold_ext_node_inuse,
                                               1,
                                               memory_order_relaxed) + 1;
    hz3_s65_stats_update_peak(&g_s65_cold_ext_node_peak, after);
}

static void hz3_s65_cold_ext_note_node_free(void) {
    hz3_s65_stats_sub_current(&g_s65_cold_ext_node_inuse, 1);
}

static void hz3_s65_cold_ext_note_alloc_fail(void) {
    atomic_fetch_add_explicit(&g_s65_cold_ext_node_alloc_fail, 1, memory_order_relaxed);
}

static void hz3_s65_cold_ext_note_fallback_hot(uint64_t n) {
    if (n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_ext_fallback_hot, n, memory_order_relaxed);
}

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
static void hz3_s65_cold_note_decommit(int ok, size_t bytes, uint64_t elapsed_ns) {
    atomic_fetch_add_explicit(&g_s65_cold_decommit_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(ok ? &g_s65_cold_decommit_ok : &g_s65_cold_decommit_fail,
                              1,
                              memory_order_relaxed);
    if (ok) {
        atomic_fetch_add_explicit(&g_s65_cold_decommit_bytes,
                                  (uint64_t)bytes,
                                  memory_order_relaxed);
    }
    atomic_fetch_add_explicit(&g_s65_cold_decommit_ns_total,
                              elapsed_ns,
                              memory_order_relaxed);
    hz3_s65_stats_update_peak(&g_s65_cold_decommit_ns_max, elapsed_ns);
}

#if !HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static void hz3_s65_cold_note_decommit_skip_policy(void) {
    atomic_fetch_add_explicit(&g_s65_cold_decommit_skip_policy, 1, memory_order_relaxed);
}
#else
#define hz3_s65_cold_note_decommit_skip_policy() do { } while (0)
#endif

static void hz3_s65_cold_note_recommit(int ok, size_t bytes, uint64_t elapsed_ns) {
    atomic_fetch_add_explicit(&g_s65_cold_recommit_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(ok ? &g_s65_cold_recommit_ok : &g_s65_cold_recommit_fail,
                              1,
                              memory_order_relaxed);
    if (ok) {
        atomic_fetch_add_explicit(&g_s65_cold_recommit_bytes,
                                  (uint64_t)bytes,
                                  memory_order_relaxed);
    }
    atomic_fetch_add_explicit(&g_s65_cold_recommit_ns_total,
                              elapsed_ns,
                              memory_order_relaxed);
    hz3_s65_stats_update_peak(&g_s65_cold_recommit_ns_max, elapsed_ns);
}

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
static void hz3_s65_cold_note_rewarm(uint64_t n) {
    if (n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_rewarm_batches, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_rewarm_nodes, n, memory_order_relaxed);
}
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK || HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static void hz3_s65_cold_note_unlock_recommit(uint64_t n) {
    if (n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_unlock_batches, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_unlock_nodes, n, memory_order_relaxed);
}
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
static void hz3_s65_cold_note_recommit_coalesce(uint64_t input_nodes,
                                                uint64_t ranges,
                                                uint64_t saved_syscalls,
                                                uint64_t max_range_bytes) {
    if (input_nodes == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_coalesce_input_nodes,
                              input_nodes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_coalesce_ranges,
                              ranges,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_coalesce_saved_syscalls,
                              saved_syscalls,
                              memory_order_relaxed);
    hz3_s65_stats_update_peak(&g_s65_cold_coalesce_max_range_bytes,
                              max_range_bytes);
}
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE || \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
static void hz3_s65_cold_note_decommit_coalesce_observe(uint64_t input_nodes,
                                                        uint64_t ranges,
                                                        uint64_t saved_syscalls,
                                                        uint64_t max_range_bytes) {
    if (input_nodes == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_decommit_coalesce_batches,
                              1,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_decommit_coalesce_input_nodes,
                              input_nodes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_decommit_coalesce_ranges,
                              ranges,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_cold_decommit_coalesce_saved_syscalls,
                              saved_syscalls,
                              memory_order_relaxed);
    hz3_s65_stats_update_peak(&g_s65_cold_decommit_coalesce_max_range_bytes,
                              max_range_bytes);
}
#endif

static void hz3_s65_cold_note_committed_push(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_committed_push_objs[sc],
                              n,
                              memory_order_relaxed);
    hz3_s65_stats_add_current(&g_s65_cold_committed_current_objs[sc],
                              &g_s65_cold_committed_peak_objs[sc],
                              n);
}

static void hz3_s65_cold_note_committed_pop(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_committed_pop_objs[sc],
                              n,
                              memory_order_relaxed);
    hz3_s65_stats_sub_current(&g_s65_cold_committed_current_objs[sc], n);
}

static void hz3_s65_cold_note_decommitted_push(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_decommitted_push_objs[sc],
                              n,
                              memory_order_relaxed);
    hz3_s65_stats_add_current(&g_s65_cold_decommitted_current_objs[sc],
                              &g_s65_cold_decommitted_peak_objs[sc],
                              n);
}

static void hz3_s65_cold_note_decommitted_pop(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_decommitted_pop_objs[sc],
                              n,
                              memory_order_relaxed);
    hz3_s65_stats_sub_current(&g_s65_cold_decommitted_current_objs[sc], n);
}

static void hz3_s65_cold_note_inflight_decommitting_add(uint64_t n) {
    hz3_s65_stats_add_current(&g_s65_cold_inflight_decommitting_current,
                              &g_s65_cold_inflight_decommitting_peak,
                              n);
}

static void hz3_s65_cold_note_inflight_decommitting_sub(uint64_t n) {
    hz3_s65_stats_sub_current(&g_s65_cold_inflight_decommitting_current, n);
}

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK || HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static void hz3_s65_cold_note_inflight_recommitting_add(uint64_t n) {
    hz3_s65_stats_add_current(&g_s65_cold_inflight_recommitting_current,
                              &g_s65_cold_inflight_recommitting_peak,
                              n);
}

static void hz3_s65_cold_note_inflight_recommitting_sub(uint64_t n) {
    hz3_s65_stats_sub_current(&g_s65_cold_inflight_recommitting_current, n);
}
#else
#define hz3_s65_cold_note_inflight_recommitting_add(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_inflight_recommitting_sub(n) do { (void)(n); } while (0)
#endif

static void hz3_s65_cold_note_state_mismatch(void) {
    atomic_fetch_add_explicit(&g_s65_cold_state_mismatch, 1, memory_order_relaxed);
}

static void hz3_s65_cold_note_ready_pop_attempt(uint64_t got) {
    atomic_fetch_add_explicit(&g_s65_cold_ready_pop_calls, 1, memory_order_relaxed);
    if (got > 0) {
        atomic_fetch_add_explicit(&g_s65_cold_ready_pop_hits, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s65_cold_ready_pop_objs, got, memory_order_relaxed);
    } else {
        atomic_fetch_add_explicit(&g_s65_cold_ready_pop_misses, 1, memory_order_relaxed);
    }
}
#endif
#endif

static void hz3_s65_central_note_push(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_central_push_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_add_current(&g_s65_central_current_objs[sc],
                              &g_s65_central_peak_objs[sc],
                              n);
}

static void hz3_s65_central_note_pop(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_central_pop_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_sub_current(&g_s65_central_current_objs[sc], n);
}

static HZ3_S65_MAYBE_UNUSED void hz3_s65_local_rem_note_add(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_local_rem_add_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_add_current(&g_s65_local_rem_current_objs[sc],
                              &g_s65_local_rem_peak_objs[sc],
                              n);
}

static HZ3_S65_MAYBE_UNUSED void hz3_s65_local_rem_note_pop(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_local_rem_pop_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_sub_current(&g_s65_local_rem_current_objs[sc], n);
}

static HZ3_S65_MAYBE_UNUSED void hz3_s65_xfer_note_push(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_xfer_push_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_add_current(&g_s65_xfer_current_objs[sc],
                              &g_s65_xfer_peak_objs[sc],
                              n);
}

static HZ3_S65_MAYBE_UNUSED void hz3_s65_xfer_note_pop(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_xfer_pop_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_sub_current(&g_s65_xfer_current_objs[sc], n);
}

#if HZ3_S65_CENTRAL_COLD_ENABLE
static void hz3_s65_cold_note_push(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_push_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_add_current(&g_s65_cold_current_objs[sc],
                              &g_s65_cold_peak_objs[sc],
                              n);
}

static void hz3_s65_cold_note_pop(int sc, uint64_t n) {
    if (sc < 0 || sc >= HZ3_NUM_SC || n == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s65_cold_pop_objs[sc], n, memory_order_relaxed);
    hz3_s65_stats_sub_current(&g_s65_cold_current_objs[sc], n);
}
#endif

static uint64_t hz3_s65_sc_bytes(int sc, uint64_t objs) {
    return objs * (uint64_t)hz3_sc_to_size(sc);
}

static void hz3_s65_dump_retention_group(const char* name,
                                         _Atomic(uint64_t)* pushed,
                                         _Atomic(uint64_t)* popped,
                                         _Atomic(uint64_t)* current,
                                         _Atomic(uint64_t)* peak) {
    uint64_t total_push = 0;
    uint64_t total_pop = 0;
    uint64_t total_current = 0;
    uint64_t total_peak = 0;
    uint64_t total_current_bytes = 0;
    uint64_t total_peak_bytes = 0;

    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t push = atomic_load_explicit(&pushed[sc], memory_order_relaxed);
        uint64_t pop = atomic_load_explicit(&popped[sc], memory_order_relaxed);
        uint64_t cur = atomic_load_explicit(&current[sc], memory_order_relaxed);
        uint64_t pk = atomic_load_explicit(&peak[sc], memory_order_relaxed);
        total_push += push;
        total_pop += pop;
        total_current += cur;
        total_peak += pk;
        total_current_bytes += hz3_s65_sc_bytes(sc, cur);
        total_peak_bytes += hz3_s65_sc_bytes(sc, pk);
    }

    if (total_push == 0 && total_pop == 0 && total_current == 0 && total_peak == 0) {
        return;
    }

    fprintf(stderr,
            "[HZ3_S65_%s] push=%" PRIu64 " pop=%" PRIu64
            " current=%" PRIu64 " peak=%" PRIu64
            " current_bytes=%" PRIu64 " peak_bytes=%" PRIu64 "\n",
            name,
            total_push,
            total_pop,
            total_current,
            total_peak,
            total_current_bytes,
            total_peak_bytes);
    fprintf(stderr, "  by_sc:");
    int printed = 0;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t push = atomic_load_explicit(&pushed[sc], memory_order_relaxed);
        uint64_t pop = atomic_load_explicit(&popped[sc], memory_order_relaxed);
        uint64_t cur = atomic_load_explicit(&current[sc], memory_order_relaxed);
        uint64_t pk = atomic_load_explicit(&peak[sc], memory_order_relaxed);
        if (push > 0 || pop > 0 || cur > 0 || pk > 0) {
            fprintf(stderr,
                    " sc%d=%" PRIu64 "p/%" PRIu64 "o/%" PRIu64 "c/%" PRIu64 "k",
                    sc,
                    push,
                    pop,
                    cur,
                    pk);
            printed = 1;
        }
    }
    if (!printed) {
        fprintf(stderr, " none");
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "  bytes_by_sc:");
    printed = 0;
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
        uint64_t cur = atomic_load_explicit(&current[sc], memory_order_relaxed);
        uint64_t pk = atomic_load_explicit(&peak[sc], memory_order_relaxed);
        if (cur > 0 || pk > 0) {
            fprintf(stderr,
                    " sc%d=%" PRIu64 "cb/%" PRIu64 "kb",
                    sc,
                    hz3_s65_sc_bytes(sc, cur),
                    hz3_s65_sc_bytes(sc, pk));
            printed = 1;
        }
    }
    if (!printed) {
        fprintf(stderr, " none");
    }
    fprintf(stderr, "\n");
}

static void hz3_s65_central_retention_dump(void) {
    hz3_s65_dump_retention_group("CENTRAL",
                                 g_s65_central_push_objs,
                                 g_s65_central_pop_objs,
                                 g_s65_central_current_objs,
                                 g_s65_central_peak_objs);
    hz3_s65_dump_retention_group("LOCAL_REM",
                                 g_s65_local_rem_add_objs,
                                 g_s65_local_rem_pop_objs,
                                 g_s65_local_rem_current_objs,
                                 g_s65_local_rem_peak_objs);
    hz3_s65_dump_retention_group("XFER",
                                 g_s65_xfer_push_objs,
                                 g_s65_xfer_pop_objs,
                                 g_s65_xfer_current_objs,
                                 g_s65_xfer_peak_objs);
#if HZ3_S65_CENTRAL_COLD_ENABLE
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_POLICY] park_purged=%d pop_on_miss=%d hot_reserve_runs=%d external_list=%d node_cap=%d\n",
            HZ3_S65_CENTRAL_COLD_PARK_PURGED,
            HZ3_S65_CENTRAL_COLD_POP_ON_MISS,
            HZ3_S65_CENTRAL_COLD_HOT_RESERVE_RUNS,
            HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE,
            HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP);
    hz3_s65_dump_retention_group("CENTRAL_COLD",
                                 g_s65_cold_push_objs,
                                 g_s65_cold_pop_objs,
                                 g_s65_cold_current_objs,
                                 g_s65_cold_peak_objs);
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_EXTERNAL] node_inuse=%" PRIu64
            " node_peak=%" PRIu64 " node_alloc_fail=%" PRIu64
            " fallback_hot=%" PRIu64 " decommit=%d\n",
            atomic_load_explicit(&g_s65_cold_ext_node_inuse, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ext_node_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ext_node_alloc_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ext_fallback_hot, memory_order_relaxed),
            HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    hz3_s65_dump_retention_group("CENTRAL_COLD_READY",
                                 g_s65_cold_committed_push_objs,
                                 g_s65_cold_committed_pop_objs,
                                 g_s65_cold_committed_current_objs,
                                 g_s65_cold_committed_peak_objs);
    hz3_s65_dump_retention_group("CENTRAL_COLD_DECOMMITTED",
                                 g_s65_cold_decommitted_push_objs,
                                 g_s65_cold_decommitted_pop_objs,
                                 g_s65_cold_decommitted_current_objs,
                                 g_s65_cold_decommitted_peak_objs);
    uint64_t decommit_calls = atomic_load_explicit(&g_s65_cold_decommit_calls,
                                                   memory_order_relaxed);
    uint64_t recommit_calls = atomic_load_explicit(&g_s65_cold_recommit_calls,
                                                   memory_order_relaxed);
    uint64_t decommit_ns_total = atomic_load_explicit(&g_s65_cold_decommit_ns_total,
                                                      memory_order_relaxed);
    uint64_t recommit_ns_total = atomic_load_explicit(&g_s65_cold_recommit_ns_total,
                                                      memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_DECOMMIT] decommit_calls=%" PRIu64
            " decommit_ok=%" PRIu64 " decommit_fail=%" PRIu64
            " decommit_skip_policy=%" PRIu64
            " decommit_bytes=%" PRIu64
            " decommit_ns_total=%" PRIu64 " decommit_ns_avg=%" PRIu64
            " decommit_ns_max=%" PRIu64
            " recommit_calls=%" PRIu64 " recommit_ok=%" PRIu64
            " recommit_fail=%" PRIu64
            " recommit_bytes=%" PRIu64
            " recommit_ns_total=%" PRIu64 " recommit_ns_avg=%" PRIu64
            " recommit_ns_max=%" PRIu64
            " rewarm_batches=%" PRIu64 " rewarm_nodes=%" PRIu64
            " unlock_batches=%" PRIu64 " unlock_nodes=%" PRIu64
            " min_pages=%d committed_reserve=%d recommit_batch=%d"
            " recommit_batch_min_pages=%d small_reserve=%d"
            " small_reserve_max_pages=%d small_rewarm_min_pages=%d"
            " small_rewarm_max_pages=%d ready_mru=%d ready_target=%d"
            " small_ready_target=%d large_ready_target=%d large_min_pages=%d"
            " recommit_failfast=%d\n",
            decommit_calls,
            atomic_load_explicit(&g_s65_cold_decommit_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_decommit_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_decommit_skip_policy, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_decommit_bytes, memory_order_relaxed),
            decommit_ns_total,
            decommit_calls ? decommit_ns_total / decommit_calls : 0,
            atomic_load_explicit(&g_s65_cold_decommit_ns_max, memory_order_relaxed),
            recommit_calls,
            atomic_load_explicit(&g_s65_cold_recommit_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_recommit_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_recommit_bytes, memory_order_relaxed),
            recommit_ns_total,
            recommit_calls ? recommit_ns_total / recommit_calls : 0,
            atomic_load_explicit(&g_s65_cold_recommit_ns_max, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_rewarm_batches, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_rewarm_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_unlock_batches, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_unlock_nodes, memory_order_relaxed),
            HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS,
            HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS,
            HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES,
            HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES,
            HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES,
            HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE,
            HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS,
            HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS,
            HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS,
            HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES,
            HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST);
    uint64_t coalesce_input =
        atomic_load_explicit(&g_s65_cold_coalesce_input_nodes,
                             memory_order_relaxed);
    uint64_t coalesce_ranges =
        atomic_load_explicit(&g_s65_cold_coalesce_ranges,
                             memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE] enabled=%d"
            " input_nodes=%" PRIu64
            " ranges=%" PRIu64
            " saved_syscalls=%" PRIu64
            " max_range_bytes=%" PRIu64
            " avg_nodes_per_range_x100=%" PRIu64 "\n",
            HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE,
            coalesce_input,
            coalesce_ranges,
            atomic_load_explicit(&g_s65_cold_coalesce_saved_syscalls,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_coalesce_max_range_bytes,
                                 memory_order_relaxed),
            coalesce_ranges ? (coalesce_input * 100) / coalesce_ranges : 0);
    uint64_t decommit_coalesce_input =
        atomic_load_explicit(&g_s65_cold_decommit_coalesce_input_nodes,
                             memory_order_relaxed);
    uint64_t decommit_coalesce_ranges =
        atomic_load_explicit(&g_s65_cold_decommit_coalesce_ranges,
                             memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE] enabled=%d"
            " observe=%d"
            " batch_runs=%d"
            " observe_batch_runs=%d"
            " batches=%" PRIu64
            " input_nodes=%" PRIu64
            " ranges=%" PRIu64
            " saved_syscalls=%" PRIu64
            " max_range_bytes=%" PRIu64
            " avg_nodes_per_range_x100=%" PRIu64 "\n",
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS,
            atomic_load_explicit(&g_s65_cold_decommit_coalesce_batches,
                                 memory_order_relaxed),
            decommit_coalesce_input,
            decommit_coalesce_ranges,
            atomic_load_explicit(&g_s65_cold_decommit_coalesce_saved_syscalls,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_decommit_coalesce_max_range_bytes,
                                 memory_order_relaxed),
            decommit_coalesce_ranges
                ? (decommit_coalesce_input * 100) / decommit_coalesce_ranges
                : 0);
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_STATE] inflight_decommitting_current=%" PRIu64
            " inflight_decommitting_peak=%" PRIu64
            " inflight_recommitting_current=%" PRIu64
            " inflight_recommitting_peak=%" PRIu64
            " state_mismatch=%" PRIu64
            " ready_pop_calls=%" PRIu64
            " ready_pop_hits=%" PRIu64
            " ready_pop_misses=%" PRIu64
            " ready_pop_objs=%" PRIu64 "\n",
            atomic_load_explicit(&g_s65_cold_inflight_decommitting_current,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_inflight_decommitting_peak,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_inflight_recommitting_current,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_inflight_recommitting_peak,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_state_mismatch,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ready_pop_calls,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ready_pop_hits,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ready_pop_misses,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ready_pop_objs,
                                 memory_order_relaxed));
#endif
#endif
#endif
}

static void hz3_s65_central_retention_register_once(void) {
    atexit(hz3_s65_central_retention_dump);
}
#else
#define hz3_s65_central_note_push(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_central_note_pop(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_local_rem_note_add(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_local_rem_note_pop(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_xfer_note_push(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_xfer_note_pop(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_note_push(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_note_pop(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_ext_note_node_alloc() do { } while (0)
#define hz3_s65_cold_ext_note_node_free() do { } while (0)
#define hz3_s65_cold_ext_note_alloc_fail() do { } while (0)
#define hz3_s65_cold_ext_note_fallback_hot(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_decommit(ok, bytes, elapsed_ns) do { (void)(ok); (void)(bytes); (void)(elapsed_ns); } while (0)
#define hz3_s65_cold_note_decommit_skip_policy() do { } while (0)
#define hz3_s65_cold_note_recommit(ok, bytes, elapsed_ns) do { (void)(ok); (void)(bytes); (void)(elapsed_ns); } while (0)
#define hz3_s65_cold_note_rewarm(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_unlock_recommit(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_decommit_coalesce_observe(input_nodes, ranges, saved_syscalls, max_range_bytes) do { (void)(input_nodes); (void)(ranges); (void)(saved_syscalls); (void)(max_range_bytes); } while (0)
#define hz3_s65_cold_note_committed_push(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_note_committed_pop(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_note_decommitted_push(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_note_decommitted_pop(sc, n) do { (void)(sc); (void)(n); } while (0)
#define hz3_s65_cold_note_inflight_decommitting_add(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_inflight_decommitting_sub(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_inflight_recommitting_add(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_inflight_recommitting_sub(n) do { (void)(n); } while (0)
#define hz3_s65_cold_note_state_mismatch() do { } while (0)
#define hz3_s65_cold_note_ready_pop_attempt(got) do { (void)(got); } while (0)
#endif

#if HZ3_S65_CENTRAL_COLD_ENABLE && HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
static void hz3_central_cold_external_pool_init(void) {
    hz3_lock_init(&g_hz3_cold_node_lock);
    g_hz3_cold_free_nodes = NULL;

    for (uint32_t i = 0; i < (uint32_t)HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP; i++) {
        g_hz3_cold_nodes[i].run = NULL;
        g_hz3_cold_nodes[i].prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
        g_hz3_cold_nodes[i].state = 0;
#endif
        g_hz3_cold_nodes[i].next = g_hz3_cold_free_nodes;
        g_hz3_cold_free_nodes = &g_hz3_cold_nodes[i];
    }
}

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#if !HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static uint32_t hz3_central_cold_committed_reserve_runs(int sc) {
    uint32_t pages = hz3_sc_to_pages(sc);
    if (pages <= HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES) {
        return (uint32_t)HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS;
    }
    return (uint32_t)HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS;
}

static int hz3_central_cold_should_decommit_locked(Hz3CentralColdExternalBin* bin, int sc) {
    if (hz3_sc_to_pages(sc) < HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES) {
        hz3_s65_cold_note_decommit_skip_policy();
        return 0;
    }
    if (bin->ready_count < hz3_central_cold_committed_reserve_runs(sc)) {
        hz3_s65_cold_note_decommit_skip_policy();
        return 0;
    }
    return 1;
}
#endif

static size_t hz3_central_cold_run_bytes(int sc) {
    return (size_t)hz3_sc_to_pages(sc) << HZ3_PAGE_SHIFT;
}

static uint32_t hz3_central_cold_ready_target_runs(int sc) {
#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
    uint32_t pages = hz3_sc_to_pages(sc);
    if (pages >= HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES) {
        return (uint32_t)HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS;
    }
    if (pages <= HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES) {
        return (uint32_t)HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS;
    }
    return (uint32_t)HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS;
#else
    return hz3_central_cold_committed_reserve_runs(sc);
#endif
}

static int hz3_central_cold_node_is_decommitted(const Hz3CentralColdNode* node) {
    return node && node->state == HZ3_CENTRAL_COLD_STATE_DECOMMITTED;
}

static void hz3_central_cold_note_state_or_mismatch(Hz3CentralColdNode* node,
                                                    Hz3CentralColdState expected) {
    if (!node || node->state != (uint8_t)expected) {
        hz3_s65_cold_note_state_mismatch();
    }
}

#if defined(_WIN32)
static hz3_once_t g_hz3_cold_qpc_once = HZ3_ONCE_INIT;
static int64_t g_hz3_cold_qpc_freq = 0;

static void hz3_central_cold_qpc_init(void) {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
        g_hz3_cold_qpc_freq = freq.QuadPart;
    }
}
#endif

static uint64_t hz3_central_cold_now_ns(void) {
#if defined(_WIN32)
    hz3_once(&g_hz3_cold_qpc_once, hz3_central_cold_qpc_init);
    if (g_hz3_cold_qpc_freq <= 0) {
        return 0;
    }
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }
    return (uint64_t)(((double)counter.QuadPart * 1000000000.0) /
                      (double)g_hz3_cold_qpc_freq);
#else
    return 0;
#endif
}

static int hz3_central_cold_decommit_run(void* run, size_t bytes) {
    if (!run || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    int ok = VirtualFree(run, bytes, MEM_DECOMMIT) ? 1 : 0;
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_decommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_decommit(1, bytes, 0);
    return 0;
#endif
}

static int hz3_central_cold_recommit_run(void* run, size_t bytes) {
    if (!run || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    void* p = VirtualAlloc(run, bytes, MEM_COMMIT, PAGE_READWRITE);
    int ok = (p == run);
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_recommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_recommit(1, bytes, 0);
    return 0;
#endif
}

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE
typedef struct Hz3CentralColdDecommitObserveBatch {
    Hz3CentralColdNode* nodes[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS];
    uint32_t count;
} Hz3CentralColdDecommitObserveBatch;

static void hz3_central_cold_decommit_observe_sort(Hz3CentralColdNode** nodes,
                                                   uint32_t n) {
    for (uint32_t i = 1; i < n; i++) {
        Hz3CentralColdNode* cur = nodes[i];
        uintptr_t cur_addr = (uintptr_t)cur->run;
        uint32_t j = i;
        while (j > 0 && (uintptr_t)nodes[j - 1]->run > cur_addr) {
            nodes[j] = nodes[j - 1];
            j--;
        }
        nodes[j] = cur;
    }
}

static void hz3_central_cold_decommit_observe_flush(
    Hz3CentralColdDecommitObserveBatch* batch,
    int sc) {
    if (!batch || batch->count == 0) {
        return;
    }

    Hz3CentralColdNode* sorted[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS];
    uint32_t n = batch->count;
    for (uint32_t i = 0; i < n; i++) {
        sorted[i] = batch->nodes[i];
    }
    batch->count = 0;

    hz3_central_cold_decommit_observe_sort(sorted, n);

    size_t run_bytes = hz3_central_cold_run_bytes(sc);
    uint64_t ranges = 0;
    uint64_t saved_syscalls = 0;
    size_t max_range_bytes = 0;
    uint32_t i = 0;
    while (i < n) {
        uintptr_t base = (uintptr_t)sorted[i]->run;
        uintptr_t end = base + run_bytes;
        uint32_t range_nodes = 1;
        i++;
        while (i < n && (uintptr_t)sorted[i]->run == end) {
            end += run_bytes;
            range_nodes++;
            i++;
        }

        size_t range_bytes = (size_t)(end - base);
        if (range_bytes > max_range_bytes) {
            max_range_bytes = range_bytes;
        }
        ranges++;
        saved_syscalls += (uint64_t)(range_nodes - 1);
    }

    hz3_s65_cold_note_decommit_coalesce_observe(n,
                                                ranges,
                                                saved_syscalls,
                                                (uint64_t)max_range_bytes);
}

static void hz3_central_cold_decommit_observe_note(
    Hz3CentralColdDecommitObserveBatch* batch,
    Hz3CentralColdNode* node,
    int sc) {
    if (!batch || !node || !node->run) {
        return;
    }
    batch->nodes[batch->count++] = node;
    if (batch->count >= HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS) {
        hz3_central_cold_decommit_observe_flush(batch, sc);
    }
}
#define HZ3_COLD_DECOMMIT_OBS_DECL(name) \
    Hz3CentralColdDecommitObserveBatch name = { { 0 }, 0 }
#define HZ3_COLD_DECOMMIT_OBS_NOTE(name, node, sc) \
    hz3_central_cold_decommit_observe_note(&(name), (node), (sc))
#define HZ3_COLD_DECOMMIT_OBS_FLUSH(name, sc) \
    hz3_central_cold_decommit_observe_flush(&(name), (sc))
#else
#define HZ3_COLD_DECOMMIT_OBS_DECL(name) do { } while (0)
#define HZ3_COLD_DECOMMIT_OBS_NOTE(name, node, sc) do { (void)(node); } while (0)
#define HZ3_COLD_DECOMMIT_OBS_FLUSH(name, sc) do { } while (0)
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
static int hz3_central_cold_decommit_range(void* base, size_t bytes) {
    if (!base || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    int ok = VirtualFree(base, bytes, MEM_DECOMMIT) ? 1 : 0;
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_decommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_decommit(1, bytes, 0);
    return 0;
#endif
}

static void hz3_central_cold_decommit_sort_nodes(Hz3CentralColdNode** nodes,
                                                 uint32_t n) {
    for (uint32_t i = 1; i < n; i++) {
        Hz3CentralColdNode* cur = nodes[i];
        uintptr_t cur_addr = (uintptr_t)cur->run;
        uint32_t j = i;
        while (j > 0 && (uintptr_t)nodes[j - 1]->run > cur_addr) {
            nodes[j] = nodes[j - 1];
            j--;
        }
        nodes[j] = cur;
    }
}

static void hz3_central_cold_mark_node_decommit_ok(Hz3CentralColdNode** nodes,
                                                   uint8_t* decommit_ok,
                                                   uint32_t n,
                                                   Hz3CentralColdNode* target,
                                                   int ok) {
    for (uint32_t i = 0; i < n; i++) {
        if (nodes[i] == target) {
            decommit_ok[i] = (uint8_t)ok;
            return;
        }
    }
}

static void hz3_central_cold_decommit_nodes_coalesced(Hz3CentralColdNode** nodes,
                                                      uint8_t* decommit_ok,
                                                      uint32_t n,
                                                      int sc) {
    if (n == 0) {
        return;
    }

    Hz3CentralColdNode* sorted[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS];
    for (uint32_t i = 0; i < n; i++) {
        sorted[i] = nodes[i];
        decommit_ok[i] = 0;
    }
    hz3_central_cold_decommit_sort_nodes(sorted, n);

    size_t run_bytes = hz3_central_cold_run_bytes(sc);
    uint32_t ranges = 0;
    uint64_t saved_syscalls = 0;
    size_t max_range_bytes = 0;
    uint32_t i = 0;

    while (i < n) {
        uint32_t start = i;
        uintptr_t base = (uintptr_t)sorted[i]->run;
        uintptr_t end = base + run_bytes;
        i++;

        while (i < n && (uintptr_t)sorted[i]->run == end) {
            end += run_bytes;
            i++;
        }

        size_t range_bytes = (size_t)(end - base);
        if (range_bytes > max_range_bytes) {
            max_range_bytes = range_bytes;
        }
        ranges++;

        int ok = hz3_central_cold_decommit_range((void*)base, range_bytes) == 0;
        if (ok) {
            for (uint32_t j = start; j < i; j++) {
                hz3_central_cold_mark_node_decommit_ok(nodes,
                                                       decommit_ok,
                                                       n,
                                                       sorted[j],
                                                       1);
            }
            saved_syscalls += (uint64_t)((i - start) - 1);
            continue;
        }

        // A range may cross a reservation boundary. Fall back to individual
        // decommit so the experiment stays conservative and rollbackable.
        for (uint32_t j = start; j < i; j++) {
            Hz3CentralColdNode* node = sorted[j];
            int single_ok =
                hz3_central_cold_decommit_run(node->run, run_bytes) == 0;
            hz3_central_cold_mark_node_decommit_ok(nodes,
                                                   decommit_ok,
                                                   n,
                                                   node,
                                                   single_ok);
        }
    }

    hz3_s65_cold_note_decommit_coalesce_observe(n,
                                                ranges,
                                                saved_syscalls,
                                                (uint64_t)max_range_bytes);
}
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
static int hz3_central_cold_recommit_range(void* base, size_t bytes) {
    if (!base || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    void* p = VirtualAlloc(base, bytes, MEM_COMMIT, PAGE_READWRITE);
    int ok = (p == base);
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_recommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_recommit(1, bytes, 0);
    return 0;
#endif
}
#endif

static void hz3_central_cold_ready_push_head_locked(Hz3CentralColdExternalBin* bin,
                                                    Hz3CentralColdNode* node) {
    node->state = HZ3_CENTRAL_COLD_STATE_READY;
    node->prev = NULL;
    node->next = bin->ready_head;
    if (bin->ready_head) {
        bin->ready_head->prev = node;
    } else {
        bin->ready_tail = node;
    }
    bin->ready_head = node;
    bin->ready_count++;
}

static Hz3CentralColdNode* hz3_central_cold_ready_pop_head_locked(
    Hz3CentralColdExternalBin* bin) {
    Hz3CentralColdNode* node = bin->ready_head;
    if (!node) {
        return NULL;
    }
    hz3_central_cold_note_state_or_mismatch(node, HZ3_CENTRAL_COLD_STATE_READY);
    bin->ready_head = node->next;
    if (bin->ready_head) {
        bin->ready_head->prev = NULL;
    } else {
        bin->ready_tail = NULL;
    }
    node->next = NULL;
    node->prev = NULL;
    if (bin->ready_count > 0) {
        bin->ready_count--;
    }
    return node;
}

static Hz3CentralColdNode* hz3_central_cold_ready_pop_tail_locked(
    Hz3CentralColdExternalBin* bin) {
    Hz3CentralColdNode* node = bin->ready_tail;
    if (!node) {
        return NULL;
    }
    hz3_central_cold_note_state_or_mismatch(node, HZ3_CENTRAL_COLD_STATE_READY);
    bin->ready_tail = node->prev;
    if (bin->ready_tail) {
        bin->ready_tail->next = NULL;
    } else {
        bin->ready_head = NULL;
    }
    node->next = NULL;
    node->prev = NULL;
    if (bin->ready_count > 0) {
        bin->ready_count--;
    }
    return node;
}

static void hz3_central_cold_decommitted_push_head_locked(
    Hz3CentralColdExternalBin* bin,
    Hz3CentralColdNode* node) {
    node->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTED;
    node->prev = NULL;
    node->next = bin->decommitted_head;
    bin->decommitted_head = node;
    bin->decommitted_count++;
}

static Hz3CentralColdNode* hz3_central_cold_decommitted_pop_head_locked(
    Hz3CentralColdExternalBin* bin) {
    Hz3CentralColdNode* node = bin->decommitted_head;
    if (!node) {
        return NULL;
    }
    hz3_central_cold_note_state_or_mismatch(node, HZ3_CENTRAL_COLD_STATE_DECOMMITTED);
    bin->decommitted_head = node->next;
    node->next = NULL;
    node->prev = NULL;
    if (bin->decommitted_count > 0) {
        bin->decommitted_count--;
    }
    return node;
}

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
static uint32_t hz3_central_cold_rewarm_decommitted_locked(Hz3CentralColdExternalBin* bin,
                                                           int sc,
                                                           uint32_t limit) {
    uint32_t warmed = 0;
    while (warmed < limit && bin->decommitted_head) {
        Hz3CentralColdNode* node = hz3_central_cold_decommitted_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (hz3_central_cold_node_is_decommitted(node) &&
            hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) != 0) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] rewarm recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
        hz3_central_cold_ready_push_head_locked(bin, node);
        warmed++;
    }

    if (warmed > 0) {
        hz3_s65_cold_note_decommitted_pop(sc, warmed);
        hz3_s65_cold_note_committed_push(sc, warmed);
        hz3_s65_cold_note_rewarm(warmed);
    }
    return warmed;
}

#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
enum { HZ3_CENTRAL_COLD_REWARM_DETACH_MAX = 16 };

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
static void hz3_central_cold_sort_nodes_by_run(Hz3CentralColdNode** nodes,
                                               uint32_t n) {
    for (uint32_t i = 1; i < n; i++) {
        Hz3CentralColdNode* cur = nodes[i];
        uintptr_t cur_addr = (uintptr_t)cur->run;
        uint32_t j = i;
        while (j > 0 && (uintptr_t)nodes[j - 1]->run > cur_addr) {
            nodes[j] = nodes[j - 1];
            j--;
        }
        nodes[j] = cur;
    }
}

static void hz3_central_cold_mark_node_commit_ok(Hz3CentralColdNode** nodes,
                                                 uint8_t* commit_ok,
                                                 uint32_t n,
                                                 Hz3CentralColdNode* target,
                                                 int ok) {
    for (uint32_t i = 0; i < n; i++) {
        if (nodes[i] == target) {
            commit_ok[i] = (uint8_t)ok;
            return;
        }
    }
}

static void hz3_central_cold_recommit_nodes_coalesced(Hz3CentralColdNode** nodes,
                                                      uint8_t* commit_ok,
                                                      uint32_t n,
                                                      int sc) {
    if (n == 0) {
        return;
    }

    Hz3CentralColdNode* sorted[HZ3_CENTRAL_COLD_REWARM_DETACH_MAX];
    for (uint32_t i = 0; i < n; i++) {
        sorted[i] = nodes[i];
    }
    hz3_central_cold_sort_nodes_by_run(sorted, n);

    size_t run_bytes = hz3_central_cold_run_bytes(sc);
    uint32_t ranges = 0;
    uint64_t saved_syscalls = 0;
    size_t max_range_bytes = 0;
    uint32_t i = 0;

    while (i < n) {
        uint32_t start = i;
        uintptr_t base = (uintptr_t)sorted[i]->run;
        uintptr_t end = base + run_bytes;
        i++;

        while (i < n && (uintptr_t)sorted[i]->run == end) {
            end += run_bytes;
            i++;
        }

        size_t range_bytes = (size_t)(end - base);
        if (range_bytes > max_range_bytes) {
            max_range_bytes = range_bytes;
        }
        ranges++;

        int ok = hz3_central_cold_recommit_range((void*)base, range_bytes) == 0;
        if (ok) {
            for (uint32_t j = start; j < i; j++) {
                hz3_central_cold_mark_node_commit_ok(nodes,
                                                     commit_ok,
                                                     n,
                                                     sorted[j],
                                                     1);
            }
            saved_syscalls += (uint64_t)((i - start) - 1);
            continue;
        }

        // Range commit can fail if adjacent runs cross separate reservations.
        // Fall back to individual commits so the coalesced lane stays safe.
        for (uint32_t j = start; j < i; j++) {
            Hz3CentralColdNode* node = sorted[j];
            int single_ok =
                hz3_central_cold_recommit_run(node->run, run_bytes) == 0;
            if (!single_ok && HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST) {
                fprintf(stderr,
                        "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] coalesced fallback recommit failed run=%p sc=%d bytes=%zu\n",
                        node->run,
                        sc,
                        run_bytes);
                abort();
            }
            hz3_central_cold_mark_node_commit_ok(nodes,
                                                 commit_ok,
                                                 n,
                                                 node,
                                                 single_ok);
        }
    }

    hz3_s65_cold_note_recommit_coalesce(n,
                                        ranges,
                                        saved_syscalls,
                                        (uint64_t)max_range_bytes);
}
#endif

static uint32_t hz3_central_cold_rewarm_decommitted_outside_lock(
    Hz3CentralColdExternalBin* bin,
    int sc,
    uint32_t limit) {
    Hz3CentralColdNode* nodes[HZ3_CENTRAL_COLD_REWARM_DETACH_MAX];
    uint8_t commit_ok[HZ3_CENTRAL_COLD_REWARM_DETACH_MAX];
    uint32_t detached = 0;

    if (limit > HZ3_CENTRAL_COLD_REWARM_DETACH_MAX) {
        limit = HZ3_CENTRAL_COLD_REWARM_DETACH_MAX;
    }

    hz3_lock_acquire(&bin->lock);
    while (detached < limit && bin->decommitted_head) {
        Hz3CentralColdNode* node = hz3_central_cold_decommitted_pop_head_locked(bin);
        if (!node) {
            break;
        }
        node->state = HZ3_CENTRAL_COLD_STATE_RECOMMITTING;
        bin->inflight_recommitting++;
        nodes[detached] = node;
        commit_ok[detached] = 0;
        detached++;
    }
    hz3_lock_release(&bin->lock);

    if (detached == 0) {
        return 0;
    }

    hz3_s65_cold_note_inflight_recommitting_add(detached);
#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
    hz3_central_cold_recommit_nodes_coalesced(nodes, commit_ok, detached, sc);
#else
    for (uint32_t i = 0; i < detached; i++) {
        Hz3CentralColdNode* node = nodes[i];
        int ok = hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) == 0;
        if (!ok && HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] mru rewarm recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
        commit_ok[i] = (uint8_t)ok;
    }
#endif

    uint32_t warmed = 0;
    uint32_t failed = 0;
    hz3_lock_acquire(&bin->lock);
    for (uint32_t i = 0; i < detached; i++) {
        Hz3CentralColdNode* node = nodes[i];
        if (bin->inflight_recommitting > 0) {
            bin->inflight_recommitting--;
        }
        if (commit_ok[i]) {
            hz3_central_cold_ready_push_head_locked(bin, node);
            warmed++;
        } else {
            hz3_central_cold_decommitted_push_head_locked(bin, node);
            failed++;
        }
    }
    hz3_lock_release(&bin->lock);
    hz3_s65_cold_note_inflight_recommitting_sub(detached);

    hz3_s65_cold_note_decommitted_pop(sc, detached);
    hz3_s65_cold_note_unlock_recommit(detached);
    if (warmed > 0) {
        hz3_s65_cold_note_committed_push(sc, warmed);
        hz3_s65_cold_note_rewarm(warmed);
    }
    if (failed > 0) {
        hz3_s65_cold_note_decommitted_push(sc, failed);
    }
    return warmed;
}
#endif
#endif
#endif

static Hz3CentralColdNode* hz3_central_cold_external_node_alloc(void* run) {
    Hz3CentralColdNode* node = NULL;

    hz3_lock_acquire(&g_hz3_cold_node_lock);
    node = g_hz3_cold_free_nodes;
    if (node) {
        g_hz3_cold_free_nodes = node->next;
    }
    hz3_lock_release(&g_hz3_cold_node_lock);

    if (!node) {
        hz3_s65_cold_ext_note_alloc_fail();
        return NULL;
    }

    node->run = run;
    node->next = NULL;
    node->prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    node->state = HZ3_CENTRAL_COLD_STATE_READY;
#endif
    hz3_s65_cold_ext_note_node_alloc();
    return node;
}

static void hz3_central_cold_external_node_free(Hz3CentralColdNode* node) {
    if (!node) {
        return;
    }

    node->run = NULL;
    node->prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    node->state = 0;
#endif
    hz3_lock_acquire(&g_hz3_cold_node_lock);
    node->next = g_hz3_cold_free_nodes;
    g_hz3_cold_free_nodes = node;
    hz3_lock_release(&g_hz3_cold_node_lock);
    hz3_s65_cold_ext_note_node_free();
}
#endif

#if HZ3_S222_CENTRAL_ATOMIC_FAST
typedef struct {
    _Atomic(void*) head;
} Hz3CentralFastBin;

static Hz3CentralFastBin g_hz3_central_fast[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static inline int hz3_s222_sc_ok(int sc) {
    return (sc >= HZ3_S222_SC_MIN && sc <= HZ3_S222_SC_MAX);
}

#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
static inline int hz3_s234_sc_ok(int sc) {
    return (sc >= HZ3_S234_SC_MIN && sc <= HZ3_S234_SC_MAX);
}
#endif

static inline void hz3_s222_push_list_to_fast(Hz3CentralFastBin* fast, void* head, void* tail) {
    void* old_head;
    do {
        old_head = atomic_load_explicit(&fast->head, memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(&fast->head, &old_head, head,
                                                     memory_order_release,
                                                     memory_order_acquire));
}

#if HZ3_S222_STATS && HZ3_S234_CENTRAL_FAST_PARTIAL_POP
static _Atomic uint64_t g_s222_s234_pop_partial_hits = 0;
static _Atomic uint64_t g_s222_s234_pop_partial_objs = 0;
static _Atomic uint64_t g_s222_s234_pop_partial_retry = 0;
static _Atomic uint64_t g_s222_s234_pop_partial_fallback = 0;
#endif

#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
// Return value:
//   >0 : popped object count
//    0 : fast head empty
//   -1 : retry budget exhausted, caller should fallback to legacy exchange path
static inline int hz3_s234_fast_pop_partial(Hz3CentralFastBin* fast, int sc, void** out, int want) {
    int retries = 0;
    for (;;) {
        void* old_head = atomic_load_explicit(&fast->head, memory_order_acquire);
        if (!old_head) {
            return 0;
        }

        void* cur = old_head;
        void* tail = NULL;
        int got = 0;
        while (got < want && cur) {
            tail = cur;
            cur = hz3_obj_get_next(cur);
            got++;
        }

        if (!tail || got <= 0) {
            return 0;
        }

        // CAS only updates stack head; internal links remain immutable.
        void* expect = old_head;
        if (atomic_compare_exchange_weak_explicit(&fast->head, &expect, cur,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            hz3_obj_set_next(tail, NULL);
            int out_got = 0;
            void* node = old_head;
            while (out_got < got && node) {
                void* next = hz3_obj_get_next(node);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
                hz3_central_shadow_verify_and_remove(node, next);
  #else
                hz3_central_shadow_remove(node);
  #endif
#endif
                out[out_got++] = node;
                node = next;
            }
#if HZ3_S222_STATS
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_hits, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_objs, (uint64_t)out_got, memory_order_relaxed);
            if (retries > 0) {
                atomic_fetch_add_explicit(&g_s222_s234_pop_partial_retry, (uint64_t)retries, memory_order_relaxed);
            }
#endif
            hz3_s65_central_note_pop(sc, (uint64_t)out_got);
            return out_got;
        }

        retries++;
        if (retries >= HZ3_S234_CAS_RETRY_MAX) {
#if HZ3_S222_STATS
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_fallback, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s222_s234_pop_partial_retry, (uint64_t)retries, memory_order_relaxed);
#endif
            return -1;
        }
    }
}
#endif

#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
typedef struct {
    void* head;
    void* tail;
    uint32_t count;
} Hz3S228LocalRem;

static HZ3_TLS Hz3S228LocalRem t_s228_local_rem[HZ3_NUM_SC];

static inline int hz3_s228_sc_ok(int sc) {
    return (sc >= HZ3_S228_SC_MIN && sc <= HZ3_S228_SC_MAX);
}

static inline int hz3_s228_consume_local(int sc, void** out, int max_take) {
    Hz3S228LocalRem* rem = &t_s228_local_rem[sc];
    if (!rem->head || max_take <= 0) {
        return 0;
    }
    int got = 0;
    void* cur = rem->head;
    while (got < max_take && cur) {
        void* next = hz3_obj_get_next(cur);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
        hz3_central_shadow_verify_and_remove(cur, next);
  #else
        hz3_central_shadow_remove(cur);
  #endif
#endif
        out[got++] = cur;
        cur = next;
    }
    rem->head = cur;
    if (!cur) {
        rem->tail = NULL;
    }
    if ((uint32_t)got >= rem->count) {
        rem->count = 0;
    } else {
        rem->count -= (uint32_t)got;
    }
    hz3_s65_local_rem_note_pop(sc, (uint64_t)got);
    return got;
}
#endif

#if HZ3_S222_STATS
static _Atomic uint64_t g_s222_push_calls = 0;
static _Atomic uint64_t g_s222_push_objs = 0;
static _Atomic uint64_t g_s222_pop_calls = 0;
static _Atomic uint64_t g_s222_pop_hits = 0;
static _Atomic uint64_t g_s222_pop_objs = 0;
static _Atomic uint64_t g_s222_pop_repush = 0;
static hz3_once_t g_s222_stats_once = HZ3_ONCE_INIT;

static void hz3_s222_dump(void) {
#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
    fprintf(stderr,
            "[HZ3_S222] push_calls=%lu push_objs=%lu pop_calls=%lu pop_hits=%lu pop_objs=%lu pop_repush=%lu "
            "s234_hits=%lu s234_objs=%lu s234_retry=%lu s234_fallback=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s222_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_repush, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_retry, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_s234_pop_partial_fallback, memory_order_relaxed));
#else
    fprintf(stderr,
            "[HZ3_S222] push_calls=%lu push_objs=%lu pop_calls=%lu pop_hits=%lu pop_objs=%lu pop_repush=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s222_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s222_pop_repush, memory_order_relaxed));
#endif
}

static void hz3_s222_register_atexit(void) {
    atexit(hz3_s222_dump);
}

#define HZ3_S222_STAT_INC(name) \
    atomic_fetch_add_explicit(&g_s222_##name, 1, memory_order_relaxed)
#define HZ3_S222_STAT_ADD(name, v) \
    atomic_fetch_add_explicit(&g_s222_##name, (uint64_t)(v), memory_order_relaxed)
#else
#define HZ3_S222_STAT_INC(name) ((void)0)
#define HZ3_S222_STAT_ADD(name, v) ((void)(v))
#endif
#endif

#if HZ3_S189_MEDIUM_TRANSFERCACHE
typedef struct {
    hz3_lock_t lock;
    void* head;
    uint32_t count;
} Hz3CentralXferBin;

static Hz3CentralXferBin g_hz3_central_xfer[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Atomic uint64_t g_s189_xfer_push_calls = 0;
static _Atomic uint64_t g_s189_xfer_push_objs = 0;
static _Atomic uint64_t g_s189_xfer_push_drop = 0;
static _Atomic uint64_t g_s189_xfer_pop_calls = 0;
static _Atomic uint64_t g_s189_xfer_pop_hits = 0;
static _Atomic uint64_t g_s189_xfer_pop_objs = 0;
static hz3_once_t g_s189_xfer_atexit_once = HZ3_ONCE_INIT;

static inline int hz3_s189_sc_ok(int sc) {
    return (sc >= HZ3_S189_SC_MIN && sc <= HZ3_S189_SC_MAX);
}

static void hz3_s189_xfer_dump(void) {
    fprintf(stderr,
            "[HZ3_S189_XFER] push_calls=%lu push_objs=%lu push_drop=%lu "
            "pop_calls=%lu pop_hits=%lu pop_objs=%lu\n",
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_objs, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_push_drop, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_calls, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_hits, memory_order_relaxed),
            (unsigned long)atomic_load_explicit(&g_s189_xfer_pop_objs, memory_order_relaxed));
}

static void hz3_s189_xfer_register_atexit(void) {
    atexit(hz3_s189_xfer_dump);
}
#endif

// Day 5: hz3_once for thread-safe initialization
static hz3_once_t g_hz3_central_once = HZ3_ONCE_INIT;

static void hz3_central_do_init(void) {
#if HZ3_S65_CENTRAL_COLD_ENABLE && HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    hz3_central_cold_external_pool_init();
#endif
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            Hz3CentralBin* bin = &g_hz3_central[shard][sc];
            hz3_lock_init(&bin->lock);
            bin->head = NULL;
            bin->count = 0;
#if HZ3_S222_CENTRAL_ATOMIC_FAST
            if (hz3_s222_sc_ok(sc)) {
                atomic_store_explicit(&g_hz3_central_fast[shard][sc].head, NULL, memory_order_relaxed);
            }
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
            if (hz3_s189_sc_ok(sc)) {
                Hz3CentralXferBin* xfer = &g_hz3_central_xfer[shard][sc];
                hz3_lock_init(&xfer->lock);
                xfer->head = NULL;
                xfer->count = 0;
            }
#endif
#if HZ3_S65_CENTRAL_COLD_ENABLE
            {
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
                Hz3CentralColdExternalBin* cold = &g_hz3_central_cold_ext[shard][sc];
                hz3_lock_init(&cold->lock);
                cold->ready_head = NULL;
                cold->ready_tail = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
                cold->decommitted_head = NULL;
                cold->ready_count = 0;
                cold->decommitted_count = 0;
                cold->inflight_decommitting = 0;
                cold->inflight_recommitting = 0;
#endif
                cold->count = 0;
#else
                Hz3CentralBin* cold = &g_hz3_central_cold[shard][sc];
                hz3_lock_init(&cold->lock);
                cold->head = NULL;
                cold->count = 0;
#endif
            }
#endif
        }
    }
}

void hz3_central_init(void) {
    hz3_once(&g_hz3_central_once, hz3_central_do_init);
#if HZ3_S65_STATS
    hz3_once(&g_s65_central_stats_once, hz3_s65_central_retention_register_once);
#endif
#if HZ3_S222_CENTRAL_ATOMIC_FAST && HZ3_S222_STATS
    hz3_once(&g_s222_stats_once, hz3_s222_register_atexit);
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
    hz3_once(&g_s189_xfer_atexit_once, hz3_s189_xfer_register_atexit);
#endif
}

void hz3_central_push(int shard, int sc, void* run) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!run) return;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Capture boundary return address at non-inlined function entry.
    void* s86_ra = __builtin_extract_return_addr(__builtin_return_address(0));
#endif

#if HZ3_S90_CENTRAL_GUARD
    {
        uint32_t page_idx = 0;
        if (hz3_arena_page_index_fast(run, &page_idx) && g_hz3_page_tag32) {
            uint32_t tag32 = hz3_pagetag32_load(page_idx);
            uint32_t have_bin = (tag32 != 0) ? hz3_pagetag32_bin(tag32) : 0;
            uint8_t have_dst = (tag32 != 0) ? hz3_pagetag32_dst(tag32) : 0;
            uint32_t want_bin = (uint32_t)(HZ3_MEDIUM_BIN_BASE + (uint32_t)sc);
            uint8_t want_dst = (uint8_t)shard;
            if (tag32 == 0 || have_bin != want_bin || have_dst != want_dst) {
                static _Atomic int g_s90_shot = 0;
                if (!HZ3_S90_CENTRAL_GUARD_SHOT ||
                    atomic_exchange_explicit(&g_s90_shot, 1, memory_order_acq_rel) == 0) {
                    // Slow path: compute base/off only on failure.
                    uintptr_t so_base = 0;
                    {
                        FILE* f = fopen("/proc/self/maps", "r");
                        if (f) {
                            char line[512];
                            while (fgets(line, sizeof(line), f)) {
                                if (strstr(line, "libhakozuna_hz3") == NULL) continue;
                                char* dash = strchr(line, '-');
                                if (!dash) continue;
                                *dash = '\0';
                                so_base = (uintptr_t)strtoull(line, NULL, 16);
                                break;
                            }
                            fclose(f);
                        }
                    }
                    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
                    unsigned long off = (so_base && ra0) ? (unsigned long)((uintptr_t)ra0 - so_base) : 0;
                    fprintf(stderr,
                            "[HZ3_S90_CENTRAL_GUARD] where=push ptr=%p shard=%d sc=%d page_idx=%u "
                            "tag32=0x%08x have_bin=%u have_dst=%u want_bin=%u want_dst=%u "
                            "ra=%p base=%p off=0x%lx\n",
                            run, shard, sc, page_idx,
                            tag32, have_bin, (unsigned)have_dst, want_bin, (unsigned)want_dst,
                            ra0, (void*)so_base, off);
                }
                if (HZ3_S90_CENTRAL_GUARD_FAILFAST) {
                    abort();
                }
            }
        }
    }
#endif

    hz3_lock_acquire(&bin->lock);

    // Intrusive list: store next at run[0]
    hz3_obj_set_next(run, bin->head);
    bin->head = run;
    bin->count++;
    hz3_s65_central_note_push(sc, 1);

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Record (ptr,next) as stored in central after linking.
    hz3_central_shadow_record(run, hz3_obj_get_next(run), s86_ra);
#endif
    hz3_lock_release(&bin->lock);
}

void* hz3_central_pop(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

    hz3_lock_acquire(&bin->lock);

    void* run = bin->head;
    if (run) {
        void* next = hz3_obj_get_next(run);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
        hz3_central_shadow_verify_and_remove(run, next);
  #else
        hz3_central_shadow_remove(run);
  #endif
#endif
        bin->head = next;
        bin->count--;
        hz3_s65_central_note_pop(sc, 1);
    }

    hz3_lock_release(&bin->lock);

    return run;
}

// ============================================================================
// Day 5: Batch API
// ============================================================================

// Push a linked list to central (head→...→tail in forward order)
// tail->next will be set to old head (caller doesn't need to set it)
void hz3_central_push_list(int shard, int sc, void* head, void* tail, uint32_t n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail || n == 0) return;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Capture return address from push boundary (caller of push_list).
    void* s86_ra = __builtin_extract_return_addr(__builtin_return_address(0));
#endif

#if HZ3_S90_CENTRAL_GUARD
    {
        uint32_t want_bin = (uint32_t)(HZ3_MEDIUM_BIN_BASE + (uint32_t)sc);
        uint8_t want_dst = (uint8_t)shard;
        void* cur = head;
        for (uint32_t i = 0; i < n && cur; i++) {
            uint32_t page_idx = 0;
            if (hz3_arena_page_index_fast(cur, &page_idx) && g_hz3_page_tag32) {
                uint32_t tag32 = hz3_pagetag32_load(page_idx);
                uint32_t have_bin = (tag32 != 0) ? hz3_pagetag32_bin(tag32) : 0;
                uint8_t have_dst = (tag32 != 0) ? hz3_pagetag32_dst(tag32) : 0;
                if (tag32 == 0 || have_bin != want_bin || have_dst != want_dst) {
                    static _Atomic int g_s90_list_shot = 0;
                    if (!HZ3_S90_CENTRAL_GUARD_SHOT ||
                        atomic_exchange_explicit(&g_s90_list_shot, 1, memory_order_acq_rel) == 0) {
                        uintptr_t so_base = 0;
                        {
                            FILE* f = fopen("/proc/self/maps", "r");
                            if (f) {
                                char line[512];
                                while (fgets(line, sizeof(line), f)) {
                                    if (strstr(line, "libhakozuna_hz3") == NULL) continue;
                                    char* dash = strchr(line, '-');
                                    if (!dash) continue;
                                    *dash = '\0';
                                    so_base = (uintptr_t)strtoull(line, NULL, 16);
                                    break;
                                }
                                fclose(f);
                            }
                        }
                        void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
                        unsigned long off = (so_base && ra0) ? (unsigned long)((uintptr_t)ra0 - so_base) : 0;
                        fprintf(stderr,
                                "[HZ3_S90_CENTRAL_GUARD] where=push_list ptr=%p shard=%d sc=%d page_idx=%u "
                                "tag32=0x%08x have_bin=%u have_dst=%u want_bin=%u want_dst=%u "
                                "ra=%p base=%p off=0x%lx\n",
                                cur, shard, sc, page_idx,
                                tag32, have_bin, (unsigned)have_dst, want_bin, (unsigned)want_dst,
                                ra0, (void*)so_base, off);
                    }
                    if (HZ3_S90_CENTRAL_GUARD_FAILFAST) {
                        abort();
                    }
                    break;
                }
            }
            if (cur == tail) break;
            cur = hz3_obj_get_next(cur);
        }
    }
#endif

#if HZ3_S222_CENTRAL_ATOMIC_FAST
    if (hz3_s222_sc_ok(sc)) {
        Hz3CentralFastBin* fast = &g_hz3_central_fast[shard][sc];
        void* old_head;
        do {
            old_head = atomic_load_explicit(&fast->head, memory_order_acquire);
            hz3_obj_set_next(tail, old_head);
        } while (!atomic_compare_exchange_weak_explicit(&fast->head, &old_head, head,
                                                         memory_order_release,
                                                         memory_order_acquire));
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
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
        HZ3_S222_STAT_INC(push_calls);
        HZ3_S222_STAT_ADD(push_objs, n);
        hz3_s65_central_note_push(sc, n);
        return;
    }
#endif

    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(tail, bin->head);  // tail->next = old head
    bin->head = head;
    bin->count += n;
    hz3_s65_central_note_push(sc, n);

#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
    // S86: Record each node after linking (tail->next overwritten on concat).
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
}

// Pop up to 'want' objects into out array (returns actual count)
int hz3_central_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    if (!out || want <= 0) return 0;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];

#if HZ3_S222_CENTRAL_ATOMIC_FAST
    if (hz3_s222_sc_ok(sc)) {
        Hz3CentralFastBin* fast = &g_hz3_central_fast[shard][sc];
        HZ3_S222_STAT_INC(pop_calls);
        int pre_got = 0;

#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
        if (hz3_s228_sc_ok(sc)) {
            int allow_local = 1;
#if HZ3_S228_ONLY_IF_LIVECOUNT_1
            if (hz3_shard_live_count((uint8_t)shard) > 1) {
                allow_local = 0;
            }
#endif
            if (!allow_local) {
#if HZ3_S228_FLUSH_LOCAL_ON_LIVECOUNT_GT1
                Hz3S228LocalRem* rem_flush = &t_s228_local_rem[sc];
                if (rem_flush->head) {
                    uint32_t rem_n = rem_flush->count;
                    hz3_s222_push_list_to_fast(fast, rem_flush->head, rem_flush->tail);
                    hz3_s65_local_rem_note_pop(sc, (uint64_t)rem_n);
                    hz3_s65_central_note_push(sc, (uint64_t)rem_n);
                    rem_flush->head = NULL;
                    rem_flush->tail = NULL;
                    rem_flush->count = 0;
                }
#endif
            } else {
                int local_got = hz3_s228_consume_local(sc, out, want);
                if (local_got >= want) {
                    HZ3_S222_STAT_INC(pop_hits);
                    HZ3_S222_STAT_ADD(pop_objs, local_got);
                    return local_got;
                }
                if (local_got > 0) {
                    pre_got = local_got;
                    out += local_got;
                    want -= local_got;
                }
            }
        }
#endif

#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
        if (hz3_s234_sc_ok(sc)) {
            int part_got = hz3_s234_fast_pop_partial(fast, sc, out, want);
            if (part_got > 0) {
                HZ3_S222_STAT_INC(pop_hits);
                HZ3_S222_STAT_ADD(pop_objs, part_got + pre_got);
                return part_got + pre_got;
            }
            if (part_got == 0 && pre_got > 0) {
                HZ3_S222_STAT_INC(pop_hits);
                HZ3_S222_STAT_ADD(pop_objs, pre_got);
                return pre_got;
            }
            // part_got == -1 -> fallback to exchange path below.
        }
#endif

        void* list = atomic_exchange_explicit(&fast->head, NULL, memory_order_acq_rel);
        if (list) {
            int got = 0;
            void* cur = list;
            while (got < want && cur) {
                void* next = hz3_obj_get_next(cur);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
                hz3_central_shadow_verify_and_remove(cur, next);
  #else
                hz3_central_shadow_remove(cur);
  #endif
#endif
                out[got++] = cur;
                cur = next;
            }

            if (cur) {
                int keep_local = 0;
#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
                if (hz3_s228_sc_ok(sc)) {
                    keep_local = 1;
#if HZ3_S228_ONLY_IF_LIVECOUNT_1
                    if (hz3_shard_live_count((uint8_t)shard) > 1) {
                        keep_local = 0;
                    }
#endif
                }
#endif
                if (keep_local) {
#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
                    Hz3S228LocalRem* rem_keep = &t_s228_local_rem[sc];
                    void* rem_head = cur;
                    void* rem_tail = cur;
                    uint32_t rem_count = 1;
                    while (hz3_obj_get_next(rem_tail)) {
                        rem_tail = hz3_obj_get_next(rem_tail);
                        rem_count++;
                    }
                    if (rem_keep->head) {
                        hz3_obj_set_next(rem_tail, rem_keep->head);
                        rem_keep->head = rem_head;
                        if (!rem_keep->tail) {
                            rem_keep->tail = rem_tail;
                        }
                        rem_keep->count += rem_count;
                    } else {
                        rem_keep->head = rem_head;
                        rem_keep->tail = rem_tail;
                        rem_keep->count = rem_count;
                    }
                    hz3_s65_central_note_pop(sc, (uint64_t)got + (uint64_t)rem_count);
                    hz3_s65_local_rem_note_add(sc, (uint64_t)rem_count);
#endif
                } else {
                    // Re-publish remaining list back to fast head.
                    void* rem_head = cur;
                    void* rem_tail = cur;
                    while (hz3_obj_get_next(rem_tail)) {
                        rem_tail = hz3_obj_get_next(rem_tail);
                    }
                    hz3_s222_push_list_to_fast(fast, rem_head, rem_tail);
                    HZ3_S222_STAT_INC(pop_repush);
                    hz3_s65_central_note_pop(sc, (uint64_t)got);
                }
            } else {
                hz3_s65_central_note_pop(sc, (uint64_t)got);
            }

            HZ3_S222_STAT_INC(pop_hits);
            HZ3_S222_STAT_ADD(pop_objs, got + pre_got);
            return got + pre_got;
        }
        if (pre_got > 0) {
            HZ3_S222_STAT_INC(pop_hits);
            HZ3_S222_STAT_ADD(pop_objs, pre_got);
            return pre_got;
        }
    }
#endif

    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* cur = bin->head;
        void* next = hz3_obj_get_next(cur);
#if HZ3_S86_CENTRAL_SHADOW && HZ3_S86_CENTRAL_SHADOW_MEDIUM
  #if HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
        hz3_central_shadow_verify_and_remove(cur, next);
  #else
        hz3_central_shadow_remove(cur);
  #endif
#endif
        out[got++] = cur;
        bin->head = next;
        bin->count--;
    }
    hz3_s65_central_note_pop(sc, (uint64_t)got);
    hz3_lock_release(&bin->lock);

    return got;
}

int hz3_central_has_supply(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
#if HZ3_S222_CENTRAL_ATOMIC_FAST
    if (hz3_s222_sc_ok(sc)) {
        Hz3CentralFastBin* fast = &g_hz3_central_fast[shard][sc];
        return atomic_load_explicit(&fast->head, memory_order_acquire) != NULL;
    }
#endif
    // Locked central bin supply check is expensive; return optimistic hint.
    return 1;
}

uint32_t hz3_central_count_snapshot(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

    Hz3CentralBin* bin = &g_hz3_central[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
}

#if HZ3_S65_CENTRAL_COLD_ENABLE
void hz3_central_cold_push_list(int shard, int sc, void* head, void* tail, uint32_t n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail || n == 0) return;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    Hz3CentralColdExternalBin* bin = &g_hz3_central_cold_ext[shard][sc];
    uint32_t pushed = 0;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    uint32_t ready_pushed = 0;
    uint32_t ready_popped = 0;
    uint32_t decommitted_pushed = 0;
#endif
    void* cur = head;
    HZ3_COLD_DECOMMIT_OBS_DECL(decommit_obs);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
    Hz3CentralColdNode* decommit_nodes[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS];
    uint8_t decommit_ok[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS];
#endif

    hz3_lock_acquire(&bin->lock);
    while (pushed < n && cur) {
        void* next = hz3_obj_get_next(cur);
        Hz3CentralColdNode* node = hz3_central_cold_external_node_alloc(cur);
        if (!node) {
            break;
        }
        hz3_obj_set_next(cur, NULL);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
        hz3_central_cold_ready_push_head_locked(bin, node);
        ready_pushed++;
        bin->count++;
        pushed++;

#if !HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
        uint32_t target = hz3_central_cold_ready_target_runs(sc);
        while (bin->ready_count > target) {
            Hz3CentralColdNode* evict = hz3_central_cold_ready_pop_tail_locked(bin);
            if (!evict) {
                break;
            }
            ready_popped++;
            evict->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTING;
            bin->inflight_decommitting++;
            hz3_s65_cold_note_inflight_decommitting_add(1);
            HZ3_COLD_DECOMMIT_OBS_NOTE(decommit_obs, evict, sc);

            hz3_lock_release(&bin->lock);
            int decommit_ok =
                hz3_central_cold_decommit_run(evict->run, hz3_central_cold_run_bytes(sc)) == 0;
            hz3_lock_acquire(&bin->lock);

            if (bin->inflight_decommitting > 0) {
                bin->inflight_decommitting--;
            }
            hz3_s65_cold_note_inflight_decommitting_sub(1);
            if (decommit_ok) {
                hz3_central_cold_decommitted_push_head_locked(bin, evict);
                decommitted_pushed++;
            } else {
                hz3_central_cold_ready_push_head_locked(bin, evict);
                ready_pushed++;
            }
        }
#endif
#else
        int did_decommit = 0;
        if (hz3_central_cold_should_decommit_locked(bin, sc)) {
            if (hz3_central_cold_decommit_run(cur, hz3_central_cold_run_bytes(sc)) == 0) {
                node->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTED;
                did_decommit = 1;
            }
        }
        if (did_decommit) {
            HZ3_COLD_DECOMMIT_OBS_NOTE(decommit_obs, node, sc);
            hz3_central_cold_decommitted_push_head_locked(bin, node);
            decommitted_pushed++;
        } else {
            hz3_central_cold_ready_push_head_locked(bin, node);
            ready_pushed++;
        }
        bin->count++;
        pushed++;
#endif
#else
        node->prev = NULL;
        node->next = bin->ready_head;
        if (bin->ready_head) {
            bin->ready_head->prev = node;
        } else {
            bin->ready_tail = node;
        }
        bin->ready_head = node;
        bin->count++;
        pushed++;
#endif
        cur = next;
    }
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
    uint32_t target = hz3_central_cold_ready_target_runs(sc);
    int stop_after_failure = 0;
    while (bin->ready_count > target && !stop_after_failure) {
        uint32_t detached = 0;
        while (detached < HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS &&
               bin->ready_count > target) {
            Hz3CentralColdNode* evict = hz3_central_cold_ready_pop_tail_locked(bin);
            if (!evict) {
                break;
            }
            ready_popped++;
            evict->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTING;
            decommit_nodes[detached++] = evict;
        }
        if (detached == 0) {
            break;
        }

        bin->inflight_decommitting += detached;
        hz3_s65_cold_note_inflight_decommitting_add(detached);

        hz3_lock_release(&bin->lock);
        hz3_central_cold_decommit_nodes_coalesced(decommit_nodes,
                                                  decommit_ok,
                                                  detached,
                                                  sc);
        hz3_lock_acquire(&bin->lock);

        if (bin->inflight_decommitting >= detached) {
            bin->inflight_decommitting -= detached;
        } else {
            bin->inflight_decommitting = 0;
        }
        hz3_s65_cold_note_inflight_decommitting_sub(detached);

        uint32_t failed = 0;
        for (uint32_t i = 0; i < detached; i++) {
            if (decommit_ok[i]) {
                hz3_central_cold_decommitted_push_head_locked(bin, decommit_nodes[i]);
                decommitted_pushed++;
            } else {
                hz3_central_cold_ready_push_head_locked(bin, decommit_nodes[i]);
                ready_pushed++;
                failed++;
            }
        }
        if (failed > 0) {
            stop_after_failure = 1;
        }
    }
#endif
    hz3_lock_release(&bin->lock);
    HZ3_COLD_DECOMMIT_OBS_FLUSH(decommit_obs, sc);

    if (pushed > 0) {
        hz3_s65_cold_note_push(sc, pushed);
    }
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    hz3_s65_cold_note_committed_push(sc, ready_pushed);
    hz3_s65_cold_note_committed_pop(sc, ready_popped);
    hz3_s65_cold_note_decommitted_push(sc, decommitted_pushed);
#endif

    if (pushed < n && cur) {
        uint32_t fallback_n = n - pushed;
        hz3_s65_cold_ext_note_fallback_hot(fallback_n);
        hz3_central_push_list(shard, sc, cur, tail, fallback_n);
    }
#else
    Hz3CentralBin* bin = &g_hz3_central_cold[shard][sc];
    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(tail, bin->head);
    bin->head = head;
    bin->count += n;
    hz3_s65_cold_note_push(sc, n);
    hz3_lock_release(&bin->lock);
#endif
}

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static int hz3_central_cold_pop_batch_mru_ready(Hz3CentralColdExternalBin* bin,
                                                int sc,
                                                void** out,
                                                int want) {
    hz3_lock_acquire(&bin->lock);

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
    if (!bin->ready_head &&
        bin->decommitted_head &&
        hz3_sc_to_pages(sc) >= HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES) {
        hz3_lock_release(&bin->lock);
        (void)hz3_central_cold_rewarm_decommitted_outside_lock(
            bin,
            sc,
            (uint32_t)HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS);
        hz3_lock_acquire(&bin->lock);
    }
#endif
#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_ON_MISS
    int small_rewarm_pages = hz3_sc_to_pages(sc);
    if (!bin->ready_head &&
        bin->decommitted_head &&
        small_rewarm_pages >= HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES &&
        small_rewarm_pages <= HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES) {
        hz3_lock_release(&bin->lock);
        (void)hz3_central_cold_rewarm_decommitted_outside_lock(
            bin,
            sc,
            (uint32_t)HZ3_S65_CENTRAL_COLD_SMALL_REWARM_BATCH_RUNS);
        hz3_lock_acquire(&bin->lock);
    }
#endif

    int got = 0;
    while (got < want && bin->ready_head) {
        Hz3CentralColdNode* node = hz3_central_cold_ready_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (bin->count > 0) {
            bin->count--;
        }
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }

    hz3_s65_cold_note_ready_pop_attempt((uint64_t)got);
    hz3_s65_cold_note_pop(sc, (uint64_t)got);
    hz3_s65_cold_note_committed_pop(sc, (uint64_t)got);
    hz3_lock_release(&bin->lock);
    return got;
}
#endif

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK
static int hz3_central_cold_pop_batch_unlocked_recommit(Hz3CentralColdExternalBin* bin,
                                                        int sc,
                                                        void** out,
                                                        int want) {
    if (want > 16) {
        want = 16;
    }

    Hz3CentralColdNode* committed_nodes[16];
    Hz3CentralColdNode* decommitted_nodes[16];
    int committed_got = 0;
    int decommitted_got = 0;

    hz3_lock_acquire(&bin->lock);
    while (committed_got < want && bin->ready_head) {
        Hz3CentralColdNode* node = hz3_central_cold_ready_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (bin->count > 0) {
            bin->count--;
        }
        committed_nodes[committed_got++] = node;
    }

    while ((committed_got + decommitted_got) < want && bin->decommitted_head) {
        Hz3CentralColdNode* node = hz3_central_cold_decommitted_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (bin->count > 0) {
            bin->count--;
        }
        node->state = HZ3_CENTRAL_COLD_STATE_RECOMMITTING;
        bin->inflight_recommitting++;
        hz3_s65_cold_note_inflight_recommitting_add(1);
        decommitted_nodes[decommitted_got++] = node;
    }
    hz3_lock_release(&bin->lock);

    int got = 0;
    for (int i = 0; i < committed_got; i++) {
        Hz3CentralColdNode* node = committed_nodes[i];
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }

    for (int i = 0; i < decommitted_got; i++) {
        Hz3CentralColdNode* node = decommitted_nodes[i];
        if (node->state == HZ3_CENTRAL_COLD_STATE_RECOMMITTING &&
            hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) != 0) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] unlocked recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }

    if (decommitted_got > 0) {
        hz3_lock_acquire(&bin->lock);
        if (bin->inflight_recommitting >= (uint32_t)decommitted_got) {
            bin->inflight_recommitting -= (uint32_t)decommitted_got;
        } else {
            bin->inflight_recommitting = 0;
        }
        hz3_lock_release(&bin->lock);
        hz3_s65_cold_note_inflight_recommitting_sub((uint64_t)decommitted_got);
    }

    hz3_s65_cold_note_pop(sc, (uint64_t)got);
    hz3_s65_cold_note_committed_pop(sc, (uint64_t)committed_got);
    hz3_s65_cold_note_decommitted_pop(sc, (uint64_t)decommitted_got);
    hz3_s65_cold_note_unlock_recommit((uint64_t)decommitted_got);
    return got;
}
#endif

int hz3_central_cold_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    if (!out || want <= 0) return 0;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    Hz3CentralColdExternalBin* bin = &g_hz3_central_cold_ext[shard][sc];
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
    return hz3_central_cold_pop_batch_mru_ready(bin, sc, out, want);
#endif
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK
    return hz3_central_cold_pop_batch_unlocked_recommit(bin, sc, out, want);
#endif
    hz3_lock_acquire(&bin->lock);
    int got = 0;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    int committed_got = 0;
    int decommitted_got = 0;
#endif
    while (got < want && (bin->ready_head
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
                          || bin->decommitted_head
#endif
                          )) {
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
        if (!bin->ready_head &&
            bin->decommitted_head &&
            hz3_sc_to_pages(sc) >= HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES) {
            (void)hz3_central_cold_rewarm_decommitted_locked(
                bin,
                sc,
                (uint32_t)HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS);
        }
#endif
        Hz3CentralColdNode* node = NULL;
        if (bin->ready_head) {
            node = hz3_central_cold_ready_pop_head_locked(bin);
            if (!node) {
                break;
            }
            committed_got++;
        } else {
            node = hz3_central_cold_decommitted_pop_head_locked(bin);
            if (!node) {
                break;
            }
            decommitted_got++;
        }
#else
        Hz3CentralColdNode* node = bin->ready_head;
        bin->ready_head = node->next;
        if (bin->ready_head) {
            bin->ready_head->prev = NULL;
        } else {
            bin->ready_tail = NULL;
        }
        node->next = NULL;
        node->prev = NULL;
#endif
        bin->count--;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
        if (hz3_central_cold_node_is_decommitted(node) &&
            hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) != 0) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
#endif
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }
    hz3_s65_cold_note_pop(sc, (uint64_t)got);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    hz3_s65_cold_note_committed_pop(sc, (uint64_t)committed_got);
    hz3_s65_cold_note_decommitted_pop(sc, (uint64_t)decommitted_got);
#endif
    hz3_lock_release(&bin->lock);
    return got;
#else
    Hz3CentralBin* bin = &g_hz3_central_cold[shard][sc];
    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* cur = bin->head;
        void* next = hz3_obj_get_next(cur);
        out[got++] = cur;
        bin->head = next;
        bin->count--;
    }
    hz3_s65_cold_note_pop(sc, (uint64_t)got);
    hz3_lock_release(&bin->lock);
    return got;
#endif
}

uint32_t hz3_central_cold_count_snapshot(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    Hz3CentralColdExternalBin* bin = &g_hz3_central_cold_ext[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
#else
    Hz3CentralBin* bin = &g_hz3_central_cold[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
#endif
}
#endif

#if HZ3_S189_MEDIUM_TRANSFERCACHE
int hz3_central_xfer_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (!hz3_s189_sc_ok(sc)) return 0;
    if (!out || want <= 0) return 0;

    atomic_fetch_add_explicit(&g_s189_xfer_pop_calls, 1, memory_order_relaxed);
    Hz3CentralXferBin* bin = &g_hz3_central_xfer[shard][sc];

    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* run = bin->head;
        bin->head = hz3_obj_get_next(run);
        bin->count--;
        out[got++] = run;
    }
    hz3_lock_release(&bin->lock);

    if (got > 0) {
        atomic_fetch_add_explicit(&g_s189_xfer_pop_hits, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s189_xfer_pop_objs, (uint64_t)got, memory_order_relaxed);
        hz3_s65_xfer_note_pop(sc, (uint64_t)got);
    }
    return got;
}

int hz3_central_xfer_push_array(int shard, int sc, void** arr, int n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (!hz3_s189_sc_ok(sc)) return 0;
    if (!arr || n <= 0) return 0;

    atomic_fetch_add_explicit(&g_s189_xfer_push_calls, 1, memory_order_relaxed);
    Hz3CentralXferBin* bin = &g_hz3_central_xfer[shard][sc];

    hz3_lock_acquire(&bin->lock);
    int avail = (int)HZ3_S189_MAX_OBJS_PER_BIN - (int)bin->count;
    if (avail < 0) avail = 0;
    int keep = (n < avail) ? n : avail;
    int pushed = 0;
    for (int i = 0; i < keep; i++) {
        void* run = arr[i];
        if (!run) continue;
        hz3_obj_set_next(run, bin->head);
        bin->head = run;
        bin->count++;
        pushed++;
    }
    hz3_lock_release(&bin->lock);

    if (pushed > 0) {
        atomic_fetch_add_explicit(&g_s189_xfer_push_objs, (uint64_t)pushed, memory_order_relaxed);
        hz3_s65_xfer_note_push(sc, (uint64_t)pushed);
    }
    if (n > pushed) {
        atomic_fetch_add_explicit(&g_s189_xfer_push_drop, (uint64_t)(n - pushed), memory_order_relaxed);
    }
    return pushed;
}
#endif
