// hz4_mid_tls_cache.inc - Mid TLS cache boundary (included from hz4_mid.c)
#if HZ4_MID_TLS_CACHE
static inline int hz4_mid_tls_sc_enabled(uint16_t sc) {
    return sc <= (uint16_t)HZ4_MID_TLS_CACHE_SC_MAX;
}

static inline int hz4_mid_tls_find_page_idx(uint16_t sc, const hz4_mid_page_t* page) {
    uint8_t n = g_mid_tls_n[sc];
    for (uint8_t i = 0; i < n; i++) {
        if (g_mid_tls_pages[sc][i] == page) {
            return (int)i;
        }
    }
    return -1;
}

static inline void hz4_mid_tls_remove_idx(uint16_t sc, uint8_t idx) {
    uint8_t n = g_mid_tls_n[sc];
    if (idx >= n) {
        return;
    }
    n--;
    g_mid_tls_pages[sc][idx] = g_mid_tls_pages[sc][n];
    g_mid_tls_pages[sc][n] = NULL;
    g_mid_tls_n[sc] = n;
}

static inline void* hz4_mid_tls_pop_obj(uint16_t sc) {
    uint8_t n = g_mid_tls_n[sc];
    while (n != 0) {
        uint8_t idx = (uint8_t)(n - 1u);
        hz4_mid_page_t* page = g_mid_tls_pages[sc][idx];
        if (!page || !HZ4_MID_PAGE_HAS_FREE(page)) {
            g_mid_tls_pages[sc][idx] = NULL;
            n = idx;
            g_mid_tls_n[sc] = n;
            continue;
        }
        void* obj = hz4_mid_page_pop(page);
        if (obj) {
            return obj;
        }
        g_mid_tls_pages[sc][idx] = NULL;
        n = idx;
        g_mid_tls_n[sc] = n;
    }
    return NULL;
}

static inline void hz4_mid_tls_insert_page_locked(uint16_t sc, hz4_mid_page_t* page, hz4_mid_page_t* prev) {
    if (!page || !HZ4_MID_PAGE_HAS_FREE(page)) {
        return;
    }
    if (hz4_mid_tls_find_page_idx(sc, page) >= 0) {
        return;
    }

    hz4_mid_bin_remove_locked(sc, page, prev);

    uint8_t n = g_mid_tls_n[sc];
    if (n < HZ4_MID_TLS_CACHE_DEPTH) {
        g_mid_tls_pages[sc][n] = page;
        g_mid_tls_n[sc] = (uint8_t)(n + 1u);
        return;
    }

    hz4_mid_page_t* evict = g_mid_tls_pages[sc][0];
#if HZ4_MID_TLS_CACHE_DEPTH > 1
    for (uint8_t i = 1; i < (uint8_t)HZ4_MID_TLS_CACHE_DEPTH; i++) {
        g_mid_tls_pages[sc][(uint8_t)(i - 1u)] = g_mid_tls_pages[sc][i];
    }
#endif
    g_mid_tls_pages[sc][(uint8_t)(HZ4_MID_TLS_CACHE_DEPTH - 1u)] = page;
    g_mid_tls_n[sc] = (uint8_t)HZ4_MID_TLS_CACHE_DEPTH;

    if (evict) {
        hz4_mid_bin_prepend_unique_locked(sc, evict);
    }
}
#endif
