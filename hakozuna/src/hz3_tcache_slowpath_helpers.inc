static inline void* hz3_inbox_drain_medium_slow(uint8_t shard, int sc, Hz3Bin* bin) {
#if HZ3_S200_INBOX_SEQ_GATE
    return hz3_inbox_drain_if_seq_advanced(shard, sc, bin,
                                           &t_hz3_s200_inbox_seq_seen[sc]);
#else
    return hz3_inbox_drain_if_nonempty(shard, sc, bin);
#endif
}

static inline void* hz3_inbox_drain_medium_from_shard_slow(uint8_t shard, int sc) {
#if HZ3_TCACHE_SOA_LOCAL
    uint32_t bin_idx = hz3_bin_index_medium(sc);
    Hz3BinRef binref = hz3_tcache_get_local_binref(bin_idx);
#if HZ3_BIN_SPLIT_COUNT
    Hz3Bin tmp_bin = { .head = hz3_split_ptr(*binref.head), .count = *binref.count };
    void* obj = hz3_inbox_drain_medium_slow(shard, sc, &tmp_bin);
    uint32_t total_count = ((uint32_t)tmp_bin.count << HZ3_SPLIT_CNT_SHIFT);
    uint32_t new_low = total_count & (uint32_t)HZ3_SPLIT_CNT_MASK;
    *binref.count = (Hz3BinCount)(total_count >> HZ3_SPLIT_CNT_SHIFT);
    *binref.head = hz3_split_pack(tmp_bin.head, new_low);
#else
    Hz3Bin tmp_bin = { .head = *binref.head, .count = *binref.count };
    void* obj = hz3_inbox_drain_medium_slow(shard, sc, &tmp_bin);
    *binref.head = tmp_bin.head;
#if !HZ3_BIN_LAZY_COUNT
    *binref.count = tmp_bin.count;
#endif
#endif  // HZ3_BIN_SPLIT_COUNT
    return obj;
#else
    Hz3Bin* bin = hz3_tcache_get_bin(sc);
    return hz3_inbox_drain_medium_slow(shard, sc, bin);
#endif
}

static inline void* hz3_inbox_drain_medium_mixed_from_shard_slow(uint8_t shard, int sc) {
#if !HZ3_S206B_MEDIUM_DST_MIXED_BATCH
    (void)shard;
    (void)sc;
    return NULL;
#elif HZ3_TCACHE_SOA_LOCAL
    uint32_t bin_idx = hz3_bin_index_medium(sc);
    Hz3BinRef binref = hz3_tcache_get_local_binref(bin_idx);
#if HZ3_BIN_SPLIT_COUNT
    Hz3Bin tmp_bin = { .head = hz3_split_ptr(*binref.head), .count = *binref.count };
    void* obj = hz3_inbox_medium_mixed_drain(shard, sc, &tmp_bin,
                                             (uint32_t)HZ3_S206B_MIXED_DRAIN_BUDGET);
    uint32_t total_count = ((uint32_t)tmp_bin.count << HZ3_SPLIT_CNT_SHIFT);
    uint32_t new_low = total_count & (uint32_t)HZ3_SPLIT_CNT_MASK;
    *binref.count = (Hz3BinCount)(total_count >> HZ3_SPLIT_CNT_SHIFT);
    *binref.head = hz3_split_pack(tmp_bin.head, new_low);
#else
    Hz3Bin tmp_bin = { .head = *binref.head, .count = *binref.count };
    void* obj = hz3_inbox_medium_mixed_drain(shard, sc, &tmp_bin,
                                             (uint32_t)HZ3_S206B_MIXED_DRAIN_BUDGET);
    *binref.head = tmp_bin.head;
#if !HZ3_BIN_LAZY_COUNT
    *binref.count = tmp_bin.count;
#endif
#endif
    return obj;
#else
    Hz3Bin* bin = hz3_tcache_get_bin(sc);
    return hz3_inbox_medium_mixed_drain(shard, sc, bin,
                                        (uint32_t)HZ3_S206B_MIXED_DRAIN_BUDGET);
#endif
}

#if HZ3_S206_STEAL_ON_MISS
static _Thread_local uint8_t t_s206_steal_cursor = 0;
#endif

#if HZ3_S208_MEDIUM_CENTRAL_RESERVE
static _Thread_local uint16_t t_s208_miss_streak[HZ3_NUM_SC];
static _Thread_local uint8_t t_s208_recent_reserved[HZ3_NUM_SC];
#define HZ3_S208_RESET_STATE(sc_) do {              \
    t_s208_miss_streak[(uint32_t)(sc_)] = 0;        \
    t_s208_recent_reserved[(uint32_t)(sc_)] = 0;    \
} while (0)
#else
#define HZ3_S208_RESET_STATE(sc_) ((void)0)
#endif

#if HZ3_S223_DYNAMIC_WANT
static _Thread_local uint16_t t_s223_miss_streak[HZ3_NUM_SC];

static inline int hz3_s223_sc_in_range(int sc) {
    return (sc >= HZ3_S223_SC_MIN && sc <= HZ3_S223_SC_MAX);
}

static inline int hz3_s223_effective_want(int sc, int want) {
    if (!hz3_s223_sc_in_range(sc)) {
        return want;
    }
    uint32_t idx = (uint32_t)sc;
    if (t_s223_miss_streak[idx] < (uint16_t)HZ3_S223_MISS_STREAK_MIN) {
        return want;
    }
    int boosted = want + HZ3_S223_WANT_BONUS;
    if (boosted > HZ3_S223_WANT_CAP) {
        boosted = HZ3_S223_WANT_CAP;
    }
    if (boosted > 16) {
        boosted = 16;
    }
    if (boosted < 1) {
        boosted = 1;
    }
    if (boosted > want) {
        S203_ALLOC_INC(s223_want_boost_calls);
    }
    return boosted;
}

#define HZ3_S223_NOTE_CENTRAL_MISS(sc_) do {                             \
    uint32_t s223_idx = (uint32_t)(sc_);                                 \
    if (s223_idx < (uint32_t)HZ3_NUM_SC &&                               \
        t_s223_miss_streak[s223_idx] < UINT16_MAX) {                     \
        t_s223_miss_streak[s223_idx]++;                                  \
    }                                                                     \
} while (0)

#define HZ3_S223_RESET_STREAK(sc_) do {                                  \
    uint32_t s223_idx = (uint32_t)(sc_);                                 \
    if (s223_idx < (uint32_t)HZ3_NUM_SC) {                               \
        t_s223_miss_streak[s223_idx] = 0;                                \
    }                                                                     \
} while (0)
#else
#define HZ3_S223_NOTE_CENTRAL_MISS(sc_) ((void)0)
#define HZ3_S223_RESET_STREAK(sc_) ((void)0)
static inline int hz3_s223_effective_want(int sc, int want) {
    (void)sc;
    return want;
}
#endif

static inline int hz3_s229_sc_in_range(int sc) {
#if HZ3_S229_CENTRAL_FIRST
    return (sc >= HZ3_S229_SC_MIN && sc <= HZ3_S229_SC_MAX);
#else
    (void)sc;
    return 0;
#endif
}

#if HZ3_S236F_MINI_CENTRAL_RETRY
static _Thread_local uint16_t t_s236f_retry_miss_streak[HZ3_NUM_SC];

static inline void hz3_s236f_reset_retry_streak(int sc) {
    uint32_t idx = (uint32_t)sc;
    if (idx < (uint32_t)HZ3_NUM_SC) {
        t_s236f_retry_miss_streak[idx] = 0;
    }
}

static inline void hz3_s236f_note_retry_miss(int sc) {
    uint32_t idx = (uint32_t)sc;
    if (idx < (uint32_t)HZ3_NUM_SC &&
        t_s236f_retry_miss_streak[idx] < UINT16_MAX) {
        t_s236f_retry_miss_streak[idx]++;
    }
}

static inline int hz3_s236f_should_retry(int sc) {
    uint32_t idx = (uint32_t)sc;
    if (idx >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    uint16_t streak = t_s236f_retry_miss_streak[idx];
    if (streak < (uint16_t)HZ3_S236F_MISS_STREAK_MIN) {
        return 0;
    }
    uint16_t over = (uint16_t)(streak - (uint16_t)HZ3_S236F_MISS_STREAK_MIN);
    return (over % (uint16_t)HZ3_S236F_RETRY_EVERY) == 0;
}
#else
static inline void hz3_s236f_reset_retry_streak(int sc) {
    (void)sc;
}

static inline void hz3_s236f_note_retry_miss(int sc) {
    (void)sc;
}

static inline int hz3_s236f_should_retry(int sc) {
    (void)sc;
    return 0;
}
#endif

void* hz3_s236_minirefill_try(int sc) {
#if !HZ3_S236_MINIREFILL
    (void)sc;
    return NULL;
#else
    void* obj = NULL;

    if (sc < HZ3_S236_SC_MIN || sc > HZ3_S236_SC_MAX) {
        return NULL;
    }
    S203_ALLOC_INC(s236_mini_calls);

#if HZ3_S236_MINIREFILL_TRY_INBOX
    obj = hz3_inbox_drain_medium_from_shard_slow(t_hz3_cache.my_shard, sc);
    if (obj) {
        hz3_s236f_reset_retry_streak(sc);
        S203_ALLOC_INC(s236_mini_hit_inbox);
        return obj;
    }
#endif

#if HZ3_S236_MINIREFILL_TRY_CENTRAL_FAST
    int do_central_try = 1;
#if HZ3_S236G_MINI_CENTRAL_HINT_GATE
    S203_ALLOC_INC(s236g_hint_checks);
    if (!hz3_central_has_supply(t_hz3_cache.my_shard, sc)) {
        S203_ALLOC_INC(s236g_hint_empty_skips);
        do_central_try = 0;
    } else {
        S203_ALLOC_INC(s236g_hint_positive);
    }
#endif

    if (do_central_try) {
        int want = HZ3_REFILL_BATCH[sc];
        if (want > HZ3_S236_MINIREFILL_K) {
            want = HZ3_S236_MINIREFILL_K;
        }
        if (want > 16) {
            want = 16;
        }
        if (want < 1) {
            want = 1;
        }

        void* batch[16];
        int got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
        if (got > 0) {
#if HZ3_TCACHE_SOA_LOCAL
            Hz3BinRef binref = hz3_tcache_get_local_binref(hz3_bin_index_medium(sc));
            for (int i = 1; i < got; i++) {
                hz3_binref_push(binref, batch[i]);
            }
#else
            Hz3Bin* bin = hz3_tcache_get_bin(sc);
            for (int i = 1; i < got; i++) {
                hz3_bin_push(bin, batch[i]);
            }
#endif
            t_hz3_cache.stats.central_pop_hit[sc]++;
            hz3_s236f_reset_retry_streak(sc);
            S203_ALLOC_INC(s236_mini_hit_central);
            return batch[0];
        }

        hz3_s236f_note_retry_miss(sc);

#if HZ3_S236E_MINI_CENTRAL_RETRY
        S203_ALLOC_INC(s236e_retry_calls);
        int do_retry = 1;
#if HZ3_S236E_MINI_CENTRAL_RETRY_REQUIRE_SUPPLY_HINT
        do_retry = hz3_central_has_supply(t_hz3_cache.my_shard, sc);
#endif
        if (do_retry) {
            int retry_want = HZ3_S236E_MINI_CENTRAL_RETRY_WANT;
            if (retry_want > want) {
                retry_want = want;
            }
            if (retry_want > 16) {
                retry_want = 16;
            }
            if (retry_want < 1) {
                retry_want = 1;
            }

            int retry_got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, retry_want);
            if (retry_got > 0) {
#if HZ3_TCACHE_SOA_LOCAL
                Hz3BinRef binref = hz3_tcache_get_local_binref(hz3_bin_index_medium(sc));
                for (int i = 1; i < retry_got; i++) {
                    hz3_binref_push(binref, batch[i]);
                }
#else
                Hz3Bin* bin = hz3_tcache_get_bin(sc);
                for (int i = 1; i < retry_got; i++) {
                    hz3_bin_push(bin, batch[i]);
                }
#endif
                t_hz3_cache.stats.central_pop_hit[sc]++;
                hz3_s236f_reset_retry_streak(sc);
                S203_ALLOC_INC(s236_mini_hit_central);
                S203_ALLOC_INC(s236e_retry_hits);
                return batch[0];
            }
        } else {
            S203_ALLOC_INC(s236e_retry_skipped_no_supply);
        }
#endif

#if HZ3_S236F_MINI_CENTRAL_RETRY
        if (hz3_s236f_should_retry(sc)) {
            int retry_want = HZ3_S236F_MINI_CENTRAL_RETRY_WANT;
            if (retry_want > want) {
                retry_want = want;
            }
            if (retry_want > 16) {
                retry_want = 16;
            }
            if (retry_want < 1) {
                retry_want = 1;
            }

            S203_ALLOC_INC(s236f_retry_calls);
            int retry_got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, retry_want);
            if (retry_got > 0) {
#if HZ3_TCACHE_SOA_LOCAL
                Hz3BinRef binref = hz3_tcache_get_local_binref(hz3_bin_index_medium(sc));
                for (int i = 1; i < retry_got; i++) {
                    hz3_binref_push(binref, batch[i]);
                }
#else
                Hz3Bin* bin = hz3_tcache_get_bin(sc);
                for (int i = 1; i < retry_got; i++) {
                    hz3_bin_push(bin, batch[i]);
                }
#endif
                t_hz3_cache.stats.central_pop_hit[sc]++;
                hz3_s236f_reset_retry_streak(sc);
                S203_ALLOC_INC(s236_mini_hit_central);
                S203_ALLOC_INC(s236f_retry_hits);
                return batch[0];
            }
        } else {
            S203_ALLOC_INC(s236f_retry_skipped_streak);
        }
#endif
    } else {
        hz3_s236f_note_retry_miss(sc);
    }
#endif

#if HZ3_S236I_MINI_INBOX_SECOND_CHANCE
    int s236i_try = 1;
#if HZ3_S236I_REQUIRE_REMOTE_HINT
    if (!t_hz3_cache.remote_hint) {
        s236i_try = 0;
        S203_ALLOC_INC(s236i_second_inbox_skip_no_hint);
    }
#endif
    if (s236i_try) {
        uint32_t s236i_backlog = hz3_inbox_count_approx(t_hz3_cache.my_shard, sc);
        if (s236i_backlog < (uint32_t)HZ3_S236I_INBOX_BACKLOG_MIN) {
            s236i_try = 0;
            S203_ALLOC_INC(s236i_second_inbox_skip_no_backlog);
        }
    }
    if (s236i_try) {
        S203_ALLOC_INC(s236i_second_inbox_calls);
        obj = hz3_inbox_drain_medium_from_shard_slow(t_hz3_cache.my_shard, sc);
        if (obj) {
            hz3_s236f_reset_retry_streak(sc);
            S203_ALLOC_INC(s236_mini_hit_inbox);
            S203_ALLOC_INC(s236i_second_inbox_hits);
            return obj;
        }
    }
#endif

    S203_ALLOC_INC(s236_mini_miss);
    return NULL;
#endif
}

static inline void* hz3_s206_medium_try_steal(int sc) {
#if HZ3_S206_STEAL_ON_MISS
    if (HZ3_NUM_SHARDS <= 1) {
        return NULL;
    }
    uint8_t my_shard = t_hz3_cache.my_shard;
    for (uint32_t i = 0; i < (uint32_t)HZ3_S206_STEAL_BUDGET_SHARDS; i++) {
        uint8_t victim = (uint8_t)(((uint32_t)t_s206_steal_cursor + 1u) % (uint32_t)HZ3_NUM_SHARDS);
        t_s206_steal_cursor = victim;
        if (victim == my_shard) {
            continue;
        }
        if (hz3_inbox_count_approx(victim, sc) < (uint32_t)HZ3_S206_STEAL_BACKLOG_OBJS) {
            continue;
        }
        S203_ALLOC_INC(s206_steal_attempts);
        void* obj = hz3_inbox_drain_medium_from_shard_slow(victim, sc);
        if (obj) {
            S203_ALLOC_INC(s206_steal_hits);
            return obj;
        }
    }
#else
    (void)sc;
#endif
    return NULL;
}

#if HZ3_S236N_ALLOC_SLOW_BULK_REFILL
static inline int hz3_s236n_sc_in_range(int sc) {
    return (sc >= HZ3_S236N_SC_MIN && sc <= HZ3_S236N_SC_MAX);
}

static inline uint32_t hz3_s236n_bulk_refill_inbox(int sc,
#if HZ3_TCACHE_SOA_LOCAL
                                                    Hz3BinRef binref
#else
                                                    Hz3Bin* bin
#endif
) {
    if (!hz3_s236n_sc_in_range(sc)) {
        return 0;
    }
    uint32_t pulled = 0;
    uint32_t limit = (uint32_t)(HZ3_S236N_BULK_WANT - 1);
    S203_ALLOC_INC(s236n_bulk_calls);
    for (uint32_t i = 0; i < limit; i++) {
        void* extra = hz3_inbox_drain_medium_from_shard_slow(t_hz3_cache.my_shard, sc);
        if (!extra) {
            break;
        }
#if HZ3_S72_MEDIUM_DEBUG
        hz3_medium_boundary_check_ptr("medium_alloc_s236n_inbox_fill", extra, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("medium_alloc_s236n_inbox_fill", extra, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(binref, extra);
#else
        hz3_bin_push(bin, extra);
#endif
        pulled++;
    }
    if (pulled == 0) {
        S203_ALLOC_INC(s236n_bulk_zero_extra);
    } else {
        S203_ALLOC_ADD(s236n_bulk_inbox_extra_objs, pulled);
    }
    return pulled;
}

static inline uint32_t hz3_s236n_bulk_refill_central(int sc, int cap,
#if HZ3_TCACHE_SOA_LOCAL
                                                      Hz3BinRef binref
#else
                                                      Hz3Bin* bin
#endif
) {
    if (!hz3_s236n_sc_in_range(sc) || cap < 1) {
        return 0;
    }
    int want = cap;
    if (want > (HZ3_S236N_BULK_WANT - 1)) {
        want = HZ3_S236N_BULK_WANT - 1;
    }
    if (want < 1) {
        return 0;
    }
    if (want > 16) {
        want = 16;
    }
    void* batch[16];
    S203_ALLOC_INC(s236n_bulk_calls);
    int got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
    if (got <= 0) {
        S203_ALLOC_INC(s236n_bulk_zero_extra);
        return 0;
    }
    for (int i = 0; i < got; i++) {
#if HZ3_S72_MEDIUM_DEBUG
        hz3_medium_boundary_check_ptr("medium_alloc_s236n_central_fill", batch[i], sc, t_hz3_cache.my_shard);
#endif
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("medium_alloc_s236n_central_fill", batch[i], sc, t_hz3_cache.my_shard);
#endif
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(binref, batch[i]);
#else
        hz3_bin_push(bin, batch[i]);
#endif
    }
    t_hz3_cache.stats.central_pop_hit[sc]++;
    S203_ALLOC_ADD(s236n_bulk_central_extra_objs, (uint32_t)got);
    return (uint32_t)got;
}
#endif

