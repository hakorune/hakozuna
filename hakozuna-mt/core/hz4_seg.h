// hz4_seg.h - HZ4 Segment Header
#ifndef HZ4_SEG_H
#define HZ4_SEG_H

#include "hz4_config.h"
#include "hz4_types.h"

// ============================================================================
// Page Metadata (stored in segment header, survives decommit)
// ============================================================================
#if HZ4_PAGE_META_SEPARATE
typedef struct hz4_page_meta {
    // ---- MPSC remote free lists ----
#if HZ4_REMOTE_META_PADBOX
    _Alignas(64) _Atomic(void*) remote_head[HZ4_REMOTE_SHARDS];
#else
    _Atomic(void*) remote_head[HZ4_REMOTE_SHARDS];
#endif
#if HZ4_REMOTE_MASK
    _Atomic(uint64_t) remote_mask;  // shard hint (bit per shard)
#endif

#if HZ4_REMOTE_META_PADBOX
    // Pad to cacheline boundary so remote_head[]/remote_mask don't share a line with queued/qnext.
  #if defined(__SIZEOF_POINTER__)
    #define HZ4_REMOTE_META_PAD_REMOTE_BYTES (HZ4_REMOTE_SHARDS * __SIZEOF_POINTER__ + (HZ4_REMOTE_MASK ? 8 : 0))
  #else
    #define HZ4_REMOTE_META_PAD_REMOTE_BYTES (HZ4_REMOTE_SHARDS * 8 + (HZ4_REMOTE_MASK ? 8 : 0))
  #endif
  #if HZ4_REMOTE_META_PAD_REMOTE_BYTES < 64
    uint8_t _pad_remote_head[64 - HZ4_REMOTE_META_PAD_REMOTE_BYTES];
  #endif
    #undef HZ4_REMOTE_META_PAD_REMOTE_BYTES
#endif

	    // ---- Page Queue (MPSC) ----
		    _Atomic(uint8_t) queued;  // 0=idle, 1=queued
#if HZ4_REMOTE_PAGE_RBUF
		    _Atomic(uint8_t) rbufq_queued;  // 0=idle, 1=queued (RemotePageRbufBox)
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
		    _Atomic(uint8_t) rbufq_lazy_left;  // empty collect passes left before dequeue
		    _Atomic(uint8_t) rbufq_empty_streak;  // consecutive empty drains (runtime hold guard)
#endif
#endif
		    hz4_page_t* qnext;        // intrusive link (queued only) - meta へ移動
#if HZ4_REMOTE_PAGE_RBUF
		    hz4_page_t* rbufq_next;   // intrusive link (rbufq queued only)
#endif

    // ---- Metadata ----
    uint16_t owner_tid;       // owner thread ID
    uint16_t sc;              // size class

    // ---- Statistics ----
    uint16_t used_count;      // allocated objects count (owner only update)
    uint16_t capacity;        // max objects in page
#if HZ4_POPULATE_BATCH
    uint16_t bump_off;        // next object index to populate
    uint16_t bump_left;       // remaining objects not yet populated
#endif
#if HZ4_BUMP_FREE_META
    // Stage5-N7: BumpFreeMetaBox (owner-thread only)
    // Free indices recorded without touching obj memory (avoid obj->next store page-faults).
    uint16_t bump_free_n;
    uint16_t bump_free_idx[HZ4_BUMP_FREE_META_CAP];
#if HZ4_REMOTE_BUMP_FREE_META
    // Stage5-N12: RemoteBumpFreeMetaBox (remote producer + owner consumer)
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
    atomic_flag bump_rfree_lock[HZ4_REMOTE_BUMP_FREE_META_SHARDS];
    _Atomic(uint16_t) bump_rfree_n[HZ4_REMOTE_BUMP_FREE_META_SHARDS];
    uint16_t bump_rfree_idx[HZ4_REMOTE_BUMP_FREE_META_SHARDS][HZ4_REMOTE_BUMP_FREE_META_CAP];
#else
    atomic_flag bump_rfree_lock;
    _Atomic(uint16_t) bump_rfree_n;
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
    _Atomic(uint16_t) bump_rfree_pub;
    _Atomic(uint8_t) bump_rfree_draining;
#endif
    uint16_t bump_rfree_idx[HZ4_REMOTE_BUMP_FREE_META_CAP];
#endif
#endif
#endif
#if HZ4_REMOTE_FREE_META
    // Stage5-N17: RemoteFreeMetaBox v2 (remote producer + owner consumer)
    atomic_flag remote_free_lock;
    _Atomic(uint16_t) remote_free_n;
    uint16_t remote_free_idx[HZ4_REMOTE_FREE_META_CAP];
#endif

    // ---- Decommit State ----
    uint8_t decommitted;      // 0=active, 1=decommitted
    uint8_t initialized;      // 0=never used, 1=initialized

    // ---- Phase 2: DecommitQueue ----
    uint32_t empty_deadline_tick;  // deadline for decommit (0=pendingなし)
    uint8_t queued_decommit;       // 0=idle, 1=queued for decommit
    hz4_page_t* dqnext;            // next in decommit queue
    // DecommitReusePoolBox archived (NO-GO). Padding only.
#if HZ4_RSSRETURN_COOLINGBOX
    uint32_t cool_deadline;
    uint8_t cooling;
    uint8_t _pad_cool[3];
#else
    uint8_t _pad[4];               // alignment padding
#endif
#if HZ4_RSSRETURN && HZ4_RSSRETURN_QUARANTINEBOX
    // RSSReturnQuarantineBox: keep a small number of empty pages out of hot reuse
    // long enough to allow decommit attempts to happen in normal workloads.
    // State machine:
    //   0: normal
    //   1: quarantined (held out of decommit queue; maturity in empty_deadline_tick)
    //   2: matured_once (re-enqueued; force prefer_cold once then clear)
    uint8_t rss_quarantined;
    uint8_t _pad_quarantine[3];
#endif
#if HZ4_CENTRAL_PAGEHEAP
    // ---- Phase 17: CentralPageHeapBox ----
    _Atomic(uint8_t) cph_queued;     // 0=not in CPH, 1=in CPH
    struct hz4_page_meta* cph_next;  // next in central pageheap (avoid conflict with dqnext/reuse_next)
#if HZ4_CPH_2TIER
    _Atomic(uint8_t) cph_state;      // ACTIVE/SEALING/HOT/COLD
    uint32_t seal_epoch;             // collect_count at seal
#endif
#endif
} hz4_page_meta_t;
#endif

// ============================================================================
// Segment Header
// ============================================================================
// Box Theory: segment は page の集合 + pending queue 用の状態
//
// 重要: segment 寿命ルール
// - qstate != IDLE の間は retire/evict 禁止（または遅延）
// - pending queue に入った seg が処理完了前に解放されると UAF
struct hz4_seg {
    // ---- Metadata ----
    uint32_t magic;       // HZ4_SEG_MAGIC
    uint8_t  kind;        // segment kind (small/medium/large)
    uint8_t  owner;       // owner shard ID

    // ---- Pending Queue State ----
    _Atomic(uint8_t) qstate;  // 0=IDLE, 1=QUEUED, 2=PROC

    // ---- MPSC Queue Link ----
    // 非atomic: producer が自分の node.next を書くだけ
    // head のみ _Atomic (g_hz4_segq[])
    struct hz4_seg* qnext;

    // ---- Page Queue (sc-bucket 化) ----
    // seg 内の page だけを列挙する MPSC (sc bucket で分離)
    _Atomic(hz4_page_t*) pageq_head[HZ4_PAGEQ_BUCKETS];

    // ---- Pending Page Bitmap ----
    // bit i = 1 → page[i] has remote objects pending
    _Atomic(uint64_t) pending_bits[HZ4_PAGEWORDS];

#if HZ4_PAGE_META_SEPARATE
    // ---- Page Metadata Array (survives decommit) ----
    // Index 0 reserved (page0 is segment header)
    hz4_page_meta_t page_meta[HZ4_PAGES_PER_SEG];
#else
    // ---- Reserved for future use ----
    uint32_t _reserved[4];
#endif
};

#if HZ4_PAGE_META_SEPARATE
_Static_assert(sizeof(struct hz4_seg) <= HZ4_PAGE_SIZE, "hz4_seg header must fit in page0");
#endif

// ============================================================================
// Segment validation
// ============================================================================
static inline bool hz4_seg_valid(hz4_seg_t* seg) {
    return seg && seg->magic == HZ4_SEG_MAGIC;
}

// ============================================================================
// Pending bitmap operations
// ============================================================================
// Set pending bit for page (any thread)
// Note: page_idx==0 は無視 (page0 は segment header 専用)
static inline void hz4_pending_set(hz4_seg_t* seg, uint32_t page_idx) {
    if (page_idx == 0) return;  // page0 は予約
    uint32_t word = page_idx >> 6;
    uint64_t mask = 1ULL << (page_idx & 63u);
    atomic_fetch_or_explicit(&seg->pending_bits[word], mask, memory_order_release);
}

// Clear pending bit for page (owner only)
// Note: page_idx==0 は無視 (page0 は segment header 専用)
static inline void hz4_pending_clear(hz4_seg_t* seg, uint32_t page_idx) {
    if (page_idx == 0) return;  // page0 は予約
    uint32_t word = page_idx >> 6;
    uint64_t mask = 1ULL << (page_idx & 63u);
    atomic_fetch_and_explicit(&seg->pending_bits[word], ~mask, memory_order_release);
}

// Exchange pending word (owner only, for batch drain)
static inline uint64_t hz4_pending_exchange_word(hz4_seg_t* seg, uint32_t word) {
    return atomic_exchange_explicit(&seg->pending_bits[word], 0, memory_order_acq_rel);
}

// Check if any pending bits are set
static inline bool hz4_pending_any(hz4_seg_t* seg) {
    for (uint32_t w = 0; w < HZ4_PAGEWORDS; w++) {
        if (atomic_load_explicit(&seg->pending_bits[w], memory_order_acquire) != 0) {
            return true;
        }
    }
    return false;
}

#if HZ4_PAGE_META_SEPARATE
// ============================================================================
// Page Meta Access (NEW)
// ============================================================================

// Get page metadata from page pointer
static inline hz4_page_meta_t* hz4_page_meta(hz4_page_t* page) {
    hz4_seg_t* seg = hz4_seg_from_page(page);
    uint32_t page_idx = hz4_page_idx(page);

#if HZ4_FAILFAST
    if (page_idx >= HZ4_PAGES_PER_SEG) {
        HZ4_FAIL("hz4_page_meta: page_idx out of range");
    }
    if (page_idx == 0) {
        HZ4_FAIL("hz4_page_meta: page0 is reserved");
    }
#endif

    return &seg->page_meta[page_idx];
}

// Get page metadata from segment + index
static inline hz4_page_meta_t* hz4_page_meta_from_seg(hz4_seg_t* seg, uint32_t page_idx) {
#if HZ4_FAILFAST
    if (page_idx >= HZ4_PAGES_PER_SEG) {
        HZ4_FAIL("hz4_page_meta_from_seg: page_idx out of range");
    }
    if (page_idx == 0) {
        HZ4_FAIL("hz4_page_meta_from_seg: page0 is reserved");
    }
#endif

    return &seg->page_meta[page_idx];
}

// Get seg from meta (inverse of hz4_page_meta)
static inline hz4_seg_t* hz4_seg_from_page_meta(hz4_page_meta_t* meta) {
    uintptr_t meta_addr = (uintptr_t)meta;
    uintptr_t seg_base = meta_addr & ~(HZ4_SEG_SIZE - 1);
    return (hz4_seg_t*)seg_base;
}

// Get page_idx from meta
static inline uint32_t hz4_page_idx_from_meta(hz4_page_meta_t* meta) {
    hz4_seg_t* seg = hz4_seg_from_page_meta(meta);
    uintptr_t meta_offset = (uintptr_t)meta - (uintptr_t)seg->page_meta;
    return (uint32_t)(meta_offset / sizeof(hz4_page_meta_t));
}

// Check if page is decommitted
static inline bool hz4_page_is_decommitted(hz4_page_meta_t* meta) {
    return meta->decommitted != 0;
}

// Check if page is initialized
static inline bool hz4_page_is_initialized(hz4_page_meta_t* meta) {
    return meta->initialized != 0;
}

// PTAG32-lite accessors (meta-separated)
static inline uint32_t hz4_page_tag_load(hz4_page_t* page) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    return hz4_page_tag_load_from_fields(&meta->owner_tid);
}

// Used count helpers (owner-thread only)
static inline uint16_t hz4_page_used_count(hz4_page_t* page) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    return meta->used_count;
}

static inline void hz4_page_used_inc_meta(hz4_page_meta_t* meta) {
#if HZ4_CENTRAL_PAGEHEAP && HZ4_FAILFAST
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        HZ4_FAIL("hz4_page_used_inc: page is in central pageheap");
    }
#endif
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER && HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX
    uint16_t prev = meta->used_count;
#if HZ4_FAILFAST
    if (prev == UINT16_MAX) {
        HZ4_FAIL("hz4_page_used_inc: overflow");
    }
#endif
    meta->used_count = (uint16_t)(prev + 1u);
    if (prev == 0) {
        atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_relaxed);
        meta->seal_epoch = 0;
    }
#else
#if HZ4_CENTRAL_PAGEHEAP && HZ4_CPH_2TIER
    if (atomic_load_explicit(&meta->cph_state, memory_order_relaxed) != HZ4_CPH_ACTIVE) {
        atomic_store_explicit(&meta->cph_state, HZ4_CPH_ACTIVE, memory_order_relaxed);
        meta->seal_epoch = 0;
    }
#endif
#if HZ4_FAILFAST
    if (meta->used_count == UINT16_MAX) {
        HZ4_FAIL("hz4_page_used_inc: overflow");
    }
#endif
    meta->used_count++;
#endif
}

static inline void hz4_page_used_inc(hz4_page_t* page) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    hz4_page_used_inc_meta(meta);
}

static inline void hz4_page_used_dec(hz4_page_t* page, uint32_t n) {
    if (n == 0) return;
    hz4_page_meta_t* meta = hz4_page_meta(page);
#if HZ4_CENTRAL_PAGEHEAP && HZ4_FAILFAST
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        HZ4_FAIL("hz4_page_used_dec: page is in central pageheap");
    }
#endif
    if (meta->used_count >= n) {
        meta->used_count = (uint16_t)(meta->used_count - (uint16_t)n);
    } else {
        meta->used_count = 0;
    }
}

// Used count dec with preloaded meta (avoid duplicate meta lookup)
static inline uint16_t hz4_page_used_dec_meta(hz4_page_meta_t* meta, uint32_t n) {
    if (n == 0) return meta->used_count;
#if HZ4_CENTRAL_PAGEHEAP && HZ4_FAILFAST
    if (atomic_load_explicit(&meta->cph_queued, memory_order_acquire) != 0) {
        HZ4_FAIL("hz4_page_used_dec_meta: page is in central pageheap");
    }
#endif
    if (meta->used_count >= n) {
        meta->used_count = (uint16_t)(meta->used_count - (uint16_t)n);
    } else {
        meta->used_count = 0;
    }
    return meta->used_count;
}

// Used count dec without cph_queued check (ST fast path)
static inline uint16_t hz4_page_used_dec_meta_relaxed(hz4_page_meta_t* meta, uint32_t n) {
    if (n == 0) return meta->used_count;
    if (meta->used_count >= n) {
        meta->used_count = (uint16_t)(meta->used_count - (uint16_t)n);
    } else {
        meta->used_count = 0;
    }
    return meta->used_count;
}

// ============================================================================
// Remote head operations (meta-separated versions)
// ============================================================================
// These are defined here when HZ4_PAGE_META_SEPARATE is enabled
// to avoid circular dependency with hz4_page.h

// Drain: atomic_exchange で全取得 (owner のみ)
static inline void* hz4_page_drain_remote(hz4_page_t* page, uint32_t shard) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    return atomic_exchange_explicit(&meta->remote_head[shard], NULL,
                                    memory_order_acquire);
}

#if HZ4_REMOTE_MASK
static inline void hz4_page_mark_remote(hz4_page_t* page, uint32_t shard) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    atomic_fetch_or_explicit(&meta->remote_mask, (1ULL << shard), memory_order_release);
}

static inline void hz4_page_clear_remote_hint(hz4_page_t* page, uint32_t shard) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    atomic_fetch_and_explicit(&meta->remote_mask, ~(1ULL << shard), memory_order_acq_rel);
}

static inline uint64_t hz4_page_remote_mask(hz4_page_t* page) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    return atomic_load_explicit(&meta->remote_mask, memory_order_acquire);
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
    hz4_page_meta_t* meta = hz4_page_meta(page);
    _Atomic(void*)* slot = &meta->remote_head[shard];

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

// Push single object to remote_head (any thread)
// Returns true if old head was NULL (empty -> non-empty transition)
static inline bool hz4_page_push_remote_one_tid_transitioned(hz4_page_t* page, void* obj, uint16_t tid) {
    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint32_t shard = hz4_select_shard_salted(page, tid);
    _Atomic(void*)* slot = &meta->remote_head[shard];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    bool transitioned = (old == NULL);
    for (;;) {
        hz4_obj_set_next(obj, old);
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, obj,
                memory_order_release, memory_order_acquire)) {
            break;
        }
        // Retry loop: update transition status based on latest old
        transitioned = (old == NULL);
    }
    hz4_page_mark_remote(page, shard);
    return transitioned;
}

// Push list to remote_head (any thread)
static inline void hz4_page_push_remote_list(hz4_page_t* page,
                                             void* head, void* tail,
                                             uint32_t n) {
    (void)n;  // n は契約のみ (while(next) 禁止)
    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint32_t shard = hz4_select_shard(page);
    _Atomic(void*)* slot = &meta->remote_head[shard];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(tail, old);  // 毎回再設定 (CAS retry 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
    hz4_page_mark_remote(page, shard);
}

static inline void hz4_page_push_remote_list_tid(hz4_page_t* page,
                                                 void* head, void* tail,
                                                 uint32_t n, uint16_t tid) {
    (void)n;  // n は契約のみ (while(next) 禁止)
    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint32_t shard = hz4_select_shard_salted(page, tid);
    _Atomic(void*)* slot = &meta->remote_head[shard];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(tail, old);  // 毎回再設定 (CAS retry 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
    hz4_page_mark_remote(page, shard);
}

// Push list to remote_head (any thread) and report empty->non-empty transition.
// NOTE: transitioned は shard 単位。queued==0 のときは「ページ全体が空」前提で使う。
static inline bool hz4_page_push_remote_list_tid_transitioned(hz4_page_t* page,
                                                              void* head, void* tail,
                                                              uint32_t n, uint16_t tid) {
    (void)n;  // n は契約のみ (while(next) 禁止)
    hz4_page_meta_t* meta = hz4_page_meta(page);
    uint32_t shard = hz4_select_shard_salted(page, tid);
    _Atomic(void*)* slot = &meta->remote_head[shard];

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    bool transitioned = (old == NULL);
    for (;;) {
        hz4_obj_set_next(tail, old);  // 毎回再設定 (CAS retry 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
        transitioned = (old == NULL);
    }
    hz4_page_mark_remote(page, shard);
    return transitioned;
}
#endif

#endif // HZ4_SEG_H
