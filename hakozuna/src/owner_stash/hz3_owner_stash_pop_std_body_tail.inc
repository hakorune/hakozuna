#if HZ3_S67_SPILL_ARRAY2 && HZ3_S67_DRAIN_LIMIT && !HZ3_S112_FULL_DRAIN_EXCHANGE
    // S67-4: Bounded drain - detach only up to (need + spill_capacity) items
    {
        int need = want - got;
        if (need <= 0) {
            STASH_BOUNDARY_CHECK_OUT("stash_pop:bounded_need0", owner, sc, out, got);
            return got;
        }

        int available = (int)HZ3_S67_SPILL_CAP - (int)tc->spill_count[sc];
        if (available < 0) {
            available = 0;
        }

        int max_take = need + available;
        if (max_take < need) {
            max_take = need;
        }

        void* list = NULL;
        int list_n = 0;
        S67_STAT_INC(g_s67_bounded_drain_calls);

        for (;;) {
            void* head = atomic_load_explicit(headp, memory_order_acquire);
            if (!head) {
                S44_4_STAT_INC(g_s44_4_fastpop_miss_count);
                STASH_BOUNDARY_CHECK_OUT("stash_pop:bounded_head_null", owner, sc, out, got);
                return got;
            }

            void* cur = head;
            void* prev = NULL;
            int n = 0;
            while (cur && n < max_take) {
                prev = cur;
                cur = hz3_obj_get_next(cur);
                n++;
            }

            void* new_head = cur;  // remainder stays in stash
            if (atomic_compare_exchange_weak_explicit(
                    headp, &head, new_head,
                    memory_order_acquire, memory_order_relaxed)) {
                if (prev) {
                    hz3_obj_set_next(prev, NULL);  // detach list
                }
                list = head;
                list_n = n;
                break;
            }
            S67_STAT_INC(g_s67_bounded_drain_cas_retry_total);
        }

        if (!list || list_n == 0) {
            S44_4_STAT_INC(g_s44_4_fastpop_miss_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:bounded_empty", owner, sc, out, got);
            return got;
        }
        S67_STAT_ADD(g_s67_bounded_drain_detach_total, (uint32_t)list_n);
        S44_4_STAT_ADD(g_s44_4_drained_sum, (uint32_t)list_n);
        S44_4_STAT_MAX(g_s44_4_drained_max, (uint32_t)list_n);

        // Consume from list into out[] first, then spill_array (no overflow)
        void* cur = list;
        while (cur && got < want) {
            out[got++] = cur;
            cur = hz3_obj_get_next(cur);
        }

        uint8_t cnt = tc->spill_count[sc];
        while (cur && cnt < HZ3_S67_SPILL_CAP) {
            tc->spill_array[sc][cnt++] = cur;
            cur = hz3_obj_get_next(cur);
        }
        tc->spill_count[sc] = cnt;

        // Safety: should not happen; keep remainder by pushing back to stash
        if (cur) {
            S67_STAT_INC(g_s67_unexpected_remainder);
#if HZ3_S67_FAILFAST
            fprintf(stderr,
                    "[HZ3_S67_FAILFAST] where=stash_pop:bounded_unexpected owner=%u sc=%d cur=%p\n",
                    owner, sc, cur);
            abort();
#endif
            void* tail;
            uint32_t n;
            hz3_list_find_tail(cur, &tail, &n);
            void* old_head = atomic_load_explicit(headp, memory_order_relaxed);
            do {
                hz3_obj_set_next(tail, old_head);
            } while (!atomic_compare_exchange_weak_explicit(
                headp, &old_head, cur,
                memory_order_release, memory_order_relaxed));
        }

        STASH_BOUNDARY_CHECK_OUT("stash_pop:bounded", owner, sc, out, got);
        return got;
    }
#endif

    // Step 2: Drain from owner stash
#if HZ3_S44_BOUNDED_DRAIN
    // S44-BD: Bounded drain (CAS partial detach instead of full drain)
    // Reduces cache pollution by only taking what we need + slack
    int need = want - got;
    int extra = HZ3_S44_BOUNDED_EXTRA;
    int max_take = need + extra;
    if (max_take > need + 64) max_take = need + 64;  // clamp

    void* list = NULL;
    int bd_n_final = 0;  // S44-4: track drained length
    for (;;) {
        void* head = atomic_load_explicit(headp, memory_order_acquire);
        if (!head) {
            S44_4_STAT_INC(g_s44_4_fastpop_miss_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:bounded_empty", owner, sc, out, got);
            return got;
        }

        void* bd_cur = head;
        void* bd_prev = NULL;
        int bd_n = 0;
        while (bd_cur && bd_n < max_take) {
            bd_prev = bd_cur;
            bd_cur = hz3_obj_get_next(bd_cur);
            bd_n++;
        }

        void* new_head = bd_cur;  // remainder stays in stash
        if (atomic_compare_exchange_weak_explicit(
                headp, &head, new_head,
                memory_order_acquire, memory_order_relaxed)) {
            if (bd_prev) hz3_obj_set_next(bd_prev, NULL);  // detach list
            list = head;
            bd_n_final = bd_n;
            break;
        }
        // CAS failed -> retry
    }
    S44_4_STAT_ADD(g_s44_4_drained_sum, (uint32_t)bd_n_final);
    S44_4_STAT_MAX(g_s44_4_drained_max, (uint32_t)bd_n_final);
#else
    // Legacy: Atomic exchange to drain the entire list
    void* list = atomic_exchange_explicit(headp, NULL, memory_order_acquire);
    if (!list) {
        S44_4_STAT_INC(g_s44_4_fastpop_miss_count);
        STASH_BOUNDARY_CHECK_OUT("stash_pop:drain_empty", owner, sc, out, got);
        return got;
    }
#endif

    // Debug: verify drained list starts with valid pointer
    STASH_CHECK_PTR("pop_batch:drain", list);
#if HZ3_LIST_FAILFAST
    // Partial check: validate head node is in arena (we don't know tail/n for MPSC drain)
    // Note: Cannot use hz3_list_failfast with n=1 because drained list may have multiple nodes
    {
        uint32_t _page_idx;
        if (!hz3_arena_page_index_fast(list, &_page_idx)) {
            fprintf(stderr,
                    "[HZ3_LIST_FAILFAST] where=stash_pop:drained owner=%u sc=%d "
                    "head=%p out of arena\n",
                    owner, sc, list);
            abort();
        }
    }
#endif

    // Step 3: Walk and take up to `want` items (O(want))
#if HZ3_S44_4_STATS && !HZ3_S44_BOUNDED_DRAIN
    int got_before_walk = got;  // S44-4: track legacy drain taken count
#endif
#if HZ3_S44_PREFETCH && (HZ3_S44_PREFETCH_DIST >= 2) && HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE
    int s179_prefetch_dist2_on = ((want - got) >= (int)HZ3_S179_OWNER_STASH_PREFETCH_DIST2_MIN_NEED);
#endif
    void* cur = list;
#if HZ3_S44_4_WALK_NOPREV
    // S44-4: No prev tracking during walk - derive from out[got-1] in Step 4
    while (cur && got < want) {
        STASH_CHECK_PTR("pop_batch:walk", cur);
        void* next = hz3_obj_get_next(cur);
#if HZ3_S44_PREFETCH && (defined(__GNUC__) || defined(__clang__))
        if (next) {
            __builtin_prefetch(next, 0, 1);
#if HZ3_S44_PREFETCH_DIST >= 2
#if HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE
            if (s179_prefetch_dist2_on) {
#endif
                void* next2 = hz3_obj_get_next(next);
                if (next2) __builtin_prefetch(next2, 0, 1);
#if HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE
            }
#endif
#endif
        }
#endif
        out[got++] = cur;
        cur = next;
    }
#else
    void* prev = NULL;
    while (cur && got < want) {
        STASH_CHECK_PTR("pop_batch:walk", cur);
#if HZ3_S44_PREFETCH
        // S44-PF: Prefetch next pointer to hide memory latency
        void* next = hz3_obj_get_next(cur);
#if defined(__GNUC__) || defined(__clang__)
#if HZ3_S44_4_WALK_PREFETCH_UNCOND && (defined(__x86_64__) || defined(__i386__))
        __builtin_prefetch(next, 0, 1);  // x86: safe even if next==NULL
#if HZ3_S44_PREFETCH_DIST >= 2
        if (next
#if HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE
            && s179_prefetch_dist2_on
#endif
        ) {
            // dist=2: also prefetch next->next if available
            void* next2 = hz3_obj_get_next(next);
            if (next2) __builtin_prefetch(next2, 0, 1);
        }
#endif
#else
        if (next) {
            __builtin_prefetch(next, 0, 1);  // L2 cache, read
#if HZ3_S44_PREFETCH_DIST >= 2
            if (
#if HZ3_S179_OWNER_STASH_PREFETCH_NEED_GATE
                s179_prefetch_dist2_on
#else
                1
#endif
            ) {
                // dist=2: also prefetch next->next if available
                void* next2 = hz3_obj_get_next(next);
                if (next2) __builtin_prefetch(next2, 0, 1);
            }
#endif
        }
#endif
#endif
        out[got++] = cur;
        prev = cur;
        cur = next;
#else
        out[got++] = cur;
        prev = cur;
        cur = hz3_obj_get_next(cur);
#endif
    }
#endif

#if HZ3_S44_4_STATS && !HZ3_S44_BOUNDED_DRAIN
    // S44-4: Legacy drain stats - record taken count as approximation of drained length
    // (bounded drain already tracked bd_n_final or list_n)
    {
        int drained_taken = got - got_before_walk;
        S44_4_STAT_ADD(g_s44_4_drained_sum, (uint32_t)drained_taken);
        S44_4_STAT_MAX(g_s44_4_drained_max, (uint32_t)drained_taken);
    }
#endif

    // Step 4: Handle leftovers
    if (cur) {
        // Terminate the taken portion
#if HZ3_S44_4_WALK_NOPREV
        // S44-4: prev was not tracked, derive from out[got-1]
        if (got > 0) {
            hz3_obj_set_next(out[got - 1], NULL);
        }
#else
        if (prev) {
            hz3_obj_set_next(prev, NULL);
        }
#endif

#if HZ3_S94_SPILL_LITE
        // S94: SpillLite leftover handling for low sc
        if (sc < HZ3_S94_SPILL_SC_MAX) {
            Hz3TCache* tc94 = &t_hz3_cache;
            uint8_t spill_n = tc94->s94_spill_count[sc];
            uint32_t space = HZ3_S94_SPILL_CAP - spill_n;

            // Fill spill array up to CAP
            if (space > 0) {
                uint32_t fill = 0;
                while (cur && fill < space) {
                    tc94->s94_spill_array[sc][spill_n + fill] = cur;
                    cur = hz3_obj_get_next(cur);
                    fill++;
                }
                tc94->s94_spill_count[sc] = (uint8_t)(spill_n + fill);
            }

            // Remainder goes to overflow list
            if (cur) {
                void* old = tc94->s94_spill_overflow[sc];
#if HZ3_S95_S94_OVERFLOW_O1
                if (old == NULL) {
                    tc94->s94_spill_overflow[sc] = cur;
                    S94_STAT_INC(g_s94_overflow_set_calls);
                } else {
                    S94_STAT_INC(g_s94_overflow_old_nonnull);
#if HZ3_S95_S94_FAILFAST
                    fprintf(stderr,
                            "[HZ3_S95_FAILFAST] where=s94_overflow owner=%u sc=%d old=%p cur=%p\n",
                            owner, sc, old, cur);
                    abort();
#endif
                    {
                        void* overflow_tail = NULL;
                        uint32_t overflow_n = 0;
                        hz3_list_find_tail(cur, &overflow_tail, &overflow_n);
                        S94_STAT_INC(g_s94_overflow_tail_scan_calls);
                        hz3_obj_set_next(overflow_tail, old);
                        tc94->s94_spill_overflow[sc] = cur;
                        S94_STAT_INC(g_s94_overflow_set_calls);
                    }
                }
#else
                {
                    void* overflow_tail = NULL;
                    uint32_t overflow_n = 0;
                    if (old != NULL) {
                        S94_STAT_INC(g_s94_overflow_old_nonnull);
                    }
                    hz3_list_find_tail(cur, &overflow_tail, &overflow_n);
                    S94_STAT_INC(g_s94_overflow_tail_scan_calls);
                    hz3_obj_set_next(overflow_tail, old);
                    tc94->s94_spill_overflow[sc] = cur;
                    S94_STAT_INC(g_s94_overflow_set_calls);
                }
#endif
            }
            // S94 consumed all leftovers for low sc, skip other spill paths
            goto s94_leftover_done;
        }
#endif  // HZ3_S94_SPILL_LITE

#if HZ3_S67_SPILL_ARRAY2
        // S67-2: Fill array, remainder to overflow (bounded)
        {
            uint8_t cnt = tc->spill_count[sc];

            // Fill array (O(CAP) max)
            while (cur && cnt < HZ3_S67_SPILL_CAP) {
                tc->spill_array[sc][cnt++] = cur;
                cur = hz3_obj_get_next(cur);
            }
            tc->spill_count[sc] = cnt;

            // S67-3: Limit overflow to CAP items (bounded O(CAP) walk at pop)
            if (cur) {
                void* overflow_head = cur;
                void* overflow_tail = cur;
                uint32_t overflow_n = 1;
                cur = hz3_obj_get_next(cur);

                while (cur && overflow_n < HZ3_S67_SPILL_CAP) {
                    overflow_tail = cur;
                    overflow_n++;
                    cur = hz3_obj_get_next(cur);
                }

                // Terminate overflow list
                hz3_obj_set_next(overflow_tail, NULL);

                // Save bounded overflow
                tc->spill_overflow[sc] = overflow_head;

                // Any remainder beyond CAP is a bug; push back to stash
                if (cur) {
                    S67_STAT_INC(g_s67_unexpected_remainder);
#if HZ3_S67_FAILFAST
                    fprintf(stderr,
                            "[HZ3_S67_FAILFAST] where=stash_pop:overflow_unexpected owner=%u sc=%d cur=%p\n",
                            owner, sc, cur);
                    abort();
#endif
                    void* tail;
                    uint32_t n;
                    hz3_list_find_tail(cur, &tail, &n);
                    void* old_head = atomic_load_explicit(headp, memory_order_relaxed);
                    do {
                        hz3_obj_set_next(tail, old_head);
                    } while (!atomic_compare_exchange_weak_explicit(
                        headp, &old_head, cur,
                        memory_order_release, memory_order_relaxed));
                }
            }
        }
#elif HZ3_S67_SPILL_ARRAY
        // S67: Save leftovers to TLS spill array (O(CAP) max)
        {
            uint8_t cnt = tc->spill_count[sc];
            while (cur && cnt < HZ3_S67_SPILL_CAP) {
                tc->spill_array[sc][cnt++] = cur;
                cur = hz3_obj_get_next(cur);
            }
            tc->spill_count[sc] = cnt;

            // Overflow: push remaining back to owner stash (CAS)
            if (cur) {
#if HZ3_S67_SPILL_OVERFLOW_SHOT
                // One-shot debug log
                static _Atomic int shot = 0;
                if (atomic_exchange(&shot, 1) == 0) {
                    fprintf(stderr, "[HZ3_S67] spill_overflow sc=%d\n", sc);
                }
#endif
                // Find tail of remaining list
                void* tail;
                uint32_t n;
                hz3_list_find_tail(cur, &tail, &n);
                // CAS push back to owner stash head
                void* old_head = atomic_load_explicit(headp, memory_order_relaxed);
                do {
                    hz3_obj_set_next(tail, old_head);
                } while (!atomic_compare_exchange_weak_explicit(
                    headp, &old_head, cur,
                    memory_order_release, memory_order_relaxed));
            }
        }
#elif HZ3_S48_OWNER_STASH_SPILL
        // S48: Save to TLS spill (O(1), no stash push-back)
        // When S94 is enabled, S48 only handles sc >= SC_MAX
#if HZ3_S94_SPILL_LITE
        if (sc >= HZ3_S94_SPILL_SC_MAX) {
            Hz3TCache* tc48 = &t_hz3_cache;
            tc48->stash_spill[sc] = cur;
        }
        // else: low sc leftovers already handled by S94 path above
#else
        tc->stash_spill[sc] = cur;
#endif
#elif HZ3_S44_OWNER_STASH_FASTPOP
        // S44-3 Fast path: if old_head == NULL, skip tail traversal
        void* old_head = atomic_load_explicit(headp, memory_order_relaxed);
        if (old_head == NULL) {
            // CAS to set remaining as new head (O(1))
        if (atomic_compare_exchange_strong_explicit(
                headp, &old_head, cur,
                memory_order_release, memory_order_relaxed)) {
            // Success: COUNT=0 forced by compile-time guard
            STASH_BOUNDARY_CHECK_OUT("stash_pop:fastpop", owner, sc, out, got);
            return got;
        }
            // CAS failed: old_head was updated by concurrent push -> slow path
        }
        // Slow path: find tail and link to old_head
        void* remaining_tail;
        uint32_t remaining_n;
        hz3_list_find_tail(cur, &remaining_tail, &remaining_n);
        // CAS loop: old_head is auto-updated on CAS failure
        do {
            hz3_obj_set_next(remaining_tail, old_head);
        } while (!atomic_compare_exchange_weak_explicit(
            headp, &old_head, cur,
            memory_order_release, memory_order_relaxed));
        // COUNT=0 forced by compile-time guard
#else
        // Legacy: always traverse tail
        void* remaining_tail;
        uint32_t remaining_n;
        hz3_list_find_tail(cur, &remaining_tail, &remaining_n);

        // CAS loop to prepend remaining back to head
        void* old_head = atomic_load_explicit(headp, memory_order_relaxed);
        do {
            hz3_obj_set_next(remaining_tail, old_head);
        } while (!atomic_compare_exchange_weak_explicit(
            headp, &old_head, cur,
            memory_order_release, memory_order_relaxed));

#if HZ3_S44_OWNER_STASH_COUNT
        // Adjust count: decrement by got (we took `got`, put back the rest)
        atomic_fetch_sub_explicit(&bin->count, (uint32_t)got, memory_order_relaxed);
#endif
#endif
    }
#if !HZ3_S48_OWNER_STASH_SPILL && !HZ3_S67_SPILL_ARRAY && !HZ3_S67_SPILL_ARRAY2
    else {
        // Took entire list
#if HZ3_S44_OWNER_STASH_COUNT
        // count becomes 0 (approximate, may race)
        atomic_store_explicit(&bin->count, 0, memory_order_relaxed);
#endif
    }
#endif

#if HZ3_S94_SPILL_LITE
s94_leftover_done:
    (void)0;  // empty statement for label
#endif

    // Struct failfast: final bounds check before return
    STASH_CHECK_OUT_BOUNDS("pop:final", got, want, owner, sc);

    STASH_BOUNDARY_CHECK_OUT("stash_pop:final", owner, sc, out, got);
    return got;
