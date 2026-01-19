// hz3_small_v2_refill.inc - Slow path allocation (refill from central/stash/page)
// Part of hz3_small_v2.c (single TU split)

// S202: Eco Mode adaptive batch sizing
// When HZ3_ECO_MODE=1, use dynamic batch size based on allocation rate
#if HZ3_ECO_MODE
#define HZ3_SMALL_V2_REFILL_ARRAY_SIZE  HZ3_ECO_REFILL_BATCH_ARRAY_SIZE
#define HZ3_SMALL_V2_REFILL_REQUEST     hz3_eco_refill_batch()
#else
#define HZ3_SMALL_V2_REFILL_ARRAY_SIZE  HZ3_SMALL_V2_REFILL_BATCH
#define HZ3_SMALL_V2_REFILL_REQUEST     HZ3_SMALL_V2_REFILL_BATCH
#endif

	void* hz3_small_v2_alloc_slow(int sc) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
	    (void)sc;
	    return NULL;
#else
#if HZ3_TCACHE_INIT_ON_MISS
    // S32-1: TLS init at slow path entry (hot path skips init check)
    hz3_tcache_ensure_init_slow();
#endif
#if HZ3_ARENA_PRESSURE_BOX
    hz3_pressure_check_and_flush();
#endif
#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_S87_SMALL_SLOW_REMOTE_FLUSH
    // S87: Budgeted remote flush at small slow-path entry (scale lane).
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
    if (t_hz3_cache.remote_hint) {
        hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
    }
#else
    hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
#endif
#endif
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return NULL;
    }

#if HZ3_TINY_STATS
    pthread_once(&g_tiny_stats_atexit_once, hz3_tiny_stats_register_atexit);
    atomic_fetch_add_explicit(&g_tiny_slow_calls[sc], 1, memory_order_relaxed);
#endif

#if HZ3_TCACHE_SOA_LOCAL
    // S40: SoA - directly access local_head
    uint32_t bin_idx = hz3_bin_index_small(sc);
    Hz3BinRef ref = hz3_tcache_get_local_binref(bin_idx);
#if !HZ3_BIN_SPLIT_COUNT
    void** head_ptr = ref.head;
#endif
#else
    Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
#endif
	    hz3_small_v2_central_init();

	    // S202: Eco Mode - use dynamic batch size (evaluated once per slow-path call)
	    const int refill_batch = HZ3_SMALL_V2_REFILL_REQUEST;
	    void* batch[HZ3_SMALL_V2_REFILL_ARRAY_SIZE];
#if HZ3_S42_SMALL_XFER && !HZ3_S42_SMALL_XFER_DISABLE
	    // S42: Try transfer cache first
	    int got_xfer = hz3_small_xfer_pop_batch(t_hz3_cache.my_shard, sc, batch, refill_batch);
#else
	    int got_xfer = 0;
#endif
	    int got_stash = 0;
	    int got_central = 0;
	    int got = got_xfer;

#if HZ3_S44_OWNER_STASH_POP && !HZ3_S44_OWNER_STASH_DISABLE
	    // S44: Try owner stash (after xfer, before central - mutex avoidance)
	    if (got < refill_batch) {
	        got_stash = hz3_owner_stash_pop_batch(t_hz3_cache.my_shard, sc,
	                                              batch + got, refill_batch - got);
	        got += got_stash;
	    }
#endif

	    // Central fallback (with S145 TLS cache layer if enabled)
	    if (got < refill_batch) {
#if HZ3_S145_CENTRAL_LOCAL_CACHE
	        got_central = hz3_s145_central_cache_pop(t_hz3_cache.my_shard, sc,
	                                                  batch + got, refill_batch - got);
#else
	        got_central = hz3_small_v2_central_pop_batch(t_hz3_cache.my_shard, sc,
	                                                     batch + got, refill_batch - got);
#endif
	        got += got_central;
	    }
    if (got > 0) {
        hz3_s85_small_v2_slow_record(sc, got_xfer, got_stash, got_central, 0, 1);
#if HZ3_S72_BOUNDARY_DEBUG
	        for (int i = 0; i < got; i++) {
	            hz3_small_v2_boundary_check_obj("small_v2:alloc_slow_batch",
	                                            t_hz3_cache.my_shard, sc, batch[i]);
        }
#endif
#if HZ3_TINY_STATS
        atomic_fetch_add_explicit(&g_tiny_central_hit[sc], 1, memory_order_relaxed);
#endif
        HZ3_REFILL_REMAINING(batch, got);
#if HZ3_S62_STALE_FAILFAST
        hz3_s62_stale_check_ptr(batch[0], "small_alloc_central", sc);
#endif
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_inc(batch[0]);
#endif
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("small_v2:alloc_batch", batch[0], sc, t_hz3_cache.my_shard);
#endif
#if HZ3_S134_EPOCH_ON_SMALL_SLOW
        hz3_epoch_maybe();
#endif
        return batch[0];
    }

#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_S88_SMALL_FLUSH_ON_EMPTY
    // S88: Flush remote stash ONLY when we were empty and are about to page-allocate.
    //      This is intended to reduce page alloc frequency with less overhead than S87.
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
    if (t_hz3_cache.remote_hint) {
        hz3_dstbin_flush_remote_budget(HZ3_S88_SMALL_FLUSH_BUDGET);
    }
#else
    hz3_dstbin_flush_remote_budget(HZ3_S88_SMALL_FLUSH_BUDGET);
#endif

    // Retry stash/central once after flush.
    got_stash = 0;
    got_central = 0;
    got = 0;

#if HZ3_S44_OWNER_STASH_POP && !HZ3_S44_OWNER_STASH_DISABLE
    got_stash = hz3_owner_stash_pop_batch(t_hz3_cache.my_shard, sc, batch, refill_batch);
    got = got_stash;
#endif

    if (got < refill_batch) {
#if HZ3_S145_CENTRAL_LOCAL_CACHE
        got_central = hz3_s145_central_cache_pop(t_hz3_cache.my_shard, sc,
                                                  batch + got, refill_batch - got);
#else
        got_central = hz3_small_v2_central_pop_batch(t_hz3_cache.my_shard, sc,
                                                     batch + got, refill_batch - got);
#endif
        got += got_central;
    }

    if (got > 0) {
        hz3_s85_small_v2_slow_record(sc, 0, got_stash, got_central, 0, 1);
        HZ3_REFILL_REMAINING(batch, got);
#if HZ3_S62_STALE_FAILFAST
        hz3_s62_stale_check_ptr(batch[0], "small_alloc_s88_retry", sc);
#endif
#if HZ3_S69_LIVECOUNT
        hz3_s69_live_count_inc(batch[0]);
#endif
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("small_v2:alloc_s88_retry", batch[0], sc, t_hz3_cache.my_shard);
#endif
#if HZ3_S134_EPOCH_ON_SMALL_SLOW
        hz3_epoch_maybe();
#endif
        return batch[0];
    }
#endif

#if HZ3_TINY_STATS
    atomic_fetch_add_explicit(&g_tiny_page_alloc[sc], 1, memory_order_relaxed);
#endif
    void* page = hz3_small_v2_alloc_page(sc);
	    if (!page) {
	        hz3_s85_small_v2_slow_record(sc, got_xfer, got_stash, got_central, 1, 0);
	        return NULL;
	    }
	    hz3_s85_small_v2_slow_record(sc, got_xfer, got_stash, got_central, 1, 1);

#if HZ3_TCACHE_SOA_LOCAL
#if HZ3_BIN_SPLIT_COUNT
    // S122: Split Count version - uses hz3_binref_prepend_list internally
    (void)hz3_small_v2_fill_binref(ref, sc, page);
#else
    uint32_t added = hz3_small_v2_fill_bin_head(head_ptr, sc, page);
#if !HZ3_BIN_LAZY_COUNT
    if (added > 0) {
        t_hz3_cache.local_count[bin_idx] += (Hz3BinCount)added;
    }
#else
    (void)added;
#endif
#endif  // HZ3_BIN_SPLIT_COUNT
    void* obj = hz3_binref_pop(ref);
#if HZ3_S62_STALE_FAILFAST
    hz3_s62_stale_check_ptr(obj, "small_alloc_page", sc);
#endif
#if HZ3_S69_LIVECOUNT
    hz3_s69_live_count_inc(obj);
#endif
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("small_v2:alloc_page", obj, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_S134_EPOCH_ON_SMALL_SLOW
    hz3_epoch_maybe();
#endif
    return obj;
#else
    hz3_small_v2_fill_bin(bin, sc, page);
    void* obj = hz3_small_bin_pop(bin);
#if HZ3_S62_STALE_FAILFAST
    hz3_s62_stale_check_ptr(obj, "small_alloc_page", sc);
#endif
#if HZ3_S69_LIVECOUNT
    hz3_s69_live_count_inc(obj);
#endif
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("small_v2:alloc_page", obj, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_S134_EPOCH_ON_SMALL_SLOW
    hz3_epoch_maybe();
#endif
    return obj;
#endif
#endif
}

#undef HZ3_SMALL_V2_REFILL_ARRAY_SIZE
#undef HZ3_SMALL_V2_REFILL_REQUEST
