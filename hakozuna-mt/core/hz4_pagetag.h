// hz4_pagetag.h - HZ4 Page Tag Table (eliminate magic routing)
// Box Theory: ptr→route の 1回 lookup でルーティング決定
#ifndef HZ4_PAGETAG_H
#define HZ4_PAGETAG_H

#include "hz4_config.h"
#include <stdint.h>

#if HZ4_PAGE_TAG_TABLE

#include <stdatomic.h>

// Global tag table and arena base
extern _Atomic(uint32_t)* g_hz4_page_tag;
extern void* g_hz4_arena_base;

// Initialize page tag table (called once on first segment alloc)
void hz4_pagetag_init(void* arena_base);

// ============================================================================
// Tag encode/decode
// ============================================================================
// Tag format (32-bit):
//   bit 31..28: kind (0=unknown, 1=small, 2=mid)
//   bit 27..16: sc (size class, 0-127 for small)
//   bit 15..0 : owner_tid (16-bit)
//
// tag=0 means "unknown" → fallback to legacy path
// Large は tag 登録しない（mmap 分散で arena 外になる可能性）

static inline uint32_t hz4_pagetag_encode(uint8_t kind, uint8_t sc, uint16_t owner) {
    return ((uint32_t)kind << 28) | ((uint32_t)sc << 16) | owner;
}

static inline uint8_t hz4_pagetag_kind(uint32_t tag) {
    return (uint8_t)(tag >> 28);
}

static inline uint8_t hz4_pagetag_sc(uint32_t tag) {
    return (uint8_t)((tag >> 16) & 0xFF);
}

static inline uint16_t hz4_pagetag_owner(uint32_t tag) {
    return (uint16_t)(tag & 0xFFFF);
}

// ============================================================================
// Fast lookup
// ============================================================================
// Returns 1 if tag found (non-zero), 0 if miss or out-of-arena
// On miss, caller should fallback to legacy magic-based routing

static inline int hz4_pagetag_lookup(const void* ptr, uint32_t* tag_out) {
    if (__builtin_expect(!g_hz4_arena_base, 0)) return 0;
    if (__builtin_expect(!g_hz4_page_tag, 0)) return 0;

    uintptr_t delta = (uintptr_t)ptr - (uintptr_t)g_hz4_arena_base;
    if (__builtin_expect(delta >= HZ4_ARENA_SIZE, 0)) return 0;

    uint32_t page_idx = (uint32_t)(delta >> HZ4_PAGE_SHIFT);
    uint32_t tag = atomic_load_explicit(&g_hz4_page_tag[page_idx], memory_order_relaxed);

    if (__builtin_expect(tag == 0, 0)) return 0;  // unknown → fallback

    *tag_out = tag;
    return 1;
}

// ============================================================================
// Tag store/clear (at allocation/deallocation)
// ============================================================================
// IMPORTANT: tag clear is MANDATORY when page is released/reused
// Otherwise stale tags cause memory corruption

static inline void hz4_pagetag_store(uint32_t page_idx, uint32_t tag) {
    atomic_store_explicit(&g_hz4_page_tag[page_idx], tag, memory_order_release);
}

static inline void hz4_pagetag_clear(uint32_t page_idx) {
    atomic_store_explicit(&g_hz4_page_tag[page_idx], 0, memory_order_release);
}

// ============================================================================
// Utility: ptr → page_idx
// ============================================================================
// Returns 1 if ptr is within arena, 0 otherwise

static inline int hz4_pagetag_page_idx(const void* ptr, uint32_t* idx_out) {
    if (__builtin_expect(!g_hz4_arena_base, 0)) return 0;
    uintptr_t delta = (uintptr_t)ptr - (uintptr_t)g_hz4_arena_base;
    if (__builtin_expect(delta >= HZ4_ARENA_SIZE, 0)) return 0;
    *idx_out = (uint32_t)(delta >> HZ4_PAGE_SHIFT);
    return 1;
}

#endif // HZ4_PAGE_TAG_TABLE
#endif // HZ4_PAGETAG_H
