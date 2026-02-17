// ============================================================================
// Day 5: Slow path with batch refill (inbox -> central -> segment)
// ============================================================================

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

void* hz3_alloc_slow(int sc) {
#if HZ3_S72_MEDIUM_DEBUG
#define HZ3_S72_MEDIUM_CHECK(where, ptr) do { \
    if ((ptr) != NULL) { \
        hz3_medium_boundary_check_ptr((where), (ptr), (sc), t_hz3_cache.my_shard); \
    } \
} while (0)
#else
#define HZ3_S72_MEDIUM_CHECK(where, ptr) ((void)0)
#endif
#if HZ3_WATCH_PTR_BOX
#define HZ3_WATCH_PTR_MEDIUM(where, ptr) do { \
    if ((ptr) != NULL) { \
        hz3_watch_ptr_on_alloc((where), (ptr), (sc), t_hz3_cache.my_shard); \
    } \
} while (0)
#else
#define HZ3_WATCH_PTR_MEDIUM(where, ptr) ((void)0)
#endif
#if HZ3_TCACHE_INIT_ON_MISS
    // S32-1: TLS init at slow path entry (hot path skips init check)
    hz3_tcache_ensure_init_slow();
#endif
    S203_ALLOC_INC(alloc_slow_calls);
#if HZ3_ARENA_PRESSURE_BOX
    hz3_pressure_check_and_flush();
#endif
#if HZ3_TCACHE_SOA_LOCAL
    uint32_t bin_idx = hz3_bin_index_medium(sc);
    Hz3BinRef binref = hz3_tcache_get_local_binref(bin_idx);
#else
    Hz3Bin* bin = hz3_tcache_get_bin(sc);
#endif
    int want = HZ3_REFILL_BATCH[sc];
    want = hz3_s223_effective_want(sc, want);
#if HZ3_S189_MEDIUM_TRANSFERCACHE
    const int s189_xfer_sc = (sc >= HZ3_S189_SC_MIN && sc <= HZ3_S189_SC_MAX);
    void* s189_extra[32];
    int s189_extra_n = 0;
#endif

    // Day 6: Update stats
    t_hz3_cache.stats.refill_calls[sc]++;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
    hz3_medium_path_stats_on_slow_enter(sc);
#endif

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24-1: Budgeted flush (round-robin, fixed cost per refill)
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
#if HZ3_S173_DSTBIN_DEMAND_GATE && HZ3_REMOTE_STASH_SPARSE
    if (!t_hz3_cache.remote_hint) {
        t_hz3_cache.remote_flush_credit = 0;
    } else if (t_hz3_cache.remote_flush_credit >= (uint16_t)HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT) {
        hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
        uint16_t credit = t_hz3_cache.remote_flush_credit;
        uint16_t consume = (uint16_t)HZ3_S173_DSTBIN_DEMAND_CREDIT_CONSUME;
        t_hz3_cache.remote_flush_credit = (credit > consume) ? (uint16_t)(credit - consume) : 0;
    }
#else
    if (t_hz3_cache.remote_hint) {
        hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
    }
#endif
#else
    hz3_dstbin_flush_remote_budget(HZ3_DSTBIN_FLUSH_BUDGET_BINS);
#endif
#endif

    // S229: optional central-first trial for selected medium classes.
#if HZ3_S229_CENTRAL_FIRST
    int s229_central_tried = 0;
    if (hz3_s229_sc_in_range(sc)) {
        int s229_maybe_supply = 1;
#if HZ3_S229_PEEK_BEFORE_POP
        s229_maybe_supply = hz3_central_has_supply(t_hz3_cache.my_shard, sc);
#endif
        if (s229_maybe_supply) {
            void* s229_batch[16];
            int s229_got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, s229_batch, want);
            s229_central_tried = 1;
            if (s229_got > 0) {
                S203_ALLOC_INC(alloc_slow_from_central);
                S203_ALLOC_INC_SC(central, sc);
                for (int i = 1; i < s229_got; i++) {
                    HZ3_S72_MEDIUM_CHECK("medium_alloc_central_first_fill", s229_batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
                    hz3_binref_push(binref, s229_batch[i]);
#else
                    hz3_bin_push(bin, s229_batch[i]);
#endif
                }
                t_hz3_cache.stats.central_pop_hit[sc]++;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
                hz3_medium_path_stats_on_central_hit(sc, s229_got);
#endif
                HZ3_S72_MEDIUM_CHECK("medium_alloc_central_first", s229_batch[0]);
                HZ3_WATCH_PTR_MEDIUM("medium_alloc_central_first", s229_batch[0]);
                HZ3_S208_RESET_STATE(sc);
                HZ3_S223_RESET_STREAK(sc);
                hz3_epoch_maybe();
                return s229_batch[0];
            }
        } else {
#if HZ3_S229_SKIP_SECOND_CENTRAL_TRY
            s229_central_tried = 1;
#endif
        }
    }
#endif

    // 1. Drain inbox first (already batches to bin)
#if HZ3_S204_LARSON_DIAG
    if (hz3_inbox_peek_check(t_hz3_cache.my_shard, sc)) {
        S204_ALLOC_INC_SEEN(sc);
        S204_ALLOC_INC_SHARD_SC(t_hz3_cache.my_shard, sc);
    }
#endif
    void* obj = hz3_inbox_drain_medium_from_shard_slow(t_hz3_cache.my_shard, sc);
    if (obj) {
        S203_ALLOC_INC(alloc_slow_from_inbox);
        S203_ALLOC_INC_SC(inbox, sc);
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
        hz3_medium_path_stats_on_inbox_hit(sc);
#endif
        HZ3_S72_MEDIUM_CHECK("medium_alloc_inbox", obj);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_inbox", obj);
#if HZ3_S236N_ALLOC_SLOW_BULK_REFILL
        (void)hz3_s236n_bulk_refill_inbox(sc,
#if HZ3_TCACHE_SOA_LOCAL
                                          binref
#else
                                          bin
#endif
                                          );
#endif
        HZ3_S223_RESET_STREAK(sc);
        hz3_epoch_maybe();  // Day 6: epoch check
        return obj;
    }

#if HZ3_S220_CPU_RRQ
    int rrq_want = want;
    if (rrq_want > HZ3_S220_CPU_RRQ_REFILL_BURST_MAX) {
        rrq_want = HZ3_S220_CPU_RRQ_REFILL_BURST_MAX;
    }
    if (rrq_want > 16) {
        rrq_want = 16;
    }
    if (rrq_want < 1) {
        rrq_want = 1;
    }
    void* rrq_batch[16];
    int rrq_got = hz3_s220_cpu_rrq_pop_batch(sc, rrq_batch, rrq_want);
    if (rrq_got > 0) {
        S203_ALLOC_INC(alloc_slow_from_inbox);
        S203_ALLOC_INC_SC(inbox, sc);
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
        hz3_medium_path_stats_on_inbox_hit(sc);
#endif
        for (int i = 1; i < rrq_got; i++) {
            HZ3_S72_MEDIUM_CHECK("medium_alloc_rrq_fill", rrq_batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, rrq_batch[i]);
#else
            hz3_bin_push(bin, rrq_batch[i]);
#endif
        }
        obj = rrq_batch[0];
        HZ3_S72_MEDIUM_CHECK("medium_alloc_rrq", obj);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_rrq", obj);
        HZ3_S223_RESET_STREAK(sc);
        hz3_epoch_maybe();
        return obj;
    }
#endif

#if HZ3_S206B_MEDIUM_DST_MIXED_BATCH
    obj = hz3_inbox_drain_medium_mixed_from_shard_slow(t_hz3_cache.my_shard, sc);
    if (obj) {
        S203_ALLOC_INC(alloc_slow_from_inbox);
        S203_ALLOC_INC_SC(inbox, sc);
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
        hz3_medium_path_stats_on_inbox_hit(sc);
#endif
        HZ3_S72_MEDIUM_CHECK("medium_alloc_inbox_mixed", obj);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_inbox_mixed", obj);
        HZ3_S223_RESET_STREAK(sc);
        hz3_epoch_maybe();
        return obj;
    }
#endif

#if HZ3_S206_STEAL_ON_MISS
    // S206: on inbox miss, try bounded steal from other shard inboxes before central.
    obj = hz3_s206_medium_try_steal(sc);
    if (obj) {
        S203_ALLOC_INC(alloc_slow_from_inbox);
        S203_ALLOC_INC_SC(inbox, sc);
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
        hz3_medium_path_stats_on_inbox_hit(sc);
#endif
        HZ3_S72_MEDIUM_CHECK("medium_alloc_steal", obj);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_steal", obj);
        HZ3_S223_RESET_STREAK(sc);
        hz3_epoch_maybe();
        return obj;
    }
#endif

    // 2. Batch pop from central pool
    void* batch[16];
#if HZ3_S189_MEDIUM_TRANSFERCACHE
    if (s189_xfer_sc) {
        int got_xfer = hz3_central_xfer_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
        if (got_xfer > 0) {
            S203_ALLOC_INC(alloc_slow_from_central);
            S203_ALLOC_INC_SC(central, sc);
            for (int i = 1; i < got_xfer; i++) {
                HZ3_S72_MEDIUM_CHECK("medium_alloc_xfer_fill", batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
                hz3_binref_push(binref, batch[i]);
#else
                hz3_bin_push(bin, batch[i]);
#endif
            }
            t_hz3_cache.stats.central_pop_hit[sc]++;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
            hz3_medium_path_stats_on_central_hit(sc, got_xfer);
#endif
            HZ3_S72_MEDIUM_CHECK("medium_alloc_xfer", batch[0]);
            HZ3_WATCH_PTR_MEDIUM("medium_alloc_xfer", batch[0]);
            HZ3_S208_RESET_STATE(sc);
            HZ3_S223_RESET_STREAK(sc);
            hz3_epoch_maybe();
            return batch[0];
        }
    }
#endif
    int got = 0;
#if HZ3_S229_CENTRAL_FIRST && HZ3_S229_SKIP_SECOND_CENTRAL_TRY
    if (!(s229_central_tried && hz3_s229_sc_in_range(sc))) {
        got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
    }
#else
    got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
#endif
    if (got > 0) {
        HZ3_S208_RESET_STATE(sc);
        S203_ALLOC_INC(alloc_slow_from_central);
        S203_ALLOC_INC_SC(central, sc);
        // Push remaining to bin, return first
        for (int i = 1; i < got; i++) {
            HZ3_S72_MEDIUM_CHECK("medium_alloc_central_fill", batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, batch[i]);
#else
            hz3_bin_push(bin, batch[i]);
#endif
        }
        // Day 6: Update stats
        t_hz3_cache.stats.central_pop_hit[sc]++;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
        hz3_medium_path_stats_on_central_hit(sc, got);
#endif
#if HZ3_S236N_ALLOC_SLOW_BULK_REFILL
        (void)hz3_s236n_bulk_refill_central(sc, HZ3_S236N_BULK_WANT - got,
#if HZ3_TCACHE_SOA_LOCAL
                                            binref
#else
                                            bin
#endif
                                            );
#endif
        HZ3_S72_MEDIUM_CHECK("medium_alloc_central", batch[0]);
        HZ3_WATCH_PTR_MEDIUM("medium_alloc_central", batch[0]);
        HZ3_S223_RESET_STREAK(sc);
        hz3_epoch_maybe();  // Day 6: epoch check
        return batch[0];
    }
    // Day 6: Update stats
    t_hz3_cache.stats.central_pop_miss[sc]++;
    HZ3_S223_NOTE_CENTRAL_MISS(sc);
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE
    if (t_s208_miss_streak[(uint32_t)sc] < UINT16_MAX) {
        t_s208_miss_streak[(uint32_t)sc]++;
    }
    if (t_s208_recent_reserved[(uint32_t)sc]) {
        S203_ALLOC_INC(s208_central_miss_after_reserve);
        t_s208_recent_reserved[(uint32_t)sc] = 0;
    }
#endif
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
    hz3_medium_path_stats_on_central_miss(sc);
#endif
    hz3_s209_medium_miss_seq_note((uint8_t)t_hz3_cache.my_shard, sc,
                                  (uint32_t)t_hz3_cache.stats.central_pop_miss[sc]);

#if HZ3_S190_MISS_TRIGGERED_REMOTE_FLUSH && HZ3_PTAG_DSTBIN_ENABLE
    const int s190_sc_in_range = (sc >= HZ3_S190_SC_MIN && sc <= HZ3_S190_SC_MAX);
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
    if (s190_sc_in_range && t_hz3_cache.remote_hint) {
#else
    if (s190_sc_in_range) {
#endif
        int s190_can_force = 1;
#if HZ3_S190_REQUIRE_CREDIT
        if (t_hz3_cache.remote_flush_credit < (uint16_t)HZ3_S190_MIN_CREDIT) {
            s190_can_force = 0;
        }
#endif
        if (s190_can_force) {
#if HZ3_S190_TARGETED_FLUSH
            hz3_dstbin_flush_remote_sc_budget((uint32_t)sc, HZ3_S190_FLUSH_BUDGET_BINS);
#else
            hz3_dstbin_flush_remote_budget(HZ3_S190_FLUSH_BUDGET_BINS);
#endif
#if HZ3_S190_REQUIRE_CREDIT
            uint16_t credit = t_hz3_cache.remote_flush_credit;
            uint16_t consume = (uint16_t)HZ3_S190_CREDIT_CONSUME;
            t_hz3_cache.remote_flush_credit = (credit > consume) ? (uint16_t)(credit - consume) : 0;
#endif

            // Retry inbox once after forced flush.
#if HZ3_TCACHE_SOA_LOCAL
#if HZ3_BIN_SPLIT_COUNT
            Hz3Bin tmp_bin_retry = { .head = hz3_split_ptr(*binref.head), .count = *binref.count };
            obj = hz3_inbox_drain_medium_slow(t_hz3_cache.my_shard, sc, &tmp_bin_retry);
            uint32_t total_count_retry = ((uint32_t)tmp_bin_retry.count << HZ3_SPLIT_CNT_SHIFT);
            uint32_t new_low_retry = total_count_retry & (uint32_t)HZ3_SPLIT_CNT_MASK;
            *binref.count = (Hz3BinCount)(total_count_retry >> HZ3_SPLIT_CNT_SHIFT);
            *binref.head = hz3_split_pack(tmp_bin_retry.head, new_low_retry);
#else
            Hz3Bin tmp_bin_retry = { .head = *binref.head, .count = *binref.count };
            obj = hz3_inbox_drain_medium_slow(t_hz3_cache.my_shard, sc, &tmp_bin_retry);
            *binref.head = tmp_bin_retry.head;
#if !HZ3_BIN_LAZY_COUNT
            *binref.count = tmp_bin_retry.count;
#endif
#endif
#else
            obj = hz3_inbox_drain_medium_slow(t_hz3_cache.my_shard, sc, bin);
#endif
            if (obj) {
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
                hz3_medium_path_stats_on_inbox_hit(sc);
#endif
                S203_ALLOC_INC(alloc_slow_from_inbox);
                S203_ALLOC_INC_SC(inbox, sc);
                HZ3_S72_MEDIUM_CHECK("medium_alloc_inbox_retry", obj);
                HZ3_WATCH_PTR_MEDIUM("medium_alloc_inbox_retry", obj);
                HZ3_S208_RESET_STATE(sc);
                HZ3_S223_RESET_STREAK(sc);
                hz3_epoch_maybe();
                return obj;
            }

#if HZ3_S190_RETRY_CENTRAL
            // Retry central once after forced flush.
#if HZ3_S189_MEDIUM_TRANSFERCACHE
            if (s189_xfer_sc) {
                int got_xfer = hz3_central_xfer_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
                if (got_xfer > 0) {
                    for (int i = 1; i < got_xfer; i++) {
                        HZ3_S72_MEDIUM_CHECK("medium_alloc_xfer_retry_fill", batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
                        hz3_binref_push(binref, batch[i]);
#else
                        hz3_bin_push(bin, batch[i]);
#endif
                    }
                    t_hz3_cache.stats.central_pop_hit[sc]++;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
                    hz3_medium_path_stats_on_central_hit(sc, got_xfer);
#endif
                    S203_ALLOC_INC(alloc_slow_from_central);
                    S203_ALLOC_INC_SC(central, sc);
                    HZ3_S72_MEDIUM_CHECK("medium_alloc_xfer_retry", batch[0]);
                    HZ3_WATCH_PTR_MEDIUM("medium_alloc_xfer_retry", batch[0]);
                    HZ3_S208_RESET_STATE(sc);
                    HZ3_S223_RESET_STREAK(sc);
                    hz3_epoch_maybe();
                    return batch[0];
                }
            }
#endif
            got = hz3_central_pop_batch(t_hz3_cache.my_shard, sc, batch, want);
            if (got > 0) {
                for (int i = 1; i < got; i++) {
                    HZ3_S72_MEDIUM_CHECK("medium_alloc_central_retry_fill", batch[i]);
#if HZ3_TCACHE_SOA_LOCAL
                    hz3_binref_push(binref, batch[i]);
#else
                    hz3_bin_push(bin, batch[i]);
#endif
                }
                t_hz3_cache.stats.central_pop_hit[sc]++;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
                hz3_medium_path_stats_on_central_hit(sc, got);
#endif
                S203_ALLOC_INC(alloc_slow_from_central);
                S203_ALLOC_INC_SC(central, sc);
                HZ3_S72_MEDIUM_CHECK("medium_alloc_central_retry", batch[0]);
                HZ3_WATCH_PTR_MEDIUM("medium_alloc_central_retry", batch[0]);
                HZ3_S208_RESET_STATE(sc);
                HZ3_S223_RESET_STREAK(sc);
                hz3_epoch_maybe();
                return batch[0];
            }
#endif
        }
    }
#endif

#if HZ3_S65_MEDIUM_RECLAIM && HZ3_S65_MEDIUM_RECLAIM_DEMAND
    hz3_s65_medium_reclaim_on_demand(sc, want, HZ3_S65_RECLAIM_REASON_CENTRAL_MISS);
#endif

#if HZ3_S208_MEDIUM_CENTRAL_RESERVE
    uint32_t s208_reserve_left = 0;
    void* s208_reserve_head = NULL;
    void* s208_reserve_tail = NULL;
    uint32_t s208_reserve_n = 0;
    const int s208_sc_in_range = (sc >= HZ3_S208_SC_MIN && sc <= HZ3_S208_SC_MAX);
    if (s208_sc_in_range &&
        t_s208_miss_streak[(uint32_t)sc] >= (uint16_t)HZ3_S208_MISS_STREAK_MIN &&
        ((uint32_t)t_hz3_cache.stats.central_pop_miss[sc] % (uint32_t)HZ3_S208_MISS_STRIDE) == 0u) {
        S203_ALLOC_INC(s208_reserve_attempts);
        uint32_t central_now = hz3_central_count_snapshot(t_hz3_cache.my_shard, sc);
        if (central_now < (uint32_t)HZ3_S208_CENTRAL_FLOOR_OBJS) {
            uint32_t deficit = (uint32_t)HZ3_S208_CENTRAL_FLOOR_OBJS - central_now;
            if (deficit > (uint32_t)HZ3_S208_RESERVE_MAX_OBJS) {
                deficit = (uint32_t)HZ3_S208_RESERVE_MAX_OBJS;
            }
            s208_reserve_left = deficit;
        }
    }
#elif HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
    uint32_t s208_reserve_left = 0;
    void* s208_reserve_head = NULL;
    void* s208_reserve_tail = NULL;
    uint32_t s208_reserve_n = 0;
    uint32_t s211_local_kept = 0;
    const int s211_sc_in_range = (sc >= HZ3_S211_SC_MIN && sc <= HZ3_S211_SC_MAX);
    if (s211_sc_in_range &&
        ((uint32_t)t_hz3_cache.stats.central_pop_miss[sc] % (uint32_t)HZ3_S211_MISS_STRIDE) == 0u) {
        s208_reserve_left = (uint32_t)HZ3_S211_RESERVE_MAX_OBJS;
    }
#endif

    // 3. Batch allocate from segment
#if HZ3_SPAN_CARVE_ENABLE
    // S26-2: Try span carve for sc=6,7 (28KB, 32KB)
    if (sc >= 6) {
#if HZ3_TCACHE_SOA_LOCAL
        Hz3Bin tmp_bin2 = { .head = *binref.head, .count = *binref.count };
        obj = hz3_slow_alloc_span_carve(sc, &tmp_bin2);
        *binref.head = tmp_bin2.head;
#if !HZ3_BIN_LAZY_COUNT
        *binref.count = tmp_bin2.count;
#endif
#else
        obj = hz3_slow_alloc_span_carve(sc, bin);
#endif
        if (obj) {
            S203_ALLOC_INC(alloc_slow_from_segment);
            S203_ALLOC_INC_SC(segment, sc);
            HZ3_S72_MEDIUM_CHECK("medium_alloc_span", obj);
            HZ3_WATCH_PTR_MEDIUM("medium_alloc_span", obj);
            hz3_epoch_maybe();
            return obj;
        }
    }
#endif
    // Fallback: normal batch allocation
    obj = NULL;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
    int segment_got = 0;
#endif
#if HZ3_S74_LANE_BATCH
    // S202: Eco Mode adaptive burst sizing
#if HZ3_ECO_MODE
    const int refill_burst = hz3_eco_refill_burst();
#define HZ3_TCACHE_BURST_ARRAY_SIZE HZ3_ECO_REFILL_BURST_ARRAY_SIZE
#else
    const int refill_burst = HZ3_S74_REFILL_BURST;
#define HZ3_TCACHE_BURST_ARRAY_SIZE HZ3_S74_REFILL_BURST
#endif
    int remaining = want;
    while (remaining > 0) {
        int burst = remaining;
        if (burst > refill_burst) {
            burst = refill_burst;
        }
        void* burst_batch[HZ3_TCACHE_BURST_ARRAY_SIZE];
        int got = hz3_s74_alloc_from_segment_burst(sc, burst_batch, burst);
        if (got <= 0) {
            break;
        }
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
        segment_got += got;
#endif
        for (int i = 0; i < got; i++) {
            void* run = burst_batch[i];
            if (!obj) {
                obj = run;  // First one to return
                continue;
            }
            HZ3_S72_MEDIUM_CHECK("medium_alloc_segment_fill", run);
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE || HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
            if (s208_reserve_left > 0) {
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
                if (s211_local_kept < (uint32_t)HZ3_S211_LOCAL_KEEP_MIN) {
                    goto s211_skip_reserve_burst;
                }
#endif
                if (!s208_reserve_head) {
                    s208_reserve_head = run;
                } else {
                    hz3_obj_set_next(s208_reserve_tail, run);
                }
                s208_reserve_tail = run;
                s208_reserve_n++;
                s208_reserve_left--;
                continue;
            }
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
s211_skip_reserve_burst:
#endif
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
            if (s189_xfer_sc && s189_extra_n < (int)(sizeof(s189_extra) / sizeof(s189_extra[0]))) {
                s189_extra[s189_extra_n++] = run;
                continue;
            }
#endif
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, run);
#else
            hz3_bin_push(bin, run);
#endif
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
            s211_local_kept++;
#endif
        }
        remaining -= got;
        if (got < burst) {
            break;
        }
    }
#undef HZ3_TCACHE_BURST_ARRAY_SIZE
#endif
    if (!obj) {
        for (int i = 0; i < want; i++) {
            void* run = hz3_slow_alloc_from_segment(sc);
            if (!run) break;
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
            segment_got++;
#endif
            if (i == 0) {
                obj = run;  // First one to return
            } else {
                HZ3_S72_MEDIUM_CHECK("medium_alloc_segment_fill", run);
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE || HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
                if (s208_reserve_left > 0) {
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
                    if (s211_local_kept < (uint32_t)HZ3_S211_LOCAL_KEEP_MIN) {
                        goto s211_skip_reserve_scalar;
                    }
#endif
                    if (!s208_reserve_head) {
                        s208_reserve_head = run;
                    } else {
                        hz3_obj_set_next(s208_reserve_tail, run);
                    }
                    s208_reserve_tail = run;
                    s208_reserve_n++;
                    s208_reserve_left--;
                    continue;
                }
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
s211_skip_reserve_scalar:
#endif
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
                if (s189_xfer_sc && s189_extra_n < (int)(sizeof(s189_extra) / sizeof(s189_extra[0]))) {
                    s189_extra[s189_extra_n++] = run;
                    continue;
                }
#endif
#if HZ3_TCACHE_SOA_LOCAL
                hz3_binref_push(binref, run);
#else
                hz3_bin_push(bin, run);
#endif
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
                s211_local_kept++;
#endif
            }
        }
    }
#if HZ3_S189_MEDIUM_TRANSFERCACHE
    if (s189_xfer_sc && s189_extra_n > 0) {
        int pushed = hz3_central_xfer_push_array(t_hz3_cache.my_shard, sc, s189_extra, s189_extra_n);
        for (int i = pushed; i < s189_extra_n; i++) {
            void* run = s189_extra[i];
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, run);
#else
            hz3_bin_push(bin, run);
#endif
        }
    }
#endif
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE || HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
    if (s208_reserve_head) {
        hz3_obj_set_next(s208_reserve_tail, NULL);
        hz3_central_push_list(t_hz3_cache.my_shard, sc,
                              s208_reserve_head, s208_reserve_tail, s208_reserve_n);
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE
        S203_ALLOC_INC(s208_reserve_hits);
        t_s208_recent_reserved[(uint32_t)sc] = 1;
#endif
    }
#endif
#if HZ3_MEDIUM_PATH_STATS && !HZ3_SHIM_FORWARD_ONLY
    if (obj) {
        hz3_medium_path_stats_on_segment_hit(sc, segment_got);
    } else {
        hz3_medium_path_stats_on_segment_fail(sc);
    }
#endif
    if (obj) {
        S203_ALLOC_INC(alloc_slow_from_segment);
        S203_ALLOC_INC_SC(segment, sc);
    }
    HZ3_S72_MEDIUM_CHECK("medium_alloc_segment", obj);
    HZ3_WATCH_PTR_MEDIUM("medium_alloc_segment", obj);
    hz3_epoch_maybe();  // Day 6: epoch check
    return obj;
}
#undef HZ3_S72_MEDIUM_CHECK
#undef HZ3_WATCH_PTR_MEDIUM
