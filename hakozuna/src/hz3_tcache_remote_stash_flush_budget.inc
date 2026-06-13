void hz3_remote_stash_push(uint8_t dst, uint32_t bin, void* ptr) {
#if HZ3_S82_STASH_GUARD
    hz3_s82_stash_guard_one("remote_stash_push", ptr, bin);
#endif
    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t h = ring->head;
    uint16_t next_h = (h + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);

    // Check for overflow BEFORE writing (prevent overwriting old entry)
    if (__builtin_expect(next_h == ring->tail, 0)) {
        // Ring full → emergency flush
        hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
    }

#if HZ3_REMOTE_STASH_DUP_FAILFAST
    // Debug: detect duplicate ptr already queued in this ring.
    uint16_t scan = ring->tail;
    while (scan != ring->head) {
        if (ring->ring[scan].ptr == ptr) {
            fprintf(stderr,
                    "[HZ3_REMOTE_STASH_DUP_FAILFAST] where=push dst=%u bin=%u ptr=%p\n",
                    (unsigned)dst, (unsigned)bin, ptr);
            abort();
        }
        scan = (scan + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
    }
#endif

    // Now safe to write
    ring->ring[h].dst = dst;
    ring->ring[h].bin = (uint8_t)bin;
#if HZ3_RBUF_KEY
    ring->ring[h].key = (uint16_t)(((uint16_t)dst << 8) | (uint16_t)bin);
#endif
    ring->ring[h].ptr = ptr;

    ring->head = next_h;
    // Mark remote activity to trigger budget flush on next slow path.
    t_hz3_cache.remote_hint = 1;
#if HZ3_S173_DSTBIN_DEMAND_GATE && HZ3_REMOTE_STASH_SPARSE
    if (t_hz3_cache.remote_flush_credit < (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP) {
        t_hz3_cache.remote_flush_credit++;
    }
#endif
}

// S41: budget_entries = number of ring entries to drain (not bin count)
void hz3_remote_stash_flush_budget_impl(uint32_t budget_entries) {
    S97_STAT_REGISTER();
    S97_STAT_INC(g_s97_flush_budget_calls);

    Hz3RemoteStashRing* ring = &t_hz3_cache.remote_stash;
    uint16_t t = ring->tail;
    uint16_t h = ring->head;
    uint32_t drained = 0;

#if HZ3_REMOTE_STASH_DUP_FAILFAST
    uintptr_t seen_table[HZ3_REMOTE_STASH_DUP_TABLE_SIZE];
    hz3_remote_stash_dup_init(seen_table);
#endif

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    uint16_t seen_keys[512];
    memset(seen_keys, 0xFF, sizeof(seen_keys));
    uint32_t local_groups = 0;  // dispatch calls
    uint32_t local_distinct = 0;
    uint32_t local_n_max = 0;
    uint32_t local_n_gt1_calls = 0;
    uint32_t local_n_gt1_entries = 0;
    uint32_t local_small_groups = 0;
    uint32_t local_sub4k_groups = 0;
    uint32_t local_medium_groups = 0;
    uint32_t local_selfdst_groups = 0;

    #define S97_LOC_GROUPS_INC() do { local_groups++; } while (0)
    #define S97_LOC_NMAX(n_) do { \
        uint32_t s97_n = (uint32_t)(n_); \
        if (s97_n > local_n_max) local_n_max = s97_n; \
    } while (0)
    #define S97_LOC_NGT1(n_) do { \
        uint32_t s97_n = (uint32_t)(n_); \
        if (s97_n > 1) { local_n_gt1_calls++; local_n_gt1_entries += s97_n; } \
    } while (0)
    #define S97_LOC_SEEN(dst_, bin_) do { \
        uint16_t s97_key = (uint16_t)(((uint16_t)(dst_) << 8) | (uint16_t)(bin_)); \
        if (hz3_s97_seen_insert(seen_keys, s97_key)) { local_distinct++; } \
    } while (0)
    #define S97_LOC_CAT(bin_) do { \
        uint32_t s97_bin32 = (uint32_t)(bin_); \
        if (s97_bin32 < HZ3_SMALL_NUM_SC) local_small_groups++; \
        else if (s97_bin32 < HZ3_MEDIUM_BIN_BASE) local_sub4k_groups++; \
        else local_medium_groups++; \
    } while (0)
    #define S97_LOC_SELFDST(dst_) do { if ((uint8_t)(dst_) == (uint8_t)t_hz3_cache.my_shard) local_selfdst_groups++; } while (0)
#endif

#if !HZ3_S97_REMOTE_STASH_FLUSH_STATS
    #define S97_LOC_GROUPS_INC() ((void)0)
    #define S97_LOC_NMAX(n_) ((void)(n_))
    #define S97_LOC_NGT1(n_) ((void)(n_))
    #define S97_LOC_SEEN(dst_, bin_) do { (void)(dst_); (void)(bin_); } while (0)
    #define S97_LOC_CAT(bin_) ((void)(bin_))
    #define S97_LOC_SELFDST(dst_) ((void)(dst_))
#endif

#if HZ3_S97_REMOTE_STASH_BUCKET
#if HZ3_S97_REMOTE_STASH_BUCKET == 2
    // ==========================================================================
    // S97-2: Direct-map + stamp (probe-less bucketize)
    // ==========================================================================
    // Key insight: flat = dst * HZ3_BIN_TOTAL + bin has bounded range, so we can
    // use direct indexing instead of hash + open addressing. Reset via epoch++.

    typedef struct {
        uint8_t dst;
        uint8_t bin;
        uint8_t tail_null_set;
        uint8_t _pad;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S97Bucket;

    enum { S97_FLAT_TOTAL = HZ3_NUM_SHARDS * HZ3_BIN_TOTAL };
    _Static_assert(S97_FLAT_TOTAL <= 65535, "S97_FLAT_TOTAL must fit in uint16_t");

    // TLS direct-map tables (stamp + bucket index)
    static HZ3_TLS uint32_t s97_stamp[S97_FLAT_TOTAL];
    static HZ3_TLS uint16_t s97_bucket_idx[S97_FLAT_TOTAL];
    static HZ3_TLS uint32_t s97_epoch = 0;

    // Local bucket array (stack allocated, reused per flush)
    Hz3S97Bucket buckets[HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS];
    uint32_t nb = 0;

    // First call initialization: set epoch to 1
    if (s97_epoch == 0) {
        s97_epoch = 1;
    }

    // Dispatch macro (same as S97-1)
    #define S97_BUCKET_FLUSH_ROUND() do { \
        for (uint32_t _i = 0; _i < nb; _i++) { \
            Hz3S97Bucket* _b = &buckets[_i]; \
            void* _head = _b->head; \
            void* _tail = _b->tail; \
            uint32_t _n = _b->n; \
            if (_n == 0) continue; \
            if (_n > 1) { \
                if (!_b->tail_null_set) { \
                    hz3_obj_set_next(_tail, NULL); \
                    _b->tail_null_set = 1; \
                } \
            } else { \
                if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) { \
                    hz3_obj_set_next(_tail, NULL); \
                } \
            } \
            S97_LOC_GROUPS_INC(); \
            S97_LOC_NMAX(_n); \
            S97_LOC_NGT1(_n); \
            S97_LOC_SEEN(_b->dst, _b->bin); \
            S97_LOC_CAT(_b->bin); \
            S97_LOC_SELFDST(_b->dst); \
            hz3_remote_stash_dispatch_list(_b->dst, (uint32_t)_b->bin, _head, _tail, _n); \
        } \
        nb = 0; \
    } while (0)

    while (t != h && drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint8_t bin = entry0->bin;
        void* ptr = entry0->ptr;
#if HZ3_REMOTE_STASH_DUP_FAILFAST
        hz3_remote_stash_dup_check(seen_table, ptr, dst, bin, "budget:s97-2");
        entry0->ptr = NULL;
        entry0->dst = 0;
        entry0->bin = 0;
#if HZ3_RBUF_KEY
        entry0->key = 0;
#endif
#endif

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

        // Direct-map lookup: flat index = dst * BIN_TOTAL + bin
        uint32_t flat = (uint32_t)dst * (uint32_t)HZ3_BIN_TOTAL + (uint32_t)bin;

        if (s97_stamp[flat] == s97_epoch) {
            // Hit: append to existing bucket
            uint16_t idx = s97_bucket_idx[flat];
            Hz3S97Bucket* b = &buckets[idx];
            // First time we see a 2nd entry, null-terminate original tail
            if (!b->tail_null_set) {
                hz3_obj_set_next(b->tail, NULL);
                b->tail_null_set = 1;
            }
            hz3_obj_set_next(ptr, b->head);
            b->head = ptr;
            b->n++;
        } else {
            // Miss: allocate new bucket
            if (nb >= (uint32_t)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS) {
                // Too many distinct keys: flush and reset epoch
                S97_BUCKET_FLUSH_ROUND();
                s97_epoch++;
                if (s97_epoch == 0) {
                    // Wrap around: clear stamp array
                    memset(s97_stamp, 0, sizeof(s97_stamp));
                    s97_epoch = 1;
                }
            }
            s97_stamp[flat] = s97_epoch;
            s97_bucket_idx[flat] = (uint16_t)nb;
            Hz3S97Bucket* b = &buckets[nb++];
            b->dst = dst;
            b->bin = bin;
            b->tail_null_set = 0;
            b->head = ptr;
            b->tail = ptr;
            b->n = 1;
        }
    }

    S97_BUCKET_FLUSH_ROUND();
    #undef S97_BUCKET_FLUSH_ROUND

    // End of flush: increment epoch for next round (O(1) reset)
    s97_epoch++;
    if (s97_epoch == 0) {
        memset(s97_stamp, 0, sizeof(s97_stamp));
        s97_epoch = 1;
    }

    ring->tail = t;
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
    hz3_medium_flush_pending();
    return;

#elif HZ3_S97_REMOTE_STASH_BUCKET == 6
    // ==========================================================================
    // S97-8: Table-less radix sort + group (stack-only, no TLS tables)
    // ==========================================================================
    // Motivation:
    // - Avoid TLS table pressure (direct-map variants) and avoid hash probe branch-miss.
    // - budget_entries is typically small (e.g., 32), so stack-local sort+group may win.
    //
    // Approach:
    // 1) Drain up to `budget_entries` entries into local arrays (key, ptr).
    // 2) Radix sort (2 passes, 8-bit digits) by 16-bit key = (dst<<8)|bin.
    // 3) Group consecutive identical keys and build a list by prepending.

    enum { S97_SORT_MAX = 256 };

    // Local buffers (stack): use small fixed max and chunk if budget is larger.
    uint16_t keys0[S97_SORT_MAX];
    void* ptrs0[S97_SORT_MAX];
    uint16_t keys1[S97_SORT_MAX];
    void* ptrs1[S97_SORT_MAX];

    // Radix counts: bin is guaranteed < HZ3_BIN_TOTAL and dst < HZ3_NUM_SHARDS.
    // Keep loops bounded to these ranges (avoid fixed 256 cost on every flush).
    _Static_assert(HZ3_BIN_TOTAL <= 255, "S97-8 requires HZ3_BIN_TOTAL <= 255");
    _Static_assert(HZ3_NUM_SHARDS <= 255, "S97-8 requires HZ3_NUM_SHARDS <= 255");

    uint16_t cnt_lo[HZ3_BIN_TOTAL];
    uint16_t pos_lo[HZ3_BIN_TOTAL];
    uint16_t cnt_hi[HZ3_NUM_SHARDS];
    uint16_t pos_hi[HZ3_NUM_SHARDS];

    while (t != h && drained < budget_entries) {
        uint32_t m = 0;
        while (t != h && drained < budget_entries && m < (uint32_t)S97_SORT_MAX) {
            Hz3RemoteStashEntry* entry0 = &ring->ring[t];
            uint8_t dst = entry0->dst;
            uint8_t bin = entry0->bin;
            void* ptr = entry0->ptr;
#if HZ3_REMOTE_STASH_DUP_FAILFAST
            hz3_remote_stash_dup_check(seen_table, ptr, dst, bin, "budget:s97-6");
            entry0->ptr = NULL;
            entry0->dst = 0;
            entry0->bin = 0;
#if HZ3_RBUF_KEY
            entry0->key = 0;
#endif
#endif

            t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
            drained++;

            keys0[m] = hz3_remote_stash_entry_key_load(entry0);
            ptrs0[m] = ptr;
            m++;
        }

        if (m == 0) {
            break;
        }

        // Pass 0: low byte
        memset(cnt_lo, 0, sizeof(cnt_lo));
        for (uint32_t i = 0; i < m; i++) {
            uint8_t bin = (uint8_t)(keys0[i] & 0xFFu);
            cnt_lo[bin]++;
        }
        uint16_t sum = 0;
        for (uint32_t b = 0; b < (uint32_t)HZ3_BIN_TOTAL; b++) {
            uint16_t c = cnt_lo[b];
            pos_lo[b] = sum;
            sum = (uint16_t)(sum + c);
        }
        for (uint32_t i = 0; i < m; i++) {
            uint8_t bin = (uint8_t)(keys0[i] & 0xFFu);
            uint16_t p = pos_lo[bin]++;
            keys1[p] = keys0[i];
            ptrs1[p] = ptrs0[i];
        }

        // Pass 1: high byte
        memset(cnt_hi, 0, sizeof(cnt_hi));
        for (uint32_t i = 0; i < m; i++) {
            uint8_t dst = (uint8_t)(keys1[i] >> 8);
            cnt_hi[dst]++;
        }
        sum = 0;
        for (uint32_t b = 0; b < (uint32_t)HZ3_NUM_SHARDS; b++) {
            uint16_t c = cnt_hi[b];
            pos_hi[b] = sum;
            sum = (uint16_t)(sum + c);
        }
        for (uint32_t i = 0; i < m; i++) {
            uint8_t dst = (uint8_t)(keys1[i] >> 8);
            uint16_t p = pos_hi[dst]++;
            keys0[p] = keys1[i];
            ptrs0[p] = ptrs1[i];
        }

        // Group + dispatch
        uint32_t i = 0;
        while (i < m) {
            uint16_t key = keys0[i];
            uint8_t dst = (uint8_t)(key >> 8);
            uint8_t bin = (uint8_t)(key & 0xFFu);

            void* head = ptrs0[i];
            void* tail = head;
            uint32_t n = 1;
            i++;

            while (i < m && keys0[i] == key) {
                void* obj = ptrs0[i];
                hz3_obj_set_next(obj, head);
                head = obj;
                n++;
                i++;
            }

            if (n > 1) {
                hz3_obj_set_next(tail, NULL);
            } else {
#if !HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
                hz3_obj_set_next(tail, NULL);
#endif
            }

            S97_LOC_GROUPS_INC();
            S97_LOC_NMAX(n);
            S97_LOC_NGT1(n);
            S97_LOC_SEEN(dst, bin);
            S97_LOC_CAT(bin);
            S97_LOC_SELFDST(dst);
            hz3_remote_stash_dispatch_list(dst, (uint32_t)bin, head, tail, n);
        }
    }

    ring->tail = t;
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
    hz3_medium_flush_pending();
    return;

#else  // HZ3_S97_REMOTE_STASH_BUCKET == 1
    // ==========================================================================
    // S97-1: Hash + open addressing bucketize
    // ==========================================================================
    // Bucketize by (dst,bin) within this flush_budget window.
    // Goal: reduce dispatch calls (push_list) when n=1 dominates but keys repeat.
    typedef struct {
        uint16_t key;  // (dst<<8)|bin
        uint8_t dst;
        uint8_t bin;
        uint8_t tail_null_set;
        uint8_t _pad;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S97Bucket;

    enum { S97_HASH_SIZE = 256 };  // must be power-of-two
    enum { S97_HASH_MASK = S97_HASH_SIZE - 1 };

#if HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS <= 0 || HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS > 256
#error "HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS must be in [1..256]"
#endif

    uint16_t map[S97_HASH_SIZE];
    Hz3S97Bucket buckets[S97_HASH_SIZE];
    uint32_t nb = 0;
    memset(map, 0xFF, sizeof(map));

    // Dispatch current buckets and reset map.
    // Note: does not touch ring indices; only uses accumulated buckets.
    #define S97_BUCKET_FLUSH_ROUND() do { \
        for (uint32_t _i = 0; _i < nb; _i++) { \
            Hz3S97Bucket* _b = &buckets[_i]; \
            void* _head = _b->head; \
            void* _tail = _b->tail; \
            uint32_t _n = _b->n; \
            if (_n == 0) continue; \
            if (_n > 1) { \
                if (!_b->tail_null_set) { \
                    hz3_obj_set_next(_tail, NULL); \
                    _b->tail_null_set = 1; \
                } \
            } else { \
                /* n==1: only null-terminate if requested */ \
                if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) { \
                    hz3_obj_set_next(_tail, NULL); \
                } \
            } \
            S97_LOC_GROUPS_INC(); \
            S97_LOC_NMAX(_n); \
            S97_LOC_NGT1(_n); \
            S97_LOC_SEEN(_b->dst, _b->bin); \
            S97_LOC_CAT(_b->bin); \
            S97_LOC_SELFDST(_b->dst); \
            hz3_remote_stash_dispatch_list(_b->dst, (uint32_t)_b->bin, _head, _tail, _n); \
        } \
        nb = 0; \
        memset(map, 0xFF, sizeof(map)); \
    } while (0)

    while (t != h && drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint8_t bin = entry0->bin;
        void* ptr = entry0->ptr;
#if HZ3_REMOTE_STASH_DUP_FAILFAST
        hz3_remote_stash_dup_check(seen_table, ptr, dst, bin, "budget:s97-1");
        entry0->ptr = NULL;
        entry0->dst = 0;
        entry0->bin = 0;
#if HZ3_RBUF_KEY
        entry0->key = 0;
#endif
#endif

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

        uint16_t key = hz3_remote_stash_entry_key_load(entry0);
        uint32_t idx = ((uint32_t)key * 2654435761u) & (uint32_t)S97_HASH_MASK;

        for (uint32_t probe = 0; probe < (uint32_t)S97_HASH_SIZE; probe++) {
            uint16_t mi = map[idx];
            if (mi == 0xFFFFu) {
                if (nb >= (uint32_t)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS) {
                    S97_BUCKET_FLUSH_ROUND();
                }
                map[idx] = (uint16_t)nb;
                Hz3S97Bucket* b = &buckets[nb++];
                b->key = key;
                b->dst = dst;
                b->bin = bin;
                b->tail_null_set = 0;
                b->head = ptr;
                b->tail = ptr;
                b->n = 1;
                break;
            }

            Hz3S97Bucket* b = &buckets[mi];
            if (b->key == key) {
                // First time we see a 2nd entry for this key, we must null-terminate
                // the current tail (which is the first element) before linking.
                if (!b->tail_null_set) {
                    hz3_obj_set_next(b->tail, NULL);
                    b->tail_null_set = 1;
                }
                hz3_obj_set_next(ptr, b->head);
                b->head = ptr;
                b->n++;
                break;
            }

            idx = (idx + 1u) & (uint32_t)S97_HASH_MASK;
        }
    }

    S97_BUCKET_FLUSH_ROUND();
    #undef S97_BUCKET_FLUSH_ROUND

    ring->tail = t;
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
    hz3_medium_flush_pending();
    return;
#endif  // HZ3_S97_REMOTE_STASH_BUCKET == 1/2/6
#endif  // HZ3_S97_REMOTE_STASH_BUCKET

#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
    typedef struct {
        uint8_t dst;
        uint8_t sc;
        uint16_t _pad;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S226Bucket;
    Hz3S226Bucket s226_buckets[HZ3_S226_BUCKET_CHUNK_MAX];
    uint32_t s226_nb = 0;

    #define S226_BUCKET_FLUSH_ROUND() do { \
        for (uint32_t _i = 0; _i < s226_nb; _i++) { \
            Hz3S226Bucket* _b = &s226_buckets[_i]; \
            if (_b->n == 0 || !_b->head || !_b->tail) continue; \
            uint32_t _bin = (uint32_t)HZ3_MEDIUM_BIN_BASE + (uint32_t)_b->sc; \
            S97_LOC_GROUPS_INC(); \
            S97_LOC_NMAX(_b->n); \
            S97_LOC_NGT1(_b->n); \
            S97_LOC_SEEN(_b->dst, _bin); \
            S97_LOC_CAT(_bin); \
            S97_LOC_SELFDST(_b->dst); \
            hz3_remote_stash_dispatch_list(_b->dst, _bin, _b->head, _b->tail, _b->n); \
            _b->head = NULL; \
            _b->tail = NULL; \
            _b->n = 0; \
        } \
        s226_nb = 0; \
    } while (0)
#endif

    while (t != h && drained < budget_entries) {
        uint32_t iters = 1;
#if HZ3_REMOTE_FLUSH_UNROLL
        iters = 4;
#endif
        for (uint32_t iter = 0; iter < iters && t != h && drained < budget_entries; iter++) {
            Hz3RemoteStashEntry* entry0 = &ring->ring[t];
            uint8_t dst = entry0->dst;
            uint32_t bin = entry0->bin;
            uint16_t key = hz3_remote_stash_entry_key_load(entry0);
            void* tail = entry0->ptr;
#if !HZ3_S84_REMOTE_STASH_BATCH
            (void)key;
#endif
#if HZ3_REMOTE_STASH_DUP_FAILFAST
            hz3_remote_stash_dup_check(seen_table, tail, dst, bin, "budget:s97-1");
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
                drained++;
#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
                S97_LOC_GROUPS_INC();
                S97_LOC_NMAX(1);
                S97_LOC_NGT1(1);
                S97_LOC_SEEN(dst, bin);
                S97_LOC_CAT(bin);
                S97_LOC_SELFDST(dst);
#endif
                continue;
            }
#endif

            void* head = tail;
#if !HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
            hz3_obj_set_next(tail, NULL);
#endif
            uint32_t n = 1;

            t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
            drained++;

#if HZ3_S84_REMOTE_STASH_BATCH
            while (t != h && drained < budget_entries && n < (uint32_t)HZ3_S84_REMOTE_STASH_BATCH_MAX) {
                Hz3RemoteStashEntry* entry = &ring->ring[t];
                if (hz3_remote_stash_entry_key_load(entry) != key) {
                    break;
                }
                void* ptr = entry->ptr;
#if HZ3_REMOTE_STASH_DUP_FAILFAST
                hz3_remote_stash_dup_check(seen_table, ptr, dst, bin, "budget:s84");
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
                drained++;
            }
#endif

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
            S97_LOC_GROUPS_INC();
            S97_LOC_NMAX(n);
            S97_LOC_NGT1(n);
            S97_LOC_SEEN(dst, bin);
            S97_LOC_CAT(bin);
            S97_LOC_SELFDST(dst);
#endif

#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
            if (bin >= (uint32_t)HZ3_MEDIUM_BIN_BASE) {
                int sc = (int)bin - HZ3_MEDIUM_BIN_BASE;
                if (sc >= HZ3_S226_SC_MIN && sc <= HZ3_S226_SC_MAX) {
                    uint32_t bi = 0;
                    int new_bucket = 0;
                    for (; bi < s226_nb; bi++) {
                        Hz3S226Bucket* b = &s226_buckets[bi];
                        if (b->dst == dst && (int)b->sc == sc) {
                            break;
                        }
                    }
                    if (bi == s226_nb) {
                        if (s226_nb >= (uint32_t)HZ3_S226_BUCKET_CHUNK_MAX) {
                            S226_BUCKET_FLUSH_ROUND();
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
                    Hz3S226Bucket* b = &s226_buckets[bi];
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
    S226_BUCKET_FLUSH_ROUND();
    #undef S226_BUCKET_FLUSH_ROUND
#endif

    hz3_medium_flush_pending();
    ring->tail = t;
    // It is safe to clear remote_hint iff the ring is now empty.
    t_hz3_cache.remote_hint = (ring->tail != ring->head);

#if HZ3_S97_REMOTE_STASH_FLUSH_STATS
    S97_STAT_ADD(g_s97_flush_budget_entries_total, drained);
    S97_STAT_ADD(g_s97_flush_budget_groups_total, local_groups);
    S97_STAT_ADD(g_s97_flush_budget_distinct_keys_total, local_distinct);
    if (local_groups > local_distinct) {
        S97_STAT_ADD(g_s97_flush_budget_potential_merge_calls_total, (local_groups - local_distinct));
    }
    if (drained > local_groups) {
        S97_STAT_ADD(g_s97_flush_budget_saved_calls_total, (drained - local_groups));
    }
    S97_STAT_MAX(g_s97_flush_budget_n_max, local_n_max);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_calls_total, local_n_gt1_calls);
    S97_STAT_ADD(g_s97_flush_budget_n_gt1_entries_total, local_n_gt1_entries);
    S97_STAT_ADD(g_s97_flush_budget_small_groups, local_small_groups);
    S97_STAT_ADD(g_s97_flush_budget_sub4k_groups, local_sub4k_groups);
    S97_STAT_ADD(g_s97_flush_budget_medium_groups, local_medium_groups);
    S97_STAT_ADD(g_s97_flush_budget_selfdst_groups, local_selfdst_groups);
#endif
}

#undef S97_LOC_GROUPS_INC
#undef S97_LOC_NMAX
#undef S97_LOC_NGT1
#undef S97_LOC_SEEN
#undef S97_LOC_CAT
#undef S97_LOC_SELFDST
