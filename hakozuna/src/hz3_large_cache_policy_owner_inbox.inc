#if HZ3_S240_LARGE_OWNER_INBOX && HZ3_S240_LARGE_FRONT_CACHE
typedef struct Hz3LargeInbox {
    _Atomic(Hz3LargeHdr*) head;
    _Atomic uint32_t count;
    _Atomic size_t bytes;
} Hz3LargeInbox;

static Hz3LargeInbox g_s240_large_inbox[HZ3_NUM_SHARDS][HZ3_LARGE_SC_COUNT];
static _Atomic uint32_t g_s240_large_inbox_pushers[HZ3_NUM_SHARDS];

static inline void hz3_large_s240_inbox_note_add(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    size_t nodes = atomic_fetch_add_explicit(&g_s240_inbox_nodes_current, 1,
                                             memory_order_relaxed) + 1;
    size_t total = atomic_fetch_add_explicit(&g_s240_inbox_bytes_current, bytes,
                                             memory_order_relaxed) + bytes;
    hz3_large_s240_update_max(&g_s240_inbox_nodes_peak, nodes);
    hz3_large_s240_update_max(&g_s240_inbox_bytes_peak, total);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_s240_inbox_note_sub(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_sub(&g_s240_inbox_nodes_current, 1);
    hz3_large_s240_sub(&g_s240_inbox_bytes_current, bytes);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_s240_inbox_account_pop(Hz3LargeInbox* inbox,
                                                    uint32_t count,
                                                    size_t bytes,
                                                    const char* where) {
#if HZ3_S240_LARGE_FLUSH_VALIDATE
    uint32_t old_count = atomic_load_explicit(&inbox->count, memory_order_acquire);
    for (;;) {
        if (old_count < count) {
            hz3_large_s240_inbox_account_underflow(where,
                                                   "inbox-count-underflow",
                                                   old_count,
                                                   count,
                                                   &g_s240_flush_inbox_count_underflow);
            atomic_store_explicit(&inbox->count, 0, memory_order_release);
            break;
        }
        if (atomic_compare_exchange_weak_explicit(&inbox->count,
                                                  &old_count,
                                                  old_count - count,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            break;
        }
    }

    size_t old_bytes = atomic_load_explicit(&inbox->bytes, memory_order_acquire);
    for (;;) {
        if (old_bytes < bytes) {
            hz3_large_s240_inbox_account_underflow(where,
                                                   "inbox-bytes-underflow",
                                                   old_bytes,
                                                   bytes,
                                                   &g_s240_flush_inbox_bytes_underflow);
            atomic_store_explicit(&inbox->bytes, 0, memory_order_release);
            break;
        }
        if (atomic_compare_exchange_weak_explicit(&inbox->bytes,
                                                  &old_bytes,
                                                  old_bytes - bytes,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire)) {
            break;
        }
    }
#else
    atomic_fetch_sub_explicit(&inbox->count, count, memory_order_relaxed);
    atomic_fetch_sub_explicit(&inbox->bytes, bytes, memory_order_relaxed);
#endif
}

static inline int hz3_large_s240_inbox_eligible(int sc, size_t map_size, size_t req_size) {
    return hz3_large_s240_front_eligible_for_req(sc, map_size, req_size);
}

static inline int hz3_large_s240_owner_accepts_remote(uint8_t owner, uint8_t cur) {
    if (owner == UINT8_MAX || owner >= HZ3_NUM_SHARDS || cur == UINT8_MAX || owner == cur) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_owner_dead);
#endif
        hz3_s282_owner_inbox_on_push_reject_owner_dead();
        return 0;
    }
    uint32_t live = hz3_shard_live_count(owner);
    if (live == 0) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_owner_dead);
#endif
        hz3_s282_owner_inbox_on_push_reject_owner_dead();
        return 0;
    }
    if (hz3_shard_is_exiting(owner)) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_owner_exiting);
#endif
        hz3_s282_owner_inbox_on_push_reject_owner_exiting();
        return 0;
    }
    if (live > 1) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_shard_collision);
#endif
        hz3_s282_owner_inbox_on_push_reject_shard_collision();
        return 0;
    }
    return 1;
}

static inline int hz3_large_s240_inbox_push_remote(int sc, Hz3LargeHdr* hdr, uint8_t cur) {
    hz3_large_s240_register_once();
    hz3_s282_owner_inbox_on_push_attempt();
    hz3_s292_on_inbox_push_attempt(hdr);
    if (!hdr || !hz3_large_s240_inbox_eligible(sc, hdr->map_size, hdr->req_size)) {
        hz3_s282_owner_inbox_on_push_reject_invalid();
        hz3_s292_on_inbox_push_result(hdr, 0, 0, 0, 1);
        return 0;
    }
    uint8_t owner = hdr->owner_shard;
    if (!hz3_large_s240_owner_accepts_remote(owner, cur)) {
        hz3_s292_on_inbox_push_result(hdr, 0, 1, 0, 0);
        return 0;
    }

    uint32_t pushers = atomic_fetch_add_explicit(&g_s240_large_inbox_pushers[owner], 1,
                                                 memory_order_acq_rel) + 1u;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_update_max(&g_s240_inbox_pushers_peak, (size_t)pushers);
#else
    (void)pushers;
#endif
    if (!hz3_large_s240_owner_accepts_remote(owner, cur)) {
        atomic_fetch_sub_explicit(&g_s240_large_inbox_pushers[owner], 1,
                                  memory_order_acq_rel);
        hz3_s292_on_inbox_push_result(hdr, 0, 1, 0, 0);
        return 0;
    }

    Hz3LargeInbox* inbox = &g_s240_large_inbox[owner][sc];
    uint32_t count = atomic_load_explicit(&inbox->count, memory_order_relaxed);
    size_t bytes = atomic_load_explicit(&inbox->bytes, memory_order_relaxed);
    hz3_s282_owner_inbox_on_push_depth((size_t)count);
    if (count >= (uint32_t)HZ3_S240_LARGE_INBOX_MAX_NODES_PER_SC ||
        bytes + hdr->map_size < bytes ||
        bytes + hdr->map_size > (size_t)HZ3_S240_LARGE_INBOX_MAX_BYTES_PER_SC) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_reject_budget);
#endif
        hz3_s282_owner_inbox_on_push_reject_budget();
        atomic_fetch_sub_explicit(&g_s240_large_inbox_pushers[owner], 1,
                                  memory_order_acq_rel);
        hz3_s292_on_inbox_push_result(hdr, 0, 0, 1, 0);
        return 0;
    }

    hdr->in_use = 0;
    hdr->next = NULL;
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    hz3_large_s251_expect(hdr,
                          HZ3_LARGE_STATE_FREEING,
                          "inbox_push",
                          &g_s251_inbox_push_state_fail);
#endif
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_REMOTE, memory_order_release);

    // Publish accounting before the stack head. The owner can drain immediately
    // after seeing `head`, so count/bytes must already describe the node.
    atomic_fetch_add_explicit(&inbox->count, 1, memory_order_acq_rel);
    atomic_fetch_add_explicit(&inbox->bytes, hdr->map_size, memory_order_acq_rel);
    hz3_large_s240_inbox_note_add(hdr->map_size);

    Hz3LargeHdr* old = atomic_load_explicit(&inbox->head, memory_order_relaxed);
    size_t s282_push_cas_failures = 0;
    for (;;) {
        hdr->next_free = old;
        hz3_s282_owner_inbox_on_push_cas_attempt();
        if (atomic_compare_exchange_weak_explicit(&inbox->head,
                                                  &old,
                                                  hdr,
                                                  memory_order_release,
                                                  memory_order_relaxed)) {
            break;
        }
        s282_push_cas_failures++;
        hz3_s282_owner_inbox_on_push_cas_failure();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_cas_retries);
#endif
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_remote_push);
#endif
    hz3_s282_owner_inbox_on_push_ok(s282_push_cas_failures);
    hz3_s292_on_inbox_push_result(hdr, 1, 0, 0, 0);
    atomic_fetch_sub_explicit(&g_s240_large_inbox_pushers[owner], 1,
                              memory_order_acq_rel);
    return 1;
}

static inline Hz3LargeHdr* hz3_large_s240_inbox_pop_one(uint8_t owner, int sc) {
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return NULL;
    }
    Hz3LargeInbox* inbox = &g_s240_large_inbox[owner][sc];
    Hz3LargeHdr* head = atomic_load_explicit(&inbox->head, memory_order_acquire);
    int s282_saw_head = head != NULL;
    size_t s282_pop_cas_failures = 0;
    while (head) {
        Hz3LargeHdr* next = head->next_free;
        hz3_s282_owner_inbox_on_pop_cas_attempt();
        if (atomic_compare_exchange_weak_explicit(&inbox->head,
                                                  &head,
                                                  next,
                                                  memory_order_acquire,
                                                  memory_order_relaxed)) {
            hz3_s282_owner_inbox_on_pop_success(s282_pop_cas_failures);
            head->next_free = NULL;
            hz3_large_s240_inbox_account_pop(inbox,
                                             1,
                                             head->map_size,
                                             "inbox-pop-one");
            hz3_large_s240_inbox_note_sub(head->map_size);
            hz3_s292_on_inbox_pop(head, 0);
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
            hz3_large_s251_transition(head,
                                      HZ3_LARGE_STATE_REMOTE,
                                      HZ3_LARGE_STATE_FREEING,
                                      "inbox_pop",
                                      &g_s251_inbox_pop_to_freeing,
                                      &g_s251_inbox_pop_state_fail);
#endif
            return head;
        }
        s282_saw_head = 1;
        s282_pop_cas_failures++;
        hz3_s282_owner_inbox_on_pop_cas_failure();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_cas_retries);
#endif
    }
    hz3_s282_owner_inbox_on_pop_empty(s282_saw_head);
    return NULL;
}

#if HZ3_S245_LARGE_INBOX_BATCH_POP || HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || \
    HZ3_S274_LARGE_INBOX_SIZE_DRAIN
static inline Hz3LargeHdr* hz3_large_s245_inbox_pop_batch(uint8_t owner,
                                                          int sc,
                                                          int max_nodes,
                                                          int* out_count) {
    if (out_count) {
        *out_count = 0;
    }
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_LARGE_SC_COUNT ||
        max_nodes <= 0) {
        return NULL;
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s245_batch_pop_calls);
#endif
    Hz3LargeInbox* inbox = &g_s240_large_inbox[owner][sc];
    Hz3LargeHdr* head = atomic_load_explicit(&inbox->head, memory_order_acquire);
    while (head) {
        Hz3LargeHdr* tail = head;
        int count = 1;
        size_t bytes = head->map_size;
        while (count < max_nodes && tail->next_free) {
            tail = tail->next_free;
            count++;
            bytes += tail->map_size;
        }

        Hz3LargeHdr* rest = tail->next_free;
        if (atomic_compare_exchange_weak_explicit(&inbox->head,
                                                  &head,
                                                  rest,
                                                  memory_order_acquire,
                                                  memory_order_relaxed)) {
            tail->next_free = NULL;
            hz3_large_s240_inbox_account_pop(inbox,
                                             (uint32_t)count,
                                             bytes,
                                             "inbox-pop-batch");
            for (Hz3LargeHdr* cur = head; cur; cur = cur->next_free) {
                hz3_large_s240_inbox_note_sub(cur->map_size);
                hz3_s292_on_inbox_pop(cur, 0);
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
                hz3_large_s251_transition(cur,
                                          HZ3_LARGE_STATE_REMOTE,
                                          HZ3_LARGE_STATE_FREEING,
                                          "inbox_pop_batch",
                                          &g_s251_inbox_pop_to_freeing,
                                          &g_s251_inbox_pop_state_fail);
#endif
            }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s245_batch_pop_nonempty);
            hz3_large_s240_add(&g_s245_batch_pop_objs, (size_t)count);
            hz3_large_s240_update_max(&g_s245_batch_pop_max_objs, (size_t)count);
#endif
            if (out_count) {
                *out_count = count;
            }
            return head;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_cas_retries);
        hz3_large_s240_inc(&g_s245_batch_pop_cas_retries);
#endif
    }
    return NULL;
}
#endif

#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN
static inline int hz3_large_s274_inbox_batch_for_request(size_t req_size,
                                                         int fallback_max_nodes) {
    if (req_size < (size_t)HZ3_S274_LARGE_INBOX_MIN_REQUEST_BYTES ||
        req_size > (size_t)HZ3_S274_LARGE_INBOX_MAX_REQUEST_BYTES) {
        return 1;
    }

    int batch = (int)HZ3_S274_LARGE_INBOX_DRAIN_BATCH;
    if (batch < 1) {
        return 1;
    }
    if (fallback_max_nodes > 0 && batch > fallback_max_nodes) {
        return fallback_max_nodes;
    }
    return batch;
}
#else
static inline int hz3_large_s274_inbox_batch_for_request(size_t req_size,
                                                         int fallback_max_nodes) {
    (void)req_size;
    return fallback_max_nodes;
}
#endif

static inline int hz3_large_s240_inbox_drain_current_to_front(int sc, int max_nodes) {
    hz3_large_s240_register_once();
    if (max_nodes <= 0) {
        return 0;
    }
    uint8_t owner = hz3_large_s240_current_owner();
    if (owner == UINT8_MAX || owner >= HZ3_NUM_SHARDS ||
        sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return 0;
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_drain_calls);
#endif
    int drained = 0;
#if HZ3_S245_LARGE_INBOX_BATCH_POP
    int batch_count = 0;
    Hz3LargeHdr* batch = hz3_large_s245_inbox_pop_batch(owner, sc, max_nodes, &batch_count);
    (void)batch_count;
    while (batch) {
        Hz3LargeHdr* hdr = batch;
        batch = hdr->next_free;
        hdr->next_free = NULL;
        hdr->in_use = 0;
        hdr->next = NULL;
        if (hz3_large_s240_front_push(sc, hdr)) {
            drained++;
            continue;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
#endif
        (void)hz3_large_s240_flush_dispose_node(hdr,
                                                sc,
                                                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                "inbox-drain-front-reject",
                                                HZ3_S240_FLUSH_DRAIN_COUNTER);
    }
#else
    for (int i = 0; i < max_nodes; i++) {
        Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
        if (!hdr) {
            break;
        }
        hdr->in_use = 0;
        hdr->next = NULL;
        if (hz3_large_s240_front_push(sc, hdr)) {
            drained++;
            continue;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
#endif
        (void)hz3_large_s240_flush_dispose_node(hdr,
                                                sc,
                                                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                "inbox-drain-front-reject",
                                                HZ3_S240_FLUSH_DRAIN_COUNTER);
    }
#endif
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    if (drained > 0) {
        hz3_large_s240_inc(&g_s240_inbox_drain_nonempty);
        hz3_large_s240_add(&g_s240_inbox_drain_objs, (size_t)drained);
    }
#endif
    return drained;
}

#if HZ3_S246_LARGE_INBOX_TAKE_FIRST
static inline Hz3LargeHdr* hz3_large_s246_inbox_drain_current_take_first_to_front(
    int sc,
    int max_nodes,
    size_t req_size,
    int* out_drained) {
    hz3_large_s240_register_once();
    if (out_drained) {
        *out_drained = 0;
    }
    if (max_nodes <= 0) {
        return NULL;
    }
    uint8_t owner = hz3_large_s240_current_owner();
    if (owner == UINT8_MAX || owner >= HZ3_NUM_SHARDS ||
        sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        return NULL;
    }
    size_t s282_take_depth = (size_t)atomic_load_explicit(
        &g_s240_large_inbox[owner][sc].count, memory_order_relaxed);
    hz3_s282_owner_inbox_on_take_first_call(s282_take_depth);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_drain_calls);
    hz3_large_s240_inc(&g_s246_take_first_calls);
#endif

    Hz3LargeHdr* first = NULL;
#if HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    int batch_count = 0;
    int use_batch_pop = 0;
#if HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    int selected_max_nodes = hz3_large_s274_inbox_batch_for_request(req_size, max_nodes);
#else
    (void)req_size;
    int selected_max_nodes = max_nodes;
#endif
    if (selected_max_nodes > 1) {
        use_batch_pop = 1;
        first = hz3_large_s245_inbox_pop_batch(owner, sc, selected_max_nodes, &batch_count);
    } else {
        first = hz3_large_s240_inbox_pop_one(owner, sc);
    }
#else
    (void)req_size;
    first = hz3_large_s240_inbox_pop_one(owner, sc);
#endif
    if (!first) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s246_take_first_misses);
#endif
        hz3_s282_owner_inbox_on_take_first_miss(s282_take_depth);
        return NULL;
    }

    int drained = 1;
    Hz3LargeHdr* extras = first->next_free;
    first->next_free = NULL;
    first->in_use = 0;
    first->next = NULL;
    hz3_s285_on_take_first_return_first();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s246_take_first_hits);
    hz3_large_s240_inc(&g_s246_take_first_direct);
#endif
    hz3_s282_owner_inbox_on_take_first_hit();
    int refill_limit = max_nodes;
#if HZ3_S286_LARGE_TAKE_FIRST_REFILL_CAP
    if (req_size >= (size_t)HZ3_S286_LARGE_TAKE_FIRST_MIN_REQUEST_BYTES &&
        req_size <= (size_t)HZ3_S286_LARGE_TAKE_FIRST_MAX_REQUEST_BYTES &&
        refill_limit > (int)HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES) {
        refill_limit = (int)HZ3_S286_LARGE_TAKE_FIRST_MAX_DRAIN_NODES;
    }
#endif

#if HZ3_S273_LARGE_INBOX_TAKE_FIRST_BATCH || HZ3_S274_LARGE_INBOX_SIZE_DRAIN
    (void)batch_count;
    if (use_batch_pop) {
        Hz3LargeHdr* pending = extras;
        while (pending) {
            Hz3LargeHdr* hdr = pending;
            pending = hdr->next_free;
            hdr->next_free = NULL;
            hdr->in_use = 0;
            hdr->next = NULL;
            if (hz3_large_s240_front_push(sc, hdr)) {
                drained++;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
                hz3_large_s240_inc(&g_s246_take_first_extra_fronted);
#endif
                hz3_s282_owner_inbox_on_extra_fronted();
                continue;
            }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
            hz3_large_s240_inc(&g_s246_take_first_extra_munmap);
#endif
            hz3_s282_owner_inbox_on_extra_munmap();
            (void)hz3_large_s240_flush_dispose_node(hdr,
                                                    sc,
                                                    HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                    "inbox-take-first-batch-extra-munmap",
                                                    HZ3_S240_FLUSH_DRAIN_COUNTER);
        }
    } else {
        (void)extras;
        for (int i = 1; i < refill_limit; i++) {
            hz3_s285_on_take_first_extra_pop_attempt();
            Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
            if (!hdr) {
                hz3_s285_on_take_first_extra_pop_empty();
                break;
            }
            hz3_s285_on_take_first_extra_pop_hit();
            hdr->in_use = 0;
            hdr->next = NULL;
            int s285_front_ok = hz3_large_s240_front_push(sc, hdr);
            hz3_s285_on_take_first_extra_front_result(s285_front_ok);
            if (s285_front_ok) {
                drained++;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
                hz3_large_s240_inc(&g_s246_take_first_extra_fronted);
#endif
                hz3_s282_owner_inbox_on_extra_fronted();
                continue;
            }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
            hz3_large_s240_inc(&g_s246_take_first_extra_munmap);
#endif
            hz3_s282_owner_inbox_on_extra_munmap();
            (void)hz3_large_s240_flush_dispose_node(hdr,
                                                    sc,
                                                    HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                    "inbox-take-first-extra-munmap",
                                                    HZ3_S240_FLUSH_DRAIN_COUNTER);
        }
    }
#else
    (void)extras;
    for (int i = 1; i < refill_limit; i++) {
        hz3_s285_on_take_first_extra_pop_attempt();
        Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
        if (!hdr) {
            hz3_s285_on_take_first_extra_pop_empty();
            break;
        }
        hz3_s285_on_take_first_extra_pop_hit();
        hdr->in_use = 0;
        hdr->next = NULL;
        int s285_front_ok = hz3_large_s240_front_push(sc, hdr);
        hz3_s285_on_take_first_extra_front_result(s285_front_ok);
        if (s285_front_ok) {
            drained++;
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s246_take_first_extra_fronted);
#endif
            hz3_s282_owner_inbox_on_extra_fronted();
            continue;
        }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_inbox_drain_front_reject);
        hz3_large_s240_inc(&g_s246_take_first_extra_munmap);
#endif
        hz3_s282_owner_inbox_on_extra_munmap();
        (void)hz3_large_s240_flush_dispose_node(hdr,
                                                sc,
                                                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                                                "inbox-take-first-extra-munmap",
                                                HZ3_S240_FLUSH_DRAIN_COUNTER);
    }
#endif

#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_drain_nonempty);
    hz3_large_s240_add(&g_s240_inbox_drain_objs, (size_t)drained);
#endif
    hz3_s285_on_take_first_drain_nodes(drained);
    hz3_s282_owner_inbox_on_drain_nodes((size_t)drained);
    if (out_drained) {
        *out_drained = drained;
    }
    return first;
}
#endif

static inline void hz3_large_s240_inbox_flush_owner(uint8_t owner) {
    if (owner >= HZ3_NUM_SHARDS) {
        return;
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_inbox_flush_calls);
#endif
    uint32_t spins = 0;
    while (spins < (uint32_t)HZ3_S240_LARGE_INBOX_DTOR_SPIN_LIMIT &&
           atomic_load_explicit(&g_s240_large_inbox_pushers[owner], memory_order_acquire) != 0) {
        spins++;
        atomic_signal_fence(memory_order_seq_cst);
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    if (spins > 0) {
        hz3_large_s240_add(&g_s240_inbox_flush_wait_spins, (size_t)spins);
    }
#endif
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        for (;;) {
            Hz3LargeHdr* hdr = hz3_large_s240_inbox_pop_one(owner, sc);
            if (!hdr) {
                break;
            }
            hz3_s292_on_inbox_flush_seen(hdr);
            size_t bytes = hz3_large_s240_flush_dispose_node(
                hdr,
                sc,
                HZ3_S240_FLUSH_INBOX_POPPED_STATE,
                "inbox-flush-owner",
                HZ3_S240_FLUSH_INBOX_COUNTER);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_inbox_flush_objs);
            hz3_large_s240_add(&g_s240_inbox_flush_bytes, bytes);
#else
            (void)bytes;
#endif
        }
    }
}
#else
static inline int hz3_large_s240_inbox_push_remote(int sc, Hz3LargeHdr* hdr, uint8_t cur) {
    (void)sc;
    (void)hdr;
    (void)cur;
    return 0;
}

static inline int hz3_large_s240_inbox_drain_current_to_front(int sc, int max_nodes) {
    (void)sc;
    (void)max_nodes;
    return 0;
}

static inline void hz3_large_s240_inbox_flush_owner(uint8_t owner) {
    (void)owner;
}
#endif

void hz3_large_s240_tls_flush(void) {
#if HZ3_S240_LARGE_FRONT_CACHE
    hz3_large_s240_register_once();
#if HZ3_S240_LARGE_OWNER_INBOX
    if (t_hz3_cache.initialized) {
        hz3_large_s240_inbox_flush_owner((uint8_t)t_hz3_cache.my_shard);
    }
#endif
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_front_flush_calls);
#endif
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        Hz3LargeHdr* hdr = t_hz3_large_front.head[sc];
        t_hz3_large_front.head[sc] = NULL;
        t_hz3_large_front.count[sc] = 0;
        while (hdr) {
            Hz3LargeHdr* next = hdr->next_free;
            hz3_s292_on_front_flush_node(hdr);
            size_t bytes = hz3_large_s240_flush_dispose_node(
                hdr,
                sc,
                HZ3_LARGE_STATE_FRONT,
                "tls-front-flush",
                HZ3_S240_FLUSH_FRONT_COUNTER);
            hz3_large_s240_front_note_sub(bytes);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
            hz3_large_s240_inc(&g_s240_front_flush_objs);
            hz3_large_s240_inc(&g_s240_front_evict_munmap);
            hz3_large_s240_add(&g_s240_front_flush_bytes, bytes);
#endif
            hdr = next;
        }
    }
    t_hz3_large_front.bytes = 0;
#endif
}

