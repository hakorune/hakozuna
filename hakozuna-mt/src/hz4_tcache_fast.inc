// hz4_tcache_fast.inc - TCache hot utility/helpers slice
#ifndef HZ4_TCACHE_FAST_INC
#define HZ4_TCACHE_FAST_INC

static inline void hz4_tcache_push(hz4_tcache_bin_t* bin, void* obj) {
#if HZ4_CENTRAL_PAGEHEAP && HZ4_FAILFAST
    hz4_page_t* page = hz4_page_from_ptr(obj);
    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint8_t cph_state = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
    if (cph_state == HZ4_CPH_QUEUED) {
        fprintf(stderr, "[HZ4_CPH_PUSH_TCACHE] obj=%p page=%p sc=%u used=%u decommitted=%u cph=%u\n",
                obj, (void*)page, (unsigned)meta->sc,
                (unsigned)meta->used_count, (unsigned)meta->decommitted,
                (unsigned)cph_state);
        HZ4_FAIL("tcache_push: page is in central pageheap");
    }
#endif
#if HZ4_TCACHE_OBJ_CACHE_ON
    uint8_t n = bin->st_cache_n;
    if (n < HZ4_TCACHE_OBJ_CACHE_SLOTS_ON) {
        bin->st_cache[n] = obj;
        bin->st_cache_n = (uint8_t)(n + 1);
#if HZ4_TCACHE_COUNT
        bin->count++;
#endif
        return;
    }
#endif
    hz4_obj_set_next(obj, bin->head);
    bin->head = obj;
#if HZ4_TCACHE_COUNT
    bin->count++;
#endif
}

static inline void* hz4_tcache_pop(hz4_tcache_bin_t* bin) {
#if HZ4_TCACHE_OBJ_CACHE_ON
    uint8_t n = bin->st_cache_n;
    if (n) {
        n--;
        void* obj = bin->st_cache[n];
        bin->st_cache[n] = NULL;
        bin->st_cache_n = n;
#if HZ4_FAILFAST
        hz4_obj_set_next(obj, NULL);
#endif
#if HZ4_TCACHE_COUNT
#if HZ4_TCACHE_COUNT_FASTDEC
#if HZ4_FAILFAST
        if (bin->count == 0) {
            HZ4_FAIL("tcache_pop: count underflow");
        }
#endif
        bin->count--;
#else
        if (bin->count > 0) {
            bin->count--;
        }
#endif
#endif
        return obj;
    }
#endif
    void* obj = bin->head;
    if (obj) {
        void* next = hz4_obj_get_next(obj);
        bin->head = next;
        if (next) {
#if HZ4_TCACHE_PREFETCH_LOCALITY_EFF
            __builtin_prefetch(next, 0, HZ4_TCACHE_PREFETCH_LOCALITY_EFF);
#endif
        }
#if HZ4_FAILFAST
        hz4_obj_set_next(obj, NULL);  // P3.5: debug only
#endif
#if HZ4_TCACHE_COUNT
#if HZ4_TCACHE_COUNT_FASTDEC
#if HZ4_FAILFAST
        if (bin->count == 0) {
            HZ4_FAIL("tcache_pop: count underflow");
        }
#endif
        bin->count--;
#else
        if (bin->count > 0) {
            bin->count--;
        }
#endif
#endif
    }
    return obj;
}

// P4.1 / Phase 11: Splice a list directly into tcache bin
// Always available (independent of HZ4_COLLECT_LIST)
static inline void hz4_tcache_splice(hz4_tcache_bin_t* bin,
                                     void* head, void* tail, uint32_t n) {
    (void)n;  // suppress unused warning when HZ4_TCACHE_COUNT=0
    if (!head) return;
    hz4_obj_set_next(tail, bin->head);
    bin->head = head;
#if HZ4_TCACHE_COUNT
    bin->count += n;
#endif
}

#if HZ4_PAGE_META_SEPARATE && HZ4_META_PREFAULTBOX
// Stage5-N?: MetaPrefaultBox
// Touch (and thus fault-in) the meta OS-page on the owner-side allocation path.
// Use a semantics-preserving atomic store (idempotent).
static inline void hz4_meta_prefault(hz4_page_meta_t* meta) {
    atomic_store_explicit(&meta->remote_head[0], NULL, memory_order_relaxed);
}
#endif

#if HZ4_TRIM_LITE
// Stage5-2: Trim-lite Box
// Amortize trim/purge fixed cost (xfer/tcbox trim) across collects.
static inline void hz4_trim_lite_post_collect(hz4_tls_t* tls, uint8_t sc,
                                              hz4_tcache_bin_t* bin,
                                              uint32_t got) {
    tls->trim_lite_debt += got;
    if ((tls->collect_count % HZ4_TRIM_LITE_PERIOD) != 0 &&
        tls->trim_lite_debt < HZ4_TRIM_LITE_DEBT_MAX) {
        return;
    }
    tls->trim_lite_debt = 0;

#if HZ4_XFER_CACHE && HZ4_TCACHE_COUNT
    hz4_xfer_trim(tls, sc, bin);
#endif
#if HZ4_TRANSFERCACHE && HZ4_TCACHE_COUNT
    hz4_tcbox_trim(tls, sc, bin);
#endif
}
#endif



#endif // HZ4_TCACHE_FAST_INC
