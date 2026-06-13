static inline void hz3_large_aligned_obs_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_large_aligned_obs_dump_final);
    }
}

static inline void hz3_large_aligned_obs_on_normal_alloc(size_t size) {
    (void)size;
    hz3_large_aligned_obs_register_once();
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_normal_calls);
}

static inline void hz3_large_aligned_obs_on_invalid(size_t size, size_t alignment) {
    (void)size;
    (void)alignment;
    hz3_large_aligned_obs_register_once();
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_aligned_invalid);
}

static inline void hz3_large_aligned_obs_on_aligned_alloc(size_t size, size_t alignment) {
    hz3_large_aligned_obs_register_once();
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_aligned_calls);
    hz3_s300_route_obs_on_aligned_alloc(size, alignment);
    hz5_p0_route_shadow_on_aligned_alloc(size, alignment);

    if (size < 4096u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_size_lt4k);
    } else if (size <= 65536u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_size_4k_64k);
    } else if (size <= 1048576u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_size_64k_1m);
    } else {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_size_gt1m);
    }

    if (alignment <= 16u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_align_le16);
    } else if (alignment <= 4096u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_align_le4096);
    } else if (alignment <= 8192u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_align_le8192);
    } else {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_align_gt8192);
    }

    if (size >= 4096u && size <= 65536u && alignment <= 4096u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_medium_candidate);
    }
    if (size < 4096u && alignment > 16u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_small_overaligned);
    }
    if (size > 65536u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_large_over64k);
    }
    if (alignment > 4096u) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_over_page_aligned);
    }
}

static inline void hz3_large_aligned_obs_on_aligned_cache_hit(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_aligned_cache_hits);
}

static inline void hz3_large_aligned_obs_on_aligned_cache_miss(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_aligned_cache_misses);
}

static inline void hz3_large_aligned_obs_on_aligned_mmap(size_t bytes) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_aligned_mmap_calls);
    hz3_large_aligned_obs_add(&g_hz3_large_obs_aligned_mmap_bytes, bytes);
}

static inline void hz3_large_aligned_obs_on_aligned_batch_mmap(size_t bytes) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_aligned_batch_mmap_calls);
    hz3_large_aligned_obs_add(&g_hz3_large_obs_aligned_batch_mmap_bytes, bytes);
}

static inline void hz3_large_aligned_obs_on_free(const Hz3LargeHdr* hdr) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_free_calls);
    uintptr_t expected = (uintptr_t)hdr->map_base + hz3_large_user_offset();
    if ((uintptr_t)hdr->user_ptr == expected) {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_free_unshifted);
    } else {
        hz3_large_aligned_obs_inc(&g_hz3_large_obs_free_shifted);
    }
}

static inline void hz3_large_aligned_obs_on_take_hit(size_t scan_steps) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_take_hits);
    hz3_large_aligned_obs_add(&g_hz3_large_obs_take_scan_steps, scan_steps);
}

static inline void hz3_large_aligned_obs_on_take_miss(size_t scan_steps) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_take_misses);
    hz3_large_aligned_obs_add(&g_hz3_large_obs_take_scan_steps, scan_steps);
}
static inline void hz3_large_aligned_obs_on_admit_invalid(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_invalid);
}
static inline void hz3_large_aligned_obs_on_admit_oversize(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_oversize);
}
static inline void hz3_large_aligned_obs_on_admit_precheck_yes(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_precheck_yes);
}
static inline void hz3_large_aligned_obs_on_admit_lock_decision(int can_cache) {
    hz3_large_aligned_obs_inc(can_cache ?
        &g_hz3_large_obs_admit_lock_yes :
        &g_hz3_large_obs_admit_lock_no);
}
static inline void hz3_large_aligned_obs_on_admit_reuse_skip(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_reuse_skip);
}
static inline void hz3_large_aligned_obs_on_admit_final_fit_fail(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_final_fit_fail);
}
static inline void hz3_large_aligned_obs_on_admit_cache_insert(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_cache_insert);
}
static inline void hz3_large_aligned_obs_on_admit_do_munmap(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_do_munmap);
}
static inline void hz3_large_aligned_obs_on_admit_defer_munmap(void) {
    hz3_large_aligned_obs_inc(&g_hz3_large_obs_admit_defer_munmap);
}
