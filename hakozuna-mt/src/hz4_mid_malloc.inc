// hz4_mid_malloc.inc - split from hz4_mid.c
void* hz4_mid_malloc(size_t size) {
    size_t max = HZ4_MID_MAX_SIZE();
    size_t aligned = hz4_align_up(size, HZ4_MID_ALIGN);
    if (aligned > max) {
        HZ4_FAIL("hz4_mid_malloc: size too large");
        abort();
    }

    uint16_t sc = hz4_mid_size_to_sc(aligned);
    if (sc == 0xFFFFu) {
        HZ4_FAIL("hz4_mid_malloc: invalid size class");
        abort();
    }

#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_calls);
    hz4_mid_stats_inc_sc(g_hz4_mid_stats_malloc_sc, sc);
#endif

    void* obj = NULL;

#if HZ4_ST_MID_LOCAL_STACK_BOX && HZ4_TLS_SINGLE
    obj = hz4_mid_st_local_stack_pop(sc);
    if (obj) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_st_local_stack_hit);
#endif
        return obj;
    }
#endif

#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    obj = hz4_mid_owner_local_stack_pop(sc);
    if (obj) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_local_stack_hit);
#endif
        return obj;
    }
#endif

#if HZ4_MID_OBJ_CACHE_EFF
    obj = hz4_mid_obj_cache_pop(sc);
    if (obj) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_obj_cache_hit);
#endif
        HZ4_MID_CLEAR_NEXT(obj);
        return obj;
    }
#endif

#if HZ4_MID_ALLOC_RUN_CACHE_BOX
    obj = hz4_mid_alloc_run_pop(sc);
    if (obj) {
#if HZ4_MID_LOCK_TIME_STATS
        hz4_mid_lock_stats_note_prelock_alloc_run_hit();
#endif
        return obj;
    }
#if HZ4_MID_LOCK_TIME_STATS
    hz4_mid_lock_stats_note_prelock_alloc_run_miss();
#endif
#endif

#if HZ4_MID_FREE_BATCH_BOX && HZ4_MID_FREE_BATCH_CONSUME_BOX && HZ4_MID_ALLOC_RUN_CACHE_BOX
    if (g_mid_free_batch_n[sc] >= HZ4_MID_FREE_BATCH_CONSUME_MIN) {
        uint16_t refill_objs = hz4_mid_alloc_run_refill_from_free_batch(sc);
#if HZ4_MID_LOCK_TIME_STATS
        if (refill_objs != 0) {
            hz4_mid_lock_stats_note_prelock_free_batch_refill((uint64_t)refill_objs);
        }
#endif
        if (refill_objs != 0) {
            obj = hz4_mid_alloc_run_pop(sc);
            if (obj) {
#if HZ4_MID_STATS_B1
                hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_free_batch_hit);
#endif
                return obj;
            }
        }
    }
#endif

#if HZ4_MID_TLS_CACHE
    // Fast path: Try TLS cache
    if (hz4_mid_tls_sc_enabled(sc)) {
        obj = hz4_mid_tls_pop_obj(sc);
    }
    if (obj) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_tls_hit);
#endif
        return obj;
    }
#endif

#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    hz4_mid_owner_tls_register_once();
    hz4_mid_page_t* opage = g_mid_owner_page[sc];
    if (opage && !hz4_mid_owner_is_self_page(opage)) {
        g_mid_owner_page[sc] = NULL;
        g_mid_owner_drain_tick[sc] = 0;
        opage = NULL;
    }
    if (opage) {
        obj = hz4_mid_page_pop(opage);
        if (obj) {
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_fast);
#endif
            return obj;
        }
        if (atomic_load_explicit(&opage->remote_head, memory_order_acquire) != NULL ||
            hz4_mid_owner_should_drain(sc, opage)) {
            uint32_t drained = hz4_mid_owner_drain_remote_to_local(opage);
#if HZ4_MID_STATS_B1
            if (drained != 0) {
                hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_drain);
                hz4_mid_stats_add(&g_hz4_mid_stats_malloc_owner_drain_objs, drained);
            }
#else
            (void)drained;
#endif
            obj = hz4_mid_page_pop(opage);
            if (obj) {
#if HZ4_MID_STATS_B1
                hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_owner_fast);
#endif
                return obj;
            }
        }
    }
#endif

#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_lock_path);
#endif
#if HZ4_MID_LOCK_TIME_STATS
    hz4_mid_lock_stats_ctx_t lock_stats;
    hz4_mid_lock_stats_enter(&lock_stats, sc);
#endif
    hz4_mid_sc_lock_acquire(sc);
#if HZ4_MID_LOCK_TIME_STATS
    hz4_mid_lock_stats_on_acquired(&lock_stats);
#endif
#if HZ4_MID_FREE_BATCH_BOX
    hz4_mid_free_batch_flush_sc_locked(sc);
#endif

    hz4_mid_page_t* prev = NULL;
    hz4_mid_page_t* page = NULL;
    uint32_t bin_steps = 0;

#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
    // B37: Try cached hint first (only if hint exists)
    int hint_probed = (g_mid_prefetched_bin_head[sc] != NULL);
    if (hint_probed) {
        page = hz4_mid_bin_try_prefetched_hint_locked(sc, &prev);
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_prefetched_hint_probe);
        if (page) {
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_prefetched_hint_hit);
        } else {
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_prefetched_hint_miss);
        }
#endif
    }
    // Fall back to full bin scan if hint not probed or missed
    if (!page) {
        page = hz4_mid_bin_find_usable_locked(sc, &prev, &bin_steps);
    }
#else
    page = hz4_mid_bin_find_usable_locked(sc, &prev, &bin_steps);
#endif
#if HZ4_MID_LOCK_TIME_STATS
    hz4_mid_lock_stats_note_binscan(&lock_stats, bin_steps);
#endif

#if HZ4_MID_STATS_B1
    if (page) {
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_bin_hit);
    } else {
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_bin_miss);
    }
#endif

#if HZ4_MID_PAGE_CREATE_SUPPRESS_BOX
    if (!page) {
        uint32_t retry_steps = 0;
        page = hz4_mid_page_create_suppress_retry_locked(sc, &prev, &retry_steps);
#if HZ4_MID_LOCK_TIME_STATS
        hz4_mid_lock_stats_note_binscan(&lock_stats, retry_steps);
#endif
    }
#endif

    if (!page) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_page_create);
#endif
#if HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_page_create_outlock_calls);
#endif
#if HZ4_MID_LOCK_TIME_STATS
        hz4_mid_lock_stats_on_release(&lock_stats);
        hz4_mid_lock_stats_on_reacquire_wait_begin(&lock_stats);
#endif
        hz4_mid_sc_lock_release(sc);
#if HZ4_MID_LOCK_TIME_STATS
        uint64_t outlock_create_t0_ns = lock_stats.sampled ? hz4_mid_lock_stats_now_ns() : 0;
#endif
        hz4_mid_page_t* new_page = hz4_mid_page_create(sc, hz4_mid_sc_to_size(sc));
#if HZ4_MID_STATS_B1 && HZ4_MID_LOCK_TIME_STATS
        if (lock_stats.sampled) {
            hz4_mid_stats_add(&g_hz4_mid_stats_malloc_page_create_outlock_ns_sum,
                              hz4_mid_lock_stats_now_ns() - outlock_create_t0_ns);
        }
#endif
        hz4_mid_sc_lock_acquire(sc);
#if HZ4_MID_LOCK_TIME_STATS
        hz4_mid_lock_stats_on_acquired(&lock_stats);
#endif

        uint32_t rescan_steps = 0;
        page = hz4_mid_bin_find_usable_locked(sc, &prev, &rescan_steps);
#if HZ4_MID_LOCK_TIME_STATS
        hz4_mid_lock_stats_note_binscan(&lock_stats, rescan_steps);
#endif
        if (page) {
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_page_create_outlock_race_hit);
#endif
            hz4_mid_bin_prepend_unique_locked(sc, new_page);
            if (!prev) {
                // We inserted new head while selected page was old head.
                // Keep predecessor consistent for later remove/claim boundaries.
                prev = new_page;
            }
        } else {
            page = new_page;
            hz4_mid_bin_prepend_unique_locked(sc, page);
            prev = NULL;
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_page_create_outlock_created);
#endif
        }
#else
        {
#if HZ4_MID_LOCK_TIME_STATS
            uint64_t create_t0_ns = lock_stats.sampled ? hz4_mid_lock_stats_now_ns() : 0;
            uint64_t create_ns = 0;
#endif
            page = hz4_mid_page_create(sc, hz4_mid_sc_to_size(sc));
            hz4_mid_bin_prepend_unique_locked(sc, page);
            prev = NULL;
#if HZ4_MID_LOCK_TIME_STATS
            if (lock_stats.sampled) {
                create_ns = hz4_mid_lock_stats_now_ns() - create_t0_ns;
            }
            hz4_mid_lock_stats_note_page_create(&lock_stats, create_ns);
#endif
        }
#endif
    }

    int owner_claimed = 0;
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    hz4_mid_owner_tls_register_once();
    hz4_mid_page_t* old_owner = g_mid_owner_page[sc];
    hz4_mid_owner_claim_page_locked(sc, page, prev);
    owner_claimed = 1;
    if (old_owner && old_owner != page && hz4_mid_owner_is_self_page(old_owner)) {
        hz4_mid_owner_release_page_locked(sc, old_owner);
    }
#endif

    obj = hz4_mid_page_pop(page);
#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
    // Update hint if page still has objects and not owner-claimed
    if (!owner_claimed && HZ4_MID_PAGE_HAS_FREE(page)) {
        g_mid_prefetched_bin_head[sc] = page;
    }
#endif
#if HZ4_MID_ALLOC_RUN_CACHE_BOX
    if (!owner_claimed) {
        hz4_mid_alloc_run_refill_from_page_locked(sc, page);
#if HZ4_MID_LOCK_BURST_REFILL_BOX
        if (page->capacity <= (uint32_t)HZ4_MID_LOCK_BURST_MAX_CAPACITY) {
            hz4_mid_alloc_run_refill_burst_locked(sc, page);
        }
#endif
    }
#endif

    if (!owner_claimed) {
        if (hz4_mid_page_freelist_empty(page)) {
            hz4_mid_bin_remove_locked(sc, page, prev);
        }

#if HZ4_MID_TLS_CACHE
        // Cache pages by size class while we already hold the matching sc lock.
        if (hz4_mid_tls_sc_enabled(sc) && HZ4_MID_PAGE_HAS_FREE(page)) {
            hz4_mid_tls_insert_page_locked(sc, page, prev);
        }
#endif
    }

#if HZ4_MID_LOCK_TIME_STATS
    hz4_mid_lock_stats_on_release(&lock_stats);
#endif
    hz4_mid_sc_lock_release(sc);

    if (!obj) {
        HZ4_FAIL("hz4_mid_malloc: empty page");
        abort();
    }

    return obj;
}
