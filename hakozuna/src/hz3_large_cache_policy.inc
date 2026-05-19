#ifndef HZ3_LARGE_ALIGNED_OBS
#define HZ3_LARGE_ALIGNED_OBS 0
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

#if HZ3_S240_LARGE_OWNER_FRONT_STATS
static _Atomic size_t g_s240_activate = 0;
static _Atomic size_t g_s240_activate_cache = 0;
static _Atomic size_t g_s240_activate_mmap = 0;
static _Atomic size_t g_s240_cache_seed = 0;
static _Atomic size_t g_s240_cache_store = 0;
static _Atomic size_t g_s240_munmap_store = 0;
static _Atomic size_t g_s240_free_seen = 0;
static _Atomic size_t g_s240_free_active = 0;
static _Atomic size_t g_s240_state_mismatch = 0;
static _Atomic size_t g_s240_sc_mismatch = 0;
static _Atomic size_t g_s240_owner_unknown_activate = 0;
static _Atomic size_t g_s240_owner_unknown_free = 0;
static _Atomic size_t g_s240_owner_same_free = 0;
static _Atomic size_t g_s240_owner_remote_free = 0;
#if HZ3_S240_LARGE_FRONT_CACHE
static _Atomic size_t g_s240_front_push = 0;
static _Atomic size_t g_s240_front_pop_hit = 0;
static _Atomic size_t g_s240_front_pop_miss = 0;
static _Atomic size_t g_s240_front_reject_limit = 0;
static _Atomic size_t g_s240_front_flush_calls = 0;
static _Atomic size_t g_s240_front_flush_objs = 0;
static _Atomic size_t g_s240_front_flush_bytes = 0;
static _Atomic size_t g_s240_front_evict_munmap = 0;
static _Atomic size_t g_s240_front_nodes_current = 0;
static _Atomic size_t g_s240_front_nodes_peak = 0;
static _Atomic size_t g_s240_front_bytes_current = 0;
static _Atomic size_t g_s240_front_bytes_peak = 0;
#endif
#if HZ3_S240_LARGE_OWNER_INBOX
static _Atomic size_t g_s240_inbox_remote_push = 0;
static _Atomic size_t g_s240_inbox_reject_owner_dead = 0;
static _Atomic size_t g_s240_inbox_reject_owner_exiting = 0;
static _Atomic size_t g_s240_inbox_reject_shard_collision = 0;
static _Atomic size_t g_s240_inbox_reject_budget = 0;
static _Atomic size_t g_s240_inbox_drain_calls = 0;
static _Atomic size_t g_s240_inbox_drain_nonempty = 0;
static _Atomic size_t g_s240_inbox_drain_objs = 0;
static _Atomic size_t g_s240_inbox_drain_front_reject = 0;
static _Atomic size_t g_s240_inbox_flush_calls = 0;
static _Atomic size_t g_s240_inbox_flush_objs = 0;
static _Atomic size_t g_s240_inbox_flush_bytes = 0;
static _Atomic size_t g_s240_inbox_cas_retries = 0;
static _Atomic size_t g_s240_inbox_nodes_current = 0;
static _Atomic size_t g_s240_inbox_nodes_peak = 0;
static _Atomic size_t g_s240_inbox_bytes_current = 0;
static _Atomic size_t g_s240_inbox_bytes_peak = 0;
static _Atomic size_t g_s240_inbox_pushers_peak = 0;
static _Atomic size_t g_s240_inbox_flush_wait_spins = 0;
#if HZ3_S245_LARGE_INBOX_BATCH_POP || HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || \
    HZ3_S274_LARGE_INBOX_SIZE_DRAIN
static _Atomic size_t g_s245_batch_pop_calls = 0;
static _Atomic size_t g_s245_batch_pop_nonempty = 0;
static _Atomic size_t g_s245_batch_pop_objs = 0;
static _Atomic size_t g_s245_batch_pop_cas_retries = 0;
static _Atomic size_t g_s245_batch_pop_max_objs = 0;
#endif
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
static _Atomic size_t g_s246_take_first_calls = 0;
static _Atomic size_t g_s246_take_first_hits = 0;
static _Atomic size_t g_s246_take_first_misses = 0;
static _Atomic size_t g_s246_take_first_direct = 0;
static _Atomic size_t g_s246_take_first_extra_fronted = 0;
static _Atomic size_t g_s246_take_first_extra_munmap = 0;
#endif
#if HZ3_S282_LARGE_OWNER_INBOX_OBS
static _Atomic size_t g_s282_push_attempt = 0;
static _Atomic size_t g_s282_push_ok = 0;
static _Atomic size_t g_s282_push_reject_invalid = 0;
static _Atomic size_t g_s282_push_reject_owner_dead = 0;
static _Atomic size_t g_s282_push_reject_owner_exiting = 0;
static _Atomic size_t g_s282_push_reject_shard_collision = 0;
static _Atomic size_t g_s282_push_reject_budget = 0;
static _Atomic size_t g_s282_push_cas_attempts = 0;
static _Atomic size_t g_s282_push_cas_failures = 0;
static _Atomic size_t g_s282_push_cas_failure_max_per_push = 0;
static _Atomic size_t g_s282_push_depth_0 = 0;
static _Atomic size_t g_s282_push_depth_1 = 0;
static _Atomic size_t g_s282_push_depth_2_3 = 0;
static _Atomic size_t g_s282_push_depth_4_7 = 0;
static _Atomic size_t g_s282_push_depth_8_15 = 0;
static _Atomic size_t g_s282_push_depth_16_plus = 0;
static _Atomic size_t g_s282_pop_cas_attempts = 0;
static _Atomic size_t g_s282_pop_cas_failures = 0;
static _Atomic size_t g_s282_pop_cas_failure_max_per_pop = 0;
static _Atomic size_t g_s282_pop_success = 0;
static _Atomic size_t g_s282_pop_empty = 0;
static _Atomic size_t g_s282_pop_race_empty = 0;
static _Atomic size_t g_s282_take_first_calls = 0;
static _Atomic size_t g_s282_take_first_hit = 0;
static _Atomic size_t g_s282_take_first_miss_empty = 0;
static _Atomic size_t g_s282_take_first_miss_race = 0;
static _Atomic size_t g_s282_take_depth_0 = 0;
static _Atomic size_t g_s282_take_depth_1 = 0;
static _Atomic size_t g_s282_take_depth_2_3 = 0;
static _Atomic size_t g_s282_take_depth_4_7 = 0;
static _Atomic size_t g_s282_take_depth_8_15 = 0;
static _Atomic size_t g_s282_take_depth_16_plus = 0;
static _Atomic size_t g_s282_drain_nodes = 0;
static _Atomic size_t g_s282_drain_return_first_nodes = 0;
static _Atomic size_t g_s282_drain_to_front_nodes = 0;
static _Atomic size_t g_s282_drain_to_munmap_nodes = 0;
#endif
#endif
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
static _Atomic size_t g_s251_free_take_to_freeing = 0;
static _Atomic size_t g_s251_free_take_state_fail = 0;
static _Atomic size_t g_s251_front_pop_to_freeing = 0;
static _Atomic size_t g_s251_front_pop_state_fail = 0;
static _Atomic size_t g_s251_inbox_pop_to_freeing = 0;
static _Atomic size_t g_s251_inbox_pop_state_fail = 0;
static _Atomic size_t g_s251_global_pop_to_freeing = 0;
static _Atomic size_t g_s251_global_pop_state_fail = 0;
static _Atomic size_t g_s251_front_push_state_fail = 0;
static _Atomic size_t g_s251_inbox_push_state_fail = 0;
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
static _Atomic size_t g_s276_front_take_try = 0;
static _Atomic size_t g_s276_front_take_hit = 0;
static _Atomic size_t g_s276_front_take_not_owner = 0;
static _Atomic size_t g_s276_front_take_not_eligible = 0;
static _Atomic size_t g_s276_front_take_nonactive = 0;
static _Atomic size_t g_s276_front_take_state_cas_fail = 0;
static _Atomic size_t g_s276_front_push_retained = 0;
static _Atomic size_t g_s276_front_push_reject_clear = 0;
static _Atomic size_t g_s276_front_activate_fast = 0;
static _Atomic size_t g_s276_front_activate_rekey = 0;
static _Atomic size_t g_s276_inbox_take_hit = 0;
static _Atomic size_t g_s276_inbox_push_retained = 0;
static _Atomic size_t g_s276_inbox_push_reject_clear = 0;
static _Atomic size_t g_s276_inbox_activate_fast = 0;
static _Atomic size_t g_s276_direct_clear = 0;
#endif
#if HZ3_S285_LARGE_FIXED_COST_OBS
static _Atomic size_t g_s285_direct_way_loads = 0;
static _Atomic size_t g_s285_direct_key_busy = 0;
static _Atomic size_t g_s285_direct_key_miss = 0;
static _Atomic size_t g_s285_direct_key_match = 0;
static _Atomic size_t g_s285_direct_invalid_hdr = 0;
static _Atomic size_t g_s285_direct_state_nonactive = 0;
static _Atomic size_t g_s285_direct_inuse_bad = 0;
static _Atomic size_t g_s285_direct_not_retainable_same_owner = 0;
static _Atomic size_t g_s285_direct_not_retainable_remote = 0;
static _Atomic size_t g_s285_direct_cas_attempt = 0;
static _Atomic size_t g_s285_direct_cas_fail = 0;
static _Atomic size_t g_s285_direct_hit_front = 0;
static _Atomic size_t g_s285_direct_hit_inbox = 0;
static _Atomic size_t g_s285_activate_check = 0;
static _Atomic size_t g_s285_activate_fail_header = 0;
static _Atomic size_t g_s285_activate_fail_slot = 0;
static _Atomic size_t g_s285_activate_fail_flag = 0;
static _Atomic size_t g_s285_activate_fail_sc = 0;
static _Atomic size_t g_s285_activate_fail_alignment = 0;
static _Atomic size_t g_s285_activate_fail_user_alignment = 0;
static _Atomic size_t g_s285_activate_fail_range = 0;
static _Atomic size_t g_s285_activate_fail_size = 0;
static _Atomic size_t g_s285_activate_success_front = 0;
static _Atomic size_t g_s285_activate_success_inbox = 0;
static _Atomic size_t g_s285_take_first_return_first = 0;
static _Atomic size_t g_s285_take_first_extra_pop_attempt = 0;
static _Atomic size_t g_s285_take_first_extra_pop_hit = 0;
static _Atomic size_t g_s285_take_first_extra_pop_empty = 0;
static _Atomic size_t g_s285_take_first_extra_front_ok = 0;
static _Atomic size_t g_s285_take_first_extra_front_reject = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_1 = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_2_3 = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_4_7 = 0;
static _Atomic size_t g_s285_take_first_drain_nodes_8_plus = 0;
#endif
#if HZ3_S240_LARGE_FLUSH_VALIDATE
static _Atomic size_t g_s240_flush_validate_calls = 0;
static _Atomic size_t g_s240_flush_validate_failures = 0;
static _Atomic size_t g_s240_flush_front_node_fail = 0;
static _Atomic size_t g_s240_flush_inbox_node_fail = 0;
static _Atomic size_t g_s240_flush_drain_node_fail = 0;
static _Atomic size_t g_s240_flush_null_node_fail = 0;
static _Atomic size_t g_s240_flush_bad_state = 0;
static _Atomic size_t g_s240_flush_bad_header = 0;
static _Atomic size_t g_s240_flush_bad_link = 0;
static _Atomic size_t g_s240_flush_sc_mismatch = 0;
static _Atomic size_t g_s240_flush_inbox_count_underflow = 0;
static _Atomic size_t g_s240_flush_inbox_bytes_underflow = 0;
#endif
#if HZ3_S240_LARGE_FLUSH_QUARANTINE
static _Atomic size_t g_s240_flush_quarantine_objs = 0;
static _Atomic size_t g_s240_flush_quarantine_bytes = 0;
#endif

static inline void hz3_large_s240_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline void hz3_large_s240_add(_Atomic size_t* p, size_t v) {
    atomic_fetch_add_explicit(p, v, memory_order_relaxed);
}

static inline void hz3_large_s240_sub(_Atomic size_t* p, size_t v) {
    atomic_fetch_sub_explicit(p, v, memory_order_relaxed);
}

static inline void hz3_large_s240_update_max(_Atomic size_t* p, size_t value) {
    size_t cur = atomic_load_explicit(p, memory_order_relaxed);
    while (value > cur) {
        if (atomic_compare_exchange_weak_explicit(
                p, &cur, value, memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

static void hz3_large_s240_dump_final(void) {
    fprintf(stderr,
            "[HZ3_LARGE_S240_META] activate=%zu activate_cache=%zu "
            "activate_mmap=%zu cache_seed=%zu cache_store=%zu "
            "munmap_store=%zu free_seen=%zu free_active=%zu "
            "state_mismatch=%zu sc_mismatch=%zu owner_unknown_activate=%zu "
            "owner_unknown_free=%zu owner_same_free=%zu owner_remote_free=%zu\n",
            atomic_load_explicit(&g_s240_activate, memory_order_relaxed),
            atomic_load_explicit(&g_s240_activate_cache, memory_order_relaxed),
            atomic_load_explicit(&g_s240_activate_mmap, memory_order_relaxed),
            atomic_load_explicit(&g_s240_cache_seed, memory_order_relaxed),
            atomic_load_explicit(&g_s240_cache_store, memory_order_relaxed),
            atomic_load_explicit(&g_s240_munmap_store, memory_order_relaxed),
            atomic_load_explicit(&g_s240_free_seen, memory_order_relaxed),
            atomic_load_explicit(&g_s240_free_active, memory_order_relaxed),
            atomic_load_explicit(&g_s240_state_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s240_sc_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_unknown_activate, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_unknown_free, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_same_free, memory_order_relaxed),
            atomic_load_explicit(&g_s240_owner_remote_free, memory_order_relaxed));
#if HZ3_S240_LARGE_FRONT_CACHE
    fprintf(stderr,
            "[HZ3_LARGE_S240_FRONT] push=%zu pop_hit=%zu pop_miss=%zu "
            "reject_limit=%zu flush_calls=%zu flush_objs=%zu flush_bytes=%zu "
            "evict_munmap=%zu nodes_current=%zu nodes_peak=%zu "
            "bytes_current=%zu bytes_peak=%zu\n",
            atomic_load_explicit(&g_s240_front_push, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_pop_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_pop_miss, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_reject_limit, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_flush_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_flush_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_flush_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_evict_munmap, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_nodes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_front_bytes_peak, memory_order_relaxed));
#endif
#if HZ3_S240_LARGE_OWNER_INBOX
    fprintf(stderr,
            "[HZ3_LARGE_S240_INBOX] remote_push=%zu reject_owner_dead=%zu "
            "reject_owner_exiting=%zu reject_shard_collision=%zu "
            "reject_budget=%zu drain_calls=%zu drain_nonempty=%zu "
            "drain_objs=%zu drain_front_reject=%zu flush_calls=%zu "
            "flush_objs=%zu flush_bytes=%zu cas_retries=%zu "
            "nodes_current=%zu nodes_peak=%zu bytes_current=%zu bytes_peak=%zu "
            "pushers_peak=%zu flush_wait_spins=%zu\n",
            atomic_load_explicit(&g_s240_inbox_remote_push, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_owner_dead, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_owner_exiting, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_shard_collision, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_reject_budget, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_nonempty, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_drain_front_reject, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_bytes, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_cas_retries, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_nodes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_nodes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_bytes_current, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_bytes_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_pushers_peak, memory_order_relaxed),
            atomic_load_explicit(&g_s240_inbox_flush_wait_spins, memory_order_relaxed));
#if HZ3_S245_LARGE_INBOX_BATCH_POP || HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || \
    HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    fprintf(stderr,
            "[HZ3_LARGE_S245_INBOX_BATCH] pop_calls=%zu pop_nonempty=%zu "
            "pop_objs=%zu pop_cas_retries=%zu pop_max_objs=%zu\n",
            atomic_load_explicit(&g_s245_batch_pop_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_nonempty, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_cas_retries, memory_order_relaxed),
            atomic_load_explicit(&g_s245_batch_pop_max_objs, memory_order_relaxed));
#endif
#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
    fprintf(stderr,
            "[HZ3_LARGE_S246_INBOX_TAKE_FIRST] calls=%zu hits=%zu "
            "misses=%zu direct=%zu extra_fronted=%zu extra_munmap=%zu\n",
            atomic_load_explicit(&g_s246_take_first_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_hits, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_misses, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_direct, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_extra_fronted, memory_order_relaxed),
            atomic_load_explicit(&g_s246_take_first_extra_munmap, memory_order_relaxed));
#endif
#if HZ3_S282_LARGE_OWNER_INBOX_OBS
    fprintf(stderr,
            "[HZ3_S282_OWNER_INBOX] push_attempt=%zu push_ok=%zu "
            "push_reject_invalid=%zu push_reject_owner_dead=%zu "
            "push_reject_owner_exiting=%zu push_reject_shard_collision=%zu "
            "push_reject_budget=%zu push_cas_attempts=%zu "
            "push_cas_failures=%zu push_cas_failure_max_per_push=%zu "
            "push_depth_0=%zu push_depth_1=%zu push_depth_2_3=%zu "
            "push_depth_4_7=%zu push_depth_8_15=%zu "
            "push_depth_16_plus=%zu pop_cas_attempts=%zu "
            "pop_cas_failures=%zu pop_cas_failure_max_per_pop=%zu "
            "pop_success=%zu pop_empty=%zu pop_race_empty=%zu "
            "take_first_calls=%zu take_first_hit=%zu "
            "take_first_miss_empty=%zu take_first_miss_race=%zu "
            "take_depth_0=%zu take_depth_1=%zu take_depth_2_3=%zu "
            "take_depth_4_7=%zu take_depth_8_15=%zu "
            "take_depth_16_plus=%zu drain_nodes=%zu "
            "drain_return_first_nodes=%zu drain_to_front_nodes=%zu "
            "drain_to_munmap_nodes=%zu\n",
            atomic_load_explicit(&g_s282_push_attempt, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_ok, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_invalid, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_owner_dead, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_owner_exiting, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_shard_collision,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_reject_budget, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_cas_attempts, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_cas_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_cas_failure_max_per_push,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_0, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_1, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_2_3, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_4_7, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_8_15, memory_order_relaxed),
            atomic_load_explicit(&g_s282_push_depth_16_plus, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_cas_attempts, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_cas_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_cas_failure_max_per_pop,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_success, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_empty, memory_order_relaxed),
            atomic_load_explicit(&g_s282_pop_race_empty, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_miss_empty, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_first_miss_race, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_0, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_1, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_2_3, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_4_7, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_8_15, memory_order_relaxed),
            atomic_load_explicit(&g_s282_take_depth_16_plus, memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_return_first_nodes,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_to_front_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s282_drain_to_munmap_nodes, memory_order_relaxed));
#endif
#endif
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    fprintf(stderr,
            "[HZ3_LARGE_S251_SAFETY] free_take_to_freeing=%zu "
            "free_take_state_fail=%zu front_pop_to_freeing=%zu "
            "front_pop_state_fail=%zu inbox_pop_to_freeing=%zu "
            "inbox_pop_state_fail=%zu global_pop_to_freeing=%zu "
            "global_pop_state_fail=%zu front_push_state_fail=%zu "
            "inbox_push_state_fail=%zu failfast=%d\n",
            atomic_load_explicit(&g_s251_free_take_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_free_take_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_front_pop_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_front_pop_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_inbox_pop_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_inbox_pop_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_global_pop_to_freeing, memory_order_relaxed),
            atomic_load_explicit(&g_s251_global_pop_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_front_push_state_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s251_inbox_push_state_fail, memory_order_relaxed),
            HZ3_S251_LARGE_OWNER_FRONT_FAILFAST);
#endif
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    fprintf(stderr,
            "[HZ3_LARGE_S276_DIRECT_RETAIN] front_take_try=%zu "
            "front_take_hit=%zu front_take_not_owner=%zu "
            "front_take_not_eligible=%zu front_take_nonactive=%zu "
            "front_take_state_cas_fail=%zu front_push_retained=%zu "
            "front_push_reject_clear=%zu front_activate_fast=%zu "
            "front_activate_rekey=%zu inbox_take_hit=%zu "
            "inbox_push_retained=%zu inbox_push_reject_clear=%zu "
            "inbox_activate_fast=%zu direct_clear=%zu\n",
            atomic_load_explicit(&g_s276_front_take_try, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_not_owner, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_not_eligible, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_nonactive, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_take_state_cas_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_push_retained, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_push_reject_clear, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_activate_fast, memory_order_relaxed),
            atomic_load_explicit(&g_s276_front_activate_rekey, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_take_hit, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_push_retained, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_push_reject_clear, memory_order_relaxed),
            atomic_load_explicit(&g_s276_inbox_activate_fast, memory_order_relaxed),
            atomic_load_explicit(&g_s276_direct_clear, memory_order_relaxed));
#endif
#if HZ3_S285_LARGE_FIXED_COST_OBS
    fprintf(stderr,
            "[HZ3_S285_FIXED_COST_DIRECT] way_loads=%zu key_busy=%zu "
            "key_miss=%zu key_match=%zu invalid_hdr=%zu state_nonactive=%zu "
            "inuse_bad=%zu not_retainable_same_owner=%zu "
            "not_retainable_remote=%zu cas_attempt=%zu cas_fail=%zu "
            "hit_front=%zu hit_inbox=%zu\n",
            atomic_load_explicit(&g_s285_direct_way_loads, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_key_busy, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_key_miss, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_key_match, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_invalid_hdr, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_state_nonactive, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_inuse_bad, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_not_retainable_same_owner,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_not_retainable_remote,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_cas_attempt, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_cas_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_hit_front, memory_order_relaxed),
            atomic_load_explicit(&g_s285_direct_hit_inbox, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_S285_FIXED_COST_ACTIVATE] checks=%zu fail_header=%zu "
            "fail_slot=%zu fail_flag=%zu fail_sc=%zu fail_alignment=%zu "
            "fail_user_alignment=%zu fail_range=%zu fail_size=%zu "
            "success_front=%zu success_inbox=%zu\n",
            atomic_load_explicit(&g_s285_activate_check, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_header, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_slot, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_flag, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_sc, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_alignment, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_user_alignment,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_range, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_fail_size, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_success_front, memory_order_relaxed),
            atomic_load_explicit(&g_s285_activate_success_inbox, memory_order_relaxed));
    fprintf(stderr,
            "[HZ3_S285_FIXED_COST_DRAIN] return_first=%zu "
            "extra_pop_attempt=%zu extra_pop_hit=%zu extra_pop_empty=%zu "
            "extra_front_ok=%zu extra_front_reject=%zu drain_nodes_1=%zu "
            "drain_nodes_2_3=%zu drain_nodes_4_7=%zu drain_nodes_8_plus=%zu\n",
            atomic_load_explicit(&g_s285_take_first_return_first,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_pop_attempt,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_pop_hit,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_pop_empty,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_front_ok,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_extra_front_reject,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_1,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_2_3,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_4_7,
                                 memory_order_relaxed),
            atomic_load_explicit(&g_s285_take_first_drain_nodes_8_plus,
                                 memory_order_relaxed));
#endif
#if HZ3_S240_LARGE_FLUSH_VALIDATE
    fprintf(stderr,
            "[HZ3_LARGE_S240_FLUSH_VALIDATE] enabled=1 failfast=%d "
            "calls=%zu failures=%zu front_node_fail=%zu inbox_node_fail=%zu "
            "drain_node_fail=%zu null_node_fail=%zu bad_state=%zu "
            "bad_header=%zu bad_link=%zu sc_mismatch=%zu "
            "inbox_count_underflow=%zu inbox_bytes_underflow=%zu\n",
            HZ3_S240_LARGE_FLUSH_FAILFAST,
            atomic_load_explicit(&g_s240_flush_validate_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_validate_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_front_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_inbox_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_drain_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_null_node_fail, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_bad_state, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_bad_header, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_bad_link, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_sc_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_inbox_count_underflow,
                                  memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_inbox_bytes_underflow,
                                  memory_order_relaxed));
#endif
#if HZ3_S240_LARGE_FLUSH_QUARANTINE
    fprintf(stderr,
            "[HZ3_LARGE_S240_FLUSH_QUARANTINE] enabled=1 objs=%zu bytes=%zu\n",
            atomic_load_explicit(&g_s240_flush_quarantine_objs, memory_order_relaxed),
            atomic_load_explicit(&g_s240_flush_quarantine_bytes, memory_order_relaxed));
#endif
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
    size_t global_cached_bytes = 0;
    size_t global_cached_peak = 0;
    size_t global_cached_nodes = 0;
    int nonempty_classes = 0;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    global_cached_bytes = hz3_large_total_cached_load();
    global_cached_peak = atomic_load_explicit(&g_s253_global_cached_bytes_peak,
                                              memory_order_relaxed);
    for (int i = 0; i < HZ3_LARGE_SC_COUNT; i++) {
        global_cached_nodes += (size_t)g_sc_nodes[i];
        if (g_sc_nodes[i] != 0 || g_sc_bytes[i] != 0) {
            nonempty_classes++;
        }
    }
#endif
    size_t front_bytes_current =
#if HZ3_S240_LARGE_FRONT_CACHE
        atomic_load_explicit(&g_s240_front_bytes_current, memory_order_relaxed);
#else
        0;
#endif
    size_t front_bytes_peak =
#if HZ3_S240_LARGE_FRONT_CACHE
        atomic_load_explicit(&g_s240_front_bytes_peak, memory_order_relaxed);
#else
        0;
#endif
    size_t inbox_bytes_current =
#if HZ3_S240_LARGE_OWNER_INBOX
        atomic_load_explicit(&g_s240_inbox_bytes_current, memory_order_relaxed);
#else
        0;
#endif
    size_t inbox_bytes_peak =
#if HZ3_S240_LARGE_OWNER_INBOX
        atomic_load_explicit(&g_s240_inbox_bytes_peak, memory_order_relaxed);
#else
        0;
#endif
    fprintf(stderr,
            "[HZ3_LARGE_S253_RSS] global_cached_bytes=%zu "
            "global_cached_peak=%zu global_cached_nodes=%zu "
            "global_nonempty_classes=%d front_bytes_current=%zu "
            "front_bytes_peak=%zu inbox_bytes_current=%zu inbox_bytes_peak=%zu "
            "front_max_class_bytes=%zu front_max_nodes_per_sc=%u "
            "front_max_bytes_per_thread=%zu inbox_max_nodes_per_sc=%u "
            "inbox_max_bytes_per_sc=%zu direct_mapless_max_class_bytes=%zu\n",
            global_cached_bytes,
            global_cached_peak,
            global_cached_nodes,
            nonempty_classes,
            front_bytes_current,
            front_bytes_peak,
            inbox_bytes_current,
            inbox_bytes_peak,
            (size_t)HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES,
            (unsigned)HZ3_S240_LARGE_FRONT_MAX_NODES_PER_SC,
            (size_t)HZ3_S240_LARGE_FRONT_MAX_BYTES_PER_THREAD,
            (unsigned)HZ3_S240_LARGE_INBOX_MAX_NODES_PER_SC,
            (size_t)HZ3_S240_LARGE_INBOX_MAX_BYTES_PER_SC,
            (size_t)HZ3_S242_DIRECT_MAPLESS_MAX_CLASS_BYTES);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    fprintf(stderr, "[HZ3_LARGE_S253_SCACHE]");
    for (int i = 0; i < HZ3_LARGE_SC_COUNT; i++) {
        if (g_sc_nodes[i] != 0 || g_sc_bytes[i] != 0) {
            fprintf(stderr,
                    " sc%d_nodes=%u sc%d_bytes=%zu",
                    i,
                    g_sc_nodes[i],
                    i,
                    g_sc_bytes[i]);
        }
    }
    if (nonempty_classes == 0) {
        fprintf(stderr, " empty=1");
    }
    fprintf(stderr, "\n");
#endif
#endif
}

static inline void hz3_large_s240_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_large_s240_dump_final);
    }
}

static inline uint8_t hz3_large_s240_current_owner(void) {
    hz3_tcache_ensure_init();
    if (!t_hz3_cache.initialized) {
        return UINT8_MAX;
    }
    return (uint8_t)t_hz3_cache.my_shard;
}

static inline uint8_t hz3_large_s240_normalize_sc(int sc) {
    return (sc >= 0 && sc < HZ3_LARGE_SC_COUNT) ? (uint8_t)sc : UINT8_MAX;
}
#endif

#if HZ3_S240_LARGE_OWNER_FRONT && !HZ3_S240_LARGE_OWNER_FRONT_STATS
static inline void hz3_large_s240_register_once(void) {
}

static inline uint8_t hz3_large_s240_current_owner(void) {
    hz3_tcache_ensure_init();
    if (!t_hz3_cache.initialized) {
        return UINT8_MAX;
    }
    return (uint8_t)t_hz3_cache.my_shard;
}

static inline uint8_t hz3_large_s240_normalize_sc(int sc) {
    return (sc >= 0 && sc < HZ3_LARGE_SC_COUNT) ? (uint8_t)sc : UINT8_MAX;
}
#endif

#if HZ3_S282_LARGE_OWNER_INBOX_OBS
static inline void hz3_s282_owner_inbox_inc(_Atomic size_t* p) {
    hz3_large_s240_inc(p);
}

static inline void hz3_s282_owner_inbox_add(_Atomic size_t* p, size_t v) {
    hz3_large_s240_add(p, v);
}

static inline void hz3_s282_owner_inbox_depth_bucket(size_t depth,
                                                     _Atomic size_t* depth_0,
                                                     _Atomic size_t* depth_1,
                                                     _Atomic size_t* depth_2_3,
                                                     _Atomic size_t* depth_4_7,
                                                     _Atomic size_t* depth_8_15,
                                                     _Atomic size_t* depth_16_plus) {
    if (depth == 0) {
        hz3_s282_owner_inbox_inc(depth_0);
    } else if (depth == 1) {
        hz3_s282_owner_inbox_inc(depth_1);
    } else if (depth <= 3) {
        hz3_s282_owner_inbox_inc(depth_2_3);
    } else if (depth <= 7) {
        hz3_s282_owner_inbox_inc(depth_4_7);
    } else if (depth <= 15) {
        hz3_s282_owner_inbox_inc(depth_8_15);
    } else {
        hz3_s282_owner_inbox_inc(depth_16_plus);
    }
}

static inline void hz3_s282_owner_inbox_on_push_attempt(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_attempt);
}

static inline void hz3_s282_owner_inbox_on_push_ok(size_t cas_failures) {
    hz3_s282_owner_inbox_inc(&g_s282_push_ok);
    hz3_large_s240_update_max(&g_s282_push_cas_failure_max_per_push,
                              cas_failures);
}

static inline void hz3_s282_owner_inbox_on_push_reject_invalid(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_invalid);
}

static inline void hz3_s282_owner_inbox_on_push_reject_owner_dead(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_owner_dead);
}

static inline void hz3_s282_owner_inbox_on_push_reject_owner_exiting(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_owner_exiting);
}

static inline void hz3_s282_owner_inbox_on_push_reject_shard_collision(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_shard_collision);
}

static inline void hz3_s282_owner_inbox_on_push_reject_budget(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_budget);
}

static inline void hz3_s282_owner_inbox_on_push_depth(size_t depth) {
    hz3_s282_owner_inbox_depth_bucket(depth,
                                      &g_s282_push_depth_0,
                                      &g_s282_push_depth_1,
                                      &g_s282_push_depth_2_3,
                                      &g_s282_push_depth_4_7,
                                      &g_s282_push_depth_8_15,
                                      &g_s282_push_depth_16_plus);
}

static inline void hz3_s282_owner_inbox_on_push_cas_attempt(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_cas_attempts);
}

static inline void hz3_s282_owner_inbox_on_push_cas_failure(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_cas_failures);
}

static inline void hz3_s282_owner_inbox_on_pop_cas_attempt(void) {
    hz3_s282_owner_inbox_inc(&g_s282_pop_cas_attempts);
}

static inline void hz3_s282_owner_inbox_on_pop_cas_failure(void) {
    hz3_s282_owner_inbox_inc(&g_s282_pop_cas_failures);
}

static inline void hz3_s282_owner_inbox_on_pop_success(size_t cas_failures) {
    hz3_s282_owner_inbox_inc(&g_s282_pop_success);
    hz3_large_s240_update_max(&g_s282_pop_cas_failure_max_per_pop,
                              cas_failures);
}

static inline void hz3_s282_owner_inbox_on_pop_empty(int saw_head) {
    hz3_s282_owner_inbox_inc(saw_head ? &g_s282_pop_race_empty :
                                        &g_s282_pop_empty);
}

static inline void hz3_s282_owner_inbox_on_take_first_call(size_t depth) {
    hz3_s282_owner_inbox_inc(&g_s282_take_first_calls);
    hz3_s282_owner_inbox_depth_bucket(depth,
                                      &g_s282_take_depth_0,
                                      &g_s282_take_depth_1,
                                      &g_s282_take_depth_2_3,
                                      &g_s282_take_depth_4_7,
                                      &g_s282_take_depth_8_15,
                                      &g_s282_take_depth_16_plus);
}

static inline void hz3_s282_owner_inbox_on_take_first_hit(void) {
    hz3_s282_owner_inbox_inc(&g_s282_take_first_hit);
    hz3_s282_owner_inbox_inc(&g_s282_drain_return_first_nodes);
}

static inline void hz3_s282_owner_inbox_on_take_first_miss(size_t depth) {
    hz3_s282_owner_inbox_inc(depth == 0 ? &g_s282_take_first_miss_empty :
                                          &g_s282_take_first_miss_race);
}

static inline void hz3_s282_owner_inbox_on_drain_nodes(size_t nodes) {
    hz3_s282_owner_inbox_add(&g_s282_drain_nodes, nodes);
}

static inline void hz3_s282_owner_inbox_on_extra_fronted(void) {
    hz3_s282_owner_inbox_inc(&g_s282_drain_to_front_nodes);
}

static inline void hz3_s282_owner_inbox_on_extra_munmap(void) {
    hz3_s282_owner_inbox_inc(&g_s282_drain_to_munmap_nodes);
}
#else
static inline void hz3_s282_owner_inbox_on_push_attempt(void) {}
static inline void hz3_s282_owner_inbox_on_push_ok(size_t cas_failures) {
    (void)cas_failures;
}
static inline void hz3_s282_owner_inbox_on_push_reject_invalid(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_owner_dead(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_owner_exiting(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_shard_collision(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_budget(void) {}
static inline void hz3_s282_owner_inbox_on_push_depth(size_t depth) {
    (void)depth;
}
static inline void hz3_s282_owner_inbox_on_push_cas_attempt(void) {}
static inline void hz3_s282_owner_inbox_on_push_cas_failure(void) {}
static inline void hz3_s282_owner_inbox_on_pop_cas_attempt(void) {}
static inline void hz3_s282_owner_inbox_on_pop_cas_failure(void) {}
static inline void hz3_s282_owner_inbox_on_pop_success(size_t cas_failures) {
    (void)cas_failures;
}
static inline void hz3_s282_owner_inbox_on_pop_empty(int saw_head) {
    (void)saw_head;
}
static inline void hz3_s282_owner_inbox_on_take_first_call(size_t depth) {
    (void)depth;
}
static inline void hz3_s282_owner_inbox_on_take_first_hit(void) {}
static inline void hz3_s282_owner_inbox_on_take_first_miss(size_t depth) {
    (void)depth;
}
static inline void hz3_s282_owner_inbox_on_drain_nodes(size_t nodes) {
    (void)nodes;
}
static inline void hz3_s282_owner_inbox_on_extra_fronted(void) {}
static inline void hz3_s282_owner_inbox_on_extra_munmap(void) {}
#endif

#if HZ3_S285_LARGE_FIXED_COST_OBS
static inline void hz3_s285_inc(_Atomic size_t* p) {
    hz3_large_s240_inc(p);
}

static inline void hz3_s285_on_direct_way_load(void) {
    hz3_s285_inc(&g_s285_direct_way_loads);
}

static inline void hz3_s285_on_direct_key_busy(void) {
    hz3_s285_inc(&g_s285_direct_key_busy);
}

static inline void hz3_s285_on_direct_key_miss(void) {
    hz3_s285_inc(&g_s285_direct_key_miss);
}

static inline void hz3_s285_on_direct_key_match(void) {
    hz3_s285_inc(&g_s285_direct_key_match);
}

static inline void hz3_s285_on_direct_invalid_hdr(void) {
    hz3_s285_inc(&g_s285_direct_invalid_hdr);
}

static inline void hz3_s285_on_direct_state_nonactive(void) {
    hz3_s285_inc(&g_s285_direct_state_nonactive);
}

static inline void hz3_s285_on_direct_inuse_bad(void) {
    hz3_s285_inc(&g_s285_direct_inuse_bad);
}

static inline void hz3_s285_on_direct_not_retainable(int same_owner) {
    hz3_s285_inc(same_owner ? &g_s285_direct_not_retainable_same_owner :
                              &g_s285_direct_not_retainable_remote);
}

static inline void hz3_s285_on_direct_cas_attempt(void) {
    hz3_s285_inc(&g_s285_direct_cas_attempt);
}

static inline void hz3_s285_on_direct_cas_fail(void) {
    hz3_s285_inc(&g_s285_direct_cas_fail);
}

static inline void hz3_s285_on_direct_hit(int inbox) {
    hz3_s285_inc(inbox ? &g_s285_direct_hit_inbox : &g_s285_direct_hit_front);
}

static inline void hz3_s285_on_activate_check(void) {
    hz3_s285_inc(&g_s285_activate_check);
}

static inline void hz3_s285_on_activate_fail_header(void) {
    hz3_s285_inc(&g_s285_activate_fail_header);
}

static inline void hz3_s285_on_activate_fail_slot(void) {
    hz3_s285_inc(&g_s285_activate_fail_slot);
}

static inline void hz3_s285_on_activate_fail_flag(void) {
    hz3_s285_inc(&g_s285_activate_fail_flag);
}

static inline void hz3_s285_on_activate_fail_sc(void) {
    hz3_s285_inc(&g_s285_activate_fail_sc);
}

static inline void hz3_s285_on_activate_fail_alignment(void) {
    hz3_s285_inc(&g_s285_activate_fail_alignment);
}

static inline void hz3_s285_on_activate_fail_user_alignment(void) {
    hz3_s285_inc(&g_s285_activate_fail_user_alignment);
}

static inline void hz3_s285_on_activate_fail_range(void) {
    hz3_s285_inc(&g_s285_activate_fail_range);
}

static inline void hz3_s285_on_activate_fail_size(void) {
    hz3_s285_inc(&g_s285_activate_fail_size);
}

static inline void hz3_s285_on_activate_success(int inbox) {
    hz3_s285_inc(inbox ? &g_s285_activate_success_inbox :
                         &g_s285_activate_success_front);
}

static inline void hz3_s285_on_take_first_return_first(void) {
    hz3_s285_inc(&g_s285_take_first_return_first);
}

static inline void hz3_s285_on_take_first_extra_pop_attempt(void) {
    hz3_s285_inc(&g_s285_take_first_extra_pop_attempt);
}

static inline void hz3_s285_on_take_first_extra_pop_hit(void) {
    hz3_s285_inc(&g_s285_take_first_extra_pop_hit);
}

static inline void hz3_s285_on_take_first_extra_pop_empty(void) {
    hz3_s285_inc(&g_s285_take_first_extra_pop_empty);
}

static inline void hz3_s285_on_take_first_extra_front_result(int ok) {
    hz3_s285_inc(ok ? &g_s285_take_first_extra_front_ok :
                     &g_s285_take_first_extra_front_reject);
}

static inline void hz3_s285_on_take_first_drain_nodes(int drained) {
    if (drained <= 1) {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_1);
    } else if (drained <= 3) {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_2_3);
    } else if (drained <= 7) {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_4_7);
    } else {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_8_plus);
    }
}
#else
static inline void hz3_s285_on_direct_way_load(void) {}
static inline void hz3_s285_on_direct_key_busy(void) {}
static inline void hz3_s285_on_direct_key_miss(void) {}
static inline void hz3_s285_on_direct_key_match(void) {}
static inline void hz3_s285_on_direct_invalid_hdr(void) {}
static inline void hz3_s285_on_direct_state_nonactive(void) {}
static inline void hz3_s285_on_direct_inuse_bad(void) {}
static inline void hz3_s285_on_direct_not_retainable(int same_owner) {
    (void)same_owner;
}
static inline void hz3_s285_on_direct_cas_attempt(void) {}
static inline void hz3_s285_on_direct_cas_fail(void) {}
static inline void hz3_s285_on_direct_hit(int inbox) {
    (void)inbox;
}
static inline void hz3_s285_on_activate_check(void) {}
static inline void hz3_s285_on_activate_fail_header(void) {}
static inline void hz3_s285_on_activate_fail_slot(void) {}
static inline void hz3_s285_on_activate_fail_flag(void) {}
static inline void hz3_s285_on_activate_fail_sc(void) {}
static inline void hz3_s285_on_activate_fail_alignment(void) {}
static inline void hz3_s285_on_activate_fail_user_alignment(void) {}
static inline void hz3_s285_on_activate_fail_range(void) {}
static inline void hz3_s285_on_activate_fail_size(void) {}
static inline void hz3_s285_on_activate_success(int inbox) {
    (void)inbox;
}
static inline void hz3_s285_on_take_first_return_first(void) {}
static inline void hz3_s285_on_take_first_extra_pop_attempt(void) {}
static inline void hz3_s285_on_take_first_extra_pop_hit(void) {}
static inline void hz3_s285_on_take_first_extra_pop_empty(void) {}
static inline void hz3_s285_on_take_first_extra_front_result(int ok) {
    (void)ok;
}
static inline void hz3_s285_on_take_first_drain_nodes(int drained) {
    (void)drained;
}
#endif

#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
static inline void hz3_large_s251_fail(const char* where,
                                       Hz3LargeHdr* hdr,
                                       uint32_t expected,
                                       uint32_t actual,
                                       _Atomic size_t* counter) {
    hz3_large_s240_register_once();
    hz3_large_s240_inc(counter);
#if HZ3_S251_LARGE_OWNER_FRONT_FAILFAST
    fprintf(stderr,
            "[HZ3_LARGE_S251_FAILFAST] where=%s hdr=%p expected=%u actual=%u "
            "map_base=%p map_size=%zu user_ptr=%p owner=%u sc=%u flags=0x%x\n",
            where,
            (void*)hdr,
            expected,
            actual,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0,
            hdr ? hdr->user_ptr : NULL,
            hdr ? (unsigned)hdr->owner_shard : 255u,
            hdr ? (unsigned)hdr->sc : 255u,
            hdr ? (unsigned)hdr->flags : 0u);
    abort();
#else
    (void)where;
    (void)hdr;
    (void)expected;
    (void)actual;
#endif
}

static inline void hz3_large_s251_transition(Hz3LargeHdr* hdr,
                                             uint32_t expected,
                                             uint32_t desired,
                                             const char* where,
                                             _Atomic size_t* ok_counter,
                                             _Atomic size_t* fail_counter) {
    if (!hdr) {
        hz3_large_s251_fail(where, hdr, expected, 0, fail_counter);
        return;
    }
    uint32_t old = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (old != expected) {
        hz3_large_s251_fail(where, hdr, expected, old, fail_counter);
    }
    atomic_store_explicit(&hdr->state, desired, memory_order_release);
    hz3_large_s240_inc(ok_counter);
}

static inline void hz3_large_s251_expect(Hz3LargeHdr* hdr,
                                         uint32_t expected,
                                         const char* where,
                                         _Atomic size_t* fail_counter) {
    if (!hdr) {
        hz3_large_s251_fail(where, hdr, expected, 0, fail_counter);
        return;
    }
    uint32_t old = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (old != expected) {
        hz3_large_s251_fail(where, hdr, expected, old, fail_counter);
    }
}

static inline void hz3_large_s251_on_global_pop(Hz3LargeHdr* hdr) {
    hz3_large_s251_transition(hdr,
                              HZ3_LARGE_STATE_GLOBAL,
                              HZ3_LARGE_STATE_FREEING,
                              "global_pop",
                              &g_s251_global_pop_to_freeing,
                              &g_s251_global_pop_state_fail);
}
#else
static inline void hz3_large_s251_on_global_pop(Hz3LargeHdr* hdr) {
    (void)hdr;
}
#endif

#if HZ3_S240_LARGE_FLUSH_VALIDATE
static void hz3_large_s240_flush_debug_fail(const char* where,
                                            Hz3LargeHdr* hdr,
                                            const char* reason,
                                            size_t actual,
                                            size_t expected,
                                            _Atomic size_t* counter) {
    hz3_large_s240_register_once();
    hz3_large_s240_inc(&g_s240_flush_validate_failures);
    if (counter) {
        hz3_large_s240_inc(counter);
    }
    fprintf(stderr,
            "[HZ3_LARGE_S240_FLUSH_FAIL] where=%s reason=%s hdr=%p "
            "actual=%zu expected=%zu map_base=%p map_size=%zu user_ptr=%p "
            "in_use=%u next=%p next_free=%p state=%u owner=%u sc=%u flags=0x%x\n",
            where,
            reason,
            (void*)hdr,
            actual,
            expected,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0,
            hdr ? hdr->user_ptr : NULL,
            hdr ? (unsigned)hdr->in_use : 0u,
            hdr ? (void*)hdr->next : NULL,
            hdr ? (void*)hdr->next_free : NULL,
            hdr ? atomic_load_explicit(&hdr->state, memory_order_acquire) : 0u,
            hdr ? (unsigned)hdr->owner_shard : 255u,
            hdr ? (unsigned)hdr->sc : 255u,
            hdr ? (unsigned)hdr->flags : 0u);
#if HZ3_S240_LARGE_FLUSH_FAILFAST
    abort();
#endif
}

static int hz3_large_s240_flush_validate_node(Hz3LargeHdr* hdr,
                                              int sc,
                                              uint32_t expected_state,
                                              const char* where,
                                              _Atomic size_t* node_counter) {
    hz3_large_s240_register_once();
    hz3_large_s240_inc(&g_s240_flush_validate_calls);
    if (!hdr) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "null-node",
                                        0,
                                        1,
                                        &g_s240_flush_null_node_fail);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_base != (void*)hdr ||
        hdr->map_size == 0) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "bad-header",
                                        hdr->map_size,
                                        1,
                                        &g_s240_flush_bad_header);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (hdr->in_use != 0) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "in-use",
                                        hdr->in_use,
                                        0,
                                        &g_s240_flush_bad_header);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (hdr->next != NULL || hdr->next_free != NULL) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "linked-node",
                                        (size_t)(uintptr_t)(hdr->next ? hdr->next
                                                                      : hdr->next_free),
                                        0,
                                        &g_s240_flush_bad_link);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    uint32_t state = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (expected_state != 0 && state != expected_state) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "bad-state",
                                        state,
                                        expected_state,
                                        &g_s240_flush_bad_state);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT && hdr->sc != UINT8_MAX &&
        hdr->sc != (uint8_t)sc) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "sc-mismatch",
                                        hdr->sc,
                                        (uint8_t)sc,
                                        &g_s240_flush_sc_mismatch);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    return 1;
}

static void hz3_large_s240_inbox_account_underflow(const char* where,
                                                   const char* reason,
                                                   size_t actual,
                                                   size_t expected,
                                                   _Atomic size_t* counter) {
    hz3_large_s240_flush_debug_fail(where, NULL, reason, actual, expected, counter);
}
#define HZ3_S240_FLUSH_FRONT_COUNTER (&g_s240_flush_front_node_fail)
#define HZ3_S240_FLUSH_INBOX_COUNTER (&g_s240_flush_inbox_node_fail)
#define HZ3_S240_FLUSH_DRAIN_COUNTER (&g_s240_flush_drain_node_fail)
#else
static inline int hz3_large_s240_flush_validate_node(Hz3LargeHdr* hdr,
                                                     int sc,
                                                     uint32_t expected_state,
                                                     const char* where,
                                                     _Atomic size_t* node_counter) {
    (void)hdr;
    (void)sc;
    (void)expected_state;
    (void)where;
    (void)node_counter;
    return 1;
}
#define HZ3_S240_FLUSH_FRONT_COUNTER NULL
#define HZ3_S240_FLUSH_INBOX_COUNTER NULL
#define HZ3_S240_FLUSH_DRAIN_COUNTER NULL
#endif

#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
#define HZ3_S240_FLUSH_INBOX_POPPED_STATE HZ3_LARGE_STATE_FREEING
#else
#define HZ3_S240_FLUSH_INBOX_POPPED_STATE HZ3_LARGE_STATE_REMOTE
#endif

static inline void hz3_large_s240_on_activate(Hz3LargeHdr* hdr, int sc, int from_cache) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    uint8_t owner = hz3_large_s240_current_owner();
    hdr->owner_shard = owner;
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    hdr->flags = 0;
    hdr->direct_slot = UINT32_MAX;
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_ACTIVE, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_activate);
    if (from_cache) {
        hz3_large_s240_inc(&g_s240_activate_cache);
    } else {
        hz3_large_s240_inc(&g_s240_activate_mmap);
    }
    if (owner == UINT8_MAX) {
        hz3_large_s240_inc(&g_s240_owner_unknown_activate);
    }
#endif
#else
    (void)hdr;
    (void)sc;
    (void)from_cache;
#endif
}

#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline void hz3_large_s276_note_front_take_try(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_try);
#endif
}

static inline void hz3_large_s276_note_front_take_hit(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_hit);
#endif
}

static inline void hz3_large_s276_note_front_take_not_owner(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_not_owner);
#endif
}

static inline void hz3_large_s276_note_front_take_not_eligible(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_not_eligible);
#endif
}

static inline void hz3_large_s276_note_front_take_nonactive(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_nonactive);
#endif
}

static inline void hz3_large_s276_note_front_take_state_cas_fail(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_state_cas_fail);
#endif
}

static inline void hz3_large_s276_note_front_push_retained(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_push_retained);
#endif
}

static inline void hz3_large_s276_note_front_push_reject_clear(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_push_reject_clear);
#endif
}

static inline void hz3_large_s276_note_front_activate_fast(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_activate_fast);
#endif
}

static inline void hz3_large_s276_note_front_activate_rekey(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_activate_rekey);
#endif
}

static inline void hz3_large_s276_note_inbox_take_hit(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_take_hit);
#endif
}

static inline void hz3_large_s276_note_inbox_push_retained(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_push_retained);
#endif
}

static inline void hz3_large_s276_note_inbox_push_reject_clear(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_push_reject_clear);
#endif
}

static inline void hz3_large_s276_note_inbox_activate_fast(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_activate_fast);
#endif
}

static inline void hz3_large_s276_note_direct_clear(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_direct_clear);
#endif
}

static inline int hz3_large_s276_can_activate_retained(Hz3LargeHdr* hdr,
                                                       size_t size,
                                                       size_t alignment,
                                                       int sc) {
    hz3_s285_on_activate_check();
    if (!hdr ||
        hdr->magic != HZ3_LARGE_MAGIC ||
        hdr->map_base == NULL ||
        hdr->map_size == 0 ||
        hdr->user_ptr == NULL) {
        hz3_s285_on_activate_fail_header();
        return 0;
    }
    if (hdr->direct_slot == UINT32_MAX) {
        hz3_s285_on_activate_fail_slot();
        return 0;
    }
    if ((hdr->flags & HZ3_LARGE_F_DIRECT_MAPLESS) == 0 ||
        (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        hz3_s285_on_activate_fail_flag();
        return 0;
    }

    uint8_t observed_sc = hz3_large_s240_normalize_sc(sc);
    if (hdr->sc != observed_sc) {
        hz3_s285_on_activate_fail_sc();
        return 0;
    }
    if (alignment == 0 || (alignment & (alignment - 1u)) != 0) {
        hz3_s285_on_activate_fail_alignment();
        return 0;
    }
    uintptr_t user = (uintptr_t)hdr->user_ptr;
    if ((user & (uintptr_t)(alignment - 1u)) != 0) {
        hz3_s285_on_activate_fail_user_alignment();
        return 0;
    }
    if (size > SIZE_MAX - HZ3_LARGE_CANARY_SIZE) {
        hz3_s285_on_activate_fail_size();
        return 0;
    }
    uintptr_t base = (uintptr_t)hdr->map_base;
    uintptr_t end = base + hdr->map_size;
    if (end < base || user < base || user > end) {
        hz3_s285_on_activate_fail_range();
        return 0;
    }
#if HZ3_LARGE_CANARY_ENABLE
    size_t required = size + HZ3_LARGE_CANARY_SIZE;
#else
    size_t required = size;
#endif
    if (required > (size_t)(end - user)) {
        hz3_s285_on_activate_fail_size();
        return 0;
    }
    return 1;
}

static inline void hz3_large_s276_on_activate_retained(Hz3LargeHdr* hdr,
                                                       int sc,
                                                       int from_cache) {
    hz3_large_s240_register_once();
    uint8_t owner = hz3_large_s240_current_owner();
    hdr->owner_shard = owner;
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    hdr->flags = (uint16_t)(HZ3_LARGE_F_DIRECT_MAPLESS | HZ3_LARGE_F_DIRECT_RETAINED);
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_ACTIVE, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_activate);
    if (from_cache) {
        hz3_large_s240_inc(&g_s240_activate_cache);
    } else {
        hz3_large_s240_inc(&g_s240_activate_mmap);
    }
    if (owner == UINT8_MAX) {
        hz3_large_s240_inc(&g_s240_owner_unknown_activate);
    }
#endif
}
#endif

static inline void hz3_large_s240_on_cache_seed(Hz3LargeHdr* hdr, int sc) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    hdr->owner_shard = UINT8_MAX;
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    hdr->flags = 0;
    hdr->direct_slot = UINT32_MAX;
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_GLOBAL, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_cache_seed);
#endif
#else
    (void)hdr;
    (void)sc;
#endif
}

static inline void hz3_large_s240_on_cache_store(Hz3LargeHdr* hdr, int sc) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_GLOBAL, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_cache_store);
#endif
#else
    (void)hdr;
    (void)sc;
#endif
}

static inline void hz3_large_s240_on_munmap_store(Hz3LargeHdr* hdr) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_MUNMAP, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_munmap_store);
#endif
#else
    (void)hdr;
#endif
}

static inline void hz3_large_s240_on_free_take(Hz3LargeHdr* hdr, int sc) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_free_seen);
#endif
    int s276_already_freeing = 0;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    if (hdr &&
        (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0 &&
        atomic_load_explicit(&hdr->state, memory_order_acquire) ==
            HZ3_LARGE_STATE_FREEING) {
        s276_already_freeing = 1;
    }
#endif
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    if (s276_already_freeing) {
        hz3_large_s240_inc(&g_s251_free_take_to_freeing);
    } else {
        hz3_large_s251_transition(hdr,
                                  HZ3_LARGE_STATE_ACTIVE,
                                  HZ3_LARGE_STATE_FREEING,
                                  "free_take",
                                  &g_s251_free_take_to_freeing,
                                  &g_s251_free_take_state_fail);
    }
    uint32_t state = HZ3_LARGE_STATE_ACTIVE;
#else
    uint32_t state = s276_already_freeing
        ? HZ3_LARGE_STATE_ACTIVE
        : atomic_load_explicit(&hdr->state, memory_order_acquire);
#endif
    if (state == HZ3_LARGE_STATE_ACTIVE) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_free_active);
#endif
    } else {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_state_mismatch);
#endif
    }
    uint8_t observed_sc = hz3_large_s240_normalize_sc(sc);
    if (hdr->sc != UINT8_MAX && hdr->sc != observed_sc) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_sc_mismatch);
#endif
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    uint8_t cur = hz3_large_s240_current_owner();
    if (hdr->owner_shard == UINT8_MAX || cur == UINT8_MAX) {
        hz3_large_s240_inc(&g_s240_owner_unknown_free);
    } else if (hdr->owner_shard == cur) {
        hz3_large_s240_inc(&g_s240_owner_same_free);
    } else {
        hz3_large_s240_inc(&g_s240_owner_remote_free);
    }
#endif
#else
    (void)hdr;
    (void)sc;
#endif
}

#if HZ3_S240_LARGE_FRONT_CACHE
typedef struct Hz3LargeFront {
    Hz3LargeHdr* head[HZ3_LARGE_SC_COUNT];
    uint16_t count[HZ3_LARGE_SC_COUNT];
    size_t bytes;
} Hz3LargeFront;

static HZ3_TLS Hz3LargeFront t_hz3_large_front;

static inline int hz3_large_s240_front_eligible(int sc, size_t map_size) {
    return sc >= 0 &&
           sc < (HZ3_LARGE_SC_COUNT - 1) &&
           map_size > 0 &&
           map_size <= (size_t)HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES;
}

static inline void hz3_large_s240_front_note_add(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    size_t nodes = atomic_fetch_add_explicit(&g_s240_front_nodes_current, 1,
                                             memory_order_relaxed) + 1;
    size_t total = atomic_fetch_add_explicit(&g_s240_front_bytes_current, bytes,
                                             memory_order_relaxed) + bytes;
    hz3_large_s240_update_max(&g_s240_front_nodes_peak, nodes);
    hz3_large_s240_update_max(&g_s240_front_bytes_peak, total);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_s240_front_note_sub(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_sub(&g_s240_front_nodes_current, 1);
    hz3_large_s240_sub(&g_s240_front_bytes_current, bytes);
#else
    (void)bytes;
#endif
}

static inline Hz3LargeHdr* hz3_large_s240_front_pop(int sc, size_t need) {
    hz3_large_s240_register_once();
    if (!hz3_large_s240_front_eligible(sc, need)) {
        return NULL;
    }
    Hz3LargeHdr* hdr = t_hz3_large_front.head[sc];
    if (!hdr || hdr->map_size < need) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_front_pop_miss);
#endif
        return NULL;
    }
    t_hz3_large_front.head[sc] = hdr->next_free;
    hdr->next_free = NULL;
    if (t_hz3_large_front.count[sc] > 0) {
        t_hz3_large_front.count[sc]--;
    }
    if (t_hz3_large_front.bytes >= hdr->map_size) {
        t_hz3_large_front.bytes -= hdr->map_size;
    } else {
        t_hz3_large_front.bytes = 0;
    }
    hz3_large_s240_front_note_sub(hdr->map_size);
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    hz3_large_s251_transition(hdr,
                              HZ3_LARGE_STATE_FRONT,
                              HZ3_LARGE_STATE_FREEING,
                              "front_pop",
                              &g_s251_front_pop_to_freeing,
                              &g_s251_front_pop_state_fail);
#endif
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_front_pop_hit);
#endif
    return hdr;
}

static inline int hz3_large_s240_front_push(int sc, Hz3LargeHdr* hdr) {
    hz3_large_s240_register_once();
    if (!hz3_large_s240_front_eligible(sc, hdr->map_size)) {
        return 0;
    }
    if (t_hz3_large_front.count[sc] >= (uint16_t)HZ3_S240_LARGE_FRONT_MAX_NODES_PER_SC ||
        t_hz3_large_front.bytes + hdr->map_size > (size_t)HZ3_S240_LARGE_FRONT_MAX_BYTES_PER_THREAD) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_front_reject_limit);
#endif
        return 0;
    }
    hdr->next = NULL;
    hdr->next_free = t_hz3_large_front.head[sc];
    t_hz3_large_front.head[sc] = hdr;
    t_hz3_large_front.count[sc]++;
    t_hz3_large_front.bytes += hdr->map_size;
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    hz3_large_s251_expect(hdr,
                          HZ3_LARGE_STATE_FREEING,
                          "front_push",
                          &g_s251_front_push_state_fail);
#endif
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_FRONT, memory_order_release);
    hz3_large_s240_front_note_add(hdr->map_size);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_front_push);
#endif
    return 1;
}

static inline size_t hz3_large_s240_flush_dispose_node(Hz3LargeHdr* hdr,
                                                       int sc,
                                                       uint32_t expected_state,
                                                       const char* where,
                                                       _Atomic size_t* node_counter) {
    if (!hdr) {
        hz3_large_s240_flush_validate_node(hdr, sc, expected_state, where, node_counter);
        return 0;
    }
    hdr->next = NULL;
    hdr->next_free = NULL;
    hz3_large_s240_flush_validate_node(hdr, sc, expected_state, where, node_counter);
    size_t bytes = hdr->map_size;
    void* base = hdr->map_base;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    hz3_s276_direct_clear_retained(hdr);
#endif
    hz3_large_s240_on_munmap_store(hdr);
#if HZ3_LARGE_DEBUG
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(hdr);
    hz3_large_cache_lock_release();
#endif
    hdr->magic = 0;
#if HZ3_S240_LARGE_FLUSH_QUARANTINE
    hz3_large_s240_inc(&g_s240_flush_quarantine_objs);
    hz3_large_s240_add(&g_s240_flush_quarantine_bytes, bytes);
    (void)base;
#else
    hz3_large_os_munmap(base, bytes);
#endif
    return bytes;
}
#endif

#if HZ3_S240_LARGE_OWNER_INBOX && HZ3_S240_LARGE_FRONT_CACHE
typedef struct Hz3LargeInbox {
    _Atomic(Hz3LargeHdr*) head;
    _Atomic uint32_t count;
    _Atomic size_t bytes;
} Hz3LargeInbox;

static Hz3LargeInbox g_s240_large_inbox[HZ3_NUM_SHARDS][HZ3_LARGE_SC_COUNT];
static _Atomic uint32_t g_s240_large_inbox_pushers[HZ3_NUM_SHARDS];

static inline void hz3_large_s240_inbox_note_add(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    size_t nodes = atomic_fetch_add_explicit(&g_s240_inbox_nodes_current, 1,
                                             memory_order_relaxed) + 1;
    size_t total = atomic_fetch_add_explicit(&g_s240_inbox_bytes_current, bytes,
                                             memory_order_relaxed) + bytes;
    hz3_large_s240_update_max(&g_s240_inbox_nodes_peak, nodes);
    hz3_large_s240_update_max(&g_s240_inbox_bytes_peak, total);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_s240_inbox_note_sub(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_sub(&g_s240_inbox_nodes_current, 1);
    hz3_large_s240_sub(&g_s240_inbox_bytes_current, bytes);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_s240_inbox_account_pop(Hz3LargeInbox* inbox,
                                                    uint32_t count,
                                                    size_t bytes,
                                                    const char* where) {
#if HZ3_S240_LARGE_FLUSH_VALIDATE
    uint32_t old_count = atomic_load_explicit(&inbox->count, memory_order_acquire);
    for (;;) {
        if (old_count < count) {
            hz3_large_s240_inbox_account_underflow(where,
                                                   "inbox-count-underflow",
                                                   old_count,
                                                   count,
                                                   &g_s240_flush_inbox_count_underflow);
            atomic_store_explicit(&inbox->count, 0, memory_order_release);
            break;
        }
        if (atomic_compare_exchange_weak_explicit(&inbox->count,
                                                  &old_count,
                                                  old_count - count,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            break;
        }
    }

    size_t old_bytes = atomic_load_explicit(&inbox->bytes, memory_order_acquire);
    for (;;) {
        if (old_bytes < bytes) {
            hz3_large_s240_inbox_account_underflow(where,
                                                   "inbox-bytes-underflow",
                                                   old_bytes,
                                                   bytes,
                                                   &g_s240_flush_inbox_bytes_underflow);
            atomic_store_explicit(&inbox->bytes, 0, memory_order_release);
            break;
        }
        if (atomic_compare_exchange_weak_explicit(&inbox->bytes,
                                                  &old_bytes,
                                                  old_bytes - bytes,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            break;
        }
    }
#else
    atomic_fetch_sub_explicit(&inbox->count, count, memory_order_relaxed);
    atomic_fetch_sub_explicit(&inbox->bytes, bytes, memory_order_relaxed);
#endif
}

static inline int hz3_large_s240_inbox_eligible(int sc, size_t map_size) {
    return hz3_large_s240_front_eligible(sc, map_size);
}

static inline int hz3_large_s240_owner_accepts_remote(uint8_t owner, uint8_t cur) {
    if (owner == UINT8_MAX || owner >= HZ3_NUM_SHARDS || cur == UINT8_MAX || owner == cur) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_owner_dead);
#endif
        hz3_s282_owner_inbox_on_push_reject_owner_dead();
        return 0;
    }
    uint32_t live = hz3_shard_live_count(owner);
    if (live == 0) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_owner_dead);
#endif
        hz3_s282_owner_inbox_on_push_reject_owner_dead();
        return 0;
    }
    if (hz3_shard_is_exiting(owner)) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_owner_exiting);
#endif
        hz3_s282_owner_inbox_on_push_reject_owner_exiting();
        return 0;
    }
    if (live > 1) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_shard_collision);
#endif
        hz3_s282_owner_inbox_on_push_reject_shard_collision();
        return 0;
    }
    return 1;
}

static inline int hz3_large_s240_inbox_push_remote(int sc, Hz3LargeHdr* hdr, uint8_t cur) {
    hz3_large_s240_register_once();
    hz3_s282_owner_inbox_on_push_attempt();
    if (!hdr || !hz3_large_s240_inbox_eligible(sc, hdr->map_size)) {
        hz3_s282_owner_inbox_on_push_reject_invalid();
        return 0;
    }
    uint8_t owner = hdr->owner_shard;
    if (!hz3_large_s240_owner_accepts_remote(owner, cur)) {
        return 0;
    }

    uint32_t pushers = atomic_fetch_add_explicit(&g_s240_large_inbox_pushers[owner], 1,
                                                 memory_order_acq_rel) + 1u;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_update_max(&g_s240_inbox_pushers_peak, (size_t)pushers);
#else
    (void)pushers;
#endif
    if (!hz3_large_s240_owner_accepts_remote(owner, cur)) {
        atomic_fetch_sub_explicit(&g_s240_large_inbox_pushers[owner], 1,
                                  memory_order_acq_rel);
        return 0;
    }

    Hz3LargeInbox* inbox = &g_s240_large_inbox[owner][sc];
    uint32_t count = atomic_load_explicit(&inbox->count, memory_order_relaxed);
    size_t bytes = atomic_load_explicit(&inbox->bytes, memory_order_relaxed);
    hz3_s282_owner_inbox_on_push_depth((size_t)count);
    if (count >= (uint32_t)HZ3_S240_LARGE_INBOX_MAX_NODES_PER_SC ||
        bytes + hdr->map_size < bytes ||
        bytes + hdr->map_size > (size_t)HZ3_S240_LARGE_INBOX_MAX_BYTES_PER_SC) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_budget);
#endif
        hz3_s282_owner_inbox_on_push_reject_budget();
        atomic_fetch_sub_explicit(&g_s240_large_inbox_pushers[owner], 1,
                                  memory_order_acq_rel);
        return 0;
    }

    hdr->in_use = 0;
    hdr->next = NULL;
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    hz3_large_s251_expect(hdr,
                          HZ3_LARGE_STATE_FREEING,
                          "inbox_push",
                          &g_s251_inbox_push_state_fail);
#endif
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_REMOTE, memory_order_release);

    // Publish accounting before the stack head. The owner can drain immediately
    // after seeing `head`, so count/bytes must already describe the node.
    atomic_fetch_add_explicit(&inbox->count, 1, memory_order_acq_rel);
    atomic_fetch_add_explicit(&inbox->bytes, hdr->map_size, memory_order_acq_rel);
    hz3_large_s240_inbox_note_add(hdr->map_size);

    Hz3LargeHdr* old = atomic_load_explicit(&inbox->head, memory_order_relaxed);
    size_t s282_push_cas_failures = 0;
    for (;;) {
        hdr->next_free = old;
        hz3_s282_owner_inbox_on_push_cas_attempt();
        if (atomic_compare_exchange_weak_explicit(&inbox->head,
                                                  &old,
                                                  hdr,
                                                  memory_order_release,
                                                  memory_order_relaxed)) {
            break;
        }
        s282_push_cas_failures++;
        hz3_s282_owner_inbox_on_push_cas_failure();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_cas_retries);
#endif
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_remote_push);
#endif
    hz3_s282_owner_inbox_on_push_ok(s282_push_cas_failures);
    atomic_fetch_sub_explicit(&g_s240_large_inbox_pushers[owner], 1,
                              memory_order_acq_rel);
    return 1;
}

static inline Hz3LargeHdr* hz3_large_s240_inbox_pop_one(uint8_t owner, int sc) {
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return NULL;
    }
    Hz3LargeInbox* inbox = &g_s240_large_inbox[owner][sc];
    Hz3LargeHdr* head = atomic_load_explicit(&inbox->head, memory_order_acquire);
    int s282_saw_head = head != NULL;
    size_t s282_pop_cas_failures = 0;
    while (head) {
        Hz3LargeHdr* next = head->next_free;
        hz3_s282_owner_inbox_on_pop_cas_attempt();
        if (atomic_compare_exchange_weak_explicit(&inbox->head,
                                                  &head,
                                                  next,
                                                  memory_order_acquire,
                                                  memory_order_relaxed)) {
            hz3_s282_owner_inbox_on_pop_success(s282_pop_cas_failures);
            head->next_free = NULL;
            hz3_large_s240_inbox_account_pop(inbox,
                                             1,
                                             head->map_size,
                                             "inbox-pop-one");
            hz3_large_s240_inbox_note_sub(head->map_size);
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
            hz3_large_s251_transition(head,
                                      HZ3_LARGE_STATE_REMOTE,
                                      HZ3_LARGE_STATE_FREEING,
                                      "inbox_pop",
                                      &g_s251_inbox_pop_to_freeing,
                                      &g_s251_inbox_pop_state_fail);
#endif
            return head;
        }
        s282_saw_head = 1;
        s282_pop_cas_failures++;
        hz3_s282_owner_inbox_on_pop_cas_failure();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_cas_retries);
#endif
    }
    hz3_s282_owner_inbox_on_pop_empty(s282_saw_head);
    return NULL;
}

#if HZ3_S245_LARGE_INBOX_BATCH_POP || HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || \
    HZ3_S274_LARGE_INBOX_SIZE_DRAIN
static inline Hz3LargeHdr* hz3_large_s245_inbox_pop_batch(uint8_t owner,
                                                          int sc,
                                                          int max_nodes,
                                                          int* out_count) {
    if (out_count) {
        *out_count = 0;
    }
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_LARGE_SC_COUNT ||
        max_nodes <= 0) {
        return NULL;
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s245_batch_pop_calls);
#endif
    Hz3LargeInbox* inbox = &g_s240_large_inbox[owner][sc];
    Hz3LargeHdr* head = atomic_load_explicit(&inbox->head, memory_order_acquire);
    while (head) {
        Hz3LargeHdr* tail = head;
        int count = 1;
        size_t bytes = head->map_size;
        while (count < max_nodes && tail->next_free) {
            tail = tail->next_free;
            count++;
            bytes += tail->map_size;
        }

        Hz3LargeHdr* rest = tail->next_free;
        if (atomic_compare_exchange_weak_explicit(&inbox->head,
                                                  &head,
                                                  rest,
                                                  memory_order_acquire,
                                                  memory_order_relaxed)) {
            tail->next_free = NULL;
            hz3_large_s240_inbox_account_pop(inbox,
                                             (uint32_t)count,
                                             bytes,
                                             "inbox-pop-batch");
            for (Hz3LargeHdr* cur = head; cur; cur = cur->next_free) {
                hz3_large_s240_inbox_note_sub(cur->map_size);
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
                hz3_large_s251_transition(cur,
                                          HZ3_LARGE_STATE_REMOTE,
                                          HZ3_LARGE_STATE_FREEING,
                                          "inbox_pop_batch",
                                          &g_s251_inbox_pop_to_freeing,
                                          &g_s251_inbox_pop_state_fail);
#endif
            }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s245_batch_pop_nonempty);
            hz3_large_s240_add(&g_s245_batch_pop_objs, (size_t)count);
            hz3_large_s240_update_max(&g_s245_batch_pop_max_objs, (size_t)count);
#endif
            if (out_count) {
                *out_count = count;
            }
            return head;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_cas_retries);
        hz3_large_s240_inc(&g_s245_batch_pop_cas_retries);
#endif
    }
    return NULL;
}
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN
static inline int hz3_large_s274_inbox_batch_for_request(size_t req_size,
                                                         int fallback_max_nodes) {
    if (req_size < (size_t)HZ3_S274_LARGE_INBOX_MIN_REQUEST_BYTES ||
        req_size > (size_t)HZ3_S274_LARGE_INBOX_MAX_REQUEST_BYTES) {
        return 1;
    }

    int batch = (int)HZ3_S274_LARGE_INBOX_DRAIN_BATCH;
    if (batch < 1) {
        return 1;
    }
    if (fallback_max_nodes > 0 && batch > fallback_max_nodes) {
        return fallback_max_nodes;
    }
    return batch;
}
#else
static inline int hz3_large_s274_inbox_batch_for_request(size_t req_size,
                                                         int fallback_max_nodes) {
    (void)req_size;
    return fallback_max_nodes;
}
#endif

static inline int hz3_large_s240_inbox_drain_current_to_front(int sc, int max_nodes) {
    hz3_large_s240_register_once();
    if (max_nodes <= 0) {
        return 0;
    }
    uint8_t owner = hz3_large_s240_current_owner();
    if (owner == UINT8_MAX || owner >= HZ3_NUM_SHARDS ||
        sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return 0;
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_drain_calls);
#endif
    int drained = 0;
#if HZ3_S245_LARGE_INBOX_BATCH_POP
    int batch_count = 0;
    Hz3LargeHdr* batch = hz3_large_s245_inbox_pop_batch(owner, sc, max_nodes, &batch_count);
    (void)batch_count;
    while (batch) {
        Hz3LargeHdr* hdr = batch;
        batch = hdr->next_free;
        hdr->next_free = NULL;
        hdr->in_use = 0;
        hdr->next = NULL;
        if (hz3_large_s240_front_push(sc, hdr)) {
            drained++;
            continue;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
#endif
        (void)hz3_large_s240_flush_dispose_node(hdr,
                                                sc,
                                                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                "inbox-drain-front-reject",
                                                HZ3_S240_FLUSH_DRAIN_COUNTER);
    }
#else
    for (int i = 0; i < max_nodes; i++) {
        Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
        if (!hdr) {
            break;
        }
        hdr->in_use = 0;
        hdr->next = NULL;
        if (hz3_large_s240_front_push(sc, hdr)) {
            drained++;
            continue;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
#endif
        (void)hz3_large_s240_flush_dispose_node(hdr,
                                                sc,
                                                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                "inbox-drain-front-reject",
                                                HZ3_S240_FLUSH_DRAIN_COUNTER);
    }
#endif
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    if (drained > 0) {
        hz3_large_s240_inc(&g_s240_inbox_drain_nonempty);
        hz3_large_s240_add(&g_s240_inbox_drain_objs, (size_t)drained);
    }
#endif
    return drained;
}

#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
static inline Hz3LargeHdr* hz3_large_s246_inbox_drain_current_take_first_to_front(
    int sc,
    int max_nodes,
    size_t req_size,
    int* out_drained) {
    hz3_large_s240_register_once();
    if (out_drained) {
        *out_drained = 0;
    }
    if (max_nodes <= 0) {
        return NULL;
    }
    uint8_t owner = hz3_large_s240_current_owner();
    if (owner == UINT8_MAX || owner >= HZ3_NUM_SHARDS ||
        sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return NULL;
    }
    size_t s282_take_depth = (size_t)atomic_load_explicit(
        &g_s240_large_inbox[owner][sc].count, memory_order_relaxed);
    hz3_s282_owner_inbox_on_take_first_call(s282_take_depth);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_drain_calls);
    hz3_large_s240_inc(&g_s246_take_first_calls);
#endif

    Hz3LargeHdr* first = NULL;
#if HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    int batch_count = 0;
    int use_batch_pop = 0;
#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    int selected_max_nodes = hz3_large_s274_inbox_batch_for_request(req_size, max_nodes);
#else
    (void)req_size;
    int selected_max_nodes = max_nodes;
#endif
    if (selected_max_nodes > 1) {
        use_batch_pop = 1;
        first = hz3_large_s245_inbox_pop_batch(owner, sc, selected_max_nodes, &batch_count);
    } else {
        first = hz3_large_s240_inbox_pop_one(owner, sc);
    }
#else
    (void)req_size;
    first = hz3_large_s240_inbox_pop_one(owner, sc);
#endif
    if (!first) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s246_take_first_misses);
#endif
        hz3_s282_owner_inbox_on_take_first_miss(s282_take_depth);
        return NULL;
    }

    int drained = 1;
    Hz3LargeHdr* extras = first->next_free;
    first->next_free = NULL;
    first->in_use = 0;
    first->next = NULL;
    hz3_s285_on_take_first_return_first();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s246_take_first_hits);
    hz3_large_s240_inc(&g_s246_take_first_direct);
#endif
    hz3_s282_owner_inbox_on_take_first_hit();
    int refill_limit = max_nodes;
#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP
    if (req_size >= (size_t)HZ3_S286_LARGE_TAKE_FIRST_MIN_REQUEST_BYTES &&
        req_size <= (size_t)HZ3_S286_LARGE_TAKE_FIRST_MAX_REQUEST_BYTES &&
        refill_limit > (int)HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES) {
        refill_limit = (int)HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES;
    }
#endif

#if HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    (void)batch_count;
    if (use_batch_pop) {
        Hz3LargeHdr* pending = extras;
        while (pending) {
            Hz3LargeHdr* hdr = pending;
            pending = hdr->next_free;
            hdr->next_free = NULL;
            hdr->in_use = 0;
            hdr->next = NULL;
            if (hz3_large_s240_front_push(sc, hdr)) {
                drained++;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
                hz3_large_s240_inc(&g_s246_take_first_extra_fronted);
#endif
                hz3_s282_owner_inbox_on_extra_fronted();
                continue;
            }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
            hz3_large_s240_inc(&g_s246_take_first_extra_munmap);
#endif
            hz3_s282_owner_inbox_on_extra_munmap();
            (void)hz3_large_s240_flush_dispose_node(hdr,
                                                    sc,
                                                    HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                    "inbox-take-first-batch-extra-munmap",
                                                    HZ3_S240_FLUSH_DRAIN_COUNTER);
        }
    } else {
        (void)extras;
        for (int i = 1; i < refill_limit; i++) {
            hz3_s285_on_take_first_extra_pop_attempt();
            Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
            if (!hdr) {
                hz3_s285_on_take_first_extra_pop_empty();
                break;
            }
            hz3_s285_on_take_first_extra_pop_hit();
            hdr->in_use = 0;
            hdr->next = NULL;
            int s285_front_ok = hz3_large_s240_front_push(sc, hdr);
            hz3_s285_on_take_first_extra_front_result(s285_front_ok);
            if (s285_front_ok) {
                drained++;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
                hz3_large_s240_inc(&g_s246_take_first_extra_fronted);
#endif
                hz3_s282_owner_inbox_on_extra_fronted();
                continue;
            }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
            hz3_large_s240_inc(&g_s246_take_first_extra_munmap);
#endif
            hz3_s282_owner_inbox_on_extra_munmap();
            (void)hz3_large_s240_flush_dispose_node(hdr,
                                                    sc,
                                                    HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                    "inbox-take-first-extra-munmap",
                                                    HZ3_S240_FLUSH_DRAIN_COUNTER);
        }
    }
#else
    (void)extras;
    for (int i = 1; i < refill_limit; i++) {
        hz3_s285_on_take_first_extra_pop_attempt();
        Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
        if (!hdr) {
            hz3_s285_on_take_first_extra_pop_empty();
            break;
        }
        hz3_s285_on_take_first_extra_pop_hit();
        hdr->in_use = 0;
        hdr->next = NULL;
        int s285_front_ok = hz3_large_s240_front_push(sc, hdr);
        hz3_s285_on_take_first_extra_front_result(s285_front_ok);
        if (s285_front_ok) {
            drained++;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s246_take_first_extra_fronted);
#endif
            hz3_s282_owner_inbox_on_extra_fronted();
            continue;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
        hz3_large_s240_inc(&g_s246_take_first_extra_munmap);
#endif
        hz3_s282_owner_inbox_on_extra_munmap();
        (void)hz3_large_s240_flush_dispose_node(hdr,
                                                sc,
                                                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                "inbox-take-first-extra-munmap",
                                                HZ3_S240_FLUSH_DRAIN_COUNTER);
    }
#endif

#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_drain_nonempty);
    hz3_large_s240_add(&g_s240_inbox_drain_objs, (size_t)drained);
#endif
    hz3_s285_on_take_first_drain_nodes(drained);
    hz3_s282_owner_inbox_on_drain_nodes((size_t)drained);
    if (out_drained) {
        *out_drained = drained;
    }
    return first;
}
#endif

static inline void hz3_large_s240_inbox_flush_owner(uint8_t owner) {
    if (owner >= HZ3_NUM_SHARDS) {
        return;
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_flush_calls);
#endif
    uint32_t spins = 0;
    while (spins < (uint32_t)HZ3_S240_LARGE_INBOX_DTOR_SPIN_LIMIT &&
           atomic_load_explicit(&g_s240_large_inbox_pushers[owner], memory_order_acquire) != 0) {
        spins++;
        atomic_signal_fence(memory_order_seq_cst);
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    if (spins > 0) {
        hz3_large_s240_add(&g_s240_inbox_flush_wait_spins, (size_t)spins);
    }
#endif
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        for (;;) {
            Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
            if (!hdr) {
                break;
            }
            size_t bytes = hz3_large_s240_flush_dispose_node(
                hdr,
                sc,
                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                "inbox-flush-owner",
                HZ3_S240_FLUSH_INBOX_COUNTER);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_inbox_flush_objs);
            hz3_large_s240_add(&g_s240_inbox_flush_bytes, bytes);
#else
            (void)bytes;
#endif
        }
    }
}
#else
static inline int hz3_large_s240_inbox_push_remote(int sc, Hz3LargeHdr* hdr, uint8_t cur) {
    (void)sc;
    (void)hdr;
    (void)cur;
    return 0;
}

static inline int hz3_large_s240_inbox_drain_current_to_front(int sc, int max_nodes) {
    (void)sc;
    (void)max_nodes;
    return 0;
}

static inline void hz3_large_s240_inbox_flush_owner(uint8_t owner) {
    (void)owner;
}
#endif

void hz3_large_s240_tls_flush(void) {
#if HZ3_S240_LARGE_FRONT_CACHE
    hz3_large_s240_register_once();
#if HZ3_S240_LARGE_OWNER_INBOX
    if (t_hz3_cache.initialized) {
        hz3_large_s240_inbox_flush_owner((uint8_t)t_hz3_cache.my_shard);
    }
#endif
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_front_flush_calls);
#endif
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        Hz3LargeHdr* hdr = t_hz3_large_front.head[sc];
        t_hz3_large_front.head[sc] = NULL;
        t_hz3_large_front.count[sc] = 0;
        while (hdr) {
            Hz3LargeHdr* next = hdr->next_free;
            size_t bytes = hz3_large_s240_flush_dispose_node(
                hdr,
                sc,
                HZ3_LARGE_STATE_FRONT,
                "tls-front-flush",
                HZ3_S240_FLUSH_FRONT_COUNTER);
            hz3_large_s240_front_note_sub(bytes);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_front_flush_objs);
            hz3_large_s240_inc(&g_s240_front_evict_munmap);
            hz3_large_s240_add(&g_s240_front_flush_bytes, bytes);
#endif
            hdr = next;
        }
    }
    t_hz3_large_front.bytes = 0;
#endif
}

#if HZ3_LARGE_ALIGNED_OBS
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

#if HZ3_S275_LARGE_ACTIVATION_OBS
typedef struct Hz3S275ActivationTimer {
    _Atomic size_t calls;
    _Atomic size_t ns_total;
    _Atomic size_t ns_max;
} Hz3S275ActivationTimer;

static Hz3S275ActivationTimer g_s275_t_userptr_front;
static Hz3S275ActivationTimer g_s275_t_userptr_inbox;
static Hz3S275ActivationTimer g_s275_t_userptr_global;
static Hz3S275ActivationTimer g_s275_t_userptr_mmap;
static Hz3S275ActivationTimer g_s275_t_meta_front;
static Hz3S275ActivationTimer g_s275_t_meta_inbox;
static Hz3S275ActivationTimer g_s275_t_meta_global;
static Hz3S275ActivationTimer g_s275_t_meta_mmap;
static Hz3S275ActivationTimer g_s275_t_insert_direct_mapless;
static Hz3S275ActivationTimer g_s275_t_insert_active_map;
static Hz3S275ActivationTimer g_s275_t_insert_direct_shadow;
static Hz3S275ActivationTimer g_s275_t_direct_take;

static _Atomic size_t g_s275_insert_direct_mapless_hit = 0;
static _Atomic size_t g_s275_insert_direct_mapless_miss = 0;
static _Atomic size_t g_s275_direct_take_hit = 0;
static _Atomic size_t g_s275_direct_take_miss = 0;

static inline size_t hz3_s275_activation_timing_start(void) {
    return hz3_large_aligned_obs_timing_start();
}

static inline void hz3_s275_activation_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static inline size_t hz3_s275_activation_load(const _Atomic size_t* p) {
    return atomic_load_explicit(p, memory_order_relaxed);
}

static inline void hz3_s275_activation_note_timer(Hz3S275ActivationTimer* timer,
                                                  size_t start_ns) {
    hz3_large_aligned_obs_note_elapsed(
        &timer->calls, &timer->ns_total, &timer->ns_max, start_ns);
}

static inline size_t hz3_s275_activation_timer_calls(Hz3S275ActivationTimer* timer) {
    return hz3_s275_activation_load(&timer->calls);
}

static inline size_t hz3_s275_activation_timer_total(Hz3S275ActivationTimer* timer) {
    return hz3_s275_activation_load(&timer->ns_total);
}

static inline size_t hz3_s275_activation_timer_max(Hz3S275ActivationTimer* timer) {
    return hz3_s275_activation_load(&timer->ns_max);
}

static inline size_t hz3_s275_activation_timer_avg(Hz3S275ActivationTimer* timer) {
    size_t calls = hz3_s275_activation_timer_calls(timer);
    size_t total = hz3_s275_activation_timer_total(timer);
    return calls ? (total / calls) : 0;
}

static inline void hz3_s275_on_userptr_front_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_front, start_ns);
}

static inline void hz3_s275_on_userptr_inbox_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_inbox, start_ns);
}

static inline void hz3_s275_on_userptr_global_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_global, start_ns);
}

static inline void hz3_s275_on_userptr_mmap_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_userptr_mmap, start_ns);
}

static inline void hz3_s275_on_meta_front_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_front, start_ns);
}

static inline void hz3_s275_on_meta_inbox_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_inbox, start_ns);
}

static inline void hz3_s275_on_meta_global_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_global, start_ns);
}

static inline void hz3_s275_on_meta_mmap_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_meta_mmap, start_ns);
}

static inline void hz3_s275_on_insert_direct_mapless_elapsed(size_t start_ns,
                                                             int hit) {
    hz3_s275_activation_inc(hit ? &g_s275_insert_direct_mapless_hit :
                                  &g_s275_insert_direct_mapless_miss);
    hz3_s275_activation_note_timer(&g_s275_t_insert_direct_mapless, start_ns);
}

static inline void hz3_s275_on_insert_active_map_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_insert_active_map, start_ns);
}

static inline void hz3_s275_on_insert_direct_shadow_elapsed(size_t start_ns) {
    hz3_s275_activation_note_timer(&g_s275_t_insert_direct_shadow, start_ns);
}

static inline void hz3_s275_on_direct_take_elapsed(size_t start_ns, int hit) {
    hz3_s275_activation_inc(hit ? &g_s275_direct_take_hit : &g_s275_direct_take_miss);
    hz3_s275_activation_note_timer(&g_s275_t_direct_take, start_ns);
}

static void hz3_s275_activation_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S275_ACTIVATION] "
            "userptr_front_calls=%zu userptr_front_ns_avg=%zu userptr_front_ns_max=%zu "
            "userptr_inbox_calls=%zu userptr_inbox_ns_avg=%zu userptr_inbox_ns_max=%zu "
            "userptr_global_calls=%zu userptr_global_ns_avg=%zu userptr_global_ns_max=%zu "
            "userptr_mmap_calls=%zu userptr_mmap_ns_avg=%zu userptr_mmap_ns_max=%zu "
            "meta_front_calls=%zu meta_front_ns_avg=%zu meta_front_ns_max=%zu "
            "meta_inbox_calls=%zu meta_inbox_ns_avg=%zu meta_inbox_ns_max=%zu "
            "meta_global_calls=%zu meta_global_ns_avg=%zu meta_global_ns_max=%zu "
            "meta_mmap_calls=%zu meta_mmap_ns_avg=%zu meta_mmap_ns_max=%zu\n",
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_front),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_front),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_front),
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_inbox),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_inbox),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_inbox),
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_global),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_global),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_global),
            hz3_s275_activation_timer_calls(&g_s275_t_userptr_mmap),
            hz3_s275_activation_timer_avg(&g_s275_t_userptr_mmap),
            hz3_s275_activation_timer_max(&g_s275_t_userptr_mmap),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_front),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_front),
            hz3_s275_activation_timer_max(&g_s275_t_meta_front),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_inbox),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_inbox),
            hz3_s275_activation_timer_max(&g_s275_t_meta_inbox),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_global),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_global),
            hz3_s275_activation_timer_max(&g_s275_t_meta_global),
            hz3_s275_activation_timer_calls(&g_s275_t_meta_mmap),
            hz3_s275_activation_timer_avg(&g_s275_t_meta_mmap),
            hz3_s275_activation_timer_max(&g_s275_t_meta_mmap));
    fprintf(stderr,
            "[HZ3_S275_DIRECT_MAP] "
            "insert_direct_mapless_calls=%zu insert_direct_mapless_hit=%zu "
            "insert_direct_mapless_miss=%zu insert_direct_mapless_ns_avg=%zu "
            "insert_direct_mapless_ns_max=%zu insert_active_map_calls=%zu "
            "insert_active_map_ns_avg=%zu insert_active_map_ns_max=%zu "
            "insert_direct_shadow_calls=%zu insert_direct_shadow_ns_avg=%zu "
            "insert_direct_shadow_ns_max=%zu direct_take_calls=%zu "
            "direct_take_hit=%zu direct_take_miss=%zu direct_take_ns_avg=%zu "
            "direct_take_ns_max=%zu\n",
            hz3_s275_activation_timer_calls(&g_s275_t_insert_direct_mapless),
            hz3_s275_activation_load(&g_s275_insert_direct_mapless_hit),
            hz3_s275_activation_load(&g_s275_insert_direct_mapless_miss),
            hz3_s275_activation_timer_avg(&g_s275_t_insert_direct_mapless),
            hz3_s275_activation_timer_max(&g_s275_t_insert_direct_mapless),
            hz3_s275_activation_timer_calls(&g_s275_t_insert_active_map),
            hz3_s275_activation_timer_avg(&g_s275_t_insert_active_map),
            hz3_s275_activation_timer_max(&g_s275_t_insert_active_map),
            hz3_s275_activation_timer_calls(&g_s275_t_insert_direct_shadow),
            hz3_s275_activation_timer_avg(&g_s275_t_insert_direct_shadow),
            hz3_s275_activation_timer_max(&g_s275_t_insert_direct_shadow),
            hz3_s275_activation_timer_calls(&g_s275_t_direct_take),
            hz3_s275_activation_load(&g_s275_direct_take_hit),
            hz3_s275_activation_load(&g_s275_direct_take_miss),
            hz3_s275_activation_timer_avg(&g_s275_t_direct_take),
            hz3_s275_activation_timer_max(&g_s275_t_direct_take));
}
#else
static inline size_t hz3_s275_activation_timing_start(void) { return 0; }
static inline void hz3_s275_on_userptr_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_mapless_elapsed(size_t start_ns,
                                                             int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s275_on_insert_active_map_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_shadow_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_direct_take_elapsed(size_t start_ns, int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s275_activation_dump_final(void) {}
#endif

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
    hz3_s243_residual_dump_final();
    hz3_s275_activation_dump_final();
    hz3_s280_free_route_dump_final();
}

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
#else
static inline size_t hz3_large_aligned_obs_timing_start(void) {
    return 0;
}
static inline void hz3_large_aligned_obs_on_map_insert_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_large_aligned_obs_on_map_take_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_large_aligned_obs_on_alloc_cache_pop_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_large_aligned_obs_on_free_admit_elapsed(size_t start_ns) {
    (void)start_ns;
}
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
static inline size_t hz3_s275_activation_timing_start(void) { return 0; }
static inline void hz3_s275_on_userptr_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_mapless_elapsed(size_t start_ns,
                                                             int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s275_on_insert_active_map_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_shadow_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_direct_take_elapsed(size_t start_ns, int hit) {
    (void)start_ns;
    (void)hit;
}
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
static inline void hz3_large_aligned_obs_on_normal_alloc(size_t size) {
    (void)size;
}
static inline void hz3_large_aligned_obs_on_invalid(size_t size, size_t alignment) {
    (void)size;
    (void)alignment;
}
static inline void hz3_large_aligned_obs_on_aligned_alloc(size_t size, size_t alignment) {
    (void)size;
    (void)alignment;
}
static inline void hz3_large_aligned_obs_on_aligned_cache_hit(void) {}
static inline void hz3_large_aligned_obs_on_aligned_cache_miss(void) {}
static inline void hz3_large_aligned_obs_on_aligned_mmap(size_t bytes) {
    (void)bytes;
}
static inline void hz3_large_aligned_obs_on_aligned_batch_mmap(size_t bytes) {
    (void)bytes;
}
static inline void hz3_large_aligned_obs_on_free(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_large_aligned_obs_on_take_hit(size_t scan_steps) {
    (void)scan_steps;
}
static inline void hz3_large_aligned_obs_on_take_miss(size_t scan_steps) {
    (void)scan_steps;
}
static inline void hz3_large_aligned_obs_on_admit_invalid(void) {}
static inline void hz3_large_aligned_obs_on_admit_oversize(void) {}
static inline void hz3_large_aligned_obs_on_admit_precheck_yes(void) {}
static inline void hz3_large_aligned_obs_on_admit_lock_decision(int can_cache) {
    (void)can_cache;
}
static inline void hz3_large_aligned_obs_on_admit_reuse_skip(void) {}
static inline void hz3_large_aligned_obs_on_admit_final_fit_fail(void) {}
static inline void hz3_large_aligned_obs_on_admit_cache_insert(void) {}
static inline void hz3_large_aligned_obs_on_admit_do_munmap(void) {}
static inline void hz3_large_aligned_obs_on_admit_defer_munmap(void) {}
#endif

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
        hz3_s267_on_defer_push(victim->map_size, g_hz3_large_unmap_defer_bytes);
        queued = 1;
    } else {
        hz3_s267_on_defer_reject(victim->map_size, g_hz3_large_unmap_defer_bytes);
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
            hz3_s267_on_defer_pop(victim->map_size, g_hz3_large_unmap_defer_bytes);
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
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    hz3_s276_direct_clear_retained(victim);
#endif
    hz3_large_s240_on_munmap_store(victim);
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
static inline int hz3_large_size_bits(size_t size) {
    uint64_t v = (uint64_t)(size - 1u);
#if defined(__GNUC__) || defined(__clang__)
    // Windows is LLP64: `long` is 32-bit even on x64, so use clzll here.
    return 64 - __builtin_clzll((unsigned long long)v);
#else
    int bits = 0;
    while (v != 0) {
        bits++;
        v >>= 1;
    }
    return bits;
#endif
}

// S50: size → class index
// 32KB〜64MB を 8 分割/倍（97 classes）
static inline int hz3_large_sc(size_t size) {
    if (size <= HZ3_LARGE_SC_MIN) return 0;
    if (size > HZ3_LARGE_SC_MAX) return HZ3_LARGE_SC_COUNT - 1;
    // ceil(log2) で bits を取得
    int bits = hz3_large_size_bits(size);
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

static inline int hz3_large_s268_cap_sc(int sc) {
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
    return (sc >= HZ3_S268_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline uint32_t hz3_large_s268_max_nodes_for_sc(int sc) {
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
    if (hz3_large_s268_cap_sc(sc)) {
        return (uint32_t)HZ3_S268_MAX_NODES_PER_SC;
    }
#else
    (void)sc;
#endif
    return UINT32_MAX;
}

static inline uint32_t hz3_large_s268_clamp_floor_nodes(int sc, uint32_t floor_nodes) {
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
    uint32_t cap_nodes = hz3_large_s268_max_nodes_for_sc(sc);
    if (cap_nodes != UINT32_MAX && floor_nodes > cap_nodes) {
#if HZ3_LARGE_CACHE_STATS
        hz3_large_stat_inc(&g_s268_floor_clamps);
#endif
        return cap_nodes;
    }
#else
    (void)sc;
#endif
    return floor_nodes;
}

static inline int hz3_large_s268_admit_under_cap_locked(int sc) {
#if HZ3_S268_LARGE_SCACHE_RSS_CAP
    uint32_t cap_nodes = hz3_large_s268_max_nodes_for_sc(sc);
    if (cap_nodes == UINT32_MAX) {
        return 1;
    }
    hz3_large_sc_lock_acquire(sc);
    uint32_t nodes = g_sc_nodes[sc];
    hz3_large_sc_lock_release(sc);
    if (nodes >= cap_nodes) {
#if HZ3_LARGE_CACHE_STATS
        hz3_large_stat_inc(&g_s268_admit_cap_skips);
#endif
        return 0;
    }
#else
    (void)sc;
#endif
    return 1;
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
    if (!hz3_large_s268_admit_under_cap_locked(sc)) {
        return 0;
    }
    uint32_t floor_nodes = hz3_large_reuse_floor_nodes_for_sc(sc);
    floor_nodes = hz3_large_s268_clamp_floor_nodes(sc, floor_nodes);
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
