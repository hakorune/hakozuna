// hz4_segment.c - HZ4 Segment Box
#include <string.h>
#include <stdlib.h>

#include "hz4_config.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_segment.h"
#include "hz4_os.h"

void hz4_seg_init_pages(hz4_seg_t* seg, uint8_t owner) {
    (void)owner;
    for (uint32_t i = HZ4_PAGE_IDX_MIN; i < HZ4_PAGES_PER_SEG; i++) {
        hz4_page_t* page = hz4_page_from_seg(seg, i);
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
    }
}

hz4_seg_t* hz4_seg_create(uint8_t owner) {
    void* mem = hz4_os_seg_acquire();
    if (!mem) {
        abort();
    }

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

    hz4_seg_init_pages(seg, owner);
    return seg;
}
