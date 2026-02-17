// hz4_collect_seg_drain.inc - SegmentDrainBox (Segment drain and release functions)
// Box Theory: Separated from CollectBox for better modularity

#ifndef HZ4_COLLECT_SEG_DRAIN_INC
#define HZ4_COLLECT_SEG_DRAIN_INC

#include "hz4_tls.h"
#include "hz4_page.h"
#include "hz4_seg.h"

#if HZ4_SEG_RELEASE_EMPTY && HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT
static inline bool hz4_seg_can_release_empty(hz4_tls_t* tls, hz4_seg_t* seg) {
    if (!seg || !hz4_seg_valid(seg)) {
        return false;
    }

    // Never release a segment still used for new allocations.
    if (seg == tls->cur_seg) {
        return false;
    }

    // Segment must not be in segq processing or queued state.
    if (atomic_load_explicit(&seg->qstate, memory_order_acquire) != HZ4_QSTATE_IDLE) {
        return false;
    }

#if HZ4_PAGEQ_ENABLE
    for (uint32_t b = 0; b < HZ4_PAGEQ_BUCKETS; b++) {
        if (atomic_load_explicit(&seg->pageq_head[b], memory_order_acquire) != NULL) {
            return false;
        }
    }
#endif

    if (hz4_pending_any(seg)) {
        return false;
    }

    // Safety: no carry slots referencing this segment.
    for (uint32_t sc = 0; sc < HZ4_SC_MAX; sc++) {
        if (tls->carry[sc].n == 0) {
            continue;
        }
        for (uint32_t s = 0; s < HZ4_CARRY_SLOTS; s++) {
            hz4_page_t* p = tls->carry[sc].slot[s].page;
            if (p && hz4_seg_from_page(p) == seg) {
                return false;
            }
        }
    }

#if HZ4_REMOTE_INBOX
    // Safety: no inbox stash head from this segment.
    for (uint32_t sc = 0; sc < HZ4_SC_MAX; sc++) {
        void* head = tls->inbox_stash[sc];
        if (head && hz4_seg_from_page(hz4_page_from_ptr(head)) == seg) {
            return false;
        }
    }
#endif

    // All pages must be fully empty+decommitted and not queued anywhere.
    for (uint32_t i = HZ4_PAGE_IDX_MIN; i < HZ4_PAGES_PER_SEG; i++) {
        hz4_page_meta_t* meta = hz4_page_meta_from_seg(seg, i);
        if (!hz4_page_is_initialized(meta)) {
            // Segment still has never-used pages => cannot be a fully-retired segment.
            return false;
        }
        if (meta->queued || meta->queued_decommit) {
            return false;
        }
#if HZ4_REMOTE_PAGE_RBUF
        if (meta->rbufq_queued) {
            return false;
        }
#endif
        if (meta->used_count != 0) {
            return false;
        }
        if (!meta->decommitted) {
            return false;
        }
        for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
            if (atomic_load_explicit(&meta->remote_head[s], memory_order_acquire) != NULL) {
                return false;
            }
        }
#if HZ4_REMOTE_MASK
        if (atomic_load_explicit(&meta->remote_mask, memory_order_acquire) != 0) {
            return false;
        }
#endif
    }

    return true;
}

static inline void hz4_seg_try_release_empty(hz4_tls_t* tls, hz4_seg_t* seg) {
    if (!hz4_seg_can_release_empty(tls, seg)) {
        return;
    }

#if HZ4_SEG_REUSE_POOL
    // Try to push to TLS pool for reuse
    if (tls->seg_reuse_pool_n < HZ4_SEG_REUSE_POOL_MAX) {
        tls->seg_reuse_pool[tls->seg_reuse_pool_n] = seg;
        tls->seg_reuse_pool_n++;
        return;
    }
#endif

    // Pool full or disabled: release to OS
    hz4_os_seg_release(seg);
}
#endif

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

#if HZ4_PAGEQ_DRAIN_PAGE_BUDGET
    uint32_t pages_left = HZ4_PAGEQ_DRAIN_PAGE_BUDGET;
    while (pagelist && got < budget && pages_left > 0) {
        pages_left--;
#else
    while (pagelist && got < budget) {
#endif
        hz4_page_t* page = pagelist;
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* meta = hz4_page_meta(page);
        pagelist = meta->qnext;
        meta->qnext = NULL;

        if (hz4_page_valid(page) && meta->sc == sc) {
            uint32_t drained = hz4_drain_page(tls, sc, page, out + got, budget - got);
            got += drained;
  #if HZ4_DECOMMIT_OBSERVE
            fprintf(stderr, "[HZ4_DRAIN_PAGE] page=%p sc=%u valid=%d drained=%u\n",
                    (void*)page, meta->sc, hz4_page_valid(page), drained);
  #endif
#if HZ4_STATS
            if (drained > 0) tls->pages_drained++;
#endif

            // Check if page still has remote objects
            bool still = false;
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&meta->remote_head[s], memory_order_acquire) != NULL) {
                    still = true;
                    break;
                }
            }

            if (still) {
                atomic_store_explicit(&meta->queued, 1, memory_order_release);
            } else {
                atomic_store_explicit(&meta->queued, 0, memory_order_release);
                // Race hardening:
                // A producer may push to remote_head after our scan but before we clear queued=0.
                // If that happens, the page can become stranded (remote_head!=NULL but not queued).
                bool raced = false;
                for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                    if (atomic_load_explicit(&meta->remote_head[s], memory_order_acquire) != NULL) {
                        raced = true;
                        break;
                    }
                }
                if (raced) {
                    atomic_store_explicit(&meta->queued, 1, memory_order_release);
                    // Requeue as if "still", and skip decommit handling.
                    if (!requeue_head) {
                        requeue_head = page;
                        requeue_tail = page;
                    } else {
                        hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
                        requeue_meta->qnext = page;
                        requeue_tail = page;
                    }
                    continue;
                }
#if HZ4_PAGE_DECOMMIT
  #if HZ4_DECOMMIT_DELAY_QUEUE
                if (meta->used_count == 0) {
                    hz4_decommit_queue_enqueue(tls, page, meta);
                }
  #else
                hz4_page_try_decommit(page, meta);
#if HZ4_DECOMMIT_REUSE_POOL
                if (meta->decommitted) {
                    hz4_reuse_pool_push(tls, page, meta);
                }
#endif
  #endif
#endif
                continue;
            }
        }

        // Requeue page if not fully drained or sc mismatch
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
            requeue_meta->qnext = page;
            requeue_tail = page;
        }
#else
        pagelist = page->qnext;
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
                // Race hardening (non-meta):
                // Re-check remote_head after clearing queued=0 to avoid stranding remote objects.
                bool raced = false;
                for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                    if (atomic_load_explicit(&page->remote_head[s], memory_order_acquire) != NULL) {
                        raced = true;
                        break;
                    }
                }
                if (raced) {
                    atomic_store_explicit(&page->queued, 1, memory_order_release);
                    if (!requeue_head) {
                        requeue_head = page;
                        requeue_tail = page;
                    } else {
                        requeue_tail->qnext = page;
                        requeue_tail = page;
                    }
                    continue;
                }
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
#endif
    }

    // If budget ran out, requeue remaining pages
    while (pagelist) {
        hz4_page_t* page = pagelist;
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* meta = hz4_page_meta(page);
        pagelist = meta->qnext;
        meta->qnext = NULL;
        atomic_store_explicit(&meta->queued, 1, memory_order_release);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
            requeue_meta->qnext = page;
            requeue_tail = page;
        }
#else
        pagelist = page->qnext;
        page->qnext = NULL;
        atomic_store_explicit(&page->queued, 1, memory_order_release);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
#endif
    }

    if (requeue_head) {
        hz4_page_t* old = atomic_load_explicit(&seg->pageq_head[bucket], memory_order_acquire);
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
        do {
            requeue_meta->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, requeue_head,
            memory_order_release, memory_order_acquire));
#else
        do {
            requeue_tail->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, requeue_head,
            memory_order_release, memory_order_acquire));
#endif
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
#if HZ4_PAGE_META_SEPARATE
            hz4_page_meta_t* meta = hz4_page_meta(page);
            if (hz4_page_valid(page) && meta->sc == sc) {
#else
            if (hz4_page_valid(page) && page->sc == sc) {
#endif
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
// hz4_drain_segment_list: PageQ から segment を drain して list を返す
// ============================================================================
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

#if HZ4_HANG_OBS
    // HangGuard (CollectBox boundary): bound pageq scan cost and detect small cycles/duplicates.
    // This is debug-only (requires FAILFAST) and must not be enabled in perf lanes.
    uint32_t hang_scan = 0;
    uint32_t hang_recent_pos = 0;
    hz4_page_t* hang_recent[HZ4_HANG_OBS_RECENT] = {0};

#define HZ4_HANG_OBS_CHECK(_phase, _page, _next) do { \
        hang_scan++; \
        if (hang_scan > HZ4_HANG_OBS_SCAN_LIMIT) { \
            fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=%s tid=%u sc=%u bucket=%u got=%u budget=%u seg=%p page=%p next=%p scan=%u\n", \
                    (_phase), (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (unsigned)got, (unsigned)budget, \
                    (void*)seg, (void*)(_page), (void*)(_next), (unsigned)hang_scan); \
            HZ4_FAIL("HZ4_HANG: scan_limit"); \
        } \
        if ((_next) == (_page)) { \
            fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=%s tid=%u sc=%u bucket=%u got=%u budget=%u seg=%p self_qnext=%p scan=%u\n", \
                    (_phase), (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (unsigned)got, (unsigned)budget, \
                    (void*)seg, (void*)(_page), (unsigned)hang_scan); \
            HZ4_FAIL("HZ4_HANG: self_qnext"); \
        } \
        for (uint32_t __i = 0; __i < (uint32_t)HZ4_HANG_OBS_RECENT; __i++) { \
            if (hang_recent[__i] == (_page)) { \
                fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=%s tid=%u sc=%u bucket=%u got=%u budget=%u seg=%p repeat_page=%p scan=%u\n", \
                        (_phase), (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (unsigned)got, (unsigned)budget, \
                        (void*)seg, (void*)(_page), (unsigned)hang_scan); \
                HZ4_FAIL("HZ4_HANG: repeat_page"); \
            } \
        } \
        hang_recent[hang_recent_pos & ((uint32_t)HZ4_HANG_OBS_RECENT - 1)] = (_page); \
        hang_recent_pos++; \
    } while (0)
#endif

    while (pagelist && got < budget) {
        hz4_page_t* page = pagelist;
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* meta = hz4_page_meta(page);
        hz4_page_t* nextp = meta->qnext;
#if HZ4_HANG_OBS
        HZ4_HANG_OBS_CHECK("main", page, nextp);
#endif
        pagelist = nextp;
        meta->qnext = NULL;

#if HZ4_CENTRAL_PAGEHEAP
        // PageQ is a segment-local remote-notify mechanism; it must not track pages managed by CPH.
        // If a CPH-managed page leaks into PageQ, PageQ drain can become pathological (loops/queue corruption).
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
#if HZ4_FAILFAST
            HZ4_FAIL("drain_segment_list: page is in central pageheap");
#endif
            atomic_store_explicit(&meta->queued, 0, memory_order_release);
            continue;
        }
#endif

        if (hz4_page_valid(page) && meta->sc == sc) {
            void* ph = NULL;
            void* pt = NULL;
            uint32_t drained = hz4_drain_page_list(tls, sc, page, &ph, &pt, budget - got);
  #if HZ4_DECOMMIT_OBSERVE
            fprintf(stderr, "[HZ4_DRAIN_LIST] page=%p sc=%u drained=%u\n",
                    (void*)page, meta->sc, drained);
  #endif
            if (drained > 0) {
                hz4_list_splice(&head, &tail, &got, ph, pt, drained);
#if HZ4_STATS
                tls->pages_drained++;
#endif
            }

            // Check if page still has remote objects
            bool still = false;
            for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                if (atomic_load_explicit(&meta->remote_head[s], memory_order_acquire) != NULL) {
                    still = true;
                    break;
                }
            }

            if (still) {
                atomic_store_explicit(&meta->queued, 1, memory_order_release);
            } else {
                // Clear queued first, then re-check remote_head to avoid losing a concurrent remote push:
                // - Producer may push after we observed "still=false" but before we clear queued.
                // - In that race, producer sees queued==1 and does NOT enqueue again; if we clear queued
                //   without re-checking, the page can become (remote_head!=NULL, queued==0, not queued),
                //   leading to pathological stalls.
                atomic_store_explicit(&meta->queued, 0, memory_order_release);

                // Re-check after clearing queued. If a remote push raced, keep queued=1 and requeue.
                for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
                    if (atomic_load_explicit(&meta->remote_head[s], memory_order_acquire) != NULL) {
                        still = true;
                        atomic_store_explicit(&meta->queued, 1, memory_order_release);
                        break;
                    }
                }

                if (still) {
                    // Fall through to requeue path below.
                } else {
#if HZ4_PAGE_DECOMMIT
  #if HZ4_DECOMMIT_DELAY_QUEUE
                    if (meta->used_count == 0) {
                        hz4_decommit_queue_enqueue(tls, page, meta);
                    }
  #else
                    hz4_page_try_decommit(page, meta);
#if HZ4_DECOMMIT_REUSE_POOL
                    if (meta->decommitted) {
                        hz4_reuse_pool_push(tls, page, meta);
                    }
#endif
  #endif
#endif
                    continue;
                }
            }
        }

        // Requeue
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
#if HZ4_HANG_OBS
            if (requeue_tail == page) {
                fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=main tid=%u sc=%u bucket=%u seg=%p self_requeue_tail=%p scan=%u\n",
                        (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (void*)seg, (void*)page, (unsigned)hang_scan);
                HZ4_FAIL("HZ4_HANG: self_requeue_tail");
            }
#endif
            hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
            requeue_meta->qnext = page;
            requeue_tail = page;
        }
#else
        hz4_page_t* nextp = page->qnext;
#if HZ4_HANG_OBS
        HZ4_HANG_OBS_CHECK("main", page, nextp);
#endif
        pagelist = nextp;
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
#if HZ4_HANG_OBS
            if (requeue_tail == page) {
                fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=main tid=%u sc=%u bucket=%u seg=%p self_requeue_tail=%p scan=%u\n",
                        (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (void*)seg, (void*)page, (unsigned)hang_scan);
                HZ4_FAIL("HZ4_HANG: self_requeue_tail");
            }
#endif
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
#endif
    }

    // Budget exhausted - requeue remaining
    while (pagelist) {
        hz4_page_t* page = pagelist;
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* meta = hz4_page_meta(page);
        hz4_page_t* nextp = meta->qnext;
#if HZ4_HANG_OBS
        HZ4_HANG_OBS_CHECK("budget_requeue", page, nextp);
#endif
        pagelist = nextp;
        meta->qnext = NULL;
#if HZ4_CENTRAL_PAGEHEAP
        if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
#if HZ4_FAILFAST
            HZ4_FAIL("drain_segment_list: page is in central pageheap (budget-requeue)");
#endif
            atomic_store_explicit(&meta->queued, 0, memory_order_release);
            continue;
        }
#endif
        atomic_store_explicit(&meta->queued, 1, memory_order_release);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
#if HZ4_HANG_OBS
            if (requeue_tail == page) {
                fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=budget_requeue tid=%u sc=%u bucket=%u seg=%p self_requeue_tail=%p scan=%u\n",
                        (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (void*)seg, (void*)page, (unsigned)hang_scan);
                HZ4_FAIL("HZ4_HANG: self_requeue_tail");
            }
#endif
            hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
            requeue_meta->qnext = page;
            requeue_tail = page;
        }
#else
        hz4_page_t* nextp = page->qnext;
#if HZ4_HANG_OBS
        HZ4_HANG_OBS_CHECK("budget_requeue", page, nextp);
#endif
        pagelist = nextp;
        page->qnext = NULL;
        atomic_store_explicit(&page->queued, 1, memory_order_release);
        if (!requeue_head) {
            requeue_head = page;
            requeue_tail = page;
        } else {
#if HZ4_HANG_OBS
            if (requeue_tail == page) {
                fprintf(stderr, "[HZ4_HANG] drain_segment_list phase=budget_requeue tid=%u sc=%u bucket=%u seg=%p self_requeue_tail=%p scan=%u\n",
                        (unsigned)tls->tid, (unsigned)sc, (unsigned)bucket, (void*)seg, (void*)page, (unsigned)hang_scan);
                HZ4_FAIL("HZ4_HANG: self_requeue_tail");
            }
#endif
            requeue_tail->qnext = page;
            requeue_tail = page;
        }
#endif
    }

    if (requeue_head) {
        hz4_page_t* old = atomic_load_explicit(&seg->pageq_head[bucket], memory_order_acquire);
#if HZ4_PAGE_META_SEPARATE
        hz4_page_meta_t* requeue_meta = hz4_page_meta(requeue_tail);
        do {
            requeue_meta->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, requeue_head,
            memory_order_release, memory_order_acquire));
#else
        do {
            requeue_tail->qnext = old;
        } while (!atomic_compare_exchange_weak_explicit(
            &seg->pageq_head[bucket], &old, requeue_head,
            memory_order_release, memory_order_acquire));
#endif
    }

    *head_out = head;
    *tail_out = tail;
#if HZ4_HANG_OBS
#undef HZ4_HANG_OBS_CHECK
#endif
    return got;
}
#endif // HZ4_PAGEQ_ENABLE

#endif // HZ4_COLLECT_SEG_DRAIN_INC
