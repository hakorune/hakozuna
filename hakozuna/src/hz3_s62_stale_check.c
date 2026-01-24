// hz3_s62_stale_check.c - Stale free_bits fail-fast detection (S62-1 debug)
// Purpose: Abort immediately when accessing an object on a page marked as free.

#include "hz3_s62_stale_check.h"

#if HZ3_S62_STALE_FAILFAST

#include "hz3_arena.h"
#include "hz3_seg_hdr.h"
#include "hz3_types.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

// Debug counters
static _Atomic uint64_t g_stale_check_calls = 0;
static _Atomic uint64_t g_stale_check_in_arena = 0;
static _Atomic uint64_t g_stale_check_small_seg = 0;
static pthread_once_t g_stale_atexit_once = PTHREAD_ONCE_INIT;

static void hz3_stale_check_dump(void) {
    uint64_t calls = atomic_load(&g_stale_check_calls);
    uint64_t in_arena = atomic_load(&g_stale_check_in_arena);
    uint64_t small_seg = atomic_load(&g_stale_check_small_seg);
    if (calls > 0) {
        fprintf(stderr, "[HZ3_S62_STALE_STATS] calls=%lu in_arena=%lu small_seg=%lu\n",
                calls, in_arena, small_seg);
    }
}

static void hz3_stale_register_atexit(void) {
    atexit(hz3_stale_check_dump);
}

// Check if a specific bit is set in free_bits bitmap
// Returns 1 if page is marked as free, 0 if used
static inline int hz3_free_bits_is_free(const uint64_t* bits, size_t page_idx) {
    size_t word = page_idx / 64;
    size_t bit = page_idx % 64;
    return (bits[word] >> bit) & 1;
}

void hz3_s62_stale_check_ptr(void* ptr, const char* where, int sc_or_bin_hint) {
    pthread_once(&g_stale_atexit_once, hz3_stale_register_atexit);
    atomic_fetch_add_explicit(&g_stale_check_calls, 1, memory_order_relaxed);

    if (!ptr) {
        return;
    }

    // Step 1: Check if ptr is within arena (fast range check)
    uint32_t page_idx_arena;
    if (!hz3_arena_page_index_fast(ptr, &page_idx_arena)) {
        // Not in arena - nothing to check (could be external allocation)
        return;
    }
    atomic_fetch_add_explicit(&g_stale_check_in_arena, 1, memory_order_relaxed);

    // Step 2: Compute page_base and seg_base
    uintptr_t ptr_val = (uintptr_t)ptr;
    uintptr_t page_base = ptr_val & ~((uintptr_t)HZ3_PAGE_SIZE - 1u);
    uintptr_t seg_base = ptr_val & ~((uintptr_t)HZ3_SEG_SIZE - 1u);

    // Step 3: Read segment header
    Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;

    // Validate magic (ensure it's really a hz3 segment)
    if (hdr->magic != HZ3_SEG_HDR_MAGIC) {
        // Not a hz3 segment - skip check
        return;
    }

    // Validate kind (only check small segments)
    if (hdr->kind != HZ3_SEG_KIND_SMALL) {
        // Not a small segment - skip check
        return;
    }
    atomic_fetch_add_explicit(&g_stale_check_small_seg, 1, memory_order_relaxed);

    // Step 4: Compute page index within segment
    size_t page_idx = (page_base - seg_base) >> HZ3_PAGE_SHIFT;
    if (page_idx >= HZ3_PAGES_PER_SEG) {
        // Page index out of bounds (should not happen)
        return;
    }

    // Step 5: Check if page is marked as free in free_bits
    if (hz3_free_bits_is_free(hdr->free_bits, page_idx)) {
        // STALE ACCESS DETECTED - abort immediately
        fprintf(stderr,
                "[HZ3_S62_STALE] where=%s ptr=%p seg=%p page=%zu sc=%d owner=%u\n",
                where, ptr, (void*)seg_base, page_idx, sc_or_bin_hint, hdr->owner);
        abort();
    }
}

#endif // HZ3_S62_STALE_FAILFAST
