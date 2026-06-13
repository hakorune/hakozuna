static _Atomic size_t g_hz3_large_obs_normal_calls = 0;
static _Atomic size_t g_hz3_large_obs_aligned_calls = 0;
static _Atomic size_t g_hz3_large_obs_aligned_invalid = 0;
static _Atomic size_t g_hz3_large_obs_size_lt4k = 0;
static _Atomic size_t g_hz3_large_obs_size_4k_64k = 0;
static _Atomic size_t g_hz3_large_obs_size_64k_1m = 0;
static _Atomic size_t g_hz3_large_obs_size_gt1m = 0;
static _Atomic size_t g_hz3_large_obs_align_le16 = 0;
static _Atomic size_t g_hz3_large_obs_align_le4096 = 0;
static _Atomic size_t g_hz3_large_obs_align_le8192 = 0;
static _Atomic size_t g_hz3_large_obs_align_gt8192 = 0;
static _Atomic size_t g_hz3_large_obs_medium_candidate = 0;
static _Atomic size_t g_hz3_large_obs_small_overaligned = 0;
static _Atomic size_t g_hz3_large_obs_large_over64k = 0;
static _Atomic size_t g_hz3_large_obs_over_page_aligned = 0;
static _Atomic size_t g_hz3_large_obs_aligned_cache_hits = 0;
static _Atomic size_t g_hz3_large_obs_aligned_cache_misses = 0;
static _Atomic size_t g_hz3_large_obs_aligned_mmap_calls = 0;
static _Atomic size_t g_hz3_large_obs_aligned_mmap_bytes = 0;
static _Atomic size_t g_hz3_large_obs_aligned_batch_mmap_calls = 0;
static _Atomic size_t g_hz3_large_obs_aligned_batch_mmap_bytes = 0;
static _Atomic size_t g_hz3_large_obs_free_calls = 0;
static _Atomic size_t g_hz3_large_obs_free_shifted = 0;
static _Atomic size_t g_hz3_large_obs_free_unshifted = 0;
static _Atomic size_t g_hz3_large_obs_take_hits = 0;
static _Atomic size_t g_hz3_large_obs_take_misses = 0;
static _Atomic size_t g_hz3_large_obs_take_scan_steps = 0;
static _Atomic size_t g_hz3_large_obs_admit_invalid = 0;
static _Atomic size_t g_hz3_large_obs_admit_oversize = 0;
static _Atomic size_t g_hz3_large_obs_admit_precheck_yes = 0;
static _Atomic size_t g_hz3_large_obs_admit_lock_yes = 0;
static _Atomic size_t g_hz3_large_obs_admit_lock_no = 0;
static _Atomic size_t g_hz3_large_obs_admit_reuse_skip = 0;
static _Atomic size_t g_hz3_large_obs_admit_final_fit_fail = 0;
static _Atomic size_t g_hz3_large_obs_admit_cache_insert = 0;
static _Atomic size_t g_hz3_large_obs_admit_do_munmap = 0;
static _Atomic size_t g_hz3_large_obs_admit_defer_munmap = 0;
static _Atomic size_t g_hz3_large_obs_map_insert_calls = 0;
static _Atomic size_t g_hz3_large_obs_map_insert_ns_total = 0;
static _Atomic size_t g_hz3_large_obs_map_insert_ns_max = 0;
static _Atomic size_t g_hz3_large_obs_map_take_calls = 0;
static _Atomic size_t g_hz3_large_obs_map_take_ns_total = 0;
static _Atomic size_t g_hz3_large_obs_map_take_ns_max = 0;
static _Atomic size_t g_hz3_large_obs_alloc_cache_pop_calls = 0;
static _Atomic size_t g_hz3_large_obs_alloc_cache_pop_ns_total = 0;
static _Atomic size_t g_hz3_large_obs_alloc_cache_pop_ns_max = 0;
static _Atomic size_t g_hz3_large_obs_free_admit_calls = 0;
static _Atomic size_t g_hz3_large_obs_free_admit_ns_total = 0;
static _Atomic size_t g_hz3_large_obs_free_admit_ns_max = 0;

#if HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS
static _Atomic size_t g_s300_route_aligned_calls = 0;
static _Atomic size_t g_s300_route_align8192_calls = 0;
static _Atomic size_t g_s300_route_eligible_total = 0;
static _Atomic size_t g_s300_route_eligible_4k_a8192 = 0;
static _Atomic size_t g_s300_route_eligible_8k_a8192 = 0;
static _Atomic size_t g_s300_route_eligible_64k_a8192 = 0;
static _Atomic size_t g_s300_route_large_current = 0;
static _Atomic size_t g_s300_route_medium_candidate = 0;
static _Atomic size_t g_s300_route_noncandidate = 0;
#endif

#if HZ5_P0_ROUTE_SHADOW
static _Atomic size_t g_hz5_p0_route_candidate_4k_a8192 = 0;
static _Atomic size_t g_hz5_p0_route_candidate_8k_a8192 = 0;
static _Atomic size_t g_hz5_p0_route_candidate_64k_a8192 = 0;
static _Atomic size_t g_hz5_p0_route_candidate_total = 0;
static _Atomic size_t g_hz5_p0_route_taken = 0;
static _Atomic size_t g_hz5_p0_route_fallback_hz3 = 0;
static _Atomic size_t g_hz5_p0_true_large_fallback = 0;
static _Atomic size_t g_hz5_p0_route_reject_a4096_guard = 0;
static _Atomic size_t g_hz5_p0_route_reject_other = 0;
#endif

#if defined(_WIN32)
static hz3_once_t g_hz3_large_obs_qpc_once = HZ3_ONCE_INIT;
static int64_t g_hz3_large_obs_qpc_freq = 0;

static void hz3_large_obs_qpc_init(void) {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
        g_hz3_large_obs_qpc_freq = freq.QuadPart;
    }
}
#endif

static inline void hz3_large_aligned_obs_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline void hz3_large_aligned_obs_add(_Atomic size_t* p, size_t v) {
    atomic_fetch_add_explicit(p, v, memory_order_relaxed);
}

static inline int hz3_s300_route_eligible(size_t size, size_t alignment) {
#if HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS
    return alignment == 8192u && size >= 4096u && size <= 65536u;
#else
    (void)size;
    (void)alignment;
    return 0;
#endif
}

static inline void hz3_s300_route_obs_on_aligned_alloc(size_t size, size_t alignment) {
#if HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS
    hz3_large_aligned_obs_inc(&g_s300_route_aligned_calls);
    if (alignment == 8192u) {
        hz3_large_aligned_obs_inc(&g_s300_route_align8192_calls);
    }
    if (!hz3_s300_route_eligible(size, alignment)) {
        hz3_large_aligned_obs_inc(&g_s300_route_noncandidate);
        return;
    }

    hz3_large_aligned_obs_inc(&g_s300_route_eligible_total);
    hz3_large_aligned_obs_inc(&g_s300_route_large_current);
    hz3_large_aligned_obs_inc(&g_s300_route_medium_candidate);
    if (size == 4096u) {
        hz3_large_aligned_obs_inc(&g_s300_route_eligible_4k_a8192);
    } else if (size == 8192u) {
        hz3_large_aligned_obs_inc(&g_s300_route_eligible_8k_a8192);
    } else if (size == 65536u) {
        hz3_large_aligned_obs_inc(&g_s300_route_eligible_64k_a8192);
    }
#else
    (void)size;
    (void)alignment;
#endif
}

static inline int hz5_p0_route_candidate(size_t size, size_t alignment) {
#if HZ5_P0_ROUTE_SHADOW
    return alignment == 8192u &&
           (size == 4096u || size == 8192u || size == 65536u);
#else
    (void)size;
    (void)alignment;
    return 0;
#endif
}

static inline void hz5_p0_route_shadow_on_aligned_alloc(size_t size, size_t alignment) {
#if HZ5_P0_ROUTE_SHADOW
    if (hz5_p0_route_candidate(size, alignment)) {
        hz3_large_aligned_obs_inc(&g_hz5_p0_route_candidate_total);
        if (size == 4096u) {
            hz3_large_aligned_obs_inc(&g_hz5_p0_route_candidate_4k_a8192);
        } else if (size == 8192u) {
            hz3_large_aligned_obs_inc(&g_hz5_p0_route_candidate_8k_a8192);
        } else {
            hz3_large_aligned_obs_inc(&g_hz5_p0_route_candidate_64k_a8192);
        }
    } else if (alignment == 4096u && size >= 4096u && size <= 65536u) {
        hz3_large_aligned_obs_inc(&g_hz5_p0_route_reject_a4096_guard);
    } else {
        hz3_large_aligned_obs_inc(&g_hz5_p0_route_reject_other);
    }

    // P0 is route-shadow only: every allocation still falls back to HZ3.
    hz3_large_aligned_obs_inc(&g_hz5_p0_route_fallback_hz3);
    if (size > (512u << 10) || alignment > (64u << 10)) {
        hz3_large_aligned_obs_inc(&g_hz5_p0_true_large_fallback);
    }
#else
    (void)size;
    (void)alignment;
#endif
}

static inline size_t hz3_large_aligned_obs_now_ns(void) {
#if defined(_WIN32)
    hz3_once(&g_hz3_large_obs_qpc_once, hz3_large_obs_qpc_init);
    if (g_hz3_large_obs_qpc_freq <= 0) {
        return 0;
    }
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }
    return (size_t)(((double)counter.QuadPart * 1000000000.0) /
                    (double)g_hz3_large_obs_qpc_freq);
#else
    return 0;
#endif
}

static inline void hz3_large_aligned_obs_update_max(_Atomic size_t* p, size_t value) {
    size_t old = atomic_load_explicit(p, memory_order_relaxed);
    while (value > old &&
           !atomic_compare_exchange_weak_explicit(
               p, &old, value, memory_order_relaxed, memory_order_relaxed)) {
    }
}

static inline void hz3_large_aligned_obs_note_elapsed(_Atomic size_t* calls,
                                                       _Atomic size_t* ns_total,
                                                       _Atomic size_t* ns_max,
                                                       size_t start_ns) {
    if (start_ns == 0) {
        return;
    }
    size_t end_ns = hz3_large_aligned_obs_now_ns();
    if (end_ns < start_ns) {
        return;
    }
    size_t elapsed = end_ns - start_ns;
    hz3_large_aligned_obs_inc(calls);
    hz3_large_aligned_obs_add(ns_total, elapsed);
    hz3_large_aligned_obs_update_max(ns_max, elapsed);
}

static inline size_t hz3_large_aligned_obs_timing_start(void) {
    return hz3_large_aligned_obs_now_ns();
}

static inline void hz3_large_aligned_obs_on_map_insert_elapsed(size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &g_hz3_large_obs_map_insert_calls,
        &g_hz3_large_obs_map_insert_ns_total,
        &g_hz3_large_obs_map_insert_ns_max,
        start_ns);
}

static inline void hz3_large_aligned_obs_on_map_take_elapsed(size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &g_hz3_large_obs_map_take_calls,
        &g_hz3_large_obs_map_take_ns_total,
        &g_hz3_large_obs_map_take_ns_max,
        start_ns);
}

static inline void hz3_large_aligned_obs_on_alloc_cache_pop_elapsed(size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &g_hz3_large_obs_alloc_cache_pop_calls,
        &g_hz3_large_obs_alloc_cache_pop_ns_total,
        &g_hz3_large_obs_alloc_cache_pop_ns_max,
        start_ns);
}

static inline void hz3_large_aligned_obs_on_free_admit_elapsed(size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &g_hz3_large_obs_free_admit_calls,
        &g_hz3_large_obs_free_admit_ns_total,
        &g_hz3_large_obs_free_admit_ns_max,
        start_ns);
}

