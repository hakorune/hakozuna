#ifndef HZ3_LARGE_ALIGNED_OBS
#define HZ3_LARGE_ALIGNED_OBS 0
#endif

#if HZ3_S293D_HIGHBAND_GLOBAL_SOFT_CAP
static _Atomic uint32_t g_s293d_global_admit_epoch[HZ3_LARGE_SC_COUNT];
#endif

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
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
static _Atomic size_t g_s268_admit_cap_skips = 0;
static _Atomic size_t g_s268_floor_clamps = 0;
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
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
            " s268_admit_cap_skips=%zu s268_floor_clamps=%zu"
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
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
            , atomic_load_explicit(&g_s268_admit_cap_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s268_floor_clamps, memory_order_relaxed)
#endif
#if HZ3_S193_DEMAND_SCAVENGE
            , atomic_load_explicit(&g_hz3_s193_mmap_miss_calls, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s193_scavenge_runs, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s193_scavenge_pages, memory_order_relaxed)
#endif
            );
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
    fprintf(stderr,
            "[HZ3_S268_LARGE_SCACHE_RSS_CAP] enabled=1 sc_min=%d "
            "max_nodes_per_sc=%u admit_cap_skips=%zu floor_clamps=%zu\n",
            HZ3_S268_SC_MIN,
            (unsigned)HZ3_S268_MAX_NODES_PER_SC,
            atomic_load_explicit(&g_s268_admit_cap_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s268_floor_clamps, memory_order_relaxed));
#endif
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

