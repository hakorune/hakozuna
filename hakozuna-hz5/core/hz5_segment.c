#include "hz5_internal.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <malloc.h>
#endif

static _Atomic(Hz5Seg*) g_hz5_p1_segs[HZ5_SEG_MAX];
static _Atomic(uint32_t) g_hz5_p1_seg_count;
static _Atomic(uintptr_t) g_hz5_p1_seg_min_base;
static _Atomic(uintptr_t) g_hz5_p1_seg_max_end;
static atomic_flag g_hz5_p1_seg_lock = ATOMIC_FLAG_INIT;

static void hz5_p1_lock(void) {
    while (atomic_flag_test_and_set_explicit(&g_hz5_p1_seg_lock,
                                             memory_order_acquire)) {
    }
}

static void hz5_p1_unlock(void) {
    atomic_flag_clear_explicit(&g_hz5_p1_seg_lock, memory_order_release);
}

static int hz5_p1_bit_is_set(const Hz5Seg* seg, uint32_t page) {
    return (seg->free_bitmap[page >> 6u] & (1ULL << (page & 63u))) != 0;
}

static void hz5_p1_bit_set(Hz5Seg* seg, uint32_t page) {
    seg->free_bitmap[page >> 6u] |= (1ULL << (page & 63u));
}

static void hz5_p1_bit_clear(Hz5Seg* seg, uint32_t page) {
    seg->free_bitmap[page >> 6u] &= ~(1ULL << (page & 63u));
}

static int hz5_p1_run_is_free(const Hz5Seg* seg, uint32_t start, uint32_t pages) {
    if (start < HZ5_FIRST_DATA_PAGE || pages == 0 || start + pages > HZ5_SEG_PAGES) {
        return 0;
    }
    for (uint32_t i = 0; i < pages; ++i) {
        if (!hz5_p1_bit_is_set(seg, start + i)) {
            return 0;
        }
    }
    return 1;
}

static void hz5_p1_segment_init(Hz5Seg* seg) {
    memset(seg, 0, sizeof(*seg));
    seg->magic = HZ5_SEG_MAGIC;
    seg->version = HZ5_SEG_VERSION;
#if !HZ5_P11_SPEED_CORE
    seg->run16_a8192_hint_page = HZ5_FIRST_DATA_PAGE;
#endif

    for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
        hz5_p1_bit_set(seg, page);
        seg->page[page].kind = HZ5_PAGE_FREE;
        seg->page[page].decommit_state = HZ5_COMMITTED;
    }
}

#if !HZ5_P11_SPEED_CORE
static int hz5_p1_is_run16_a8192(uint32_t pages,
                                 uint32_t align_pages,
                                 uint8_t align_log2) {
    return pages == 16u && align_pages == 2u && align_log2 == 13u;
}

static uint32_t hz5_p1_align_page_up(uint32_t page, uint32_t align_pages) {
    if (align_pages <= 1u) {
        return page;
    }
    uint32_t rem = page % align_pages;
    if (rem == 0) {
        return page;
    }
    return page + (align_pages - rem);
}
#endif

static Hz5Seg* hz5_p1_segment_alloc(void) {
#if defined(_WIN32)
    Hz5Seg* seg = (Hz5Seg*)_aligned_malloc(HZ5_SEG_SIZE, HZ5_SEG_SIZE);
    if (!seg) {
        return NULL;
    }
#else
    void* mem = NULL;
    if (posix_memalign(&mem, HZ5_SEG_SIZE, HZ5_SEG_SIZE) != 0) {
        return NULL;
    }
    Hz5Seg* seg = (Hz5Seg*)mem;
#endif
    hz5_p1_segment_init(seg);
    return seg;
}

static void hz5_p1_publish_segment_range(Hz5Seg* seg) {
    uintptr_t base = (uintptr_t)seg;
    uintptr_t end = base + HZ5_SEG_SIZE;

    uintptr_t min_base = atomic_load_explicit(&g_hz5_p1_seg_min_base,
                                              memory_order_relaxed);
    while ((min_base == 0 || base < min_base) &&
           !atomic_compare_exchange_weak_explicit(&g_hz5_p1_seg_min_base,
                                                  &min_base,
                                                  base,
                                                  memory_order_release,
                                                  memory_order_relaxed)) {
    }

    uintptr_t max_end = atomic_load_explicit(&g_hz5_p1_seg_max_end,
                                             memory_order_relaxed);
    while (end > max_end &&
           !atomic_compare_exchange_weak_explicit(&g_hz5_p1_seg_max_end,
                                                  &max_end,
                                                  end,
                                                  memory_order_release,
                                                  memory_order_relaxed)) {
    }
}

#if !HZ5_P11_SPEED_CORE
static int hz5_p1_segment_is_empty_for_release(Hz5Seg* seg) {
    if (!seg || seg->magic != HZ5_SEG_MAGIC || seg->live_pages != 0) {
        return 0;
    }
    if (atomic_load_explicit(&seg->tcache_refs, memory_order_relaxed) != 0 ||
        atomic_load_explicit(&seg->remote_buffer_pending_hint,
                             memory_order_relaxed) != 0) {
        return 0;
    }
    for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
        Hz5PageMeta* meta = &seg->page[page];
        if (meta->kind != HZ5_PAGE_FREE) {
            return 0;
        }
        if (atomic_load_explicit(&meta->remote_head, memory_order_acquire) != NULL ||
            atomic_load_explicit(&meta->remote_count_hint, memory_order_relaxed) != 0) {
            return 0;
        }
        if (!hz5_p1_bit_is_set(seg, page)) {
            return 0;
        }
    }
    return 1;
}

static int hz5_p1_segment_is_retired(const Hz5Seg* seg) {
    return seg && (seg->flags & HZ5_SEG_FLAG_RETIRED) != 0;
}
#endif

#if HZ5_P14_RETIRED_QUARANTINE && !HZ5_P11_SPEED_CORE
static int hz5_p1_segment_has_remote_pending(Hz5Seg* seg) {
    if (!seg || seg->magic != HZ5_SEG_MAGIC) {
        return 0;
    }
    for (uint32_t page = HZ5_FIRST_DATA_PAGE; page < HZ5_SEG_PAGES; ++page) {
        Hz5PageMeta* meta = &seg->page[page];
        if (atomic_load_explicit(&meta->remote_head, memory_order_acquire) != NULL ||
            atomic_load_explicit(&meta->remote_count_hint, memory_order_relaxed) != 0) {
            return 1;
        }
    }
    return 0;
}
#endif

#if !HZ5_P11_SPEED_CORE
static void hz5_p1_segment_free_memory(Hz5Seg* seg) {
#if defined(_WIN32)
    _aligned_free(seg);
#else
    free(seg);
#endif
}
#endif

Hz5Seg* hz5_p1_segment_get(void) {
    hz5_p1_lock();
    uint32_t count = atomic_load_explicit(&g_hz5_p1_seg_count,
                                          memory_order_acquire);
    if (count == 0) {
        Hz5Seg* first = hz5_p1_segment_alloc();
        if (first) {
            hz5_p1_publish_segment_range(first);
            atomic_store_explicit(&g_hz5_p1_segs[0],
                                  first,
                                  memory_order_release);
            atomic_store_explicit(&g_hz5_p1_seg_count,
                                  1u,
                                  memory_order_release);
            count = 1u;
        }
    }
    Hz5Seg* seg = NULL;
#if HZ5_P11_SPEED_CORE
    seg = count != 0
              ? atomic_load_explicit(&g_hz5_p1_segs[0],
                                     memory_order_acquire)
              : NULL;
#else
    for (uint32_t i = 0; i < count; ++i) {
        Hz5Seg* candidate = atomic_load_explicit(&g_hz5_p1_segs[i],
                                                 memory_order_acquire);
        if (!hz5_p1_segment_is_retired(candidate)) {
            seg = candidate;
            break;
        }
    }
#endif
    hz5_p1_unlock();
    return seg;
}

uint32_t hz5_p1_segment_count(void) {
    return atomic_load_explicit(&g_hz5_p1_seg_count, memory_order_acquire);
}

Hz5Seg* hz5_p1_segment_at(uint32_t index) {
    uint32_t count = atomic_load_explicit(&g_hz5_p1_seg_count,
                                          memory_order_acquire);
    if (index >= count) {
        return NULL;
    }
    return atomic_load_explicit(&g_hz5_p1_segs[index], memory_order_acquire);
}

uint32_t hz5_p1_segment_retire_empty_quarantine(void) {
    uint32_t retired = 0;
#if HZ5_P14_RETIRED_QUARANTINE && !HZ5_P11_SPEED_CORE
    hz5_stats_inc(HZ5_STAT_P14_RETIRE_SCAN_CALL);
    hz5_p1_lock();
    uint32_t count = atomic_load_explicit(&g_hz5_p1_seg_count,
                                          memory_order_acquire);
    for (uint32_t i = 0; i < count; ++i) {
        Hz5Seg* seg = atomic_load_explicit(&g_hz5_p1_segs[i],
                                           memory_order_acquire);
        if (hz5_p1_segment_is_retired(seg)) {
            hz5_stats_inc(HZ5_STAT_P14_RETIRE_REJECT_STATE);
            continue;
        }
        if (!seg || seg->magic != HZ5_SEG_MAGIC || seg->live_pages != 0) {
            hz5_stats_inc(HZ5_STAT_P14_RETIRE_REJECT_LIVE);
            continue;
        }
        if (atomic_load_explicit(&seg->tcache_refs, memory_order_relaxed) != 0) {
            hz5_stats_inc(HZ5_STAT_P14_RETIRE_REJECT_STATE);
            continue;
        }
        if (atomic_load_explicit(&seg->remote_buffer_pending_hint,
                                 memory_order_relaxed) != 0) {
            hz5_stats_inc(HZ5_STAT_P14_RETIRE_REJECT_REMOTE);
            continue;
        }
        if (hz5_p1_segment_has_remote_pending(seg)) {
            hz5_stats_inc(HZ5_STAT_P14_RETIRE_REJECT_REMOTE);
            continue;
        }
        if (!hz5_p1_segment_is_empty_for_release(seg)) {
            hz5_stats_inc(HZ5_STAT_P14_RETIRE_REJECT_STATE);
            continue;
        }
        hz5_stats_inc(HZ5_STAT_P14_RETIRE_CANDIDATE_SEGMENT);
        hz5_stats_add(HZ5_STAT_P14_RETIRE_CANDIDATE_BYTES, HZ5_SEG_SIZE);
        seg->flags |= HZ5_SEG_FLAG_RETIRED;
        hz5_stats_inc(HZ5_STAT_P14_RETIRE_OK);
        hz5_stats_add(HZ5_STAT_P14_RETIRE_OK_BYTES, HZ5_SEG_SIZE);
        ++retired;
    }
    hz5_p1_unlock();
#endif
    return retired;
}

uint32_t hz5_p1_segment_release_empty_for_shutdown(void) {
    uint32_t released = 0;
#if HZ5_P11_SPEED_CORE
    return released;
#else
    hz5_p1_lock();
    uint32_t count = atomic_load_explicit(&g_hz5_p1_seg_count,
                                          memory_order_acquire);
    for (uint32_t i = 0; i < count; ++i) {
        Hz5Seg* seg = atomic_load_explicit(&g_hz5_p1_segs[i],
                                           memory_order_acquire);
        if (!hz5_p1_segment_is_empty_for_release(seg)) {
            continue;
        }
        atomic_store_explicit(&g_hz5_p1_segs[i], NULL, memory_order_release);
        hz5_p1_segment_free_memory(seg);
        ++released;
    }

    uint32_t new_count = count;
    while (new_count != 0) {
        Hz5Seg* seg = atomic_load_explicit(&g_hz5_p1_segs[new_count - 1u],
                                           memory_order_acquire);
        if (seg) {
            break;
        }
        --new_count;
    }
    atomic_store_explicit(&g_hz5_p1_seg_count, new_count, memory_order_release);
    if (new_count == 0) {
        atomic_store_explicit(&g_hz5_p1_seg_min_base, 0, memory_order_release);
        atomic_store_explicit(&g_hz5_p1_seg_max_end, 0, memory_order_release);
    }
    hz5_p1_unlock();
    return released;
#endif
}

static Hz5Seg* hz5_p1_segment_append(void) {
    hz5_p1_lock();
    uint32_t count = atomic_load_explicit(&g_hz5_p1_seg_count,
                                          memory_order_acquire);
    if (count >= HZ5_SEG_MAX) {
        hz5_p1_unlock();
        return NULL;
    }
    uint32_t slot = count;
    Hz5Seg* seg = hz5_p1_segment_alloc();
    if (seg) {
        hz5_p1_publish_segment_range(seg);
        atomic_store_explicit(&g_hz5_p1_segs[slot],
                              seg,
                              memory_order_release);
        atomic_store_explicit(&g_hz5_p1_seg_count,
                              slot + 1u,
                              memory_order_release);
    }
    hz5_p1_unlock();
    return seg;
}

void* hz5_p1_segment_alloc_run_aligned(Hz5Seg* seg,
                                       uint32_t pages,
                                       uint32_t align_pages,
                                       uint8_t align_log2,
                                       Hz5OwnerToken owner) {
    if (!seg || seg->magic != HZ5_SEG_MAGIC || pages == 0) {
        return NULL;
    }
    if (align_pages == 0) {
        align_pages = 1;
    }

    hz5_p1_lock();
#if HZ5_P11_SPEED_CORE
    for (uint32_t page = HZ5_FIRST_DATA_PAGE; page + pages <= HZ5_SEG_PAGES; ++page) {
        hz5_stats_inc_pages(HZ5_STAT_SEGMENT_ALLOC_SCAN_STEP, pages);
        if ((page % align_pages) != 0) {
            continue;
        }
        if (!hz5_p1_run_is_free(seg, page, pages)) {
            continue;
        }

        for (uint32_t i = 0; i < pages; ++i) {
            uint32_t p = page + i;
            hz5_p1_bit_clear(seg, p);
            seg->page[p].kind = (i == 0) ? HZ5_PAGE_RUN_START : HZ5_PAGE_RUN_CONT;
            seg->page[p].sc = (uint16_t)pages;
            seg->page[p].run_pages = (uint16_t)pages;
            seg->page[p].run_start_page = (uint16_t)page;
            seg->page[p].align_log2 = align_log2;
            seg->page[p].decommit_state = HZ5_COMMITTED;
            seg->page[p].owner = owner;
            atomic_store_explicit(&seg->page[p].remote_head,
                                  NULL,
                                  memory_order_relaxed);
            atomic_store_explicit(&seg->page[p].remote_count_hint,
                                  0,
                                  memory_order_relaxed);
        }
        seg->live_pages += pages;
        hz5_p1_unlock();
        return (void*)((uintptr_t)seg + ((uintptr_t)page * HZ5_PAGE_SIZE));
    }
#else
    int use_run16_hint = hz5_p1_is_run16_a8192(pages, align_pages, align_log2);
    uint32_t first_page = HZ5_FIRST_DATA_PAGE;
    uint32_t hint_page = 0;
    if (use_run16_hint &&
        seg->run16_a8192_hint_page >= HZ5_FIRST_DATA_PAGE &&
        seg->run16_a8192_hint_page + pages <= HZ5_SEG_PAGES) {
        hint_page = hz5_p1_align_page_up(seg->run16_a8192_hint_page, align_pages);
        first_page = hint_page;
    }

    for (uint32_t pass = 0; pass < (use_run16_hint ? 2u : 1u); ++pass) {
        uint32_t start = (pass == 0) ? first_page : HZ5_FIRST_DATA_PAGE;
        uint32_t end = (pass == 0 || hint_page == 0) ? HZ5_SEG_PAGES : hint_page;
        for (uint32_t page = start; page + pages <= end; ++page) {
            hz5_stats_inc_pages(HZ5_STAT_SEGMENT_ALLOC_SCAN_STEP, pages);
            if ((page % align_pages) != 0) {
                continue;
            }
            if (!hz5_p1_run_is_free(seg, page, pages)) {
                continue;
            }

            for (uint32_t i = 0; i < pages; ++i) {
                uint32_t p = page + i;
                hz5_p1_bit_clear(seg, p);
                seg->page[p].kind = (i == 0) ? HZ5_PAGE_RUN_START : HZ5_PAGE_RUN_CONT;
                seg->page[p].sc = (uint16_t)pages;
                seg->page[p].run_pages = (uint16_t)pages;
                seg->page[p].run_start_page = (uint16_t)page;
                seg->page[p].align_log2 = align_log2;
                seg->page[p].decommit_state = HZ5_COMMITTED;
                seg->page[p].owner = owner;
                atomic_store_explicit(&seg->page[p].remote_head,
                                      NULL,
                                      memory_order_relaxed);
                atomic_store_explicit(&seg->page[p].remote_count_hint,
                                      0,
                                      memory_order_relaxed);
            }
            seg->live_pages += pages;
            if (use_run16_hint) {
                uint32_t next = page + pages;
                seg->run16_a8192_hint_page =
                    next + pages <= HZ5_SEG_PAGES ? next : HZ5_FIRST_DATA_PAGE;
            }
            hz5_p1_unlock();
            return (void*)((uintptr_t)seg + ((uintptr_t)page * HZ5_PAGE_SIZE));
        }
    }
#endif
    hz5_p1_unlock();
    return NULL;
}

void* hz5_p1_segment_alloc_any_run_aligned(uint32_t pages,
                                           uint32_t align_pages,
                                           uint8_t align_log2,
                                           Hz5OwnerToken owner) {
    if (!hz5_p1_segment_get()) {
        return NULL;
    }

    uint32_t count = hz5_p1_segment_count();
    for (uint32_t i = 0; i < count; ++i) {
        Hz5Seg* seg = hz5_p1_segment_at(i);
#if !HZ5_P11_SPEED_CORE
        if (hz5_p1_segment_is_retired(seg)) {
            continue;
        }
#endif
        void* ptr = hz5_p1_segment_alloc_run_aligned(seg,
                                                     pages,
                                                     align_pages,
                                                     align_log2,
                                                     owner);
        if (ptr) {
            return ptr;
        }
    }

    Hz5Seg* seg = hz5_p1_segment_append();
    if (!seg) {
        return NULL;
    }
    return hz5_p1_segment_alloc_run_aligned(seg,
                                            pages,
                                            align_pages,
                                            align_log2,
                                            owner);
}

void hz5_p1_segment_free_run(Hz5Seg* seg, uint32_t page_idx) {
    if (!seg || seg->magic != HZ5_SEG_MAGIC || page_idx >= HZ5_SEG_PAGES) {
        return;
    }

    hz5_p1_lock();
    Hz5PageMeta* meta = &seg->page[page_idx];
    if (meta->kind != HZ5_PAGE_RUN_START || meta->run_pages == 0) {
        hz5_p1_unlock();
        return;
    }

    uint32_t pages = meta->run_pages;
#if !HZ5_P11_SPEED_CORE
    uint8_t align_log2 = meta->align_log2;
#endif
    void* remote = atomic_load_explicit(&meta->remote_head, memory_order_acquire);
    if (remote) {
        hz5_p1_unlock();
        return;
    }
    for (uint32_t i = 0; i < pages && page_idx + i < HZ5_SEG_PAGES; ++i) {
        uint32_t p = page_idx + i;
        memset(&seg->page[p], 0, sizeof(seg->page[p]));
        seg->page[p].kind = HZ5_PAGE_FREE;
        seg->page[p].decommit_state = HZ5_COMMITTED;
        hz5_p1_bit_set(seg, p);
    }
    uint32_t old_live_pages = seg->live_pages;
    if (old_live_pages >= pages) {
        seg->live_pages = old_live_pages - pages;
    } else {
        seg->live_pages = 0;
    }
    if (old_live_pages != 0 && seg->live_pages == 0) {
        hz5_stats_inc(HZ5_STAT_P14_EMPTY_TRANSITION);
        hz5_stats_add(HZ5_STAT_P14_EMPTY_TRANSITION_BYTES, HZ5_SEG_SIZE);
    }
#if !HZ5_P11_SPEED_CORE
    if (pages == 16u && align_log2 == 13u &&
        (seg->run16_a8192_hint_page < HZ5_FIRST_DATA_PAGE ||
         page_idx < seg->run16_a8192_hint_page)) {
        seg->run16_a8192_hint_page = page_idx;
    }
#endif
    hz5_p1_unlock();
}

Hz5Seg* hz5_p1_seg_from_ptr(void* ptr) {
    if (!ptr) {
        return NULL;
    }
#if HZ5_P14_LOCKED_LOOKUP_RELEASE
    hz5_stats_inc(HZ5_STAT_P14_LOCKED_LOOKUP_CALL);
#endif
    uintptr_t raw = (uintptr_t)ptr;
    uintptr_t min_base = atomic_load_explicit(&g_hz5_p1_seg_min_base,
                                              memory_order_acquire);
    uintptr_t max_end = atomic_load_explicit(&g_hz5_p1_seg_max_end,
                                             memory_order_acquire);
    if (min_base == 0 || raw < min_base || raw >= max_end) {
#if HZ5_P14_LOCKED_LOOKUP_RELEASE
        hz5_stats_inc(HZ5_STAT_P14_LOCKED_LOOKUP_MISS_RANGE);
#endif
        return NULL;
    }

#if HZ5_P14_LOCKED_LOOKUP_RELEASE
    hz5_p1_lock();
#endif
    uint32_t count = atomic_load_explicit(&g_hz5_p1_seg_count,
                                          memory_order_acquire);
    for (uint32_t i = 0; i < count; ++i) {
        Hz5Seg* seg = atomic_load_explicit(&g_hz5_p1_segs[i],
                                           memory_order_acquire);
        if (seg && raw >= (uintptr_t)seg && raw < (uintptr_t)seg + HZ5_SEG_SIZE) {
            if (seg->magic != HZ5_SEG_MAGIC || seg->version != HZ5_SEG_VERSION) {
#if HZ5_P14_LOCKED_LOOKUP_RELEASE
                hz5_stats_inc(HZ5_STAT_P14_LOCKED_LOOKUP_BAD_MAGIC);
                hz5_p1_unlock();
#endif
                return NULL;
            }
#if HZ5_P14_LOCKED_LOOKUP_RELEASE
            hz5_stats_inc(HZ5_STAT_P14_LOCKED_LOOKUP_HIT);
            hz5_p1_unlock();
#endif
            return seg;
        }
    }
#if HZ5_P14_LOCKED_LOOKUP_RELEASE
    hz5_stats_inc(HZ5_STAT_P14_LOCKED_LOOKUP_MISS_TABLE);
    hz5_p1_unlock();
#endif
    return NULL;
}

uint32_t hz5_p1_page_index(Hz5Seg* seg, void* ptr) {
    return (uint32_t)(((uintptr_t)ptr - (uintptr_t)seg) / HZ5_PAGE_SIZE);
}
