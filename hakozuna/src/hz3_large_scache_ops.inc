#if HZ3_S184_LARGE_FREE_PRECHECK
static inline size_t hz3_large_total_cached_hint_load(void) {
    return atomic_load_explicit(&g_total_cached_bytes_hint, memory_order_relaxed);
}
#endif

static inline void hz3_large_sc_lock_acquire(int sc) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_lock_acquire(&g_sc_locks[sc]);
#else
    (void)sc;
#endif
}

static inline void hz3_large_sc_lock_release(int sc) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_lock_release(&g_sc_locks[sc]);
#else
    (void)sc;
#endif
}

static inline Hz3LargeHdr* hz3_large_sc_try_pop_locked(int sc, size_t need, int exact_fit) {
    Hz3LargeHdr* hdr = g_sc_head[sc];
    if (!hdr || hdr->magic != HZ3_LARGE_MAGIC) {
        return NULL;
    }
    if (exact_fit ? (hdr->map_size != need) : (hdr->map_size < need)) {
        return NULL;
    }
    g_sc_head[sc] = hdr->next_free;
    g_sc_bytes[sc] -= hdr->map_size;
    g_sc_nodes[sc]--;
    hz3_large_total_cached_sub(hdr->map_size);
    hz3_s267_on_scache_pop(sc, hdr->map_size);
    hz3_s270_on_scache_pop_locked(sc);
    hz3_large_debug_on_cache_remove_locked(hdr);
    return hdr;
}

static inline Hz3LargeHdr* hz3_large_sc_pop_head(int sc) {
    hz3_large_sc_lock_acquire(sc);
    Hz3LargeHdr* hdr = g_sc_head[sc];
    if (hdr) {
        g_sc_head[sc] = hdr->next_free;
        g_sc_bytes[sc] -= hdr->map_size;
        g_sc_nodes[sc]--;
        hz3_large_total_cached_sub(hdr->map_size);
        hz3_s267_on_scache_pop(sc, hdr->map_size);
        hz3_s270_on_scache_pop_locked(sc);
        hz3_large_debug_on_cache_remove_locked(hdr);
    }
    hz3_large_sc_lock_release(sc);
    return hdr;
}

static inline void hz3_large_sc_push_head(int sc, Hz3LargeHdr* hdr) {
    hz3_large_sc_lock_acquire(sc);
    hdr->next_free = g_sc_head[sc];
    g_sc_head[sc] = hdr;
    g_sc_bytes[sc] += hdr->map_size;
    g_sc_nodes[sc]++;
    hz3_large_total_cached_add(hdr->map_size);
    hz3_s267_on_scache_push(sc, hdr->map_size);
    hz3_s270_on_scache_push_locked(sc);
    hz3_large_debug_on_cache_insert_locked(hdr);
    hz3_large_sc_lock_release(sc);
}
