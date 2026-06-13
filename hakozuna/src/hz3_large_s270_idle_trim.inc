#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
static inline void hz3_large_sc_lock_acquire(int sc);
static inline void hz3_large_sc_lock_release(int sc);
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
static uint32_t g_s270_sc_pushes_since_pop[HZ3_LARGE_SC_COUNT];

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS
static _Atomic size_t g_s270_eligible_pushes = 0;
static _Atomic size_t g_s270_hot_skips = 0;
static _Atomic size_t g_s270_trigger_skips = 0;
static _Atomic size_t g_s270_trim_calls = 0;
static _Atomic size_t g_s270_trim_batches = 0;
static _Atomic size_t g_s270_trim_cap_hits = 0;
static _Atomic size_t g_s270_trimmed_nodes = 0;
static _Atomic size_t g_s270_trimmed_bytes = 0;
static _Atomic size_t g_s270_trimmed_nodes_by_sc[HZ3_LARGE_SC_COUNT];
static _Atomic size_t g_s270_trimmed_bytes_by_sc[HZ3_LARGE_SC_COUNT];
#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS
static _Atomic size_t g_s270_quarantine_victim_calls = 0;
static _Atomic size_t g_s270_quarantine_victim_bytes = 0;
#endif
#if HZ3_S270_DEBUG_VALIDATE
static _Atomic size_t g_s270_validate_calls = 0;
static _Atomic size_t g_s270_validate_failures = 0;
static _Atomic size_t g_s270_validate_node_failures = 0;
static _Atomic size_t g_s270_validate_count_mismatch = 0;
static _Atomic size_t g_s270_validate_bytes_mismatch = 0;
static _Atomic size_t g_s270_validate_cycle_failures = 0;
static _Atomic size_t g_s270_validate_long_walk_failures = 0;
static _Atomic size_t g_s270_validate_victim_still_linked = 0;
static _Atomic size_t g_s270_validate_victim_failures = 0;
static _Atomic size_t g_s270_dispose_victim_calls = 0;
#endif

static void hz3_s270_large_idle_trim_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S270_LARGE_SCACHE_IDLE_TRIM] enabled=1 sc_min=%d "
            "target_nodes=%u trigger_nodes=%u idle_pushes=%u batch_max=%u "
            "rearm_after_trim=%d stats=1 "
            "eligible_pushes=%zu hot_skips=%zu trigger_skips=%zu "
            "trim_calls=%zu trim_batches=%zu trim_cap_hits=%zu "
            "trimmed_nodes=%zu trimmed_bytes=%zu\n",
            HZ3_S270_SC_MIN,
            (unsigned)HZ3_S270_TARGET_NODES_PER_SC,
            (unsigned)HZ3_S270_TRIGGER_NODES_PER_SC,
            (unsigned)HZ3_S270_IDLE_PUSHES_PER_SC,
            (unsigned)HZ3_S270_TRIM_BATCH_MAX,
            HZ3_S270_REARM_AFTER_TRIM,
            atomic_load_explicit(&g_s270_eligible_pushes, memory_order_relaxed),
            atomic_load_explicit(&g_s270_hot_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trigger_skips, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trim_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trim_batches, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trim_cap_hits, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trimmed_nodes, memory_order_relaxed),
            atomic_load_explicit(&g_s270_trimmed_bytes, memory_order_relaxed));

    int emitted = 0;
    for (int sc = 0; sc < HZ3_LARGE_SC_COUNT; sc++) {
        size_t nodes =
            atomic_load_explicit(&g_s270_trimmed_nodes_by_sc[sc], memory_order_relaxed);
        size_t bytes =
            atomic_load_explicit(&g_s270_trimmed_bytes_by_sc[sc], memory_order_relaxed);
        if (nodes == 0 && bytes == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S270_LARGE_SCACHE_IDLE_TRIM_SC] sc=%d "
                "trimmed_nodes=%zu trimmed_bytes=%zu\n",
                sc,
                nodes,
                bytes);
        emitted = 1;
    }
    if (!emitted) {
        fprintf(stderr, "[HZ3_S270_LARGE_SCACHE_IDLE_TRIM_SC] empty=1\n");
    }
#if HZ3_S270_DEBUG_VALIDATE
    fprintf(stderr,
            "[HZ3_S270_DEBUG_VALIDATE] enabled=1 failfast=%d "
            "validate_calls=%zu failures=%zu node_failures=%zu "
            "count_mismatch=%zu bytes_mismatch=%zu cycle_failures=%zu "
            "long_walk_failures=%zu victim_still_linked=%zu victim_failures=%zu "
            "dispose_victim_calls=%zu\n",
            HZ3_S270_DEBUG_FAILFAST,
            atomic_load_explicit(&g_s270_validate_calls, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_node_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_count_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_bytes_mismatch, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_cycle_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_long_walk_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_victim_still_linked,
                                  memory_order_relaxed),
            atomic_load_explicit(&g_s270_validate_victim_failures, memory_order_relaxed),
            atomic_load_explicit(&g_s270_dispose_victim_calls, memory_order_relaxed));
#endif
#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS
    fprintf(stderr,
            "[HZ3_S270_DEBUG_QUARANTINE_VICTIMS] enabled=1 calls=%zu bytes=%zu\n",
            atomic_load_explicit(&g_s270_quarantine_victim_calls,
                                  memory_order_relaxed),
            atomic_load_explicit(&g_s270_quarantine_victim_bytes,
                                  memory_order_relaxed));
#endif
}

static inline void hz3_s270_register_once(void) {
    static _Atomic int registered = 0;
    if (atomic_exchange_explicit(&registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s270_large_idle_trim_dump_final);
    }
}

static inline void hz3_s270_note_eligible_push(void) {
    atomic_fetch_add_explicit(&g_s270_eligible_pushes, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_trigger_skip(void) {
    atomic_fetch_add_explicit(&g_s270_trigger_skips, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_hot_skip(void) {
    atomic_fetch_add_explicit(&g_s270_hot_skips, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_trim_call(void) {
    atomic_fetch_add_explicit(&g_s270_trim_calls, 1, memory_order_relaxed);
}

static inline void hz3_s270_note_trim_locked(int sc,
                                             size_t nodes,
                                             size_t bytes,
                                             int capped) {
    if (nodes == 0) {
        return;
    }
    atomic_fetch_add_explicit(&g_s270_trimmed_nodes, nodes, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trimmed_bytes, bytes, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trimmed_nodes_by_sc[sc],
                              nodes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trimmed_bytes_by_sc[sc],
                              bytes,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s270_trim_batches, 1, memory_order_relaxed);
    if (capped) {
        atomic_fetch_add_explicit(&g_s270_trim_cap_hits, 1, memory_order_relaxed);
    }
}
#else
static inline void hz3_s270_register_once(void) {
}

static inline void hz3_s270_note_eligible_push(void) {
}

static inline void hz3_s270_note_trigger_skip(void) {
}

static inline void hz3_s270_note_hot_skip(void) {
}

static inline void hz3_s270_note_trim_call(void) {
}

static inline void hz3_s270_note_trim_locked(int sc,
                                             size_t nodes,
                                             size_t bytes,
                                             int capped) {
    (void)sc;
    (void)nodes;
    (void)bytes;
    (void)capped;
}
#endif

#if HZ3_S270_DEBUG_VALIDATE
static void hz3_s270_debug_fail(const char* where,
                                int sc,
                                const char* reason,
                                const Hz3LargeHdr* hdr,
                                size_t actual,
                                size_t expected,
                                _Atomic size_t* counter) {
    atomic_fetch_add_explicit(&g_s270_validate_failures, 1, memory_order_relaxed);
    if (counter) {
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    }
    fprintf(stderr,
            "[HZ3_S270_DEBUG_FAIL] where=%s sc=%d reason=%s hdr=%p "
            "actual=%zu expected=%zu map_base=%p map_size=%zu user_ptr=%p "
            "in_use=%u next_free=%p",
            where,
            sc,
            reason,
            (const void*)hdr,
            actual,
            expected,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0,
            hdr ? hdr->user_ptr : NULL,
            hdr ? (unsigned)hdr->in_use : 0u,
            hdr ? (void*)hdr->next_free : NULL);
#if HZ3_S240_LARGE_OWNER_FRONT
    fprintf(stderr,
            " state=%u owner=%u hdr_sc=%u flags=0x%x",
            hdr ? atomic_load_explicit(&hdr->state, memory_order_acquire) : 0u,
            hdr ? (unsigned)hdr->owner_shard : 255u,
            hdr ? (unsigned)hdr->sc : 255u,
            hdr ? (unsigned)hdr->flags : 0u);
#endif
    fprintf(stderr, "\n");
#if HZ3_S270_DEBUG_FAILFAST
    abort();
#endif
}

static int hz3_s270_validate_node_locked(int sc,
                                         const Hz3LargeHdr* hdr,
                                         const char* where) {
    if (!hdr) {
        hz3_s270_debug_fail(where,
                            sc,
                            "null-node",
                            hdr,
                            0,
                            1,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->magic != HZ3_LARGE_MAGIC) {
        hz3_s270_debug_fail(where,
                            sc,
                            "bad-magic",
                            hdr,
                            (size_t)hdr->magic,
                            (size_t)HZ3_LARGE_MAGIC,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->map_base != (const void*)hdr || hdr->map_size == 0) {
        hz3_s270_debug_fail(where,
                            sc,
                            "bad-map",
                            hdr,
                            hdr->map_size,
                            1,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->in_use != 0) {
        hz3_s270_debug_fail(where,
                            sc,
                            "in-use-cache-node",
                            hdr,
                            hdr->in_use,
                            0,
                            &g_s270_validate_node_failures);
        return 0;
    }
#if HZ3_S240_LARGE_OWNER_FRONT
    uint32_t state = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (state != HZ3_LARGE_STATE_GLOBAL) {
        hz3_s270_debug_fail(where,
                            sc,
                            "non-global-state",
                            hdr,
                            state,
                            HZ3_LARGE_STATE_GLOBAL,
                            &g_s270_validate_node_failures);
        return 0;
    }
    if (hdr->sc != UINT8_MAX && hdr->sc != (uint8_t)sc) {
        hz3_s270_debug_fail(where,
                            sc,
                            "sc-mismatch",
                            hdr,
                            hdr->sc,
                            (uint8_t)sc,
                            &g_s270_validate_node_failures);
        return 0;
    }
#endif
    return 1;
}

static void hz3_s270_validate_list_locked(int sc, const char* where) {
    atomic_fetch_add_explicit(&g_s270_validate_calls, 1, memory_order_relaxed);
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT) {
        hz3_s270_debug_fail(where,
                            sc,
                            "bad-sc",
                            NULL,
                            (size_t)sc,
                            HZ3_LARGE_SC_COUNT,
                            &g_s270_validate_node_failures);
        return;
    }

    Hz3LargeHdr* slow = g_sc_head[sc];
    Hz3LargeHdr* fast = g_sc_head[sc];
    while (fast && fast->next_free) {
        slow = slow ? slow->next_free : NULL;
        fast = fast->next_free->next_free;
        if (slow && fast && slow == fast) {
            hz3_s270_debug_fail(where,
                                sc,
                                "cycle",
                                slow,
                                0,
                                0,
                                &g_s270_validate_cycle_failures);
            return;
        }
    }

    size_t nodes = 0;
    size_t bytes = 0;
    size_t limit = (size_t)g_sc_nodes[sc] + HZ3_S270_TRIM_BATCH_MAX + 1024u;
    if (limit < 1024u) {
        limit = 1024u;
    }

    for (Hz3LargeHdr* cur = g_sc_head[sc]; cur; cur = cur->next_free) {
        if (++nodes > limit) {
            hz3_s270_debug_fail(where,
                                sc,
                                "walk-limit",
                                cur,
                                nodes,
                                limit,
                                &g_s270_validate_long_walk_failures);
            return;
        }
        if (!hz3_s270_validate_node_locked(sc, cur, where)) {
            return;
        }
        if (bytes + cur->map_size < bytes) {
            hz3_s270_debug_fail(where,
                                sc,
                                "bytes-overflow",
                                cur,
                                bytes,
                                cur->map_size,
                                &g_s270_validate_bytes_mismatch);
            return;
        }
        bytes += cur->map_size;
    }

    if (nodes != (size_t)g_sc_nodes[sc]) {
        hz3_s270_debug_fail(where,
                            sc,
                            "node-count-mismatch",
                            g_sc_head[sc],
                            nodes,
                            g_sc_nodes[sc],
                            &g_s270_validate_count_mismatch);
        return;
    }
    if (bytes != g_sc_bytes[sc]) {
        hz3_s270_debug_fail(where,
                            sc,
                            "byte-count-mismatch",
                            g_sc_head[sc],
                            bytes,
                            g_sc_bytes[sc],
                            &g_s270_validate_bytes_mismatch);
    }
}

static int hz3_s270_list_contains_locked(int sc, const Hz3LargeHdr* needle) {
    size_t limit = (size_t)g_sc_nodes[sc] + HZ3_S270_TRIM_BATCH_MAX + 1024u;
    size_t n = 0;
    for (Hz3LargeHdr* cur = g_sc_head[sc]; cur; cur = cur->next_free) {
        if (++n > limit) {
            return 0;
        }
        if (cur == needle) {
            return 1;
        }
    }
    return 0;
}

static void hz3_s270_validate_victims_detached_locked(int sc,
                                                      Hz3LargeHdr** victims,
                                                      size_t count,
                                                      const char* where) {
    for (size_t i = 0; i < count; i++) {
        Hz3LargeHdr* victim = victims[i];
        if (!hz3_s270_validate_node_locked(sc, victim, where)) {
            continue;
        }
        if (victim->next_free != NULL) {
            hz3_s270_debug_fail(where,
                                sc,
                                "victim-next-not-null",
                                victim,
                                (size_t)(uintptr_t)victim->next_free,
                                0,
                                &g_s270_validate_victim_failures);
        }
        if (hz3_s270_list_contains_locked(sc, victim)) {
            hz3_s270_debug_fail(where,
                                sc,
                                "victim-still-linked",
                                victim,
                                i,
                                count,
                                &g_s270_validate_victim_still_linked);
        }
    }
}

static void hz3_s270_validate_victim_before_dispose(Hz3LargeHdr* victim,
                                                    const char* where) {
    atomic_fetch_add_explicit(&g_s270_dispose_victim_calls, 1, memory_order_relaxed);
    if (!victim) {
        hz3_s270_debug_fail(where,
                            -1,
                            "dispose-null-victim",
                            victim,
                            0,
                            1,
                            &g_s270_validate_victim_failures);
        return;
    }
    int sc = -1;
#if HZ3_S240_LARGE_OWNER_FRONT
    sc = (victim->sc == UINT8_MAX) ? -1 : (int)victim->sc;
#endif
    (void)hz3_s270_validate_node_locked(sc, victim, where);
    if (victim->next_free != NULL) {
        hz3_s270_debug_fail(where,
                            sc,
                            "dispose-victim-next-not-null",
                            victim,
                            (size_t)(uintptr_t)victim->next_free,
                            0,
                            &g_s270_validate_victim_failures);
    }
}
#else
static inline void hz3_s270_validate_list_locked(int sc, const char* where) {
    (void)sc;
    (void)where;
}

static inline void hz3_s270_validate_victims_detached_locked(int sc,
                                                             Hz3LargeHdr** victims,
                                                             size_t count,
                                                             const char* where) {
    (void)sc;
    (void)victims;
    (void)count;
    (void)where;
}

static inline void hz3_s270_validate_victim_before_dispose(Hz3LargeHdr* victim,
                                                           const char* where) {
    (void)victim;
    (void)where;
}
#endif

static inline int hz3_s270_trim_sc(int sc) {
    return (sc >= HZ3_S270_SC_MIN && sc < (HZ3_LARGE_SC_COUNT - 1));
}

static inline void hz3_s270_on_scache_pop_locked(int sc) {
    if (!hz3_s270_trim_sc(sc)) {
        return;
    }
    g_s270_sc_pushes_since_pop[sc] = 0;
}

static inline void hz3_s270_on_scache_push_locked(int sc) {
    if (!hz3_s270_trim_sc(sc)) {
        return;
    }
    hz3_s270_register_once();
    g_s270_sc_pushes_since_pop[sc]++;
}

static size_t hz3_s270_collect_idle_trim_locked(int sc,
                                                Hz3LargeHdr** victims,
                                                size_t cap) {
    if (!hz3_s270_trim_sc(sc) || cap == 0) {
        return 0;
    }
    hz3_s270_register_once();
    hz3_s270_note_eligible_push();

    if (g_sc_nodes[sc] <= (uint32_t)HZ3_S270_TRIGGER_NODES_PER_SC) {
        hz3_s270_note_trigger_skip();
        return 0;
    }
    if (g_s270_sc_pushes_since_pop[sc] < (uint32_t)HZ3_S270_IDLE_PUSHES_PER_SC) {
        hz3_s270_note_hot_skip();
        return 0;
    }

    hz3_s270_note_trim_call();
    hz3_s270_validate_list_locked(sc, "s270-pre-trim");

    uint32_t keep = (uint32_t)HZ3_S270_TARGET_NODES_PER_SC;
    if (keep >= g_sc_nodes[sc]) {
        return 0;
    }

    Hz3LargeHdr* keep_tail = g_sc_head[sc];
    for (uint32_t i = 1; keep_tail && i < keep; i++) {
        keep_tail = keep_tail->next_free;
    }
    if (!keep_tail || !keep_tail->next_free) {
        return 0;
    }

    Hz3LargeHdr* cur = keep_tail->next_free;
    keep_tail->next_free = NULL;

    size_t n = 0;
    size_t bytes = 0;
    while (cur && n < cap) {
        Hz3LargeHdr* next = cur->next_free;
        cur->next_free = NULL;
        victims[n++] = cur;
        bytes += cur->map_size;
        g_sc_bytes[sc] -= cur->map_size;
        g_sc_nodes[sc]--;
        hz3_large_total_cached_sub(cur->map_size);
        hz3_s267_on_scache_pop(sc, cur->map_size);
        hz3_large_debug_on_cache_remove_locked(cur);
        cur = next;
    }
    if (cur) {
        keep_tail->next_free = cur;
    }

    if (HZ3_S270_REARM_AFTER_TRIM &&
        g_sc_nodes[sc] > (uint32_t)HZ3_S270_TARGET_NODES_PER_SC) {
        g_s270_sc_pushes_since_pop[sc] =
            (uint32_t)HZ3_S270_IDLE_PUSHES_PER_SC - 1u;
    } else {
        g_s270_sc_pushes_since_pop[sc] = 0;
    }
    hz3_s270_note_trim_locked(sc, n, bytes, cur != NULL);
    hz3_s270_validate_victims_detached_locked(sc,
                                              victims,
                                              n,
                                              "s270-post-detach-victims");
    hz3_s270_validate_list_locked(sc, "s270-post-trim");
    return n;
}

static size_t hz3_s270_collect_idle_trim(int sc, Hz3LargeHdr** victims, size_t cap) {
    size_t n;
    hz3_large_sc_lock_acquire(sc);
    n = hz3_s270_collect_idle_trim_locked(sc, victims, cap);
    hz3_large_sc_lock_release(sc);
    return n;
}
#else
static inline void hz3_s270_on_scache_pop_locked(int sc) {
    (void)sc;
}
static inline void hz3_s270_on_scache_push_locked(int sc) {
    (void)sc;
}
static inline size_t hz3_s270_collect_idle_trim(int sc,
                                                Hz3LargeHdr** victims,
                                                size_t cap) {
    (void)sc;
    (void)victims;
    (void)cap;
    return 0;
}
#endif
