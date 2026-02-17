// ============================================================================
// S41: Sparse RemoteStash implementation (scale lane)
// ============================================================================

#if HZ3_REMOTE_STASH_SPARSE
// Forward declarations for dispatcher
extern void hz3_small_v2_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
#if HZ3_LANE_T16_R90_PAGE_REMOTE
extern void hz3_lane16_push_remote_list_small(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
#endif
#if HZ3_HZ4_BRIDGE
extern void hz3_hz4_bridge_push_remote_list_small(uint8_t owner, int sc,
                                                  void* head, void* tail, uint32_t n);
#endif
extern void hz3_sub4k_push_remote_list(uint8_t dst, int sc, void* head, void* tail, uint32_t n);
extern void hz3_inbox_push_list(uint8_t dst, int sc, void* head, void* tail, uint32_t n);
extern void hz3_inbox_medium_mixed_push_list(uint8_t owner, void* head, void* tail, uint32_t n);

#if HZ3_S199_MEDIUM_DISPATCH_MICROBATCH
typedef struct {
    void* head;
    void* tail;
    uint32_t count;
} Hz3S199MediumOutbox;

static HZ3_TLS Hz3S199MediumOutbox t_s199_medium_outbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static HZ3_TLS uint8_t t_s199_medium_touched_flag[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static HZ3_TLS uint16_t t_s199_medium_touched_keys[HZ3_NUM_SHARDS * HZ3_NUM_SC];
static HZ3_TLS uint16_t t_s199_medium_touched_count;

static inline void hz3_s199_medium_flush_one(uint8_t dst, int sc) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS) {
        return;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return;
    }
    Hz3S199MediumOutbox* ob = &t_s199_medium_outbox[dst][sc];
    if (!ob->head || !ob->tail || ob->count == 0) {
        ob->head = NULL;
        ob->tail = NULL;
        ob->count = 0;
        t_s199_medium_touched_flag[dst][sc] = 0;
        return;
    }
    hz3_obj_set_next(ob->tail, NULL);
    hz3_inbox_push_list(dst, sc, ob->head, ob->tail, ob->count);
    ob->head = NULL;
    ob->tail = NULL;
    ob->count = 0;
    t_s199_medium_touched_flag[dst][sc] = 0;
}

static inline void hz3_s199_medium_touch(uint8_t dst, int sc) {
    if (t_s199_medium_touched_flag[dst][sc]) {
        return;
    }
    if (t_s199_medium_touched_count >= (uint16_t)(HZ3_NUM_SHARDS * HZ3_NUM_SC)) {
        for (uint16_t i = 0; i < t_s199_medium_touched_count; i++) {
            uint16_t key = t_s199_medium_touched_keys[i];
            uint8_t key_dst = (uint8_t)(key / HZ3_NUM_SC);
            int key_sc = (int)(key % HZ3_NUM_SC);
            hz3_s199_medium_flush_one(key_dst, key_sc);
        }
        t_s199_medium_touched_count = 0;
    }
    t_s199_medium_touched_flag[dst][sc] = 1;
    t_s199_medium_touched_keys[t_s199_medium_touched_count++] =
        (uint16_t)((uint16_t)dst * (uint16_t)HZ3_NUM_SC + (uint16_t)sc);
}

static inline void hz3_s199_medium_dispatch_push(uint8_t dst, int sc,
                                                 void* head, void* tail, uint32_t n) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC ||
        !head || !tail || n == 0) {
        hz3_inbox_push_list(dst, sc, head, tail, n);
        return;
    }

    Hz3S199MediumOutbox* ob = &t_s199_medium_outbox[dst][sc];
    if (!ob->head) {
        hz3_s199_medium_touch(dst, sc);
        ob->head = head;
        ob->tail = tail;
        ob->count = n;
    } else {
        hz3_obj_set_next(ob->tail, head);
        ob->tail = tail;
        ob->count += n;
    }

    if (ob->count >= (uint32_t)HZ3_S199_MEDIUM_OUTBOX_FLUSH_N) {
        hz3_s199_medium_flush_one(dst, sc);
    }
}

static inline void hz3_s199_medium_flush_pending(void) {
    for (uint16_t i = 0; i < t_s199_medium_touched_count; i++) {
        uint16_t key = t_s199_medium_touched_keys[i];
        uint8_t dst = (uint8_t)(key / HZ3_NUM_SC);
        int sc = (int)(key % HZ3_NUM_SC);
        hz3_s199_medium_flush_one(dst, sc);
    }
    t_s199_medium_touched_count = 0;
}
#else
static inline void hz3_s199_medium_dispatch_push(uint8_t dst, int sc,
                                                 void* head, void* tail, uint32_t n) {
    hz3_inbox_push_list(dst, sc, head, tail, n);
}

static inline void hz3_s199_medium_flush_pending(void) {
}
#endif

#if HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
typedef struct {
    void* head;
    void* tail;
    uint32_t count;
} Hz3S221MediumOutbox;

static HZ3_TLS Hz3S221MediumOutbox t_s221_medium_outbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static HZ3_TLS uint8_t t_s221_medium_touched_flag[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static HZ3_TLS uint16_t t_s221_medium_touched_keys[HZ3_NUM_SHARDS * HZ3_NUM_SC];
static HZ3_TLS uint16_t t_s221_medium_touched_count;

static inline void hz3_s221_medium_flush_one(uint8_t dst, int sc) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS) {
        return;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return;
    }
    Hz3S221MediumOutbox* ob = &t_s221_medium_outbox[dst][sc];
    if (!ob->head || !ob->tail || ob->count == 0) {
        ob->head = NULL;
        ob->tail = NULL;
        ob->count = 0;
        t_s221_medium_touched_flag[dst][sc] = 0;
        return;
    }
    hz3_obj_set_next(ob->tail, NULL);
    hz3_inbox_push_list(dst, sc, ob->head, ob->tail, ob->count);
    ob->head = NULL;
    ob->tail = NULL;
    ob->count = 0;
    t_s221_medium_touched_flag[dst][sc] = 0;
}

static inline void hz3_s221_medium_touch(uint8_t dst, int sc) {
    if (t_s221_medium_touched_flag[dst][sc]) {
        return;
    }
    if (t_s221_medium_touched_count >= (uint16_t)(HZ3_NUM_SHARDS * HZ3_NUM_SC)) {
        for (uint16_t i = 0; i < t_s221_medium_touched_count; i++) {
            uint16_t key = t_s221_medium_touched_keys[i];
            uint8_t key_dst = (uint8_t)(key / HZ3_NUM_SC);
            int key_sc = (int)(key % HZ3_NUM_SC);
            hz3_s221_medium_flush_one(key_dst, key_sc);
        }
        t_s221_medium_touched_count = 0;
    }
    t_s221_medium_touched_flag[dst][sc] = 1;
    t_s221_medium_touched_keys[t_s221_medium_touched_count++] =
        (uint16_t)((uint16_t)dst * (uint16_t)HZ3_NUM_SC + (uint16_t)sc);
}

static inline int hz3_s221_medium_dispatch_try_push(uint8_t dst, int sc,
                                                    void* head, void* tail, uint32_t n) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC ||
        !head || !tail || n == 0) {
        return 0;
    }
    if (dst == t_hz3_cache.my_shard) {
        return 0;
    }
    if (sc < HZ3_S221_SC_MIN || sc > HZ3_S221_SC_MAX) {
        return 0;
    }
#if HZ3_S221_ONLY_N1
    if (n != 1 || head != tail) {
        return 0;
    }
#endif

    Hz3S221MediumOutbox* ob = &t_s221_medium_outbox[dst][sc];
    if (!ob->head) {
        hz3_s221_medium_touch(dst, sc);
        ob->head = head;
        ob->tail = tail;
        ob->count = n;
    } else {
        hz3_obj_set_next(ob->tail, head);
        ob->tail = tail;
        ob->count += n;
    }

    if (ob->count >= (uint32_t)HZ3_S221_OUTBOX_FLUSH_N) {
        hz3_s221_medium_flush_one(dst, sc);
    }
    return 1;
}

static inline void hz3_s221_medium_flush_pending(void) {
    for (uint16_t i = 0; i < t_s221_medium_touched_count; i++) {
        uint16_t key = t_s221_medium_touched_keys[i];
        uint8_t dst = (uint8_t)(key / HZ3_NUM_SC);
        int sc = (int)(key % HZ3_NUM_SC);
        hz3_s221_medium_flush_one(dst, sc);
    }
    t_s221_medium_touched_count = 0;
}
#else
static inline int hz3_s221_medium_dispatch_try_push(uint8_t dst, int sc,
                                                    void* head, void* tail, uint32_t n) {
    (void)dst;
    (void)sc;
    (void)head;
    (void)tail;
    (void)n;
    return 0;
}

static inline void hz3_s221_medium_flush_pending(void) {
}
#endif

#if HZ3_S224_MEDIUM_N1_PAIR_BATCH
typedef struct {
    void* pending;
} Hz3S224MediumPairSlot;

static HZ3_TLS Hz3S224MediumPairSlot t_s224_medium_pair_slot[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static HZ3_TLS uint8_t t_s224_medium_touched_flag[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static HZ3_TLS uint16_t t_s224_medium_touched_keys[HZ3_NUM_SHARDS * HZ3_NUM_SC];
static HZ3_TLS uint16_t t_s224_medium_touched_count;

static inline void hz3_s224_medium_flush_one(uint8_t dst, int sc) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS) {
        return;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return;
    }
    Hz3S224MediumPairSlot* slot = &t_s224_medium_pair_slot[dst][sc];
    void* pending = slot->pending;
    if (!pending) {
        t_s224_medium_touched_flag[dst][sc] = 0;
        return;
    }
    slot->pending = NULL;
    t_s224_medium_touched_flag[dst][sc] = 0;
    hz3_obj_set_next(pending, NULL);
    hz3_inbox_push_list(dst, sc, pending, pending, 1);
}

static inline void hz3_s224_medium_touch(uint8_t dst, int sc) {
    if (t_s224_medium_touched_flag[dst][sc]) {
        return;
    }
    if (t_s224_medium_touched_count >= (uint16_t)(HZ3_NUM_SHARDS * HZ3_NUM_SC)) {
        for (uint16_t i = 0; i < t_s224_medium_touched_count; i++) {
            uint16_t key = t_s224_medium_touched_keys[i];
            uint8_t key_dst = (uint8_t)(key / HZ3_NUM_SC);
            int key_sc = (int)(key % HZ3_NUM_SC);
            hz3_s224_medium_flush_one(key_dst, key_sc);
        }
        t_s224_medium_touched_count = 0;
    }
    t_s224_medium_touched_flag[dst][sc] = 1;
    t_s224_medium_touched_keys[t_s224_medium_touched_count++] =
        (uint16_t)((uint16_t)dst * (uint16_t)HZ3_NUM_SC + (uint16_t)sc);
}

static inline int hz3_s224_medium_dispatch_try_pair_n1(uint8_t dst, int sc, void* ptr) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC ||
        !ptr) {
        return 0;
    }
    if (sc < HZ3_S224_SC_MIN || sc > HZ3_S224_SC_MAX) {
        return 0;
    }
#if HZ3_S224_SKIP_SELF_DST
    if (dst == t_hz3_cache.my_shard) {
        return 0;
    }
#endif

    Hz3S224MediumPairSlot* slot = &t_s224_medium_pair_slot[dst][sc];
    if (!slot->pending) {
        hz3_s224_medium_touch(dst, sc);
        slot->pending = ptr;
        return 1;
    }

    void* head = slot->pending;
    slot->pending = NULL;
    t_s224_medium_touched_flag[dst][sc] = 0;
    hz3_obj_set_next(head, ptr);
    hz3_obj_set_next(ptr, NULL);
    hz3_inbox_push_list(dst, sc, head, ptr, 2);
    return 1;
}

static inline void hz3_s224_medium_flush_pending(void) {
    for (uint16_t i = 0; i < t_s224_medium_touched_count; i++) {
        uint16_t key = t_s224_medium_touched_keys[i];
        uint8_t dst = (uint8_t)(key / HZ3_NUM_SC);
        int sc = (int)(key % HZ3_NUM_SC);
        hz3_s224_medium_flush_one(dst, sc);
    }
    t_s224_medium_touched_count = 0;
}
#else
static inline int hz3_s224_medium_dispatch_try_pair_n1(uint8_t dst, int sc, void* ptr) {
    (void)dst;
    (void)sc;
    (void)ptr;
    return 0;
}

static inline void hz3_s224_medium_flush_one(uint8_t dst, int sc) {
    (void)dst;
    (void)sc;
}

static inline void hz3_s224_medium_flush_pending(void) {
}
#endif

#if HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
typedef struct {
    void* pending;
    uint8_t dst;
    uint8_t sc;
    uint8_t has_pending;
} Hz3S225MediumLastKeyState;

static HZ3_TLS Hz3S225MediumLastKeyState t_s225_medium_lastkey;

static inline int hz3_s225_medium_key_allowed(uint8_t dst, int sc) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    if (sc < HZ3_S225_SC_MIN || sc > HZ3_S225_SC_MAX) {
        return 0;
    }
#if HZ3_S225_SKIP_SELF_DST
    if (dst == t_hz3_cache.my_shard) {
        return 0;
    }
#endif
    return 1;
}

static inline void hz3_s225_medium_flush_pending(void) {
    if (!t_s225_medium_lastkey.has_pending || !t_s225_medium_lastkey.pending) {
        t_s225_medium_lastkey.has_pending = 0;
        t_s225_medium_lastkey.pending = NULL;
        return;
    }

    void* pending = t_s225_medium_lastkey.pending;
    uint8_t dst = t_s225_medium_lastkey.dst;
    int sc = (int)t_s225_medium_lastkey.sc;

    t_s225_medium_lastkey.has_pending = 0;
    t_s225_medium_lastkey.pending = NULL;
    hz3_obj_set_next(pending, NULL);
    hz3_inbox_push_list(dst, sc, pending, pending, 1);
}

static inline int hz3_s225_medium_dispatch_try_pair_n1(uint8_t dst, int sc, void* ptr) {
    if (!ptr) {
        return 0;
    }
    if (!hz3_s225_medium_key_allowed(dst, sc)) {
        return 0;
    }

    if (!t_s225_medium_lastkey.has_pending || !t_s225_medium_lastkey.pending) {
        t_s225_medium_lastkey.pending = ptr;
        t_s225_medium_lastkey.dst = dst;
        t_s225_medium_lastkey.sc = (uint8_t)sc;
        t_s225_medium_lastkey.has_pending = 1;
        return 1;
    }

    if (t_s225_medium_lastkey.dst == dst &&
        t_s225_medium_lastkey.sc == (uint8_t)sc) {
        void* head = t_s225_medium_lastkey.pending;
        t_s225_medium_lastkey.has_pending = 0;
        t_s225_medium_lastkey.pending = NULL;
        hz3_obj_set_next(head, ptr);
        hz3_obj_set_next(ptr, NULL);
        hz3_inbox_push_list(dst, sc, head, ptr, 2);
        return 1;
    }

    hz3_s225_medium_flush_pending();
    t_s225_medium_lastkey.pending = ptr;
    t_s225_medium_lastkey.dst = dst;
    t_s225_medium_lastkey.sc = (uint8_t)sc;
    t_s225_medium_lastkey.has_pending = 1;
    return 1;
}
#else
static inline void hz3_s225_medium_flush_pending(void) {
}

static inline int hz3_s225_medium_dispatch_try_pair_n1(uint8_t dst, int sc, void* ptr) {
    (void)dst;
    (void)sc;
    (void)ptr;
    return 0;
}
#endif

#if HZ3_S206B_MEDIUM_DST_MIXED_BATCH
typedef struct {
    void* head;
    void* tail;
    uint32_t count;
} Hz3S206BMixedOutbox;

static HZ3_TLS Hz3S206BMixedOutbox t_s206b_mixed_outbox[HZ3_NUM_SHARDS];
static HZ3_TLS uint8_t t_s206b_mixed_touched_flag[HZ3_NUM_SHARDS];
static HZ3_TLS uint8_t t_s206b_mixed_touched_shards[HZ3_NUM_SHARDS];
static HZ3_TLS uint8_t t_s206b_mixed_touched_count;

static inline void hz3_s206b_medium_flush_one(uint8_t dst) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS) {
        return;
    }
    Hz3S206BMixedOutbox* ob = &t_s206b_mixed_outbox[dst];
    if (!ob->head || !ob->tail || ob->count == 0) {
        ob->head = NULL;
        ob->tail = NULL;
        ob->count = 0;
        t_s206b_mixed_touched_flag[dst] = 0;
        return;
    }
    hz3_obj_set_next(ob->tail, NULL);
    hz3_inbox_medium_mixed_push_list(dst, ob->head, ob->tail, ob->count);
    ob->head = NULL;
    ob->tail = NULL;
    ob->count = 0;
    t_s206b_mixed_touched_flag[dst] = 0;
}

static inline void hz3_s206b_medium_touch(uint8_t dst) {
    if (t_s206b_mixed_touched_flag[dst]) {
        return;
    }
    if (t_s206b_mixed_touched_count >= (uint8_t)HZ3_NUM_SHARDS) {
        for (uint8_t i = 0; i < t_s206b_mixed_touched_count; i++) {
            hz3_s206b_medium_flush_one(t_s206b_mixed_touched_shards[i]);
        }
        t_s206b_mixed_touched_count = 0;
    }
    t_s206b_mixed_touched_flag[dst] = 1;
    t_s206b_mixed_touched_shards[t_s206b_mixed_touched_count++] = dst;
}

static inline void hz3_s206b_medium_dispatch_push(uint8_t dst, void* head, void* tail, uint32_t n) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS || !head || !tail || n == 0) {
        return;
    }
    Hz3S206BMixedOutbox* ob = &t_s206b_mixed_outbox[dst];
    if (!ob->head) {
        hz3_s206b_medium_touch(dst);
        ob->head = head;
        ob->tail = tail;
        ob->count = n;
    } else {
        hz3_obj_set_next(ob->tail, head);
        ob->tail = tail;
        ob->count += n;
    }
    if (ob->count >= (uint32_t)HZ3_S206B_MIXED_OUTBOX_FLUSH_N) {
        hz3_s206b_medium_flush_one(dst);
    }
}

static inline void hz3_s206b_medium_flush_pending(void) {
    for (uint8_t i = 0; i < t_s206b_mixed_touched_count; i++) {
        hz3_s206b_medium_flush_one(t_s206b_mixed_touched_shards[i]);
    }
    t_s206b_mixed_touched_count = 0;
}
#else
static inline void hz3_s206b_medium_dispatch_push(uint8_t dst, void* head, void* tail, uint32_t n) {
    (void)dst;
    (void)head;
    (void)tail;
    (void)n;
}

static inline void hz3_s206b_medium_flush_pending(void) {
}
#endif

#if HZ3_S236_BATCHIT_LITE
typedef struct {
    uint16_t key;
    uint16_t count;
    void* head;
    void* tail;
} Hz3S236CBatchEntry;

static HZ3_TLS Hz3S236CBatchEntry t_s236c_batch_cache[HZ3_S236_BATCH_CACHE_ENTRIES];

static inline uint32_t hz3_s236c_batch_hash(uint16_t key) {
    return ((uint32_t)key * 2654435761u) % (uint32_t)HZ3_S236_BATCH_CACHE_ENTRIES;
}

static inline void hz3_s236c_batch_reset(Hz3S236CBatchEntry* ent) {
    ent->key = 0;
    ent->count = 0;
    ent->head = NULL;
    ent->tail = NULL;
}

static inline void hz3_s236c_batch_flush_entry(Hz3S236CBatchEntry* ent) {
    if (!ent || ent->key == 0 || ent->count == 0 || !ent->head || !ent->tail) {
        if (ent) {
            hz3_s236c_batch_reset(ent);
        }
        return;
    }

    uint16_t key = (uint16_t)(ent->key - 1u);
    uint8_t dst = (uint8_t)(key / (uint16_t)HZ3_NUM_SC);
    int sc = (int)(key % (uint16_t)HZ3_NUM_SC);

    hz3_obj_set_next(ent->tail, NULL);
    hz3_inbox_push_list(dst, sc, ent->head, ent->tail, ent->count);
    S203_STAT_INC(s236c_batch_flush_msgs);
    S203_STAT_ADD(s236c_batch_flush_objs, ent->count);
    hz3_s236c_batch_reset(ent);
}

static inline int hz3_s236c_medium_try_batch_n1(uint8_t dst, int sc, void* ptr) {
    if (!ptr || (uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS || (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    if (sc < HZ3_S236_SC_MIN || sc > HZ3_S236_SC_MAX) {
        return 0;
    }
    if (dst == t_hz3_cache.my_shard) {
        return 0;
    }

    S203_STAT_INC(s236c_batch_push_attempts);

#if HZ3_S236_PRESSURE_ENABLE
    if (hz3_inbox_count_approx(dst, sc) < (uint32_t)HZ3_S236_PRESSURE_INBOX_MIN) {
        S203_STAT_INC(s236c_pressure_bypass);
        return 0;
    }
#endif

    uint16_t key = (uint16_t)((uint16_t)dst * (uint16_t)HZ3_NUM_SC + (uint16_t)sc + 1u);
    uint32_t idx = hz3_s236c_batch_hash(key);
    Hz3S236CBatchEntry* ent = &t_s236c_batch_cache[idx];
    if (ent->key != key && ent->count > 0) {
        hz3_s236c_batch_flush_entry(ent);
    }

    if (ent->key == key && ent->count > 0) {
        S203_STAT_INC(s236c_batch_cache_hits);
        hz3_obj_set_next(ent->tail, ptr);
        ent->tail = ptr;
        ent->count = (uint16_t)(ent->count + 1u);
    } else {
        S203_STAT_INC(s236c_batch_cache_misses);
        ent->key = key;
        ent->count = 1;
        ent->head = ptr;
        ent->tail = ptr;
    }

    if ((uint32_t)ent->count >= (uint32_t)HZ3_S236_BATCH_FLUSH_N) {
        hz3_s236c_batch_flush_entry(ent);
    }
    return 1;
}

static inline void hz3_s236c_batch_flush_pending(void) {
    for (uint32_t i = 0; i < (uint32_t)HZ3_S236_BATCH_CACHE_ENTRIES; i++) {
        hz3_s236c_batch_flush_entry(&t_s236c_batch_cache[i]);
    }
}
#else
static inline int hz3_s236c_medium_try_batch_n1(uint8_t dst, int sc, void* ptr) {
    (void)dst;
    (void)sc;
    (void)ptr;
    return 0;
}

static inline void hz3_s236c_batch_flush_pending(void) {
}
#endif

static inline void hz3_medium_dispatch_push(uint8_t dst, int sc,
                                            void* head, void* tail, uint32_t n) {
#if HZ3_S220_CPU_RRQ
    if (n == 1 && head == tail && dst != t_hz3_cache.my_shard &&
        (uint32_t)sc < (uint32_t)HZ3_NUM_SC &&
        sc >= HZ3_S220_CPU_RRQ_SC_MIN &&
        sc <= HZ3_S220_CPU_RRQ_SC_MAX) {
        if (hz3_s220_cpu_rrq_try_push(sc, head)) {
            return;
        }
    }
#endif
#if HZ3_S236_BATCHIT_LITE
    if (n == 1 && head == tail) {
        if (hz3_s236c_medium_try_batch_n1(dst, sc, head)) {
            return;
        }
    } else {
        hz3_s236c_batch_flush_pending();
    }
#endif
#if HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
    if (n == 1 && head == tail) {
        if (hz3_s225_medium_dispatch_try_pair_n1(dst, sc, head)) {
            return;
        }
    } else {
        hz3_s225_medium_flush_pending();
    }
#endif
#if HZ3_S224_MEDIUM_N1_PAIR_BATCH
    if (n == 1 && head == tail) {
        if (hz3_s224_medium_dispatch_try_pair_n1(dst, sc, head)) {
            return;
        }
    } else {
        hz3_s224_medium_flush_one(dst, sc);
    }
#endif
#if HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
    if (hz3_s221_medium_dispatch_try_push(dst, sc, head, tail, n)) {
        return;
    }
#endif
#if HZ3_S206B_MEDIUM_DST_MIXED_BATCH
    (void)sc;
    hz3_s206b_medium_dispatch_push(dst, head, tail, n);
#else
    hz3_s199_medium_dispatch_push(dst, sc, head, tail, n);
#endif
}

static inline void hz3_medium_flush_pending(void) {
    hz3_s236c_batch_flush_pending();
    hz3_s225_medium_flush_pending();
    hz3_s224_medium_flush_pending();
    hz3_s221_medium_flush_pending();
    hz3_s199_medium_flush_pending();
#if HZ3_S206B_MEDIUM_DST_MIXED_BATCH
    hz3_s206b_medium_flush_pending();
#endif
}

#if HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
static _Thread_local uint32_t t_s205_last_drain_seq[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Thread_local uint16_t t_s205_pushes_since_drain[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static inline int hz3_s205_medium_should_rescue(uint8_t dst, int sc, uint32_t n) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS || (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    if (n < (uint32_t)HZ3_S205_MIN_N) {
        return 0;
    }

    uint32_t drain_seq = hz3_inbox_drain_seq_load(dst, sc);
    uint32_t* last_seq = &t_s205_last_drain_seq[dst][sc];
    uint16_t* stale_pushes = &t_s205_pushes_since_drain[dst][sc];
    if (drain_seq != *last_seq) {
        *last_seq = drain_seq;
        *stale_pushes = 0;
    }

    uint32_t add = (n > 0xFFFFu) ? 0xFFFFu : n;
    uint32_t next = (uint32_t)(*stale_pushes) + add;
    *stale_pushes = (uint16_t)(next > 0xFFFFu ? 0xFFFFu : next);

    if ((uint32_t)(*stale_pushes) < (uint32_t)HZ3_S205_STALE_PUSHES) {
        return 0;
    }

    if (hz3_inbox_count_approx(dst, sc) < (uint32_t)HZ3_S205_BACKLOG_OBJS) {
        return 0;
    }

    *stale_pushes = 0;
    return 1;
}
#else
static inline int hz3_s205_medium_should_rescue(uint8_t dst, int sc, uint32_t n) {
    (void)dst;
    (void)sc;
    (void)n;
    return 0;
}
#endif

#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG
static _Thread_local uint32_t t_s209_last_drain_seq[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Thread_local uint16_t t_s209_pushes_since_drain[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Thread_local uint32_t t_s209_last_miss_seq_seen[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static inline int hz3_s209_medium_should_rescue(uint8_t dst, int sc, uint32_t n) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS || (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    if (sc < HZ3_S209_SC_MIN || sc > HZ3_S209_SC_MAX) {
        return 0;
    }
    if (n < (uint32_t)HZ3_S209_MIN_N) {
        return 0;
    }

    uint32_t drain_seq = hz3_inbox_drain_seq_load(dst, sc);
    uint32_t* last_drain_seq = &t_s209_last_drain_seq[dst][sc];
    uint16_t* stale_pushes = &t_s209_pushes_since_drain[dst][sc];
    if (drain_seq != *last_drain_seq) {
        *last_drain_seq = drain_seq;
        *stale_pushes = 0;
    }

    uint32_t add = (n > 0xFFFFu) ? 0xFFFFu : n;
    uint32_t next = (uint32_t)(*stale_pushes) + add;
    *stale_pushes = (uint16_t)(next > 0xFFFFu ? 0xFFFFu : next);

    if ((uint32_t)(*stale_pushes) < (uint32_t)HZ3_S209_STALE_PUSHES) {
        return 0;
    }
    if (hz3_inbox_count_approx(dst, sc) < (uint32_t)HZ3_S209_BACKLOG_OBJS) {
        return 0;
    }

    uint32_t miss_seq = hz3_s209_medium_miss_seq_load(dst, sc);
    uint32_t* seen_miss_seq = &t_s209_last_miss_seq_seen[dst][sc];
    if (miss_seq == 0 || miss_seq == *seen_miss_seq) {
        return 0;
    }

    *seen_miss_seq = miss_seq;
    *stale_pushes = 0;
    return 1;
}
#else
static inline int hz3_s209_medium_should_rescue(uint8_t dst, int sc, uint32_t n) {
    (void)dst;
    (void)sc;
    (void)n;
    return 0;
}
#endif

#if HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
static _Atomic uint32_t g_s210_last_rescue_miss_seq[HZ3_NUM_SHARDS][HZ3_NUM_SC];

static inline int hz3_s210_medium_should_rescue(uint8_t dst, int sc, uint32_t n) {
    if ((uint32_t)dst >= (uint32_t)HZ3_NUM_SHARDS || (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    if (sc < HZ3_S210_SC_MIN || sc > HZ3_S210_SC_MAX) {
        return 0;
    }
    if (n < (uint32_t)HZ3_S210_MIN_N) {
        return 0;
    }
    if (hz3_inbox_count_approx(dst, sc) < (uint32_t)HZ3_S210_BACKLOG_OBJS) {
        return 0;
    }

    uint32_t miss_seq = hz3_s209_medium_miss_seq_load(dst, sc);
    if (miss_seq == 0) {
        return 0;
    }

    _Atomic uint32_t* last_seq_cell = &g_s210_last_rescue_miss_seq[dst][sc];
    uint32_t seen = atomic_load_explicit(last_seq_cell, memory_order_relaxed);
    for (;;) {
        if (miss_seq <= seen) {
            return 0;
        }
        if ((uint32_t)(miss_seq - seen) < (uint32_t)HZ3_S210_MISS_STRIDE) {
            return 0;
        }
        if (atomic_compare_exchange_weak_explicit(last_seq_cell, &seen, miss_seq,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return 1;
        }
    }
}
#else
static inline int hz3_s210_medium_should_rescue(uint8_t dst, int sc, uint32_t n) {
    (void)dst;
    (void)sc;
    (void)n;
    return 0;
}
#endif

static inline void hz3_remote_stash_dispatch_list(uint8_t dst, uint32_t bin,
                                                 void* head, void* tail, uint32_t n) {
    if (!head || !tail || n == 0) {
        return;
    }
    S196_STAT_REGISTER();

    if (bin < HZ3_SMALL_NUM_SC) {
        S196_STAT_INC(g_s196_dispatch_small_calls);
        S196_STAT_ADD(g_s196_dispatch_small_objs, n);
#if HZ3_HZ4_BRIDGE
        hz3_hz4_bridge_push_remote_list_small(dst, (int)bin, head, tail, n);
#elif HZ3_LANE_T16_R90_PAGE_REMOTE
        hz3_lane16_push_remote_list_small(dst, (int)bin, head, tail, n);
#else
        hz3_small_v2_push_remote_list(dst, (int)bin, head, tail, n);
#endif
    } else if (bin < HZ3_MEDIUM_BIN_BASE) {
        S196_STAT_INC(g_s196_dispatch_sub4k_calls);
        S196_STAT_ADD(g_s196_dispatch_sub4k_objs, n);
        int sc = (int)bin - HZ3_SUB4K_BIN_BASE;
        hz3_sub4k_push_remote_list(dst, sc, head, tail, n);
    } else {
        int sc = (int)bin - HZ3_MEDIUM_BIN_BASE;
        int to_central = 0;
        int to_mailbox_s236 = 0;
        int to_central_s209 = 0;
        int to_central_s210 = 0;
        int to_central_s230 = 0;
        S196_STAT_INC(g_s196_dispatch_medium_calls);
        S196_STAT_ADD(g_s196_dispatch_medium_objs, n);
        S196_MEDIUM_SC_ADD(sc, n);

#if HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
        if (hz3_s205_medium_should_rescue(dst, sc, n)) {
            to_central = 1;
        }
#endif
#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG
        if (!to_central && hz3_s209_medium_should_rescue(dst, sc, n)) {
            to_central = 1;
            to_central_s209 = 1;
        }
#endif
#if HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
        if (!to_central && hz3_s210_medium_should_rescue(dst, sc, n)) {
            to_central = 1;
            to_central_s210 = 1;
        }
#endif
#if HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
        if (!to_central && sc >= HZ3_S230_SC_MIN && sc <= HZ3_S230_SC_MAX) {
#if HZ3_S230_SKIP_SELF_DST
            if (dst != t_hz3_cache.my_shard)
#endif
            {
#if HZ3_S230_ONLY_N_EQ_1
                if (n == 1 && head == tail)
#endif
                {
                    to_central = 1;
                    to_central_s230 = 1;
                }
            }
        }
#endif
#if HZ3_S236_MEDIUM_MAILBOX
        if (!to_central && !to_mailbox_s236 &&
            sc >= HZ3_S236_SC_MIN && sc <= HZ3_S236_SC_MAX) {
#if HZ3_S236_SKIP_SELF_DST
            if (dst != t_hz3_cache.my_shard)
#endif
            {
#if HZ3_S236_ONLY_N_EQ_1
                if (n == 1 && head == tail)
#endif
                {
                    if (hz3_s236_medium_mailbox_try_push(dst, sc, head)) {
                        to_mailbox_s236 = 1;
                    }
                }
            }
        }
#endif
#if HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL
        if (!to_central && n >= (uint32_t)HZ3_S202_MEDIUM_TO_CENTRAL_MIN_N) {
            to_central = 1;
        }
#endif

#if HZ3_S203_COUNTERS
        S203_STAT_INC(medium_dispatch_calls);
        S203_STAT_ADD(medium_dispatch_objs, n);
        t_s203_medium_dispatch_sc_calls[sc]++;
        t_s203_medium_dispatch_sc_objs[sc] += n;
        S204_SHARD_INC(dst);
        if (n == 1) S203_STAT_INC(medium_dispatch_n_hist_1);
        else if (n <= 3) S203_STAT_INC(medium_dispatch_n_hist_2_3);
        else if (n <= 7) S203_STAT_INC(medium_dispatch_n_hist_4_7);
        else if (n <= 15) S203_STAT_INC(medium_dispatch_n_hist_8_15);
        else S203_STAT_INC(medium_dispatch_n_hist_16_plus);

        if (to_mailbox_s236) {
            S203_STAT_INC(medium_dispatch_to_mailbox_calls);
            S203_STAT_ADD(medium_dispatch_to_mailbox_objs, n);
        } else if (to_central) {
            S203_STAT_INC(medium_dispatch_to_central_calls);
            S203_STAT_ADD(medium_dispatch_to_central_objs, n);
            if (to_central_s209) {
                S203_STAT_INC(medium_dispatch_to_central_s209_calls);
                S203_STAT_ADD(medium_dispatch_to_central_s209_objs, n);
            }
            if (to_central_s210) {
                S203_STAT_INC(medium_dispatch_to_central_s210_calls);
                S203_STAT_ADD(medium_dispatch_to_central_s210_objs, n);
            }
            if (to_central_s230) {
                S203_STAT_INC(medium_dispatch_to_central_s230_calls);
                S203_STAT_ADD(medium_dispatch_to_central_s230_objs, n);
            }
        } else {
            S203_STAT_INC(medium_dispatch_to_inbox_calls);
            S203_STAT_ADD(medium_dispatch_to_inbox_objs, n);
        }
#endif
        (void)to_central_s209;
        (void)to_central_s210;
        (void)to_central_s230;
        (void)to_mailbox_s236;

        if (to_mailbox_s236) {
            return;
        }
        if (to_central) {
            hz3_central_push_list((int)dst, sc, head, tail, n);
            return;
        }
        hz3_medium_dispatch_push(dst, sc, head, tail, n);
    }
}

static inline int hz3_remote_stash_try_direct_n1(uint8_t dst, uint32_t bin, void* ptr) {
#if HZ3_HZ4_BRIDGE
    if (bin < HZ3_SMALL_NUM_SC) {
        S196_STAT_REGISTER();
        S196_STAT_INC(g_s196_direct_n1_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_objs);
        hz3_hz4_bridge_push_remote_list_small(dst, (int)bin, ptr, ptr, 1);
        return 1;
    }
#elif HZ3_LANE_T16_R90_PAGE_REMOTE
    if (bin < HZ3_SMALL_NUM_SC) {
        S196_STAT_REGISTER();
        S196_STAT_INC(g_s196_direct_n1_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_calls);
        S196_STAT_INC(g_s196_dispatch_small_objs);
        hz3_lane16_push_remote_list_small(dst, (int)bin, ptr, ptr, 1);
        return 1;
    }
#endif
#if HZ3_S161_REMOTE_STASH_N1_DIRECT && HZ3_S44_OWNER_STASH_PUSH && !HZ3_S44_OWNER_STASH_DISABLE
    if (bin < HZ3_SMALL_NUM_SC) {
        if (hz3_owner_stash_push_one(dst, (int)bin, ptr)) {
            S196_STAT_REGISTER();
            S196_STAT_INC(g_s196_direct_n1_small_calls);
            S196_STAT_INC(g_s196_dispatch_small_calls);
            S196_STAT_INC(g_s196_dispatch_small_objs);
            return 1;
        }
    }
#else
    (void)dst;
    (void)bin;
    (void)ptr;
#endif
    return 0;
}

static inline uint16_t hz3_remote_stash_entry_key_load(const Hz3RemoteStashEntry* entry) {
#if HZ3_RBUF_KEY
    return entry->key;
#else
    return (uint16_t)(((uint16_t)entry->dst << 8) | (uint16_t)entry->bin);
#endif
}

static inline int hz3_remote_stash_n1_direct_allowed(uint16_t h, uint16_t t) {
#if HZ3_S178_N1_DIRECT_RING_GUARD
    uint32_t remaining = (uint32_t)((h - t) & (HZ3_REMOTE_STASH_RING_SIZE - 1));
    return remaining <= (uint32_t)HZ3_S178_N1_DIRECT_MAX_RING;
#else
    (void)h;
    (void)t;
    return 1;
#endif
}

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
#endif  // HZ3_REMOTE_STASH_SPARSE
