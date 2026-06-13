static inline size_t hz3_large_total_cached_load(void) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    return atomic_load_explicit(&g_total_cached_bytes, memory_order_relaxed);
#else
    return g_total_cached_bytes;
#endif
}

static inline void hz3_large_total_cached_add(size_t bytes) {
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    size_t current;
#endif
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    current = atomic_fetch_add_explicit(&g_total_cached_bytes,
                                        bytes,
                                        memory_order_relaxed) + bytes;
#else
    atomic_fetch_add_explicit(&g_total_cached_bytes,
                              bytes,
                              memory_order_relaxed);
#endif
#else
    g_total_cached_bytes += bytes;
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    current = g_total_cached_bytes;
#endif
#endif
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    hz3_s253_update_global_cached_peak(current);
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
    atomic_fetch_add_explicit(&g_total_cached_bytes_hint, bytes, memory_order_relaxed);
#endif
}

static inline void hz3_large_total_cached_sub(size_t bytes) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    atomic_fetch_sub_explicit(&g_total_cached_bytes, bytes, memory_order_relaxed);
#else
    g_total_cached_bytes -= bytes;
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
    atomic_fetch_sub_explicit(&g_total_cached_bytes_hint, bytes, memory_order_relaxed);
#endif
}

#if HZ3_S267_LARGE_RSS_PLATEAU_OBS
static _Atomic size_t g_s267_global_cached_bytes_peak = 0;
static _Atomic size_t g_s267_global_cached_nodes_current = 0;
static _Atomic size_t g_s267_global_cached_nodes_peak = 0;
static _Atomic size_t g_s267_sc_bytes_peak[HZ3_LARGE_SC_COUNT];
static _Atomic size_t g_s267_sc_nodes_peak[HZ3_LARGE_SC_COUNT];
static _Atomic size_t g_s267_defer_push = 0;
static _Atomic size_t g_s267_defer_pop = 0;
static _Atomic size_t g_s267_defer_reject = 0;
static _Atomic size_t g_s267_defer_bytes_current = 0;
static _Atomic size_t g_s267_defer_bytes_peak = 0;

static inline void hz3_s267_update_peak(_Atomic size_t* slot, size_t value) {
    size_t cur = atomic_load_explicit(slot, memory_order_relaxed);
    while (value > cur) {
        if (atomic_compare_exchange_weak_explicit(
                slot, &cur, value, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_s267_large_rss_dump_final(void) {
    size_t current_nodes =
        atomic_load_explicit(&g_s267_global_cached_nodes_current, memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_S267_LARGE_RSS] cache_enable=%d scache=%d cache_budget=%d "
            "soft_bytes=%zu hard_bytes=%zu cache_max_bytes=%zu cache_max_nodes=%u "
            "cached_bytes_current=%zu cached_bytes_peak=%zu cached_nodes_current=%zu "
            "cached_nodes_peak=%zu defer_push=%zu defer_pop=%zu defer_reject=%zu "
            "defer_bytes_current=%zu defer_bytes_peak=%zu s186_defer=%d "
            "s212_defer_plus=%d s212_defer_max_bytes=%zu s212_free_budget=%u "
            "s212_drain_on_miss=%d s212_miss_stride=%u s212_miss_budget=%u "
            "s212_trigger_bytes=%zu\n",
            HZ3_LARGE_CACHE_ENABLE,
            HZ3_S50_LARGE_SCACHE,
            HZ3_LARGE_CACHE_BUDGET,
            (size_t)HZ3_LARGE_CACHE_SOFT_BYTES,
            (size_t)HZ3_LARGE_CACHE_HARD_BYTES,
            (size_t)HZ3_LARGE_CACHE_MAX_BYTES,
            (unsigned)HZ3_LARGE_CACHE_MAX_NODES,
            hz3_large_total_cached_load(),
            atomic_load_explicit(&g_s267_global_cached_bytes_peak, memory_order_relaxed),
            current_nodes,
            atomic_load_explicit(&g_s267_global_cached_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_push, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_pop, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s267_defer_bytes_peak, memory_order_relaxed),
            HZ3_S186_LARGE_UNMAP_DEFER,
            HZ3_S212_LARGE_UNMAP_DEFER_PLUS,
            (size_t)HZ3_S212_UNMAP_DEFER_MAX_BYTES,
            (unsigned)HZ3_S212_UNMAP_DEFER_FREE_BUDGET,
            HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS,
            (unsigned)HZ3_S212_DEFER_DRAIN_MISS_STRIDE,
            (unsigned)HZ3_S212_DEFER_DRAIN_MISS_BUDGET,
            (size_t)HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES);

    int emitted = 0;
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        size_t bytes_peak =
            atomic_load_explicit(&g_s267_sc_bytes_peak[sc], memory_order_relaxed);
        size_t nodes_peak =
            atomic_load_explicit(&g_s267_sc_nodes_peak[sc], memory_order_relaxed);
        if (g_sc_bytes[sc] == 0 && g_sc_nodes[sc] == 0 &&
            bytes_peak == 0 && nodes_peak == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S267_LARGE_RSS_SC] sc=%d bytes_current=%zu bytes_peak=%zu "
                "nodes_current=%u nodes_peak=%zu\n",
                sc,
                g_sc_bytes[sc],
                bytes_peak,
                g_sc_nodes[sc],
                nodes_peak);
        emitted = 1;
    }
    if (!emitted) {
        fprintf(stderr, "[HZ3_S267_LARGE_RSS_SC] empty=1\n");
    }
}

static inline void hz3_s267_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s267_large_rss_dump_final);
    }
}

static inline void hz3_s267_note_scache_state_locked(int sc) {
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return;
    }
    hz3_s267_register_once();
    hz3_s267_update_peak(&g_s267_global_cached_bytes_peak,
                         hz3_large_total_cached_load());
    hz3_s267_update_peak(&g_s267_sc_bytes_peak[sc], g_sc_bytes[sc]);
    hz3_s267_update_peak(&g_s267_sc_nodes_peak[sc], (size_t)g_sc_nodes[sc]);
}

static inline void hz3_s267_on_scache_push(int sc, size_t bytes) {
    (void)bytes;
    size_t nodes =
        atomic_fetch_add_explicit(&g_s267_global_cached_nodes_current,
                                  1,
                                  memory_order_relaxed) + 1;
    hz3_s267_update_peak(&g_s267_global_cached_nodes_peak, nodes);
    hz3_s267_note_scache_state_locked(sc);
}

static inline void hz3_s267_on_scache_pop(int sc, size_t bytes) {
    (void)bytes;
    atomic_fetch_sub_explicit(&g_s267_global_cached_nodes_current,
                              1,
                              memory_order_relaxed);
    hz3_s267_note_scache_state_locked(sc);
}

static inline void hz3_s267_on_defer_push(size_t bytes, size_t current_bytes) {
    hz3_s267_register_once();
    atomic_fetch_add_explicit(&g_s267_defer_push, 1, memory_order_relaxed);
    atomic_store_explicit(&g_s267_defer_bytes_current,
                          current_bytes,
                          memory_order_relaxed);
    hz3_s267_update_peak(&g_s267_defer_bytes_peak, current_bytes);
    (void)bytes;
}

static inline void hz3_s267_on_defer_pop(size_t bytes, size_t current_bytes) {
    hz3_s267_register_once();
    atomic_fetch_add_explicit(&g_s267_defer_pop, 1, memory_order_relaxed);
    atomic_store_explicit(&g_s267_defer_bytes_current,
                          current_bytes,
                          memory_order_relaxed);
    (void)bytes;
}

static inline void hz3_s267_on_defer_reject(size_t bytes, size_t current_bytes) {
    hz3_s267_register_once();
    atomic_fetch_add_explicit(&g_s267_defer_reject, 1, memory_order_relaxed);
    atomic_store_explicit(&g_s267_defer_bytes_current,
                          current_bytes,
                          memory_order_relaxed);
    (void)bytes;
}
#else
static inline void hz3_s267_on_scache_push(int sc, size_t bytes) {
    (void)sc;
    (void)bytes;
}

static inline void hz3_s267_on_scache_pop(int sc, size_t bytes) {
    (void)sc;
    (void)bytes;
}

static inline void hz3_s267_on_defer_push(size_t bytes, size_t current_bytes) {
    (void)bytes;
    (void)current_bytes;
}

static inline void hz3_s267_on_defer_pop(size_t bytes, size_t current_bytes) {
    (void)bytes;
    (void)current_bytes;
}

static inline void hz3_s267_on_defer_reject(size_t bytes, size_t current_bytes) {
    (void)bytes;
    (void)current_bytes;
}
#endif
