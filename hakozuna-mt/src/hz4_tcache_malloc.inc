// hz4_tcache_malloc.inc - TCache refill/small malloc slice
#ifndef HZ4_TCACHE_MALLOC_INC
#define HZ4_TCACHE_MALLOC_INC

#include "hz4_tcache_refill.inc"
HZ4_ALWAYS_INLINE
static void* hz4_small_malloc(size_t size) {
    hz4_small_stats_b19_malloc_calls_inc();
    hz4_tls_t* tls = hz4_tls_get();

    uint8_t sc = hz4_size_to_sc(size);
    if (sc == HZ4_SC_INVALID) {
        HZ4_FAIL("hz4_small_malloc: size out of range");
        abort();
    }

#if HZ4_TLS_MERGE
    hz4_tcache_bin_t* bin = &tls->bins[sc];
    void* obj = hz4_tcache_pop(bin);
    if (obj) {
        hz4_small_stats_b19_malloc_bin_hit_inc();
    } else {
        hz4_small_stats_b19_malloc_bin_miss_inc();
    }
    if (!obj) {
        hz4_small_stats_b19_malloc_refill_calls_inc();
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
        obj = hz4_bump_pop_one(tls, bin, sc, hz4_sc_to_size(sc));
        if (!obj) {
#endif
#if HZ4_ST_SMALL_REFILL_DIRECT
        hz4_st_stats_b1_refill_direct_calls_inc();
        {
            hz4_page_t* page = hz4_alloc_page(tls, sc);
            if (!page) abort();
            hz4_populate_page(page, bin, hz4_sc_to_size(sc));
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
            obj = hz4_bump_pop_one(tls, bin, sc, hz4_sc_to_size(sc));
            if (!obj)
#endif
            obj = hz4_tcache_pop(bin);
        }
#if HZ4_ST_STATS_B1
        if (obj) {
            hz4_st_stats_b1_refill_direct_hit_inc();
        }
#endif
#elif HZ4_ST_REFILL_DIRECT_BOX
        if (sc <= (uint8_t)HZ4_ST_REFILL_DIRECT_SC_MAX) {
            hz4_st_stats_b1_refill_direct_calls_inc();
            {
                hz4_page_t* page = hz4_alloc_page(tls, sc);
                if (!page) abort();
                hz4_populate_page(page, bin, hz4_sc_to_size(sc));
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
                obj = hz4_bump_pop_one(tls, bin, sc, hz4_sc_to_size(sc));
                if (!obj)
#endif
                obj = hz4_tcache_pop(bin);
            }
            if (obj) {
                hz4_st_stats_b1_refill_direct_hit_inc();
            } else {
                hz4_st_stats_b1_refill_to_slow_inc();
                obj = hz4_refill(tls, sc);
            }
        } else {
            hz4_st_stats_b1_refill_to_slow_inc();
            obj = hz4_refill(tls, sc);
        }
#else
        hz4_st_stats_b1_refill_to_slow_inc();
        obj = hz4_refill(tls, sc);
#endif
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
        }
#endif
        if (obj) {
            hz4_small_stats_b19_malloc_refill_hit_inc();
        } else {
            hz4_small_stats_b19_malloc_refill_miss_inc();
        }
    }
#if HZ4_PAGE_META_SEPARATE
    if (obj) {
        hz4_page_t* page = hz4_page_from_ptr(obj);
        hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_PAGE_DECOMMIT
#if HZ4_REBUILD_COLD
        if (meta->decommitted) {
            hz4_page_rebuild_decommitted_cold(tls, bin, page, meta, sc, obj);
        }
#else
        hz4_page_rebuild_decommitted_inline(tls, bin, page, meta, sc, obj);
#endif
#else
        (void)meta;  // suppress unused warning when decommit disabled
#endif
        hz4_page_used_inc(page);
    }
#endif
#else
    hz4_alloc_tls_t* atls = hz4_alloc_tls_get(tls);
    hz4_tcache_bin_t* bin = &atls->bins[sc];
    void* obj = hz4_tcache_pop(bin);
    if (obj) {
        hz4_small_stats_b19_malloc_bin_hit_inc();
    } else {
        hz4_small_stats_b19_malloc_bin_miss_inc();
    }
    if (!obj) {
        hz4_small_stats_b19_malloc_refill_calls_inc();
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
        obj = hz4_bump_pop_one(tls, bin, sc, hz4_sc_to_size(sc));
        if (!obj) {
#endif
#if HZ4_ST_SMALL_REFILL_DIRECT
        hz4_st_stats_b1_refill_direct_calls_inc();
        {
            hz4_page_t* page = hz4_alloc_page(tls, atls, sc);
            if (!page) abort();
            hz4_populate_page(page, bin, hz4_sc_to_size(sc));
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
            obj = hz4_bump_pop_one(tls, bin, sc, hz4_sc_to_size(sc));
            if (!obj)
#endif
            obj = hz4_tcache_pop(bin);
        }
#if HZ4_ST_STATS_B1
        if (obj) {
            hz4_st_stats_b1_refill_direct_hit_inc();
        }
#endif
#elif HZ4_ST_REFILL_DIRECT_BOX
        if (sc <= (uint8_t)HZ4_ST_REFILL_DIRECT_SC_MAX) {
            hz4_st_stats_b1_refill_direct_calls_inc();
            {
                hz4_page_t* page = hz4_alloc_page(tls, atls, sc);
                if (!page) abort();
                hz4_populate_page(page, bin, hz4_sc_to_size(sc));
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
                obj = hz4_bump_pop_one(tls, bin, sc, hz4_sc_to_size(sc));
                if (!obj)
#endif
                obj = hz4_tcache_pop(bin);
            }
            if (obj) {
                hz4_st_stats_b1_refill_direct_hit_inc();
            } else {
                hz4_st_stats_b1_refill_to_slow_inc();
                obj = hz4_refill(tls, atls, sc);
            }
        } else {
            hz4_st_stats_b1_refill_to_slow_inc();
            obj = hz4_refill(tls, atls, sc);
        }
#else
        hz4_st_stats_b1_refill_to_slow_inc();
        obj = hz4_refill(tls, atls, sc);
#endif
#if HZ4_POPULATE_BATCH && HZ4_POPULATE_NO_NEXT
        }
#endif
        if (obj) {
            hz4_small_stats_b19_malloc_refill_hit_inc();
        } else {
            hz4_small_stats_b19_malloc_refill_miss_inc();
        }
    }
#if HZ4_PAGE_META_SEPARATE
    if (obj) {
        hz4_page_t* page = hz4_page_from_ptr(obj);
        hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_PAGE_DECOMMIT
#if HZ4_REBUILD_COLD
        if (meta->decommitted) {
            hz4_page_rebuild_decommitted_cold(tls, bin, page, meta, sc, obj);
        }
#else
        hz4_page_rebuild_decommitted_inline(tls, bin, page, meta, sc, obj);
#endif
#else
        (void)meta;  // suppress unused warning when decommit disabled
#endif
        hz4_page_used_inc(page);
    }
#endif
#endif
    return obj;
}


HZ4_ALWAYS_INLINE_EXTERN
void* hz4_malloc(size_t size) {
    if (size <= HZ4_SIZE_MAX) {
        hz4_small_stats_b19_malloc_route_small_inc();
        return hz4_small_malloc(size);
    }
    if (size <= HZ4_MID_MAX_SIZE()) {
        hz4_small_stats_b19_malloc_route_mid_inc();
        return hz4_mid_malloc(size);
    }
    hz4_small_stats_b19_malloc_route_large_inc();
    return hz4_large_malloc(size);
}

#endif // HZ4_TCACHE_MALLOC_INC
