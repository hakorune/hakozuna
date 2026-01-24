// hz4_tcache.c - HZ4 TCache Box (small-only allocator)
#include <stdint.h>
#include <stdlib.h>

#include "hz4_config.h"
#include "hz4_types.h"
#include "hz4_page.h"
#include "hz4_seg.h"
#include "hz4_sizeclass.h"
#include "hz4_segment.h"
#include "hz4_tls_init.h"
#include "hz4_mid.h"
#include "hz4_large.h"
#include "hz4_tcache.h"
#if HZ4_PAGE_TAG_TABLE
#include "hz4_pagetag.h"
#endif

// Boundary boxes (single TU ownership)
#include "hz4_segq.inc"
#include "hz4_remote_flush.inc"
#include "hz4_collect.inc"

#if !HZ4_TLS_MERGE
typedef struct hz4_tcache_bin {
    void* head;
    uint32_t count;
} hz4_tcache_bin_t;

typedef struct hz4_alloc_tls {
    hz4_tcache_bin_t bins[HZ4_SC_MAX];
    hz4_seg_t* cur_seg;
    uint32_t next_page_idx;
    uint8_t owner;
    uint8_t inited;
} hz4_alloc_tls_t;

static __thread hz4_alloc_tls_t g_hz4_alloc_tls;
#endif

static inline void hz4_tcache_push(hz4_tcache_bin_t* bin, void* obj) {
    hz4_obj_set_next(obj, bin->head);
    bin->head = obj;
#if HZ4_TCACHE_COUNT
    bin->count++;
#endif
}

static inline void* hz4_tcache_pop(hz4_tcache_bin_t* bin) {
    void* obj = bin->head;
    if (obj) {
        bin->head = hz4_obj_get_next(obj);
#if HZ4_FAILFAST
        hz4_obj_set_next(obj, NULL);  // P3.5: debug only
#endif
#if HZ4_TCACHE_COUNT
        if (bin->count > 0) {
            bin->count--;
        }
#endif
    }
    return obj;
}

// P4.1: Splice a list directly into tcache bin
#if HZ4_COLLECT_LIST
static inline void hz4_tcache_splice(hz4_tcache_bin_t* bin,
                                     void* head, void* tail, uint32_t n) {
    if (!head) return;
    hz4_obj_set_next(tail, bin->head);
    bin->head = head;
#if HZ4_TCACHE_COUNT
    bin->count += n;
#endif
}
#endif

#if !HZ4_TLS_MERGE
static hz4_alloc_tls_t* hz4_alloc_tls_get(hz4_tls_t* tls) {
    // HZ4_TLS_SINGLE: init check を cold path へ (likely hint)
    if (__builtin_expect(!g_hz4_alloc_tls.inited, 0)) {
        for (uint32_t i = 0; i < HZ4_SC_MAX; i++) {
            g_hz4_alloc_tls.bins[i].head = NULL;
            g_hz4_alloc_tls.bins[i].count = 0;
        }
        g_hz4_alloc_tls.cur_seg = NULL;
        g_hz4_alloc_tls.next_page_idx = HZ4_PAGE_IDX_MIN;
        g_hz4_alloc_tls.owner = (uint8_t)hz4_owner_shard(tls->tid);
        g_hz4_alloc_tls.inited = 1;
    }
    return &g_hz4_alloc_tls;
}
#endif

#if HZ4_TLS_MERGE
static hz4_page_t* hz4_alloc_page(hz4_tls_t* tls, uint8_t sc) {
    if (!tls->cur_seg || tls->next_page_idx >= HZ4_PAGES_PER_SEG) {
        tls->cur_seg = hz4_seg_create(tls->owner);
        if (!tls->cur_seg) {
            abort();
        }
        tls->next_page_idx = HZ4_PAGE_IDX_MIN;
    }

    hz4_page_t* page = hz4_page_from_seg(tls->cur_seg, tls->next_page_idx++);

    page->free = NULL;
    page->local_free = NULL;
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        atomic_store_explicit(&page->remote_head[s], NULL, memory_order_relaxed);
    }
#if HZ4_REMOTE_MASK
    atomic_store_explicit(&page->remote_mask, 0, memory_order_relaxed);
#endif
    atomic_store_explicit(&page->queued, 0, memory_order_relaxed);
    page->qnext = NULL;

    page->magic = HZ4_PAGE_MAGIC;
    page->owner_tid = tls->tid;
    page->sc = sc;
    page->used_count = 0;
    page->capacity = 0;

#if HZ4_PAGE_TAG_TABLE
    // Register page tag for fast lookup
    uint32_t page_idx;
    if (hz4_pagetag_page_idx(page, &page_idx)) {
        uint32_t tag = hz4_pagetag_encode(HZ4_TAG_KIND_SMALL, sc, tls->tid);
        hz4_pagetag_store(page_idx, tag);
    }
#endif

    return page;
}
#else
static hz4_page_t* hz4_alloc_page(hz4_tls_t* tls, hz4_alloc_tls_t* atls, uint8_t sc) {
    if (!atls->cur_seg || atls->next_page_idx >= HZ4_PAGES_PER_SEG) {
        atls->cur_seg = hz4_seg_create(atls->owner);
        if (!atls->cur_seg) {
            abort();
        }
        atls->next_page_idx = HZ4_PAGE_IDX_MIN;
    }

    hz4_page_t* page = hz4_page_from_seg(atls->cur_seg, atls->next_page_idx++);

    page->free = NULL;
    page->local_free = NULL;
    for (uint32_t s = 0; s < HZ4_REMOTE_SHARDS; s++) {
        atomic_store_explicit(&page->remote_head[s], NULL, memory_order_relaxed);
    }
#if HZ4_REMOTE_MASK
    atomic_store_explicit(&page->remote_mask, 0, memory_order_relaxed);
#endif
    atomic_store_explicit(&page->queued, 0, memory_order_relaxed);
    page->qnext = NULL;

    page->magic = HZ4_PAGE_MAGIC;
    page->owner_tid = tls->tid;
    page->sc = sc;
    page->used_count = 0;
    page->capacity = 0;

#if HZ4_PAGE_TAG_TABLE
    // Register page tag for fast lookup
    uint32_t page_idx;
    if (hz4_pagetag_page_idx(page, &page_idx)) {
        uint32_t tag = hz4_pagetag_encode(HZ4_TAG_KIND_SMALL, sc, tls->tid);
        hz4_pagetag_store(page_idx, tag);
    }
#endif

    return page;
}
#endif

static void hz4_populate_page(hz4_page_t* page, hz4_tcache_bin_t* bin, size_t obj_size) {
    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
    uintptr_t end = (uintptr_t)page + HZ4_PAGE_SIZE;

    uint32_t count = 0;
    for (uintptr_t p = start; p + obj_size <= end; p += obj_size) {
        void* obj = (void*)p;
        hz4_tcache_push(bin, obj);
        count++;
    }

    page->capacity = count;
}

// ============================================================================
// P3.4: Inbox Splice Refill (eliminate double traversal)
// ============================================================================
#if HZ4_INBOX_SPLICE && HZ4_REMOTE_INBOX
static inline void* hz4_refill_from_inbox_splice(hz4_tls_t* tls,
                                                 hz4_tcache_bin_t* bin,
                                                 uint8_t sc) {
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);

    // Try stash first, then inbox
    void* list = tls->inbox_stash[sc];
    if (!list) list = hz4_inbox_pop_all(owner, sc);  // P3.1 で空チェック済み
    if (!list) return NULL;

    // Walk list to find prefix (up to budget)
    void* head = list;
    void* cur = list;
    void* tail = NULL;
    uint32_t n = 0;
    while (cur && n < HZ4_COLLECT_OBJ_BUDGET) {
        tail = cur;
        cur = hz4_obj_get_next(cur);
        n++;
    }

    // Safety check (念のため - budget=0 等の保険)
    if (!tail) return NULL;

    // Store remainder in stash
    tls->inbox_stash[sc] = cur;

    // Splice prefix to bin
    hz4_obj_set_next(tail, bin->head);
    bin->head = head;
#if HZ4_TCACHE_COUNT
    bin->count += n;
#endif

    return hz4_tcache_pop(bin);
}
#endif

#if HZ4_TLS_MERGE
static void* hz4_refill(hz4_tls_t* tls, uint8_t sc) {
    hz4_tcache_bin_t* bin = &tls->bins[sc];

#if HZ4_INBOX_SPLICE && HZ4_REMOTE_INBOX && !HZ4_COLLECT_LIST
    // P3.4: splice fast path (1回走査) - P4.1 では使用しない
    void* obj = hz4_refill_from_inbox_splice(tls, bin, sc);
    if (obj) return obj;
#endif

#if HZ4_COLLECT_LIST
    // P4.1: list mode (out[] 配列を排除)
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = hz4_collect_list(tls, sc, &head, &tail);
    if (got) {
        hz4_tcache_splice(bin, head, tail, got);
        return hz4_tcache_pop(bin);
    }
#else
    void* out[HZ4_COLLECT_OBJ_BUDGET];
    uint32_t got = hz4_collect_default(tls, sc, out);
    if (got) {
        for (uint32_t i = 0; i < got; i++) {
            hz4_tcache_push(bin, out[i]);
        }
        return hz4_tcache_pop(bin);
    }
#endif

    hz4_page_t* page = hz4_alloc_page(tls, sc);
    if (!page) {
        abort();
    }

    size_t obj_size = hz4_sc_to_size(sc);
    hz4_populate_page(page, bin, obj_size);

    return hz4_tcache_pop(bin);
}
#else
static void* hz4_refill(hz4_tls_t* tls, hz4_alloc_tls_t* atls, uint8_t sc) {
    hz4_tcache_bin_t* bin = &atls->bins[sc];

#if HZ4_INBOX_SPLICE && HZ4_REMOTE_INBOX && !HZ4_COLLECT_LIST
    // P3.4: splice fast path (1回走査) - P4.1 では使用しない
    void* obj = hz4_refill_from_inbox_splice(tls, bin, sc);
    if (obj) return obj;
#endif

#if HZ4_COLLECT_LIST
    // P4.1: list mode (out[] 配列を排除)
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = hz4_collect_list(tls, sc, &head, &tail);
    if (got) {
        hz4_tcache_splice(bin, head, tail, got);
        return hz4_tcache_pop(bin);
    }
#else
    void* out[HZ4_COLLECT_OBJ_BUDGET];
    uint32_t got = hz4_collect_default(tls, sc, out);
    if (got) {
        for (uint32_t i = 0; i < got; i++) {
            hz4_tcache_push(bin, out[i]);
        }
        return hz4_tcache_pop(bin);
    }
#endif

    hz4_page_t* page = hz4_alloc_page(tls, atls, sc);
    if (!page) {
        abort();
    }

    size_t obj_size = hz4_sc_to_size(sc);
    hz4_populate_page(page, bin, obj_size);

    return hz4_tcache_pop(bin);
}
#endif

static void* hz4_small_malloc(size_t size) {
    hz4_tls_t* tls = hz4_tls_get();

    uint8_t sc = hz4_size_to_sc(size);
    if (sc == HZ4_SC_INVALID) {
        HZ4_FAIL("hz4_small_malloc: size out of range");
        abort();
    }

#if HZ4_TLS_MERGE
    hz4_tcache_bin_t* bin = &tls->bins[sc];
    void* obj = hz4_tcache_pop(bin);
    if (!obj) {
        obj = hz4_refill(tls, sc);
    }
#else
    hz4_alloc_tls_t* atls = hz4_alloc_tls_get(tls);
    hz4_tcache_bin_t* bin = &atls->bins[sc];
    void* obj = hz4_tcache_pop(bin);
    if (!obj) {
        obj = hz4_refill(tls, atls, sc);
    }
#endif
    return obj;
}

// page を渡す版（二重計算削減）
static void hz4_small_free_with_page(void* ptr, hz4_page_t* page) {
#if HZ4_FAILFAST || HZ4_LOCAL_MAGIC_CHECK
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("hz4_small_free_with_page: invalid page");
        abort();
    }
#endif

    hz4_tls_t* tls = hz4_tls_get();

    uint8_t sc = page->sc;
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("hz4_small_free_with_page: invalid size class");
        abort();
    }
#endif

    if (page->owner_tid == tls->tid) {
#if HZ4_TLS_MERGE
        hz4_tcache_push(&tls->bins[sc], ptr);
#else
        hz4_alloc_tls_t* atls = hz4_alloc_tls_get(tls);
        hz4_tcache_push(&atls->bins[sc], ptr);
#endif
    } else {
        hz4_remote_free(tls, ptr);
    }
}

__attribute__((unused))
static void hz4_small_free(void* ptr) {
    hz4_page_t* page = hz4_page_from_ptr(ptr);
    hz4_small_free_with_page(ptr, page);
}

void* hz4_malloc(size_t size) {
    if (size <= HZ4_SIZE_MAX) {
        return hz4_small_malloc(size);
    }
    if (size <= hz4_mid_max_size()) {
        return hz4_mid_malloc(size);
    }
    return hz4_large_malloc(size);
}

void hz4_free(void* ptr) {
    if (!ptr) {
        return;
    }

#if HZ4_PAGE_TAG_TABLE
    // Fast path: single lookup via page tag table
    uint32_t tag;
    if (hz4_pagetag_lookup(ptr, &tag)) {
        uint8_t kind = hz4_pagetag_kind(tag);

        if (kind == HZ4_TAG_KIND_SMALL) {
            uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
            hz4_page_t* page = (hz4_page_t*)base;
            hz4_small_free_with_page(ptr, page);
            return;
        }
        if (kind == HZ4_TAG_KIND_MID) {
            hz4_mid_free(ptr);
            return;
        }
        // kind=0 (unknown) → fallback to legacy path
    }
#endif

    // Legacy path (PAGE_TAG_TABLE=0 / lookup miss / Large)
    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
    uint32_t head_magic = *(uint32_t*)base;

    if (head_magic == HZ4_MID_MAGIC) {
        hz4_mid_free(ptr);
        return;
    }
    if (head_magic == HZ4_LARGE_MAGIC) {
        hz4_large_free(ptr);
        return;
    }

    // base はすでに計算済みなので page として再利用（二重計算削減）
    hz4_page_t* page = (hz4_page_t*)base;
    if (hz4_page_valid(page)) {
        hz4_small_free_with_page(ptr, page);  // page を渡す
        return;
    }

    HZ4_FAIL("hz4_free: unknown pointer");
    abort();
}

size_t hz4_small_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    hz4_page_t* page = hz4_page_from_ptr(ptr);
    if (!hz4_page_valid(page)) {
        HZ4_FAIL("hz4_small_usable_size: invalid page");
        abort();
    }
    return hz4_sc_to_size(page->sc);
}
