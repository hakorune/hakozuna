// =============================================================================
// hz3_owner_stash_push_list: Push a linked list to owner stash
// =============================================================================
int hz3_owner_stash_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n) {
    // Struct failfast: check ranges before anything
    STASH_CHECK_RANGE("push:entry", owner, sc);

#if HZ3_S96_OWNER_STASH_PUSH_FAILFAST
    if (!head || !tail || n == 0 || owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        fprintf(stderr,
                "[HZ3_S96_FAILFAST] where=stash_push owner=%u sc=%d head=%p tail=%p n=%u\n",
                owner, sc, head, tail, n);
        abort();
    }
#endif

    if (!head || !tail || n == 0) {
        return 0;
    }
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

    // Debug: verify list integrity before push
    STASH_CHECK_LIST("push_list:entry", head, tail, n);
#if HZ3_LIST_FAILFAST
    hz3_list_failfast("stash_push:entry", owner, sc, head, tail, n);
#endif
#if HZ3_S72_BOUNDARY_DEBUG
    hz3_small_v2_boundary_check_list("stash_push:entry", owner, sc, head, tail, n);
#endif

    hz3_owner_stash_init();

#if HZ3_S121_PAGE_LOCAL_REMOTE
    // S121: Page-local remote push
    // Walk object list, prepend each to its page->remote_head.
    S121_STAT_REGISTER();

    void* cur = head;
    while (cur) {
        void* next = hz3_obj_get_next(cur);

        // Get page header from object pointer
        Hz3S121PageHdr* page = (Hz3S121PageHdr*)(
            (uintptr_t)cur & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));

#if HZ3_S121_G_ATOMIC_PACK

#if HZ3_S121_G3_BRANCHLESS
        // S121-G3: Branchless state computation
        // Avoids branch misprediction by using arithmetic instead of conditionals
        uintptr_t oldv, newv;
        int need_notify;
        do {
            oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
            void* old_head = HZ3_REM_PTR(oldv);
            uint8_t old_state = HZ3_REM_STATE(oldv);

            hz3_obj_set_next(cur, old_head);

            // Branchless: is_empty & is_idle gives 1 only when both true
            int is_empty = (old_head == NULL);
            int is_idle = (old_state == HZ3_REM_STATE_IDLE);
            int transition = is_empty & is_idle;
            uint8_t new_state = old_state | (uint8_t)transition;
            newv = HZ3_REM_MAKE(cur, new_state);
            need_notify = transition;
        } while (!atomic_compare_exchange_weak_explicit(
            &page->remote_tagged, &oldv, newv,
            memory_order_release, memory_order_relaxed));

        if (need_notify) {
            hz3_pageq_push(owner, (uint8_t)sc, page);
            S121_STAT_INC(g_s121_page_state_0to1);
            S121_STAT_INC(g_s121_push_old_null);
        }

#elif HZ3_S121_G2_FAST_PATH
        // S121-G2: Fast path for non-empty pages (majority of pushes)
        // Key insight: most pushes are to non-empty pages where state doesn't change.
        // Split into fast path (no state computation) and slow path (state transition).
        uintptr_t oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
        void* old_head = HZ3_REM_PTR(oldv);

        if (__builtin_expect(old_head != NULL, 1)) {
            // Fast path: page is non-empty, just prepend (no state change)
            uintptr_t newv;
            do {
                hz3_obj_set_next(cur, old_head);
                // Keep same state bits, just update pointer
                newv = HZ3_REM_MAKE(cur, HZ3_REM_STATE(oldv));
            } while (!atomic_compare_exchange_weak_explicit(
                &page->remote_tagged, &oldv, newv,
                memory_order_release, memory_order_relaxed) &&
                (old_head = HZ3_REM_PTR(oldv), old_head != NULL));

            // If CAS failed and page became empty, fall through to slow path
            if (old_head != NULL) {
                goto g2_done;  // Success on fast path
            }
        }

        // Slow path: page was empty, need to check/transition state
        {
            uintptr_t newv;
            int need_notify = 0;
            do {
                uint8_t old_state = HZ3_REM_STATE(oldv);
                old_head = HZ3_REM_PTR(oldv);
                hz3_obj_set_next(cur, old_head);

                // Only transition 0→1 if truly empty
                uint8_t new_state = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE)
                                    ? HZ3_REM_STATE_ACTIVE : old_state;
                newv = HZ3_REM_MAKE(cur, new_state);
                need_notify = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE);
            } while (!atomic_compare_exchange_weak_explicit(
                &page->remote_tagged, &oldv, newv,
                memory_order_release, memory_order_relaxed));

            if (need_notify) {
#if HZ3_S121_D_PAGE_PACKET
                S121D_STAT_REGISTER();
                hz3_s121d_packet_add_page(owner, (uint8_t)sc, page);
#else
                hz3_pageq_push(owner, (uint8_t)sc, page);
#endif
                S121_STAT_INC(g_s121_page_state_0to1);
                S121_STAT_INC(g_s121_push_old_null);
            }
        }
g2_done:
        ;

#else  // !HZ3_S121_G2_FAST_PATH
        // S121-G: Single CAS handles prepend + state transition + notification decision
        uintptr_t oldv, newv;
        int need_notify = 0;
        do {
            oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
            void* old_head = HZ3_REM_PTR(oldv);
            uint8_t old_state = HZ3_REM_STATE(oldv);

            hz3_obj_set_next(cur, old_head);

            // IDLE(0)→ACTIVE(1) if empty, else keep current state
            uint8_t new_state = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE)
                                ? HZ3_REM_STATE_ACTIVE : old_state;
            newv = HZ3_REM_MAKE(cur, new_state);

            need_notify = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE);
        } while (!atomic_compare_exchange_weak_explicit(
            &page->remote_tagged, &oldv, newv,
            memory_order_release, memory_order_relaxed));

        // Notify after successful CAS (outside CAS loop)
        if (need_notify) {
            hz3_pageq_push(owner, (uint8_t)sc, page);
            S121_STAT_INC(g_s121_page_state_0to1);
            S121_STAT_INC(g_s121_push_old_null);
        }
#endif  // HZ3_S121_G2_FAST_PATH

#else
        // S121-C: Prepend to remote_head + exchange remote_state for notification
        void* old_head;
        do {
            old_head = atomic_load_explicit(&page->remote_head, memory_order_relaxed);
            hz3_obj_set_next(cur, old_head);
        } while (!atomic_compare_exchange_weak_explicit(
            &page->remote_head, &old_head, cur,
            memory_order_release, memory_order_relaxed));

        // Only enqueue when empty→non-empty transition
        if (old_head == NULL) {
            // Page was empty, try to enqueue
            uint8_t was = atomic_exchange_explicit(&page->remote_state, 1,
                                                   memory_order_acq_rel);
            if (was == 0) {
#if HZ3_S121_D_PAGE_PACKET
                // S121-D: Add to packet instead of direct pageq push
                S121D_STAT_REGISTER();
                hz3_s121d_packet_add_page(owner, (uint8_t)sc, page);
#else
                hz3_pageq_push(owner, (uint8_t)sc, page);
#endif
                S121_STAT_INC(g_s121_page_state_0to1);
            }
            S121_STAT_INC(g_s121_push_old_null);
        }
#endif  // HZ3_S121_G_ATOMIC_PACK

        cur = next;
        S121_STAT_INC(g_s121_stash_push_objs);
    }

    return 1;
#endif  // HZ3_S121_PAGE_LOCAL_REMOTE

#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Hash-based instance selection for push
    Hz3OwnerStashBin* bin = hz3_s144_get_bin_push(owner, sc, t_hz3_cache.owner_stash_seed);
#else
    Hz3OwnerStashBin* bin = &g_hz3_owner_stash[owner][sc];
#endif

#if HZ3_S96_OWNER_STASH_PUSH_STATS
    S96_STAT_REGISTER();
    S96_STAT_INC(g_s96_push_calls);
    S96_STAT_ADD(g_s96_push_objs_total, n);
    if (n == 1) {
        S96_STAT_INC(g_s96_push_n1_calls);
    }
    S96_STAT_MAX(g_s96_push_n_max, n);
    if (n == 1) {
        S96_STAT_INC(g_s96_push_hist_1);
    } else if (n == 2) {
        S96_STAT_INC(g_s96_push_hist_2);
    } else if (n <= 4) {
        S96_STAT_INC(g_s96_push_hist_3_4);
    } else if (n <= 8) {
        S96_STAT_INC(g_s96_push_hist_5_8);
    } else if (n <= 16) {
        S96_STAT_INC(g_s96_push_hist_9_16);
    } else if (n <= 32) {
        S96_STAT_INC(g_s96_push_hist_17_32);
    } else {
        S96_STAT_INC(g_s96_push_hist_33p);
    }
    if (sc < 32) {
        S96_STAT_ADD(g_s96_push_sc_lt32_objs, n);
    } else {
        S96_STAT_ADD(g_s96_push_sc_ge32_objs, n);
    }
#endif

#if HZ3_S96_OWNER_STASH_PUSH_SHOT
    if (__builtin_expect(!g_s96_push_shot_fired, 0)) {
        g_s96_push_shot_fired = 1;
        fprintf(stderr,
                "[HZ3_S96_PUSH_SHOT] owner=%u sc=%d n=%u head=%p tail=%p\n",
                owner, sc, n, head, tail);
    }
#endif

#if HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2
    S93_ATEXIT_ONCE();
    S93_STAT_INC(g_s93_push_calls);
    if (n == 1) {
        S93_STAT_INC(g_s93_push_calls_n1);
    }
    hz3_s93_stat_push_n_max(n);

// Avoid -Wtype-limits when MIN_N==0 (uint32_t n < 0 is always false).
#if HZ3_S93_PACKET_MIN_N > 0
    if (n < (uint32_t)HZ3_S93_PACKET_MIN_N) {
        S93_STAT_INC(g_s93_push_calls_skip_small_n);
        return 0;  // fallback (xfer/central)
    }
#endif

    uint32_t need_pkts = (n + (uint32_t)HZ3_S93_PACKET_K - 1u) / (uint32_t)HZ3_S93_PACKET_K;
    Hz3S93StashPacket* phead = NULL;
    Hz3S93StashPacket* ptail = NULL;

    // All-or-nothing: allocate all packets first.
    for (uint32_t i = 0; i < need_pkts; i++) {
        Hz3S93StashPacket* p = hz3_s93_pkt_alloc(owner);
        if (!p) {
            // Free already allocated packets and fallback (caller still owns the object list).
            while (phead) {
                Hz3S93StashPacket* next = phead->next;
                hz3_s93_pkt_free(owner, phead);
                phead = next;
            }
            return 0;
        }
        p->next = NULL;
        p->sc = (uint8_t)sc;
        p->n = 0;
        if (!phead) {
            phead = ptail = p;
        } else {
            ptail->next = p;
            ptail = p;
        }
    }

    // Fill packets by walking exactly `n` objects.
    //
    // Important: `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1` is a common perf setting
    // in r90_pf2 lanes; in that mode, the incoming list tail is not guaranteed to
    // have `tail->next == NULL`. Never walk until NULL here.
    void* cur = head;
    Hz3S93StashPacket* p = phead;
    for (uint32_t i = 0; i < n; i++) {
#if HZ3_S93_FAILFAST
        if (!p) {
            fprintf(stderr, "[HZ3_S93_FAILFAST] where=push_fill p=NULL owner=%u sc=%d n=%u\n",
                    owner, sc, n);
            abort();
        }
        if (!cur) {
            fprintf(stderr, "[HZ3_S93_FAILFAST] where=push_fill cur=NULL owner=%u sc=%d n=%u i=%u\n",
                    owner, sc, n, i);
            abort();
        }
#endif
        if (!cur) {
            // Corrupt list: free packets and fallback.
            while (phead) {
                Hz3S93StashPacket* next = phead->next;
                hz3_s93_pkt_free(owner, phead);
                phead = next;
            }
            return 0;
        }
        p->ptrs[p->n++] = cur;
        if (p->n == (uint16_t)HZ3_S93_PACKET_K) {
            p = p->next;
        }
        cur = hz3_obj_get_next(cur);
    }

    // Track underfill (r50 regression risk).
    S93_STAT_ADD(g_s93_push_packets, need_pkts);
    S93_STAT_ADD(g_s93_push_objs, n);
#if HZ3_S93_STATS
    for (Hz3S93StashPacket* q = phead; q; q = q->next) {
        if (q->n < (uint16_t)HZ3_S93_PACKET_K) {
            HZ3_DTOR_STAT_INC(g_s93_push_underfill_packets);
            uint32_t minv = atomic_load_explicit(&g_s93_push_underfill_min, memory_order_relaxed);
            uint32_t nn = (uint32_t)q->n;
            if (minv == 0 || nn < minv) {
                atomic_store_explicit(&g_s93_push_underfill_min, nn, memory_order_relaxed);
            }
        }
    }
#endif

    // MPSC push: prepend packet chain to bin head.
    void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
#if HZ3_S96_OWNER_STASH_PUSH_STATS
    uint32_t cas_retry = 0;
    do {
        ptail->next = (Hz3S93StashPacket*)old_head;
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, phead,
        memory_order_release, memory_order_relaxed) && ++cas_retry);

    if (cas_retry > 0) {
        S96_STAT_INC(g_s96_cas_retry_gt0_calls);
        S96_STAT_ADD(g_s96_cas_retry_total, cas_retry);
    }
    S96_STAT_MAX(g_s96_cas_retry_max, cas_retry);
#else
    do {
        ptail->next = (Hz3S93StashPacket*)old_head;
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, phead,
        memory_order_release, memory_order_relaxed));
#endif

    return 1;
#endif

#if !HZ3_S93_OWNER_STASH_PACKET && !HZ3_S93_OWNER_STASH_PACKET_V2
#if HZ3_S44_OWNER_STASH_COUNT
    // Check count before push (approximate, may race but OK)
    uint32_t current = atomic_load_explicit(&bin->count, memory_order_relaxed);
    if (current >= HZ3_S44_STASH_MAX_OBJS) {
        return 0;  // overflow -> caller falls back to xfer/central
    }
#endif

    // MPSC push: CAS loop to prepend list to head
    void* old_head_list = atomic_load_explicit(&bin->head, memory_order_relaxed);
#if HZ3_S96_OWNER_STASH_PUSH_STATS
    uint32_t cas_retry = 0;
    do {
        hz3_obj_set_next(tail, old_head_list);
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head_list, head,
        memory_order_release, memory_order_relaxed) && ++cas_retry);

    if (cas_retry > 0) {
        S96_STAT_INC(g_s96_cas_retry_gt0_calls);
        S96_STAT_ADD(g_s96_cas_retry_total, cas_retry);
    }
    S96_STAT_MAX(g_s96_cas_retry_max, cas_retry);
#else
    do {
        hz3_obj_set_next(tail, old_head_list);
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head_list, head,
        memory_order_release, memory_order_relaxed));
#endif

#if HZ3_S44_OWNER_STASH_COUNT
    // Increment count (approximate)
    atomic_fetch_add_explicit(&bin->count, n, memory_order_relaxed);
#endif
    return 1;
#endif  // !HZ3_S93_OWNER_STASH_PACKET && !HZ3_S93_OWNER_STASH_PACKET_V2
}

// =============================================================================
// S111: Push a single object (n==1 fastpath)
// =============================================================================
int hz3_owner_stash_push_one(uint8_t owner, int sc, void* obj) {
    // Range check
    STASH_CHECK_RANGE("push_one:entry", owner, sc);
    if (!obj || owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

    hz3_owner_stash_init();

#if HZ3_S121_PAGE_LOCAL_REMOTE
    // S121: Page-local remote push (single object)
    S121_STAT_REGISTER();

    // Get page header from object pointer
    Hz3S121PageHdr* page = (Hz3S121PageHdr*)(
        (uintptr_t)obj & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));

#if HZ3_S121_G_ATOMIC_PACK

#if HZ3_S121_G3_BRANCHLESS

#if HZ3_S121_L_PRECALC
    // S121-L: Pre-calculate state transition outside CAS loop
    // Motivation: perf shows 53.66% overhead in state calculation inside loop
    // Strategy: Only recalculate when CAS fails (state may have changed)
    uintptr_t oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
    void* old_head = HZ3_REM_PTR(oldv);
    uint8_t old_state = HZ3_REM_STATE(oldv);

    // Pre-calculate transition decision (branchless)
    int need_notify = (old_head == NULL) & (old_state == HZ3_REM_STATE_IDLE);
    uint8_t new_state = old_state | (uint8_t)need_notify;

    do {
        hz3_obj_set_next(obj, old_head);
        uintptr_t newv = HZ3_REM_MAKE(obj, new_state);
        if (__builtin_expect(atomic_compare_exchange_weak_explicit(
                &page->remote_tagged, &oldv, newv,
                memory_order_release, memory_order_relaxed), 1)) {
            break;  // CAS success
        }
        // CAS failed: recalculate state from updated oldv
        old_head = HZ3_REM_PTR(oldv);
        old_state = HZ3_REM_STATE(oldv);
        need_notify = (old_head == NULL) & (old_state == HZ3_REM_STATE_IDLE);
        new_state = old_state | (uint8_t)need_notify;
    } while (1);

#else  // !HZ3_S121_L_PRECALC
    // S121-G3: Branchless state computation (single object version)
    uintptr_t oldv, newv;
    int need_notify;
    do {
        oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
        void* old_head = HZ3_REM_PTR(oldv);
        uint8_t old_state = HZ3_REM_STATE(oldv);

        hz3_obj_set_next(obj, old_head);

        // Branchless: is_empty & is_idle gives 1 only when both true
        int is_empty = (old_head == NULL);
        int is_idle = (old_state == HZ3_REM_STATE_IDLE);
        int transition = is_empty & is_idle;
        uint8_t new_state = old_state | (uint8_t)transition;
        newv = HZ3_REM_MAKE(obj, new_state);
        need_notify = transition;
    } while (!atomic_compare_exchange_weak_explicit(
        &page->remote_tagged, &oldv, newv,
        memory_order_release, memory_order_relaxed));
#endif  // HZ3_S121_L_PRECALC

    if (need_notify) {
#if HZ3_S121_D_PAGE_PACKET
        S121D_STAT_REGISTER();
        hz3_s121d_packet_add_page(owner, (uint8_t)sc, page);
#elif HZ3_S121_M_PAGEQ_BATCH
        // S121-M: Batch pageq pushes to reduce CAS contention
        hz3_s121m_batch_add(&t_hz3_cache, owner, (uint8_t)sc, page);
#else
        hz3_pageq_push(owner, (uint8_t)sc, page);
#endif
        S121_STAT_INC(g_s121_page_state_0to1);
        S121_STAT_INC(g_s121_push_old_null);
    }

#elif HZ3_S121_G2_FAST_PATH
    // S121-G2: Fast path for non-empty pages (single object version)
    uintptr_t oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
    void* old_head = HZ3_REM_PTR(oldv);

    if (__builtin_expect(old_head != NULL, 1)) {
        // Fast path: page is non-empty, just prepend (no state change)
        uintptr_t newv;
        do {
            hz3_obj_set_next(obj, old_head);
            newv = HZ3_REM_MAKE(obj, HZ3_REM_STATE(oldv));
        } while (!atomic_compare_exchange_weak_explicit(
            &page->remote_tagged, &oldv, newv,
            memory_order_release, memory_order_relaxed) &&
            (old_head = HZ3_REM_PTR(oldv), old_head != NULL));

        if (old_head != NULL) {
            goto g2_push_one_done;  // Success on fast path
        }
    }

    // Slow path: page was empty, need to check/transition state
    {
        uintptr_t newv;
        int need_notify = 0;
        do {
            uint8_t old_state = HZ3_REM_STATE(oldv);
            old_head = HZ3_REM_PTR(oldv);
            hz3_obj_set_next(obj, old_head);

            uint8_t new_state = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE)
                                ? HZ3_REM_STATE_ACTIVE : old_state;
            newv = HZ3_REM_MAKE(obj, new_state);
            need_notify = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE);
        } while (!atomic_compare_exchange_weak_explicit(
            &page->remote_tagged, &oldv, newv,
            memory_order_release, memory_order_relaxed));

        if (need_notify) {
#if HZ3_S121_M_PAGEQ_BATCH
            hz3_s121m_batch_add(&t_hz3_cache, owner, (uint8_t)sc, page);
#else
            hz3_pageq_push(owner, (uint8_t)sc, page);
#endif
            S121_STAT_INC(g_s121_page_state_0to1);
            S121_STAT_INC(g_s121_push_old_null);
        }
    }
g2_push_one_done:
    ;

#else  // !HZ3_S121_G2_FAST_PATH
    // S121-G: Single CAS handles prepend + state transition + notification decision
    uintptr_t oldv, newv;
    int need_notify = 0;
    do {
        oldv = atomic_load_explicit(&page->remote_tagged, memory_order_relaxed);
        void* old_head = HZ3_REM_PTR(oldv);
        uint8_t old_state = HZ3_REM_STATE(oldv);

        hz3_obj_set_next(obj, old_head);

        // IDLE(0)→ACTIVE(1) if empty, else keep current state
        uint8_t new_state = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE)
                            ? HZ3_REM_STATE_ACTIVE : old_state;
        newv = HZ3_REM_MAKE(obj, new_state);

        need_notify = (old_head == NULL && old_state == HZ3_REM_STATE_IDLE);
    } while (!atomic_compare_exchange_weak_explicit(
        &page->remote_tagged, &oldv, newv,
        memory_order_release, memory_order_relaxed));

    // Notify after successful CAS (outside CAS loop)
    if (need_notify) {
#if HZ3_S121_D_PAGE_PACKET
        S121D_STAT_REGISTER();
        hz3_s121d_packet_add_page(owner, (uint8_t)sc, page);
#elif HZ3_S121_M_PAGEQ_BATCH
        hz3_s121m_batch_add(&t_hz3_cache, owner, (uint8_t)sc, page);
#else
        hz3_pageq_push(owner, (uint8_t)sc, page);
#endif
        S121_STAT_INC(g_s121_page_state_0to1);
        S121_STAT_INC(g_s121_push_old_null);
    }
#endif  // HZ3_S121_G2_FAST_PATH

#else
    // S121-C: Prepend to remote_head + exchange remote_state for notification
    void* old_head;
    do {
        old_head = atomic_load_explicit(&page->remote_head, memory_order_relaxed);
        hz3_obj_set_next(obj, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &page->remote_head, &old_head, obj,
        memory_order_release, memory_order_relaxed));

    // Only enqueue when empty→non-empty transition
    if (old_head == NULL) {
        // Page was empty, try to enqueue
        uint8_t was = atomic_exchange_explicit(&page->remote_state, 1,
                                               memory_order_acq_rel);
        if (was == 0) {
#if HZ3_S121_D_PAGE_PACKET
            // S121-D: Add to packet instead of direct pageq push
            S121D_STAT_REGISTER();
            hz3_s121d_packet_add_page(owner, (uint8_t)sc, page);
#elif HZ3_S121_M_PAGEQ_BATCH
            hz3_s121m_batch_add(&t_hz3_cache, owner, (uint8_t)sc, page);
#else
            hz3_pageq_push(owner, (uint8_t)sc, page);
#endif
            S121_STAT_INC(g_s121_page_state_0to1);
        }
        S121_STAT_INC(g_s121_push_old_null);
    }
#endif  // HZ3_S121_G_ATOMIC_PACK

    S121_STAT_INC(g_s121_stash_push_objs);
    return 1;

#else  // !HZ3_S121_PAGE_LOCAL_REMOTE

#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Hash-based instance selection for push
    Hz3OwnerStashBin* bin = hz3_s144_get_bin_push(owner, sc, t_hz3_cache.owner_stash_seed);
#else
    Hz3OwnerStashBin* bin = &g_hz3_owner_stash[owner][sc];
#endif

#if HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2
    // PacketBox: keep the stash homogeneous (packets only).
    // PushOne allocates a single packet instead of pushing raw objects.
    {
        Hz3S93StashPacket* p = hz3_s93_pkt_alloc(owner);
        if (!p) {
            return 0;
        }
        p->next = NULL;
        p->sc = (uint8_t)sc;
        p->n = 1;
        p->ptrs[0] = obj;

        void* old_head_pkt = atomic_load_explicit(&bin->head, memory_order_relaxed);
        do {
            p->next = (Hz3S93StashPacket*)old_head_pkt;
        } while (!atomic_compare_exchange_weak_explicit(
            &bin->head, &old_head_pkt, p,
            memory_order_release, memory_order_relaxed));
        return 1;
    }
#endif

#if HZ3_S44_OWNER_STASH_COUNT
    // Check count before push (approximate, may race but OK)
    uint32_t current = atomic_load_explicit(&bin->count, memory_order_relaxed);
    if (current >= HZ3_S44_STASH_MAX_OBJS) {
        return 0;  // overflow -> caller falls back to xfer/central
    }
#endif

    // MPSC push: CAS loop to prepend single object
    void* old_head = atomic_load_explicit(&bin->head, memory_order_relaxed);
    do {
        hz3_obj_set_next(obj, old_head);
    } while (!atomic_compare_exchange_weak_explicit(
        &bin->head, &old_head, obj,
        memory_order_release, memory_order_relaxed));

#if HZ3_S44_OWNER_STASH_COUNT
    atomic_fetch_add_explicit(&bin->count, 1, memory_order_relaxed);
#endif
    return 1;
#endif  // HZ3_S121_PAGE_LOCAL_REMOTE
}
