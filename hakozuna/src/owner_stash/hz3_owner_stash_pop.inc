// =============================================================================
// hz3_owner_stash_pop_batch: Pop up to `want` objects from owner stash
// =============================================================================
int hz3_owner_stash_pop_batch(uint8_t owner, int sc, void** out, int want) {
    // Struct failfast: check ranges before anything
    STASH_CHECK_RANGE("pop:entry", owner, sc);

    if (want <= 0) {
        return 0;
    }
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }

#if HZ3_S162_OWNER_STASH_INIT_FASTPATH
    if (!t_hz3_cache.initialized) {
        hz3_owner_stash_init();
    }
#else
    hz3_owner_stash_init();
#endif

#if HZ3_S121_PAGE_LOCAL_REMOTE
    // S121-C: Page-Local Remote pop path with pageq notification
    // 1. Check TLS spill first
    // 2. Pop pages from pageq, drain page->remote_head
    // 3. Store leftovers in TLS spill
    {
        S121_STAT_REGISTER();
        S121_STAT_INC(g_s121_stash_pop_calls);

        int got = 0;
        Hz3TCache* tc = &t_hz3_cache;

        // Step 0a: Drain TLS spill_array first (O(1) memcpy)
        uint8_t cnt = tc->spill_count[sc];
        if (cnt > 0) {
            int take = (cnt > (uint8_t)want) ? want : (int)cnt;
            // Copy from end of array (LIFO)
            memcpy(out, &tc->spill_array[sc][cnt - take], (size_t)take * sizeof(void*));
            tc->spill_count[sc] = (uint8_t)(cnt - take);
            got = take;
            if (got >= want) {
                S121_STAT_ADD(g_s121_stash_pop_objs, got);
                return got;
            }
        }

        // Step 0b: Drain TLS spill_overflow
        void* ov = tc->spill_overflow[sc];
        while (ov && got < want) {
            out[got++] = ov;
            void* next_ov = hz3_obj_get_next(ov);
#if HZ3_S154_SPILL_PREFETCH && (defined(__GNUC__) || defined(__clang__))
            if (next_ov) {
                __builtin_prefetch(next_ov, 0, 3);
            }
#endif
            ov = next_ov;
        }
        tc->spill_overflow[sc] = ov;
        if (got >= want) {
            S121_STAT_ADD(g_s121_stash_pop_objs, got);
            return got;
        }

        // Step 1: S121-C drain-all - take entire pageq with single atomic_exchange
        // This eliminates CAS retry contention between pop and push

        // Requeue list for pages that need to go back
        Hz3S121PageHdr* requeue_head = NULL;
        Hz3S121PageHdr* requeue_tail = NULL;

        Hz3S121PageHdr* page_list = (Hz3S121PageHdr*)hz3_pageq_drain_all(owner, (uint8_t)sc);

        // Track drain result
        if (!page_list) {
            S121_STAT_INC(g_s121_drain_empty_calls);
        } else {
            S121_STAT_INC(g_s121_drain_success_calls);
        }

        {
        // Process drained pages
        Hz3S121PageHdr* page = page_list;
        while (page) {
            Hz3S121PageHdr* next_page = (Hz3S121PageHdr*)page->page_qnext;
            S121_STAT_INC(g_s121_pageq_deq);

#if HZ3_S121_G_ATOMIC_PACK
#if HZ3_S121_J_CLEAR_STATE_ON_DEQ
            // S121-J + S121-G: Exchange with (NULL, IDLE) immediately
            uintptr_t old_tagged = atomic_exchange_explicit(
                &page->remote_tagged, HZ3_REM_MAKE(NULL, HZ3_REM_STATE_IDLE),
                memory_order_acq_rel);
            void* list = HZ3_REM_PTR(old_tagged);
#else
            // S121-G: Exchange with (NULL, ACTIVE) to drain while keeping state
            uintptr_t old_tagged = atomic_exchange_explicit(
                &page->remote_tagged, HZ3_REM_MAKE(NULL, HZ3_REM_STATE_ACTIVE),
                memory_order_acq_rel);
            void* list = HZ3_REM_PTR(old_tagged);
#endif
#else
#if HZ3_S121_J_CLEAR_STATE_ON_DEQ
            // S121-J: clear state before drain
            atomic_store_explicit(&page->remote_state, 0, memory_order_release);
            S121_STAT_INC(g_s121_page_state_1to0);

            // Pop path: conditional exchange (avoid RMW if empty)
            void* list = NULL;
            {
                // load-acquire to check empty
                void* head = atomic_load_explicit(&page->remote_head, memory_order_acquire);
                if (head != NULL) {
                    // exchange-acquire to take the list
                    list = atomic_exchange_explicit(&page->remote_head, NULL, memory_order_acquire);
                    // Note: head was non-null, so list should get something
                    (void)head;  // suppress unused warning
                }
            }
#else
            // S121-C: unconditional exchange for drain
            void* list = atomic_exchange_explicit(&page->remote_head, NULL, memory_order_acquire);
#endif
#endif

            // 3. Fill out[] with needed objects, rest to spill
            int page_got = 0;
            void* cur = list;
            // Fill out[] first
            while (cur && got < want) {
                out[got++] = cur;
                cur = hz3_obj_get_next(cur);
                page_got++;
            }

            // 4. ALL remaining objects from this page go to TLS spill
            if (cur) {
                // First fill spill_array
                uint8_t scnt = tc->spill_count[sc];
                while (cur && scnt < HZ3_S67_SPILL_CAP) {
                    tc->spill_array[sc][scnt++] = cur;
                    cur = hz3_obj_get_next(cur);
                    page_got++;
                }
                tc->spill_count[sc] = scnt;

                // Remainder to spill_overflow
                if (cur) {
                    // Find tail and prepend to existing overflow
                    void* tail = cur;
                    while (hz3_obj_get_next(tail)) {
                        tail = hz3_obj_get_next(tail);
                        page_got++;
                    }
                    page_got++;  // count tail itself
                    hz3_obj_set_next(tail, tc->spill_overflow[sc]);
                    tc->spill_overflow[sc] = cur;
                }
            }

            S121_STAT_ADD(g_s121_page_drain_objs_total, page_got);
            S121_STAT_MAX(g_s121_page_drain_objs_max, page_got);

            // 5. Lost wakeup prevention: reset state and double-check
#if HZ3_S121_J_CLEAR_STATE_ON_DEQ
            // S121-J: no double-check, no requeue
            // Push side will pageq_push if needed
#elif HZ3_S121_G_ATOMIC_PACK
            // S121-G: CAS from (NULL, ACTIVE) to (NULL, IDLE)
            // After drain, try to transition state back to IDLE
            uintptr_t expect_empty = HZ3_REM_MAKE(NULL, HZ3_REM_STATE_ACTIVE);
            uintptr_t desire_idle = HZ3_REM_MAKE(NULL, HZ3_REM_STATE_IDLE);

            if (atomic_compare_exchange_strong_explicit(
                    &page->remote_tagged, &expect_empty, desire_idle,
                    memory_order_acq_rel, memory_order_relaxed)) {
                // Successfully transitioned to IDLE, page is done
                S121_STAT_INC(g_s121_page_state_1to0);
            } else {
                // CAS failed: concurrent push arrived (ptr != NULL) or state changed
                // Requeue the page for next drain cycle
                page->page_qnext = NULL;
                if (!requeue_head) {
                    requeue_head = requeue_tail = page;
                } else {
                    requeue_tail->page_qnext = page;
                    requeue_tail = page;
                }
                S121_STAT_INC(g_s121_page_requeue);
            }
#else
            if (atomic_load_explicit(&page->remote_head, memory_order_acquire) == NULL) {
                // Original S121-C: transition state 1→0 immediately
                atomic_store_explicit(&page->remote_state, 0, memory_order_release);
                S121_STAT_INC(g_s121_page_state_1to0);

                // Double-check: concurrent push may have arrived
                if (atomic_load_explicit(&page->remote_head, memory_order_acquire) != NULL) {
                    uint8_t expected = 0;
                    if (atomic_compare_exchange_strong_explicit(
                            &page->remote_state, &expected, 1,
                            memory_order_acq_rel, memory_order_relaxed)) {
                        // Add to requeue list instead of immediate push
                        page->page_qnext = NULL;
                        if (!requeue_head) {
                            requeue_head = requeue_tail = page;
                        } else {
                            requeue_tail->page_qnext = page;
                            requeue_tail = page;
                        }
                        S121_STAT_INC(g_s121_page_requeue);
                    }
                }
            } else {
                // Page still has items (concurrent push during drain), add to requeue list
                page->page_qnext = NULL;
                if (!requeue_head) {
                    requeue_head = requeue_tail = page;
                } else {
                    requeue_tail->page_qnext = page;
                    requeue_tail = page;
                }
                S121_STAT_INC(g_s121_page_requeue);
            }
#endif

            page = next_page;
        }  // end while (page)

        }  // end drain block

        // Phase 3: Batch requeue all pages that got new pushes during drain
        if (requeue_head) {
            hz3_pageq_push_list(owner, (uint8_t)sc, requeue_head, requeue_tail);
        }

        S121_STAT_ADD(g_s121_stash_pop_objs, got);
        return got;
    }

#endif  // HZ3_S121_PAGE_LOCAL_REMOTE

#if HZ3_S67_STATS
    HZ3_DTOR_STAT_INC(g_s67_pop_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s67, hz3_s67_atexit_dump);
#endif
#if HZ3_S112_STATS
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s112, hz3_s112_atexit_dump);
#endif
#if HZ3_S170_S112_BOUNDED_STATS
    S170_STAT_REGISTER();
#endif
#if HZ3_S94_STATS
    if (sc < HZ3_S94_SPILL_SC_MAX) {
        S94_STAT_INC(g_s94_pop_calls);
        S94_STAT_REGISTER();
    }
#endif
#if HZ3_S44_4_STATS
    S44_4_STAT_REGISTER();
    S44_4_STAT_INC(g_s44_4_pop_calls);
    S44_4_STAT_ADD(g_s44_4_want_sum32, (uint32_t)want);
    if (want == 32) {
        S44_4_STAT_INC(g_s44_4_want_32_count);
    }
#endif

#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Round-robin instance selection for pop (try 1 instance only)
    Hz3TCache* tc_s144 = &t_hz3_cache;
    Hz3OwnerStashBin* bin = hz3_s144_get_bin_pop(owner, sc, &tc_s144->owner_stash_rr);
#else
    Hz3OwnerStashBin* bin = &g_hz3_owner_stash[owner][sc];
#endif

    int got = 0;  // S48/S67/S67-2: defined at function start

    _Atomic(void*)* headp = &bin->head;

#if HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2
    S93_ATEXIT_ONCE();
    S93_STAT_INC(g_s93_pop_calls);

    Hz3TCache* tc = &t_hz3_cache;

#if HZ3_S93_OWNER_STASH_PACKET_V2
    // Step 0: consume per-sc spill (S67-2 spill array + overflow list).
    {
        uint8_t spill_n = tc->spill_count[sc];
        if (spill_n > 0) {
            int take = (spill_n < (uint8_t)want) ? (int)spill_n : want;
            uint32_t start = (uint32_t)spill_n - (uint32_t)take;
            memcpy(out + got, &tc->spill_array[sc][start], (size_t)take * sizeof(void*));
            tc->spill_count[sc] = (uint8_t)start;
            got += take;
            if (got >= want) {
                STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_v2_spill_array", owner, sc, out, got);
                return got;
            }
        }

        void* ov = tc->spill_overflow[sc];
        while (ov && got < want) {
            out[got++] = ov;
            void* next_ov = hz3_obj_get_next(ov);
#if HZ3_S154_SPILL_PREFETCH && (defined(__GNUC__) || defined(__clang__))
            if (next_ov) {
                __builtin_prefetch(next_ov, 0, 3);
            }
#endif
            ov = next_ov;
        }
        tc->spill_overflow[sc] = ov;
        if (got >= want) {
            STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_v2_spill_overflow", owner, sc, out, got);
            return got;
        }
    }
#else
    // Step 0: consume per-sc overflow packet list (stored in stash_spill[sc]).
    Hz3S93StashPacket* ov = (Hz3S93StashPacket*)tc->stash_spill[sc];
    while (ov && got < want) {
        Hz3S93StashPacket* next = ov->next;
#if HZ3_S93_FAILFAST
        if (ov->sc != (uint8_t)sc) {
            fprintf(stderr, "[HZ3_S93_FAILFAST] where=pop:spill_sc_mismatch owner=%u want_sc=%d pkt_sc=%u\n",
                    owner, sc, (unsigned)ov->sc);
            abort();
        }
#endif
        int need = want - got;
        int take = (ov->n < (uint16_t)need) ? (int)ov->n : need;
        memcpy(out + got, ov->ptrs, (size_t)take * sizeof(void*));
        got += take;
        ov->n = (uint16_t)(ov->n - (uint16_t)take);
        if (ov->n == 0) {
            hz3_s93_pkt_free(owner, ov);
            ov = next;
            continue;
        }
        // Partial: shift remaining pointers to front and keep packet as new overflow head.
        memmove(ov->ptrs, &ov->ptrs[take], (size_t)ov->n * sizeof(void*));
        ov->next = next;
        tc->stash_spill[sc] = ov;
        S93_STAT_INC(g_s93_pop_partial_packets);
        STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_spill_partial", owner, sc, out, got);
        return got;
    }
    tc->stash_spill[sc] = ov;  // remainder or NULL
#endif  // HZ3_S93_OWNER_STASH_PACKET_V2

    // Step 1: drain stash packets.
    //
    // V2 uses S67-2 spill (array + overflow list) and aims to be mimalloc-like:
    // take the entire packet chain with one atomic_exchange (no per-packet CAS),
    // then spill leftovers to TLS.
#if HZ3_S93_OWNER_STASH_PACKET_V2
    Hz3S93StashPacket* pkts =
        (Hz3S93StashPacket*)atomic_exchange_explicit(headp, NULL, memory_order_acquire);

    uint8_t scnt = tc->spill_count[sc];
    void* ov_head = tc->spill_overflow[sc];  // prepend onto existing overflow (normally NULL here)
    tc->spill_overflow[sc] = NULL;
    int spilled = 0;

    while (pkts) {
        Hz3S93StashPacket* pkt = pkts;
        pkts = pkt->next;

#if HZ3_S93_FAILFAST
        if (pkt->sc != (uint8_t)sc || pkt->n > (uint16_t)HZ3_S93_PACKET_K) {
            fprintf(stderr,
                    "[HZ3_S93_FAILFAST] where=pop:bad_pkt owner=%u want_sc=%d pkt_sc=%u pkt_n=%u\n",
                    owner, sc, (unsigned)pkt->sc, (unsigned)pkt->n);
            abort();
        }
#endif
        S93_STAT_INC(g_s93_pop_packets_consumed);

        for (uint16_t i = 0; i < pkt->n; i++) {
            void* obj = pkt->ptrs[i];
            if (got < want) {
                out[got++] = obj;
                continue;
            }
            spilled = 1;
            if (scnt < (uint8_t)HZ3_S67_SPILL_CAP) {
                tc->spill_array[sc][scnt++] = obj;
                continue;
            }
            hz3_obj_set_next(obj, ov_head);
            ov_head = obj;
        }

        hz3_s93_pkt_free(owner, pkt);
    }

    tc->spill_count[sc] = scnt;
    if (ov_head) {
        tc->spill_overflow[sc] = ov_head;
    }
    if (spilled) {
        S93_STAT_INC(g_s93_pop_partial_packets);
    }
    STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_v2_final", owner, sc, out, got);
    return got;
#else
    // V1: pop packets from stash head (one-by-one) until want is satisfied.
    while (got < want) {
        void* head = atomic_load_explicit(headp, memory_order_acquire);
        if (!head) {
            break;
        }
        Hz3S93StashPacket* pkt = (Hz3S93StashPacket*)head;
        Hz3S93StashPacket* next = pkt->next;
        if (!atomic_compare_exchange_weak_explicit(
                headp, &head, next,
                memory_order_acquire, memory_order_relaxed)) {
            continue;
        }

#if HZ3_S93_FAILFAST
        if (pkt->sc != (uint8_t)sc) {
            fprintf(stderr, "[HZ3_S93_FAILFAST] where=pop:head_sc_mismatch owner=%u want_sc=%d pkt_sc=%u\n",
                    owner, sc, (unsigned)pkt->sc);
            abort();
        }
#endif
        S93_STAT_INC(g_s93_pop_packets_consumed);

        int need = want - got;
        int take = (pkt->n < (uint16_t)need) ? (int)pkt->n : need;
        memcpy(out + got, pkt->ptrs, (size_t)take * sizeof(void*));
        got += take;
        pkt->n = (uint16_t)(pkt->n - (uint16_t)take);
        if (pkt->n == 0) {
            hz3_s93_pkt_free(owner, pkt);
            continue;
        }

        memmove(pkt->ptrs, &pkt->ptrs[take], (size_t)pkt->n * sizeof(void*));
        pkt->next = (Hz3S93StashPacket*)tc->stash_spill[sc];
        tc->stash_spill[sc] = pkt;
        S93_STAT_INC(g_s93_pop_partial_packets);
        break;
    }

    STASH_BOUNDARY_CHECK_OUT("stash_pop:s93_final", owner, sc, out, got);
    return got;
#endif  // HZ3_S93_OWNER_STASH_PACKET_V2
#else  // !HZ3_S93_OWNER_STASH_PACKET && !HZ3_S93_OWNER_STASH_PACKET_V2

#if HZ3_S44_4_EARLY_PREFETCH && (defined(__GNUC__) || defined(__clang__))
    void* s44_4_head_hint = NULL;
#endif

#if HZ3_S44_4_EARLY_PREFETCH
#if defined(__GNUC__) || defined(__clang__)
    // S44-4 EPF: start prefetching stash head before spill checks to hide latency.
    // Safe: this is a speculative read of freed-object memory (intrusive next).
    {
        s44_4_head_hint = atomic_load_explicit(headp, memory_order_relaxed);
        if (s44_4_head_hint) {
            __builtin_prefetch(s44_4_head_hint, 0, 1);
#if HZ3_S44_4_EARLY_PREFETCH_DIST >= 2
            void* next_hint = hz3_obj_get_next(s44_4_head_hint);
            if (next_hint) __builtin_prefetch(next_hint, 0, 1);
#endif
        }
    }
#endif
#endif

#if HZ3_S94_SPILL_LITE
    // S94: SpillLite - array for low sc only (sc < SC_MAX)
    if (sc < HZ3_S94_SPILL_SC_MAX) {
        Hz3TCache* tc = &t_hz3_cache;
        uint8_t spill_n = tc->s94_spill_count[sc];

        // Step 0a: Take from spill array first (O(1) memcpy)
        if (spill_n > 0) {
            uint32_t take = (spill_n < want) ? spill_n : want;
            uint32_t start = spill_n - take;
            memcpy(out, &tc->s94_spill_array[sc][start], take * sizeof(void*));
            tc->s94_spill_count[sc] = (uint8_t)start;
            got = take;
            S94_STAT_INC(g_s94_spill_hit_calls);
            S94_STAT_ADD(g_s94_spill_take_total, take);
            if (got == want) {
                S44_4_STAT_INC(g_s44_4_spill_hit_count);
                STASH_BOUNDARY_CHECK_OUT("stash_pop:s94_spill_array", owner, sc, out, got);
                return got;
            }
        }

        // Step 0b: Take from overflow list if needed
        if (tc->s94_spill_overflow[sc]) {
            void* cur = tc->s94_spill_overflow[sc];
            uint32_t got_before = (uint32_t)got;
            while (cur && got < want) {
                out[got++] = cur;
                void* next_cur = hz3_obj_get_next(cur);
#if HZ3_S154_SPILL_PREFETCH && (defined(__GNUC__) || defined(__clang__))
                if (next_cur) {
                    __builtin_prefetch(next_cur, 0, 3);
                }
#endif
                cur = next_cur;
            }
            tc->s94_spill_overflow[sc] = cur;
            S94_STAT_ADD(g_s94_overflow_take_total, (uint32_t)(got - got_before));
            if (got == want) {
                S44_4_STAT_INC(g_s44_4_spill_hit_count);
                STASH_BOUNDARY_CHECK_OUT("stash_pop:s94_spill_overflow", owner, sc, out, got);
                return got;
            }
        }
        // Fall through to owner stash if still need more
    }
#endif  // HZ3_S94_SPILL_LITE

#if HZ3_S67_SPILL_ARRAY2
    // S67-2 Step 0a: Check TLS spill_array first (O(1) memcpy)
    Hz3TCache* tc = &t_hz3_cache;
    uint8_t cnt = tc->spill_count[sc];
    if (cnt > 0) {
        S67_STAT_INC(g_s67_spill_array_hit_calls);
        int take = (cnt < want) ? cnt : want;
        // Batch copy from end of array (LIFO order)
        memcpy(out, &tc->spill_array[sc][cnt - take], take * sizeof(void*));
        tc->spill_count[sc] = cnt - take;
        S67_STAT_ADD(g_s67_spill_array_take_total, (uint32_t)take);
        got = take;
        if (got == want) {
            S44_4_STAT_INC(g_s44_4_spill_hit_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:spill_array", owner, sc, out, got);
            return got;  // fully satisfied from spill array
        }
        // got < want: continue to overflow/owner stash
    }

    // S67-2 Step 0b: Check TLS spill_overflow (O(n) walk, but local/rare)
    void* overflow = tc->spill_overflow[sc];
    if (overflow) {
        S67_STAT_INC(g_s67_spill_overflow_hit_calls);
        tc->spill_overflow[sc] = NULL;  // take ownership
#if HZ3_S67_STATS
        int got_before = got;
#endif
        void* cur = overflow;
        while (cur && got < want) {
            out[got++] = cur;
            void* next_cur = hz3_obj_get_next(cur);
#if HZ3_S154_SPILL_PREFETCH && (defined(__GNUC__) || defined(__clang__))
            if (next_cur) {
                __builtin_prefetch(next_cur, 0, 3);
            }
#endif
            cur = next_cur;
        }
#if HZ3_S67_STATS
        S67_STAT_ADD(g_s67_spill_overflow_take_total, (uint32_t)(got - got_before));
#endif
        // Save remainder back (O(1), head only)
        tc->spill_overflow[sc] = cur;  // cur is NULL or remainder
        if (got == want) {
            S44_4_STAT_INC(g_s44_4_spill_hit_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:spill_overflow", owner, sc, out, got);
            return got;  // fully satisfied from overflow
        }
    }
    // Here: spill_overflow is NULL (fully drained) -> safe to set new overflow in Step 4

#elif HZ3_S67_SPILL_ARRAY
    // S67 Step 0: Check TLS spill array first (O(1) batch pop)
    Hz3TCache* tc = &t_hz3_cache;
    uint8_t cnt = tc->spill_count[sc];
    if (cnt > 0) {
        int take = (cnt < want) ? cnt : want;
        // Batch copy from end of array (LIFO order)
        memcpy(out, &tc->spill_array[sc][cnt - take], take * sizeof(void*));
        tc->spill_count[sc] = cnt - take;
        got = take;
        if (got == want) {
            S44_4_STAT_INC(g_s44_4_spill_hit_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:spill_array", owner, sc, out, got);
            return got;  // fully satisfied from spill array
        }
        // got < want: continue to owner stash
    }
#elif HZ3_S48_OWNER_STASH_SPILL
    // S48 Step 0: Check TLS spill first (linked-list, O(n) walk)
    // When S94 is enabled, S48 only handles sc >= SC_MAX (S94 handles low sc)
#if HZ3_S94_SPILL_LITE
    if (sc >= HZ3_S94_SPILL_SC_MAX) {
#endif
    Hz3TCache* tc = &t_hz3_cache;
    void* spill = tc->stash_spill[sc];
    if (spill) {
        tc->stash_spill[sc] = NULL;  // take ownership
        // Walk spill list, take up to want
        void* cur = spill;
        void* prev = NULL;
        while (cur && got < want) {
            out[got++] = cur;
            prev = cur;
            cur = hz3_obj_get_next(cur);
        }
        if (cur) {
            // Still have leftovers -> save back to spill
            if (prev) hz3_obj_set_next(prev, NULL);
            tc->stash_spill[sc] = cur;
        }
        if (got == want) {
            S44_4_STAT_INC(g_s44_4_spill_hit_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:spill", owner, sc, out, got);
            return got;  // fully satisfied from spill
        }
        // got < want: continue to stash
    }
#if HZ3_S94_SPILL_LITE
    }  // end if (sc >= HZ3_S94_SPILL_SC_MAX)
#endif
#endif

    // Step 1: Quick check (relaxed: false negative allowed)
    // S44-4 opt: can skip this check and let the atomic drain detect empty via NULL.
#if !HZ3_S44_4_SKIP_QUICK_EMPTY_CHECK
#if HZ3_S164_SKIP_QUICK_EMPTY_IF_GOT
    if (got == 0) {
#endif
#if HZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT && HZ3_S44_4_EARLY_PREFETCH && (defined(__GNUC__) || defined(__clang__))
    // If EPF hint says empty, confirm once before skipping drain.
    if (s44_4_head_hint == NULL && atomic_load_explicit(headp, memory_order_relaxed) == NULL) {
        STASH_BOUNDARY_CHECK_OUT("stash_pop:quick_empty", owner, sc, out, got);
        return got;  // 0 or partial from spill
    }
#else
    if (atomic_load_explicit(headp, memory_order_relaxed) == NULL) {
        STASH_BOUNDARY_CHECK_OUT("stash_pop:quick_empty", owner, sc, out, got);
        return got;  // 0 or partial from spill
    }
#endif
#if HZ3_S164_SKIP_QUICK_EMPTY_IF_GOT
    }
#endif
#endif

#if HZ3_S67_SPILL_ARRAY2 && HZ3_S112_FULL_DRAIN_EXCHANGE
    // S112: Full drain exchange - avoid bounded drain CAS retry + re-walk.
    {
        Hz3TCache* tc = &t_hz3_cache;  // S112: re-acquire tc (may be out of scope from S67-2 block)
        int need = want - got;
        if (need <= 0) {
            STASH_BOUNDARY_CHECK_OUT("stash_pop:s112_need0", owner, sc, out, got);
            return got;
        }

#if HZ3_S112_FAILFAST
        // Step 0b above guarantees spill_overflow is drained before we reach here.
        if (tc->spill_overflow[sc] != NULL) {
            fprintf(stderr, "[HZ3_S112_FAILFAST] where=pop:spill_overflow_nonnull owner=%u sc=%d head=%p\n",
                    owner, sc, tc->spill_overflow[sc]);
            abort();
        }
#endif

        S112_STAT_INC(g_s112_exchange_calls);
#if HZ3_S169_S112_BOUNDED_DRAIN
        S170_STAT_INC(g_s170_bounded_calls);
        void* list = NULL;
#if HZ3_S170_S112_BOUNDED_STATS
        int cas_retry = 0;
        int detach_n = 0;
#endif
        {
            int max_take = need + (int)HZ3_S67_SPILL_CAP;
            if (max_take < need) {
                max_take = need;
            }

            for (;;) {
                void* head = atomic_load_explicit(headp, memory_order_acquire);
                if (!head) {
#if HZ3_S170_S112_BOUNDED_STATS
                    S170_STAT_ADD(g_s170_bounded_cas_retry_total, (uint32_t)cas_retry);
#endif
                    S112_STAT_INC(g_s112_exchange_empty);
                    S44_4_STAT_INC(g_s44_4_fastpop_miss_count);
                    STASH_BOUNDARY_CHECK_OUT("stash_pop:s112_empty", owner, sc, out, got);
                    return got;
                }

                void* cur = head;
                void* prev = NULL;
                int n = 0;
                while (cur && n < max_take) {
                    void* next = hz3_obj_get_next(cur);
                    prev = cur;
                    cur = next;
                    n++;
                }

                void* new_head = cur;  // remainder stays in stash
                if (atomic_compare_exchange_weak_explicit(
                        headp, &head, new_head,
                        memory_order_acquire, memory_order_relaxed)) {
                    if (prev) {
                        hz3_obj_set_next(prev, NULL);
                    }
                    list = head;
#if HZ3_S170_S112_BOUNDED_STATS
                    detach_n = n;
#endif
                    break;
                }
#if HZ3_S170_S112_BOUNDED_STATS
                cas_retry++;
#endif
            }
        }
#if HZ3_S170_S112_BOUNDED_STATS
        S170_STAT_ADD(g_s170_bounded_cas_retry_total, (uint32_t)cas_retry);
        if (detach_n > 0) {
            S170_STAT_ADD(g_s170_bounded_detach_total, (uint32_t)detach_n);
            S170_STAT_MAX(g_s170_bounded_detach_max, (uint32_t)detach_n);
        }
#endif
#else
        void* list = atomic_exchange_explicit(headp, NULL, memory_order_acquire);
        if (!list) {
            S112_STAT_INC(g_s112_exchange_empty);
            S44_4_STAT_INC(g_s44_4_fastpop_miss_count);
            STASH_BOUNDARY_CHECK_OUT("stash_pop:s112_empty", owner, sc, out, got);
            return got;
        }
#endif  // HZ3_S169_S112_BOUNDED_DRAIN

        // Consume into out[] first, then fill spill_array. Remainder goes to spill_overflow (unbounded).
        void* cur = list;
        while (cur && got < want) {
            out[got++] = cur;
            cur = hz3_obj_get_next(cur);
        }

#if HZ3_S163_S112_SKIP_SPILL_ARRAY
        // S163: Skip spill_array fill to reduce per-pop instruction count.
#if HZ3_S165_S112_OVERFLOW_CAP
        // S165: Cap overflow length and push remainder back to stash.
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

            hz3_obj_set_next(overflow_tail, NULL);

            if (tc->spill_overflow[sc] != NULL) {
                S112_STAT_INC(g_s112_overflow_old_nonnull);
#if HZ3_S112_FAILFAST
                fprintf(stderr, "[HZ3_S112_FAILFAST] where=pop:overflow_old_nonnull owner=%u sc=%d old=%p cur=%p\n",
                        owner, sc, tc->spill_overflow[sc], overflow_head);
                abort();
#endif
            }
            tc->spill_overflow[sc] = overflow_head;
            S112_STAT_INC(g_s112_overflow_set_calls);

            if (cur) {
                void* tail;
                uint32_t n = 0;
#if HZ3_S170_S112_BOUNDED_STATS
                int tail_scanned = 0;
#endif
                hz3_list_find_tail(cur, &tail, &n);
#if HZ3_S170_S112_BOUNDED_STATS
                tail_scanned = 1;
#endif
#if HZ3_S170_S112_BOUNDED_STATS
                S170_STAT_INC(g_s170_remainder_calls);
                S170_STAT_ADD(g_s170_remainder_len_total, n);
                S170_STAT_MAX(g_s170_remainder_len_max, n);
                if (n == 1) {
                    S170_STAT_INC(g_s170_remainder_small1);
                } else if (n == 2) {
                    S170_STAT_INC(g_s170_remainder_small2);
                }
                if (tail_scanned) {
                    S170_STAT_INC(g_s170_remainder_tail_scan_calls);
                }
#endif
                void* old_head = atomic_load_explicit(headp, memory_order_relaxed);
                do {
                    hz3_obj_set_next(tail, old_head);
                } while (!atomic_compare_exchange_weak_explicit(
                    headp, &old_head, cur,
                    memory_order_release, memory_order_relaxed));
            }
        }
        STASH_BOUNDARY_CHECK_OUT("stash_pop:s112_nospill_cap", owner, sc, out, got);
        return got;
#else
        if (cur) {
            if (tc->spill_overflow[sc] != NULL) {
                S112_STAT_INC(g_s112_overflow_old_nonnull);
#if HZ3_S112_FAILFAST
                fprintf(stderr, "[HZ3_S112_FAILFAST] where=pop:overflow_old_nonnull owner=%u sc=%d old=%p cur=%p\n",
                        owner, sc, tc->spill_overflow[sc], cur);
                abort();
#endif
            }
            tc->spill_overflow[sc] = cur;
            S112_STAT_INC(g_s112_overflow_set_calls);
        }
        STASH_BOUNDARY_CHECK_OUT("stash_pop:s112_nospill", owner, sc, out, got);
        return got;
#endif  // HZ3_S165_S112_OVERFLOW_CAP
#else
        uint8_t cnt = tc->spill_count[sc];
        while (cur && cnt < HZ3_S67_SPILL_CAP) {
            tc->spill_array[sc][cnt++] = cur;
            cur = hz3_obj_get_next(cur);
        }
        tc->spill_count[sc] = cnt;

        if (cur) {
            if (tc->spill_overflow[sc] != NULL) {
                S112_STAT_INC(g_s112_overflow_old_nonnull);
#if HZ3_S112_FAILFAST
                fprintf(stderr, "[HZ3_S112_FAILFAST] where=pop:overflow_old_nonnull owner=%u sc=%d old=%p cur=%p\n",
                        owner, sc, tc->spill_overflow[sc], cur);
                abort();
#endif
            }
            tc->spill_overflow[sc] = cur;
            S112_STAT_INC(g_s112_overflow_set_calls);
        }

        STASH_BOUNDARY_CHECK_OUT("stash_pop:s112", owner, sc, out, got);
        return got;
#endif  // HZ3_S163_S112_SKIP_SPILL_ARRAY
    }
#endif  // HZ3_S67_SPILL_ARRAY2 && HZ3_S112_FULL_DRAIN_EXCHANGE

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
            void* next2 = hz3_obj_get_next(next);
            if (next2) __builtin_prefetch(next2, 0, 1);
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
        if (next) {
            // dist=2: also prefetch next->next if available
            void* next2 = hz3_obj_get_next(next);
            if (next2) __builtin_prefetch(next2, 0, 1);
        }
#endif
#else
        if (next) {
            __builtin_prefetch(next, 0, 1);  // L2 cache, read
#if HZ3_S44_PREFETCH_DIST >= 2
            // dist=2: also prefetch next->next if available
            void* next2 = hz3_obj_get_next(next);
            if (next2) __builtin_prefetch(next2, 0, 1);
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
#endif  // HZ3_S93_OWNER_STASH_PACKET
}
