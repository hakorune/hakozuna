void hz3_remote_stash_flush_all_impl(void) {
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t t = ring->tail;
    uint16_t h = ring->head;

#if HZ3_REMOTE_STASH_DUP_FAILFAST
    uintptr_t seen_table[HZ3_REMOTE_STASH_DUP_TABLE_SIZE];
    hz3_remote_stash_dup_init(seen_table);
#endif

#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
    typedef struct {
        uint8_t dst;
        uint8_t sc;
        uint16_t _pad;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S226BucketAll;
    Hz3S226BucketAll s226_buckets[HZ3_S226_BUCKET_CHUNK_MAX];
    uint32_t s226_nb = 0;

    #define S226_BUCKET_FLUSH_ROUND_ALL() do { \
        for (uint32_t _i = 0; _i < s226_nb; _i++) { \
            Hz3S226BucketAll* _b = &s226_buckets[_i]; \
            if (_b->n == 0 || !_b->head || !_b->tail) continue; \
            uint32_t _bin = (uint32_t)HZ3_MEDIUM_BIN_BASE + (uint32_t)_b->sc; \
            hz3_remote_stash_dispatch_list(_b->dst, _bin, _b->head, _b->tail, _b->n); \
            _b->head = NULL; \
            _b->tail = NULL; \
            _b->n = 0; \
        } \
        s226_nb = 0; \
    } while (0)
#endif

    while (t != h) {
        uint32_t iters = 1;
#if HZ3_REMOTE_FLUSH_UNROLL
        iters = 4;
#endif
        for (uint32_t iter = 0; iter < iters && t != h; iter++) {
            Hz3RemoteStashEntry* entry0 = &ring->ring[t];
            uint8_t dst = entry0->dst;
            uint32_t bin = entry0->bin;
            uint16_t key = hz3_remote_stash_entry_key_load(entry0);
            void* tail = entry0->ptr;
#if !HZ3_S84_REMOTE_STASH_BATCH
            (void)key;
#endif
#if HZ3_REMOTE_STASH_DUP_FAILFAST
            hz3_remote_stash_dup_check(seen_table, tail, dst, bin, "flush_all");
            entry0->ptr = NULL;
            entry0->dst = 0;
            entry0->bin = 0;
#if HZ3_RBUF_KEY
            entry0->key = 0;
#endif
#endif

#if HZ3_S161_REMOTE_STASH_N1_DIRECT
            if (hz3_remote_stash_n1_direct_allowed(h, t) &&
                hz3_remote_stash_try_direct_n1(dst, bin, tail)) {
                t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
                continue;
            }
#endif

            void* head = tail;
#if !HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
            hz3_obj_set_next(tail, NULL);
#endif
            uint32_t n = 1;

            t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);

#if HZ3_S84_REMOTE_STASH_BATCH
            while (t != h && n < (uint32_t)HZ3_S84_REMOTE_STASH_BATCH_MAX) {
                Hz3RemoteStashEntry* entry = &ring->ring[t];
                if (hz3_remote_stash_entry_key_load(entry) != key) {
                    break;
                }
                void* ptr = entry->ptr;
#if HZ3_REMOTE_STASH_DUP_FAILFAST
                hz3_remote_stash_dup_check(seen_table, ptr, dst, bin, "flush_all:s84");
                entry->ptr = NULL;
                entry->dst = 0;
                entry->bin = 0;
#if HZ3_RBUF_KEY
                entry->key = 0;
#endif
#endif
                hz3_obj_set_next(ptr, head);  // LIFO build
                head = ptr;
                n++;
                t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
            }
#endif

#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
            if (bin >= (uint32_t)HZ3_MEDIUM_BIN_BASE) {
                int sc = (int)bin - HZ3_MEDIUM_BIN_BASE;
                if (sc >= HZ3_S226_SC_MIN && sc <= HZ3_S226_SC_MAX) {
                    uint32_t bi = 0;
                    int new_bucket = 0;
                    for (; bi < s226_nb; bi++) {
                        Hz3S226BucketAll* b = &s226_buckets[bi];
                        if (b->dst == dst && (int)b->sc == sc) {
                            break;
                        }
                    }
                    if (bi == s226_nb) {
                        if (s226_nb >= (uint32_t)HZ3_S226_BUCKET_CHUNK_MAX) {
                            S226_BUCKET_FLUSH_ROUND_ALL();
                            bi = 0;
                            new_bucket = 1;
                        } else {
                            new_bucket = 1;
                        }
                    }
                    if (new_bucket) {
                        if (bi != s226_nb) {
                            bi = s226_nb;
                        }
                        s226_nb = bi + 1u;
                    }
                    Hz3S226BucketAll* b = &s226_buckets[bi];
                    if (new_bucket || b->n == 0 || !b->head) {
                        b->dst = dst;
                        b->sc = (uint8_t)sc;
                        b->head = head;
                        b->tail = tail;
                        b->n = n;
                    } else {
                        hz3_obj_set_next(tail, b->head);
                        b->head = head;
                        b->n += n;
                    }
                    continue;
                }
            }
#endif

            hz3_remote_stash_dispatch_list(dst, bin, head, tail, n);
        }
    }

#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
    S226_BUCKET_FLUSH_ROUND_ALL();
    #undef S226_BUCKET_FLUSH_ROUND_ALL
#endif

    hz3_medium_flush_pending();
    ring->tail = t;
    t_hz3_cache.remote_hint = 0;  // safe, guaranteed empty
}

// Outbox stubs for sparse mode (not used but may be referenced)
void hz3_outbox_flush(uint8_t owner, int sc) {
    (void)owner;
    (void)sc;
}

void hz3_outbox_push(uint8_t owner, int sc, void* obj) {
    (void)owner;
    (void)sc;
    (void)obj;
}
