#define _GNU_SOURCE

#include "hz3_seg_hdr.h"

#include "hz3_arena.h"
#include "hz3_config.h"
#include "hz3_segment.h"
#include "hz3_tcache.h"

#include <stdlib.h>

static inline void hz3_seg_hdr_init_bits(Hz3SegHdr* hdr) {
    for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
        hdr->free_bits[i] = UINT64_MAX;
    }
    hz3_bitmap_mark_used(hdr->free_bits, 0, 1);
    hdr->free_pages = (uint16_t)(HZ3_PAGES_PER_SEG - 1);

    // S62-1b: zero-initialize sub4k_run_start_bits
    for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
        hdr->sub4k_run_start_bits[i] = 0;
    }

#if HZ3_LANE_T16_R90_PAGE_REMOTE
    atomic_store_explicit(&hdr->t16_qstate, 0, memory_order_relaxed);
    hdr->t16_qnext = NULL;
    for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
        atomic_store_explicit(&hdr->t16_remote_pending[i], 0, memory_order_relaxed);
    }
#endif

#if HZ3_S110_META_ENABLE
    // S110: zero-initialize page_bin_plus1 (seg alloc is slow, loop OK)
    for (size_t i = 0; i < HZ3_PAGES_PER_SEG; i++) {
        atomic_store_explicit(&hdr->page_bin_plus1[i], 0, memory_order_relaxed);
    }
#endif
}

Hz3SegHdr* hz3_seg_from_ptr(const void* ptr) {
#if !HZ3_SEG_SELF_DESC_ENABLE
    (void)ptr;
    return NULL;
#else
#if HZ3_S12_V2_STATS
    if (t_hz3_cache.initialized) {
        t_hz3_cache.stats.s12_v2_seg_from_ptr_calls++;
    }
#endif

    uint32_t idx;
    void* base;
    // Use fast path: no pthread_once, returns 0 if arena not initialized
    if (!hz3_arena_contains_fast(ptr, &idx, &base)) {
#if HZ3_S12_V2_STATS
        if (t_hz3_cache.initialized) {
            t_hz3_cache.stats.s12_v2_seg_from_ptr_miss++;
        }
#endif
        return NULL;
    }

    Hz3SegHdr* hdr = (Hz3SegHdr*)base;
    if (hdr->magic != HZ3_SEG_HDR_MAGIC) {
#if HZ3_SMALL_V2_FAILFAST
        abort();
#endif
#if HZ3_S12_V2_STATS
        if (t_hz3_cache.initialized) {
            t_hz3_cache.stats.s12_v2_seg_from_ptr_miss++;
        }
#endif
        return NULL;
    }
#if HZ3_S12_V2_STATS
    if (t_hz3_cache.initialized) {
        t_hz3_cache.stats.s12_v2_seg_from_ptr_hit++;
    }
#endif
    return hdr;
#endif
}

Hz3SegHdr* hz3_seg_alloc_small(uint8_t owner) {
#if !HZ3_SEG_SELF_DESC_ENABLE
    (void)owner;
    return NULL;
#else
    uint32_t arena_idx = 0;
    void* base = hz3_arena_alloc(&arena_idx);
    if (!base) {
        return NULL;
    }

    Hz3SegHdr* hdr = (Hz3SegHdr*)base;
    hdr->magic = HZ3_SEG_HDR_MAGIC;
    hdr->kind = HZ3_SEG_KIND_SMALL;
    hdr->owner = owner;
    hdr->reserved = 0;
    hdr->flags = 0;
    hdr->reserved2 = 0;
    hdr->reserved3 = 0;

    hz3_seg_hdr_init_bits(hdr);

#if HZ3_LANE_T16_R90_PAGE_REMOTE
    if (t_hz3_cache.initialized) {
        uint16_t count = t_hz3_cache.t16_small_seg_count;
        if (count < HZ3_T16_SMALL_SEG_CAP) {
            t_hz3_cache.t16_small_seg_ids[count] = arena_idx;
            t_hz3_cache.t16_small_seg_count = (uint16_t)(count + 1);
        }
    }
#endif
    return hdr;
#endif
}
