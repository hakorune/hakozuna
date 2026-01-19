// Archived code: S97-4 (Direct-map + touched reset; stamp-less, probe-less)
// Copied from: hakozuna/hz3/src/hz3_tcache_remote.c
// Note: This is a reference snapshot; not built.

#elif HZ3_S97_REMOTE_STASH_BUCKET == 4
    // ==========================================================================
    // S97-4: Direct-map + touched reset (stamp-less, probe-less, no stale check)
    // ==========================================================================
    // Goal: keep probe-less direct-map, reduce TLS footprint like S97-3, but avoid
    // the "stale sparse index -> key mismatch" overhead by explicitly clearing
    // only touched slots at the end of the round.
    //
    // Approach:
    // - slot[flat] stores (idx+1) into the local buckets[] array (0 means empty).
    // - Each time we insert a new key, record flat into touched[].
    // - Reset is O(touched) (<= MAX_KEYS): clear slot[flat]=0 and nb=0.

    typedef struct {
        uint8_t dst;
        uint8_t bin;
        uint8_t _pad0;
        uint8_t _pad1;
        void* head;
        void* tail;
        uint32_t n;
    } Hz3S97Bucket;

    enum { S97_FLAT_TOTAL = HZ3_NUM_SHARDS * HZ3_BIN_TOTAL };
    _Static_assert(S97_FLAT_TOTAL <= 65535, "S97_FLAT_TOTAL must fit in uint16_t");

#if HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS <= 0 || HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS > 256
#error "HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS must be in [1..256]"
#endif

    static __thread uint16_t s97_slot[S97_FLAT_TOTAL];  // idx+1, 0 means empty

    Hz3S97Bucket buckets[HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS];
    uint16_t touched[HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS];
    uint32_t nb = 0;
    uint32_t nt = 0;

    #define S97_BUCKET_FLUSH_ROUND() do { \
        for (uint32_t _i = 0; _i < nb; _i++) { \
            Hz3S97Bucket* _b = &buckets[_i]; \
            void* _head = _b->head; \
            void* _tail = _b->tail; \
            uint32_t _n = _b->n; \
            if (_n == 0) continue; \
            if (_n == 1) { \
                if (!HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL) { \
                    hz3_obj_set_next(_tail, NULL); \
                } \
            } \
            /* n>1: tail->next is set to NULL at the moment we add the 2nd entry */ \
            S97_LOC_GROUPS_INC(); \
            S97_LOC_NMAX(_n); \
            S97_LOC_NGT1(_n); \
            S97_LOC_SEEN(_b->dst, _b->bin); \
            S97_LOC_CAT(_b->bin); \
            S97_LOC_SELFDST(_b->dst); \
            hz3_remote_stash_dispatch_list(_b->dst, (uint32_t)_b->bin, _head, _tail, _n); \
        } \
        for (uint32_t _j = 0; _j < nt; _j++) { \
            s97_slot[(uint32_t)touched[_j]] = 0; \
        } \
        nb = 0; \
        nt = 0; \
    } while (0)

    while (t != h && drained < budget_entries) {
        Hz3RemoteStashEntry* entry0 = &ring->ring[t];
        uint8_t dst = entry0->dst;
        uint8_t bin = entry0->bin;
        void* ptr = entry0->ptr;

        t = (t + 1) & (HZ3_REMOTE_STASH_RING_SIZE - 1);
        drained++;

        uint32_t flat = (uint32_t)dst * (uint32_t)HZ3_BIN_TOTAL + (uint32_t)bin;
        uint16_t v = s97_slot[flat];
        if (v != 0) {
            uint32_t idx = (uint32_t)v - 1u;
            Hz3S97Bucket* b = &buckets[idx];
            if (b->n == 1) {
                hz3_obj_set_next(b->tail, NULL);
            }
            hz3_obj_set_next(ptr, b->head);
            b->head = ptr;
            b->n++;
            continue;
        }

        if (nb >= (uint32_t)HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS) {
            S97_BUCKET_FLUSH_ROUND();
        }

        uint32_t new_idx = nb++;
        s97_slot[flat] = (uint16_t)(new_idx + 1u);
        touched[nt++] = (uint16_t)flat;
        Hz3S97Bucket* b = &buckets[new_idx];
        b->dst = dst;
        b->bin = bin;
        b->head = ptr;
        b->tail = ptr;
        b->n = 1;
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
    return;

