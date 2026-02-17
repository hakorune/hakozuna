// hz4_segment.c - HZ4 Segment Box
#include <string.h>
#include <stdlib.h>
#include "hz4_config.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_segment.h"
#include "hz4_os.h"
#include "hz4_tls_init.h"


void hz4_seg_init_pages(hz4_seg_t* seg, uint8_t owner) {
    (void)owner;
    for (uint32_t i = HZ4_PAGE_IDX_MIN; i < HZ4_PAGES_PER_SEG; i++) {
        hz4_page_t* page = hz4_page_from_seg(seg, i);
#if !HZ4_PAGE_META_SEPARATE
        // Original initialization (all fields in page struct)
        page->free = NULL;
        page->local_free = NULL;
        for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
            atomic_store_explicit(&page->remote_head[s], NULL, memory_order_relaxed);
        }
#if HZ4_REMOTE_MASK
        atomic_store_explicit(&page->remote_mask, 0, memory_order_relaxed);
#endif
        page->magic = HZ4_PAGE_MAGIC;
        page->owner_tid = 0;
        page->sc = 0;
        page->used_count = 0;
        page->capacity = 0;
        atomic_store_explicit(&page->queued, 0, memory_order_relaxed);
        page->qnext = NULL;
#else
        // NEW: Initialize page (thin) + meta (separated)
        page->free = NULL;
        page->local_free = NULL;
        page->magic = HZ4_PAGE_MAGIC;
        page->_reserved = 0;

        // Initialize metadata
        hz4_page_meta_t* meta = hz4_page_meta_from_seg(seg, i);
        for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
            atomic_store_explicit(&meta->remote_head[s], NULL, memory_order_relaxed);
        }
#if HZ4_REMOTE_MASK
        atomic_store_explicit(&meta->remote_mask, 0, memory_order_relaxed);
#endif
	        atomic_store_explicit(&meta->queued, 0, memory_order_relaxed);
#if HZ4_REMOTE_PAGE_RBUF
	        atomic_store_explicit(&meta->rbufq_queued, 0, memory_order_relaxed);
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
	        atomic_store_explicit(&meta->rbufq_lazy_left, 0, memory_order_relaxed);
	        atomic_store_explicit(&meta->rbufq_empty_streak, 0, memory_order_relaxed);
#endif
#endif
	        meta->qnext = NULL;
#if HZ4_REMOTE_PAGE_RBUF
	        meta->rbufq_next = NULL;
#endif
	        meta->owner_tid = 0;
	        meta->sc = 0;
	        meta->used_count = 0;
	        meta->capacity = 0;
#if HZ4_POPULATE_BATCH
        meta->bump_off = 0;
        meta->bump_left = 0;
#endif
        meta->decommitted = 0;
        meta->initialized = 0;
#if HZ4_CENTRAL_PAGEHEAP
        atomic_store_explicit(&meta->cph_queued, 0, memory_order_relaxed);
        meta->cph_next = NULL;
#if HZ4_CPH_2TIER
        atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_relaxed);
        meta->seal_epoch = 0;
#endif
#endif
#endif
    }
}

hz4_seg_t* hz4_seg_create(uint8_t owner) {
#if HZ4_SEG_REUSE_POOL
    // Try to reuse from TLS pool first (before seg_acq_epoch++)
    {
        hz4_tls_t* tls = hz4_tls_get();
        if (tls->seg_reuse_pool_n > 0) {
            tls->seg_reuse_pool_n--;
            hz4_seg_t* seg = tls->seg_reuse_pool[tls->seg_reuse_pool_n];
            if (hz4_seg_valid(seg)) {
                // Complete re-initialization (like new segment)
                seg->magic = HZ4_SEG_MAGIC;
                seg->kind = 0;
                seg->owner = owner;
                atomic_store_explicit(&seg->qstate, HZ4_QSTATE_IDLE, memory_order_relaxed);
                seg->qnext = NULL;
                for (uint32_t b = 0; b < HZ4_PAGEQ_BUCKETS; b++) {
                    atomic_store_explicit(&seg->pageq_head[b], NULL, memory_order_relaxed);
                }
                for (uint32_t w = 0; w < HZ4_PAGEWORDS; w++) {
                    atomic_store_explicit(&seg->pending_bits[w], 0, memory_order_relaxed);
                }
#if !HZ4_SEG_REUSE_LAZY_INIT
                hz4_seg_init_pages(seg, owner);
#endif
                hz4_os_seg_research_advise(seg);
                return seg;
            }
            // Invalid seg: fall through to OS allocation
        }
    }
#endif

#if HZ4_SEG_ACQ_GUARDBOX || HZ4_SEG_ACQ_GATEBOX || HZ4_RSSRETURN_COOL_PRESSURE_SEG_ACQ
    // Count segment acquisition for guard/gate (OS acquire only)
    {
        hz4_tls_t* tls = hz4_tls_get();
        tls->seg_acq_epoch++;
    }
#endif
    void* mem = hz4_os_seg_acquire();
    if (!mem) {
        abort();
    }

    // Research-only: keep knobs isolated in OS box.
    hz4_os_seg_research_advise(mem);

#if HZ4_SEG_ZERO_FULL
    memset(mem, 0, (size_t)HZ4_SEG_SIZE);
#endif

    hz4_seg_t* seg = (hz4_seg_t*)mem;
    seg->magic = HZ4_SEG_MAGIC;
    seg->kind = 0;
    seg->owner = owner;
    atomic_store_explicit(&seg->qstate, HZ4_QSTATE_IDLE, memory_order_relaxed);
    seg->qnext = NULL;
    for (uint32_t b = 0; b < HZ4_PAGEQ_BUCKETS; b++) {
        atomic_store_explicit(&seg->pageq_head[b], NULL, memory_order_relaxed);
    }
    for (uint32_t w = 0; w < HZ4_PAGEWORDS; w++) {
        atomic_store_explicit(&seg->pending_bits[w], 0, memory_order_relaxed);
    }

#if !HZ4_SEG_CREATE_LAZY_INIT_PAGES
    hz4_seg_init_pages(seg, owner);
#endif
    return seg;
}
