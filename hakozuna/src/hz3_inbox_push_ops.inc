void hz3_inbox_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    if (owner >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail) return;

#if HZ3_S82_STASH_GUARD
    {
        uint32_t expect_bin = (uint32_t)hz3_bin_index_medium(sc);
        hz3_s82_stash_guard_list("inbox_push_list", head, expect_bin, HZ3_S82_STASH_GUARD_MAX_WALK);
    }
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_MAX_LIVE_COUNT >= 0
    {
        uint32_t owner_live_count = hz3_shard_live_count(owner);
        if (owner_live_count <= (uint32_t)HZ3_S65_INBOX_TO_CENTRAL_MAX_LIVE_COUNT) {
            hz3_obj_set_next(tail, NULL);
            if (owner_live_count == 0) {
                hz3_s65_inbox_note_orphan_to_central(sc, (uint64_t)n);
            } else {
                hz3_s65_inbox_note_live_to_central(sc, (uint64_t)n);
            }
            hz3_central_push_list(owner, sc, head, tail, n);
            hz3_s65_inbox_reclaim_after_to_central(owner, sc, n, owner_live_count);
            return;
        }
    }
#else
    uint32_t owner_live_count = hz3_shard_live_count(owner);
    (void)owner_live_count;
#if HZ3_S65_EXITING_INBOX_TO_CENTRAL
    size_t owner_sc_pages = hz3_sc_to_pages(sc);
    uint32_t owner_sc_page_bit =
        owner_sc_pages < 32 ? (uint32_t)(1u << (uint32_t)owner_sc_pages) : 0u;
    if (hz3_shard_is_exiting(owner) &&
        owner_sc_pages >= (size_t)HZ3_S65_EXITING_INBOX_TO_CENTRAL_MIN_PAGES &&
        owner_sc_pages <= (size_t)HZ3_S65_EXITING_INBOX_TO_CENTRAL_MAX_PAGES &&
        (owner_sc_page_bit & (uint32_t)HZ3_S65_EXITING_INBOX_TO_CENTRAL_PAGE_MASK) != 0) {
        hz3_obj_set_next(tail, NULL);
        if (owner_live_count == 0) {
            hz3_s65_inbox_note_orphan_to_central(sc, (uint64_t)n);
        } else {
            hz3_s65_inbox_note_exiting_to_central(sc, (uint64_t)n);
        }
        hz3_central_push_list(owner, sc, head, tail, n);
        hz3_s65_inbox_reclaim_after_to_central(owner, sc, n, owner_live_count);
        return;
    }
#endif
#if HZ3_S65_ORPHAN_INBOX_TO_CENTRAL
    if (owner_live_count == 0) {
        hz3_obj_set_next(tail, NULL);
        hz3_s65_inbox_note_orphan_to_central(sc, (uint64_t)n);
        hz3_central_push_list(owner, sc, head, tail, n);
        hz3_s65_inbox_reclaim_after_to_central(owner, sc, n, owner_live_count);
        return;
    }
#endif
#endif

    Hz3Inbox* inbox = &g_hz3_inbox[owner][sc];
    uint32_t lane = hz3_s231_fast_sc_enabled(sc) ? 0u : hz3_inbox_lane_select(owner, sc, head, tail);
    hz3_s197_stats_register_once();

    // CAS loop to push list atomically
    void* old_head;
#if HZ3_S195_MEDIUM_INBOX_STATS || HZ3_S197_MEDIUM_INBOX_CAS_STATS || HZ3_S203_COUNTERS
    uint64_t attempts = 0;
    do {
        attempts++;
        old_head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head[lane], &old_head, head,
        memory_order_release, memory_order_relaxed));

#if HZ3_S203_COUNTERS
    t_s203_inbox_push_calls++;
    t_s203_inbox_push_objs += n;
    t_s203_inbox_push_sc_objs[sc] += n;
    if (attempts > 1) {
        t_s203_inbox_push_retries += (attempts - 1);
    }
#if HZ3_S204_LARSON_DIAG
    if (t_s204_in_dtor) {
        t_s204_inbox_push_dtor_sc_objs[sc] += n;
    }
#endif
#endif
#else
    do {
        old_head = atomic_load_explicit(&inbox->head[lane], memory_order_acquire);
        hz3_obj_set_next(tail, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &inbox->head[lane], &old_head, head,
        memory_order_release, memory_order_relaxed));
#endif



    atomic_fetch_add_explicit(&inbox->count[lane], n, memory_order_relaxed);
    hz3_s65_inbox_note_push(sc, (uint64_t)n);
    hz3_s65_inbox_note_push_owner(owner, sc, (uint64_t)n);
#if !(HZ3_S231_INBOX_FAST_MPSC && HZ3_S231_SKIP_PUSH_SEQ)
    atomic_fetch_add_explicit(&inbox->seq, 1, memory_order_release);
#else
    if (!hz3_s231_fast_sc_enabled(sc)) {
        atomic_fetch_add_explicit(&inbox->seq, 1, memory_order_release);
    }
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS || HZ3_S197_MEDIUM_INBOX_CAS_STATS
    uint64_t retries = (attempts > 0) ? (attempts - 1) : 0;
    hz3_s197_stats_on_push(sc, n, retries);
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS
    atomic_fetch_add_explicit(&g_s195_push_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_push_objs, n, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_push_retries, retries, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s195_lane_pushes[lane], 1, memory_order_relaxed);
    hz3_s195_stats_max_retry_update(retries);
#endif
}

void hz3_inbox_medium_mixed_push_list(uint8_t owner, void* head, void* tail, uint32_t n) {
    if (owner >= HZ3_NUM_SHARDS) return;
    if (!head || !tail || n == 0) return;

    Hz3InboxMixed* inbox = &g_hz3_inbox_medium_mixed[owner];
    uint32_t lane = hz3_inbox_lane_select(owner, -1, head, tail);

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

void hz3_inbox_push_remaining(uint8_t shard, int sc, void* obj) {
    if (shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!obj) return;

    // Find tail and count
    void* head = obj;
    void* tail = obj;
    uint32_t n = 1;
    void* next = hz3_obj_get_next(obj);
    while (next) {
        tail = next;
        n++;
        next = hz3_obj_get_next(next);
    }

    hz3_inbox_push_list(shard, sc, head, tail, n);
}
