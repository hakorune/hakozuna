// hz4_mid_free.inc - split from hz4_mid.c
void hz4_mid_free(void* ptr) {
    if (!ptr) {
        return;
    }

    hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1));
    if (!page) {
        HZ4_FAIL("hz4_mid_free: null page");
        abort();
    }
    if (page->magic != HZ4_MID_MAGIC) {
        HZ4_FAIL("hz4_mid_free: invalid page magic");
        abort();
    }
    uint16_t sc = page->sc;
    if (sc >= (HZ4_PAGE_SIZE / HZ4_MID_ALIGN)) {
        HZ4_FAIL("hz4_mid_free: invalid size class");
        abort();
    }

#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_free_calls);
    hz4_mid_stats_inc_sc(g_hz4_mid_stats_free_sc, sc);
#endif

#if HZ4_ST_MID_LOCAL_STACK_BOX && HZ4_TLS_SINGLE
    if (hz4_mid_st_local_stack_push(sc, ptr)) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_free_st_local_stack_hit);
#endif
        return;
    }
#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_free_st_local_stack_overflow);
#endif
#endif

#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    uintptr_t owner = hz4_mid_owner_tag_load(page);
    if (owner != 0) {
        if (owner == hz4_mid_owner_token()) {
#if HZ4_MID_OWNER_LOCAL_STACK_BOX
            if (hz4_mid_owner_local_stack_push(sc, page, ptr)) {
#if HZ4_MID_STATS_B1
                hz4_mid_stats_inc(&g_hz4_mid_stats_free_owner_local_stack_hit);
#endif
                return;
            }
#if HZ4_MID_STATS_B1
            if (sc < HZ4_MID_OWNER_LOCAL_STACK_SC_MAX) {
                hz4_mid_stats_inc(&g_hz4_mid_stats_free_owner_local_stack_overflow);
            }
#endif
#endif
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_free_owner_local);
#endif
            hz4_mid_page_freelist_push_raw(page, ptr);
            hz4_mid_page_freelist_push_count(page, 1);
            return;
        }
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_free_owner_remote);
#endif
        hz4_mid_owner_remote_push(page, ptr);
        return;
    }
#endif

#if HZ4_MID_OBJ_CACHE_EFF
    if (hz4_mid_obj_cache_push(sc, ptr)) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_free_obj_cache_fast);
#endif
        return;
    }
#endif

#if HZ4_MID_TLS_CACHE
    int tls_idx = hz4_mid_tls_sc_enabled(sc) ? hz4_mid_tls_find_page_idx(sc, page) : -1;
    if (tls_idx >= 0) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_free_tls_fast);
#endif
        hz4_mid_page_freelist_push_raw(page, ptr);
        hz4_mid_page_freelist_push_count(page, 1);
        
        // If full (all objects free), return to global bin?
        // Actually, if "free_count == capacity", the page is completely empty.
        // We might want to return it to global bin to allow segment release/reuse by others?
        // Safe policy: if full, evict.
        if (page->free_count == page->capacity) {
            uint8_t tls_depth = g_mid_tls_n[sc];
            if (tls_depth >= (uint8_t)HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH) {
                hz4_mid_sc_lock_acquire(sc);

                hz4_mid_bin_prepend_unique_locked(sc, page);
                hz4_mid_tls_remove_idx(sc, (uint8_t)tls_idx);

                hz4_mid_sc_lock_release(sc);
            }
        }
        return;
    }
#endif

#if HZ4_MID_FREE_BATCH_BOX
    hz4_mid_free_batch_push(sc, ptr);
    return;
#endif

#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_free_locked);
#endif
    hz4_mid_sc_lock_acquire(sc);

    if (hz4_mid_page_freelist_empty(page)) {
        hz4_mid_bin_prepend_unique_locked(sc, page);
    }

    hz4_mid_page_freelist_push_raw(page, ptr);
    hz4_mid_page_freelist_push_count(page, 1);

    hz4_mid_sc_lock_release(sc);
}
