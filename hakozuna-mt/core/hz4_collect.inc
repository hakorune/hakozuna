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

#include <stdio.h>
#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_os.h"
#include "hz4_segq.inc"
#include "hz4_collect_carry.inc"
#if HZ4_CENTRAL_PAGEHEAP
#include "hz4_central_pageheap.h"
#endif
#if HZ4_REMOTE_INBOX
#include "hz4_inbox.inc"
#endif
#include "hz4_remote_page_rbuf.inc"

// ============================================================================
// Stage5-1b: Inbox-lite Box (skip inbox consume when inbox is empty)
// ============================================================================
#if HZ4_REMOTE_INBOX && HZ4_INBOX_LITE
static inline bool hz4_inbox_lite_should_skip(hz4_tls_t* tls, uint8_t sc) {
    // If TLS stash has remainder for this sc, we must not skip.
    if (tls->inbox_stash[sc] != NULL) {
        return false;
    }
#if HZ4_PRESSURE_GATE
    if (!hz4_pressure_gate_on(tls)) {
        return false;
    }
#endif

#if HZ4_TLS_MERGE
    uint8_t owner = tls->owner;
#else
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
#endif

    uint32_t epoch = atomic_load_explicit(&g_hz4_inbox_epoch[owner], memory_order_acquire);
    if (__builtin_expect(epoch == tls->inbox_lite_epoch, 1)) {
        if (tls->inbox_lite_nonempty == 0) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
            tls->stats_tls.inbox_lite_shortcut++;
#else
            hz4_os_stats_inbox_lite_shortcut();
#endif
#endif
            return true;
        }
        return false;
    }

#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.inbox_lite_scan_calls++;
#else
    hz4_os_stats_inbox_lite_scan_call();
#endif
#endif

    // Epoch changed => light scan all sc heads for this owner shard.
    bool any = false;
    for (uint32_t i = 0; i < HZ4_SC_MAX; i++) {
        void* h = atomic_load_explicit(&g_hz4_inbox[owner][i].head, memory_order_acquire);
        if (h != NULL) {
            any = true;
            break;
        }
    }

    tls->inbox_lite_epoch = epoch;
    tls->inbox_lite_nonempty = (uint8_t)(any ? 1 : 0);

    if (!any) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        tls->stats_tls.inbox_lite_shortcut++;
#else
        hz4_os_stats_inbox_lite_shortcut();
#endif
#endif
        return true;
    }
    return false;
}
#else
static inline bool hz4_inbox_lite_should_skip(hz4_tls_t* tls, uint8_t sc) {
    (void)tls; (void)sc;
    return false;
}
#endif

// Forward declaration for tcache purge (defined in hz4_tcache.c)
#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
extern uint32_t hz4_tcache_purge_page_for_sc(struct hz4_tls* tls, uint8_t sc, struct hz4_page* page);
#endif

// Forward declarations for SegmentDrainBox functions (defined in hz4_collect_seg_drain.inc)
#if HZ4_SEG_RELEASE_EMPTY && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline bool hz4_seg_can_release_empty(hz4_tls_t* tls, hz4_seg_t* seg);
static inline void hz4_seg_try_release_empty(hz4_tls_t* tls, hz4_seg_t* seg);
#endif
static inline uint32_t hz4_drain_segment(hz4_tls_t* tls, uint8_t sc, hz4_seg_t* seg, void** out, uint32_t budget);
#if HZ4_PAGEQ_ENABLE
static inline uint32_t hz4_drain_segment_list(hz4_tls_t* tls, uint8_t sc, hz4_seg_t* seg, void** head_out, void** tail_out, uint32_t budget);
#endif

// Forward declarations for PageDrainBox functions (defined in hz4_collect_page_drain.inc)
#if HZ4_PAGE_META_SEPARATE
static inline void hz4_inbox_account_array(hz4_tls_t* tls, void** out, uint32_t n);
static inline void hz4_inbox_account_list(hz4_tls_t* tls, void* head, uint32_t n);
#endif
static inline uint32_t hz4_drain_page(hz4_tls_t* tls, uint8_t sc, hz4_page_t* page, void** out, uint32_t budget);
#if HZ4_COLLECT_LIST
static inline uint32_t hz4_drain_page_list(hz4_tls_t* tls, uint8_t sc, hz4_page_t* page, void** head_out, void** tail_out, uint32_t budget);
#endif

#if HZ4_DECOMMIT_DELAY_QUEUE
static inline void hz4_decommit_queue_process(hz4_tls_t* tls);
static inline void hz4_decommit_queue_maybe_process(hz4_tls_t* tls);
#endif

#if HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
// Decommit empty page (owner only, called from collect boundary)
static inline void hz4_page_try_decommit(hz4_page_t* page, hz4_page_meta_t* meta) {
    if (!hz4_page_is_initialized(meta)) {
        hz4_os_stats_decommit_skip_uninit();
        return;
    }
    if (meta->decommitted) {
        hz4_os_stats_decommit_skip_already();
        return;
    }
  #if HZ4_DECOMMIT_OBSERVE
    fprintf(stderr, "[HZ4_DECOMMIT_CHECK] page=%p used_count=%u capacity=%u queued=%u\n",
            (void*)page, meta->used_count, meta->capacity, meta->queued);
  #endif
    if (meta->used_count != 0) {
        hz4_os_stats_decommit_skip_used();
  #if HZ4_DECOMMIT_OBSERVE
        fprintf(stderr, "[HZ4_DECOMMIT_SKIP] used_count=%u (non-zero)\n", meta->used_count);
  #endif
        return;
    }
#if HZ4_FAILFAST
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("page_try_decommit: invalid page");
    }
#endif
  #if HZ4_DECOMMIT_OBSERVE
    fprintf(stderr, "[HZ4_DECOMMIT_OK] page=%p used_count=0 -> decommit\n", (void*)page);
  #endif
    hz4_os_stats_decommit_attempt();
    hz4_os_page_decommit(page);
    meta->decommitted = 1;
    hz4_os_stats_decommit_success();
}
#endif

#if HZ4_DECOMMIT_REUSE_POOL && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_reuse_pool_push(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta);
#endif

// DecommitQueueBox functions are in hz4_collect_decommit_queue.inc

#if HZ4_DECOMMIT_REUSE_POOL && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_reuse_pool_push(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta) {
    uint8_t sc = (uint8_t)meta->sc;
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("reuse_pool_push: invalid sc");
    }
    if (!meta->decommitted) {
        HZ4_FAIL("reuse_pool_push: page not decommitted");
    }
#endif
    meta->reuse_next = tls->reuse_head[sc];
    tls->reuse_head[sc] = page;
}
#endif

// PageDrainBox functions are in hz4_collect_page_drain.inc (refactor-only extraction).

// ============================================================================
// Segment Drain: pending bitmap に基づいて pages を drain
// ============================================================================
// Note: hz4_drain_segment() and hz4_drain_segment_list() are in hz4_collect_seg_drain.inc

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
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.inbox_consume_calls++;
#else
    hz4_os_stats_inbox_consume_call();
#endif
#endif
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
        void* next = hz4_obj_get_next(cur);
#if HZ4_COLLECT_PREFETCH
        if (next) {
            __builtin_prefetch(next, 0, 3);
        }
#endif
        cur = next;
        n++;
    }

    tls->inbox_stash[sc] = cur;  // 余り
#if HZ4_OS_STATS
    if (cur) {
        uint32_t stash_len = 0;
        void* tmp = cur;
        while (tmp) {
            stash_len++;
            tmp = hz4_obj_get_next(tmp);
        }
        hz4_os_stats_inbox_stash_len(stash_len);
    }
#endif
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

// Note: hz4_drain_page_list() is in hz4_collect_page_drain.inc
// Note: hz4_drain_segment_list() is in hz4_collect_seg_drain.inc

#endif // HZ4_COLLECT_LIST

// ============================================================================
// Collect (boundary API)
// ============================================================================
uint32_t hz4_collect(hz4_tls_t* tls,
                     uint8_t sc,
                     void** out,
                     uint32_t obj_budget,
                     uint32_t seg_budget) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.collect_calls++;
#else
    hz4_os_stats_collect_call();
#endif
#endif
    tls->collect_count++;

#if HZ4_DECOMMIT_DELAY_QUEUE
#if HZ4_DECOMMIT_PROCESS_GUARD
    if (tls->decommit_queue.head != NULL) {
        hz4_decommit_queue_maybe_process(tls);
    }
#else
    hz4_decommit_queue_maybe_process(tls);
#endif
#endif

    uint32_t got = 0;
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);

#if HZ4_REMOTE_PAGE_RBUF
    // ---- Phase -2: RemotePageRbufBox (bypass inbox/segq) ----
    if (hz4_remote_drain_demand_try_consume(tls, sc)) {
        uint32_t rbuf_got = hz4_remote_page_rbuf_consume(tls, owner, sc, out + got, obj_budget - got);
        got += rbuf_got;
        if (got) goto out;
    }
#endif

#if HZ4_REMOTE_INBOX
    // ---- Phase -1: Consume inbox (inbox mode only) ----
    uint32_t inbox_got = 0;
#if HZ4_INBOX_LITE
    if (!hz4_inbox_lite_should_skip(tls, sc))
#endif
    {
        inbox_got = hz4_inbox_consume(tls, sc, out + got, obj_budget - got);
    }
#if HZ4_PAGE_META_SEPARATE
    if (inbox_got) {
        hz4_inbox_account_array(tls, out + got, inbox_got);
    }
#endif
    got += inbox_got;
    if (got) goto out;  // inbox から取れたらすぐ return

#if HZ4_INBOX_ONLY
    // P3.2: 256回に1回だけ旧レーンを確認（残骸回収用）
    tls->inbox_probe_tick++;
    if ((tls->inbox_probe_tick & HZ4_INBOX_SEGQ_PROBE_MASK) != 0) {
        got = 0;
        goto out;  // 旧レーンはスキップ
    }
#endif
#endif

    // ---- Phase 0: Consume carry (if any) ----
    if (got < obj_budget) {
        uint32_t carry_got = hz4_carry_consume(tls, sc, out + got, obj_budget - got);
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        if (carry_got) {
            tls->stats_tls.carry_hit++;
        } else {
            tls->stats_tls.carry_miss++;
        }
#else
        if (carry_got) {
            hz4_os_stats_carry_hit();
        } else {
            hz4_os_stats_carry_miss();
        }
#endif
#endif
        got += carry_got;
        if (got >= obj_budget) {
            goto out;
        }
        if (sc < HZ4_SC_MAX && tls->carry[sc].n > 0) {
#if HZ4_CARRY_SKIP_SEGQ
            goto out;
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

out:
#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
    hz4_os_stats_tls_flush(tls);
#endif
    return got;
}

// Include DecommitQueueBox (RSSReturn, CoolingBox, QuarantineBox, PressureGateBox, ReleaseBox)
#include "hz4_collect_decommit_queue.inc"

// Include PageDrainBox (provides hz4_drain_page_* for SegmentDrainBox)
#include "hz4_collect_page_drain.inc"

// Include SegmentDrainBox (provides hz4_drain_segment_* for EntryBox)
#include "hz4_collect_seg_drain.inc"

// Include CollectOrchestratorBox (entry points: collect_default, collect_list, seg_acq_guard)
#include "hz4_collect_entry.inc"

#endif // HZ4_COLLECT_INC
