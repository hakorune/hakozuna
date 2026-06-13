#if HZ3_S240_LARGE_FLUSH_VALIDATE
static void hz3_large_s240_flush_debug_fail(const char* where,
                                            Hz3LargeHdr* hdr,
                                            const char* reason,
                                            size_t actual,
                                            size_t expected,
                                            _Atomic size_t* counter) {
    hz3_large_s240_register_once();
    hz3_large_s240_inc(&g_s240_flush_validate_failures);
    if (counter) {
        hz3_large_s240_inc(counter);
    }
    fprintf(stderr,
            "[HZ3_LARGE_S240_FLUSH_FAIL] where=%s reason=%s hdr=%p "
            "actual=%zu expected=%zu map_base=%p map_size=%zu user_ptr=%p "
            "in_use=%u next=%p next_free=%p state=%u owner=%u sc=%u flags=0x%x\n",
            where,
            reason,
            (void*)hdr,
            actual,
            expected,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0,
            hdr ? hdr->user_ptr : NULL,
            hdr ? (unsigned)hdr->in_use : 0u,
            hdr ? (void*)hdr->next : NULL,
            hdr ? (void*)hdr->next_free : NULL,
            hdr ? atomic_load_explicit(&hdr->state, memory_order_acquire) : 0u,
            hdr ? (unsigned)hdr->owner_shard : 255u,
            hdr ? (unsigned)hdr->sc : 255u,
            hdr ? (unsigned)hdr->flags : 0u);
#if HZ3_S240_LARGE_FLUSH_FAILFAST
    abort();
#endif
}

static int hz3_large_s240_flush_validate_node(Hz3LargeHdr* hdr,
                                              int sc,
                                              uint32_t expected_state,
                                              const char* where,
                                              _Atomic size_t* node_counter) {
    hz3_large_s240_register_once();
    hz3_large_s240_inc(&g_s240_flush_validate_calls);
    if (!hdr) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "null-node",
                                        0,
                                        1,
                                        &g_s240_flush_null_node_fail);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_base != (void*)hdr ||
        hdr->map_size == 0) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "bad-header",
                                        hdr->map_size,
                                        1,
                                        &g_s240_flush_bad_header);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (hdr->in_use != 0) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "in-use",
                                        hdr->in_use,
                                        0,
                                        &g_s240_flush_bad_header);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (hdr->next != NULL || hdr->next_free != NULL) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "linked-node",
                                        (size_t)(uintptr_t)(hdr->next ? hdr->next
                                                                      : hdr->next_free),
                                        0,
                                        &g_s240_flush_bad_link);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    uint32_t state = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (expected_state != 0 && state != expected_state) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "bad-state",
                                        state,
                                        expected_state,
                                        &g_s240_flush_bad_state);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT && hdr->sc != UINT8_MAX &&
        hdr->sc != (uint8_t)sc) {
        hz3_large_s240_flush_debug_fail(where,
                                        hdr,
                                        "sc-mismatch",
                                        hdr->sc,
                                        (uint8_t)sc,
                                        &g_s240_flush_sc_mismatch);
        if (node_counter) {
            hz3_large_s240_inc(node_counter);
        }
        return 0;
    }
    return 1;
}

static void hz3_large_s240_inbox_account_underflow(const char* where,
                                                   const char* reason,
                                                   size_t actual,
                                                   size_t expected,
                                                   _Atomic size_t* counter) {
    hz3_large_s240_flush_debug_fail(where, NULL, reason, actual, expected, counter);
}
#define HZ3_S240_FLUSH_FRONT_COUNTER (&g_s240_flush_front_node_fail)
#define HZ3_S240_FLUSH_INBOX_COUNTER (&g_s240_flush_inbox_node_fail)
#define HZ3_S240_FLUSH_DRAIN_COUNTER (&g_s240_flush_drain_node_fail)
#else
static inline int hz3_large_s240_flush_validate_node(Hz3LargeHdr* hdr,
                                                     int sc,
                                                     uint32_t expected_state,
                                                     const char* where,
                                                     _Atomic size_t* node_counter) {
    (void)hdr;
    (void)sc;
    (void)expected_state;
    (void)where;
    (void)node_counter;
    return 1;
}
#define HZ3_S240_FLUSH_FRONT_COUNTER NULL
#define HZ3_S240_FLUSH_INBOX_COUNTER NULL
#define HZ3_S240_FLUSH_DRAIN_COUNTER NULL
#endif

#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
#define HZ3_S240_FLUSH_INBOX_POPPED_STATE HZ3_LARGE_STATE_FREEING
#else
#define HZ3_S240_FLUSH_INBOX_POPPED_STATE HZ3_LARGE_STATE_REMOTE
#endif

static inline void hz3_large_s240_on_activate(Hz3LargeHdr* hdr, int sc, int from_cache) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    uint8_t owner = hz3_large_s240_current_owner();
    hdr->owner_shard = owner;
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    hdr->flags = 0;
    hdr->direct_slot = UINT32_MAX;
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_ACTIVE, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_activate);
    if (from_cache) {
        hz3_large_s240_inc(&g_s240_activate_cache);
    } else {
        hz3_large_s240_inc(&g_s240_activate_mmap);
    }
    if (owner == UINT8_MAX) {
        hz3_large_s240_inc(&g_s240_owner_unknown_activate);
    }
#endif
#else
    (void)hdr;
    (void)sc;
    (void)from_cache;
#endif
}

#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline void hz3_large_s276_note_front_take_try(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_try);
#endif
}

static inline void hz3_large_s276_note_front_take_hit(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_hit);
#endif
}

static inline void hz3_large_s276_note_front_take_not_owner(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_not_owner);
#endif
}

static inline void hz3_large_s276_note_front_take_not_eligible(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_not_eligible);
#endif
}

static inline void hz3_large_s276_note_front_take_nonactive(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_nonactive);
#endif
}

static inline void hz3_large_s276_note_front_take_state_cas_fail(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_take_state_cas_fail);
#endif
}

static inline void hz3_large_s276_note_front_push_retained(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_push_retained);
#endif
}

static inline void hz3_large_s276_note_front_push_reject_clear(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_push_reject_clear);
#endif
}

static inline void hz3_large_s276_note_front_activate_fast(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_activate_fast);
#endif
}

static inline void hz3_large_s276_note_front_activate_rekey(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_front_activate_rekey);
#endif
}

static inline void hz3_large_s276_note_inbox_take_hit(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_take_hit);
#endif
}

static inline void hz3_large_s276_note_inbox_push_retained(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_push_retained);
#endif
}

static inline void hz3_large_s276_note_inbox_push_reject_clear(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_push_reject_clear);
#endif
}

static inline void hz3_large_s276_note_inbox_activate_fast(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_inbox_activate_fast);
#endif
}

static inline void hz3_large_s276_note_direct_clear(void) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_STATS
    hz3_large_s240_inc(&g_s276_direct_clear);
#endif
}

static inline int hz3_large_s276_can_activate_retained(Hz3LargeHdr* hdr,
                                                       size_t size,
                                                       size_t alignment,
                                                       int sc) {
    hz3_s285_on_activate_check();
    if (!hdr ||
        hdr->magic != HZ3_LARGE_MAGIC ||
        hdr->map_base == NULL ||
        hdr->map_size == 0 ||
        hdr->user_ptr == NULL) {
        hz3_s285_on_activate_fail_header();
        return 0;
    }
    if (hdr->direct_slot == UINT32_MAX) {
        hz3_s285_on_activate_fail_slot();
        return 0;
    }
    if ((hdr->flags & HZ3_LARGE_F_DIRECT_MAPLESS) == 0 ||
        (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        hz3_s285_on_activate_fail_flag();
        return 0;
    }

    uint8_t observed_sc = hz3_large_s240_normalize_sc(sc);
    if (hdr->sc != observed_sc) {
        hz3_s285_on_activate_fail_sc();
        return 0;
    }
    if (alignment == 0 || (alignment & (alignment - 1u)) != 0) {
        hz3_s285_on_activate_fail_alignment();
        return 0;
    }
    uintptr_t user = (uintptr_t)hdr->user_ptr;
    if ((user & (uintptr_t)(alignment - 1u)) != 0) {
        hz3_s285_on_activate_fail_user_alignment();
        return 0;
    }
    if (size > SIZE_MAX - HZ3_LARGE_CANARY_SIZE) {
        hz3_s285_on_activate_fail_size();
        return 0;
    }
    uintptr_t base = (uintptr_t)hdr->map_base;
    uintptr_t end = base + hdr->map_size;
    if (end < base || user < base || user > end) {
        hz3_s285_on_activate_fail_range();
        return 0;
    }
#if HZ3_LARGE_CANARY_ENABLE
    size_t required = size + HZ3_LARGE_CANARY_SIZE;
#else
    size_t required = size;
#endif
    if (required > (size_t)(end - user)) {
        hz3_s285_on_activate_fail_size();
        return 0;
    }
    return 1;
}

static inline int hz3_large_s287_can_activate_retained_exact_fast(Hz3LargeHdr* hdr,
                                                                 size_t size,
                                                                 size_t alignment,
                                                                 int sc) {
#if HZ3_S287_LARGE_RETAINED_ACTIVATE_EXACT_FAST
    if (size < (size_t)HZ3_S287_LARGE_RETAINED_ACTIVATE_MIN_REQUEST_BYTES ||
        size > (size_t)HZ3_S287_LARGE_RETAINED_ACTIVATE_MAX_REQUEST_BYTES) {
        return 0;
    }
    if (!hdr ||
        hdr->magic != HZ3_LARGE_MAGIC ||
        hdr->map_base == NULL ||
        hdr->map_size == 0 ||
        hdr->user_ptr == NULL ||
        hdr->direct_slot == UINT32_MAX ||
        (hdr->flags & HZ3_LARGE_F_DIRECT_MAPLESS) == 0 ||
        (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        return 0;
    }
    if (hdr->sc != hz3_large_s240_normalize_sc(sc) || hdr->req_size != size) {
        return 0;
    }
    if (alignment == 0 || (alignment & (alignment - 1u)) != 0) {
        return 0;
    }
    return (((uintptr_t)hdr->user_ptr & (uintptr_t)(alignment - 1u)) == 0);
#else
    (void)hdr;
    (void)size;
    (void)alignment;
    (void)sc;
    return 0;
#endif
}

static inline void hz3_large_s276_on_activate_retained(Hz3LargeHdr* hdr,
                                                       int sc,
                                                       int from_cache) {
    hz3_large_s240_register_once();
    uint8_t owner = hz3_large_s240_current_owner();
    hdr->owner_shard = owner;
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    hdr->flags = (uint16_t)(HZ3_LARGE_F_DIRECT_MAPLESS | HZ3_LARGE_F_DIRECT_RETAINED);
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_ACTIVE, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_activate);
    if (from_cache) {
        hz3_large_s240_inc(&g_s240_activate_cache);
    } else {
        hz3_large_s240_inc(&g_s240_activate_mmap);
    }
    if (owner == UINT8_MAX) {
        hz3_large_s240_inc(&g_s240_owner_unknown_activate);
    }
#endif
}
#endif

static inline void hz3_large_s240_on_cache_seed(Hz3LargeHdr* hdr, int sc) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    hdr->owner_shard = UINT8_MAX;
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    hdr->flags = 0;
    hdr->direct_slot = UINT32_MAX;
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_GLOBAL, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_cache_seed);
#endif
#else
    (void)hdr;
    (void)sc;
#endif
}

static inline void hz3_large_s240_on_cache_store(Hz3LargeHdr* hdr, int sc) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    hdr->sc = hz3_large_s240_normalize_sc(sc);
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_GLOBAL, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_cache_store);
#endif
#else
    (void)hdr;
    (void)sc;
#endif
}

static inline void hz3_large_s240_on_munmap_store(Hz3LargeHdr* hdr) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_MUNMAP, memory_order_release);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_munmap_store);
#endif
#else
    (void)hdr;
#endif
}

static inline void hz3_large_s240_on_free_take(Hz3LargeHdr* hdr, int sc) {
#if HZ3_S240_LARGE_OWNER_FRONT
    hz3_large_s240_register_once();
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_free_seen);
#endif
    int s276_already_freeing = 0;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    if (hdr &&
        (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0 &&
        atomic_load_explicit(&hdr->state, memory_order_acquire) ==
            HZ3_LARGE_STATE_FREEING) {
        s276_already_freeing = 1;
    }
#endif
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    if (s276_already_freeing) {
        hz3_large_s240_inc(&g_s251_free_take_to_freeing);
    } else {
        hz3_large_s251_transition(hdr,
                                  HZ3_LARGE_STATE_ACTIVE,
                                  HZ3_LARGE_STATE_FREEING,
                                  "free_take",
                                  &g_s251_free_take_to_freeing,
                                  &g_s251_free_take_state_fail);
    }
    uint32_t state = HZ3_LARGE_STATE_ACTIVE;
#else
    uint32_t state = s276_already_freeing
        ? HZ3_LARGE_STATE_ACTIVE
        : atomic_load_explicit(&hdr->state, memory_order_acquire);
#endif
    if (state == HZ3_LARGE_STATE_ACTIVE) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_free_active);
#endif
    } else {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_state_mismatch);
#endif
    }
    uint8_t observed_sc = hz3_large_s240_normalize_sc(sc);
    if (hdr->sc != UINT8_MAX && hdr->sc != observed_sc) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_sc_mismatch);
#endif
    }
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    uint8_t cur = hz3_large_s240_current_owner();
    if (hdr->owner_shard == UINT8_MAX || cur == UINT8_MAX) {
        hz3_large_s240_inc(&g_s240_owner_unknown_free);
    } else if (hdr->owner_shard == cur) {
        hz3_large_s240_inc(&g_s240_owner_same_free);
    } else {
        hz3_large_s240_inc(&g_s240_owner_remote_free);
    }
#endif
#else
    (void)hdr;
    (void)sc;
#endif
}

#if HZ3_S240_LARGE_FRONT_CACHE
typedef struct Hz3LargeFront {
    Hz3LargeHdr* head[HZ3_LARGE_SC_COUNT];
    uint16_t count[HZ3_LARGE_SC_COUNT];
    size_t bytes;
} Hz3LargeFront;

static HZ3_TLS Hz3LargeFront t_hz3_large_front;

static inline int hz3_s291_highband_req_eligible(size_t req_size) {
#if HZ3_S291_HIGHBAND_OWNER_WINDOW
    return req_size > (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES &&
           req_size <= (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES;
#else
    (void)req_size;
    return 0;
#endif
}

static inline int hz3_large_s240_front_eligible(int sc, size_t map_size) {
    if (sc < 0 || sc >= (HZ3_LARGE_SC_COUNT - 1) || map_size == 0) {
        return 0;
    }
    return map_size <= (size_t)HZ3_S240_LARGE_FRONT_MAX_CLASS_BYTES;
}

static inline int hz3_large_s240_front_eligible_for_req(int sc,
                                                        size_t map_size,
                                                        size_t req_size) {
    if (hz3_large_s240_front_eligible(sc, map_size)) {
        return 1;
    }
    return hz3_s291_highband_req_eligible(req_size);
}

static inline void hz3_large_s240_front_note_add(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    size_t nodes = atomic_fetch_add_explicit(&g_s240_front_nodes_current, 1,
                                             memory_order_relaxed) + 1;
    size_t total = atomic_fetch_add_explicit(&g_s240_front_bytes_current, bytes,
                                             memory_order_relaxed) + bytes;
    hz3_large_s240_update_max(&g_s240_front_nodes_peak, nodes);
    hz3_large_s240_update_max(&g_s240_front_bytes_peak, total);
#else
    (void)bytes;
#endif
}

static inline void hz3_large_s240_front_note_sub(size_t bytes) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_sub(&g_s240_front_nodes_current, 1);
    hz3_large_s240_sub(&g_s240_front_bytes_current, bytes);
#else
    (void)bytes;
#endif
}

static inline Hz3LargeHdr* hz3_large_s240_front_pop(int sc, size_t need, size_t req_size) {
    hz3_large_s240_register_once();
    if (!hz3_large_s240_front_eligible_for_req(sc, need, req_size)) {
        hz3_s292_on_front_pop(req_size, 0, 0);
        return NULL;
    }
    Hz3LargeHdr* hdr = t_hz3_large_front.head[sc];
    if (!hdr || hdr->map_size < need) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_front_pop_miss);
#endif
        hz3_s292_on_front_pop(req_size, 0, 0);
        return NULL;
    }
    t_hz3_large_front.head[sc] = hdr->next_free;
    hdr->next_free = NULL;
    if (t_hz3_large_front.count[sc] > 0) {
        t_hz3_large_front.count[sc]--;
    }
    if (t_hz3_large_front.bytes >= hdr->map_size) {
        t_hz3_large_front.bytes -= hdr->map_size;
    } else {
        t_hz3_large_front.bytes = 0;
    }
    hz3_large_s240_front_note_sub(hdr->map_size);
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    hz3_large_s251_transition(hdr,
                              HZ3_LARGE_STATE_FRONT,
                              HZ3_LARGE_STATE_FREEING,
                              "front_pop",
                              &g_s251_front_pop_to_freeing,
                              &g_s251_front_pop_state_fail);
#endif
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_front_pop_hit);
#endif
    hz3_s292_on_front_pop(req_size, 1, hdr->map_size);
    return hdr;
}

static inline int hz3_large_s240_front_push(int sc, Hz3LargeHdr* hdr) {
    hz3_large_s240_register_once();
    if (!hz3_large_s240_front_eligible_for_req(sc, hdr->map_size, hdr->req_size)) {
        hz3_s292_on_front_push_result(hdr, 0);
        return 0;
    }
    if (t_hz3_large_front.count[sc] >= (uint16_t)HZ3_S240_LARGE_FRONT_MAX_NODES_PER_SC ||
        t_hz3_large_front.bytes + hdr->map_size > (size_t)HZ3_S240_LARGE_FRONT_MAX_BYTES_PER_THREAD) {
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
        hz3_large_s240_inc(&g_s240_front_reject_limit);
#endif
        hz3_s292_on_front_push_result(hdr, 0);
        return 0;
    }
    hdr->next = NULL;
    hdr->next_free = t_hz3_large_front.head[sc];
    t_hz3_large_front.head[sc] = hdr;
    t_hz3_large_front.count[sc]++;
    t_hz3_large_front.bytes += hdr->map_size;
#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
    hz3_large_s251_expect(hdr,
                          HZ3_LARGE_STATE_FREEING,
                          "front_push",
                          &g_s251_front_push_state_fail);
#endif
    atomic_store_explicit(&hdr->state, HZ3_LARGE_STATE_FRONT, memory_order_release);
    hz3_large_s240_front_note_add(hdr->map_size);
#if HZ3_S240_LARGE_OWNER_FRONT_STATS
    hz3_large_s240_inc(&g_s240_front_push);
#endif
    hz3_s292_on_front_push_result(hdr, 1);
    return 1;
}

static inline size_t hz3_large_s240_flush_dispose_node(Hz3LargeHdr* hdr,
                                                       int sc,
                                                       uint32_t expected_state,
                                                       const char* where,
                                                       _Atomic size_t* node_counter) {
    if (!hdr) {
        hz3_large_s240_flush_validate_node(hdr, sc, expected_state, where, node_counter);
        return 0;
    }
    hdr->next = NULL;
    hdr->next_free = NULL;
    hz3_large_s240_flush_validate_node(hdr, sc, expected_state, where, node_counter);
    size_t bytes = hdr->map_size;
    void* base = hdr->map_base;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    hz3_s276_direct_clear_retained(hdr);
#endif
    hz3_large_s240_on_munmap_store(hdr);
#if HZ3_LARGE_DEBUG
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(hdr);
    hz3_large_cache_lock_release();
#endif
    hdr->magic = 0;
#if HZ3_S240_LARGE_FLUSH_QUARANTINE
    hz3_large_s240_inc(&g_s240_flush_quarantine_objs);
    hz3_large_s240_add(&g_s240_flush_quarantine_bytes, bytes);
    (void)base;
#else
    hz3_large_os_munmap(base, bytes);
#endif
    return bytes;
}
#endif

