#pragma once

#include "hz3_types.h"

// ============================================================================
// Segment allocation
// ============================================================================

// Allocate a 2MB-aligned segment
// Returns aligned base, fills out_reserve_base/out_reserve_size for munmap
void* hz3_segment_alloc(void** out_reserve_base, size_t* out_reserve_size);

// Initialize SegMeta for a segment (allocates SegMeta separately)
// S12-5A: arena_idx added for hz3_arena_free() (UINT32_MAX for legacy non-arena)
Hz3SegMeta* hz3_segment_init(void* base, uint8_t owner, void* reserve_base, size_t reserve_size, uint32_t arena_idx);

// Free a segment (clears segmap, munmaps segment and meta)
void hz3_segment_free(void* base);

// ============================================================================
// Bitmap helpers (inline, 0-cost)
// ============================================================================

// Find contiguous free pages in bitmap
// Returns start page index, or -1 if not found
static inline int hz3_bitmap_find_free(const uint64_t* bits, size_t pages) {
    // Simple linear scan (Day 3: correctness first)
    for (size_t i = 0; i + pages <= HZ3_PAGES_PER_SEG; i++) {
        int found = 1;
        for (size_t j = 0; j < pages && found; j++) {
            size_t idx = i + j;
            size_t word = idx / 64;
            size_t bit = idx % 64;
            if (!(bits[word] & (1ULL << bit))) {
                found = 0;
            }
        }
        if (found) {
            return (int)i;
        }
    }
    return -1;
}

// Mark pages as used (clear bits)
static inline void hz3_bitmap_mark_used(uint64_t* bits, size_t start, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        size_t idx = start + i;
        size_t word = idx / 64;
        size_t bit = idx % 64;
        bits[word] &= ~(1ULL << bit);
    }
}

// Mark pages as free (set bits)
static inline void hz3_bitmap_mark_free(uint64_t* bits, size_t start, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        size_t idx = start + i;
        size_t word = idx / 64;
        size_t bit = idx % 64;
        bits[word] |= (1ULL << bit);
    }
}

// ============================================================================
// Run allocation (Day 3)
// ============================================================================

// Allocate a run of pages from segment
// Returns start page index, or -1 if not enough space
int hz3_segment_alloc_run(Hz3SegMeta* meta, size_t pages);

// Free a run of pages back to segment
void hz3_segment_free_run(Hz3SegMeta* meta, size_t start_page, size_t pages);

// Create a new segment with full bitmap
Hz3SegMeta* hz3_new_segment(uint8_t owner);

// S49-2: Calculate max contiguous free pages (O(PAGES_PER_SEG/64))
// Used for pack pool bucket selection in slow path
uint16_t hz3_segment_max_contig_free(Hz3SegMeta* meta);

