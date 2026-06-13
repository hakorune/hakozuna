// ============================================================================
// TLS Destructor
// ============================================================================

static void hz3_tcache_destructor(void* arg) {
    if (!arg) return;
    if (!t_hz3_cache.initialized) return;

#if HZ3_S65_EXITING_INBOX_TO_CENTRAL && HZ3_S65_EXITING_MARK_AT_DTOR_START
    hz3_shard_exiting_store(t_hz3_cache.my_shard, 1);
#endif

#if HZ3_S204_LARSON_DIAG
    t_s204_in_dtor = true;
#endif

#if HZ3_S12_V2_STATS && !HZ3_SHIM_FORWARD_ONLY
    // S12-3C triage: Aggregate thread stats before cleanup
    hz3_s12_v2_stats_aggregate_thread();
#endif
    // Day 6: Force epoch cleanup first
    hz3_epoch_force();

#if HZ3_S203_COUNTERS
    // S203: Flush TLS stats to global aggregators
    hz3_s203_flush_tls();
#endif
#if HZ3_S220_CPU_RRQ && HZ3_S220_CPU_RRQ_STATS
    hz3_s220_rrq_flush_tls();
#endif
#if HZ3_S240_LARGE_FRONT_CACHE
    hz3_large_s240_tls_flush();
#endif

// Remote path detection for S65/S62 guard
#ifndef HZ3_S42_SMALL_XFER_DISABLE
#define HZ3_S42_SMALL_XFER_DISABLE 0
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif
#ifndef HZ3_REMOTE_ENABLED
#define HZ3_REMOTE_ENABLED \
    ((HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE) || \
     (HZ3_S44_OWNER_STASH && !HZ3_S44_OWNER_STASH_DISABLE))
#endif

// S65 Remote Guard: Skip S65 ledger flush when remote paths are active
#ifndef HZ3_S65_REMOTE_GUARD
#define HZ3_S65_REMOTE_GUARD 0
#endif
#if !(HZ3_S65_REMOTE_GUARD && HZ3_REMOTE_ENABLED)
#if HZ3_S65_RELEASE_LEDGER
    // S65: Flush TLS ledger (drain all pending entries)
    hz3_s65_ledger_flush_all();
#endif
#endif  // S65_REMOTE_GUARD

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24-1: Full flush on thread exit (event-only)
    hz3_dstbin_flush_remote_all();
#endif

    // S121-D: Flush page packets before flushing other remote state
    hz3_s121d_packet_flush_all_extern();

    // S121-M: Flush pending pageq batch before thread exit
    hz3_s121m_flush_pending_extern();

    // 1. Flush all outboxes (redundant after epoch, but safe)
    for (int owner = 0; owner < HZ3_NUM_SHARDS; owner++) {
        for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
            hz3_outbox_flush((uint8_t)owner, sc);
        }
    }

    // 2. Return bins to central (batch)
    for (int sc = 0; sc < HZ3_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        uint32_t bin_idx = hz3_bin_index_medium(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            // Build list: find tail
            void* tail = head;
            uint32_t n = 1;
            void* obj = hz3_obj_get_next(head);
            while (obj) {
                tail = obj;
                n++;
                obj = hz3_obj_get_next(obj);
            }
            hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            hz3_local_head_clear(bin_idx);
        }
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        if (!hz3_bin_is_empty(bin)) {
            // Build list: find tail
            void* head = hz3_bin_take_all(bin);
            void* tail = head;
            uint32_t n = 1;
            void* obj = hz3_obj_get_next(head);
            while (obj) {
                tail = obj;
                n++;
                obj = hz3_obj_get_next(obj);
            }
            hz3_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
        }
#endif
    }

    // 3. Return small bins to central
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // S40: SoA - directly access local_head for small bins
        uint32_t bin_idx = hz3_bin_index_small(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
#if HZ3_SMALL_V2_ENABLE
            hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
#endif
            hz3_local_head_clear(bin_idx);
        }
#else  // !HZ3_TCACHE_SOA_LOCAL
#if HZ3_SMALL_V2_ENABLE
        hz3_small_v2_bin_flush(sc, hz3_tcache_get_small_bin(sc));
#elif HZ3_SMALL_V1_ENABLE
        hz3_small_bin_flush(sc, hz3_tcache_get_small_bin(sc));
#endif
#endif  // HZ3_TCACHE_SOA_LOCAL
    }

#if HZ3_S145_CENTRAL_LOCAL_CACHE
    // S145-A: Flush TLS central cache back to central pool
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        if (t_hz3_cache.s145_cache_count[sc] > 0) {
            void* head = t_hz3_cache.s145_cache_head[sc];
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            t_hz3_cache.s145_cache_head[sc] = NULL;
            t_hz3_cache.s145_cache_count[sc] = 0;
        }
    }
#endif  // HZ3_S145_CENTRAL_LOCAL_CACHE

#if HZ3_SUB4K_ENABLE
    // 4. Return sub4k bins to central
    for (int sc = 0; sc < HZ3_SUB4K_NUM_SC; sc++) {
#if HZ3_TCACHE_SOA_LOCAL
        // S40: SoA - directly access local_head for sub4k bins
        uint32_t bin_idx = hz3_bin_index_sub4k(sc);
        void* head = hz3_local_head_get_ptr(bin_idx);
        if (head) {
            void* tail = head;
            uint32_t n = 1;
            void* cur = hz3_obj_get_next(head);
            while (cur) {
                tail = cur;
                n++;
                cur = hz3_obj_get_next(cur);
            }
            hz3_sub4k_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
            hz3_local_head_clear(bin_idx);
        }
#else
        hz3_sub4k_bin_flush(sc, hz3_tcache_get_sub4k_bin(sc));
#endif
    }
#endif

#if HZ3_S65_STATS || HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL
    hz3_s65_dtor_inbox_probe((uint8_t)t_hz3_cache.my_shard);
#endif
#if HZ3_S65_DTOR_RECLAIM_AFTER_INBOX_DRAIN && HZ3_S65_MEDIUM_RECLAIM
    hz3_s65_medium_reclaim_tick();
#endif
#if HZ3_S65_EXITING_INBOX_TO_CENTRAL && !HZ3_S65_EXITING_MARK_AT_DTOR_START
    hz3_shard_exiting_store(t_hz3_cache.my_shard, 1);
#endif

    // =========================================================================
    // S62 Remote Guard: Skip S62 when remote paths (xfer/stash) are enabled
    // =========================================================================
    // Compile-time check: if remote paths are active, S62 may race with
    // objects still in transit, causing memory corruption.
    // Note: HZ3_REMOTE_ENABLED defined earlier in this file (before S65 guard)
    //
    // S62-1G: SingleThreadRetireGate overrides REMOTE_GUARD when total_live==1.
    // This allows RSS reduction in safe single-thread scenarios while maintaining
    // safety in multi-thread environments.

    int s62_allowed = 1;

#if HZ3_S62_REMOTE_GUARD && HZ3_REMOTE_ENABLED
    s62_allowed = 0;  // Remote guard active: block S62 by default
#endif

#if HZ3_S62_SINGLE_THREAD_GATE
    int single = hz3_s62_single_thread_ok();  // total_live <= 1
    if (!s62_allowed && single) {
        s62_allowed = 1;  // single-threadなら remote guard を解除
    }
#if HZ3_S62_SINGLE_THREAD_FAILFAST
    if (!single) {
        uint32_t total = 0;
        for (uint32_t i = 0; i < HZ3_NUM_SHARDS; i++) {
            total += hz3_shard_live_count((uint8_t)i);
        }
        fprintf(stderr, "[HZ3_S62_SINGLE_THREAD_FAILFAST] Multi-thread detected (total=%u)\n", total);
        abort();  // multi-thread検出時のみ failfast
    }
#endif
#endif

    if (s62_allowed) {
#if HZ3_S62_RETIRE && !HZ3_S62_RETIRE_DISABLE
        // S62-1: PageRetireBox - retire fully-empty small pages to free_bits
        hz3_s62_retire();
#endif

#if HZ3_S62_SUB4K_RETIRE && !HZ3_S62_SUB4K_RETIRE_DISABLE
        // S62-1b: Sub4kRunRetireBox - retire fully-empty sub4k 2-page runs to free_bits
        hz3_s62_sub4k_retire();
#endif

#if HZ3_S62_PURGE && !HZ3_S62_PURGE_DISABLE
        // S62-2: DtorSmallPagePurgeBox - purge retired small pages
        hz3_s62_purge();
#endif
    }

#if HZ3_S62_OBSERVE && !HZ3_S62_OBSERVE_DISABLE
    // S62-0: OBSERVE - count free_bits purge potential (stats only, no madvise)
    hz3_s62_observe();
#endif

#if HZ3_S61_DTOR_HARD_PURGE
    // S61: Hard purge of central pool (thread-exit, cold path only)
    hz3_s61_dtor_hard_purge();
#endif

    // Note: segment cleanup is deferred to process exit

#if HZ3_LANE_SPLIT
    if (t_hz3_cache.lane) {
        Hz3ShardCore* core = &g_hz3_shards[t_hz3_cache.my_shard];
        if (t_hz3_cache.lane != &core->lane0) {
            hz3_lane_free(t_hz3_cache.lane);
        }
        t_hz3_cache.lane = NULL;
    }
#endif

#if (HZ3_S65_STATS || HZ3_S65_DTOR_DRAIN_INBOX_TO_CENTRAL) && \
    HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS > 0
    for (int round = 0; round < HZ3_S65_DTOR_FINAL_INBOX_DRAIN_ROUNDS; round++) {
        hz3_s65_dtor_inbox_probe((uint8_t)t_hz3_cache.my_shard);
#if HZ3_S65_DTOR_FINAL_RECLAIM_AFTER_DRAIN && HZ3_S65_MEDIUM_RECLAIM
        hz3_s65_medium_reclaim_tick();
#endif
    }
#endif

    // Shard collision tracking: decrement only after flushing all TLS state.
    // If we decrement earlier, S65 reclaim in another thread could release a run
    // while this thread still holds stale pointers in bins.
    {
        uint8_t shard = t_hz3_cache.my_shard;
        if (shard < HZ3_NUM_SHARDS) {
            (void)atomic_fetch_sub_explicit(&g_shard_live_count[shard], 1, memory_order_acq_rel);
        }
    }
}

static void hz3_tcache_create_key(void) {
    pthread_key_create(&g_hz3_tcache_key, hz3_tcache_destructor);
}
