// hz4_collect_entry.inc - CollectOrchestratorBox (Collect Entry Points)
// Box Theory: collect操作のエントリポイントとオーケストレーション
//
// Functions:
// - hz4_collect_default(): 標準的なcollect（配列モード）
// - hz4_collect_list(): listモードcollect（P4.1）
// - hz4_seg_acq_guard*(): seg_acq爆発防止ガード

#ifndef HZ4_COLLECT_ENTRY_INC
#define HZ4_COLLECT_ENTRY_INC

#include "hz4_tls.h"
#include "hz4_page.h"

// Forward declarations from other boxes
static inline uint32_t hz4_inbox_consume_list(hz4_tls_t* tls, uint8_t sc,
                                              void** head_out, void** tail_out,
                                              uint32_t budget);
#if HZ4_PAGE_META_SEPARATE
static inline void hz4_inbox_account_list(hz4_tls_t* tls, void* head, uint32_t n);
#endif
static inline uint32_t hz4_carry_consume_list(hz4_tls_t* tls, uint8_t sc,
                                              void** head_out, void** tail_out,
                                              uint32_t budget);
static inline uint32_t hz4_drain_segment_list(hz4_tls_t* tls, uint8_t sc,
                                              hz4_seg_t* seg,
                                              void** head_out, void** tail_out,
                                              uint32_t budget);
#if HZ4_DECOMMIT_DELAY_QUEUE
static inline void hz4_decommit_queue_process(hz4_tls_t* tls);
static inline void hz4_decommit_queue_maybe_process(hz4_tls_t* tls);
#endif

static inline uint32_t hz4_collect_obj_budget_for_sc(uint8_t sc) {
#if HZ4_COLLECT_OBJ_BUDGET_HI_SC_ENABLE
    if (sc >= HZ4_COLLECT_OBJ_BUDGET_HI_SC_MIN) {
        return HZ4_COLLECT_OBJ_BUDGET_HI_SC;
    }
#else
    (void)sc;
#endif
    return HZ4_COLLECT_OBJ_BUDGET;
}

// ============================================================================
// Collect Entry Point (simplified)
// ============================================================================
static inline uint32_t hz4_collect_default(hz4_tls_t* tls, uint8_t sc, void** out) {
    const uint32_t obj_budget = hz4_collect_obj_budget_for_sc(sc);
    return hz4_collect(tls, sc, out,
                       obj_budget,
                       HZ4_COLLECT_SEG_BUDGET);
}

// ============================================================================
// P4.1: List-mode collect API (eliminate out[] in refill path)
// ============================================================================
#if HZ4_COLLECT_LIST

static inline uint32_t hz4_collect_list(hz4_tls_t* tls, uint8_t sc,
                                        void** head_out, void** tail_out) {
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

    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
    const uint32_t obj_budget = hz4_collect_obj_budget_for_sc(sc);

#if HZ4_REMOTE_PAGE_RBUF
    // Phase -2: RemotePageRbufBox (list mode)
    if (hz4_remote_drain_demand_try_consume(tls, sc)) {
        got = hz4_remote_page_rbuf_consume_list(tls, owner, sc, &head, &tail, obj_budget);
        if (got) {
            if (!tail) { head = NULL; tail = NULL; got = 0; goto out_list; }
            goto out_list;
        }
    }
#endif

    // Phase -1: Inbox (list mode)
    got = 0;
#if HZ4_REMOTE_INBOX
#if HZ4_INBOX_LITE
    if (!hz4_inbox_lite_should_skip(tls, sc))
#endif
    {
        got = hz4_inbox_consume_list(tls, sc, &head, &tail, obj_budget);
    }
#endif
    if (got) {
#if HZ4_PAGE_META_SEPARATE
        hz4_inbox_account_list(tls, head, got);
#endif
        // Safety: got > 0 なら tail != NULL を保証
        if (!tail) { head = NULL; tail = NULL; got = 0; goto out_list; }
        goto out_list;
    }

    // Phase 0: Carry (list mode)
    got = hz4_carry_consume_list(tls, sc, &head, &tail, obj_budget);
    if (got) {
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
        tls->stats_tls.carry_hit++;
#else
        hz4_os_stats_carry_hit();
#endif
#endif
        // Safety: got > 0 なら tail != NULL を保証
        if (!tail) { head = NULL; tail = NULL; got = 0; goto out_list; }
        goto out_list;
    }
#if HZ4_OS_STATS
#if HZ4_OS_STATS_FAST
    tls->stats_tls.carry_miss++;
#else
    hz4_os_stats_carry_miss();
#endif
#endif

#if HZ4_PAGEQ_ENABLE
    // Phase 1: Segq drain (list mode - P4.1b)
    hz4_seg_t* qlist_tail = NULL;
    hz4_seg_t* qlist = hz4_segq_pop_all(owner, &qlist_tail);
    uint32_t seg_budget = HZ4_COLLECT_SEG_BUDGET;

    while (qlist && got < obj_budget && seg_budget > 0) {
        hz4_seg_t* next = qlist->qnext;
        qlist->qnext = NULL;

        atomic_store_explicit(&qlist->qstate, HZ4_QSTATE_PROC, memory_order_release);

        void* sh = NULL;
        void* st = NULL;
        uint32_t drained = hz4_drain_segment_list(tls, sc, qlist, &sh, &st,
                                                  obj_budget - got);
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
    void* out[HZ4_COLLECT_OBJ_BUDGET_MAX];
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

out_list:
    *head_out = head;
    *tail_out = tail;
#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
    hz4_os_stats_tls_flush(tls);
#endif
    return got;
}

#endif // HZ4_COLLECT_LIST

// ============================================================================
// Phase 18: SegAcquireGuardBox - limit seg_acq explosion
// ============================================================================
#if HZ4_SEG_ACQ_GUARDBOX

static inline bool hz4_seg_acq_guard_should_fire(hz4_tls_t* tls) {
    uint32_t now = tls->collect_count;
    if (tls->seg_acq_epoch < HZ4_SEG_ACQ_BUDGET) return false;
    if (now - tls->seg_acq_guard_last < HZ4_SEG_ACQ_GUARD_COOLDOWN) return false;
    tls->seg_acq_guard_last = now;
    return true;
}

static inline void hz4_seg_acq_guard(hz4_tls_t* tls, uint8_t sc) {
    if (!hz4_seg_acq_guard_should_fire(tls)) return;

    // 1) decommit queue を一度強めに
#if HZ4_DECOMMIT_DELAY_QUEUE
#if HZ4_DECOMMIT_PROCESS_GUARD
    if (tls->decommit_queue.head != NULL) {
        hz4_decommit_queue_maybe_process(tls);
    }
#else
    hz4_decommit_queue_maybe_process(tls);
#endif
#endif

    // 2) collect を1回（sc優先）
#if HZ4_COLLECT_LIST
    void* head = NULL;
    void* tail = NULL;
    (void)hz4_collect_list(tls, sc, &head, &tail);
#else
    void* out[HZ4_COLLECT_OBJ_BUDGET_MAX];
    (void)hz4_collect_default(tls, sc, out);
#endif

    // 3) さらに他のscも少し（簡易版: ラウンドロビン）
    for (uint8_t other_sc = 0; other_sc < HZ4_SC_MAX; other_sc++) {
        if (other_sc == sc) continue;
#if HZ4_COLLECT_LIST
        void* h = NULL;
        void* t = NULL;
        (void)hz4_collect_list(tls, other_sc, &h, &t);
#else
        void* o[HZ4_COLLECT_OBJ_BUDGET_MAX];
        (void)hz4_collect_default(tls, other_sc, o);
#endif
        if (tls->seg_acq_epoch < HZ4_SEG_ACQ_BUDGET / 2) break;
    }
}

#endif // HZ4_SEG_ACQ_GUARDBOX

#endif // HZ4_COLLECT_ENTRY_INC
