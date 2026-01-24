// hz4_collect.inc - CollectBox (Bounded Remote Object Collection)
// Box Theory: 境界 API (2箇所のうち1つ)
//
// 設計:
// - pending queue 優先 (scan は最後の手段)
// - obj_budget / seg_budget で上限
// - word atomic_exchange で bit-per-bit CAS を排除
// - n 本固定ループ (while(next) 禁止)

#ifndef HZ4_COLLECT_INC
#define HZ4_COLLECT_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_segq.inc"
#if HZ4_REMOTE_INBOX
#include "hz4_inbox.inc"
#endif

// ============================================================================
// Page Drain: remote_head[] から objects を回収 (owner のみ)
// ============================================================================
static inline uint32_t hz4_carry_consume(hz4_tls_t* tls, uint8_t sc,
                                         void** out, uint32_t budget) {
    // Fail-Fast: sc out of range
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("carry_consume: sc out of range");
    }
#endif
    if (sc >= HZ4_SC_MAX) {
        return 0;
    }

    hz4_carry_t* c = &tls->carry[sc];

    // Fail-Fast: n overflow
#if HZ4_FAILFAST
    if (c->n > HZ4_CARRY_SLOTS) {
        HZ4_FAIL("carry_consume: n overflow");
    }
#endif

    if (c->n == 0) {
        return 0;
    }

    uint32_t got = 0;

    // Consume from slots (LIFO: start from top slot)
    while (c->n > 0 && got < budget) {
        uint32_t idx = c->n - 1;
        hz4_carry_slot_t* slot = &c->slot[idx];

        if (!slot->head) {
            // Empty slot, pop it
            slot->page = NULL;
            c->n--;
            continue;
        }

        // Drain from this slot
        void* cur = slot->head;
        while (cur && got < budget) {
            void* next = hz4_obj_get_next(cur);
            out[got++] = cur;
            cur = next;
        }

        if (cur) {
            // Still has items, update head
            slot->head = cur;
        } else {
            // Slot exhausted, pop it
            slot->head = NULL;
            slot->page = NULL;
            c->n--;
        }
    }

    return got;
}

static inline uint32_t hz4_drain_page(hz4_tls_t* tls, uint8_t sc,
                                      hz4_page_t* page,
                                      void** out, uint32_t budget) {
    uint32_t got = 0;

    uint64_t mask = hz4_page_remote_mask(page);

    // Drain shards with pending hint only (when enabled)
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS && got < budget; s++) {
#if HZ4_REMOTE_MASK
        if ((mask & (1ULL << s)) == 0) {
            continue;
        }
#else
        (void)mask;
#endif
        void* list = hz4_page_drain_remote(page, s);

        // Walk list with bounded count (n 本固定)
        // Note: 本来は list の長さを知るべきだが、ここでは budget で制限
        uint32_t count = 0;
        while (list && got < budget && count < 1024) {  // safety bound
            void* next = hz4_obj_get_next(list);
            out[got++] = list;
            list = next;
            count++;
        }

        if (!list) {
#if HZ4_REMOTE_MASK
            hz4_page_clear_remote_hint(page, s);
            if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                hz4_page_mark_remote(page, s);
            }
#endif
            continue;
        }

        // If list has remainder, try to push to carry slots
        if (list) {
            hz4_carry_t* c = &tls->carry[sc];

            // Try to push to carry slot (if room)
            if (sc < HZ4_SC_MAX && c->n < HZ4_CARRY_SLOTS) {
#if HZ4_FAILFAST
                if (c->n >= HZ4_CARRY_SLOTS) {
                    HZ4_FAIL("carry push: slots full but called");
                }
#endif
                hz4_carry_slot_t* slot = &c->slot[c->n];
                slot->head = list;
                slot->page = page;
                c->n++;
                return got;
            }

            // Fallback: pushback only if carry slots are full
            void* tail = list;
            uint32_t rem = 1;
            void* cur = hz4_obj_get_next(list);
            while (cur && rem < 1024) {
                tail = cur;
                cur = hz4_obj_get_next(cur);
                rem++;
            }
            hz4_obj_set_next(tail, NULL);
            hz4_page_push_remote_list_tid(page, list, tail, rem, tls->tid);
            hz4_seg_t* seg = hz4_seg_from_page(page);
            hz4_pending_set(seg, hz4_page_idx(page));
        }
    }

    return got;
}

// ============================================================================
// Segment Drain: pending bitmap に基づいて pages を drain
// ============================================================================
static inline uint32_t hz4_drain_segment(hz4_tls_t* tls, uint8_t sc,
                                         hz4_seg_t* seg,
                                         void** out, uint32_t budget) {
    uint32_t got = 0;

#if HZ4_PAGEQ_ENABLE
    // Fail-Fast: sc out of range
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("drain_segment: sc out of range");
    }
#endif

    // Only drain bucket matching sc
    uint32_t bucket = sc >> HZ4_PAGEQ_BUCKET_SHIFT;

    // Fail-Fast: bucket out of range
#if HZ4_FAILFAST
    if (bucket >= HZ4_PAGEQ_BUCKETS) {
        HZ4_FAIL("drain_segment: bucket out of range");
    }
#endif

    // PageQ fast path (scan-less, bucket specific)
    hz4_page_t* pagelist = atomic_exchange_explicit(&seg->pageq_head[bucket], NULL,
                                                    memory_order_acq_rel);
    hz4_page_t* requeue_head = NULL;
    hz4_page_t* requeue_tail = NULL;

    while (pagelist && got < budget) {
        hz4_page_t* page = pagelist;
        pagelist = pagelist->qnext;
        page->qnext = NULL;

        if (hz4_page_valid(page) && page->sc == sc) {
            uint32_t drained = hz4_drain_page(tls, sc, page, out + got, budget - got);
            got += drained;
#if HZ4_STATS
            if (drained > 0) tls->pages_drained++;
#endif

            // Check if page still has remote objects
            bool still = false;
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                    still = true;
                    break;
                }
            }

            if (still) {
                atomic_store_explicit(&page->queued, 1, memory_order_release);
            } else {
                atomic_store_explicit(&page->queued, 0, memory_order_release);
                continue;
            }
        }

        // Requeue page if not fully drained or sc mismatch
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
    }

    // If budget ran out, requeue remaining pages
    while (pagelist) {
        hz4_page_t* page = pagelist;
        pagelist = pagelist->qnext;
        page->qnext = NULL;
        atomic_store_explicit(&page->queued, 1, memory_order_release);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
    }

    if (requeue_head) {
        hz4_page_t* old = atomic_load_explicit(&seg->pageq_head[bucket], memory_order_acquire);
        do {
            requeue_tail->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, requeue_head,
            memory_order_release, memory_order_acquire));
    }

    return got;
#else
    // Walk pending bitmap (word atomic_exchange)
    for (uint32_t w = 0; w < HZ4_PAGEWORDS && got < budget; w++) {
        uint64_t bits = hz4_pending_exchange_word(seg, w);

        // page0 は segment header と重なるため除外 (bit0 mask)
        if (w == 0) {
            bits &= ~1ULL;
        }

        while (bits && got < budget) {
            uint32_t bit = (uint32_t)__builtin_ctzll(bits);
            uint32_t page_idx = (w << 6) | bit;

            hz4_page_t* page = hz4_page_from_seg(seg, page_idx);

            // Validate page and size class
            if (hz4_page_valid(page) && page->sc == sc) {
                uint32_t drained = hz4_drain_page(tls, sc, page, out + got, budget - got);
                got += drained;
#if HZ4_STATS
                if (drained > 0) tls->pages_drained++;
#endif
            }

            bits &= bits - 1;  // clear lowest bit
        }

        // If we stopped early due to budget, restore remaining bits
        if (bits) {
            // Re-set the bits we didn't process
            atomic_fetch_or_explicit(&seg->pending_bits[w], bits, memory_order_release);
        }
    }

    return got;
#endif
}

// ============================================================================
// P4.1: List-mode collect (eliminate out[] intermediate array)
// ============================================================================
#if HZ4_COLLECT_LIST

// List splice helper: 2つの list を連結 (tail1->next = head2)
static inline void hz4_list_splice(void** head, void** tail, uint32_t* n,
                                   void* h2, void* t2, uint32_t n2) {
    if (!h2) return;
    if (!*head) {
        *head = h2;
        *tail = t2;
    } else {
        hz4_obj_set_next(*tail, h2);
        *tail = t2;
    }
    *n += n2;
}

// hz4_inbox_consume_list: inbox から list を直接取得
static inline uint32_t hz4_inbox_consume_list(hz4_tls_t* tls, uint8_t sc,
                                              void** head_out, void** tail_out,
                                              uint32_t budget) {
#if HZ4_REMOTE_INBOX
    void* list = tls->inbox_stash[sc];
    if (!list) {
        uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
        list = hz4_inbox_pop_all(owner, sc);  // P3.1 の空チェック済み
    }
    if (!list) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }

    void* head = list;
    void* cur = list;
    void* tail = NULL;
    uint32_t n = 0;
    while (cur && n < budget) {
        tail = cur;
        cur = hz4_obj_get_next(cur);
        n++;
    }

    tls->inbox_stash[sc] = cur;  // 余り
    *head_out = head;
    *tail_out = tail;
    return n;
#else
    (void)tls; (void)sc; (void)budget;
    *head_out = NULL;
    *tail_out = NULL;
    return 0;
#endif
}

// hz4_carry_consume_list: carry から list を直接取得
static inline uint32_t hz4_carry_consume_list(hz4_tls_t* tls, uint8_t sc,
                                              void** head_out, void** tail_out,
                                              uint32_t budget) {
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("carry_consume_list: sc out of range");
    }
#endif
    if (sc >= HZ4_SC_MAX) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }

    hz4_carry_t* c = &tls->carry[sc];
    if (c->n == 0) {
        *head_out = NULL;
        *tail_out = NULL;
        return 0;
    }

    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;

    while (c->n > 0 && got < budget) {
        uint32_t idx = c->n - 1;
        hz4_carry_slot_t* slot = &c->slot[idx];

        if (!slot->head) {
            slot->page = NULL;
            c->n--;
            continue;
        }

        void* cur = slot->head;
        while (cur && got < budget) {
            void* next = hz4_obj_get_next(cur);
            // Prepend to result list (順序逆転するが freelist なので OK)
            hz4_obj_set_next(cur, head);
            if (!tail) tail = cur;
            head = cur;
            cur = next;
            got++;
        }

        if (cur) {
            slot->head = cur;
        } else {
            slot->head = NULL;
            slot->page = NULL;
            c->n--;
        }
    }

    *head_out = head;
    *tail_out = tail;
    return got;
}

// hz4_drain_page_list: page の remote_head[] から list を直接取得
static inline uint32_t hz4_drain_page_list(hz4_tls_t* tls, uint8_t sc,
                                           hz4_page_t* page,
                                           void** head_out, void** tail_out,
                                           uint32_t budget) {
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;

    uint64_t mask = hz4_page_remote_mask(page);

    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS && got < budget; s++) {
#if HZ4_REMOTE_MASK
        if ((mask & (1ULL << s)) == 0) continue;
#else
        (void)mask;
#endif
        void* list = hz4_page_drain_remote(page, s);
        if (!list) {
#if HZ4_REMOTE_MASK
            hz4_page_clear_remote_hint(page, s);
            if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                hz4_page_mark_remote(page, s);
            }
#endif
            continue;
        }

        // Walk list to extract prefix (up to remaining budget)
        void* cur = list;
        void* prefix_tail = NULL;
        uint32_t count = 0;
        uint32_t remaining = budget - got;
        while (cur && count < remaining && count < 1024) {
            prefix_tail = cur;
            cur = hz4_obj_get_next(cur);
            count++;
        }

        // Splice prefix to result
        if (prefix_tail) {
            hz4_obj_set_next(prefix_tail, NULL);  // Cut prefix
            hz4_list_splice(&head, &tail, &got, list, prefix_tail, count);
        }

        // Handle remainder (cur points to remainder)
        if (cur) {
            hz4_carry_t* c = &tls->carry[sc];
            if (sc < HZ4_SC_MAX && c->n < HZ4_CARRY_SLOTS) {
                // Push to carry
                hz4_carry_slot_t* slot = &c->slot[c->n];
                slot->head = cur;
                slot->page = page;
                c->n++;
            } else {
                // Pushback to page
                void* rem_tail = cur;
                uint32_t rem = 1;
                void* next = hz4_obj_get_next(cur);
                while (next && rem < 1024) {
                    rem_tail = next;
                    next = hz4_obj_get_next(next);
                    rem++;
                }
                hz4_obj_set_next(rem_tail, NULL);
                hz4_page_push_remote_list_tid(page, cur, rem_tail, rem, tls->tid);
                hz4_seg_t* seg = hz4_seg_from_page(page);
                hz4_pending_set(seg, hz4_page_idx(page));
            }
        }
    }

    *head_out = head;
    *tail_out = tail;
    return got;
}

// hz4_drain_segment_list: PageQ から segment を drain して list を返す
#if HZ4_PAGEQ_ENABLE
static inline uint32_t hz4_drain_segment_list(hz4_tls_t* tls, uint8_t sc,
                                              hz4_seg_t* seg,
                                              void** head_out, void** tail_out,
                                              uint32_t budget) {
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;

#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("drain_segment_list: sc out of range");
    }
#endif

    uint32_t bucket = sc >> HZ4_PAGEQ_BUCKET_SHIFT;

#if HZ4_FAILFAST
    if (bucket >= HZ4_PAGEQ_BUCKETS) {
        HZ4_FAIL("drain_segment_list: bucket out of range");
    }
#endif

    hz4_page_t* pagelist = atomic_exchange_explicit(&seg->pageq_head[bucket], NULL,
                                                    memory_order_acq_rel);
    hz4_page_t* requeue_head = NULL;
    hz4_page_t* requeue_tail = NULL;

    while (pagelist && got < budget) {
        hz4_page_t* page = pagelist;
        pagelist = pagelist->qnext;
        page->qnext = NULL;

        if (hz4_page_valid(page) && page->sc == sc) {
            void* ph = NULL;
            void* pt = NULL;
            uint32_t drained = hz4_drain_page_list(tls, sc, page, &ph, &pt, budget - got);
            if (drained > 0) {
                hz4_list_splice(&head, &tail, &got, ph, pt, drained);
#if HZ4_STATS
                tls->pages_drained++;
#endif
            }

            // Check if page still has remote objects
            bool still = false;
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                    still = true;
                    break;
                }
            }

            if (still) {
                atomic_store_explicit(&page->queued, 1, memory_order_release);
            } else {
                atomic_store_explicit(&page->queued, 0, memory_order_release);
                continue;
            }
        }

        // Requeue
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
    }

    // Budget exhausted - requeue remaining
    while (pagelist) {
        hz4_page_t* page = pagelist;
        pagelist = pagelist->qnext;
        page->qnext = NULL;
        atomic_store_explicit(&page->queued, 1, memory_order_release);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
    }

    if (requeue_head) {
        hz4_page_t* old = atomic_load_explicit(&seg->pageq_head[bucket], memory_order_acquire);
        do {
            requeue_tail->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, requeue_head,
            memory_order_release, memory_order_acquire));
    }

    *head_out = head;
    *tail_out = tail;
    return got;
}
#endif // HZ4_PAGEQ_ENABLE

#endif // HZ4_COLLECT_LIST

// ============================================================================
// Collect (boundary API)
// ============================================================================
uint32_t hz4_collect(hz4_tls_t* tls,
                     uint8_t sc,
                     void** out,
                     uint32_t obj_budget,
                     uint32_t seg_budget) {
    tls->collect_count++;

    uint32_t got = 0;
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);

#if HZ4_REMOTE_INBOX
    // ---- Phase -1: Consume inbox (inbox mode only) ----
    got += hz4_inbox_consume(tls, sc, out + got, obj_budget - got);
    if (got) return got;  // inbox から取れたらすぐ return

#if HZ4_INBOX_ONLY
    // P3.2: 256回に1回だけ旧レーンを確認（残骸回収用）
    tls->inbox_probe_tick++;
    if ((tls->inbox_probe_tick & HZ4_INBOX_SEGQ_PROBE_MASK) != 0) {
        return 0;  // 旧レーンはスキップ
    }
#endif
#endif

    // ---- Phase 0: Consume carry (if any) ----
    if (got < obj_budget) {
        got += hz4_carry_consume(tls, sc, out + got, obj_budget - got);
        if (got >= obj_budget) {
            return got;
        }
        if (sc < HZ4_SC_MAX && tls->carry[sc].n > 0) {
#if HZ4_CARRY_SKIP_SEGQ
            return got;
#endif
        }
    }

    // ---- Phase 1: Drain pending queue (優先) ----
    hz4_seg_t* qlist_tail = NULL;
    hz4_seg_t* qlist = hz4_segq_pop_all(owner, &qlist_tail);

    while (qlist && got < obj_budget && seg_budget > 0) {
        hz4_seg_t* next = qlist->qnext;
        qlist->qnext = NULL;

        // Transition: QUEUED → PROC
        atomic_store_explicit(&qlist->qstate, HZ4_QSTATE_PROC, memory_order_release);

        // Drain segment
        uint32_t drained = hz4_drain_segment(tls, sc, qlist, out + got, obj_budget - got);
        got += drained;

#if HZ4_STATS
        tls->segs_popped++;
        tls->segs_drained++;
#endif

        // Finish: re-queue if still pending, else IDLE
        // (hz4_segq_finish will set segs_requeued if requeued)
        hz4_segq_finish(owner, qlist, tls);

        seg_budget--;
        qlist = next;
    }

    // Re-queue remaining segments if any (O(1) - tail は pop_all で取得済み)
    if (qlist) {
        hz4_segq_push_list(owner, qlist, qlist_tail);
    }

#if HZ4_STATS
    tls->objs_drained += got;
#endif

    // ---- Phase 2: Scan fallback (queue empty のときのみ) ----
    // Note: 本実装では scan fallback は省略
    //       hz4 の設計では pending queue が正しく動けば scan 不要
    if (got == 0 && seg_budget > 0) {
#if HZ4_STATS
        tls->scan_fallback++;
#endif
        // scan は最小限 or 省略 (将来実装)
    }

    return got;
}

// ============================================================================
// Collect Entry Point (simplified)
// ============================================================================
static inline uint32_t hz4_collect_default(hz4_tls_t* tls, uint8_t sc, void** out) {
    return hz4_collect(tls, sc, out,
                       HZ4_COLLECT_OBJ_BUDGET,
                       HZ4_COLLECT_SEG_BUDGET);
}

// ============================================================================
// P4.1: List-mode collect API (eliminate out[] in refill path)
// ============================================================================
#if HZ4_COLLECT_LIST

static inline uint32_t hz4_collect_list(hz4_tls_t* tls, uint8_t sc,
                                        void** head_out, void** tail_out) {
    tls->collect_count++;

    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;

    // Phase -1: Inbox (list mode)
    got = hz4_inbox_consume_list(tls, sc, &head, &tail, HZ4_COLLECT_OBJ_BUDGET);
    if (got) {
        // Safety: got > 0 なら tail != NULL を保証
        if (!tail) { *head_out = NULL; *tail_out = NULL; return 0; }
        *head_out = head;
        *tail_out = tail;
        return got;
    }

    // Phase 0: Carry (list mode)
    got = hz4_carry_consume_list(tls, sc, &head, &tail, HZ4_COLLECT_OBJ_BUDGET);
    if (got) {
        // Safety: got > 0 なら tail != NULL を保証
        if (!tail) { *head_out = NULL; *tail_out = NULL; return 0; }
        *head_out = head;
        *tail_out = tail;
        return got;
    }

#if HZ4_PAGEQ_ENABLE
    // Phase 1: Segq drain (list mode - P4.1b)
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
    hz4_seg_t* qlist_tail = NULL;
    hz4_seg_t* qlist = hz4_segq_pop_all(owner, &qlist_tail);
    uint32_t seg_budget = HZ4_COLLECT_SEG_BUDGET;

    while (qlist && got < HZ4_COLLECT_OBJ_BUDGET && seg_budget > 0) {
        hz4_seg_t* next = qlist->qnext;
        qlist->qnext = NULL;

        atomic_store_explicit(&qlist->qstate, HZ4_QSTATE_PROC, memory_order_release);

        void* sh = NULL;
        void* st = NULL;
        uint32_t drained = hz4_drain_segment_list(tls, sc, qlist, &sh, &st,
                                                  HZ4_COLLECT_OBJ_BUDGET - got);
        if (drained > 0) {
            hz4_list_splice(&head, &tail, &got, sh, st, drained);
        }

#if HZ4_STATS
        tls->segs_popped++;
        tls->segs_drained++;
#endif

        hz4_segq_finish(owner, qlist, tls);
        seg_budget--;
        qlist = next;
    }

    if (qlist) {
        hz4_segq_push_list(owner, qlist, qlist_tail);
    }

#if HZ4_STATS
    tls->objs_drained += got;
#endif

#else
    // Fallback: PageQ disabled - use old path with conversion
    void* out[HZ4_COLLECT_OBJ_BUDGET];
    got = hz4_collect_default(tls, sc, out);
    if (got > 0) {
        head = out[0];
        tail = out[got - 1];
        for (uint32_t i = 0; i < got - 1; i++) {
            hz4_obj_set_next(out[i], out[i + 1]);
        }
        hz4_obj_set_next(tail, NULL);
    }
#endif

    *head_out = head;
    *tail_out = tail;
    return got;
}

#endif // HZ4_COLLECT_LIST

#endif // HZ4_COLLECT_INC
