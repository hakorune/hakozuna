// Original path: page.remote_head + segq/pageq
void hz4_remote_flush(hz4_tls_t* tls) {
    if (tls->rlen == 0) {
        return;
    }

#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.remote_flush_calls++;
    hz4_stats_tls_remote_flush_rlen(tls, tls->rlen);
#else
    hz4_os_stats_remote_flush_call();
    hz4_os_stats_remote_flush_rlen(tls->rlen);
#endif
#endif
    tls->flush_count++;

    hz4_remote_flush_retry_bumpmeta(tls);
    if (tls->rlen == 0) {
        return;
    }

    // ---- n==1 fast path ----
    if (tls->rlen == 1) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        tls->stats_tls.remote_flush_n1++;
#else
        hz4_os_stats_remote_flush_fastpath_n1();
#endif
#endif
        hz4_page_t* page = tls->rbuf[0].page;
        void* obj = tls->rbuf[0].obj;
        HZ4_REMOTE_FLUSH_CLEAR_NEXT(obj);  // 安全のため NULL 終端

        hz4_page_push_remote_one_tid(page, obj, tls->tid);

        // Notify pending queue
#if HZ4_PAGEQ_ENABLE
        hz4_pageq_notify(page);
#else
        hz4_seg_t* seg = hz4_seg_from_page(page);
        hz4_segq_notify(seg, hz4_page_idx(page));
#endif

        tls->rlen = 0;
        return;
    }

    // ---- n<=4 fast path (no bucketing) ----
    if (tls->rlen <= HZ4_REMOTE_FLUSH_FASTPATH_MAX) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        tls->stats_tls.remote_flush_le4++;
#else
        hz4_os_stats_remote_flush_fastpath_le4();
#endif
#endif
        for (uint32_t i = 0; i < tls->rlen; i++) {
            hz4_page_t* page = tls->rbuf[i].page;
            void* obj = tls->rbuf[i].obj;
            HZ4_REMOTE_FLUSH_CLEAR_NEXT(obj);

            hz4_page_push_remote_one_tid(page, obj, tls->tid);

#if HZ4_PAGEQ_ENABLE
            hz4_pageq_notify(page);
#else
            hz4_seg_t* seg = hz4_seg_from_page(page);
            hz4_segq_notify(seg, hz4_page_idx(page));
#endif
        }
        tls->rlen = 0;
        return;
    }

    // ---- General path: bucket sort by page ----
    hz4_flush_bucket_t buckets[HZ4_REMOTE_FLUSH_BUCKETS];
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        buckets[i].page = NULL;
        buckets[i].head = NULL;
        buckets[i].tail = NULL;
        buckets[i].n = 0;
    }

    // Phase 1: Distribute to buckets (n 本固定ループ)
    for (uint32_t i = 0; i < tls->rlen; i++) {
        hz4_page_t* page = tls->rbuf[i].page;
        void* obj = tls->rbuf[i].obj;
        HZ4_REMOTE_FLUSH_CLEAR_NEXT(obj);  // 安全のため NULL 終端

        uint32_t idx = (uint32_t)(((uintptr_t)page >> HZ4_PAGE_SHIFT) & (HZ4_REMOTE_FLUSH_BUCKETS - 1));
        hz4_flush_bucket_t* b = &buckets[idx];

        if (b->page == NULL) {
            // New bucket
            b->page = page;
            b->head = obj;
            b->tail = obj;
            b->n = 1;
        } else if (b->page == page) {
            // Same page: link to tail
            hz4_obj_set_next(b->tail, obj);
            b->tail = obj;
            b->n++;
        } else {
            // Collision: push single object immediately
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
            tls->stats_tls.remote_flush_probe_ovf++;
#else
            hz4_os_stats_remote_flush_probe_overflow();
#endif
#endif
            hz4_page_push_remote_one_tid(page, obj, tls->tid);
#if HZ4_PAGEQ_ENABLE
            hz4_pageq_notify(page);
#else
            hz4_seg_t* seg = hz4_seg_from_page(page);
            hz4_segq_notify(seg, hz4_page_idx(page));
#endif
        }
    }

    // Phase 2: Flush buckets to pages
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        hz4_flush_bucket_t* b = &buckets[i];
        if (b->page == NULL || b->n == 0) {
            continue;
        }

        // Push list to page (CAS with tail.next re-sync)
        hz4_page_push_remote_list_tid(b->page, b->head, b->tail, b->n, tls->tid);

        // Notify pending queue
#if HZ4_PAGEQ_ENABLE
        hz4_pageq_notify(b->page);
#else
        hz4_seg_t* seg = hz4_seg_from_page(b->page);
        hz4_segq_notify(seg, hz4_page_idx(b->page));
#endif
    }

    tls->rlen = 0;
}
