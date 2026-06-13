#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
void hz3_inbox_aligned_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    if (owner >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail || n == 0) return;

    if (hz3_shard_live_count(owner) == 0 || hz3_shard_is_exiting(owner)) {
        hz3_obj_set_next(tail, NULL);
        hz3_central_aligned_push_list(owner, sc, head, tail, n);
        return;
    }

    Hz3Inbox* inbox = &g_hz3_inbox_medium_aligned[owner][sc];
    uint32_t lane = hz3_s231_fast_sc_enabled(sc) ? 0u : hz3_inbox_lane_select(owner, sc, head, tail);

    void* old_head;
    do {
        old_head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head[lane], &old_head, head,
        memory_order_release, memory_order_relaxed));

    atomic_fetch_add_explicit(&inbox->count[lane], n, memory_order_relaxed);
    atomic_fetch_add_explicit(&inbox->seq, 1, memory_order_release);
}

void* hz3_inbox_aligned_drain_if_nonempty(uint8_t shard, int sc, Hz3Bin* bin) {
    if (shard >= HZ3_NUM_SHARDS) return NULL;
    if (sc < 0 || sc >= HZ3_NUM_SC) return NULL;
    if (!bin) return NULL;

    Hz3Inbox* inbox = &g_hz3_inbox_medium_aligned[shard][sc];
    uint32_t lane_max = hz3_s231_fast_sc_enabled(sc) ? 1u : (uint32_t)HZ3_S195_MEDIUM_INBOX_SHARDS;
    int nonempty = 0;
    for (uint32_t lane = 0; lane < lane_max; lane++) {
        if (atomic_load_explicit(&inbox->head[lane], memory_order_acquire) != NULL) {
            nonempty = 1;
            break;
        }
    }
    if (!nonempty) return NULL;

    void* first = NULL;
    void* overflow_head = NULL;
    void* overflow_tail = NULL;
    uint32_t overflow_n = 0;

#if HZ3_BIN_LAZY_COUNT && HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY
    uint16_t bin_len = hz3_bin_is_empty(bin) ? 0 : HZ3_BIN_HARD_CAP;
#elif HZ3_BIN_LAZY_COUNT
    uint16_t bin_len = hz3_bin_count_walk(bin, HZ3_BIN_HARD_CAP);
#endif

    for (uint32_t lane = 0; lane < lane_max; lane++) {
        void* head = atomic_exchange_explicit(&inbox->head[lane], NULL, memory_order_acq_rel);
        if (!head) continue;
        atomic_store_explicit(&inbox->count[lane], 0, memory_order_relaxed);

        void* obj = head;
        if (!first) {
            first = obj;
            obj = hz3_obj_get_next(obj);
        }

        while (obj) {
            void* next = hz3_obj_get_next(obj);
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
    }

    if (!first) return NULL;

    if (overflow_head) {
        hz3_obj_set_next(overflow_tail, NULL);
        hz3_central_aligned_push_list(shard, sc, overflow_head, overflow_tail, overflow_n);
    }

    atomic_fetch_add_explicit(&inbox->drain_seq, 1, memory_order_release);
    return first;
}
#endif
