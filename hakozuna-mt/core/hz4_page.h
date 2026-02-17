// hz4_page.h - HZ4 Page Header
#ifndef HZ4_PAGE_H
#define HZ4_PAGE_H

#include "hz4_config.h"
#include "hz4_types.h"

// Forward declaration for page meta (defined in hz4_seg.h)
#if HZ4_PAGE_META_SEPARATE
typedef struct hz4_page_meta hz4_page_meta_t;
#endif

// ============================================================================
// Page Header
// ============================================================================
// Box Theory: page は small allocation の単位
// - free / local_free は owner のみがアクセス（page 側に残す、decommit 後 rebuild）
// - remote_head[] は MPSC (multiple producer, single consumer)
//   → meta 分離時は meta 側へ移動（decommit 中もアクセス可能）
struct hz4_page {
#if !HZ4_PAGE_META_SEPARATE
    // ---- Owner-only fields ----
    void* free;           // current free list head (owner only)
    void* local_free;     // local free list (owner only, not yet merged)

    // ---- MPSC remote free lists ----
    // Sharding reduces contention (default: 4 shards)
    _Atomic(void*) remote_head[HZ4_REMOTE_SHARDS];
#if HZ4_REMOTE_MASK
    _Atomic(uint64_t) remote_mask;  // shard hint (bit per shard)
#endif

    // ---- Metadata ----
    uint32_t magic;       // HZ4_PAGE_MAGIC
    uint16_t owner_tid;   // owner thread ID
    uint16_t sc;          // size class

    // ---- Statistics (optional) ----
    uint32_t used_count;  // allocated objects count
    uint32_t capacity;    // max objects in page

    // ---- Page Queue (MPSC) ----
    _Atomic(uint8_t) queued;  // 0=idle, 1=queued
    hz4_page_t* qnext;        // intrusive link (queued only)

#else
    // ---- Thin structure for decommitted pages ----
    // free/local_free は page 側に残す（decommit 後に rebuild で復元）
    void* free;           // current free list head (owner only)
    void* local_free;     // local free list (owner only, not yet merged)

    // ---- Validation ----
    uint32_t magic;       // HZ4_PAGE_MAGIC
    uint32_t _reserved;   // padding/future use
#endif
};

// ============================================================================
// Page validation
// ============================================================================
static inline bool hz4_page_valid(hz4_page_t* page) {
#if HZ4_FAILFAST || HZ4_LOCAL_MAGIC_CHECK
    return page && page->magic == HZ4_PAGE_MAGIC;
#else
    // release 時のみ magic check 省略
    return page != NULL;
#endif
}

// ============================================================================
// Remote head operations
// ============================================================================
// Note: When HZ4_PAGE_META_SEPARATE is enabled, these functions are
//       implemented in hz4_seg.h (included at end) to access hz4_page_meta_t.

#if !HZ4_PAGE_META_SEPARATE
// PTAG32-lite accessors (non-meta)
static inline uint32_t hz4_page_tag_load(hz4_page_t* page) {
    return hz4_page_tag_load_from_fields(&page->owner_tid);
}

// Drain: atomic_exchange で全取得 (owner のみ)
static inline void* hz4_page_drain_remote(hz4_page_t* page, uint32_t shard) {
    return atomic_exchange_explicit(&page->remote_head[shard], NULL,
                                    memory_order_acquire);
}

#if HZ4_REMOTE_MASK
static inline void hz4_page_mark_remote(hz4_page_t* page, uint32_t shard) {
    atomic_fetch_or_explicit(&page->remote_mask, (1ULL << shard), memory_order_release);
}

static inline void hz4_page_clear_remote_hint(hz4_page_t* page, uint32_t shard) {
    atomic_fetch_and_explicit(&page->remote_mask, ~(1ULL << shard), memory_order_acq_rel);
}

static inline uint64_t hz4_page_remote_mask(hz4_page_t* page) {
    return atomic_load_explicit(&page->remote_mask, memory_order_acquire);
}
#else
static inline void hz4_page_mark_remote(hz4_page_t* page, uint32_t shard) {
    (void)page;
    (void)shard;
}
static inline void hz4_page_clear_remote_hint(hz4_page_t* page, uint32_t shard) {
    (void)page;
    (void)shard;
}
static inline uint64_t hz4_page_remote_mask(hz4_page_t* page) {
    (void)page;
    return ~0ULL;
}
#endif

// Push single object to remote_head (any thread)
static inline void hz4_page_push_remote_one_shard(hz4_page_t* page, void* obj, uint32_t shard) {
    _Atomic(void*)* slot = &page->remote_head[shard];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(obj, old);
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, obj,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
    hz4_page_mark_remote(page, shard);
}

static inline void hz4_page_push_remote_one(hz4_page_t* page, void* obj) {
    hz4_page_push_remote_one_shard(page, obj, hz4_select_shard(page));
}

static inline void hz4_page_push_remote_one_tid(hz4_page_t* page, void* obj, uint16_t tid) {
    hz4_page_push_remote_one_shard(page, obj, hz4_select_shard_salted(page, tid));
}

// Push list to remote_head (any thread)
// 重要: 毎 iteration で tail.next を再設定 (Lane16 バグ修正)
static inline void hz4_page_push_remote_list(hz4_page_t* page,
                                             void* head, void* tail,
                                             uint32_t n) {
    (void)n;  // n は契約のみ (while(next) 禁止)
    _Atomic(void*)* slot = &page->remote_head[hz4_select_shard(page)];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(tail, old);  // 毎回再設定 (CAS retry 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
    hz4_page_mark_remote(page, hz4_select_shard(page));
}

static inline void hz4_page_push_remote_list_tid(hz4_page_t* page,
                                                 void* head, void* tail,
                                                 uint32_t n, uint16_t tid) {
    (void)n;  // n は契約のみ (while(next) 禁止)
    _Atomic(void*)* slot = &page->remote_head[hz4_select_shard_salted(page, tid)];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(tail, old);  // 毎回再設定 (CAS retry 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
    hz4_page_mark_remote(page, hz4_select_shard_salted(page, tid));
}
#endif // !HZ4_PAGE_META_SEPARATE

// When HZ4_PAGE_META_SEPARATE is enabled, source files must also include hz4_seg.h
// to get the meta-separated versions of these functions.

#endif // HZ4_PAGE_H
