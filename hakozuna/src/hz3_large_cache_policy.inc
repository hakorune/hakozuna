// S53: LargeCacheBudgetBox statistics (one-shot)
#if HZ3_LARGE_CACHE_STATS
static _Atomic size_t g_budget_soft_hits = 0;
static _Atomic size_t g_budget_hard_evicts = 0;
static _Atomic size_t g_budget_madvise_bytes = 0;
// S53-2: Throttle stats
static _Atomic size_t g_throttle_skips = 0;
static _Atomic size_t g_throttle_fires = 0;
// Large path stats (triage)
static _Atomic size_t g_large_alloc_calls = 0;
static _Atomic size_t g_large_alloc_cache_hits = 0;
static _Atomic size_t g_large_alloc_cache_misses = 0;
static _Atomic size_t g_large_free_calls = 0;
static _Atomic size_t g_large_free_cached = 0;
static _Atomic size_t g_large_free_munmap = 0;
static _Atomic size_t g_large_mmap_calls = 0;
static _Atomic size_t g_large_mmap_bytes = 0;
static _Atomic size_t g_large_munmap_calls = 0;
static _Atomic size_t g_large_munmap_bytes = 0;
static _Atomic size_t g_large_evict_dispose = 0;
static _Atomic size_t g_large_madvise_calls = 0;
static _Atomic size_t g_large_madvise_bytes = 0;
#if HZ3_S207_TARGETED_REUSE
static _Atomic size_t g_s207_reuse_credit_adds = 0;
static _Atomic size_t g_s207_reuse_admit_hits = 0;
static _Atomic size_t g_s207_reuse_admit_skips = 0;
static _Atomic size_t g_s207_reuse_floor_hits = 0;
#if HZ3_S233_HOTBAND_ALWAYS_ADMIT
static _Atomic size_t g_s233_hotband_force_admit_hits = 0;
#endif
#if HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST
static _Atomic size_t g_s218_floor_boost_hits = 0;
#endif
#endif
#if HZ3_S237_HIBAND_RETAIN_TUNE
static _Atomic size_t g_s237_budget_exhausted = 0;
static _Atomic size_t g_s237_slack_admit_hits = 0;
#endif
#if HZ3_S218_C1_LARGE_BATCH_MMAP
static _Atomic size_t g_s218_c1_batch_runs = 0;
static _Atomic size_t g_s218_c1_batch_cached_blocks = 0;
static _Atomic size_t g_s218_c1_batch_dropped_blocks = 0;
#endif
#if HZ3_S193_DEMAND_SCAVENGE
static _Atomic size_t g_hz3_s193_mmap_miss_calls = 0;
static _Atomic size_t g_hz3_s193_scavenge_runs = 0;
static _Atomic size_t g_hz3_s193_scavenge_pages = 0;
#endif

static inline void hz3_large_stat_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline void hz3_large_stat_add(_Atomic size_t* p, size_t v) {
    atomic_fetch_add_explicit(p, v, memory_order_relaxed);
}

static void hz3_large_cache_stats_dump_final(void) {
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    fprintf(stderr, "[HZ3_LARGE_CACHE_BUDGET] cached=%zu soft_hits=%zu hard_evicts=%zu "
            "madvise_bytes=%zu throttle_skips=%zu throttle_fires=%zu\n",
            hz3_large_total_cached_load(),
            atomic_load_explicit(&g_budget_soft_hits, memory_order_relaxed),
            atomic_load_explicit(&g_budget_hard_evicts, memory_order_relaxed),
            atomic_load_explicit(&g_budget_madvise_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_throttle_skips, memory_order_relaxed),
            atomic_load_explicit(&g_throttle_fires, memory_order_relaxed));
#endif
    fprintf(stderr, "[HZ3_LARGE_PATH] alloc_calls=%zu alloc_cache_hits=%zu alloc_cache_misses=%zu "
            "free_calls=%zu free_cached=%zu free_munmap=%zu mmap_calls=%zu mmap_bytes=%zu "
            "munmap_calls=%zu munmap_bytes=%zu evict_dispose=%zu madvise_calls=%zu madvise_bytes=%zu"
#if HZ3_S207_TARGETED_REUSE
            " s207_credit_adds=%zu s207_admit_hits=%zu s207_admit_skips=%zu s207_floor_hits=%zu"
#if HZ3_S233_HOTBAND_ALWAYS_ADMIT
            " s233_hotband_force_admit_hits=%zu"
#endif
#if HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST
            " s218_floor_boost_hits=%zu"
#endif
#endif
#if HZ3_S218_C1_LARGE_BATCH_MMAP
            " s218_c1_batch_runs=%zu s218_c1_cached_blocks=%zu s218_c1_dropped_blocks=%zu"
#endif
#if HZ3_S237_HIBAND_RETAIN_TUNE
            " s237_budget_exhausted=%zu s237_slack_admit_hits=%zu"
#endif
#if HZ3_S193_DEMAND_SCAVENGE
            " s193_miss=%zu s193_runs=%zu s193_pages=%zu"
#endif
            "\n",
            atomic_load_explicit(&g_large_alloc_calls, memory_order_relaxed),
            atomic_load_explicit(&g_large_alloc_cache_hits, memory_order_relaxed),
            atomic_load_explicit(&g_large_alloc_cache_misses, memory_order_relaxed),
            atomic_load_explicit(&g_large_free_calls, memory_order_relaxed),
            atomic_load_explicit(&g_large_free_cached, memory_order_relaxed),
            atomic_load_explicit(&g_large_free_munmap, memory_order_relaxed),
            atomic_load_explicit(&g_large_mmap_calls, memory_order_relaxed),
            atomic_load_explicit(&g_large_mmap_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_large_munmap_calls, memory_order_relaxed),
            atomic_load_explicit(&g_large_munmap_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_large_evict_dispose, memory_order_relaxed),
            atomic_load_explicit(&g_large_madvise_calls, memory_order_relaxed),
            atomic_load_explicit(&g_large_madvise_bytes, memory_order_relaxed)
#if HZ3_S207_TARGETED_REUSE
            , atomic_load_explicit(&g_s207_reuse_credit_adds, memory_order_relaxed),
            atomic_load_explicit(&g_s207_reuse_admit_hits, memory_order_relaxed),
            atomic_load_explicit(&g_s207_reuse_admit_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s207_reuse_floor_hits, memory_order_relaxed)
#if HZ3_S233_HOTBAND_ALWAYS_ADMIT
            , atomic_load_explicit(&g_s233_hotband_force_admit_hits, memory_order_relaxed)
#endif
#if HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST
            , atomic_load_explicit(&g_s218_floor_boost_hits, memory_order_relaxed)
#endif
#endif
#if HZ3_S218_C1_LARGE_BATCH_MMAP
            , atomic_load_explicit(&g_s218_c1_batch_runs, memory_order_relaxed),
            atomic_load_explicit(&g_s218_c1_batch_cached_blocks, memory_order_relaxed),
            atomic_load_explicit(&g_s218_c1_batch_dropped_blocks, memory_order_relaxed)
#endif
#if HZ3_S237_HIBAND_RETAIN_TUNE
            , atomic_load_explicit(&g_s237_budget_exhausted, memory_order_relaxed),
            atomic_load_explicit(&g_s237_slack_admit_hits, memory_order_relaxed)
#endif
#if HZ3_S193_DEMAND_SCAVENGE
            , atomic_load_explicit(&g_hz3_s193_mmap_miss_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s193_scavenge_runs, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s193_scavenge_pages, memory_order_relaxed)
#endif
            );
}

static void hz3_large_cache_stats_dump(void) {
    static _Atomic int g_stats_registered = 0;
    if (atomic_exchange_explicit(&g_stats_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_large_cache_stats_dump_final);
    }
}
#else
static inline void hz3_large_cache_stats_dump(void) {}
#endif

static inline void hz3_large_stats_on_alloc_call(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_alloc_calls);
#endif
}

static inline void hz3_large_stats_on_alloc_cache_hit(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_alloc_cache_hits);
#endif
}

static inline void hz3_large_stats_on_alloc_cache_miss(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_alloc_cache_misses);
#endif
}

static inline void hz3_large_stats_on_free_call(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_free_calls);
#endif
}

static inline void hz3_large_stats_on_free_cached(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_free_cached);
#endif
}

static inline void hz3_large_stats_on_free_munmap(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_free_munmap);
#endif
}

static inline void hz3_large_stats_on_mmap(size_t bytes) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_mmap_calls);
    hz3_large_stat_add(&g_large_mmap_bytes, bytes);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_stats_on_munmap(size_t bytes) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_munmap_calls);
    hz3_large_stat_add(&g_large_munmap_bytes, bytes);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_stats_on_evict_dispose(void) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_evict_dispose);
#endif
}

static inline void hz3_large_stats_on_madvise(size_t bytes) {
#if HZ3_LARGE_CACHE_STATS
    hz3_large_stat_inc(&g_large_madvise_calls);
    hz3_large_stat_add(&g_large_madvise_bytes, bytes);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_stats_on_s218_c1_batch(size_t cached_blocks, size_t dropped_blocks) {
#if HZ3_LARGE_CACHE_STATS && HZ3_S218_C1_LARGE_BATCH_MMAP
    hz3_large_stat_inc(&g_s218_c1_batch_runs);
    hz3_large_stat_add(&g_s218_c1_batch_cached_blocks, cached_blocks);
    hz3_large_stat_add(&g_s218_c1_batch_dropped_blocks, dropped_blocks);
#else
    (void)cached_blocks;
    (void)dropped_blocks;
#endif
}

static inline int hz3_large_soft_purge_advice(void) {
#if HZ3_S192_SOFT_PURGE_MODE == 1
#ifdef MADV_FREE
    return MADV_FREE;
#else
    return MADV_DONTNEED;
#endif
#elif HZ3_S192_SOFT_PURGE_MODE == 2
    return MADV_DONTNEED;
#else
    return MADV_DONTNEED;
#endif
}

static inline void hz3_large_soft_purge(void* addr, size_t bytes) {
    if (bytes == 0) {
        return;
    }
    hz3_large_stats_on_madvise(bytes);
    madvise(addr, bytes, hz3_large_soft_purge_advice());
}

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_LARGE_CACHE_BUDGET && HZ3_S193_DEMAND_SCAVENGE
static _Atomic uint32_t g_hz3_s193_mmap_miss_counter = 0;

static void hz3_large_scavenge_on_mmap_miss(int prefer_sc) {
    uint32_t miss = atomic_fetch_add_explicit(&g_hz3_s193_mmap_miss_counter, 1, memory_order_relaxed) + 1u;
#if HZ3_LARGE_CACHE_STATS
    atomic_fetch_add_explicit(&g_hz3_s193_mmap_miss_calls, 1, memory_order_relaxed);
#endif
    if ((miss % HZ3_S193_SCAVENGE_MISS_STRIDE) != 0u) {
        return;
    }

    size_t remaining_pages = HZ3_S193_SCAVENGE_BUDGET_PAGES;
    if (remaining_pages == 0) {
        return;
    }

    if (prefer_sc < 0 || prefer_sc >= (HZ3_LARGE_SC_COUNT - 1)) {
        prefer_sc = HZ3_LARGE_SC_COUNT - 2;
    }

    hz3_large_cache_lock_acquire();
    size_t purged_pages = 0;
    for (int pass = 0; pass < 2 && remaining_pages > 0; pass++) {
        int hi = (pass == 0) ? prefer_sc : (HZ3_LARGE_SC_COUNT - 2);
        int lo = (pass == 0) ? 0 : (prefer_sc + 1);
        for (int sc = hi; sc >= lo && remaining_pages > 0; sc--) {
            Hz3LargeHdr* hdr = g_sc_head[sc];
            while (hdr && remaining_pages > 0) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                uintptr_t aligned_start = (start + 4095u) & ~(uintptr_t)4095u;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (aligned_start < end) {
                    size_t bytes = (end - aligned_start) & ~(size_t)4095u;
                    size_t pages = bytes >> 12;
                    if (pages > remaining_pages) {
                        pages = remaining_pages;
                        bytes = pages << 12;
                    }
                    if (bytes > 0) {
                        hz3_large_soft_purge((void*)aligned_start, bytes);
                        remaining_pages -= pages;
                        purged_pages += pages;
                    }
                }
                hdr = hdr->next_free;
            }
        }
    }
    hz3_large_cache_lock_release();

#if HZ3_LARGE_CACHE_STATS
    atomic_fetch_add_explicit(&g_hz3_s193_scavenge_runs, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz3_s193_scavenge_pages, purged_pages, memory_order_relaxed);
#endif
}
#else
static inline void hz3_large_scavenge_on_mmap_miss(int prefer_sc) {
    (void)prefer_sc;
}
#endif

static inline void* hz3_large_os_mmap(size_t bytes) {
    void* base = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base != MAP_FAILED) {
        hz3_large_stats_on_mmap(bytes);
    }
    return base;
}

static inline int hz3_large_os_munmap(void* base, size_t bytes) {
    hz3_large_stats_on_munmap(bytes);
    return munmap(base, bytes);
}

// S53-2: ThrottleBox state
#if HZ3_LARGE_CACHE_BUDGET && HZ3_S53_THROTTLE
static _Atomic int64_t g_hz3_s53_scavenge_counter_pages = HZ3_S53_MADVISE_RATE_PAGES;
static _Atomic uint32_t g_hz3_s53_free_counter = 0;
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
#if HZ3_S186_LARGE_UNMAP_DEFER || HZ3_S212_LARGE_UNMAP_DEFER_PLUS
static hz3_lock_t g_hz3_large_unmap_defer_lock = HZ3_LOCK_INITIALIZER;
static Hz3LargeHdr* g_hz3_large_unmap_defer_head = NULL;
static size_t g_hz3_large_unmap_defer_bytes = 0;

static int hz3_large_unmap_defer_try_push(Hz3LargeHdr* victim) {
    int queued = 0;
    size_t max_bytes = HZ3_S186_UNMAP_DEFER_MAX_BYTES;
#if HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    max_bytes = HZ3_S212_UNMAP_DEFER_MAX_BYTES;
#endif
    hz3_lock_acquire(&g_hz3_large_unmap_defer_lock);
    if (g_hz3_large_unmap_defer_bytes + victim->map_size <= max_bytes) {
        victim->next_free = g_hz3_large_unmap_defer_head;
        g_hz3_large_unmap_defer_head = victim;
        g_hz3_large_unmap_defer_bytes += victim->map_size;
        queued = 1;
    }
    hz3_lock_release(&g_hz3_large_unmap_defer_lock);
    return queued;
}

static inline size_t hz3_large_unmap_defer_bytes_snapshot(void) {
    size_t bytes;
    hz3_lock_acquire(&g_hz3_large_unmap_defer_lock);
    bytes = g_hz3_large_unmap_defer_bytes;
    hz3_lock_release(&g_hz3_large_unmap_defer_lock);
    return bytes;
}

static void hz3_large_unmap_defer_drain_n(unsigned budget) {
    for (unsigned i = 0; i < budget; i++) {
        Hz3LargeHdr* victim = NULL;
        hz3_lock_acquire(&g_hz3_large_unmap_defer_lock);
        if (g_hz3_large_unmap_defer_head) {
            victim = g_hz3_large_unmap_defer_head;
            g_hz3_large_unmap_defer_head = victim->next_free;
            g_hz3_large_unmap_defer_bytes -= victim->map_size;
        }
        hz3_lock_release(&g_hz3_large_unmap_defer_lock);
        if (!victim) {
            break;
        }
        hz3_large_os_munmap(victim->map_base, victim->map_size);
    }
}

static inline void hz3_large_unmap_defer_drain_budget(void) {
    unsigned budget = HZ3_S186_UNMAP_DEFER_BUDGET;
#if HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    budget = HZ3_S212_UNMAP_DEFER_FREE_BUDGET;
#endif
    hz3_large_unmap_defer_drain_n(budget);
}

#if HZ3_S212_LARGE_UNMAP_DEFER_PLUS
#if HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS
static _Atomic uint32_t g_hz3_s212_unmap_drain_miss_counter = 0;
#endif

static inline void hz3_large_unmap_defer_drain_on_alloc_miss(void) {
#if HZ3_S212_DEFER_DRAIN_ON_ALLOC_MISS
    uint32_t miss = atomic_fetch_add_explicit(&g_hz3_s212_unmap_drain_miss_counter, 1u,
                                              memory_order_relaxed) + 1u;
    if ((miss % (uint32_t)HZ3_S212_DEFER_DRAIN_MISS_STRIDE) != 0u) {
        return;
    }
    if (hz3_large_unmap_defer_bytes_snapshot() < HZ3_S212_DEFER_DRAIN_TRIGGER_BYTES) {
        return;
    }
    hz3_large_unmap_defer_drain_n(HZ3_S212_DEFER_DRAIN_MISS_BUDGET);
#endif
}

static inline int hz3_large_try_defer_direct_unmap(Hz3LargeHdr* hdr) {
#if HZ3_S212_DEFER_MUNMAP_ON_FREE_PATH
    hdr->next = NULL;
    hdr->next_free = NULL;
    return hz3_large_unmap_defer_try_push(hdr);
#else
    (void)hdr;
    return 0;
#endif
}
#else
static inline void hz3_large_unmap_defer_drain_on_alloc_miss(void) {}
static inline int hz3_large_try_defer_direct_unmap(Hz3LargeHdr* hdr) {
    (void)hdr;
    return 0;
}
#endif
#else
static inline void hz3_large_unmap_defer_drain_budget(void) {}
static inline void hz3_large_unmap_defer_drain_on_alloc_miss(void) {}
static inline int hz3_large_try_defer_direct_unmap(Hz3LargeHdr* hdr) {
    (void)hdr;
    return 0;
}
#endif

static inline void hz3_large_dispose_victim_unlocked(Hz3LargeHdr* victim) {
    hz3_large_stats_on_evict_dispose();
    victim->magic = 0;
#if HZ3_S186_LARGE_UNMAP_DEFER || HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    victim->next = NULL;
    victim->next_free = NULL;
    if (!hz3_large_unmap_defer_try_push(victim)) {
        hz3_large_os_munmap(victim->map_base, victim->map_size);
    }
#else
    hz3_large_os_munmap(victim->map_base, victim->map_size);
#endif
}
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S50_LARGE_SCACHE_EVICT && HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
static size_t hz3_large_collect_evict_victims_locked(int prefer_sc, size_t incoming, size_t hard_cap,
                                                      Hz3LargeHdr** victims, size_t cap,
                                                      size_t* budget_left) {
    size_t n = 0;
    while (hz3_large_total_cached_load() + incoming > hard_cap && n < cap) {
        if (budget_left && *budget_left == 0) {
            break;
        }
        Hz3LargeHdr* victim = hz3_large_sc_pop_head(prefer_sc);
        if (!victim) {
            for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                victim = hz3_large_sc_pop_head(i);
                if (victim) {
                    break;
                }
            }
        }
        if (!victim) {
            break;
        }
#if HZ3_LARGE_CACHE_STATS
        atomic_fetch_add(&g_budget_hard_evicts, 1);
        hz3_large_cache_stats_dump();
#endif
        hz3_large_debug_on_munmap_locked(victim);
        victims[n++] = victim;
        if (budget_left && *budget_left > 0) {
            (*budget_left)--;
        }
    }
    return n;
}

static inline void hz3_large_munmap_victims_unlocked(Hz3LargeHdr** victims, size_t n) {
    for (size_t i = 0; i < n; i++) {
        hz3_large_dispose_victim_unlocked(victims[i]);
    }
}
#endif

static inline uint32_t hz3_large_hash_ptr(const void* ptr) {
    uintptr_t v = (uintptr_t)ptr;
    v >>= 4;
    v ^= v >> 33;
    v ^= v >> 11;
    return (uint32_t)v & (HZ3_LARGE_HASH_SIZE - 1u);
}

static inline hz3_lock_t* hz3_large_map_lock_for_idx(uint32_t idx) {
    return &g_hz3_large_map_locks[idx & (HZ3_S181_LARGE_MAP_LOCK_STRIPES - 1u)];
}

static inline size_t hz3_large_user_offset(void) {
    return (sizeof(Hz3LargeHdr) + (HZ3_LARGE_ALIGN - 1u)) & ~(HZ3_LARGE_ALIGN - 1u);
}

// S50: Page alignment helper
static inline size_t hz3_page_align(size_t n) {
    return (n + 4095) & ~(size_t)4095;
}

#if HZ3_S50_LARGE_SCACHE
// S50: size → class index
// 32KB〜64MB を 8 分割/倍（97 classes）
static inline int hz3_large_sc(size_t size) {
    if (size <= HZ3_LARGE_SC_MIN) return 0;
    if (size > HZ3_LARGE_SC_MAX) return HZ3_LARGE_SC_COUNT - 1;
    // ceil(log2) で bits を取得
    int bits = 64 - __builtin_clzl((uint64_t)(size - 1));
    // 32KB〜64KB (bits=16) が class 0..7 になるよう bits-16
    int base = (bits - 16) * 8;
    if (base < 0) return 0;  // safety clamp
    size_t range_min = 1UL << (bits - 1);
    int sub = (int)((size - range_min) * 8 / range_min);
    if (sub < 0) sub = 0;  // safety clamp
    if (sub > 7) sub = 7;  // クランプ
    int sc = base + sub;
    if (sc < 0) return 0;
    if (sc >= HZ3_LARGE_SC_COUNT) return HZ3_LARGE_SC_COUNT - 1;
    return sc;
}

// S50: class index → size (for mmap allocation)
// Returns the UPPER bound of the class (maximum size that fits in this class)
static inline size_t hz3_large_sc_size(int sc) {
    if (sc < 0) sc = 0;
    if (sc >= HZ3_LARGE_SC_COUNT - 1) return 0;  // >64MB: キャッシュ対象外
    int range = sc / 8;
    int sub = sc % 8;
    size_t base_size = 1UL << (15 + range);  // 32KB * 2^range
    // Upper bound: base + base * (sub + 1) / 8
    return base_size + base_size * (sub + 1) / 8;
}
#endif

static inline size_t hz3_large_hard_cap_for_sc(int sc) {
#if !HZ3_S207_HIBAND_CACHE
    (void)sc;
#endif
#if HZ3_LARGE_CACHE_BUDGET
    size_t hard_cap = HZ3_LARGE_CACHE_HARD_BYTES;
    if (hard_cap < HZ3_LARGE_CACHE_MAX_BYTES) {
        hard_cap = HZ3_LARGE_CACHE_MAX_BYTES;
    }
#else
    size_t hard_cap = HZ3_LARGE_CACHE_MAX_BYTES;
#endif
#if HZ3_S207_HIBAND_CACHE
    if (sc >= HZ3_S207_HIBAND_SC_MIN) {
        size_t hiband_cap = HZ3_S207_HIBAND_HARD_BYTES;
        if (hiband_cap > hard_cap) {
            hard_cap = hiband_cap;
        }
    }
#endif
#if HZ3_S237_HIBAND_RETAIN_TUNE
    if (sc >= HZ3_S237_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1)) {
        size_t hiband_cap = HZ3_S237_HIBAND_HARD_BYTES;
        if (hiband_cap > hard_cap) {
            hard_cap = hiband_cap;
        }
    }
#endif
    return hard_cap;
}

static inline int hz3_large_s237_hiband_sc(int sc) {
#if HZ3_S237_HIBAND_RETAIN_TUNE
    return (sc >= HZ3_S237_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline size_t hz3_large_insert_cap_for_sc(int sc, size_t hard_cap) {
#if HZ3_S237_HIBAND_RETAIN_TUNE
    if (hz3_large_s237_hiband_sc(sc)) {
        size_t slack = (size_t)HZ3_S237_INSERT_SLACK_BYTES;
        if (SIZE_MAX - hard_cap < slack) {
            return SIZE_MAX;
        }
        return hard_cap + slack;
    }
#else
    (void)sc;
#endif
    return hard_cap;
}

static inline size_t hz3_large_evict_budget_for_sc(int sc) {
#if HZ3_S237_HIBAND_RETAIN_TUNE
    if (hz3_large_s237_hiband_sc(sc)) {
        return (size_t)HZ3_S237_EVICT_BUDGET;
    }
#else
    (void)sc;
#endif
    return SIZE_MAX;
}

#if HZ3_S207_TARGETED_REUSE
static _Atomic uint32_t g_s218_alloc_miss_counter = 0;

static inline uint32_t hz3_large_reuse_floor_nodes_for_sc(int sc) {
    uint32_t floor_nodes = (uint32_t)HZ3_S207_REUSE_FLOOR_NODES;
#if HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST
    if (sc >= HZ3_S218_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1)) {
        uint32_t misses = atomic_load_explicit(&g_s218_alloc_miss_counter, memory_order_relaxed);
        if (misses >= HZ3_S218_MISS_TRIGGER && (uint32_t)HZ3_S218_FLOOR_NODES > floor_nodes) {
            floor_nodes = (uint32_t)HZ3_S218_FLOOR_NODES;
        }
    }
#else
    (void)sc;
#endif
    return floor_nodes;
}
#endif

static inline int hz3_large_reuse_trackable_sc(int sc) {
#if HZ3_S207_TARGETED_REUSE
    return (sc >= HZ3_S207_REUSE_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline int hz3_s233_hotband_force_sc(int sc) {
#if HZ3_S207_TARGETED_REUSE && HZ3_S233_HOTBAND_ALWAYS_ADMIT
    return (sc >= HZ3_S233_SC_MIN && sc <= HZ3_S233_SC_MAX &&
            sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline void hz3_large_reuse_note_alloc_miss(int sc) {
#if HZ3_S207_TARGETED_REUSE
    atomic_fetch_add_explicit(&g_s218_alloc_miss_counter, 1u, memory_order_relaxed);
    if (!hz3_large_reuse_trackable_sc(sc)) {
        return;
    }
    uint16_t cur = atomic_load_explicit(&g_sc_reuse_credit[sc], memory_order_relaxed);
    for (;;) {
        uint32_t next_u32 = (uint32_t)cur + (uint32_t)HZ3_S207_REUSE_MISS_CREDIT_INC;
        if (next_u32 > (uint32_t)HZ3_S207_REUSE_CREDIT_MAX) {
            next_u32 = (uint32_t)HZ3_S207_REUSE_CREDIT_MAX;
        }
        uint16_t next = (uint16_t)next_u32;
        if (next == cur) {
            break;
        }
        if (atomic_compare_exchange_weak_explicit(&g_sc_reuse_credit[sc], &cur, next,
                memory_order_relaxed, memory_order_relaxed)) {
#if HZ3_LARGE_CACHE_STATS
            hz3_large_stat_inc(&g_s207_reuse_credit_adds);
#endif
            break;
        }
    }
#else
    (void)sc;
#endif
}

static inline int hz3_large_reuse_try_admit(int sc) {
#if HZ3_S207_TARGETED_REUSE
    if (hz3_s233_hotband_force_sc(sc)) {
#if HZ3_LARGE_CACHE_STATS && HZ3_S233_HOTBAND_ALWAYS_ADMIT
        hz3_large_stat_inc(&g_s233_hotband_force_admit_hits);
#endif
        return 1;
    }
    if (!hz3_large_reuse_trackable_sc(sc)) {
        return 1;
    }
    uint16_t cur = atomic_load_explicit(&g_sc_reuse_credit[sc], memory_order_relaxed);
    for (;;) {
        if (cur < (uint16_t)HZ3_S207_REUSE_ADMIT_COST) {
#if HZ3_LARGE_CACHE_STATS
            hz3_large_stat_inc(&g_s207_reuse_admit_skips);
#endif
            return 0;
        }
        uint16_t next = (uint16_t)(cur - (uint16_t)HZ3_S207_REUSE_ADMIT_COST);
        if (atomic_compare_exchange_weak_explicit(&g_sc_reuse_credit[sc], &cur, next,
                memory_order_relaxed, memory_order_relaxed)) {
#if HZ3_LARGE_CACHE_STATS
            hz3_large_stat_inc(&g_s207_reuse_admit_hits);
#endif
            return 1;
        }
    }
#else
    (void)sc;
    return 1;
#endif
}

static inline int hz3_large_reuse_try_admit_locked(int sc) {
#if HZ3_S207_TARGETED_REUSE
    if (hz3_s233_hotband_force_sc(sc)) {
#if HZ3_LARGE_CACHE_STATS && HZ3_S233_HOTBAND_ALWAYS_ADMIT
        hz3_large_stat_inc(&g_s233_hotband_force_admit_hits);
#endif
        return 1;
    }
    if (!hz3_large_reuse_trackable_sc(sc)) {
        return 1;
    }
    uint32_t floor_nodes = hz3_large_reuse_floor_nodes_for_sc(sc);
    if (floor_nodes > 0) {
        uint32_t nodes;
        hz3_large_sc_lock_acquire(sc);
        nodes = g_sc_nodes[sc];
        hz3_large_sc_lock_release(sc);
        if (nodes < floor_nodes) {
#if HZ3_LARGE_CACHE_STATS
            hz3_large_stat_inc(&g_s207_reuse_floor_hits);
#if HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST
            if (floor_nodes > (uint32_t)HZ3_S207_REUSE_FLOOR_NODES) {
                hz3_large_stat_inc(&g_s218_floor_boost_hits);
            }
#endif
#endif
            return 1;
        }
    }
#endif
    return hz3_large_reuse_try_admit(sc);
}
