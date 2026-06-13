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
    uint64_t cold_node_pool_allocated = 0;
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_POOL_LAZY
    cold_node_pool_allocated =
        atomic_load_explicit(&g_hz3_cold_node_pool_allocated, memory_order_relaxed);
#else
    cold_node_pool_allocated = HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP;
#endif
    fprintf(stderr,
            "[HZ3_S65_CENTRAL_COLD_EXTERNAL] node_inuse=%" PRIu64
            " node_peak=%" PRIu64 " node_alloc_fail=%" PRIu64
            " fallback_hot=%" PRIu64 " decommit=%d"
            " pool_lazy=%d pool_allocated=%" PRIu64
            " pool_committed_bytes=%" PRIu64
            " chunk=%d\n",
            atomic_load_explicit(&g_s65_cold_ext_node_inuse, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ext_node_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ext_node_alloc_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s65_cold_ext_fallback_hot, memory_order_relaxed),
            HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE,
            HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_POOL_LAZY,
            cold_node_pool_allocated,
            cold_node_pool_allocated * (uint64_t)sizeof(Hz3CentralColdNode),
            HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CHUNK);
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
            " min_pages=%d"
            " max_pages=%d"
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
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_MIN_PAGES,
            HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_MAX_PAGES,
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
