// Extracted from hz4_remote_flush.inc (move-only): flush core path.
#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
static inline void hz4_stats_tls_remote_flush_rlen(hz4_tls_t* tls, uint32_t rlen) {
    if (rlen >= HZ4_REMOTE_FLUSH_THRESHOLD) {
        tls->stats_tls.rf_rlen_ge_th++;
        return;
    }
    if (rlen == 1) {
        tls->stats_tls.rf_rlen_1++;
    } else if (rlen <= 4) {
        tls->stats_tls.rf_rlen_2_4++;
    } else if (rlen <= 8) {
        tls->stats_tls.rf_rlen_5_8++;
    } else if (rlen <= 16) {
        tls->stats_tls.rf_rlen_9_16++;
    } else if (rlen <= 32) {
        tls->stats_tls.rf_rlen_17_32++;
    } else if (rlen <= 64) {
        tls->stats_tls.rf_rlen_33_64++;
    } else {
        tls->stats_tls.rf_rlen_65_127++;
    }
}
#endif

// Stage5-N10a: RemoteFlushNoClearNextBox (opt-in)
#if HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT
#define HZ4_REMOTE_FLUSH_CLEAR_NEXT(obj) do { (void)(obj); } while (0)
#else
#define HZ4_REMOTE_FLUSH_CLEAR_NEXT(obj) hz4_obj_set_next((obj), NULL)
#endif
#if HZ4_REMOTE_INBOX && HZ4_REMOTE_FLUSH_INBOX_NO_CLEAR_NEXT
#define HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj) do { (void)(obj); } while (0)
#else
#define HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj) HZ4_REMOTE_FLUSH_CLEAR_NEXT(obj)
#endif

// ============================================================================
// Remote Free Enqueue (hot path inline)
// ============================================================================
#if HZ4_REMOTE_INBOX && HZ4_RBUF_KEY
static inline void hz4_rbuf_push(hz4_tls_t* tls, hz4_page_t* page, void* obj,
                                 uint8_t owner, uint8_t sc) {
    if (tls->rlen >= HZ4_REMOTE_FLUSH_THRESHOLD) {
        hz4_remote_flush(tls);  // boundary call
    }
    tls->rbuf[tls->rlen].page = page;
    tls->rbuf[tls->rlen].obj = obj;
    tls->rbuf[tls->rlen].owner = owner;
    tls->rbuf[tls->rlen].sc = sc;
    tls->rlen++;
}
#else
static inline void hz4_rbuf_push(hz4_tls_t* tls, hz4_page_t* page, void* obj) {
    if (tls->rlen >= HZ4_REMOTE_FLUSH_THRESHOLD) {
        hz4_remote_flush(tls);  // boundary call
    }
    tls->rbuf[tls->rlen].page = page;
    tls->rbuf[tls->rlen].obj = obj;
    tls->rlen++;
}
#endif

#if HZ4_REMOTE_FLUSH_RETRY_BUMPMETA
// Stage5-N21: retry rbuf entries into remote bump-free-meta at flush boundary.
// Successful entries are removed from rbuf and skip inbox/page-list touch path.
static inline void hz4_remote_flush_retry_bumpmeta(hz4_tls_t* tls) {
#if HZ4_REMOTE_PAGE_RBUF_GATEBOX
    if (!tls->rbuf_gate_on) {
        return;
    }
#endif

    uint16_t out_n = 0;
    uint16_t n = tls->rlen;
    for (uint16_t i = 0; i < n; i++) {
        hz4_page_t* page = tls->rbuf[i].page;
        hz4_page_meta_t* meta = hz4_page_meta(page);

#if HZ4_CENTRAL_PAGEHEAP
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
            if (out_n != i) {
                tls->rbuf[out_n] = tls->rbuf[i];
            }
            out_n++;
            continue;
        }
#endif

#if HZ4_RBUF_KEY
        uint8_t owner = tls->rbuf[i].owner;
        uint8_t sc = tls->rbuf[i].sc;
#else
        uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
        uint8_t sc = (uint8_t)meta->sc;
#endif
        if (hz4_remote_bump_free_meta_try_push(page, meta, tls->rbuf[i].obj, owner, sc)) {
            continue;
        }
        if (out_n != i) {
            tls->rbuf[out_n] = tls->rbuf[i];
        }
        out_n++;
    }
    tls->rlen = out_n;
}
#else
static inline void hz4_remote_flush_retry_bumpmeta(hz4_tls_t* tls) {
    (void)tls;
}
#endif

// ============================================================================
// Remote Flush (boundary API)
// ============================================================================
#if HZ4_REMOTE_INBOX
static inline void hz4_remote_flush_get_owner_sc(
    hz4_tls_t* tls, uint32_t idx, uint8_t* owner, uint8_t* sc) {
#if HZ4_RBUF_KEY
    *owner = tls->rbuf[idx].owner;
    *sc = tls->rbuf[idx].sc;
#else
    hz4_page_t* page = tls->rbuf[idx].page;
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
    *owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
    *sc = (uint8_t)meta->sc;
#else
    *owner = (uint8_t)hz4_owner_shard(page->owner_tid);
    *sc = (uint8_t)page->sc;
#endif
#endif
}

#if HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX
#define HZ4_REMOTE_FLUSH_DIRECT_INDEX_KEYS (HZ4_NUM_SHARDS * HZ4_SC_MAX)
static __thread uint32_t g_hz4_rf_direct_index_epoch;
static __thread uint32_t g_hz4_rf_direct_index_stamp[HZ4_REMOTE_FLUSH_DIRECT_INDEX_KEYS];
static __thread void* g_hz4_rf_direct_index_head[HZ4_REMOTE_FLUSH_DIRECT_INDEX_KEYS];
static __thread void* g_hz4_rf_direct_index_tail[HZ4_REMOTE_FLUSH_DIRECT_INDEX_KEYS];

static inline void hz4_remote_flush_inbox_direct_index(hz4_tls_t* tls) {
    uint32_t n = tls->rlen;
    uint8_t active_owner[HZ4_RBUF_CAP];
    uint8_t active_sc[HZ4_RBUF_CAP];
    uint32_t active_n = 0;

    uint32_t epoch = ++g_hz4_rf_direct_index_epoch;
    if (epoch == 0) {
        for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_DIRECT_INDEX_KEYS; i++) {
            g_hz4_rf_direct_index_stamp[i] = 0;
        }
        epoch = ++g_hz4_rf_direct_index_epoch;
    }

#if HZ4_OS_STATS && !HZ4_OS_STATS_FAST
    hz4_os_stats_remote_flush_direct_index_call();
    hz4_os_stats_remote_flush_direct_index_objs(n);
#endif

    for (uint32_t i = 0; i < n; i++) {
        void* obj = tls->rbuf[i].obj;
        HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj);

        uint8_t owner = 0;
        uint8_t sc = 0;
        hz4_remote_flush_get_owner_sc(tls, i, &owner, &sc);

#if HZ4_FAILFAST
        if ((uint32_t)owner >= HZ4_NUM_SHARDS) {
            HZ4_FAIL("hz4_remote_flush_direct_index: invalid owner");
        }
        if ((uint32_t)sc >= HZ4_SC_MAX) {
            HZ4_FAIL("hz4_remote_flush_direct_index: invalid sc");
        }
#endif

        uint32_t key = ((uint32_t)owner * (uint32_t)HZ4_SC_MAX) + (uint32_t)sc;
        if (g_hz4_rf_direct_index_stamp[key] != epoch) {
            g_hz4_rf_direct_index_stamp[key] = epoch;
            g_hz4_rf_direct_index_head[key] = obj;
            g_hz4_rf_direct_index_tail[key] = obj;
            active_owner[active_n] = owner;
            active_sc[active_n] = sc;
            active_n++;
        } else {
            hz4_obj_set_next(g_hz4_rf_direct_index_tail[key], obj);
            g_hz4_rf_direct_index_tail[key] = obj;
        }
    }

#if HZ4_OS_STATS && !HZ4_OS_STATS_FAST
    hz4_os_stats_remote_flush_direct_index_groups(active_n);
#endif

    for (uint32_t i = 0; i < active_n; i++) {
        uint8_t owner = active_owner[i];
        uint8_t sc = active_sc[i];
        uint32_t key = ((uint32_t)owner * (uint32_t)HZ4_SC_MAX) + (uint32_t)sc;
        void* head = g_hz4_rf_direct_index_head[key];
        void* tail = g_hz4_rf_direct_index_tail[key];
        if (head == tail) {
            hz4_inbox_push_one(tls, owner, sc, head);
        } else {
            hz4_inbox_push_list(tls, owner, sc, head, tail);
        }
    }

    tls->rlen = 0;
}
#endif

static inline void hz4_remote_flush_inbox_bucket_general(hz4_tls_t* tls) {
    // ---- General path: bucket sort by (owner, sc) ----
#if HZ4_REMOTE_FLUSH_TLS_BUCKETS && !HZ4_REMOTE_FLUSH_PAGE_BUCKET
    uint32_t epoch = ++tls->flush_epoch;
    if (epoch == 0) {
        for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
            tls->ibucket_stamp[i] = 0;
        }
        epoch = ++tls->flush_epoch;
    }
    hz4_inbox_bucket_t* ibuckets = tls->ibuckets;
    uint32_t* ibucket_stamp = tls->ibucket_stamp;
#if HZ4_REMOTE_FLUSH_ACTIVE
    uint32_t* active_idx = tls->active_idx;
    uint32_t active_n = 0;
#endif
#else
    hz4_inbox_bucket_t ibuckets[HZ4_REMOTE_FLUSH_BUCKETS];
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        ibuckets[i].owner = 0xFF;  // sentinel
        ibuckets[i].sc = 0;
        ibuckets[i].head = NULL;
        ibuckets[i].tail = NULL;
    }

#if HZ4_REMOTE_FLUSH_ACTIVE
    uint32_t active_idx[HZ4_REMOTE_FLUSH_BUCKETS];
    uint32_t active_n = 0;
#endif
#endif

    // Phase 1: Distribute to buckets (n 本固定ループ)

    // Helper macros for conditional logic
#if HZ4_RBUF_KEY
#define HZ4_GET_OWNER_SC(idx, o, s) do { \
    o = tls->rbuf[idx].owner; \
    s = tls->rbuf[idx].sc; \
} while(0)
#else
#if HZ4_PAGE_META_SEPARATE
#define HZ4_GET_OWNER_SC(idx, o, s) do { \
    hz4_page_t* page = tls->rbuf[idx].page; \
    hz4_page_meta_t* meta = hz4_page_meta(page); \
    o = (uint8_t)hz4_owner_shard(meta->owner_tid); \
    s = (uint8_t)meta->sc; \
} while(0)
#else
#define HZ4_GET_OWNER_SC(idx, o, s) do { \
    hz4_page_t* page = tls->rbuf[idx].page; \
    o = (uint8_t)hz4_owner_shard(page->owner_tid); \
    s = (uint8_t)page->sc; \
} while(0)
#endif
#endif

#if HZ4_REMOTE_FLUSH_ACTIVE
#define HZ4_RECORD_ACTIVE(slot) do { \
    if (active_n < HZ4_REMOTE_FLUSH_BUCKETS) { \
        active_idx[active_n++] = slot; \
    } \
} while(0)
#else
#define HZ4_RECORD_ACTIVE(slot) do {} while(0)
#endif

#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
#define HZ4_RECORD_PROBE_OVF() tls->stats_tls.remote_flush_probe_ovf++
#else
#define HZ4_RECORD_PROBE_OVF() hz4_os_stats_remote_flush_probe_overflow()
#endif
#else
#define HZ4_RECORD_PROBE_OVF() do {} while(0)
#endif

    // Helper macro for distribution logic (to support unrolling)
#if HZ4_REMOTE_FLUSH_TLS_BUCKETS && !HZ4_REMOTE_FLUSH_PAGE_BUCKET
#define HZ4_DISTRIBUTE_ONE(idx) do { \
        void* obj = tls->rbuf[idx].obj; \
        HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj); \
\
        uint8_t owner; \
        uint8_t sc; \
        HZ4_GET_OWNER_SC(idx, owner, sc); \
\
        /* P3.3b: Better hash with multiplicative mix (golden ratio constants) */ \
        uint32_t h = ((uint32_t)owner * 0x9e3779b1u) ^ ((uint32_t)sc * 0x85ebca6bu); \
        uint32_t base = h & (HZ4_REMOTE_FLUSH_BUCKETS - 1); \
        int placed = 0; \
\
        /* P3.3: Linear probe up to PROBE times */ \
        for (uint32_t p = 0; p < HZ4_REMOTE_FLUSH_PROBE; p++) { \
            uint32_t slot = (base + p) & (HZ4_REMOTE_FLUSH_BUCKETS - 1); \
            hz4_inbox_bucket_t* b = &ibuckets[slot]; \
            if (ibucket_stamp[slot] != epoch) { \
                ibucket_stamp[slot] = epoch; \
                b->owner = 0xFF; \
                b->sc = 0; \
                b->head = NULL; \
                b->tail = NULL; \
            } \
\
            if (b->owner == 0xFF) { \
                /* New bucket - place here */ \
                b->owner = owner; \
                b->sc = sc; \
                b->head = obj; \
                b->tail = obj; \
                HZ4_RECORD_ACTIVE(slot); \
                placed = 1; \
                break; \
            } else if (b->owner == owner && b->sc == sc) { \
                /* Same (owner, sc) - append */ \
                hz4_obj_set_next(b->tail, obj); \
                b->tail = obj; \
                placed = 1; \
                break; \
            } \
        } \
\
        /* Probe exhausted: fallback to push_one */ \
        if (!placed) { \
            HZ4_RECORD_PROBE_OVF(); \
            hz4_inbox_push_one(tls, owner, sc, obj); \
        } \
    } while (0)
#else
#define HZ4_DISTRIBUTE_ONE(idx) do { \
        void* obj = tls->rbuf[idx].obj; \
        HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj); \
\
        uint8_t owner; \
        uint8_t sc; \
        HZ4_GET_OWNER_SC(idx, owner, sc); \
\
        /* P3.3b: Better hash with multiplicative mix (golden ratio constants) */ \
        uint32_t h = ((uint32_t)owner * 0x9e3779b1u) ^ ((uint32_t)sc * 0x85ebca6bu); \
        uint32_t base = h & (HZ4_REMOTE_FLUSH_BUCKETS - 1); \
        int placed = 0; \
\
        /* P3.3: Linear probe up to PROBE times */ \
        for (uint32_t p = 0; p < HZ4_REMOTE_FLUSH_PROBE; p++) { \
            uint32_t slot = (base + p) & (HZ4_REMOTE_FLUSH_BUCKETS - 1); \
            hz4_inbox_bucket_t* b = &ibuckets[slot]; \
\
            if (b->owner == 0xFF) { \
                /* New bucket - place here */ \
                b->owner = owner; \
                b->sc = sc; \
                b->head = obj; \
                b->tail = obj; \
                HZ4_RECORD_ACTIVE(slot); \
                placed = 1; \
                break; \
            } else if (b->owner == owner && b->sc == sc) { \
                /* Same (owner, sc) - append */ \
                hz4_obj_set_next(b->tail, obj); \
                b->tail = obj; \
                placed = 1; \
                break; \
            } \
        } \
\
        /* Probe exhausted: fallback to push_one */ \
        if (!placed) { \
            HZ4_RECORD_PROBE_OVF(); \
            hz4_inbox_push_one(tls, owner, sc, obj); \
        } \
    } while (0)
#endif

#if HZ4_REMOTE_FLUSH_UNROLL
    // Unrolled loop (4-way)
    uint32_t i = 0;
    for (; i + 3 < tls->rlen; i += 4) {
        HZ4_DISTRIBUTE_ONE(i);
        HZ4_DISTRIBUTE_ONE(i + 1);
        HZ4_DISTRIBUTE_ONE(i + 2);
        HZ4_DISTRIBUTE_ONE(i + 3);
    }
    // Handle remainders
    for (; i < tls->rlen; i++) {
        HZ4_DISTRIBUTE_ONE(i);
    }
#else
    // Standard loop
    for (uint32_t i = 0; i < tls->rlen; i++) {
        HZ4_DISTRIBUTE_ONE(i);
    }
#endif

#undef HZ4_DISTRIBUTE_ONE
#undef HZ4_GET_OWNER_SC
#undef HZ4_RECORD_ACTIVE
#undef HZ4_RECORD_PROBE_OVF

    // Phase 2: Flush buckets to inbox
#if HZ4_REMOTE_FLUSH_ACTIVE
    if (active_n <= HZ4_REMOTE_FLUSH_BUCKETS) {
        // Scan only active buckets
        for (uint32_t ai = 0; ai < active_n; ai++) {
            uint32_t i = active_idx[ai];
            hz4_inbox_bucket_t* b = &ibuckets[i];
            if (b->head != NULL) {
                hz4_inbox_push_list(tls, b->owner, b->sc, b->head, b->tail);
            }
        }
    } else {
        // Overflow guard: fall back to full scan
        for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
            hz4_inbox_bucket_t* b = &ibuckets[i];
            if (b->owner == 0xFF || b->head == NULL) {
                continue;
            }
            hz4_inbox_push_list(tls, b->owner, b->sc, b->head, b->tail);
        }
    }
#else
    // Full scan (legacy)
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        hz4_inbox_bucket_t* b = &ibuckets[i];
#if HZ4_REMOTE_FLUSH_TLS_BUCKETS && !HZ4_REMOTE_FLUSH_PAGE_BUCKET
        if (ibucket_stamp[i] != epoch) {
            continue;
        }
#endif
        if (b->owner == 0xFF || b->head == NULL) {
            continue;
        }
        hz4_inbox_push_list(tls, b->owner, b->sc, b->head, b->tail);
    }
#endif

    tls->rlen = 0;
}

#if HZ4_REMOTE_FLUSH_COMPACT_BOX
static inline void hz4_remote_flush_inbox_compact(hz4_tls_t* tls) {
    uint32_t n = tls->rlen;
    uint8_t owners[HZ4_REMOTE_FLUSH_COMPACT_MAX];
    uint8_t scs[HZ4_REMOTE_FLUSH_COMPACT_MAX];
    void* heads[HZ4_REMOTE_FLUSH_COMPACT_MAX];
    void* tails[HZ4_REMOTE_FLUSH_COMPACT_MAX];
    uint32_t used = 0;
    uint32_t fallback_objs = 0;

#if HZ4_OS_STATS && !HZ4_OS_STATS_FAST
    hz4_os_stats_remote_flush_compact_call();
    hz4_os_stats_remote_flush_compact_objs(n);
#endif

    for (uint32_t i = 0; i < n; i++) {
        void* obj = tls->rbuf[i].obj;
        HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj);

        uint8_t owner = 0;
        uint8_t sc = 0;
        hz4_remote_flush_get_owner_sc(tls, i, &owner, &sc);

        uint32_t slot = 0;
        for (; slot < used; slot++) {
            if (owners[slot] == owner && scs[slot] == sc) {
                break;
            }
        }

        if (slot == used) {
            if (used >= (uint32_t)HZ4_REMOTE_FLUSH_COMPACT_MAX) {
                hz4_inbox_push_one(tls, owner, sc, obj);
                fallback_objs++;
                continue;
            }
            owners[used] = owner;
            scs[used] = sc;
            heads[used] = obj;
            tails[used] = obj;
            used++;
            continue;
        }

        hz4_obj_set_next(tails[slot], obj);
        tails[slot] = obj;
    }

    for (uint32_t i = 0; i < used; i++) {
        if (heads[i] == tails[i]) {
            hz4_inbox_push_one(tls, owners[i], scs[i], heads[i]);
        } else {
            hz4_inbox_push_list(tls, owners[i], scs[i], heads[i], tails[i]);
        }
    }

#if HZ4_OS_STATS && !HZ4_OS_STATS_FAST
    hz4_os_stats_remote_flush_compact_hit();
    hz4_os_stats_remote_flush_compact_groups(used);
    if (fallback_objs != 0) {
        hz4_os_stats_remote_flush_compact_fallback(fallback_objs);
    }
#endif

    tls->rlen = 0;
}
#endif

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
        void* obj = tls->rbuf[0].obj;
        HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj);
        uint8_t owner = 0;
        uint8_t sc = 0;
        hz4_remote_flush_get_owner_sc(tls, 0, &owner, &sc);
        hz4_inbox_push_one(tls, owner, sc, obj);
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
            void* obj = tls->rbuf[i].obj;
            HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj);
            uint8_t owner = 0;
            uint8_t sc = 0;
            hz4_remote_flush_get_owner_sc(tls, i, &owner, &sc);
            hz4_inbox_push_one(tls, owner, sc, obj);
        }
        tls->rlen = 0;
        return;
    }

#if HZ4_REMOTE_FLUSH_COMPACT_BOX
    // ---- compact path (small band): 5..COMPACT_MAX ----
    if (tls->rlen <= HZ4_REMOTE_FLUSH_COMPACT_MAX) {
        hz4_remote_flush_inbox_compact(tls);
        return;
    }
#endif

#if HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX
    // ---- direct-index path (threshold band): owner/sc stamped lookup ----
    if (tls->rlen >= HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN) {
        hz4_remote_flush_inbox_direct_index(tls);
        return;
    }
#endif

#if HZ4_REMOTE_FLUSH_PAGE_BUCKET
    // PressureGate v0: when OFF, fall back to inbox bucket path.
#if HZ4_PRESSURE_GATE
    if (!hz4_pressure_gate_on(tls)) {
        hz4_remote_flush_inbox_bucket_general(tls);
        return;
    }
#endif
    // ---- General path: page-based bucketing ----
#if HZ4_REMOTE_FLUSH_TLS_BUCKETS
    uint32_t epoch = ++tls->flush_epoch;
    if (epoch == 0) {
        for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
            tls->pbucket_stamp[i] = 0;
        }
        epoch = ++tls->flush_epoch;
    }
    hz4_flush_bucket_t* pbuckets = tls->pbuckets;
    uint32_t* pbucket_stamp = tls->pbucket_stamp;
#else
    hz4_flush_bucket_t pbuckets[HZ4_REMOTE_FLUSH_BUCKETS];
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        pbuckets[i].page = NULL;
        pbuckets[i].head = NULL;
        pbuckets[i].tail = NULL;
        pbuckets[i].n = 0;
    }
#endif

    // Phase 1: Distribute to buckets by page (n 本固定ループ)
    for (uint32_t i = 0; i < tls->rlen; i++) {
        hz4_page_t* page = tls->rbuf[i].page;
        void* obj = tls->rbuf[i].obj;
        HZ4_REMOTE_FLUSH_CLEAR_NEXT_INBOX(obj);

        // Hash: XOR mix for better distribution
        uintptr_t paddr = (uintptr_t)page;
        uint32_t base_idx = (uint32_t)(((paddr >> HZ4_PAGE_SHIFT) ^ (paddr >> 12)) & (HZ4_REMOTE_FLUSH_BUCKETS - 1));

        // Linear probe for collision
        uint32_t slot = base_idx;
        int found = 0;
        for (uint32_t probe = 0; probe < HZ4_REMOTE_FLUSH_PROBE; probe++) {
            slot = (base_idx + probe) & (HZ4_REMOTE_FLUSH_BUCKETS - 1);
            hz4_flush_bucket_t* b = &pbuckets[slot];
#if HZ4_REMOTE_FLUSH_TLS_BUCKETS
            if (pbucket_stamp[slot] != epoch) {
                pbucket_stamp[slot] = epoch;
                b->page = NULL;
                b->head = NULL;
                b->tail = NULL;
                b->n = 0;
            }
#endif
            if (b->page == NULL) {
                // New bucket
                b->page = page;
                b->head = obj;
                b->tail = obj;
                b->n = 1;
                found = 1;
                break;
            } else if (b->page == page) {
                // Same page: link to tail
                hz4_obj_set_next(b->tail, obj);
                b->tail = obj;
                b->n++;
                found = 1;
                break;
            }
        }

        if (!found) {
            // Probe limit exceeded: push single object immediately
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
            tls->stats_tls.remote_flush_probe_ovf++;
#else
            hz4_os_stats_remote_flush_probe_overflow();
#endif
#endif
#if HZ4_RBUF_KEY
            uint8_t owner = tls->rbuf[i].owner;
            uint8_t sc = tls->rbuf[i].sc;
#else
#if HZ4_PAGE_META_SEPARATE
            hz4_page_meta_t* meta = hz4_page_meta(page);
            uint8_t owner = (uint8_t)hz4_owner_shard(meta->owner_tid);
            uint8_t sc = (uint8_t)meta->sc;
#else
            uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
            uint8_t sc = (uint8_t)page->sc;
#endif
#endif
#if HZ4_FAILFAST
            if (sc >= HZ4_SC_MAX) {
                HZ4_FAIL("hz4_remote_flush: invalid sc (probe overflow)");
            }
#endif
            hz4_inbox_push_one(tls, owner, sc, obj);
        }
    }

    // Phase 2: Flush buckets to inbox
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        hz4_flush_bucket_t* b = &pbuckets[i];
#if HZ4_REMOTE_FLUSH_TLS_BUCKETS
        if (pbucket_stamp[i] != epoch) {
            continue;
        }
#endif
        if (b->page == NULL || b->n == 0) {
            continue;
        }

        // Get owner/sc from page header
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* meta = hz4_page_meta(b->page);
        uint8_t sc = (uint8_t)meta->sc;
        uint16_t owner_tid = meta->owner_tid;
#else
        uint8_t sc = (uint8_t)b->page->sc;
        uint16_t owner_tid = b->page->owner_tid;
#endif

#if HZ4_FAILFAST
        if (sc >= HZ4_SC_MAX) {
            HZ4_FAIL("hz4_remote_flush: invalid sc");
        }
#endif

        uint8_t owner = (uint8_t)hz4_owner_shard(owner_tid);

        // n==1 uses push_one (lightweight)
        if (b->n == 1) {
            hz4_inbox_push_one(tls, owner, sc, b->head);
        } else {
            hz4_inbox_push_list(tls, owner, sc, b->head, b->tail);
        }
    }

    tls->rlen = 0;
#else
    hz4_remote_flush_inbox_bucket_general(tls);
#endif  // HZ4_REMOTE_FLUSH_PAGE_BUCKET
}
#else
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
#endif  // HZ4_REMOTE_INBOX
