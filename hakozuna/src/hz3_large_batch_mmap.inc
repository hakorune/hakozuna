#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
static inline int hz3_large_s218_c1_batch_enabled_for_sc(int sc) {
#if HZ3_S218_C1_LARGE_BATCH_MMAP
    return (sc >= HZ3_S218_C1_SC_MIN &&
            sc <= HZ3_S218_C1_SC_MAX &&
            sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline size_t hz3_large_s218_c1_seed_req_size(size_t class_size, size_t offset) {
    size_t reserve = offset;
#if HZ3_LARGE_CANARY_ENABLE
    reserve += HZ3_LARGE_CANARY_SIZE;
#endif
    if (class_size <= reserve) {
        return 1;
    }
    return class_size - reserve;
}

static Hz3LargeHdr* hz3_large_s218_c1_batch_mmap_alloc(size_t req_size,
                                                       size_t class_size,
                                                       size_t offset,
                                                       int sc,
                                                       size_t align_pad,
                                                       int aligned_alloc) {
#if HZ3_S218_C1_LARGE_BATCH_MMAP
    const size_t blocks = (size_t)HZ3_S218_C1_BATCH_BLOCKS;
    const size_t max_cache_per_batch = (size_t)HZ3_S218_C1_MAX_CACHE_PER_BATCH;
    if (!hz3_large_s218_c1_batch_enabled_for_sc(sc) || blocks < 2 || class_size == 0) {
        return NULL;
    }
    if (class_size > (SIZE_MAX / blocks)) {
        return NULL;
    }
    if (max_cache_per_batch == 0) {
        return NULL;
    }

    size_t hard_cap = hz3_large_hard_cap_for_sc(sc);
    const size_t seed_cap = (size_t)HZ3_S218_C1_SEED_CAP_BYTES;
    if (seed_cap > 0 && hard_cap > seed_cap) {
        hard_cap = seed_cap;
    }
    if (hard_cap < class_size) {
        return NULL;
    }
    // Require room for at least one seeded block; otherwise fallback to normal mmap.
    if ((hz3_large_total_cached_load() + class_size) > hard_cap) {
        return NULL;
    }

    const size_t total = class_size * blocks;
    void* region = hz3_large_os_mmap(total);
    if (region == MAP_FAILED) {
        return NULL;
    }
    if (aligned_alloc) {
        hz3_large_aligned_obs_on_aligned_batch_mmap(total);
    }

    uintptr_t user = (uintptr_t)region + offset;
    if (aligned_alloc && align_pad > 0) {
        user = (user + align_pad) & ~(uintptr_t)align_pad;
    }

    Hz3LargeHdr* hdr = (Hz3LargeHdr*)region;
    hdr->magic = HZ3_LARGE_MAGIC;
    hdr->req_size = req_size;
    hdr->map_base = region;
    hdr->map_size = class_size;
    hdr->user_ptr = (void*)user;
    hdr->next = NULL;
    hdr->in_use = 1;
    hdr->next_free = NULL;
    hz3_large_s240_on_activate(hdr, sc, 0);
#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif

    size_t cached_blocks = 0;
    size_t dropped_blocks = 0;
    size_t seed_req = hz3_large_s218_c1_seed_req_size(class_size, offset);
    void* dropped_bases[(HZ3_S218_C1_BATCH_BLOCKS > 1) ? (HZ3_S218_C1_BATCH_BLOCKS - 1) : 1];
    size_t dropped_count = 0;
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    Hz3LargeHdr* s270_victims[HZ3_S270_TRIM_BATCH_MAX];
    size_t s270_victim_count = 0;
#endif

#if !HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_large_cache_lock_acquire();
#endif

    for (size_t index = 1; index < blocks; index++) {
        void* extra_base = (void*)((char*)region + (index * class_size));
        Hz3LargeHdr* extra = (Hz3LargeHdr*)extra_base;
        extra->magic = HZ3_LARGE_MAGIC;
        extra->req_size = seed_req;
        extra->map_base = extra_base;
        extra->map_size = class_size;
        extra->user_ptr = (char*)extra_base + offset;
        extra->next = NULL;
        extra->in_use = 0;
        extra->next_free = NULL;
        hz3_large_s240_on_cache_seed(extra, sc);
#if HZ3_LARGE_CANARY_ENABLE
        hz3_large_debug_write_canary(extra);
#endif

        int can_cache = (cached_blocks < max_cache_per_batch);
        if (can_cache) {
            can_cache = (hz3_large_total_cached_load() + class_size <= hard_cap);
        }
        if (can_cache) {
            can_cache = hz3_large_reuse_try_admit_locked(sc);
        }
        if (can_cache) {
            can_cache = hz3_s293_global_admit_under_cap_locked(sc, extra);
        }
        if (can_cache) {
            can_cache = hz3_s293d_global_admit_under_soft_cap_locked(sc, extra);
        }
        if (can_cache) {
            hz3_large_s240_on_cache_store(extra, sc);
            hz3_large_sc_push_head(sc, extra);
            cached_blocks++;
        } else {
            dropped_bases[dropped_count++] = extra_base;
            dropped_blocks++;
        }
    }

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    s270_victim_count =
        hz3_s270_collect_idle_trim(sc, s270_victims, HZ3_S270_TRIM_BATCH_MAX);
#endif

#if !HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_large_cache_lock_release();
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    for (size_t index = 0; index < s270_victim_count; index++) {
        hz3_s270_dispose_detached_victim(s270_victims[index],
                                          "s270-batch-seed-dispose");
    }
#endif

    for (size_t index = 0; index < dropped_count; index++) {
        hz3_large_os_munmap(dropped_bases[index], class_size);
    }

    hz3_large_stats_on_s218_c1_batch(cached_blocks, dropped_blocks);
    return hdr;
#else
    (void)req_size;
    (void)class_size;
    (void)offset;
    (void)sc;
    (void)align_pad;
    (void)aligned_alloc;
    return NULL;
#endif
}
#endif
