// hz4_tcache_refill_helpers.inc - TCache populate/rebuild/refill helper slice
#ifndef HZ4_TCACHE_REFILL_HELPERS_INC
#define HZ4_TCACHE_REFILL_HELPERS_INC

static void hz4_populate_page(hz4_page_t* page, hz4_tcache_bin_t* bin, size_t obj_size) {
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t end = (uintptr_t)page + HZ4_PAGE_SIZE;

#if HZ4_POPULATE_BATCH
    // Lazy populate: only push a batch, track remaining in meta
    uint32_t count = (uint32_t)((end - start) / obj_size);
    hz4_page_meta_t* meta = hz4_page_meta(page);
    meta->capacity = (uint16_t)count;

#if HZ4_POPULATE_NO_NEXT
    // N5: PopulateNoNextWriteBox (opt-in)
    // Do not touch objects (avoid obj->next stores / page-faults).
    meta->bump_off = 0;
    meta->bump_left = (uint16_t)count;
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
    bin->bump_page = page;
#else
    uint32_t batch = HZ4_POPULATE_BATCH;
    if (batch == 0 || batch >= count) {
        batch = count;
    }

    uintptr_t p = start;
    for (uint32_t i = 0; i < batch; i++, p += obj_size) {
        hz4_tcache_push(bin, (void*)p);
    }

    meta->bump_off = (uint16_t)batch;
    meta->bump_left = (uint16_t)(count - batch);

    if (meta->bump_left > 0) {
        bin->bump_page = page;
    } else {
        bin->bump_page = NULL;
    }
#endif
#else
    // Original: push one-by-one (full populate)
    uint32_t count = 0;
    for (uintptr_t p = start; p + obj_size <= end; p += obj_size) {
        void* obj = (void*)p;
        hz4_tcache_push(bin, obj);
        count++;
    }

#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
    meta->capacity = count;
#else
    page->capacity = count;
#endif
#endif
}

// DecommitReusePoolBox archived (NO-GO). See hakozuna/archive/research/reuse_pool_box/.

#if HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
// Rebuild free list after decommit (owner only)
static inline void hz4_page_rebuild_decommitted_inline(hz4_tls_t* tls,
                                                       hz4_tcache_bin_t* bin,
                                                       hz4_page_t* page,
                                                       hz4_page_meta_t* meta,
                                                       uint8_t sc,
                                                       void* keep_obj) {
    if (!meta->decommitted) {
        return;
    }

#if HZ4_CENTRAL_PAGEHEAP
    uint8_t cph_state = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
    if (cph_state == HZ4_CPH_QUEUED) {
        (void)hz4_cph_remove_meta(meta);
        cph_state = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
    }
#endif

#if HZ4_FAILFAST
    hz4_seg_t* seg_dbg = hz4_seg_from_page_meta(meta);
    uint32_t page_idx_dbg = hz4_page_idx_from_meta(meta);
    if (!seg_dbg || seg_dbg->magic != HZ4_SEG_MAGIC ||
        page_idx_dbg >= HZ4_PAGES_PER_SEG) {
        fprintf(stderr, "[HZ4_CPH_BAD_PAGE] meta=%p seg=%p seg_magic=0x%x page_idx=%u page=%p page_magic=0x%x sc=%u decommitted=%u\n",
                (void*)meta, (void*)seg_dbg, seg_dbg ? seg_dbg->magic : 0u,
                page_idx_dbg, (void*)page, page ? page->magic : 0u,
                (unsigned)sc, (unsigned)meta->decommitted);
        HZ4_FAIL("rebuild: invalid page meta");
    }
    if (meta->sc != sc) {
        HZ4_FAIL("rebuild: sc mismatch");
    }
#if HZ4_CENTRAL_PAGEHEAP
    if (cph_state != HZ4_CPH_INFLIGHT && cph_state != HZ4_CPH_NONE) {
        fprintf(stderr,
                "[HZ4_CPH_REBUILD_STATE] meta=%p page=%p sc=%u cph=%u decommitted=%u used=%u\n",
                (void*)meta, (void*)page, (unsigned)sc, (unsigned)cph_state,
                (unsigned)meta->decommitted, (unsigned)meta->used_count);
        HZ4_FAIL("rebuild: unexpected central pageheap state");
    }
#endif
#endif
    (void)tls;

    hz4_os_page_commit(page);
    page->free = NULL;
    page->local_free = NULL;
    page->magic = HZ4_PAGE_MAGIC;

    // Rebuild tcache from page contents (skip keep_obj)
    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t end = (uintptr_t)page + HZ4_PAGE_SIZE;

    uint32_t count = 0;
    for (uintptr_t p = start; p + obj_size <= end; p += obj_size) {
        void* obj = (void*)p;
        if (obj == keep_obj) {
            continue;
        }
        hz4_tcache_push(bin, obj);
        count++;
    }

    page->free = NULL;
    page->local_free = NULL;
    page->magic = HZ4_PAGE_MAGIC;

    if (meta->capacity == 0) {
        meta->capacity = (uint16_t)(count + 1);
    }
#if HZ4_POPULATE_BATCH
    meta->bump_off = 0;
    meta->bump_left = 0;
    bin->bump_page = NULL;
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
#if HZ4_CENTRAL_PAGEHEAP
    atomic_store_explicit(&meta->cph_queued, 0, memory_order_release);
#if HZ4_CPH_2TIER
    atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_relaxed);
    meta->seal_epoch = 0;
#endif
#endif
}

#if HZ4_REBUILD_COLD
__attribute__((noinline, cold))
static void hz4_page_rebuild_decommitted_cold(hz4_tls_t* tls,
                                              hz4_tcache_bin_t* bin,
                                              hz4_page_t* page,
                                              hz4_page_meta_t* meta,
                                              uint8_t sc,
                                              void* keep_obj) {
    hz4_page_rebuild_decommitted_inline(tls, bin, page, meta, sc, keep_obj);
}
#endif
#endif

#if HZ4_POPULATE_BATCH
// ============================================================================
// Lazy populate refill: use remaining objects in bump_page before seg_acq
// ============================================================================
#if HZ4_POPULATE_NO_NEXT
// Stage5-N7: BumpFreeMetaBox (owner-thread only)
// Record freed indices in page meta to avoid obj->next stores (page-faults) on free.
#if HZ4_BUMP_FREE_META
static inline bool hz4_bump_free_meta_try_push(hz4_tls_t* tls,
                                               hz4_tcache_bin_t* bin,
                                               hz4_page_t* page,
                                               hz4_page_meta_t* meta,
                                               uint8_t sc,
                                               void* ptr) {
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    // When remote pressure is high, prefer "touchful" freelist (tcache_push) to
    // avoid shifting page-faults into remote free path.
    if (tls->rbuf_gate_on) {
#if !HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE
        return false;
#endif
    }
#endif
    if (meta->bump_free_n >= HZ4_BUMP_FREE_META_CAP) {
        return false;
    }
    if (meta->capacity == 0) {
        return false;
    }
#if !HZ4_LOCAL_FREE_META
    if ((meta->bump_off | meta->bump_left) == 0) {
        return false;
    }
#endif

    size_t obj_size = hz4_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t p = (uintptr_t)ptr;
    if (p < start || p >= (uintptr_t)page + HZ4_PAGE_SIZE) {
        return false;
    }
    uintptr_t off = p - start;
    if ((off % obj_size) != 0) {
        return false;
    }

    uint32_t idx32 = (uint32_t)(off / obj_size);
    if (idx32 >= meta->capacity) {
        return false;
    }
#if HZ4_FAILFAST
#if HZ4_LOCAL_FREE_META
    {
        uint16_t limit = meta->bump_off;
        if (limit == 0) {
            limit = meta->capacity;
        }
        if (idx32 >= limit) {
            HZ4_FAIL("bump_free_meta_try_push: idx not yet allocated");
        }
    }
#else
    if (idx32 >= meta->bump_off) {
        HZ4_FAIL("bump_free_meta_try_push: idx not yet allocated");
    }
#endif
#endif

    meta->bump_free_idx[meta->bump_free_n++] = (uint16_t)idx32;

    // Ensure alloc path sees this page as a bump source (minimal v0).
    if (!bin->bump_page) {
        bin->bump_page = page;
    }
    return true;
}
#endif

static inline void* hz4_bump_pop_one(hz4_tls_t* tls,
                                     hz4_tcache_bin_t* bin,
                                     uint8_t sc,
                                     size_t obj_size) {
    hz4_page_t* page = bin->bump_page;
    if (!page) return NULL;

    hz4_page_meta_t* meta = hz4_page_meta(page);
    if (meta->owner_tid != tls->tid || meta->sc != sc || meta->decommitted) {
        bin->bump_page = NULL;
        return NULL;
    }
#if HZ4_CENTRAL_PAGEHEAP
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != HZ4_CPH_NONE) {
        bin->bump_page = NULL;
        return NULL;
    }
#if HZ4_CPH_2TIER
    if (atomic_load_explicit(&meta->cph_state, memory_order_relaxed) != HZ4_CPH_ACTIVE) {
        bin->bump_page = NULL;
        return NULL;
    }
#endif
#endif

    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);

#if HZ4_BUMP_FREE_META
    if (meta->bump_free_n > 0) {
        uint16_t idx = meta->bump_free_idx[--meta->bump_free_n];
#if HZ4_FAILFAST
        if (idx >= meta->capacity) {
            HZ4_FAIL("hz4_bump_pop_one: bump_free idx out of range");
        }
#endif
        uintptr_t p = start + ((uintptr_t)idx * obj_size);
        if (meta->bump_left == 0 && meta->bump_free_n == 0) {
            bin->bump_page = NULL;
        }
        return (void*)p;
    }
#endif

    if (meta->bump_left == 0) {
        bin->bump_page = NULL;
        return NULL;
    }

    uintptr_t p = start + ((uintptr_t)meta->bump_off * obj_size);

    meta->bump_off = (uint16_t)(meta->bump_off + 1);
    meta->bump_left = (uint16_t)(meta->bump_left - 1);
    if (meta->bump_left == 0) {
#if HZ4_BUMP_FREE_META
        if (meta->bump_free_n == 0) {
            bin->bump_page = NULL;
        }
#else
        bin->bump_page = NULL;
#endif
    }
    return (void*)p;
}
#endif

static inline bool hz4_bump_populate(hz4_tls_t* tls,
                                     hz4_tcache_bin_t* bin,
                                     uint8_t sc,
                                     size_t obj_size) {
    hz4_page_t* page = bin->bump_page;
    if (!page) return false;

    hz4_page_meta_t* meta = hz4_page_meta(page);
    if (meta->owner_tid != tls->tid || meta->sc != sc || meta->decommitted) {
        bin->bump_page = NULL;
        return false;
    }
#if HZ4_CENTRAL_PAGEHEAP
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != HZ4_CPH_NONE) {
        bin->bump_page = NULL;
        return false;
    }
#if HZ4_CPH_2TIER
    if (atomic_load_explicit(&meta->cph_state, memory_order_relaxed) != HZ4_CPH_ACTIVE) {
        bin->bump_page = NULL;
        return false;
    }
#endif
#endif

    if (meta->bump_left == 0) {
        bin->bump_page = NULL;
        return false;
    }

    uint32_t batch = meta->bump_left < HZ4_POPULATE_BATCH
                         ? meta->bump_left
                         : HZ4_POPULATE_BATCH;

    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t p = start + ((uintptr_t)meta->bump_off * obj_size);
    for (uint32_t i = 0; i < batch; i++, p += obj_size) {
        hz4_tcache_push(bin, (void*)p);
    }

    meta->bump_off = (uint16_t)(meta->bump_off + batch);
    meta->bump_left = (uint16_t)(meta->bump_left - batch);
    if (meta->bump_left == 0) {
        bin->bump_page = NULL;
    }
    return true;
}
#endif

// ============================================================================
// P3.4: Inbox Splice Refill (eliminate double traversal)
// ============================================================================
#if HZ4_INBOX_SPLICE && HZ4_REMOTE_INBOX
static inline void* hz4_refill_from_inbox_splice(hz4_tls_t* tls,
                                                 hz4_tcache_bin_t* bin,
                                                 uint8_t sc) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.inbox_consume_calls++;
#else
    hz4_os_stats_inbox_consume_call();
#endif
#endif
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);

    // Try stash first, then inbox
    void* list = tls->inbox_stash[sc];
    if (!list) list = hz4_inbox_pop_all(owner, sc);  // P3.1 で空チェック済み
    if (!list) return NULL;

    // Walk list to find prefix (up to budget)
    void* head = list;
    void* cur = list;
    void* tail = NULL;
    uint32_t n = 0;
    while (cur && n < HZ4_COLLECT_OBJ_BUDGET) {
        tail = cur;
        cur = hz4_obj_get_next(cur);
        n++;
    }

    // Safety check (念のため - budget=0 等の保険)
    if (!tail) return NULL;

    // Store remainder in stash
    tls->inbox_stash[sc] = cur;
#if HZ4_OS_STATS
    if (cur) {
        uint32_t stash_len = 0;
        void* tmp = cur;
        while (tmp) {
            stash_len++;
            tmp = hz4_obj_get_next(tmp);
        }
        hz4_os_stats_inbox_stash_len(stash_len);
    }
#endif

    // Splice prefix to bin
    hz4_obj_set_next(tail, bin->head);
    bin->head = head;
#if HZ4_TCACHE_COUNT
    bin->count += n;
#endif

    return hz4_tcache_pop(bin);
}
#endif

#if HZ4_SEG_ACQ_GATEBOX
// ============================================================================
// Phase 19: SegAcquireGateBox - seg_acq直前に再利用を必ず一度通す
// ============================================================================
static inline bool hz4_seg_acq_gatebox(hz4_tls_t* tls, hz4_tcache_bin_t* bin, uint8_t sc) {
    if (tls->seg_acq_epoch < HZ4_SEG_ACQ_GATE_BUDGET) return false;
    uint32_t now = (uint32_t)tls->collect_count;
    if (now - tls->seg_acq_gate_last < HZ4_SEG_ACQ_GATE_COOLDOWN) return false;
    tls->seg_acq_gate_last = now;

#if HZ4_DECOMMIT_DELAY_QUEUE
    hz4_decommit_queue_maybe_process(tls);
#endif

    for (uint32_t pass = 0; pass < HZ4_SEG_ACQ_GATE_COLLECT_PASSES; pass++) {
        hz4_remote_drain_demand_note_underflow(tls, sc);
#if HZ4_COLLECT_LIST
        void* head = NULL;
        void* tail = NULL;
        uint32_t got = hz4_collect_list(tls, sc, &head, &tail);
        if (got) {
            hz4_tcache_splice(bin, head, tail, got);
            tls->seg_acq_epoch = 0;
            return true;
        }
#else
        void* out[HZ4_COLLECT_OBJ_BUDGET_MAX];
        uint32_t got = hz4_collect_default(tls, sc, out);
        if (got) {
            for (uint32_t i = 0; i < got; i++) {
                hz4_tcache_push(bin, out[i]);
            }
            tls->seg_acq_epoch = 0;
            return true;
        }
#endif
    }
    return false;
}
#endif


#endif // HZ4_TCACHE_REFILL_HELPERS_INC
