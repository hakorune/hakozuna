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
