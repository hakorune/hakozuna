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

#if HZ4_DECOMMIT_DELAY_QUEUE
#if HZ4_RSSRETURN && HZ4_RSSRETURN_QUARANTINEBOX && HZ4_PAGE_META_SEPARATE
// RSSReturnQuarantineBox v2:
// Hold a bounded number of empty pages out of the decommit queue until due.
// This prevents O(N) decommit-queue scans over not-ready quarantined nodes.
static inline void hz4_rss_quarantine_promote(hz4_tls_t* tls) {
    hz4_page_t* cur = tls->rss_quarantine_head;
    hz4_page_t* prev = NULL;
    while (cur) {
        hz4_page_meta_t* meta = hz4_page_meta(cur);
        hz4_page_t* next = meta->dqnext;

        // Drop if reused before maturity.
        if (meta->used_count != 0) {
            meta->rss_quarantined = 0;
            if (tls->rss_quarantine_inflight > 0) {
                tls->rss_quarantine_inflight--;
            }
            hz4_os_stats_rss_quarantine_drop_used();
            // unlink
            if (prev) {
                hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
                prev_meta->dqnext = next;
            } else {
                tls->rss_quarantine_head = next;
            }
            if (!next) {
                tls->rss_quarantine_tail = prev;
            }
            meta->dqnext = NULL;
            cur = next;
            continue;
        }

        if (meta->empty_deadline_tick > tls->collect_count) {
            prev = cur;
            cur = next;
            continue;
        }

        // Matured: move back into decommit queue with immediate due.
        meta->rss_quarantined = 2;
        if (tls->rss_quarantine_inflight > 0) {
            tls->rss_quarantine_inflight--;
        }
        hz4_os_stats_rss_quarantine_mature();

        // unlink from quarantine list first
        if (prev) {
            hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
            prev_meta->dqnext = next;
        } else {
            tls->rss_quarantine_head = next;
        }
        if (!next) {
            tls->rss_quarantine_tail = prev;
        }
        meta->dqnext = NULL;

        // Re-enqueue to decommit queue with due "now" so it can be processed immediately.
        meta->queued_decommit = 0;
        meta->empty_deadline_tick = 0;
        hz4_decommit_queue_enqueue_due(tls, cur, meta, (uint32_t)tls->collect_count);

        cur = next;
    }
}
#endif

#if HZ4_RSSRETURN && HZ4_RSSRETURN_PRESSUREGATEBOX
// PressureGateBox: seg_acq pressure-based quarantine control
// 圧力（seg_acq増加）が高いときだけ QuarantineBox を有効化
static inline bool hz4_rss_gate_pressure_check(hz4_tls_t* tls) {
    uint64_t seg_acq_now = hz4_os_segs_acquired_relaxed();
    uint32_t collect_now = tls->collect_count;

    // 初回は初期化のみ
    if (tls->collect_count_last == 0) {
        tls->seg_acq_epoch_last = (uint32_t)seg_acq_now;
        tls->collect_count_last = collect_now;
        tls->rss_gate_on = false;
        return false;
    }

    // 直近の増分を計算
    uint32_t delta = (uint32_t)(seg_acq_now - (uint64_t)tls->seg_acq_epoch_last);

    // 統計更新
    if (delta > tls->seg_acq_delta_max) {
        tls->seg_acq_delta_max = delta;
        hz4_os_stats_rss_gate_seg_acq_delta(delta);
    }

    // 前回から一定 tick 経過していれば判定
    uint32_t collect_delta = collect_now - tls->collect_count_last;
    if (collect_delta >= HZ4_RSSRETURN_GATE_TICK_WINDOW) {
        bool pressure_high = (delta >= HZ4_RSSRETURN_GATE_THRESH_SEG_ACQ);

        // 状態更新
        tls->seg_acq_epoch_last = (uint32_t)seg_acq_now;
        tls->collect_count_last = collect_now;

        if (pressure_high) {
            // 圧力高い: Gate ON、HOLD期間設定
            tls->rss_gate_on = true;
            tls->rss_gate_until = collect_now + HZ4_RSSRETURN_GATE_HOLD_TICKS;
            hz4_os_stats_rss_gate_fire();
            hz4_os_stats_rss_gate_on_window();
        } else if (collect_now >= tls->rss_gate_until) {
            // 圧力低い & HOLD期間終了: Gate OFF
            tls->rss_gate_on = false;
        }
        // HOLD期間中は状態維持
        return tls->rss_gate_on;
    }

    // 判定window中は「最後の状態」を維持
    return tls->rss_gate_on;
}
#endif

static inline void hz4_decommit_queue_process(hz4_tls_t* tls) {
    // Phase 2: Process decommit queue with RSSReturnBox control
    // inbox hitで早期returnする前に処理する必要がある
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_RSSRETURN && HZ4_RSSRETURN_COOLINGBOX
    // CoolingBox: promote cooled pages into decommit queue before budget calc
    hz4_cooling_promote(tls);
#endif
#if HZ4_RSSRETURN && HZ4_RSSRETURN_QUARANTINEBOX && HZ4_PAGE_META_SEPARATE
    hz4_rss_quarantine_promote(tls);
#endif
#if HZ4_RSSRETURN
    // RSSReturnBox: 2段階 budget 制御
    uint32_t budget;
  #if (HZ4_RSSRETURN_THRESH_PAGES == 0)
  #if HZ4_RSSRETURN_ADAPTIVE_BUDGET
    uint32_t dyn = tls->rssreturn_pending / HZ4_RSSRETURN_BATCH_DIV;
    if (dyn < HZ4_RSSRETURN_BATCH_PAGES) {
        dyn = HZ4_RSSRETURN_BATCH_PAGES;
    }
    if (dyn > HZ4_RSSRETURN_BATCH_MAX_PAGES) {
        dyn = HZ4_RSSRETURN_BATCH_MAX_PAGES;
    }
    budget = dyn;
  #else
    budget = HZ4_RSSRETURN_BATCH_PAGES;
  #endif
  #else
    if (tls->rssreturn_pending >= HZ4_RSSRETURN_THRESH_PAGES) {
        // [通常] threshold 到達: batch 処理
#if HZ4_RSSRETURN_ADAPTIVE_BUDGET
        uint32_t dyn = tls->rssreturn_pending / HZ4_RSSRETURN_BATCH_DIV;
        if (dyn < HZ4_RSSRETURN_BATCH_PAGES) {
            dyn = HZ4_RSSRETURN_BATCH_PAGES;
        }
        if (dyn > HZ4_RSSRETURN_BATCH_MAX_PAGES) {
            dyn = HZ4_RSSRETURN_BATCH_MAX_PAGES;
        }
        budget = dyn;
#else
        budget = HZ4_RSSRETURN_BATCH_PAGES;
#endif
    } else if (tls->rssreturn_pending > 0 &&
               (tls->collect_count % HZ4_RSSRETURN_SAFETY_PERIOD) == 0) {
        // [保険] threshold 未満でも PERIOD ごとに 1 ページ返す
        budget = 1;
    } else {
        // [hysteresis] それ以外は処理しない
        budget = 0;
    }
  #endif
#else
    uint32_t budget = HZ4_DECOMMIT_BUDGET_PER_COLLECT;
#endif

#if HZ4_RSSRETURN && HZ4_RSSRETURN_RELEASEBOX
    // ReleaseRateBox (periodic poll): token bucket.
    //
    // Goals:
    // - Keep malloc/free hot path clean (this runs in collect/decommit boundary only).
    // - Allow decommit to proceed even when rssreturn_pending < threshold (budget==0),
    //   but cap work per interval to avoid over-release.
    bool relbox_active = false;
    uint32_t relbox_budget_start = 0;
    uint32_t relbox_tokens_start = 0;

    // IMPORTANT:
    // - Never override a non-zero base budget.
    // - Avoid any work when budget==0 and dq is empty (no candidates).
    if (budget == 0 && tls->decommit_queue.head != NULL) {
        const uint64_t cc = tls->collect_count;
        if ((cc & (HZ4_RSSRETURN_RELEASE_PERIOD - 1)) == 0 && tls->rssreturn_release_last != cc) {
            tls->rssreturn_release_last = cc;
            if (tls->rssreturn_tokens < HZ4_RSSRETURN_RELEASE_BUDGET_MAX) {
                uint32_t add = HZ4_RSSRETURN_RELEASE_RATE;
                uint32_t room = (uint32_t)(HZ4_RSSRETURN_RELEASE_BUDGET_MAX - tls->rssreturn_tokens);
                if (add > room) add = room;
                tls->rssreturn_tokens += add;
            }
        }

        if (tls->rssreturn_tokens != 0) {
            hz4_os_stats_rss_release_fire();
            budget = tls->rssreturn_tokens;
            relbox_active = true;
            relbox_budget_start = budget;
            relbox_tokens_start = tls->rssreturn_tokens;
        }
    }
#endif

    if (budget == 0 && tls->decommit_queue.head != NULL) {
        hz4_os_stats_decommit_skip_budget0();
    }

    hz4_page_t* cur = tls->decommit_queue.head;
    hz4_page_t* prev = NULL;

    while (cur && budget > 0) {
        hz4_page_meta_t* meta = hz4_page_meta(cur);
        hz4_page_t* next = meta->dqnext;

        hz4_seg_t* seg_to_try_release = NULL;
        // Plan CPH push in this iteration, but perform the actual push only after
        // we dequeue from the decommit queue. Otherwise, another thread can pop
        // from CPH and overwrite meta->dqnext while we still rely on it.
        bool plan_cph_hot = false;
        bool plan_cph_cold = false;
        bool plan_cph_hot_fallback_decommit = false;
        bool still_local = false;
        bool cph_pushed = false;
        (void)plan_cph_hot_fallback_decommit;  // may be unused in some config combos
        (void)cph_pushed;  // may be unused in some config combos

        // Defensive: stale nodes (queued_decommit==0) must not remain linked in the dq list.
        // If they do, we can loop forever without consuming budget. Drop them eagerly.
        if (!meta->queued_decommit) {
            if (prev) {
                hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
                prev_meta->dqnext = next;
            } else {
                tls->decommit_queue.head = next;
            }
            if (!next) {
                tls->decommit_queue.tail = prev;
            }
            meta->dqnext = NULL;
            cur = next;
            continue;
        }

#if HZ4_CENTRAL_PAGEHEAP
        uint8_t cph_qstate = atomic_load_explicit(&meta->cph_queued, memory_order_acquire);
        if (cph_qstate != HZ4_CPH_NONE) {
            // CPH-managed pages are globally visible and can be popped/adopted by other threads.
            // Keep the per-thread decommit queue strictly disjoint from CPH to avoid dqnext corruption.
            hz4_os_stats_decommit_skip_cph();
            if (cph_qstate == HZ4_CPH_QUEUED) {
                hz4_os_stats_decommit_skip_cph_queued();
            }
            hz4_os_stats_dq_dequeue();

            // unlink and drop this node from dq (do not requeue)
            meta->queued_decommit = 0;
            meta->empty_deadline_tick = 0;
            meta->dqnext = NULL;
            if (prev) {
                hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
                prev_meta->dqnext = next;
            } else {
                tls->decommit_queue.head = next;
            }
            if (!next) {
                tls->decommit_queue.tail = prev;
            }
#if HZ4_RSSRETURN
            if (tls->rssreturn_pending > 0) {
                tls->rssreturn_pending--;
            }
#endif

            cur = next;
            continue;
        }
#endif

        // 二重enqueue防止・再利用済みチェック
        if (meta->queued_decommit && meta->empty_deadline_tick <= tls->collect_count) {
            hz4_os_stats_dq_due_ready();
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
            uint8_t cph_state = atomic_load_explicit(&meta->cph_state, memory_order_acquire);
            if (cph_state == HZ4_CPH_SEALING) {
                uint32_t delta = (uint32_t)(tls->collect_count - meta->seal_epoch);
                if (delta < HZ4_CPH_SEAL_GRACE) {
                    hz4_os_stats_cph_grace_block();
                    hz4_os_stats_decommit_skip_cph_grace();
                    hz4_os_stats_cand_requeue_grace();
                    prev = cur;
                    cur = next;
                    continue;
                }
            }
#endif
            // Deadline到来 - まだ空かチェック
            if (meta->used_count == 0) {
                // まだ空 - purgeしてdecommit
#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
                uint8_t sc = (uint8_t)meta->sc;
                hz4_tcache_purge_page_for_sc(tls, sc, cur);
                still_local = hz4_tcache_has_page_for_sc(tls, sc, cur);
#endif
                if (!still_local) {
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
                    if (atomic_load_explicit(&meta->cph_state, memory_order_acquire) == HZ4_CPH_SEALING) {
                        bool remote_empty = true;
                        for (uint32_t i = 0; i < HZ4_REMOTE_SHARDS; i++) {
                            if (atomic_load_explicit(&meta->remote_head[i], memory_order_relaxed)) {
                                remote_empty = false;
                                break;
                            }
                        }
                        bool prefer_cold = false;
#if HZ4_RSSRETURN && HZ4_CPH_PREFER_COLD_ON_RSSRETURN
#if (HZ4_RSSRETURN_THRESH_PAGES == 0)
                        prefer_cold = true;
#else
                        prefer_cold = (tls->rssreturn_pending >= HZ4_RSSRETURN_THRESH_PAGES);
#endif
#endif
                        if (remote_empty) {
#if HZ4_RSSRETURN && HZ4_RSSRETURN_QUARANTINEBOX && HZ4_PAGE_META_SEPARATE
                            // RSSReturnQuarantineBox:
                            // Keep a bounded number of empty pages out of CPH hot reuse for a short time.
                            // This is the minimum "candidate maturity" lever to make d_attempt>0 happen
                            // under normal workloads where empty pages get reused before due.
                            if (meta->rss_quarantined == 2) {
                                // A matured page was re-enqueued by hz4_rss_quarantine_promote().
                                // Force the cold path once, then clear.
                                meta->rss_quarantined = 0;
                                prefer_cold = true;
                            } else if (!prefer_cold) {
#if HZ4_RSSRETURN_PRESSUREGATEBOX
                                // PressureGateBox 制御下でのみ QuarantineBox を有効化
                                bool quarantine_enabled = hz4_rss_gate_pressure_check(tls);
                                uint32_t q_max_pages = quarantine_enabled ? HZ4_RSSRETURN_GATE_QUARANTINE_MAX_PAGES : 0;
                                uint32_t q_ticks = quarantine_enabled ? HZ4_RSSRETURN_GATE_QUARANTINE_TICKS : 0;
                                if (quarantine_enabled && tls->rss_quarantine_inflight < q_max_pages) {
#else
                                // 元々の処理（常時有効）
                                if (tls->rss_quarantine_inflight < HZ4_RSSRETURN_QUARANTINE_MAX_PAGES) {
#endif
                                    uint32_t due = (uint32_t)tls->collect_count
#if HZ4_RSSRETURN_PRESSUREGATEBOX
                                        + q_ticks
#else
                                        + HZ4_RSSRETURN_QUARANTINE_TICKS
#endif
                                    ;
                                    // Keep the existing grace alignment invariant (SSOT).
                                    uint32_t grace_due = meta->seal_epoch + HZ4_CPH_SEAL_GRACE;
                                    if ((int32_t)(grace_due - due) > 0) {
                                        due = grace_due;
                                    }

                                    // Unlink from decommit queue so it won't be scanned while waiting.
                                    meta->queued_decommit = 0;
                                    hz4_os_stats_dq_dequeue();
                                    if (prev) {
                                        hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
                                        prev_meta->dqnext = next;
                                    } else {
                                        tls->decommit_queue.head = next;
                                    }
                                    if (!next) {
                                        tls->decommit_queue.tail = prev;
                                    }
#if HZ4_RSSRETURN
                                    if (tls->rssreturn_pending > 0) {
                                        tls->rssreturn_pending--;
                                    }
#endif

                                    // Keep due in meta while quarantined.
                                    meta->rss_quarantined = 1;
                                    meta->empty_deadline_tick = due;
                                    meta->dqnext = NULL;

                                    // Push to quarantine list tail (bounded).
                                    if (!tls->rss_quarantine_tail) {
                                        tls->rss_quarantine_head = cur;
                                    } else {
                                        hz4_page_meta_t* qtail_meta = hz4_page_meta(tls->rss_quarantine_tail);
                                        qtail_meta->dqnext = cur;
                                    }
                                    tls->rss_quarantine_tail = cur;

                                    tls->rss_quarantine_inflight++;
                                    hz4_os_stats_rss_quarantine_set();

                                    // Continue without advancing prev (cur was removed from dq).
                                    cur = next;
                                    continue;
                                }
                            }
#endif
                            if (prefer_cold) {
                                if (!meta->decommitted) {
                                    hz4_page_try_decommit(cur, meta);
                                }
                                if (meta->decommitted) {
                                    plan_cph_cold = true;
                                }
                            }
                            if (!plan_cph_cold && !meta->decommitted) {
                                plan_cph_hot = true;
                                plan_cph_hot_fallback_decommit = true;
                            }
                        }
                    }
#endif
                    if (!plan_cph_hot && !plan_cph_cold) {
                        hz4_page_try_decommit(cur, meta);
                    }
                } else {
                    hz4_os_stats_decommit_skip_still_local();
                }
#if HZ4_DECOMMIT_REUSE_POOL
                // Phase 16: recycle decommitted page for future reuse
                bool reused = false;
                if (meta->decommitted) {
                    hz4_reuse_pool_push(tls, cur, meta);
                    reused = true;
                }
#endif
#if HZ4_CENTRAL_PAGEHEAP && !HZ4_CPH_2TIER
                // Phase 17: CPH push decommitted page to central heap
                // Box-theoretically clean: decommit済み & remote_empty が保証
                const bool reused_local =
#if HZ4_DECOMMIT_REUSE_POOL
                    reused;
#else
                    false;
#endif
                if ((meta->decommitted || HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT) && !reused_local) {
                    bool remote_empty = true;
                    for (uint32_t i = 0; i < HZ4_REMOTE_SHARDS; i++) {
                        if (atomic_load_explicit(&meta->remote_head[i], memory_order_relaxed)) {
                            remote_empty = false;
                            break;
                        }
                    }
                    if (remote_empty && meta->used_count == 0) {
                        hz4_cph_push_empty_meta(meta);
                    }
                }
#endif
            } else {
                hz4_os_stats_decommit_skip_used();
#if HZ4_RSSRETURN && HZ4_RSSRETURN_QUARANTINEBOX && HZ4_PAGE_META_SEPARATE
                // Defensive: if a matured-once page gets reused before we reach the cold attempt, clear the flag.
                if (meta->rss_quarantined) {
                    meta->rss_quarantined = 0;
                }
#endif
            }

            // キューから削除・フラグクリア
#if HZ4_RSSRETURN && HZ4_RSSRETURN_QUARANTINEBOX && HZ4_PAGE_META_SEPARATE
            // Defensive: should not be quarantined while in decommit queue; just clear.
            if (meta->rss_quarantined) meta->rss_quarantined = 0;
#endif
            meta->queued_decommit = 0;
            meta->empty_deadline_tick = 0;
            hz4_os_stats_dq_dequeue();
#if HZ4_RSSRETURN
            // O(1): dequeue 時に pending--（キュー全走査不要）
            if (tls->rssreturn_pending > 0) {
                tls->rssreturn_pending--;
            }
#endif
            if (prev) {
                hz4_page_meta_t* prev_meta = hz4_page_meta(prev);
                prev_meta->dqnext = next;
            } else {
                tls->decommit_queue.head = next;
            }
            if (!next) {
                tls->decommit_queue.tail = prev;
            }

#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
            // IMPORTANT: make CPH visible only after dequeuing from the decommit queue.
            // Otherwise, a concurrent CPH pop can overwrite meta->dqnext and corrupt the queue.
            if (plan_cph_cold) {
                if (meta->decommitted) {
                    cph_pushed = hz4_cph_cold_push_meta(meta);
                }
            } else if (plan_cph_hot) {
                if (!meta->decommitted) {
                    cph_pushed = hz4_cph_hot_push_meta(meta);
                }
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_RSSRETURN && HZ4_RSSRETURN_COOLINGBOX
                if (cph_pushed) {
                    hz4_cooling_enqueue(tls, cur, meta);
                }
#endif
                if (!cph_pushed && plan_cph_hot_fallback_decommit && !meta->decommitted) {
                    hz4_page_try_decommit(cur, meta);
                    if (meta->decommitted) {
                        cph_pushed = hz4_cph_cold_push_meta(meta);
                    }
                }
            }
#endif

            if (meta->decommitted && !cph_pushed) {
                seg_to_try_release = hz4_seg_from_page(cur);
            }
            budget--;

#if HZ4_SEG_RELEASE_EMPTY
            // Segment release must happen after unlinking the node to avoid UAF via dqnext.
            if (seg_to_try_release) {
                hz4_seg_try_release_empty(tls, seg_to_try_release);
            }
#else
            (void)seg_to_try_release;
#endif
        } else {
            if (meta->queued_decommit && meta->empty_deadline_tick > tls->collect_count) {
                hz4_os_stats_dq_due_notready();
                // Queue is deadline-ordered; if the head isn't ready, nothing behind it is ready either.
                break;
            }
            prev = cur;
        }
        cur = next;
    }

#if HZ4_RSSRETURN && HZ4_RSSRETURN_RELEASEBOX
    // Consume tokens by actual work done (pages dequeued) *only* when ReleaseRateBox fired.
    if (relbox_active) {
        uint32_t spent = (uint32_t)(relbox_budget_start - budget);
        if (spent > relbox_tokens_start) spent = relbox_tokens_start;
        tls->rssreturn_tokens = (uint32_t)(relbox_tokens_start - spent);
    }
#endif
}

// DecommitQueue DueGateBox boundary API (single call-site boundary).
// Callers should prefer this over calling hz4_decommit_queue_process() directly.
static inline void hz4_decommit_queue_maybe_process(hz4_tls_t* tls) {
#if HZ4_DECOMMIT_PROCESS_GUARD
    hz4_page_t* head = tls->decommit_queue.head;
    if (!head) {
        return;
    }
#else
    hz4_page_t* head = tls->decommit_queue.head;
#endif

#if HZ4_DECOMMIT_DUEGATEBOX
    // If the dq head isn't due, skip the whole processing to avoid churn.
    // Queue is expected to be deadline-ordered.
    hz4_page_meta_t* meta = hz4_page_meta(head);
    if (meta->queued_decommit && meta->empty_deadline_tick > tls->collect_count) {
        hz4_os_stats_dq_process_skip_duegate();
        return;
    }
#endif

    hz4_os_stats_dq_process_call();
    hz4_decommit_queue_process(tls);
}
#endif

#endif // HZ4_COLLECT_DECOMMIT_QUEUE_INC
