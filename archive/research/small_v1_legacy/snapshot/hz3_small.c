#include "hz3_small.h"

#include "hz3_arena.h"
#include "hz3_central_bin.h"
#include "hz3_list_util.h"
#include "hz3_segment.h"
#include "hz3_tag.h"
#include "hz3_tcache.h"

#include <pthread.h>
#include <stdint.h>

#if !HZ3_SMALL_V1_ENABLE
void* hz3_small_alloc(size_t size) {
    (void)size;
    return NULL;
}

void hz3_small_free(void* ptr, int sc) {
    (void)ptr;
    (void)sc;
}

void hz3_small_bin_flush(int sc, Hz3Bin* bin) {
    (void)sc;
    (void)bin;
}

#if HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_V1_ENABLE
void hz3_small_free_by_tag(void* ptr, int sc, int owner) {
    (void)ptr;
    (void)sc;
    (void)owner;
}
#endif

#else  // HZ3_SMALL_V1_ENABLE

#define HZ3_SMALL_REFILL_BATCH 32

#if HZ3_SMALL_V1_OBSERVE
#include <stdatomic.h>
#include <stdio.h>

static _Atomic uint64_t g_v1_alloc_calls = 0;
static _Atomic uint64_t g_v1_free_calls = 0;
static _Atomic uint64_t g_v1_flush_calls = 0;
static _Atomic uint64_t g_v1_by_tag_calls = 0;
static _Atomic int g_v1_atexit_registered = 0;

static void hz3_small_v1_observe_dump(void) {
    uint64_t alloc_calls = atomic_load_explicit(&g_v1_alloc_calls, memory_order_relaxed);
    uint64_t free_calls = atomic_load_explicit(&g_v1_free_calls, memory_order_relaxed);
    uint64_t flush_calls = atomic_load_explicit(&g_v1_flush_calls, memory_order_relaxed);
    uint64_t by_tag_calls = atomic_load_explicit(&g_v1_by_tag_calls, memory_order_relaxed);
    if (alloc_calls || free_calls || flush_calls || by_tag_calls) {
        fprintf(stderr,
                "[HZ3_V1_OBS] alloc=%lu free=%lu flush=%lu by_tag=%lu\n",
                (unsigned long)alloc_calls,
                (unsigned long)free_calls,
                (unsigned long)flush_calls,
                (unsigned long)by_tag_calls);
    }
}

static inline void hz3_small_v1_observe_hit(_Atomic uint64_t* counter) {
    atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    if (atomic_exchange_explicit(&g_v1_atexit_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_small_v1_observe_dump);
    }
}
#endif

static pthread_once_t g_hz3_small_central_once = PTHREAD_ONCE_INIT;
static Hz3CentralBin g_hz3_small_central[HZ3_SMALL_NUM_SC];

static void hz3_small_central_do_init(void) {
    for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
        hz3_central_bin_init(&g_hz3_small_central[sc]);
    }
}

static inline void hz3_small_central_init(void) {
    pthread_once(&g_hz3_small_central_once, hz3_small_central_do_init);
}

static inline void hz3_small_central_push_list(int sc, void* head, void* tail, uint32_t n) {
    hz3_central_bin_push_list(&g_hz3_small_central[sc], head, tail, n);
}

static inline int hz3_small_central_pop_batch(int sc, void** out, int want) {
    return hz3_central_bin_pop_batch(&g_hz3_small_central[sc], out, want);
}

static void* hz3_small_alloc_page(int sc) {
    Hz3SegMeta* meta = t_hz3_cache.current_seg;

    if (!meta || meta->free_pages < 1) {
        meta = hz3_new_segment(t_hz3_cache.my_shard);
        if (!meta) {
            return NULL;
        }
        t_hz3_cache.current_seg = meta;
    }

    int start_page = hz3_segment_alloc_run(meta, 1);
    if (start_page < 0) {
        meta = hz3_new_segment(t_hz3_cache.my_shard);
        if (!meta) {
            return NULL;
        }
        t_hz3_cache.current_seg = meta;
        start_page = hz3_segment_alloc_run(meta, 1);
        if (start_page < 0) {
            return NULL;
        }
    }

    meta->sc_tag[start_page] = hz3_tag_make_small(sc);
    void* page_base = (char*)meta->seg_base + ((size_t)start_page << HZ3_PAGE_SHIFT);

#if (HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_V1_ENABLE) || HZ3_PTAG_DSTBIN_ENABLE
    // This page is in arena (since S12-5A), so page_idx is valid
    uint32_t page_idx;
    if (hz3_arena_page_index_fast(page_base, &page_idx)) {
#if HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_V1_ENABLE
        // S12-5B: Set PageTagMap tag for v1 allocation
        uint16_t tag = hz3_pagetag_encode_v1(sc, t_hz3_cache.my_shard);
        hz3_pagetag_store(page_idx, tag);
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
        // S17: dst/bin direct tag set (page allocation boundary)
        int bin = hz3_bin_index_small(sc);
        if (g_hz3_page_tag32) {
            uint32_t tag32 = hz3_pagetag32_encode(bin, t_hz3_cache.my_shard);
            hz3_pagetag32_store(page_idx, tag32);
        }
#endif
    }
#endif

    return page_base;
}

#if HZ3_TCACHE_SOA_LOCAL
static void hz3_small_fill_binref(Hz3BinRef binref, int sc, void* page_base) {
    size_t obj_size = hz3_small_sc_to_size(sc);
    size_t count = HZ3_PAGE_SIZE / obj_size;
    char* cur = (char*)page_base;

    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    hz3_binref_prepend_list(binref, page_base, cur, (uint32_t)count);
}
#else
static void hz3_small_fill_bin(Hz3Bin* bin, int sc, void* page_base) {
    size_t obj_size = hz3_small_sc_to_size(sc);
    size_t count = HZ3_PAGE_SIZE / obj_size;
    char* cur = (char*)page_base;

    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    hz3_small_bin_prepend_list(bin, page_base, cur, (uint32_t)count);
}
#endif

void* hz3_small_alloc(size_t size) {
#if HZ3_SMALL_V1_OBSERVE
    hz3_small_v1_observe_hit(&g_v1_alloc_calls);
#endif
    int sc = hz3_small_sc_from_size(size);
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return NULL;
    }

    hz3_tcache_ensure_init();
#if HZ3_TCACHE_SOA_LOCAL
    Hz3BinRef binref = hz3_tcache_get_local_binref(hz3_bin_index_small(sc));
#else
    Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
#endif

    void* obj = NULL;
#if HZ3_TCACHE_SOA_LOCAL
    obj = hz3_binref_pop(binref);
#else
    obj = hz3_bin_pop(bin);
#endif
    if (obj) {
        return obj;
    }

    hz3_small_central_init();

    void* batch[HZ3_SMALL_REFILL_BATCH];
    int got = hz3_small_central_pop_batch(sc, batch, HZ3_SMALL_REFILL_BATCH);
    if (got > 0) {
        for (int i = 1; i < got; i++) {
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(binref, batch[i]);
#else
            hz3_bin_push(bin, batch[i]);
#endif
        }
        return batch[0];
    }

    void* page = hz3_small_alloc_page(sc);
    if (!page) {
        return NULL;
    }

#if HZ3_TCACHE_SOA_LOCAL
    hz3_small_fill_binref(binref, sc, page);
    return hz3_binref_pop(binref);
#else
    hz3_small_fill_bin(bin, sc, page);
    return hz3_bin_pop(bin);
#endif
}

void hz3_small_free(void* ptr, int sc) {
#if HZ3_SMALL_V1_OBSERVE
    hz3_small_v1_observe_hit(&g_v1_free_calls);
#endif
    if (!ptr || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

    hz3_tcache_ensure_init();
#if HZ3_TCACHE_SOA_LOCAL
    hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_small(sc)), ptr);
#else
    Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
    hz3_bin_push(bin, ptr);
#endif
}

void hz3_small_bin_flush(int sc, Hz3Bin* bin) {
#if HZ3_SMALL_V1_OBSERVE
    hz3_small_v1_observe_hit(&g_v1_flush_calls);
#endif
#if HZ3_TCACHE_SOA_LOCAL
    (void)bin;
#endif
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }
#if HZ3_TCACHE_SOA_LOCAL
    uint32_t bin_idx = hz3_bin_index_small(sc);
    void* head = t_hz3_cache.local_head[bin_idx];
    if (!head) {
        return;
    }
#else
    if (!bin || hz3_bin_is_empty(bin)
#if !HZ3_BIN_LAZY_COUNT
        || hz3_bin_count_get(bin) == 0
#endif
        ) {
        return;
    }
#endif

    hz3_small_central_init();

#if HZ3_TCACHE_SOA_LOCAL
    void* tail = head;
    uint32_t n = 1;
    void* cur = hz3_obj_get_next(head);
    while (cur) {
        tail = cur;
        n++;
        cur = hz3_obj_get_next(cur);
    }
    t_hz3_cache.local_head[bin_idx] = NULL;
#if !HZ3_BIN_LAZY_COUNT
    t_hz3_cache.local_count[bin_idx] = 0;
#endif
#else
    void* head = hz3_bin_take_all(bin);
    void* tail;
    uint32_t n;
    hz3_list_find_tail(head, &tail, &n);
#endif

    hz3_small_central_push_list(sc, head, tail, n);
}

#if HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_V1_ENABLE
// S12-5C: Free by tag (unified dispatch path for v1)
// Similar to hz3_small_v2_free_by_tag() but for v1 allocations
void hz3_small_free_by_tag(void* ptr, int sc, int owner) {
#if HZ3_SMALL_V1_OBSERVE
    hz3_small_v1_observe_hit(&g_v1_by_tag_calls);
#endif
    if (!ptr || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

    hz3_tcache_ensure_init();

    if (__builtin_expect((uint8_t)owner == t_hz3_cache.my_shard, 1)) {
        // Local: push to TLS bin (fast path)
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_small(sc)), ptr);
#else
        Hz3Bin* bin = hz3_tcache_get_small_bin(sc);
        hz3_bin_push(bin, ptr);
#endif
        return;
    }

    // Remote: push to central (owner-agnostic for v1, unlike v2's per-owner bins)
    hz3_small_central_init();
    hz3_small_central_push_list(sc, ptr, ptr, 1);
}
#endif

#endif  // HZ3_SMALL_V1_ENABLE
