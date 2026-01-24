#pragma once

#include "hz3_types.h"

// Size class conversion for 4KB-64KB (16 classes)
// sc 0 = 4KB (1 page), sc 15 = 64KB (16 pages)

#define HZ3_SC_MIN_SIZE   HZ3_PAGE_SIZE          // 4KB
#define HZ3_SC_MAX_SIZE   (HZ3_PAGE_SIZE * 16)   // 64KB
#define HZ3_SC_COUNT      16

// Convert size to size class
// Returns -1 if out of range
static inline int hz3_sc_from_size(size_t size) {
    if (size < HZ3_SC_MIN_SIZE || size > HZ3_SC_MAX_SIZE) {
        return -1;
    }
    // pages = ceil(size / PAGE_SIZE) = (size + PAGE_SIZE - 1) / PAGE_SIZE
    size_t pages = (size + HZ3_PAGE_SIZE - 1) >> HZ3_PAGE_SHIFT;
    // sc = pages - 1 (0..15)
    return (int)(pages - 1);
}

// Convert size class to allocation size (in bytes)
static inline size_t hz3_sc_to_size(int sc) {
    return (size_t)(sc + 1) << HZ3_PAGE_SHIFT;
}

// Convert size class to page count
static inline size_t hz3_sc_to_pages(int sc) {
    return (size_t)(sc + 1);
}

