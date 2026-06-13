// ============================================================================
// TLS Initialization (slow path)
// ============================================================================

void hz3_tcache_ensure_init_slow(void) {
    if (t_hz3_cache.initialized) {
        return;
    }

    // Day 5: Register pthread_key destructor (once per process)
    pthread_once(&g_hz3_tcache_key_once, hz3_tcache_create_key);
    pthread_setspecific(g_hz3_tcache_key, &t_hz3_cache);

#if HZ3_STATS_DUMP && !HZ3_SHIM_FORWARD_ONLY
    // S15: Register atexit stats dump (once per process)
    pthread_once(&g_stats_dump_atexit_once, hz3_stats_register_atexit);
#endif
#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S12-3C triage: Register atexit stats dump (once per process)
    pthread_once(&g_s12_v2_stats_atexit_once, hz3_s12_v2_stats_register_atexit);
#endif
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S187: Register medium-path triage stats dump (once per process)
    pthread_once(&g_medium_path_stats_atexit_once, hz3_medium_path_stats_register_atexit);
#endif
#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S54: Register atexit scavenge observation dump (once per process)
    pthread_once(&g_scavenge_obs_atexit_once, hz3_seg_scavenge_obs_register_atexit);
#endif
#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S55: Register atexit retention observation dump (once per process)
    pthread_once(&g_retention_obs_atexit_once, hz3_retention_obs_register_atexit);
#endif
#if HZ3_S62_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
    // S62-0: Register atexit observation dump (once per process).
    // Best-effort snapshot will run at exit even if thread destructors don't.
    hz3_s62_observe_register_atexit();
#endif
#if HZ3_S62_ATEXIT_GATE && !HZ3_SHIM_FORWARD_ONLY
    // S62-1A: Register atexit S62 cleanup (once per process).
    // Ensures S62 runs at process exit even if thread destructors don't fire.
    pthread_once(&g_s62_atexit_once, hz3_s62_atexit_register);
#endif
#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S56-2: Register atexit active-set stats dump (once per process)
    hz3_s56_active_set_register_atexit();
#endif
#if HZ3_S220_CPU_RRQ && HZ3_S220_CPU_RRQ_STATS
    // S220: Register RRQ counters dump (once per process)
    hz3_s220_rrq_register_once();
#endif
    // 1. Shard assignment FIRST (before any segment creation)
    //
    // Prefer an exclusive shard (live_count==0) to avoid collisions when threads<=shards.
    // If no free shard exists, we must collide (threads > shards).
    uint32_t start = atomic_fetch_add_explicit(&g_shard_counter, 1, memory_order_relaxed);
    uint8_t shard = 0;
    uint32_t prev_live = 0;
    int claimed_exclusive = 0;

    for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
        shard = (uint8_t)((start + i) % HZ3_NUM_SHARDS);
        uint32_t expected = 0;
        if (atomic_compare_exchange_strong_explicit(&g_shard_live_count[shard], &expected, 1,
                memory_order_acq_rel, memory_order_acquire)) {
            claimed_exclusive = 1;
            prev_live = 0;
            break;
        }
    }

    if (!claimed_exclusive) {
        shard = (uint8_t)(start % HZ3_NUM_SHARDS);
        prev_live = atomic_fetch_add_explicit(&g_shard_live_count[shard], 1, memory_order_acq_rel);
    }

    t_hz3_cache.my_shard = shard;
    if (prev_live == 0) {
        hz3_shard_exiting_store(shard, 0);
    }
#if HZ3_TLS_INIT_LOG || HZ3_TLS_INIT_FAILFAST
    {
        static _Atomic(uint32_t) g_tls_init_count = 0;
        static _Atomic(uintptr_t) g_tls_addr_first = 0;
        uint32_t idx = atomic_fetch_add_explicit(&g_tls_init_count, 1, memory_order_relaxed);
        uintptr_t addr = (uintptr_t)&t_hz3_cache;
        if (idx == 0) {
            atomic_store_explicit(&g_tls_addr_first, addr, memory_order_relaxed);
        }
        uintptr_t first = atomic_load_explicit(&g_tls_addr_first, memory_order_relaxed);
        int same_first = (first == addr);
#if HZ3_TLS_INIT_LOG
        fprintf(stderr,
                "[HZ3_TLS_INIT] tid=%lu idx=%u cache=%p shard=%u prev_live=%u start=%u same_first=%d\n",
                hz3_thread_id(), (unsigned)idx, (void*)addr, (unsigned)shard,
                (unsigned)prev_live, (unsigned)start, same_first);
#endif
#if HZ3_TLS_INIT_FAILFAST
        if (idx > 0 && same_first) {
            fprintf(stderr,
                    "[HZ3_TLS_INIT_FAILFAST] shared_tls cache=%p tid=%lu idx=%u\n",
                    (void*)addr, hz3_thread_id(), (unsigned)idx);
            abort();
        }
#endif
    }
#endif
#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Initialize instance selection fields (use shard_counter as unique seed)
    t_hz3_cache.owner_stash_seed = start;  // unique per thread
    t_hz3_cache.owner_stash_rr = 0;        // round-robin cursor starts at 0
#endif
#if HZ3_S121_F_PAGEQ_SHARD
    // S121-F: Hash thread ID for sub-shard selection (use shard_counter as unique seed)
    t_hz3_cache.my_tid_hash = start;
#endif
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = NULL;
    t_hz3_cache.small_current_seg_base = NULL;

    Hz3ShardCore* core = &g_hz3_shards[shard];
    core->lane0.core = core;
    if (prev_live == 0) {
        t_hz3_cache.lane = &core->lane0;
    } else {
        Hz3Lane* lane = hz3_lane_alloc();
        if (lane) {
            lane->core = core;
            lane->placeholder = 0;
            t_hz3_cache.lane = lane;
        } else {
            t_hz3_cache.lane = &core->lane0;
        }
    }
#endif

    // Precise (concurrent) collision detection:
    // - collision means >1 thread concurrently shares the same shard.
    if (prev_live != 0) {
        atomic_store_explicit(&g_shard_collision_detected, 1, memory_order_release);
#if HZ3_SHARD_COLLISION_FAILFAST
        fprintf(stderr, "[HZ3_SHARD_COLLISION_FAILFAST] shards=%u hit_shard=%u counter=%u\n",
            (unsigned)HZ3_NUM_SHARDS, (unsigned)t_hz3_cache.my_shard,
            (unsigned)atomic_load_explicit(&g_shard_counter, memory_order_relaxed));
        abort();
#elif HZ3_SHARD_COLLISION_SHOT
        if (atomic_exchange_explicit(&g_shard_collision_shot_fired, 1, memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_SHARD_COLLISION] shards=%u (threads>shards) example_shard=%u counter=%u\n",
                (unsigned)HZ3_NUM_SHARDS, (unsigned)t_hz3_cache.my_shard,
                (unsigned)atomic_load_explicit(&g_shard_counter, memory_order_relaxed));
        }
#endif
    }

#if HZ3_OWNER_LEASE_STATS
    hz3_owner_lease_stats_register_atexit();
#endif

    // 2. Initialize central pool (thread-safe via pthread_once)
    hz3_central_init();

    // 3. Initialize inbox pool (thread-safe via pthread_once)
    hz3_inbox_init();
#if HZ3_S203_COUNTERS
    // S203: register one-shot atexit dumps out of hot path.
    S203_ALLOC_REGISTER();
    hz3_s203_remote_register_once();
#endif
    S204_ALLOC_REGISTER();

    // 4. Initialize bins and outbox
    // S40-2: LOCAL and BANK can be independently configured
#if HZ3_TCACHE_SOA_LOCAL
    // S40: SoA layout for local bins
    memset(t_hz3_cache.local_head, 0, sizeof(t_hz3_cache.local_head));
    memset(t_hz3_cache.local_count, 0, sizeof(t_hz3_cache.local_count));
#elif HZ3_LOCAL_BINS_SPLIT
    // S33: Unified local_bins (AoS)
    memset(t_hz3_cache.local_bins, 0, sizeof(t_hz3_cache.local_bins));
#else
    // Legacy: separate bins + small_bins (AoS)
    memset(t_hz3_cache.bins, 0, sizeof(t_hz3_cache.bins));
    memset(t_hz3_cache.small_bins, 0, sizeof(t_hz3_cache.small_bins));
#endif

    // === Bank bins (remote free) ===
#if HZ3_REMOTE_STASH_SPARSE
    // S41: Sparse ring initialization
    memset(&t_hz3_cache.remote_stash, 0, sizeof(t_hz3_cache.remote_stash));
#else
#if HZ3_PTAG_DSTBIN_ENABLE
#if HZ3_TCACHE_SOA_BANK
    // S40: SoA layout for bank bins
    memset(t_hz3_cache.bank_head, 0, sizeof(t_hz3_cache.bank_head));
    memset(t_hz3_cache.bank_count, 0, sizeof(t_hz3_cache.bank_count));
#else
    // AoS layout for bank bins
    memset(t_hz3_cache.bank, 0, sizeof(t_hz3_cache.bank));
#endif
#endif
#endif

#if !HZ3_REMOTE_STASH_SPARSE
    memset(t_hz3_cache.outbox, 0, sizeof(t_hz3_cache.outbox));
#endif

#if HZ3_S67_SPILL_ARRAY2
    // S67-2: Initialize spill_count and spill_overflow
    memset(t_hz3_cache.spill_count, 0, sizeof(t_hz3_cache.spill_count));
    memset(t_hz3_cache.spill_overflow, 0, sizeof(t_hz3_cache.spill_overflow));
#elif HZ3_S67_SPILL_ARRAY
    // S67: Initialize spill_count to 0 (spill_array is lazy, count=0 means empty)
    memset(t_hz3_cache.spill_count, 0, sizeof(t_hz3_cache.spill_count));
#elif HZ3_S48_OWNER_STASH_SPILL
    // S48: Initialize stash_spill to NULL
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        t_hz3_cache.stash_spill[sc] = NULL;
    }
#endif
#if HZ3_S94_SPILL_LITE
    // S94: Initialize spill_count and spill_overflow to zero
    // s94_spill_array is lazy (count=0 means empty, no read)
    memset(t_hz3_cache.s94_spill_count, 0, sizeof(t_hz3_cache.s94_spill_count));
    memset(t_hz3_cache.s94_spill_overflow, 0, sizeof(t_hz3_cache.s94_spill_overflow));
#endif
    // S121-E: owner_pages is now GLOBAL (g_owner_pages), not TLS
    // No TLS initialization needed; g_owner_pages is static zero-initialized
#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_PTAG_DSTBIN_TLS
    t_hz3_cache.arena_base =
        atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    t_hz3_cache.page_tag32 = g_hz3_page_tag32;
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
    t_hz3_cache.remote_dst_cursor = 0;
    t_hz3_cache.remote_bin_cursor = 0;
    t_hz3_cache.remote_hint = 0;
    t_hz3_cache.remote_flush_credit = 0;
#endif

#if HZ3_S44_OWNER_STASH
    // S162: Initialize owner stash once here to avoid hot-path init calls.
    hz3_owner_stash_init();
#endif

    // 5. No current segment yet
    t_hz3_cache.current_seg = NULL;
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = NULL;
#endif
#if HZ3_S56_ACTIVE_SET
    t_hz3_cache.active_seg1 = NULL;
#endif
    t_hz3_cache.small_current_seg = NULL;
#if HZ3_LANE_SPLIT
    t_hz3_cache.small_current_seg_base = NULL;
#endif
#if HZ3_LANE_T16_R90_PAGE_REMOTE
    t_hz3_cache.t16_small_seg_count = 0;
    t_hz3_cache.t16_small_seg_cursor = 0;
    memset(t_hz3_cache.t16_small_seg_ids, 0, sizeof(t_hz3_cache.t16_small_seg_ids));
#endif

    // 6. Day 6: Initialize knobs (thread-safe via pthread_once)
    hz3_knobs_init();

    // 7. Day 6: Copy global knobs to TLS snapshot
    t_hz3_cache.knobs = g_hz3_knobs;
    t_hz3_cache.knobs_ver = atomic_load_explicit(&g_hz3_knobs_ver, memory_order_acquire);

    // 8. Day 6: Initialize stats to zero
    memset(&t_hz3_cache.stats, 0, sizeof(t_hz3_cache.stats));

#if HZ3_S58_TCACHE_DECAY
    // 9. S58: Initialize decay tracking fields
    hz3_s58_init();
#if HZ3_S58_STATS
    hz3_s58_stats_register_atexit();
#endif
#endif

    t_hz3_cache.initialized = 1;
}
