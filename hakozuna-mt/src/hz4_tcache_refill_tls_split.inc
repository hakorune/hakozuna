// hz4_tcache_refill_tls_split.inc - TCache refill (!TLS_MERGE) slice
#ifndef HZ4_TCACHE_REFILL_TLS_SPLIT_INC
#define HZ4_TCACHE_REFILL_TLS_SPLIT_INC

#if !HZ4_TLS_MERGE
#if !HZ4_ST_SMALL_REFILL_DIRECT
static void* hz4_refill(hz4_tls_t* tls, hz4_alloc_tls_t* atls, uint8_t sc) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.refill_calls++;
#else
    hz4_os_stats_refill_call();
#endif
#endif
    hz4_tcache_bin_t* bin = &atls->bins[sc];

#if HZ4_INBOX_SPLICE && HZ4_REMOTE_INBOX && !HZ4_COLLECT_LIST
    // P3.4: splice fast path (1回走査) - P4.1 では使用しない
    void* obj = hz4_refill_from_inbox_splice(tls, bin, sc);
    if (obj) return obj;
#endif

#if HZ4_XFER_CACHE
    // Phase 20: Try TransferCache before collect (NEW)
    if (hz4_xfer_pop(tls, sc, bin)) {
    return hz4_tcache_pop(bin);
}
#endif  // !HZ4_ST_SMALL_REFILL_DIRECT

#if HZ4_COLLECT_LIST
    // P4.1: list mode (out[] 配列を排除)
    void* head = NULL;
    void* tail = NULL;
    hz4_remote_drain_demand_note_underflow(tls, sc);
    uint32_t got = hz4_collect_list(tls, sc, &head, &tail);
    if (got) {
        hz4_tcache_splice(bin, head, tail, got);
#if HZ4_TRIM_LITE
        hz4_trim_lite_post_collect(tls, sc, bin, got);
#else
#if HZ4_XFER_CACHE
#if HZ4_TCACHE_COUNT
        // Phase 20: Trim if bin exceeded threshold (NEW)
        hz4_xfer_trim(tls, sc, bin);
#endif
#endif
#endif
        return hz4_tcache_pop(bin);
    }
#else
    void* out[HZ4_COLLECT_OBJ_BUDGET_MAX];
    hz4_remote_drain_demand_note_underflow(tls, sc);
    uint32_t got = hz4_collect_default(tls, sc, out);
    if (got) {
        for (uint32_t i = 0; i < got; i++) {
            hz4_tcache_push(bin, out[i]);
        }
#if HZ4_TRIM_LITE
        hz4_trim_lite_post_collect(tls, sc, bin, got);
#else
#if HZ4_XFER_CACHE
#if HZ4_TCACHE_COUNT
        // Phase 20: Trim if bin exceeded threshold (NEW)
        hz4_xfer_trim(tls, sc, bin);
#endif
#endif
#if HZ4_TRANSFERCACHE
#if HZ4_TCACHE_COUNT
        // Phase 20B: Trim if bin exceeded threshold
        hz4_tcbox_trim(tls, sc, bin);
#endif
#endif
#endif
        return hz4_tcache_pop(bin);
    }
#endif

    size_t obj_size = hz4_sc_to_size(sc);

#if HZ4_CENTRAL_PAGEHEAP
    // Phase 17: CPH pop (highest priority for reuse)
#if HZ4_CPH_2TIER
    hz4_page_meta_t* meta = hz4_cph_hot_pop_meta(sc);
    if (!meta) {
        meta = hz4_cph_cold_pop_meta(sc);
    }
#else
    hz4_page_meta_t* meta = hz4_cph_pop_empty_meta(sc, tls->tid);
#endif
    if (meta) {
#if HZ4_CPH_2TIER
        uint8_t state = atomic_load_explicit(&meta->cph_state, memory_order_acquire);
        if (state != HZ4_CPH_HOT && state != HZ4_CPH_COLD) {
#if HZ4_FAILFAST
            fprintf(stderr,
                    "[HZ4_CPH_POP_STATE_INVALID] want_sc=%u meta=%p state=%u decommitted=%u used=%u\n",
                    (unsigned)sc, (void*)meta, (unsigned)state,
                    (unsigned)meta->decommitted, (unsigned)meta->used_count);
            HZ4_FAIL("CPH pop: invalid state");
#endif
            meta = NULL;
        }
#else
        uint8_t cph_state = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
        if (cph_state != HZ4_CPH_INFLIGHT) {
#if HZ4_FAILFAST
            fprintf(stderr,
                    "[HZ4_CPH_POP_STATE_INVALID] want_sc=%u meta=%p cph=%u decommitted=%u used=%u\n",
                    (unsigned)sc, (void*)meta, (unsigned)cph_state,
                    (unsigned)meta->decommitted, (unsigned)meta->used_count);
            HZ4_FAIL("CPH pop: meta not inflight");
#endif
            meta = NULL;
        }
#endif
    }
    if (meta) {
        if (((!meta->decommitted) && !HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT) ||
            meta->used_count != 0 || meta->sc != sc) {
#if HZ4_FAILFAST
            fprintf(stderr, "[HZ4_CPH_POP_INVALID] want_sc=%u meta=%p meta_sc=%u used=%u decommitted=%u cph_queued=%u\n",
                    (unsigned)sc, (void*)meta, (unsigned)meta->sc,
                    (unsigned)meta->used_count, (unsigned)meta->decommitted,
                    (unsigned)atomic_load_explicit(&meta->cph_queued, memory_order_relaxed));
            HZ4_FAIL("CPH pop: invalid meta state");
#endif
            meta = NULL;
        }
    }
    if (meta) {
        // metaからpageを計算
        hz4_seg_t* seg = hz4_seg_from_page_meta(meta);
        uint32_t page_idx = hz4_page_idx_from_meta(meta);
        hz4_page_t* page = (hz4_page_t*)((char*)seg + (page_idx << HZ4_PAGE_SHIFT));
        // Adopt boundary: reset cross-thread state and take ownership
        meta->owner_tid = tls->tid;
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
        atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_relaxed);
        meta->seal_epoch = 0;
#endif
        for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
            atomic_store_explicit(&meta->remote_head[s], NULL, memory_order_relaxed);
        }
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
#if HZ4_DECOMMIT_DELAY_QUEUE
        meta->queued_decommit = 0;
        meta->empty_deadline_tick = 0;
        meta->dqnext = NULL;
#endif
        atomic_store_explicit(&meta->cph_queued, HZ4_CPH_NONE, memory_order_release);
#if HZ4_CPH_2TIER
        if (!meta->decommitted) {
            meta->decommitted = 1;
        }
#endif
        // rebuildを使う（安全）
#if HZ4_REBUILD_COLD
        if (meta->decommitted) {
            hz4_page_rebuild_decommitted_cold(tls, bin, page, meta, sc, NULL);
        }
#else
        hz4_page_rebuild_decommitted_inline(tls, bin, page, meta, sc, NULL);
#endif
        return hz4_tcache_pop(bin);
    }
#endif

#if HZ4_SEG_ACQ_GATEBOX
    // Note: GateBox not implemented for non-TLS_MERGE path
    // (would need atls->bins[sc] and atls-based epoch tracking)
#endif

#if HZ4_POPULATE_BATCH
#if HZ4_POPULATE_NO_NEXT
    {
        void* obj = hz4_bump_pop_one(tls, bin, sc, obj_size);
        if (obj) return obj;
    }
#else
    if (hz4_bump_populate(tls, bin, sc, obj_size)) {
        return hz4_tcache_pop(bin);
    }
#endif
#endif

    hz4_page_t* page = hz4_alloc_page(tls, atls, sc);
    if (!page) abort();
    hz4_populate_page(page, bin, obj_size);
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
    void* obj = hz4_bump_pop_one(tls, bin, sc, obj_size);
    if (obj) return obj;
#endif

    return hz4_tcache_pop(bin);
}
#endif
#endif

#endif // HZ4_TCACHE_REFILL_TLS_SPLIT_INC
