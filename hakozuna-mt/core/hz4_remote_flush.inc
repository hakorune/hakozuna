// hz4_remote_flush.inc - RemoteFlushBox (TLS rbuf → page.remote_head)
// Box Theory: 境界 API (2箇所のうち1つ)
//
// 設計:
// - TLS rbuf は固定長配列 (stale entry 問題を回避)
// - flush 時に page ごとにまとめて publish
// - n 本固定ループ (while(next) 禁止)
// - CAS retry 時に tail.next を毎回再設定 (Lane16 バグ修正)

#ifndef HZ4_REMOTE_FLUSH_INC
#define HZ4_REMOTE_FLUSH_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_segq.inc"
#if HZ4_REMOTE_INBOX
#include "hz4_inbox.inc"
#endif

// ============================================================================
// Remote Free Enqueue (hot path inline)
// ============================================================================
static inline void hz4_rbuf_push(hz4_tls_t* tls, hz4_page_t* page, void* obj) {
    if (tls->rlen >= HZ4_REMOTE_FLUSH_THRESHOLD) {
        hz4_remote_flush(tls);  // boundary call
    }
    tls->rbuf[tls->rlen].page = page;
    tls->rbuf[tls->rlen].obj = obj;
    tls->rlen++;
}

// ============================================================================
// Bucket for page-local batching
// ============================================================================
typedef struct hz4_flush_bucket {
    hz4_page_t* page;
    void* head;
    void* tail;
    uint32_t n;
} hz4_flush_bucket_t;

// ============================================================================
// Remote Flush (boundary API)
// ============================================================================
#if HZ4_REMOTE_INBOX
// Inbox bucket: group by (owner, sc)
typedef struct hz4_inbox_bucket {
    uint8_t owner;
    uint8_t sc;
    void* head;
    void* tail;
} hz4_inbox_bucket_t;

void hz4_remote_flush(hz4_tls_t* tls) {
    if (tls->rlen == 0) {
        return;
    }

    tls->flush_count++;

    // ---- n==1 fast path ----
    if (tls->rlen == 1) {
        hz4_page_t* page = tls->rbuf[0].page;
        void* obj = tls->rbuf[0].obj;
        hz4_obj_set_next(obj, NULL);
        uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
        uint8_t sc = (uint8_t)page->sc;
        hz4_inbox_push_one(owner, sc, obj);
        tls->rlen = 0;
        return;
    }

    // ---- n<=4 fast path (no bucketing) ----
    if (tls->rlen <= HZ4_REMOTE_FLUSH_FASTPATH_MAX) {
        for (uint32_t i = 0; i < tls->rlen; i++) {
            hz4_page_t* page = tls->rbuf[i].page;
            void* obj = tls->rbuf[i].obj;
            hz4_obj_set_next(obj, NULL);
            uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
            uint8_t sc = (uint8_t)page->sc;
            hz4_inbox_push_one(owner, sc, obj);
        }
        tls->rlen = 0;
        return;
    }

#if HZ4_REMOTE_FLUSH_PAGE_BUCKET
    // ---- General path: page-based bucketing ----
    hz4_flush_bucket_t pbuckets[HZ4_REMOTE_FLUSH_BUCKETS];
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        pbuckets[i].page = NULL;
        pbuckets[i].head = NULL;
        pbuckets[i].tail = NULL;
        pbuckets[i].n = 0;
    }

    // Phase 1: Distribute to buckets by page (n 本固定ループ)
    for (uint32_t i = 0; i < tls->rlen; i++) {
        hz4_page_t* page = tls->rbuf[i].page;
        void* obj = tls->rbuf[i].obj;
        hz4_obj_set_next(obj, NULL);

        // Hash: XOR mix for better distribution
        uintptr_t paddr = (uintptr_t)page;
        uint32_t base_idx = (uint32_t)(((paddr >> HZ4_PAGE_SHIFT) ^ (paddr >> 12)) & (HZ4_REMOTE_FLUSH_BUCKETS - 1));

        // Linear probe for collision
        uint32_t slot = base_idx;
        int found = 0;
        for (uint32_t probe = 0; probe < HZ4_REMOTE_FLUSH_PROBE; probe++) {
            slot = (base_idx + probe) & (HZ4_REMOTE_FLUSH_BUCKETS - 1);
            hz4_flush_bucket_t* b = &pbuckets[slot];
            if (b->page == NULL) {
                // New bucket
                b->page = page;
                b->head = obj;
                b->tail = obj;
                b->n = 1;
                found = 1;
                break;
            } else if (b->page == page) {
                // Same page: link to tail
                hz4_obj_set_next(b->tail, obj);
                b->tail = obj;
                b->n++;
                found = 1;
                break;
            }
        }

        if (!found) {
            // Probe limit exceeded: push single object immediately
            uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
            uint8_t sc = (uint8_t)page->sc;
#if HZ4_FAILFAST
            if (sc >= HZ4_SC_MAX) {
                HZ4_FAIL("hz4_remote_flush: invalid sc (probe overflow)");
            }
#endif
            hz4_inbox_push_one(owner, sc, obj);
        }
    }

    // Phase 2: Flush buckets to inbox
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        hz4_flush_bucket_t* b = &pbuckets[i];
        if (b->page == NULL || b->n == 0) {
            continue;
        }

        // Get owner/sc from page header
        uint8_t sc = (uint8_t)b->page->sc;
        uint16_t owner_tid = b->page->owner_tid;

#if HZ4_FAILFAST
        if (sc >= HZ4_SC_MAX) {
            HZ4_FAIL("hz4_remote_flush: invalid sc");
        }
#endif

        uint8_t owner = (uint8_t)hz4_owner_shard(owner_tid);

        // n==1 uses push_one (lightweight)
        if (b->n == 1) {
            hz4_inbox_push_one(owner, sc, b->head);
        } else {
            hz4_inbox_push_list(owner, sc, b->head, b->tail);
        }
    }

    tls->rlen = 0;
}
#else
    // ---- General path: bucket sort by (owner, sc) ----
    hz4_inbox_bucket_t ibuckets[HZ4_REMOTE_FLUSH_BUCKETS];
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        ibuckets[i].owner = 0xFF;  // sentinel
        ibuckets[i].sc = 0;
        ibuckets[i].head = NULL;
        ibuckets[i].tail = NULL;
    }

    // Phase 1: Distribute to buckets (n 本固定ループ)
    for (uint32_t i = 0; i < tls->rlen; i++) {
        hz4_page_t* page = tls->rbuf[i].page;
        void* obj = tls->rbuf[i].obj;
        hz4_obj_set_next(obj, NULL);

        uint8_t owner = (uint8_t)hz4_owner_shard(page->owner_tid);
        uint8_t sc = (uint8_t)page->sc;
        uint32_t idx = ((uint32_t)owner * 31u + (uint32_t)sc) & (HZ4_REMOTE_FLUSH_BUCKETS - 1);
        hz4_inbox_bucket_t* b = &ibuckets[idx];

        if (b->owner == 0xFF) {
            // New bucket
            b->owner = owner;
            b->sc = sc;
            b->head = obj;
            b->tail = obj;
        } else if (b->owner == owner && b->sc == sc) {
            // Same (owner, sc): link to tail
            hz4_obj_set_next(b->tail, obj);
            b->tail = obj;
        } else {
            // Collision: push single object immediately
            hz4_inbox_push_one(owner, sc, obj);
        }
    }

    // Phase 2: Flush buckets to inbox
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        hz4_inbox_bucket_t* b = &ibuckets[i];
        if (b->owner == 0xFF || b->head == NULL) {
            continue;
        }
        hz4_inbox_push_list(b->owner, b->sc, b->head, b->tail);
    }

    tls->rlen = 0;
}
#endif  // HZ4_REMOTE_FLUSH_PAGE_BUCKET
#else
// Original path: page.remote_head + segq/pageq
void hz4_remote_flush(hz4_tls_t* tls) {
    if (tls->rlen == 0) {
        return;
    }

    tls->flush_count++;

    // ---- n==1 fast path ----
    if (tls->rlen == 1) {
        hz4_page_t* page = tls->rbuf[0].page;
        void* obj = tls->rbuf[0].obj;
        hz4_obj_set_next(obj, NULL);  // 安全のため NULL 終端

        hz4_page_push_remote_one_tid(page, obj, tls->tid);

        // Notify pending queue
#if HZ4_PAGEQ_ENABLE
        hz4_pageq_notify(page);
#else
        hz4_seg_t* seg = hz4_seg_from_page(page);
        hz4_segq_notify(seg, hz4_page_idx(page));
#endif

        tls->rlen = 0;
        return;
    }

    // ---- n<=4 fast path (no bucketing) ----
    if (tls->rlen <= HZ4_REMOTE_FLUSH_FASTPATH_MAX) {
        for (uint32_t i = 0; i < tls->rlen; i++) {
            hz4_page_t* page = tls->rbuf[i].page;
            void* obj = tls->rbuf[i].obj;
            hz4_obj_set_next(obj, NULL);

            hz4_page_push_remote_one_tid(page, obj, tls->tid);

#if HZ4_PAGEQ_ENABLE
            hz4_pageq_notify(page);
#else
            hz4_seg_t* seg = hz4_seg_from_page(page);
            hz4_segq_notify(seg, hz4_page_idx(page));
#endif
        }
        tls->rlen = 0;
        return;
    }

    // ---- General path: bucket sort by page ----
    hz4_flush_bucket_t buckets[HZ4_REMOTE_FLUSH_BUCKETS];
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        buckets[i].page = NULL;
        buckets[i].head = NULL;
        buckets[i].tail = NULL;
        buckets[i].n = 0;
    }

    // Phase 1: Distribute to buckets (n 本固定ループ)
    for (uint32_t i = 0; i < tls->rlen; i++) {
        hz4_page_t* page = tls->rbuf[i].page;
        void* obj = tls->rbuf[i].obj;
        hz4_obj_set_next(obj, NULL);  // 安全のため NULL 終端

        uint32_t idx = (uint32_t)(((uintptr_t)page >> HZ4_PAGE_SHIFT) & (HZ4_REMOTE_FLUSH_BUCKETS - 1));
        hz4_flush_bucket_t* b = &buckets[idx];

        if (b->page == NULL) {
            // New bucket
            b->page = page;
            b->head = obj;
            b->tail = obj;
            b->n = 1;
        } else if (b->page == page) {
            // Same page: link to tail
            hz4_obj_set_next(b->tail, obj);
            b->tail = obj;
            b->n++;
        } else {
            // Collision: push single object immediately
            hz4_page_push_remote_one_tid(page, obj, tls->tid);
#if HZ4_PAGEQ_ENABLE
            hz4_pageq_notify(page);
#else
            hz4_seg_t* seg = hz4_seg_from_page(page);
            hz4_segq_notify(seg, hz4_page_idx(page));
#endif
        }
    }

    // Phase 2: Flush buckets to pages
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        hz4_flush_bucket_t* b = &buckets[i];
        if (b->page == NULL || b->n == 0) {
            continue;
        }

        // Push list to page (CAS with tail.next re-sync)
        hz4_page_push_remote_list_tid(b->page, b->head, b->tail, b->n, tls->tid);

        // Notify pending queue
#if HZ4_PAGEQ_ENABLE
        hz4_pageq_notify(b->page);
#else
        hz4_seg_t* seg = hz4_seg_from_page(b->page);
        hz4_segq_notify(seg, hz4_page_idx(b->page));
#endif
    }

    tls->rlen = 0;
}
#endif  // HZ4_REMOTE_INBOX

// ============================================================================
// Remote Free Entry Point (hot path)
// ============================================================================
static inline void hz4_remote_free(hz4_tls_t* tls, void* ptr) {
    hz4_page_t* page = hz4_page_from_ptr(ptr);

#if HZ4_FAILFAST
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("hz4_remote_free: invalid page");
    }
#endif

    hz4_rbuf_push(tls, page, ptr);
}

#endif // HZ4_REMOTE_FLUSH_INC
