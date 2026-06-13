#if HZ3_S243_LARGE_RESIDUAL_OBS
typedef struct Hz3S243ResidualTimer {
    _Atomic size_t calls;
    _Atomic size_t ns_total;
    _Atomic size_t ns_max;
} Hz3S243ResidualTimer;

static _Atomic size_t g_s243_alloc_front_first_hit = 0;
static _Atomic size_t g_s243_alloc_front_first_miss = 0;
static _Atomic size_t g_s243_alloc_inbox_drain_calls = 0;
static _Atomic size_t g_s243_alloc_inbox_drain_objs = 0;
static _Atomic size_t g_s243_alloc_front_after_drain_hit = 0;
static _Atomic size_t g_s243_alloc_front_after_drain_miss = 0;
static _Atomic size_t g_s243_alloc_global_cache_hit = 0;
static _Atomic size_t g_s243_alloc_mmap_path = 0;

static _Atomic size_t g_s243_free_take_hit = 0;
static _Atomic size_t g_s243_free_take_miss = 0;
static _Atomic size_t g_s243_free_same_owner_front_success = 0;
static _Atomic size_t g_s243_free_same_owner_front_reject = 0;
static _Atomic size_t g_s243_free_remote_inbox_success = 0;
static _Atomic size_t g_s243_free_remote_inbox_reject = 0;
static _Atomic size_t g_s243_free_fallback_global = 0;

static Hz3S243ResidualTimer g_s243_t_alloc_front_first;
static Hz3S243ResidualTimer g_s243_t_alloc_inbox_drain;
static Hz3S243ResidualTimer g_s243_t_alloc_front_after_drain;
static Hz3S243ResidualTimer g_s243_t_alloc_activate_front;
static Hz3S243ResidualTimer g_s243_t_alloc_activate_inbox_front;
static Hz3S243ResidualTimer g_s243_t_alloc_activate_global_cache;
static Hz3S243ResidualTimer g_s243_t_alloc_mmap_activate;

static Hz3S243ResidualTimer g_s243_t_free_total;
static Hz3S243ResidualTimer g_s243_t_free_take;
static Hz3S243ResidualTimer g_s243_t_free_s240_meta;
static Hz3S243ResidualTimer g_s243_t_free_front_push;
static Hz3S243ResidualTimer g_s243_t_free_inbox_push;
static Hz3S243ResidualTimer g_s243_t_free_fallback_global;

static inline size_t hz3_s243_residual_timing_start(void) {
    return hz3_large_aligned_obs_timing_start();
}

static inline void hz3_s243_residual_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline void hz3_s243_residual_add(_Atomic size_t* p, size_t v) {
    atomic_fetch_add_explicit(p, v, memory_order_relaxed);
}

static inline void hz3_s243_residual_note_timer(Hz3S243ResidualTimer* timer,
                                                size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &timer->calls, &timer->ns_total, &timer->ns_max, start_ns);
}

static inline size_t hz3_s243_residual_load(const _Atomic size_t* p) {
    return atomic_load_explicit(p, memory_order_relaxed);
}

static inline size_t hz3_s243_residual_timer_calls(Hz3S243ResidualTimer* timer) {
    return hz3_s243_residual_load(&timer->calls);
}

static inline size_t hz3_s243_residual_timer_total(Hz3S243ResidualTimer* timer) {
    return hz3_s243_residual_load(&timer->ns_total);
}

static inline size_t hz3_s243_residual_timer_max(Hz3S243ResidualTimer* timer) {
    return hz3_s243_residual_load(&timer->ns_max);
}

static inline size_t hz3_s243_residual_timer_avg(Hz3S243ResidualTimer* timer) {
    size_t calls = hz3_s243_residual_timer_calls(timer);
    size_t total = hz3_s243_residual_timer_total(timer);
    return calls ? (total / calls) : 0;
}

static inline void hz3_s243_residual_on_alloc_front_first_elapsed(size_t start_ns,
                                                                   int hit) {
    hz3_s243_residual_inc(hit ? &g_s243_alloc_front_first_hit :
                                &g_s243_alloc_front_first_miss);
    hz3_s243_residual_note_timer(&g_s243_t_alloc_front_first, start_ns);
}

static inline void hz3_s243_residual_on_alloc_inbox_drain_elapsed(size_t start_ns,
                                                                   int drained) {
    hz3_s243_residual_inc(&g_s243_alloc_inbox_drain_calls);
    if (drained > 0) {
        hz3_s243_residual_add(&g_s243_alloc_inbox_drain_objs, (size_t)drained);
    }
    hz3_s243_residual_note_timer(&g_s243_t_alloc_inbox_drain, start_ns);
}

static inline void hz3_s243_residual_on_alloc_front_after_drain_elapsed(size_t start_ns,
                                                                        int hit) {
    hz3_s243_residual_inc(hit ? &g_s243_alloc_front_after_drain_hit :
                                &g_s243_alloc_front_after_drain_miss);
    hz3_s243_residual_note_timer(&g_s243_t_alloc_front_after_drain, start_ns);
}

static inline void hz3_s243_residual_on_alloc_activate_front_elapsed(size_t start_ns) {
    hz3_s243_residual_note_timer(&g_s243_t_alloc_activate_front, start_ns);
}

static inline void hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(size_t start_ns) {
    hz3_s243_residual_note_timer(&g_s243_t_alloc_activate_inbox_front, start_ns);
}

static inline void hz3_s243_residual_on_alloc_global_cache_hit_elapsed(size_t start_ns) {
    hz3_s243_residual_inc(&g_s243_alloc_global_cache_hit);
    hz3_s243_residual_note_timer(&g_s243_t_alloc_activate_global_cache, start_ns);
}

static inline void hz3_s243_residual_on_alloc_mmap_activate_elapsed(size_t start_ns) {
    hz3_s243_residual_inc(&g_s243_alloc_mmap_path);
    hz3_s243_residual_note_timer(&g_s243_t_alloc_mmap_activate, start_ns);
}

static inline void hz3_s243_residual_on_free_take_elapsed(size_t start_ns,
                                                          int hit) {
    hz3_s243_residual_inc(hit ? &g_s243_free_take_hit : &g_s243_free_take_miss);
    hz3_s243_residual_note_timer(&g_s243_t_free_take, start_ns);
}

static inline void hz3_s243_residual_on_free_s240_meta_elapsed(size_t start_ns) {
    hz3_s243_residual_note_timer(&g_s243_t_free_s240_meta, start_ns);
}

static inline void hz3_s243_residual_on_free_front_push_elapsed(size_t start_ns,
                                                                int success) {
    hz3_s243_residual_inc(success ? &g_s243_free_same_owner_front_success :
                                    &g_s243_free_same_owner_front_reject);
    hz3_s243_residual_note_timer(&g_s243_t_free_front_push, start_ns);
}

static inline void hz3_s243_residual_on_free_inbox_push_elapsed(size_t start_ns,
                                                                int success) {
    hz3_s243_residual_inc(success ? &g_s243_free_remote_inbox_success :
                                    &g_s243_free_remote_inbox_reject);
    hz3_s243_residual_note_timer(&g_s243_t_free_inbox_push, start_ns);
}

static inline void hz3_s243_residual_on_free_fallback_global_elapsed(size_t start_ns) {
    hz3_s243_residual_inc(&g_s243_free_fallback_global);
    hz3_s243_residual_note_timer(&g_s243_t_free_fallback_global, start_ns);
}

static inline void hz3_s243_residual_on_free_total_elapsed(size_t start_ns) {
    hz3_s243_residual_note_timer(&g_s243_t_free_total, start_ns);
}

static void hz3_s243_residual_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S243_RESIDUAL_COUNTS] "
            "alloc_front_first_hit=%zu alloc_front_first_miss=%zu "
            "alloc_inbox_drain_calls=%zu alloc_inbox_drain_objs=%zu "
            "alloc_front_after_drain_hit=%zu alloc_front_after_drain_miss=%zu "
            "alloc_global_cache_hit=%zu alloc_mmap_path=%zu "
            "free_take_hit=%zu free_take_miss=%zu "
            "free_same_owner_front_success=%zu free_same_owner_front_reject=%zu "
            "free_remote_inbox_success=%zu free_remote_inbox_reject=%zu "
            "free_fallback_global=%zu\n",
            hz3_s243_residual_load(&g_s243_alloc_front_first_hit),
            hz3_s243_residual_load(&g_s243_alloc_front_first_miss),
            hz3_s243_residual_load(&g_s243_alloc_inbox_drain_calls),
            hz3_s243_residual_load(&g_s243_alloc_inbox_drain_objs),
            hz3_s243_residual_load(&g_s243_alloc_front_after_drain_hit),
            hz3_s243_residual_load(&g_s243_alloc_front_after_drain_miss),
            hz3_s243_residual_load(&g_s243_alloc_global_cache_hit),
            hz3_s243_residual_load(&g_s243_alloc_mmap_path),
            hz3_s243_residual_load(&g_s243_free_take_hit),
            hz3_s243_residual_load(&g_s243_free_take_miss),
            hz3_s243_residual_load(&g_s243_free_same_owner_front_success),
            hz3_s243_residual_load(&g_s243_free_same_owner_front_reject),
            hz3_s243_residual_load(&g_s243_free_remote_inbox_success),
            hz3_s243_residual_load(&g_s243_free_remote_inbox_reject),
            hz3_s243_residual_load(&g_s243_free_fallback_global));
    fprintf(stderr,
            "[HZ3_S243_RESIDUAL_TIMING] "
            "alloc_front_first_calls=%zu alloc_front_first_ns_total=%zu "
            "alloc_front_first_ns_avg=%zu alloc_front_first_ns_max=%zu "
            "alloc_inbox_drain_calls=%zu alloc_inbox_drain_ns_total=%zu "
            "alloc_inbox_drain_ns_avg=%zu alloc_inbox_drain_ns_max=%zu "
            "alloc_front_after_drain_calls=%zu alloc_front_after_drain_ns_total=%zu "
            "alloc_front_after_drain_ns_avg=%zu alloc_front_after_drain_ns_max=%zu "
            "alloc_activate_front_calls=%zu alloc_activate_front_ns_total=%zu "
            "alloc_activate_front_ns_avg=%zu alloc_activate_front_ns_max=%zu "
            "alloc_activate_inbox_front_calls=%zu alloc_activate_inbox_front_ns_total=%zu "
            "alloc_activate_inbox_front_ns_avg=%zu alloc_activate_inbox_front_ns_max=%zu "
            "alloc_activate_global_cache_calls=%zu alloc_activate_global_cache_ns_total=%zu "
            "alloc_activate_global_cache_ns_avg=%zu alloc_activate_global_cache_ns_max=%zu "
            "alloc_mmap_activate_calls=%zu alloc_mmap_activate_ns_total=%zu "
            "alloc_mmap_activate_ns_avg=%zu alloc_mmap_activate_ns_max=%zu "
            "free_total_calls=%zu free_total_ns_total=%zu free_total_ns_avg=%zu "
            "free_total_ns_max=%zu free_take_calls=%zu free_take_ns_total=%zu "
            "free_take_ns_avg=%zu free_take_ns_max=%zu free_s240_meta_calls=%zu "
            "free_s240_meta_ns_total=%zu free_s240_meta_ns_avg=%zu "
            "free_s240_meta_ns_max=%zu free_front_push_calls=%zu "
            "free_front_push_ns_total=%zu free_front_push_ns_avg=%zu "
            "free_front_push_ns_max=%zu free_inbox_push_calls=%zu "
            "free_inbox_push_ns_total=%zu free_inbox_push_ns_avg=%zu "
            "free_inbox_push_ns_max=%zu free_fallback_global_calls=%zu "
            "free_fallback_global_ns_total=%zu free_fallback_global_ns_avg=%zu "
            "free_fallback_global_ns_max=%zu\n",
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_front_first),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_front_first),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_front_first),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_front_first),
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_inbox_drain),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_inbox_drain),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_inbox_drain),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_inbox_drain),
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_front_after_drain),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_front_after_drain),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_front_after_drain),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_front_after_drain),
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_activate_front),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_activate_front),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_activate_front),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_activate_front),
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_activate_inbox_front),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_activate_inbox_front),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_activate_inbox_front),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_activate_inbox_front),
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_activate_global_cache),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_activate_global_cache),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_activate_global_cache),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_activate_global_cache),
            hz3_s243_residual_timer_calls(&g_s243_t_alloc_mmap_activate),
            hz3_s243_residual_timer_total(&g_s243_t_alloc_mmap_activate),
            hz3_s243_residual_timer_avg(&g_s243_t_alloc_mmap_activate),
            hz3_s243_residual_timer_max(&g_s243_t_alloc_mmap_activate),
            hz3_s243_residual_timer_calls(&g_s243_t_free_total),
            hz3_s243_residual_timer_total(&g_s243_t_free_total),
            hz3_s243_residual_timer_avg(&g_s243_t_free_total),
            hz3_s243_residual_timer_max(&g_s243_t_free_total),
            hz3_s243_residual_timer_calls(&g_s243_t_free_take),
            hz3_s243_residual_timer_total(&g_s243_t_free_take),
            hz3_s243_residual_timer_avg(&g_s243_t_free_take),
            hz3_s243_residual_timer_max(&g_s243_t_free_take),
            hz3_s243_residual_timer_calls(&g_s243_t_free_s240_meta),
            hz3_s243_residual_timer_total(&g_s243_t_free_s240_meta),
            hz3_s243_residual_timer_avg(&g_s243_t_free_s240_meta),
            hz3_s243_residual_timer_max(&g_s243_t_free_s240_meta),
            hz3_s243_residual_timer_calls(&g_s243_t_free_front_push),
            hz3_s243_residual_timer_total(&g_s243_t_free_front_push),
            hz3_s243_residual_timer_avg(&g_s243_t_free_front_push),
            hz3_s243_residual_timer_max(&g_s243_t_free_front_push),
            hz3_s243_residual_timer_calls(&g_s243_t_free_inbox_push),
            hz3_s243_residual_timer_total(&g_s243_t_free_inbox_push),
            hz3_s243_residual_timer_avg(&g_s243_t_free_inbox_push),
            hz3_s243_residual_timer_max(&g_s243_t_free_inbox_push),
            hz3_s243_residual_timer_calls(&g_s243_t_free_fallback_global),
            hz3_s243_residual_timer_total(&g_s243_t_free_fallback_global),
            hz3_s243_residual_timer_avg(&g_s243_t_free_fallback_global),
            hz3_s243_residual_timer_max(&g_s243_t_free_fallback_global));
}
#else
static inline size_t hz3_s243_residual_timing_start(void) { return 0; }
static inline void hz3_s243_residual_on_alloc_front_first_elapsed(size_t start_ns,
                                                                   int hit) {
    (void)start_ns; (void)hit;
}
static inline void hz3_s243_residual_on_alloc_inbox_drain_elapsed(size_t start_ns,
                                                                   int drained) {
    (void)start_ns; (void)drained;
}
static inline void hz3_s243_residual_on_alloc_front_after_drain_elapsed(size_t start_ns,
                                                                        int hit) {
    (void)start_ns; (void)hit;
}
static inline void hz3_s243_residual_on_alloc_activate_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_alloc_global_cache_hit_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_alloc_mmap_activate_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_free_take_elapsed(size_t start_ns, int hit) {
    (void)start_ns; (void)hit;
}
static inline void hz3_s243_residual_on_free_s240_meta_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_free_front_push_elapsed(size_t start_ns,
                                                                int success) {
    (void)start_ns; (void)success;
}
static inline void hz3_s243_residual_on_free_inbox_push_elapsed(size_t start_ns,
                                                                int success) {
    (void)start_ns; (void)success;
}
static inline void hz3_s243_residual_on_free_fallback_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_free_total_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_dump_final(void) {}
#endif

