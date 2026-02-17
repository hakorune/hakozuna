// hz4_tcache_init.inc - TCache init/page-alloc slice
#ifndef HZ4_TCACHE_INIT_INC
#define HZ4_TCACHE_INIT_INC

#if !HZ4_TLS_MERGE
static hz4_alloc_tls_t* hz4_alloc_tls_get(hz4_tls_t* tls) {
    // HZ4_TLS_SINGLE: init check を cold path へ (likely hint)
    if (__builtin_expect(!g_hz4_alloc_tls.inited, 0)) {
        for (uint32_t i = 0; i < HZ4_SC_MAX; i++) {
            g_hz4_alloc_tls.bins[i].head = NULL;
            g_hz4_alloc_tls.bins[i].count = 0;
#if HZ4_POPULATE_BATCH
            g_hz4_alloc_tls.bins[i].bump_page = NULL;
#endif
        }
        g_hz4_alloc_tls.cur_seg = NULL;
        g_hz4_alloc_tls.next_page_idx = HZ4_PAGE_IDX_MIN;
        g_hz4_alloc_tls.owner = (uint8_t)hz4_owner_shard(tls->tid);
        g_hz4_alloc_tls.inited = 1;
    }
    return &g_hz4_alloc_tls;
}
#endif

#if HZ4_TLS_MERGE
static hz4_page_t* hz4_alloc_page(hz4_tls_t* tls, uint8_t sc) {
retry_tls_merge: __attribute__((unused));
    if (!tls->cur_seg || tls->next_page_idx >= HZ4_PAGES_PER_SEG) {
#if HZ4_SEG_ACQ_GUARDBOX
        hz4_seg_acq_guard(tls, sc);
#endif
        tls->cur_seg = hz4_seg_create(tls->owner);
        if (!tls->cur_seg) {
            abort();
        }
        tls->next_page_idx = HZ4_PAGE_IDX_MIN;
    }

    hz4_page_t* page = hz4_page_from_seg(tls->cur_seg, tls->next_page_idx++);

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX
    // B76: Lite initialization path with optional sampling verification
    hz4_small_stats_b19_small_alloc_page_calls_inc();

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT > 0
    {
        static __thread uint32_t tls_alloc_page_verify_tick;
        uint32_t tick = ++tls_alloc_page_verify_tick;

        if ((tick & ((1u << HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT) - 1u)) == 0u) {
            // Sampling verification: check if remote fields are clean
            bool need_full = false;
#if HZ4_PAGE_META_SEPARATE
            hz4_page_meta_t* vmeta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
            if (atomic_load_explicit(&vmeta->cph_queued, memory_order_acquire) != 0) {
                goto retry_tls_merge;
            }
#endif
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&vmeta->remote_head[s], memory_order_relaxed) != NULL) {
                    need_full = true;
                    break;
                }
            }
#if HZ4_REMOTE_MASK
            if (!need_full && atomic_load_explicit(&vmeta->remote_mask, memory_order_relaxed) != 0) {
                need_full = true;
            }
#endif
            if (!need_full && atomic_load_explicit(&vmeta->queued, memory_order_relaxed) != 0) {
                need_full = true;
            }
#else
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&page->remote_head[s], memory_order_relaxed) != NULL) {
                    need_full = true;
                    break;
                }
            }
#if HZ4_REMOTE_MASK
            if (!need_full && atomic_load_explicit(&page->remote_mask, memory_order_relaxed) != 0) {
                need_full = true;
            }
#endif
            if (!need_full && atomic_load_explicit(&page->queued, memory_order_relaxed) != 0) {
                need_full = true;
            }
#endif
            if (need_full) {
                hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail_inc();
                hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full_inc();
                goto full_init_tls_merge;
            }
        }
    }
#endif // HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT > 0

    // Lite initialization: essential fields only
    page->free = NULL;
    page->local_free = NULL;
    page->magic = HZ4_PAGE_MAGIC;
#if HZ4_PAGE_META_SEPARATE
    {
        hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
            goto retry_tls_merge;
        }
#endif
        meta->owner_tid = tls->tid;
        meta->sc = sc;
        meta->used_count = 0;
        meta->capacity = 0;
        meta->decommitted = 0;
        meta->initialized = 1;
        page->_reserved = 0;
    }
#else
    page->owner_tid = tls->tid;
    page->sc = sc;
    page->used_count = 0;
    page->capacity = 0;
#endif

    hz4_small_stats_b19_small_alloc_page_init_lite_taken_inc();
    goto done_init_tls_merge;

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT > 0
full_init_tls_merge:
#endif

#endif // HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX

    // Full initialization path (existing logic)
    page->free = NULL;
    page->local_free = NULL;
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        goto retry_tls_merge;
    }
#endif
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        atomic_store_explicit(&meta->remote_head[s], NULL, memory_order_relaxed);
    }
#if HZ4_META_PREFAULTBOX
    hz4_meta_prefault(meta);
#endif
#if HZ4_REMOTE_MASK
    atomic_store_explicit(&meta->remote_mask, 0, memory_order_relaxed);
#endif
    atomic_store_explicit(&meta->queued, 0, memory_order_relaxed);
    meta->qnext = NULL;
#if HZ4_REMOTE_PAGE_RBUF
    atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_relaxed);
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
    atomic_store_explicit(&meta->rbufq_lazy_left, 0, memory_order_relaxed);
    atomic_store_explicit(&meta->rbufq_empty_streak, 0, memory_order_relaxed);
#endif
    meta->rbufq_next = NULL;
#endif
    meta->owner_tid = tls->tid;
    meta->sc = sc;
    meta->used_count = 0;
    meta->capacity = 0;
#if HZ4_POPULATE_BATCH
    meta->bump_off = 0;
    meta->bump_left = 0;
#if HZ4_BUMP_FREE_META
    meta->bump_free_n = 0;
#if HZ4_REMOTE_BUMP_FREE_META
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
    for (uint32_t s = 0; s < HZ4_REMOTE_BUMP_FREE_META_SHARDS; s++) {
        atomic_flag_clear_explicit(&meta->bump_rfree_lock[s], memory_order_relaxed);
        atomic_store_explicit(&meta->bump_rfree_n[s], 0, memory_order_relaxed);
    }
#else
    atomic_flag_clear_explicit(&meta->bump_rfree_lock, memory_order_relaxed);
    atomic_store_explicit(&meta->bump_rfree_n, 0, memory_order_relaxed);
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
    atomic_store_explicit(&meta->bump_rfree_pub, 0, memory_order_relaxed);
    atomic_store_explicit(&meta->bump_rfree_draining, 0, memory_order_relaxed);
#endif
#endif
#endif
#endif
#if HZ4_REMOTE_FREE_META
    atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_relaxed);
    atomic_store_explicit(&meta->remote_free_n, 0, memory_order_relaxed);
#endif
#endif
    meta->decommitted = 0;
    meta->initialized = 1;
#if HZ4_DECOMMIT_DELAY_QUEUE
    // Phase 2: ページ再利用時 - queued_decommitをクリア
    // （queue内に古いノードが残っていても、dequeue時にqueued_decommit==0なら捨てる）
    meta->queued_decommit = 0;
    meta->empty_deadline_tick = 0;
    meta->dqnext = NULL;
#endif
#else
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        atomic_store_explicit(&page->remote_head[s], NULL, memory_order_relaxed);
    }
#if HZ4_REMOTE_MASK
    atomic_store_explicit(&page->remote_mask, 0, memory_order_relaxed);
#endif
    atomic_store_explicit(&page->queued, 0, memory_order_relaxed);
    page->qnext = NULL;

    page->owner_tid = tls->tid;
    page->sc = sc;
    page->used_count = 0;
    page->capacity = 0;
#endif

    page->magic = HZ4_PAGE_MAGIC;
#if HZ4_PAGE_META_SEPARATE
    page->_reserved = 0;
#endif

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX
done_init_tls_merge:
#endif

#if HZ4_PAGE_TAG_TABLE
    // Register page tag for fast lookup
    uint32_t page_idx;
    if (hz4_pagetag_page_idx(page, &page_idx)) {
        uint32_t tag = hz4_pagetag_encode(HZ4_TAG_KIND_SMALL, sc, tls->tid);
        hz4_pagetag_store(page_idx, tag);
    }
#endif

    return page;
}
#else
static hz4_page_t* hz4_alloc_page(hz4_tls_t* tls, hz4_alloc_tls_t* atls, uint8_t sc) {
retry_alloc_tls:
    if (!atls->cur_seg || atls->next_page_idx >= HZ4_PAGES_PER_SEG) {
#if HZ4_SEG_ACQ_GUARDBOX
        hz4_seg_acq_guard(tls, sc);
#endif
        atls->cur_seg = hz4_seg_create(atls->owner);
        if (!atls->cur_seg) {
            abort();
        }
        atls->next_page_idx = HZ4_PAGE_IDX_MIN;
    }

    hz4_page_t* page = hz4_page_from_seg(atls->cur_seg, atls->next_page_idx++);

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX
    // B76: Lite initialization path with optional sampling verification
    hz4_small_stats_b19_small_alloc_page_calls_inc();

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT > 0
    {
        static __thread uint32_t tls_alloc_page_verify_tick;
        uint32_t tick = ++tls_alloc_page_verify_tick;

        if ((tick & ((1u << HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT) - 1u)) == 0u) {
            // Sampling verification: check if remote fields are clean
            bool need_full = false;
#if HZ4_PAGE_META_SEPARATE
            hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
            if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
                goto retry_alloc_tls;
            }
#endif
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&meta->remote_head[s], memory_order_relaxed) != NULL) {
                    need_full = true;
                    break;
                }
            }
#if HZ4_REMOTE_MASK
            if (!need_full && atomic_load_explicit(&meta->remote_mask, memory_order_relaxed) != 0) {
                need_full = true;
            }
#endif
            if (!need_full && atomic_load_explicit(&meta->queued, memory_order_relaxed) != 0) {
                need_full = true;
            }
#else
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&page->remote_head[s], memory_order_relaxed) != NULL) {
                    need_full = true;
                    break;
                }
            }
#if HZ4_REMOTE_MASK
            if (!need_full && atomic_load_explicit(&page->remote_mask, memory_order_relaxed) != 0) {
                need_full = true;
            }
#endif
            if (!need_full && atomic_load_explicit(&page->queued, memory_order_relaxed) != 0) {
                need_full = true;
            }
#endif
            if (need_full) {
                hz4_small_stats_b19_small_alloc_page_init_lite_verify_fail_inc();
                hz4_small_stats_b19_small_alloc_page_init_lite_fallback_full_inc();
                goto full_init_alloc_tls;
            }
        }
    }
#endif // HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT > 0

    // Lite initialization: essential fields only
    page->free = NULL;
    page->local_free = NULL;
    page->magic = HZ4_PAGE_MAGIC;
#if HZ4_PAGE_META_SEPARATE
    {
        hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
            goto retry_alloc_tls;
        }
#endif
        meta->owner_tid = tls->tid;
        meta->sc = sc;
        meta->used_count = 0;
        meta->capacity = 0;
        meta->decommitted = 0;
        meta->initialized = 1;
        page->_reserved = 0;
    }
#else
    page->owner_tid = tls->tid;
    page->sc = sc;
    page->used_count = 0;
    page->capacity = 0;
#endif

    hz4_small_stats_b19_small_alloc_page_init_lite_taken_inc();
    goto done_init_alloc_tls;

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT > 0
full_init_alloc_tls:
#endif

#endif // HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX

    // Full initialization path (existing logic)
    page->free = NULL;
    page->local_free = NULL;
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        goto retry_alloc_tls;
    }
#endif
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        atomic_store_explicit(&meta->remote_head[s], NULL, memory_order_relaxed);
    }
#if HZ4_META_PREFAULTBOX
    hz4_meta_prefault(meta);
#endif
#if HZ4_REMOTE_MASK
    atomic_store_explicit(&meta->remote_mask, 0, memory_order_relaxed);
#endif
    atomic_store_explicit(&meta->queued, 0, memory_order_relaxed);
    meta->qnext = NULL;
#if HZ4_REMOTE_PAGE_RBUF
    atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_relaxed);
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
    atomic_store_explicit(&meta->rbufq_lazy_left, 0, memory_order_relaxed);
    atomic_store_explicit(&meta->rbufq_empty_streak, 0, memory_order_relaxed);
#endif
    meta->rbufq_next = NULL;
#endif
    meta->owner_tid = tls->tid;
    meta->sc = sc;
    meta->used_count = 0;
    meta->capacity = 0;
    meta->initialized = 1;
#if HZ4_POPULATE_BATCH
    meta->bump_off = 0;
    meta->bump_left = 0;
#if HZ4_BUMP_FREE_META
    meta->bump_free_n = 0;
#if HZ4_REMOTE_BUMP_FREE_META
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
    for (uint32_t s = 0; s < HZ4_REMOTE_BUMP_FREE_META_SHARDS; s++) {
        atomic_flag_clear_explicit(&meta->bump_rfree_lock[s], memory_order_relaxed);
        atomic_store_explicit(&meta->bump_rfree_n[s], 0, memory_order_relaxed);
    }
#else
    atomic_flag_clear_explicit(&meta->bump_rfree_lock, memory_order_relaxed);
    atomic_store_explicit(&meta->bump_rfree_n, 0, memory_order_relaxed);
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
    atomic_store_explicit(&meta->bump_rfree_pub, 0, memory_order_relaxed);
    atomic_store_explicit(&meta->bump_rfree_draining, 0, memory_order_relaxed);
#endif
#endif
#endif
#endif
#if HZ4_REMOTE_FREE_META
    atomic_flag_clear_explicit(&meta->remote_free_lock, memory_order_relaxed);
    atomic_store_explicit(&meta->remote_free_n, 0, memory_order_relaxed);
#endif
#endif
#if HZ4_DECOMMIT_DELAY_QUEUE
    // Phase 2: ページ再利用時 - queued_decommitをクリア
    meta->queued_decommit = 0;
    meta->empty_deadline_tick = 0;
    meta->dqnext = NULL;
#endif
#if HZ4_CENTRAL_PAGEHEAP
    uint8_t cph_state = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
#if HZ4_FAILFAST
    if (cph_state != HZ4_CPH_NONE) {
        fprintf(stderr,
                "[HZ4_CPH_ALLOC_PAGE_INVALID] page=%p meta=%p sc=%u cph=%u decommitted=%u used=%u\n",
                (void*)page, (void*)meta, (unsigned)sc, (unsigned)cph_state,
                (unsigned)meta->decommitted, (unsigned)meta->used_count);
        HZ4_FAIL("alloc_page: page is in central pageheap");
    }
#endif
    meta->cph_next = NULL;
#endif
#else
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        atomic_store_explicit(&page->remote_head[s], NULL, memory_order_relaxed);
    }
#if HZ4_REMOTE_MASK
    atomic_store_explicit(&page->remote_mask, 0, memory_order_relaxed);
#endif
    atomic_store_explicit(&page->queued, 0, memory_order_relaxed);
    page->qnext = NULL;
    page->owner_tid = tls->tid;
    page->sc = sc;
    page->used_count = 0;
    page->capacity = 0;
#endif

    page->magic = HZ4_PAGE_MAGIC;
#if HZ4_PAGE_META_SEPARATE
    page->_reserved = 0;
#endif

#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX
done_init_alloc_tls:
#endif

#if HZ4_PAGE_TAG_TABLE
    // Register page tag for fast lookup
    uint32_t page_idx;
    if (hz4_pagetag_page_idx(page, &page_idx)) {
        uint32_t tag = hz4_pagetag_encode(HZ4_TAG_KIND_SMALL, sc, tls->tid);
        hz4_pagetag_store(page_idx, tag);
    }
#endif

    return page;
}
#endif


#endif // HZ4_TCACHE_INIT_INC
