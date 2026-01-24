// hz4_seg.h - HZ4 Segment Header
#ifndef HZ4_SEG_H
#define HZ4_SEG_H

#include "hz4_types.h"

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

    // ---- Reserved for future use ----
    uint32_t _reserved[4];
};

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

#endif // HZ4_SEG_H
