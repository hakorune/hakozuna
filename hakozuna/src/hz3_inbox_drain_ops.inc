void* hz3_inbox_drain_if_nonempty(uint8_t shard, int sc, Hz3Bin* bin) {
#if HZ3_S198_INBOX_DRAIN_HINT_GATE
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        if (atomic_load_explicit(&inbox->head[lane], memory_order_relaxed) != NULL) {
            return hz3_inbox_drain(shard, sc, bin);
        }
    }
    return NULL;
#else
    return hz3_inbox_drain(shard, sc, bin);
#endif
}

void* hz3_inbox_drain_if_seq_advanced(uint8_t shard, int sc, Hz3Bin* bin,
                                      uint32_t* last_seq) {
#if HZ3_S200_INBOX_SEQ_GATE
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin || !last_seq) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    uint32_t seq_now = atomic_load_explicit(&inbox->seq, memory_order_acquire);
    if (seq_now == *last_seq) {
        return NULL;
    }
    *last_seq = seq_now;

#if HZ3_S200_INBOX_SEQ_STRICT
    return hz3_inbox_drain(shard, sc, bin);
#else
    return hz3_inbox_drain_if_nonempty(shard, sc, bin);
#endif
#else
    (void)last_seq;
    return hz3_inbox_drain_if_nonempty(shard, sc, bin);
#endif
}

void* hz3_inbox_drain(uint8_t shard, int sc, Hz3Bin* bin) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox[shard][sc];
    hz3_s197_stats_register_once();
#if HZ3_S195_MEDIUM_INBOX_STATS
    atomic_fetch_add_explicit(&g_s195_drain_calls, 1, memory_order_relaxed);
#endif
#if HZ3_S203_COUNTERS
    t_s203_inbox_drain_calls++;
#endif
    void* first = NULL;
    void* obj = NULL;
    uint64_t drained_objs = 0;

    // Day 5: Build overflow list for central (don't re-push to inbox)
    void* overflow_head = NULL;
    void* overflow_tail = NULL;
    uint32_t overflow_n = 0;

#if HZ3_S201_MEDIUM_INBOX_SPLICE
    // Helper for S201-lite: count contiguous chain and capture tail.
    // Keeps writes minimal by appending/splicing whole chains.
    #define HZ3_INBOX_CHAIN_COUNT(_head, _tail_out, _n_out) \
        do {                                                 \
            void* _cur = (_head);                           \
            void* _tail = _cur;                             \
            uint32_t _n = 0;                                \
            while (_cur) {                                  \
                _tail = _cur;                               \
                _n++;                                       \
                _cur = hz3_obj_get_next(_cur);              \
            }                                               \
            *(_tail_out) = _tail;                           \
            *(_n_out) = _n;                                 \
        } while (0)
#endif

    // Push to local bin, respecting HZ3_BIN_HARD_CAP
#if HZ3_BIN_LAZY_COUNT && HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY
    if (!hz3_bin_is_empty(bin)) {
#if HZ3_S123_INBOX_DRAIN_FAILFAST
        fprintf(stderr, "[HZ3_S123_INBOX] bin_not_empty sc=%d head=%p\n", sc, hz3_bin_head(bin));
        abort();
#endif
        // Conservative fallback: treat as full to avoid overfilling.
        // Keeps bounded behavior without a full walk.
    }
    uint16_t bin_len = hz3_bin_is_empty(bin) ? 0 : HZ3_BIN_HARD_CAP;
#elif HZ3_BIN_LAZY_COUNT
    uint16_t bin_len = hz3_bin_count_walk(bin, HZ3_BIN_HARD_CAP);
#endif
    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        // Fast path per lane: skip exchange when lane is empty.
        void* head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        if (!head) {
            continue;
        }
        head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) {
            continue;
        }
        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        if (!first) {
            first = head;
            obj = hz3_obj_get_next(head);
            drained_objs++;
        } else {
            obj = head;
        }

#if HZ3_S201_MEDIUM_INBOX_SPLICE
        if (!obj) {
            continue;
        }

        uint32_t cap_left = 0;
#if HZ3_BIN_LAZY_COUNT
        if (bin_len < HZ3_BIN_HARD_CAP) {
            cap_left = (uint32_t)(HZ3_BIN_HARD_CAP - bin_len);
        }
#else
        {
            Hz3BinCount cur_count = hz3_bin_count_get(bin);
            if (cur_count < HZ3_BIN_HARD_CAP) {
                cap_left = (uint32_t)(HZ3_BIN_HARD_CAP - cur_count);
            }
        }
#endif

        if (cap_left == 0) {
            void* rem_tail = NULL;
            uint32_t rem_n = 0;
            HZ3_INBOX_CHAIN_COUNT(obj, &rem_tail, &rem_n);
            if (!overflow_head) {
                overflow_head = obj;
            } else {
                hz3_obj_set_next(overflow_tail, obj);
            }
            overflow_tail = rem_tail;
            overflow_n += rem_n;
            drained_objs += rem_n;
            continue;
        }

        void* take_head = obj;
        void* take_tail = NULL;
        uint32_t take_n = 0;
        while (obj && take_n < cap_left) {
            take_tail = obj;
            take_n++;
            obj = hz3_obj_get_next(obj);
        }

        if (take_n > 0) {
            hz3_bin_prepend_list(bin, take_head, take_tail, take_n);
#if HZ3_BIN_LAZY_COUNT
            bin_len += (uint16_t)take_n;
#endif
            drained_objs += take_n;
        }

        if (obj) {
            void* rem_tail = NULL;
            uint32_t rem_n = 0;
            HZ3_INBOX_CHAIN_COUNT(obj, &rem_tail, &rem_n);
            if (!overflow_head) {
                overflow_head = obj;
            } else {
                hz3_obj_set_next(overflow_tail, obj);
            }
            overflow_tail = rem_tail;
            overflow_n += rem_n;
            drained_objs += rem_n;
        }
#else
        while (obj) {
            void* next = hz3_obj_get_next(obj);
            drained_objs++;
#if HZ3_BIN_LAZY_COUNT
            if (bin_len < HZ3_BIN_HARD_CAP) {
                hz3_bin_push(bin, obj);
                bin_len++;
            } else {
#else
            if (hz3_bin_count_get(bin) < HZ3_BIN_HARD_CAP) {
                hz3_bin_push(bin, obj);
            } else {
#endif
                // Add to overflow list
                if (!overflow_head) {
                    overflow_head = obj;
                } else {
                    hz3_obj_set_next(overflow_tail, obj);
                }
                overflow_tail = obj;
                overflow_n++;
            }
            obj = next;
        }
#endif
    }

    if (!first) {
        hz3_s197_stats_on_drain(sc, 0, 0);
        return NULL;
    }

    // Day 5: Send overflow to central (NOT back to inbox)
    if (overflow_head) {
        hz3_obj_set_next(overflow_tail, NULL);
        hz3_central_push_list(shard, sc, overflow_head, overflow_tail, overflow_n);
    }

#if HZ3_S195_MEDIUM_INBOX_STATS
    atomic_fetch_add_explicit(&g_s195_drain_nonempty, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_drain_objs, drained_objs, memory_order_relaxed);
#endif
#if HZ3_S203_COUNTERS
    t_s203_inbox_drain_nonempty++;
    t_s203_inbox_drain_objs += drained_objs;
    t_s203_inbox_drain_sc_objs[sc] += drained_objs;
#endif
    hz3_s65_inbox_note_drain(sc, drained_objs);
    atomic_fetch_add_explicit(&inbox->drain_seq, 1, memory_order_release);
    hz3_s197_stats_on_drain(sc, drained_objs, 1);

#if HZ3_S201_MEDIUM_INBOX_SPLICE
    #undef HZ3_INBOX_CHAIN_COUNT
#endif

    return first;
}
