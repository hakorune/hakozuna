#include "hz5_internal.h"

#include <stddef.h>
#include <stdint.h>

static _Thread_local Hz5RunCache t_hz5_run_cache;

static const Hz5RunClassPolicy g_hz5_default_policy = {
    0u,
    0u,
    HZ5_RUNBAND_UNKNOWN,
    HZ5_RUN_CACHE_CAP,
    0u,
    HZ5_REMOTE_BUF_CAP,
    0u,
    0u,
};

static const Hz5RunClassPolicy g_hz5_policy_4k_a8192 = {
    1u,
    13u,
    HZ5_RUNBAND_SMALL_OVA,
    HZ5_RUN_CACHE_CAP,
    0u,
    HZ5_REMOTE_BUF_CAP,
    0u,
    0u,
};

#if HZ5_P22_SPLIT_1P_SIZE_CLASS
static const Hz5RunClassPolicy g_hz5_policy_2k_a8192 = {
    1u,
    13u,
    HZ5_RUNBAND_SMALL_OVA,
    HZ5_RUN_CACHE_CAP,
    0u,
    HZ5_REMOTE_BUF_CAP,
    0u,
    0u,
};
#endif

static const Hz5RunClassPolicy g_hz5_policy_8k_a8192 = {
    2u,
    13u,
    HZ5_RUNBAND_SMALL_OVA,
    HZ5_RUN_CACHE_CAP,
    0u,
    HZ5_REMOTE_BUF_CAP,
    0u,
    0u,
};

static const Hz5RunClassPolicy g_hz5_policy_64k_a8192 = {
    16u,
    13u,
    HZ5_RUNBAND_LARGE16,
    HZ5_P12_RUN16_OWNER_CACHE_CAP,
    0u,
    HZ5_P12_RUN16_REMOTE_FLUSH_CAP,
    0u,
    0u,
};

uint16_t hz5_run_sc_for_size(size_t size, uint32_t pages) {
#if HZ5_P22_SPLIT_1P_SIZE_CLASS
    if (pages <= 1u) {
        return size <= 2048u ? 2048u : 4096u;
    }
#endif
    return (uint16_t)pages;
}

uint32_t hz5_run_class_index(uint32_t pages, uint8_t align_log2, uint16_t sc) {
#if HZ5_P22_SPLIT_1P_SIZE_CLASS
    if (pages <= 1u && align_log2 == 13u && sc <= 2048u) {
        return 0u;
    }
#endif
    uint32_t align_bucket = align_log2 > 12u ? (uint32_t)(align_log2 - 12u) : 0u;
    if (align_bucket > 3u) {
        align_bucket = 3u;
    }

    uint32_t page_bucket;
    if (pages <= 1u) {
        page_bucket = 0u;
    } else if (pages <= 2u) {
        page_bucket = 1u;
    } else if (pages <= 16u) {
        page_bucket = 2u;
    } else {
        page_bucket = 3u;
    }
    return (page_bucket * 4u) + align_bucket;
}

const Hz5RunClassPolicy* hz5_run_policy_for(uint32_t pages,
                                            uint8_t align_log2,
                                            uint16_t sc) {
    if (align_log2 == 13u) {
        if (pages == 1u) {
#if HZ5_P22_SPLIT_1P_SIZE_CLASS
            if (sc <= 2048u) {
                return &g_hz5_policy_2k_a8192;
            }
#else
            (void)sc;
#endif
            return &g_hz5_policy_4k_a8192;
        }
        if (pages == 2u) {
            return &g_hz5_policy_8k_a8192;
        }
        if (pages == 16u) {
            return &g_hz5_policy_64k_a8192;
        }
    }
    return &g_hz5_default_policy;
}

void* hz5_tcache_pop(uint32_t pages, uint8_t align_log2, uint16_t sc) {
    uint32_t idx = hz5_run_class_index(pages, align_log2, sc);
    if (idx >= HZ5_RUN_CACHE_CLASS_COUNT) {
        return NULL;
    }
    Hz5RunCacheClass* cls = &t_hz5_run_cache.cls[idx];
    if (cls->count == 0) {
        return NULL;
    }
    void* ptr = cls->slots[--cls->count];
    Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
#if !HZ5_P11_SPEED_CORE
    if (seg) {
        atomic_fetch_sub_explicit(&seg->tcache_refs, 1u, memory_order_relaxed);
    }
#else
    (void)seg;
#endif
    return ptr;
}

int hz5_tcache_push(void* ptr) {
    Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
    if (!seg) {
        return 0;
    }
    uint32_t page = hz5_p1_page_index(seg, ptr);
    if (page >= HZ5_SEG_PAGES) {
        return 0;
    }
    Hz5PageMeta* meta = &seg->page[page];
    if (meta->kind != HZ5_PAGE_RUN_START || meta->run_pages == 0) {
        return 0;
    }

    uint32_t idx = hz5_run_class_index(meta->run_pages, meta->align_log2,
                                       meta->sc);
    if (idx >= HZ5_RUN_CACHE_CLASS_COUNT) {
        return 0;
    }
    Hz5RunCacheClass* cls = &t_hz5_run_cache.cls[idx];
    const Hz5RunClassPolicy* policy =
        hz5_run_policy_for(meta->run_pages, meta->align_log2, meta->sc);
    uint16_t owner_cache_cap = policy->owner_cache_cap;
    if (owner_cache_cap > HZ5_RUN_CACHE_CAP) {
        owner_cache_cap = HZ5_RUN_CACHE_CAP;
    }
    if (cls->count >= owner_cache_cap) {
        return 0;
    }
    cls->slots[cls->count++] = ptr;
#if !HZ5_P11_SPEED_CORE
    atomic_fetch_add_explicit(&seg->tcache_refs, 1u, memory_order_relaxed);
#endif
    return 1;
}

size_t hz5_tcache_release_all(void) {
    size_t released = 0;
    for (uint32_t idx = 0; idx < HZ5_RUN_CACHE_CLASS_COUNT; ++idx) {
        Hz5RunCacheClass* cls = &t_hz5_run_cache.cls[idx];
        while (cls->count != 0) {
            void* ptr = cls->slots[--cls->count];
            Hz5Seg* seg = hz5_p1_seg_from_ptr(ptr);
            if (!seg) {
                continue;
            }
            uint32_t page = hz5_p1_page_index(seg, ptr);
            if (page >= HZ5_SEG_PAGES) {
                continue;
            }
            uint32_t pages = seg->page[page].run_pages;
#if !HZ5_P11_SPEED_CORE
            atomic_fetch_sub_explicit(&seg->tcache_refs, 1u, memory_order_relaxed);
#endif
            hz5_p1_segment_free_run(seg, page);
            hz5_stats_inc_pages(HZ5_STAT_TCACHE_DESTRUCTOR_RELEASE, pages);
            (void)pages;
            ++released;
        }
    }
    return released;
}
