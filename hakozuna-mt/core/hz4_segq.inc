// hz4_segq.inc - PendingSegQueueBox (MPSC Segment Queue)
// Box Theory: scan を排除して「pending のある seg だけ」collect する
//
// 状態遷移:
//   IDLE ──(pending set)──► QUEUED ──(pop_all)──► PROC
//     ▲                                             │
//     └──────────(pending==0)───────────────────────┘
//     └──────────(pending!=0)──► QUEUED (re-queue)──┘
//
// 重要:
// - qnext は非atomic (producer が自分の node.next を書くだけ)
// - head + tail で O(1) push_list を実現
// - finish で pending 残りがあれば必ず再 queue (取りこぼし防止)

#ifndef HZ4_SEGQ_INC
#define HZ4_SEGQ_INC

#include "hz4_seg.h"
#include "hz4_page.h"

// ============================================================================
// Global MPSC Queue (per owner shard)
// ============================================================================
// O(1) push_list: head + tail で管理
// tail は補助用（requeue 時の scan 省略）
typedef struct {
    _Atomic(hz4_seg_t*) head;
    _Atomic(hz4_seg_t*) tail;
} hz4_segq_t;

static hz4_segq_t g_hz4_segq[HZ4_NUM_SHARDS];

// ============================================================================
// Push List: 残りを一括で再 enqueue (owner のみ) - O(1) 版
// ============================================================================
// Used when collect budget exceeded
// tail: リストの末尾を呼び出し側が渡す（scan 不要）
static inline void hz4_segq_push_list(uint8_t owner, hz4_seg_t* head, hz4_seg_t* tail) {
    if (!head) return;
    hz4_segq_t* q = &g_hz4_segq[owner];

    // Prepend entire list to queue (LIFO: tail.next = old_head)
    hz4_seg_t* old_head = atomic_load_explicit(&q->head, memory_order_acquire);
    do {
        tail->qnext = old_head;
    } while (!atomic_compare_exchange_weak_explicit(
            &q->head, &old_head, head,
            memory_order_release, memory_order_acquire));

    // 元々 empty だった場合は tail を設定
    if (!old_head) {
        atomic_store_explicit(&q->tail, tail, memory_order_release);
    }
}

// ============================================================================
// Push: idle→queued 時のみ enqueue
// ============================================================================
// Called from remote flush path after setting pending bit
static inline void hz4_segq_push(uint8_t owner, hz4_seg_t* seg) {
    // CAS: IDLE → QUEUED (only one thread wins)
    uint8_t expect = HZ4_QSTATE_IDLE;
    if (!atomic_compare_exchange_strong_explicit(
            &seg->qstate, &expect, HZ4_QSTATE_QUEUED,
            memory_order_acq_rel, memory_order_acquire)) {
        // Already QUEUED or PROC: do nothing
        return;
    }

    // Prepend to MPSC queue (single segment: head==tail)
    seg->qnext = NULL;
    hz4_segq_push_list(owner, seg, seg);
}

// ============================================================================
// Pop All: atomic_exchange で全取得 (owner のみ)
// ============================================================================
// tail_out: オプション。元のリストの tail を返す（requeue 時に使用）
static inline hz4_seg_t* hz4_segq_pop_all(uint8_t owner, hz4_seg_t** tail_out) {
    hz4_segq_t* q = &g_hz4_segq[owner];
    hz4_seg_t* old_tail = atomic_load_explicit(&q->tail, memory_order_acquire);
    hz4_seg_t* head = atomic_exchange_explicit(&q->head, NULL, memory_order_acq_rel);
    if (head) {
        // tail が old_tail のままなら NULL に（race で変わってたら触らない）
        (void)atomic_compare_exchange_strong_explicit(
            &q->tail, &old_tail, NULL, memory_order_release, memory_order_relaxed);
    }
    if (tail_out) *tail_out = old_tail;
    return head;
}

// ============================================================================
// Finish: drain 後の状態遷移 (owner のみ)
// ============================================================================
// 重要: pending_bits が非ゼロなら QUEUED に戻して再 enqueue
//       ゼロなら IDLE に戻す（取りこぼし防止）
// tls: 統計更新用 (HZ4_STATS 有効時のみ使用)
static inline void hz4_segq_finish(uint8_t owner, hz4_seg_t* seg, hz4_tls_t* tls) {
    (void)tls;  // HZ4_STATS 無効時の警告抑制

    // Check if any pending bits remain (or pageq has work)
#if HZ4_PAGEQ_ENABLE
    bool pageq_has_work = false;
    for (uint32_t b = 0; b < HZ4_PAGEQ_BUCKETS && !pageq_has_work; b++) {
        if (atomic_load_explicit(&seg->pageq_head[b], memory_order_acquire) != NULL) {
            pageq_has_work = true;
        }
    }
    bool has_pending = pageq_has_work || hz4_pending_any(seg);
#else
    bool has_pending = hz4_pending_any(seg);
#endif

    if (has_pending) {
        // Still has pending: re-queue
        // Note: qstate is already PROC, need to set to QUEUED before push
        atomic_store_explicit(&seg->qstate, HZ4_QSTATE_QUEUED, memory_order_release);

#if HZ4_STATS
        if (tls) tls->segs_requeued++;
#endif

        // Prepend to queue (single segment)
        seg->qnext = NULL;
        hz4_segq_push_list(owner, seg, seg);
    } else {
        // No pending: back to IDLE
        atomic_store_explicit(&seg->qstate, HZ4_QSTATE_IDLE, memory_order_release);
    }
}

// ============================================================================
// Notify: pending set 後に queue への追加を試みる
// ============================================================================
// Called from remote flush after pushing to page.remote_head
static inline void hz4_segq_notify(hz4_seg_t* seg, uint32_t page_idx) {
    // Set pending bit first
    hz4_pending_set(seg, page_idx);

    // Try to enqueue (will be no-op if already QUEUED or PROC)
    hz4_segq_push(seg->owner, seg);
}

// Notify without pending bits (pageq mode)
static inline void hz4_segq_notify_seg(hz4_seg_t* seg) {
    hz4_segq_push(seg->owner, seg);
}

#if HZ4_PAGEQ_ENABLE
static inline void hz4_pageq_notify(hz4_page_t* page) {
#if HZ4_PAGE_META_SEPARATE
    hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP
    // PageQ is a segment-local remote-notify mechanism; it must not track pages managed by CPH.
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
#if HZ4_FAILFAST
        HZ4_FAIL("pageq_notify: page is in central pageheap");
#endif
        return;
    }
#endif
    if (atomic_exchange_explicit(&meta->queued, 1, memory_order_acq_rel) == 0) {
        hz4_seg_t* seg = hz4_seg_from_page(page);

        // Fail-Fast: sc out of range
#if HZ4_FAILFAST
        if (meta->sc >= HZ4_SC_MAX) {
            HZ4_FAIL("pageq_notify: sc out of range");
        }
#endif

        // Select bucket based on size class
        uint32_t bucket = meta->sc >> HZ4_PAGEQ_BUCKET_SHIFT;

        // Fail-Fast: bucket out of range
#if HZ4_FAILFAST
        if (bucket >= HZ4_PAGEQ_BUCKETS) {
            HZ4_FAIL("pageq_notify: bucket out of range");
        }
#endif

        hz4_page_t* old = atomic_load_explicit(&seg->pageq_head[bucket], memory_order_acquire);
        do {
            meta->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, page,
            memory_order_release, memory_order_acquire));
        hz4_segq_notify_seg(seg);
    }
#else
    if (atomic_exchange_explicit(&page->queued, 1, memory_order_acq_rel) == 0) {
        hz4_seg_t* seg = hz4_seg_from_page(page);

        // Fail-Fast: sc out of range
#if HZ4_FAILFAST
        if (page->sc >= HZ4_SC_MAX) {
            HZ4_FAIL("pageq_notify: sc out of range");
        }
#endif

        // Select bucket based on size class
        uint32_t bucket = page->sc >> HZ4_PAGEQ_BUCKET_SHIFT;

        // Fail-Fast: bucket out of range
#if HZ4_FAILFAST
        if (bucket >= HZ4_PAGEQ_BUCKETS) {
            HZ4_FAIL("pageq_notify: bucket out of range");
        }
#endif

        hz4_page_t* old = atomic_load_explicit(&seg->pageq_head[bucket], memory_order_acquire);
        do {
            page->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, page,
            memory_order_release, memory_order_acquire));
        hz4_segq_notify_seg(seg);
    }
#endif
}
#else
static inline void hz4_pageq_notify(hz4_page_t* page) {
    (void)page;
}
#endif

// ============================================================================
// Debug: queue length (for testing only)
// ============================================================================
#if HZ4_FAILFAST
static inline uint32_t hz4_segq_count(uint8_t owner) {
    uint32_t count = 0;
    hz4_seg_t* cur = atomic_load_explicit(&g_hz4_segq[owner].head, memory_order_acquire);
    while (cur && count < 10000) {  // bounded to detect cycles
        count++;
        cur = cur->qnext;
    }
    return count;
}
#endif

#endif // HZ4_SEGQ_INC
