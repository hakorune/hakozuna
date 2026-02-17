// Extracted from hz4_remote_flush.inc (move-only): remote free entry path.
// ============================================================================
// Remote Staging Box v1 (Safe Option B)
// ============================================================================
#if HZ4_REMOTE_STAGING
static inline void hz4_remote_flush_stash(hz4_tls_t* tls,
                                          hz4_remote_stash_t* st) {
    if (st->count == 0) return;

    // Flush list to (owner, sc) inbox (MPSC) in one publish.
    hz4_inbox_push_list(tls, st->owner, st->sc, st->head, st->tail);

    // Reset stash
    st->count = 0;
    st->head = NULL;
    st->tail = NULL;
}

static inline bool hz4_remote_staging_try_push(hz4_tls_t* tls,
                                               void* obj,
                                               uint8_t owner,
                                               uint8_t sc) {
    // Hash owner into a small number of per-thread stashes.
    // NOTE: This is TLS-local only; no need for page-based sharding here.
    uint32_t idx = (uint32_t)(owner & (HZ4_REMOTE_SHARDS - 1));
    hz4_remote_stash_t* st = &tls->remote_stash[idx];

    // Case 1: Empty stash -> Init
    if (st->count == 0) {
        st->head = obj;
        st->tail = obj;
        st->owner = owner;
        st->sc = sc;
        st->count = 1;
        return true;
    }

    // Case 2: Match (append)
    if (st->owner == owner && st->sc == sc) {
        hz4_obj_set_next(st->tail, obj);
        st->tail = obj;
        st->count++;
        if (st->count < HZ4_REMOTE_STAGING_MAX) {
            return true;
        }
        // Full -> Flush
        hz4_remote_flush_stash(tls, st);
        return true;
    }

    // Case 3: Mismatch -> Flush old, then Init new
    hz4_remote_flush_stash(tls, st);
    
    // Init new
    st->head = obj;
    st->tail = obj;
    st->owner = owner;
    st->sc = sc;
    st->count = 1;
    return true;
}

static inline void hz4_remote_staging_flush_all(hz4_tls_t* tls) {
    for (uint32_t i = 0; i < HZ4_REMOTE_SHARDS; i++) {
        hz4_remote_flush_stash(tls, &tls->remote_stash[i]);
    }
}
#endif

// ============================================================================
// Remote Free Entry Point (hot path)
// ============================================================================
#if HZ4_REMOTE_LITE && HZ4_PAGE_META_SEPARATE
// Remote-lite v1a2:
// - inbox/rbuf を迂回して page remote list + pageq_notify へ直行する（R=90専用）
// - notify は empty->non-empty transition のみ
// - R=50 では退行し得るため、既定では lane 隔離（HZ4_REMOTE_LITE=0）
static inline bool hz4_remote_lite_try_push(hz4_tls_t* tls,
                                            hz4_page_t* page,
                                            hz4_page_meta_t* meta,
                                            void* ptr) {
#if HZ4_CENTRAL_PAGEHEAP
    // PageQ must not track pages managed by CPH. Keep legacy inbox path in that case.
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        return false;
    }
#else
    (void)meta;
#endif

    // v1a2: notify only on empty->non-empty transition
    bool transitioned = hz4_page_push_remote_one_tid_transitioned(page, ptr, tls->tid);
    if (!transitioned) {
        return true;
    }

    // Notify pending queue
#if HZ4_PAGEQ_ENABLE
    hz4_pageq_notify(page);
#else
    hz4_seg_t* seg = hz4_seg_from_page(page);
    hz4_segq_notify(seg, hz4_page_idx(page));
#endif
    return true;
}
#endif

#if HZ4_REMOTE_INBOX && HZ4_REMOTE_FREE_KEYED && HZ4_PAGE_META_SEPARATE
// Keyed variant: caller provides meta/owner/sc to avoid re-fetch
static inline void hz4_remote_free_keyed(hz4_tls_t* tls,
                                         void* ptr,
                                         hz4_page_t* page,
                                         hz4_page_meta_t* meta,
                                         uint8_t owner,
                                         uint8_t sc) {
#if !(HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER)
    (void)meta;
#endif
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
    if (atomic_load_explicit(&meta->cph_state, memory_order_relaxed) != HZ4_CPH_ACTIVE) {
        hz4_page_push_remote_one(page, ptr);
#if HZ4_PAGEQ_ENABLE
        hz4_pageq_notify(page);
#else
        hz4_seg_t* seg = hz4_seg_from_page(page);
        hz4_segq_notify(seg, hz4_page_idx(page));
#endif
        return;
    }
#endif

#if HZ4_REMOTE_PAGE_RBUF
    if (hz4_remote_page_rbuf_try_push(tls, page, meta, ptr, owner, sc)) {
        return;
    }
#endif

#if HZ4_REMOTE_LITE && HZ4_PAGE_META_SEPARATE
    if (hz4_remote_lite_try_push(tls, page, meta, ptr)) {
        return;
    }
#endif

#if HZ4_REMOTE_STAGING
    // Staging Hook (Pre-Inbox)
    if (hz4_remote_staging_try_push(tls, ptr, owner, sc)) {
        if ((++tls->remote_staging_tick & (HZ4_REMOTE_STAGING_PERIOD - 1)) == 0) {
            hz4_remote_staging_flush_all(tls);
        }
        return;
    }
#endif

#if HZ4_REMOTE_DIRECT_INBOX
    // Remote Direct Inbox (low-frequency check version)
    // Check condition only 1/PERIOD times to minimize branch overhead
    if ((tls->flush_count & (HZ4_REMOTE_DIRECT_INBOX_PERIOD - 1)) == 0) {
        if (tls->rlen >= HZ4_REMOTE_DIRECT_THRESHOLD) {
            // Flush rbuf first to prevent overflow
            hz4_remote_flush(tls);
            // Direct push to inbox
            hz4_inbox_push_one(tls, owner, sc, ptr);
            return;
        }
    }
#endif

#if HZ4_RBUF_KEY
    hz4_rbuf_push(tls, page, ptr, owner, sc);
#else
    hz4_rbuf_push(tls, page, ptr);
#endif
}
#endif

static inline void hz4_remote_free(hz4_tls_t* tls, void* ptr) {
    hz4_page_t* page = hz4_page_from_ptr(ptr);

#if HZ4_FAILFAST
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("hz4_remote_free: invalid page");
    }
#endif

#if HZ4_REMOTE_INBOX && HZ4_RBUF_KEY
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta_key = hz4_page_meta(page);
    uint8_t owner = (uint8_t)hz4_owner_shard(meta_key->owner_tid);
    uint8_t sc = (uint8_t)meta_key->sc;
#else
    uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
    uint8_t sc = (uint8_t)page->sc;
#endif
#endif

#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
    hz4_page_meta_t* meta = hz4_page_meta(page);
    if (atomic_load_explicit(&meta->cph_state, memory_order_relaxed) != HZ4_CPH_ACTIVE) {
        hz4_page_push_remote_one(page, ptr);
#if HZ4_PAGEQ_ENABLE
        hz4_pageq_notify(page);
#else
        hz4_seg_t* seg = hz4_seg_from_page(page);
        hz4_segq_notify(seg, hz4_page_idx(page));
#endif
        return;
    }
#endif

#if HZ4_REMOTE_PAGE_RBUF && HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta_rbuf = hz4_page_meta(page);
    uint8_t owner_rbuf = (uint8_t)hz4_owner_shard(meta_rbuf->owner_tid);
    uint8_t sc_rbuf = (uint8_t)meta_rbuf->sc;
    if (hz4_remote_page_rbuf_try_push(tls, page, meta_rbuf, ptr, owner_rbuf, sc_rbuf)) {
        return;
    }
#endif

#if HZ4_REMOTE_LITE && HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta_lite = hz4_page_meta(page);
    if (hz4_remote_lite_try_push(tls, page, meta_lite, ptr)) {
        return;
    }
#endif

#if HZ4_REMOTE_STAGING
    // Re-derive owner/sc if not available?
    // In hz4_remote_free, owner/sc are derived at top if certain flags are set.
    // Assuming HZ4_REMOTE_INBOX && HZ4_RBUF_KEY is ON (default), 'owner' and 'sc' variables exist.
    // If not, we map them here.
    // Safe fallback:
#if !HZ4_RBUF_KEY
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
    uint8_t sc = (uint8_t)meta->sc;
#else
    uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
    uint8_t sc = (uint8_t)page->sc;
#endif
#endif
    if (hz4_remote_staging_try_push(tls, ptr, owner, sc)) {
        if ((++tls->remote_staging_tick & (HZ4_REMOTE_STAGING_PERIOD - 1)) == 0) {
            hz4_remote_staging_flush_all(tls);
        }
        return;
    }
#endif

#if HZ4_REMOTE_INBOX && HZ4_REMOTE_DIRECT_INBOX
    // Remote Direct Inbox (low-frequency check version)
    // Check condition only 1/PERIOD times to minimize branch overhead
    if ((tls->flush_count & (HZ4_REMOTE_DIRECT_INBOX_PERIOD - 1)) == 0) {
        if (tls->rlen >= HZ4_REMOTE_DIRECT_THRESHOLD) {
            // Flush rbuf first to prevent overflow
            hz4_remote_flush(tls);
            // Direct push to inbox
#if HZ4_RBUF_KEY
            hz4_inbox_push_one(tls, owner, sc, ptr);
#else
#if HZ4_PAGE_META_SEPARATE
            hz4_page_meta_t* meta = hz4_page_meta(page);
            uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
            uint8_t sc = (uint8_t)meta->sc;
#else
            uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
            uint8_t sc = (uint8_t)page->sc;
#endif
            hz4_inbox_push_one(tls, owner, sc, ptr);
#endif
            return;
        }
    }
#endif

#if HZ4_REMOTE_INBOX && HZ4_RBUF_KEY
    hz4_rbuf_push(tls, page, ptr, owner, sc);
#else
    hz4_rbuf_push(tls, page, ptr);
#endif
}
