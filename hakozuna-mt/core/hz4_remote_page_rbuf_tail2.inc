// Producer side: push directly to page.remote_head[] + notify (if needed).
static inline bool hz4_remote_page_rbuf_try_push(hz4_tls_t* tls,
                                                 hz4_page_t* page,
                                                 hz4_page_meta_t* meta,
                                                 void* ptr,
                                                 uint8_t owner,
                                                 uint8_t sc) {
#if HZ4_REMOTE_PAGE_RBUF_GATEBOX
    // Gate OFF -> skip RBUF, fall through to next path (inbox/legacy)
    if (!tls->rbuf_gate_on) {
        return false;
    }
#endif

#if HZ4_CENTRAL_PAGEHEAP
    // Disjointness: do not track CPH-managed pages in this queue.
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        return false;
    }
#endif

#if HZ4_REMOTE_BUMP_FREE_META
    hz4_remote_page_rbufq_lazy_fb_on_try(tls);
    if (hz4_remote_bump_free_meta_try_push(page, meta, ptr, owner, sc)) {
        return true;
    }
    hz4_os_stats_rbmf_fallback();
    hz4_remote_page_rbufq_lazy_fb_on_fallback(tls);
#endif
#if HZ4_REMOTE_FREE_META
    if (hz4_remote_free_meta_try_push(page, meta, ptr, owner, sc)) {
        return true;
    }
#endif

#if HZ4_REMOTE_PAGE_STAGING
#if !HZ4_REMOTE_BUMP_FREE_META
    (void)owner;
    (void)sc;
    (void)meta;
#endif

    uint32_t idx = hz4_remote_page_staging_idx(page);
    hz4_remote_page_stash_ent_t* ent = &tls->remote_page_stash[idx];

#if HZ4_REMOTE_PAGE_STAGING_META
    uint16_t obj_idx = 0;
    if (!hz4_remote_page_staging_calc_idx(page, meta, ptr, sc, &obj_idx)) {
        return false;
    }
    if (ent->count == 0) {
        ent->page = page;
        ent->idx[0] = obj_idx;
        ent->count = 1;
        ent->defer_retries = 0;
    } else if (ent->page == page) {
        if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
            hz4_remote_page_staging_flush_ent(tls, ent);
            if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
                return false;
            }
            if (ent->count == 0) {
                ent->page = page;
                ent->idx[0] = obj_idx;
                ent->count = 1;
                ent->defer_retries = 0;
                if ((++tls->remote_page_staging_tick & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) == 0) {
                    hz4_remote_page_staging_flush_all(tls);
                }
                return true;
            }
        }
        ent->idx[ent->count] = obj_idx;
        ent->count++;
        if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
            hz4_remote_page_staging_flush_ent(tls, ent);
        }
    } else {
        // Evict: flush old entry first, then install new.
        hz4_remote_page_staging_flush_ent(tls, ent);
        if (ent->count != 0) {
            // Keep deferred old page entry; fall back current pointer to non-staging path.
            return false;
        }
        ent->page = page;
        ent->idx[0] = obj_idx;
        ent->count = 1;
        ent->defer_retries = 0;
    }

    if ((++tls->remote_page_staging_tick & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) == 0) {
        hz4_remote_page_staging_flush_all(tls);
    }
    return true;
#else
    // Ensure obj is a proper list node.
#if !HZ4_REMOTE_PAGE_STAGING_NO_CLEAR_NEXT
    hz4_obj_set_next(ptr, NULL);
#endif

    if (ent->count == 0) {
        ent->page = page;
        ent->head = ptr;
        ent->tail = ptr;
        ent->count = 1;
    } else if (ent->page == page) {
        hz4_obj_set_next(ent->tail, ptr);
        ent->tail = ptr;
        ent->count++;
        if (ent->count >= HZ4_REMOTE_PAGE_STAGING_MAX) {
            hz4_remote_page_staging_flush_ent(tls, ent);
        }
    } else {
        // Evict: flush old entry first, then install new.
        hz4_remote_page_staging_flush_ent(tls, ent);
        ent->page = page;
        ent->head = ptr;
        ent->tail = ptr;
        ent->count = 1;
    }

    if ((++tls->remote_page_staging_tick & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) == 0) {
        hz4_remote_page_staging_flush_all(tls);
    }
    return true;
#endif
#else
    // NOTE: transitioned は shard 単位。page 既に queued の場合は notify が抑止される。
    bool transitioned = hz4_page_push_remote_one_tid_transitioned(page, ptr, tls->tid);
    if (!transitioned) {
        return true;
    }
    hz4_remote_page_rbufq_notify(owner, sc, page, meta);
    return true;
#endif
}
