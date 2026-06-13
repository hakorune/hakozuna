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
