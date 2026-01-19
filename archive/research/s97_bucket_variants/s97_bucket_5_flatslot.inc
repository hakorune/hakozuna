// Archived code: S97-5 (Direct-map flat slot; epoch+idx in one table)
// Copied from: hakozuna/hz3/src/hz3_tcache_remote.c
// Note: This is a reference snapshot; not built.

#elif HZ3_S97_REMOTE_STASH_BUCKET == 5
    // ==========================================================================
    // S97-5: Direct-map flat slot (epoch+idx in one table)
    // ==========================================================================
    // Goal: remove one TLS load vs S97-2 (stamp + idx arrays).
    //
    // Approach:
    // - slot[flat] stores (epoch<<16) | (idx+1). epoch is uint16_t.
    // - Hit if slot_epoch == epoch (idx is valid).
    // - Reset by epoch++ (wrap => memset slot).

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

#if HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS <= 0 || HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS > 256
#error "HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS must be in [1..256]"
#endif

    static __thread uint32_t s97_slot[S97_FLAT_TOTAL];
    static __thread uint16_t s97_epoch = 0;

    Hz3S97Bucket buckets[HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS];
    uint32_t nb = 0;

    if (s97_epoch == 0) {
        s97_epoch = 1;
    }

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

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

        uint32_t flat = (uint32_t)dst * (uint32_t)HZ3_BIN_TOTAL + (uint32_t)bin;
        uint32_t slot = s97_slot[flat];
        uint16_t slot_epoch = (uint16_t)(slot >> 16);
        if (slot_epoch == s97_epoch) {
            uint16_t idx = (uint16_t)slot;
            if (idx != 0) {
                idx = (uint16_t)(idx - 1u);
                if ((uint32_t)idx < nb) {
                    Hz3S97Bucket* b = &buckets[idx];
                    if (!b->tail_null_set) {
                        hz3_obj_set_next(b->tail, NULL);
                        b->tail_null_set = 1;
                    }
                    hz3_obj_set_next(ptr, b->head);
                    b->head = ptr;
                    b->n++;
                    continue;
                }
            }
        }

        if (nb >= (uint32_t)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS) {
            S97_BUCKET_FLUSH_ROUND();
            s97_epoch++;
            if (s97_epoch == 0) {
                memset(s97_slot, 0, sizeof(s97_slot));
                s97_epoch = 1;
            }
        }

        uint32_t new_idx = nb++;
        s97_slot[flat] = ((uint32_t)s97_epoch << 16) | (uint32_t)(new_idx + 1u);
        Hz3S97Bucket* b = &buckets[new_idx];
        b->dst = dst;
        b->bin = bin;
        b->tail_null_set = 0;
        b->head = ptr;
        b->tail = ptr;
        b->n = 1;
    }

    S97_BUCKET_FLUSH_ROUND();
    #undef S97_BUCKET_FLUSH_ROUND

    s97_epoch++;
    if (s97_epoch == 0) {
        memset(s97_slot, 0, sizeof(s97_slot));
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
    return;

