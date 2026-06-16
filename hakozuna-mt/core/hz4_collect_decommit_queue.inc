// hz4_collect_decommit_queue.inc - DecommitQueueBox
// Box Theory: Decommit queue management separated from CollectBox
//
// This file contains:
// - Decommit queue enqueue/dequeue operations
// - CoolingBox (RSSReturn cooling pressure management)
// - RSSReturnQuarantineBox
// - RSSReturn PressureGateBox
// - Main decommit queue processing loop

#ifndef HZ4_COLLECT_DECOMMIT_QUEUE_INC
#define HZ4_COLLECT_DECOMMIT_QUEUE_INC

#include <stdio.h>
#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_os.h"
#if HZ4_CENTRAL_PAGEHEAP
#include "hz4_central_pageheap.h"
#endif

// Forward declaration for tcache purge (defined in hz4_tcache.c)
#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
extern uint32_t hz4_tcache_purge_page_for_sc(struct hz4_tls* tls, uint8_t sc, struct hz4_page* page);
#endif

// Forward declarations from hz4_collect.inc
#if HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_page_try_decommit(hz4_page_t* page, hz4_page_meta_t* meta);
#endif
#if HZ4_DECOMMIT_REUSE_POOL && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_reuse_pool_push(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta);
#endif
#if HZ4_SEG_RELEASE_EMPTY && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline bool hz4_seg_can_release_empty(hz4_tls_t* tls, hz4_seg_t* seg);
static inline void hz4_seg_try_release_empty(hz4_tls_t* tls, hz4_seg_t* seg);
#endif

#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline void hz4_decommit_queue_enqueue_due(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta, uint32_t deadline) {
    if (meta->queued_decommit) {
        hz4_os_stats_dq_enqueue_already();
        return;
    }
#if HZ4_CENTRAL_PAGEHEAP
    // Decommit queue is per-thread and uses meta->dqnext (non-atomic link).
    // Never enqueue a page that is visible in the global CPH, otherwise a concurrent
    // CPH pop/adopt can clobber dqnext and corrupt the queue.
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != HZ4_CPH_NONE) {
#if HZ4_FAILFAST
        fprintf(stderr, "[HZ4_DQ_ENQ_INVALID] tls_tid=%u page=%p meta=%p sc=%u cph_queued=%u\n",
                (unsigned)tls->tid, (void*)page, (void*)meta, (unsigned)meta->sc,
                (unsigned)atomic_load_explicit(&meta->cph_queued, memory_order_relaxed));
        HZ4_FAIL("decommit_queue_enqueue_due: page is in CPH");
#else
        return;
#endif
    }
#endif
    hz4_os_stats_dq_enqueue();
    meta->empty_deadline_tick = deadline;
    meta->queued_decommit = 1;
#if HZ4_RSSRETURN
    // O(1): enqueue 時に pending++（キュー全走査不要）
    tls->rssreturn_pending++;
#endif
    // NOTE:
    // - This is owner-thread only (SPSC).
    // - Queue is expected to be deadline-ordered, so decommit processing can stop
    //   on the first not-ready node. However, some callers may "align" deadline
    //   (e.g., seal_epoch+grace), which can break simple tail-append monotonicity.
    // - If the deadline is already due, insert at head to avoid starving behind not-ready nodes.
    if (!tls->decommit_queue.head) {
        tls->decommit_queue.head = page;
        tls->decommit_queue.tail = page;
        meta->dqnext = NULL;
        return;
    }

    if (deadline <= (uint32_t)tls->collect_count) {
        meta->dqnext = tls->decommit_queue.head;
        tls->decommit_queue.head = page;
        return;
    }

    hz4_page_meta_t* tail_meta = hz4_page_meta(tls->decommit_queue.tail);
    if ((int32_t)(deadline - tail_meta->empty_deadline_tick) >= 0) {
        // Fast path: monotonic append (common case).
        tail_meta->dqnext = page;
        tls->decommit_queue.tail = page;
        meta->dqnext = NULL;
        return;
    }

    // Slow path: bounded insertion to keep the queue deadline-ordered.
    // This is SPSC and typically small; keep correctness over "assume monotonic".
    hz4_page_t* prev = NULL;
    hz4_page_t* cur = tls->decommit_queue.head;
#if HZ4_FAILFAST
    uint32_t steps = 0;
#endif
    while (cur) {
#if HZ4_FAILFAST
        if (++steps > 1000000u) {
            fprintf(stderr, "[HZ4_DQ_CYCLE] tls_tid=%u new_page=%p meta=%p head=%p tail=%p deadline=%u\n",
                    (unsigned)tls->tid, (void*)page, (void*)meta,
                    (void*)tls->decommit_queue.head, (void*)tls->decommit_queue.tail,
                    (unsigned)deadline);
            HZ4_FAIL("decommit_queue_enqueue_due: dqnext cycle suspected");
        }
#endif
        hz4_page_meta_t* cur_meta = hz4_page_meta(cur);
        if ((int32_t)(deadline - cur_meta->empty_deadline_tick) < 0) {
            break;  // insert before cur
        }
        prev = cur;
        cur = cur_meta->dqnext;
    }

    if (!prev) {
        meta->dqnext = tls->decommit_queue.head;
        tls->decommit_queue.head = page;
        return;
    }
    hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
    meta->dqnext = cur;
    prev_meta->dqnext = page;
    if (!cur) {
        tls->decommit_queue.tail = page;
    }
}

static inline void hz4_decommit_queue_enqueue(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta) {
    uint32_t deadline = tls->collect_count + HZ4_DECOMMIT_DELAY_TICKS;
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER && HZ4_CPH_DEADLINE_ALIGN_GRACE
    if (atomic_load_explicit(&meta->cph_state, memory_order_relaxed) == HZ4_CPH_SEALING) {
        uint32_t seal_deadline = meta->seal_epoch + HZ4_CPH_SEAL_GRACE;
        if ((int32_t)(seal_deadline - deadline) > 0) {
            deadline = seal_deadline;
        }
    }
#endif
    hz4_decommit_queue_enqueue_due(tls, page, meta, deadline);
}
#endif

#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_RSSRETURN && HZ4_RSSRETURN_COOLINGBOX
static inline bool hz4_cooling_pressure_on(hz4_tls_t* tls) {
#if HZ4_RSSRETURN_COOL_PRESSURE_SEG_ACQ
    return (tls->seg_acq_epoch >= HZ4_RSSRETURN_COOL_PRESSURE_BUDGET);
#else
    (void)tls;
    return true;
#endif
}

static inline void hz4_cooling_enqueue(hz4_tls_t* tls, hz4_page_t* page, hz4_page_meta_t* meta) {
    if (!hz4_cooling_pressure_on(tls)) {
        return;
    }
    if (meta->cooling) {
        hz4_os_stats_rss_cool_skip_already();
        return;
    }
    meta->cooling = 1;
    uint32_t deadline = tls->collect_count + HZ4_RSSRETURN_COOL_DELAY_TICKS;
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
    uint32_t seal_deadline = meta->seal_epoch + HZ4_CPH_SEAL_GRACE;
    if ((int32_t)(seal_deadline - deadline) > 0) {
        deadline = seal_deadline;
    }
#endif
    meta->cool_deadline = deadline;
    if (!tls->cool_tail) {
        tls->cool_head = page;
    } else {
        hz4_page_meta_t* tail_meta = hz4_page_meta(tls->cool_tail);
        tail_meta->dqnext = page;
    }
    tls->cool_tail = page;
    meta->dqnext = NULL;
    tls->cool_count++;
    hz4_os_stats_rss_cool_enq();
    hz4_os_stats_rss_cool_len(tls->cool_count);
}

static inline void hz4_cooling_promote(hz4_tls_t* tls) {
    if (!tls->cool_head) {
        return;
    }
    uint32_t budget = HZ4_RSSRETURN_COOL_SWEEP_BUDGET;
    hz4_page_t* cur = tls->cool_head;
    hz4_page_t* prev = NULL;
    while (cur && budget > 0) {
        hz4_page_meta_t* meta = hz4_page_meta(cur);
        hz4_page_t* next = meta->dqnext;
        if (meta->cool_deadline > tls->collect_count) {
            hz4_os_stats_rss_cool_block_deadline();
            prev = cur;
            cur = next;
            continue;
        }
        if (meta->used_count != 0) {
            hz4_os_stats_rss_cool_drop_used();
            meta->cooling = 0;
        } else {
            bool removed = true;
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
            if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != HZ4_CPH_NONE) {
                removed = hz4_cph_remove_meta_any(meta);
                if (removed) {
                    atomic_store_explicit(&meta->cph_queued, HZ4_CPH_NONE, memory_order_release);
                    atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_release);
                }
            }
#endif
            if (removed) {
                meta->cooling = 0;
                hz4_decommit_queue_enqueue(tls, cur, meta);
                hz4_os_stats_rss_cool_promote();
                budget--;
            } else {
                // CPH remove failed: keep in list for retry
                prev = cur;
                cur = next;
                continue;
            }
        }
        // remove cur from cooling list
        if (prev) {
            hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
            prev_meta->dqnext = next;
        } else {
            tls->cool_head = next;
        }
        if (!next) {
            tls->cool_tail = prev;
        }
        tls->cool_count--;
        cur = next;
    }
    if (cur && budget == 0) {
        hz4_os_stats_rss_cool_sweep_budget_hit();
    }
    hz4_os_stats_rss_cool_len(tls->cool_count);
}
#endif


#include "hz4_collect_decommit_queue_tail.inc"
#endif // HZ4_COLLECT_DECOMMIT_QUEUE_INC
