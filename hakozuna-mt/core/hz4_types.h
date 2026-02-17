// hz4_types.h - HZ4 Common Types and Utilities
#ifndef HZ4_TYPES_H
#define HZ4_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "hz4_config.h"

// ============================================================================
// Forward declarations
// ============================================================================
typedef struct hz4_page hz4_page_t;
typedef struct hz4_seg  hz4_seg_t;
typedef struct hz4_tls  hz4_tls_t;

// ============================================================================
// Object next pointer (first word of freed object)
// ============================================================================
static inline void* hz4_obj_get_next(void* obj) {
    return *(void**)obj;
}

static inline void hz4_obj_set_next(void* obj, void* next) {
    *(void**)obj = next;
}

// ============================================================================
// Address utilities
// ============================================================================
static inline hz4_page_t* hz4_page_from_ptr(void* ptr) {
    return (hz4_page_t*)((uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1));
}

static inline hz4_seg_t* hz4_seg_from_ptr(void* ptr) {
    return (hz4_seg_t*)((uintptr_t)ptr & ~(HZ4_SEG_SIZE - 1));
}

static inline hz4_seg_t* hz4_seg_from_page(hz4_page_t* page) {
    return (hz4_seg_t*)((uintptr_t)page & ~(HZ4_SEG_SIZE - 1));
}

static inline hz4_page_t* hz4_page_from_seg(hz4_seg_t* seg, uint32_t page_idx) {
    return (hz4_page_t*)((uintptr_t)seg + ((uintptr_t)page_idx << HZ4_PAGE_SHIFT));
}

static inline uint32_t hz4_page_idx(hz4_page_t* page) {
    return (uint32_t)(((uintptr_t)page & (HZ4_SEG_SIZE - 1)) >> HZ4_PAGE_SHIFT);
}

// ============================================================================
// Shard selection (for remote_head[] and pending queue)
// ============================================================================
static inline uint32_t hz4_select_shard(hz4_page_t* page) {
    // Simple hash based on page address
    return (uint32_t)(((uintptr_t)page >> HZ4_PAGE_SHIFT) & (HZ4_REMOTE_SHARDS - 1));
}

static inline uint32_t hz4_select_shard_salted(hz4_page_t* page, uint16_t tid) {
    // Producer-dependent shard to avoid "page-fixed" contention
    uint32_t salt = (uint32_t)tid * 2654435761u;
    uint32_t mix = (uint32_t)((uintptr_t)page >> HZ4_PAGE_SHIFT);
    return (uint32_t)((mix ^ salt) & (HZ4_REMOTE_SHARDS - 1));
}

static inline uint32_t hz4_owner_shard(uint16_t tid) {
    return (uint32_t)(tid & HZ4_SHARD_MASK);
}

// ============================================================================
// Page tag (PTAG32-lite): load owner_tid+sc as one 32-bit value
// Layout: lower 16 bits = owner_tid, upper 16 bits = sc
// ============================================================================
static inline uint32_t hz4_page_tag_load_from_fields(const void* p) {
    uint32_t tag;
    __builtin_memcpy(&tag, p, sizeof(tag));
    return tag;
}

static inline uint16_t hz4_page_tag_owner(uint32_t tag) {
    return (uint16_t)(tag & 0xFFFFu);
}

static inline uint8_t hz4_page_tag_sc(uint32_t tag) {
    return (uint8_t)((tag >> 16) & 0xFFu);
}

#endif // HZ4_TYPES_H
