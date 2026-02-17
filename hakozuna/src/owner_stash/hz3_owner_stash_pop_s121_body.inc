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

