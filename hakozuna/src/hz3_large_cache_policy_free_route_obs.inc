#if HZ3_S280_LARGE_RETAINED_FREE_ROUTE_OBS
static _Atomic size_t g_s280_free_taken_retained = 0;
static _Atomic size_t g_s280_free_taken_nonretained = 0;
static _Atomic size_t g_s280_free_sc_invalid = 0;
static _Atomic size_t g_s280_free_sc_oversize = 0;
static _Atomic size_t g_s280_free_route_same_owner = 0;
static _Atomic size_t g_s280_free_route_remote_owner = 0;
static _Atomic size_t g_s280_free_route_unknown_owner = 0;
static _Atomic size_t g_s280_front_try = 0;
static _Atomic size_t g_s280_front_success = 0;
static _Atomic size_t g_s280_front_reject = 0;
static _Atomic size_t g_s280_front_retained_success = 0;
static _Atomic size_t g_s280_front_retained_clear = 0;
static _Atomic size_t g_s280_inbox_try = 0;
static _Atomic size_t g_s280_inbox_success = 0;
static _Atomic size_t g_s280_inbox_reject = 0;
static _Atomic size_t g_s280_inbox_retained_success = 0;
static _Atomic size_t g_s280_inbox_retained_clear = 0;
static _Atomic size_t g_s280_global_fallback = 0;
static _Atomic size_t g_s280_global_fallback_after_front = 0;
static _Atomic size_t g_s280_global_fallback_after_inbox = 0;
static _Atomic size_t g_s280_global_fallback_no_fast_route = 0;
static _Atomic size_t g_s280_munmap_path = 0;

static inline void hz3_s280_free_route_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline void hz3_s280_free_route_on_taken(const Hz3LargeHdr* hdr) {
    if (hdr && (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
        hz3_s280_free_route_inc(&g_s280_free_taken_retained);
    } else {
        hz3_s280_free_route_inc(&g_s280_free_taken_nonretained);
    }
}

static inline void hz3_s280_free_route_on_sc_invalid(void) {
    hz3_s280_free_route_inc(&g_s280_free_sc_invalid);
}

static inline void hz3_s280_free_route_on_sc_oversize(void) {
    hz3_s280_free_route_inc(&g_s280_free_sc_oversize);
}

static inline void hz3_s280_free_route_on_same_owner(void) {
    hz3_s280_free_route_inc(&g_s280_free_route_same_owner);
}

static inline void hz3_s280_free_route_on_remote_owner(void) {
    hz3_s280_free_route_inc(&g_s280_free_route_remote_owner);
}

static inline void hz3_s280_free_route_on_unknown_owner(void) {
    hz3_s280_free_route_inc(&g_s280_free_route_unknown_owner);
}

static inline void hz3_s280_free_route_on_front_result(int retained,
                                                       int success) {
    hz3_s280_free_route_inc(&g_s280_front_try);
    if (success) {
        hz3_s280_free_route_inc(&g_s280_front_success);
        if (retained) {
            hz3_s280_free_route_inc(&g_s280_front_retained_success);
        }
        return;
    }
    hz3_s280_free_route_inc(&g_s280_front_reject);
    if (retained) {
        hz3_s280_free_route_inc(&g_s280_front_retained_clear);
    }
}

static inline void hz3_s280_free_route_on_inbox_result(int retained,
                                                       int success) {
    hz3_s280_free_route_inc(&g_s280_inbox_try);
    if (success) {
        hz3_s280_free_route_inc(&g_s280_inbox_success);
        if (retained) {
            hz3_s280_free_route_inc(&g_s280_inbox_retained_success);
        }
        return;
    }
    hz3_s280_free_route_inc(&g_s280_inbox_reject);
    if (retained) {
        hz3_s280_free_route_inc(&g_s280_inbox_retained_clear);
    }
}

static inline void hz3_s280_free_route_on_global_fallback(int had_fast_route,
                                                          int front_reject,
                                                          int inbox_reject) {
    hz3_s280_free_route_inc(&g_s280_global_fallback);
    if (!had_fast_route) {
        hz3_s280_free_route_inc(&g_s280_global_fallback_no_fast_route);
    }
    if (front_reject) {
        hz3_s280_free_route_inc(&g_s280_global_fallback_after_front);
    }
    if (inbox_reject) {
        hz3_s280_free_route_inc(&g_s280_global_fallback_after_inbox);
    }
}

static inline void hz3_s280_free_route_on_munmap(void) {
    hz3_s280_free_route_inc(&g_s280_munmap_path);
}

static void hz3_s280_free_route_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S280_FREE_ROUTE] "
            "taken_retained=%zu taken_nonretained=%zu sc_invalid=%zu "
            "sc_oversize=%zu same_owner=%zu remote_owner=%zu "
            "unknown_owner=%zu front_try=%zu front_success=%zu "
            "front_reject=%zu front_retained_success=%zu "
            "front_retained_clear=%zu inbox_try=%zu inbox_success=%zu "
            "inbox_reject=%zu inbox_retained_success=%zu "
            "inbox_retained_clear=%zu global_fallback=%zu "
            "global_fallback_after_front=%zu "
            "global_fallback_after_inbox=%zu "
            "global_fallback_no_fast_route=%zu munmap_path=%zu\n",
            atomic_load_explicit(&g_s280_free_taken_retained, memory_order_relaxed),
            atomic_load_explicit(&g_s280_free_taken_nonretained, memory_order_relaxed),
            atomic_load_explicit(&g_s280_free_sc_invalid, memory_order_relaxed),
            atomic_load_explicit(&g_s280_free_sc_oversize, memory_order_relaxed),
            atomic_load_explicit(&g_s280_free_route_same_owner, memory_order_relaxed),
            atomic_load_explicit(&g_s280_free_route_remote_owner, memory_order_relaxed),
            atomic_load_explicit(&g_s280_free_route_unknown_owner, memory_order_relaxed),
            atomic_load_explicit(&g_s280_front_try, memory_order_relaxed),
            atomic_load_explicit(&g_s280_front_success, memory_order_relaxed),
            atomic_load_explicit(&g_s280_front_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s280_front_retained_success, memory_order_relaxed),
            atomic_load_explicit(&g_s280_front_retained_clear, memory_order_relaxed),
            atomic_load_explicit(&g_s280_inbox_try, memory_order_relaxed),
            atomic_load_explicit(&g_s280_inbox_success, memory_order_relaxed),
            atomic_load_explicit(&g_s280_inbox_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s280_inbox_retained_success, memory_order_relaxed),
            atomic_load_explicit(&g_s280_inbox_retained_clear, memory_order_relaxed),
            atomic_load_explicit(&g_s280_global_fallback, memory_order_relaxed),
            atomic_load_explicit(&g_s280_global_fallback_after_front, memory_order_relaxed),
            atomic_load_explicit(&g_s280_global_fallback_after_inbox, memory_order_relaxed),
            atomic_load_explicit(&g_s280_global_fallback_no_fast_route, memory_order_relaxed),
            atomic_load_explicit(&g_s280_munmap_path, memory_order_relaxed));
}
#else
static inline void hz3_s280_free_route_on_taken(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s280_free_route_on_sc_invalid(void) {}
static inline void hz3_s280_free_route_on_sc_oversize(void) {}
static inline void hz3_s280_free_route_on_same_owner(void) {}
static inline void hz3_s280_free_route_on_remote_owner(void) {}
static inline void hz3_s280_free_route_on_unknown_owner(void) {}
static inline void hz3_s280_free_route_on_front_result(int retained, int success) {
    (void)retained;
    (void)success;
}
static inline void hz3_s280_free_route_on_inbox_result(int retained, int success) {
    (void)retained;
    (void)success;
}
static inline void hz3_s280_free_route_on_global_fallback(int had_fast_route,
                                                          int front_reject,
                                                          int inbox_reject) {
    (void)had_fast_route;
    (void)front_reject;
    (void)inbox_reject;
}
static inline void hz3_s280_free_route_on_munmap(void) {}
static inline void hz3_s280_free_route_dump_final(void) {}
#endif

static void hz3_large_aligned_obs_dump_final(void) {
    fprintf(stderr,
            "[HZ3_LARGE_ALIGNED_OBS_CALLS] normal=%zu aligned=%zu invalid=%zu "
            "size_lt4k=%zu size_4k_64k=%zu size_64k_1m=%zu size_gt1m=%zu "
            "align_le16=%zu align_le4096=%zu align_le8192=%zu align_gt8192=%zu "
            "medium_candidate=%zu small_overaligned=%zu large_over64k=%zu "
            "over_page_aligned=%zu\n",
            atomic_load_explicit(&g_hz3_large_obs_normal_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_invalid, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_size_lt4k, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_size_4k_64k, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_size_64k_1m, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_size_gt1m, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_align_le16, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_align_le4096, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_align_le8192, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_align_gt8192, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_medium_candidate, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_small_overaligned, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_large_over64k, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_over_page_aligned, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_LARGE_ALIGNED_OBS_PATH] aligned_cache_hits=%zu "
            "aligned_cache_misses=%zu aligned_mmap_calls=%zu "
            "aligned_mmap_bytes=%zu aligned_batch_mmap_calls=%zu "
            "aligned_batch_mmap_bytes=%zu free_calls=%zu free_shifted=%zu "
            "free_unshifted=%zu take_hits=%zu take_misses=%zu "
            "take_scan_steps=%zu\n",
            atomic_load_explicit(&g_hz3_large_obs_aligned_cache_hits, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_cache_misses, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_mmap_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_mmap_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_batch_mmap_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_aligned_batch_mmap_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_free_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_free_shifted, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_free_unshifted, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_take_hits, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_take_misses, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_take_scan_steps, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_LARGE_ALIGNED_OBS_ADMIT] invalid=%zu oversize=%zu "
            "precheck_yes=%zu lock_yes=%zu lock_no=%zu reuse_skip=%zu "
            "final_fit_fail=%zu cache_insert=%zu do_munmap=%zu "
            "defer_munmap=%zu\n",
            atomic_load_explicit(&g_hz3_large_obs_admit_invalid, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_oversize, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_precheck_yes, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_lock_yes, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_lock_no, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_reuse_skip, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_final_fit_fail, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_cache_insert, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_do_munmap, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_large_obs_admit_defer_munmap, memory_order_relaxed));
    size_t map_insert_calls =
        atomic_load_explicit(&g_hz3_large_obs_map_insert_calls, memory_order_relaxed);
    size_t map_insert_total =
        atomic_load_explicit(&g_hz3_large_obs_map_insert_ns_total, memory_order_relaxed);
    size_t map_take_calls =
        atomic_load_explicit(&g_hz3_large_obs_map_take_calls, memory_order_relaxed);
    size_t map_take_total =
        atomic_load_explicit(&g_hz3_large_obs_map_take_ns_total, memory_order_relaxed);
    size_t alloc_cache_pop_calls =
        atomic_load_explicit(&g_hz3_large_obs_alloc_cache_pop_calls, memory_order_relaxed);
    size_t alloc_cache_pop_total =
        atomic_load_explicit(&g_hz3_large_obs_alloc_cache_pop_ns_total, memory_order_relaxed);
    size_t free_admit_calls =
        atomic_load_explicit(&g_hz3_large_obs_free_admit_calls, memory_order_relaxed);
    size_t free_admit_total =
        atomic_load_explicit(&g_hz3_large_obs_free_admit_ns_total, memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_LARGE_ALIGNED_OBS_TIMING] "
            "map_insert_calls=%zu map_insert_ns_total=%zu map_insert_ns_avg=%zu "
            "map_insert_ns_max=%zu map_take_calls=%zu map_take_ns_total=%zu "
            "map_take_ns_avg=%zu map_take_ns_max=%zu alloc_cache_pop_calls=%zu "
            "alloc_cache_pop_ns_total=%zu alloc_cache_pop_ns_avg=%zu "
            "alloc_cache_pop_ns_max=%zu free_admit_calls=%zu "
            "free_admit_ns_total=%zu free_admit_ns_avg=%zu free_admit_ns_max=%zu\n",
            map_insert_calls,
            map_insert_total,
            map_insert_calls ? (map_insert_total / map_insert_calls) : 0,
            atomic_load_explicit(&g_hz3_large_obs_map_insert_ns_max, memory_order_relaxed),
            map_take_calls,
            map_take_total,
            map_take_calls ? (map_take_total / map_take_calls) : 0,
            atomic_load_explicit(&g_hz3_large_obs_map_take_ns_max, memory_order_relaxed),
            alloc_cache_pop_calls,
            alloc_cache_pop_total,
            alloc_cache_pop_calls ? (alloc_cache_pop_total / alloc_cache_pop_calls) : 0,
            atomic_load_explicit(&g_hz3_large_obs_alloc_cache_pop_ns_max, memory_order_relaxed),
            free_admit_calls,
            free_admit_total,
            free_admit_calls ? (free_admit_total / free_admit_calls) : 0,
            atomic_load_explicit(&g_hz3_large_obs_free_admit_ns_max, memory_order_relaxed));
#if HZ3_S300_OVERALIGNED_MEDIUM_ROUTE_OBS
    fprintf(stderr,
            "[HZ3_S300_ROUTE_OBS] aligned_calls=%zu align8192_calls=%zu "
            "eligible_total=%zu eligible_4k_a8192=%zu eligible_8k_a8192=%zu "
            "eligible_64k_a8192=%zu route_large_current=%zu "
            "route_medium_candidate=%zu noncandidate=%zu\n",
            atomic_load_explicit(&g_s300_route_aligned_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_align8192_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_eligible_total, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_eligible_4k_a8192, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_eligible_8k_a8192, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_eligible_64k_a8192, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_large_current, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_medium_candidate, memory_order_relaxed),
            atomic_load_explicit(&g_s300_route_noncandidate, memory_order_relaxed));
#endif
#if HZ5_P0_ROUTE_SHADOW
    fprintf(stderr,
            "[HZ5_P0_ROUTE_SHADOW] candidate_4k_a8192=%zu "
            "candidate_8k_a8192=%zu candidate_64k_a8192=%zu "
            "candidate_total=%zu route_taken=%zu route_fallback_hz3=%zu "
            "true_large_fallback=%zu route_reject_a4096_guard=%zu "
            "route_reject_other=%zu\n",
            atomic_load_explicit(&g_hz5_p0_route_candidate_4k_a8192,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_candidate_8k_a8192,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_candidate_64k_a8192,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_candidate_total,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_taken, memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_fallback_hz3,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_true_large_fallback,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_reject_a4096_guard,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_hz5_p0_route_reject_other,
                                 memory_order_relaxed));
#endif
    hz3_s243_residual_dump_final();
    hz3_s275_activation_dump_final();
    hz3_s280_free_route_dump_final();
}

