#include "hz5.h"
#include "hz5_internal.h"

#include <stdint.h>

static uint32_t hz5_p1_ceil_div_pages(size_t size) {
    return (uint32_t)((size + HZ5_PAGE_SIZE - 1u) / HZ5_PAGE_SIZE);
}

static uint8_t hz5_p1_log2_size(size_t value) {
    uint8_t log2 = 0;
    while (value > 1u) {
        value >>= 1u;
        ++log2;
    }
    return log2;
}

static int hz5_p1_is_power_of_two(size_t value) {
    return value != 0 && (value & (value - 1u)) == 0;
}

void* hz5_p1_alloc_aligned(size_t size, size_t alignment) {
    if (size == 0 || alignment < HZ5_PAGE_SIZE ||
        !hz5_p1_is_power_of_two(alignment)) {
        return NULL;
    }

    uint32_t pages = hz5_p1_ceil_div_pages(size);
    uint32_t align_pages = (uint32_t)((alignment + HZ5_PAGE_SIZE - 1u) /
                                      HZ5_PAGE_SIZE);
    Hz5Seg* seg = hz5_p1_segment_get();
    if (!seg) {
        return NULL;
    }
    return hz5_p1_segment_alloc_run_aligned(seg,
                                            pages,
                                            align_pages,
                                            hz5_p1_log2_size(alignment),
                                            (uint16_t)pages,
                                            (Hz5OwnerToken){0, 0});
}

void hz5_p1_free(void* ptr) {
    Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
    if (!seg) {
        return;
    }
    uint32_t page = hz5_p1_page_index(seg, ptr);
    if (page >= HZ5_SEG_PAGES) {
        return;
    }
    if (((uintptr_t)ptr & (HZ5_PAGE_SIZE - 1u)) != 0) {
        return;
    }
    hz5_p1_segment_free_run(seg, page);
}

int hz5_p1_owns(void* ptr) {
    return hz5_p1_seg_from_ptr(ptr) != NULL;
}

int hz5_p1_validate(void* ptr, size_t size, size_t alignment) {
    Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
    if (!seg || !ptr || alignment == 0) {
        return 0;
    }
    if (((uintptr_t)ptr % alignment) != 0) {
        return 0;
    }
    if (((uintptr_t)ptr & (HZ5_PAGE_SIZE - 1u)) != 0) {
        return 0;
    }
    uint32_t page = hz5_p1_page_index(seg, ptr);
    if (page >= HZ5_SEG_PAGES) {
        return 0;
    }
    Hz5PageMeta* meta = &seg->page[page];
    if (meta->kind != HZ5_PAGE_RUN_START) {
        return 0;
    }
    if (meta->run_pages != hz5_p1_ceil_div_pages(size)) {
        return 0;
    }
    if (meta->align_log2 != hz5_p1_log2_size(alignment)) {
        return 0;
    }
    return 1;
}

void* hz5_p2_alloc_aligned(size_t size, size_t alignment) {
    if (size == 0 || alignment < HZ5_PAGE_SIZE ||
        !hz5_p1_is_power_of_two(alignment)) {
        return NULL;
    }

    uint32_t pages = hz5_p1_ceil_div_pages(size);
    uint32_t align_pages = (uint32_t)((alignment + HZ5_PAGE_SIZE - 1u) /
                                      HZ5_PAGE_SIZE);
    uint8_t align_log2 = hz5_p1_log2_size(alignment);
    uint16_t sc = hz5_run_sc_for_size(size, pages);
    hz5_stats_inc_pages(HZ5_STAT_ALLOC_CALL, pages);
    void* cached = hz5_tcache_pop(pages, align_log2, sc);
    if (cached) {
        hz5_stats_inc_pages(HZ5_STAT_ALLOC_TCACHE_HIT, pages);
        return cached;
    }

    hz5_stats_inc_pages(HZ5_STAT_ALLOC_DRAIN_CALL, pages);
    size_t drained = hz5_p2_drain_current_owner();
    if (drained != 0) {
        hz5_stats_inc_pages(HZ5_STAT_ALLOC_DRAIN_HIT, pages);
    }
    cached = hz5_tcache_pop(pages, align_log2, sc);
    if (cached) {
        hz5_stats_inc_pages(HZ5_STAT_ALLOC_TCACHE_HIT, pages);
        return cached;
    }

    void* ptr = hz5_p1_segment_alloc_any_run_aligned(pages,
                                                     align_pages,
                                                     align_log2,
                                                     sc,
                                                     hz5_owner_current());
    if (ptr) {
        hz5_stats_inc_pages(HZ5_STAT_ALLOC_SEGMENT, pages);
    }
    return ptr;
}

void hz5_p2_free(void* ptr) {
    Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
    if (!seg) {
        return;
    }
    uint32_t page = hz5_p1_page_index(seg, ptr);
    if (page >= HZ5_SEG_PAGES) {
        return;
    }
    if (((uintptr_t)ptr & (HZ5_PAGE_SIZE - 1u)) != 0) {
        return;
    }

    Hz5PageMeta* meta = &seg->page[page];
    if (meta->kind != HZ5_PAGE_RUN_START) {
        return;
    }

    Hz5OwnerToken current = hz5_owner_current();
    if (hz5_owner_equal(meta->owner, current)) {
        if (!hz5_tcache_push(ptr)) {
            hz5_stats_inc_pages(HZ5_STAT_FREE_LOCAL_RELEASE, meta->run_pages);
            hz5_p1_segment_free_run(seg, page);
        } else {
            hz5_stats_inc_pages(HZ5_STAT_FREE_LOCAL_CACHE, meta->run_pages);
        }
        return;
    }

    if (!hz5_owner_is_alive(meta->owner)) {
        hz5_stats_inc_pages(HZ5_STAT_OWNER_DESTRUCTOR_RELEASE, meta->run_pages);
        hz5_p1_segment_free_run(seg, page);
        return;
    }

    hz5_stats_inc_pages(HZ5_STAT_FREE_REMOTE_BUFFERED, meta->run_pages);
    hz5_remote_buffer_add(seg, page, ptr, meta->owner);
}

size_t hz5_p2_flush_remote(void) {
    return hz5_remote_flush_buffer(hz5_remote_tls_buffer());
}

size_t hz5_p2_drain_current_owner(void) {
    return hz5_remote_drain_owner(hz5_owner_current());
}
