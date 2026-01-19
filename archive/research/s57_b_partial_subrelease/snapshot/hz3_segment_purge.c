// hakozuna/hz3/src/hz3_segment_purge.c
//
// S57-B: Partial Subrelease
// Return contiguous free page ranges to OS via madvise(DONTNEED)
//
// Design:
// - Boundary: hz3_segment_purge_tick() called from hz3_epoch_force()
// - Candidate: pack pool segments, cold (last_touch_epoch expired), not active
// - Purge: free_bits & ~purged_bits → contiguous ranges → madvise(DONTNEED)
// - Safety: arena range check + ENOMEM disable

#define _GNU_SOURCE

#include "hz3_config.h"

#if HZ3_S57_PARTIAL_PURGE

#include "hz3_segment_purge.h"
#include "hz3_segment_packing.h"
#include "hz3_tcache.h"
#include "hz3_arena.h"
#include "hz3_types.h"

#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

// ============================================================================
// TLS Cache Reference
// ============================================================================

extern __thread Hz3TCache t_hz3_cache;

// ============================================================================
// Global State
// ============================================================================

// Global epoch counter for purge timing (shared across all threads)
// Note: extern referenced from hz3_segment.c for touch epoch updates
_Atomic uint32_t g_purge_epoch = 0;

// madvise disable flag (set on ENOMEM)
static bool g_madvise_disabled = false;

// ============================================================================
// Stats (observability)
// ============================================================================

#if HZ3_S57_PURGE_STATS
static _Atomic uint64_t g_s57b_madvise_calls = 0;
static _Atomic uint64_t g_s57b_purged_pages = 0;
static _Atomic uint64_t g_s57b_skipped_hot = 0;
static _Atomic uint64_t g_s57b_skipped_small = 0;
static _Atomic uint64_t g_s57b_skipped_active = 0;
static _Atomic uint64_t g_s57b_skipped_draining = 0;

// Debug counters for pack_iterate (defined below, used in atexit)
static _Atomic uint32_t g_s57b_iterate_calls = 0;
static _Atomic uint32_t g_s57b_iterate_segments = 0;

static void hz3_s57b_atexit(void) {
    fprintf(stderr, "[HZ3_S57B_STATS] madvise_calls=%lu purged_pages=%lu "
            "skipped_hot=%lu skipped_small=%lu skipped_active=%lu skipped_draining=%lu "
            "iterate_calls=%lu iterate_segments=%lu\n",
            (unsigned long)atomic_load(&g_s57b_madvise_calls),
            (unsigned long)atomic_load(&g_s57b_purged_pages),
            (unsigned long)atomic_load(&g_s57b_skipped_hot),
            (unsigned long)atomic_load(&g_s57b_skipped_small),
            (unsigned long)atomic_load(&g_s57b_skipped_active),
            (unsigned long)atomic_load(&g_s57b_skipped_draining),
            (unsigned long)atomic_load(&g_s57b_iterate_calls),
            (unsigned long)atomic_load(&g_s57b_iterate_segments));
}

static _Atomic int g_s57b_atexit_registered = 0;

static void hz3_s57b_ensure_atexit(void) {
    int expected = 0;
    if (atomic_compare_exchange_strong(&g_s57b_atexit_registered, &expected, 1)) {
        atexit(hz3_s57b_atexit);
    }
}
#endif  // HZ3_S57_PURGE_STATS

// ============================================================================
// Arena Safety: Range Check + ENOMEM Guard
// ============================================================================

static int hz3_madvise_safe(void* addr, size_t len) {
    // 1. Check if madvise has been disabled (ENOMEM occurred)
    if (g_madvise_disabled) {
        return 0;  // Silent no-op
    }

    // 2. Arena range check
    void* arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    void* arena_end = atomic_load_explicit(&g_hz3_arena_end, memory_order_relaxed);

    if (!arena_base || !arena_end) {
        return -1;  // Arena not initialized
    }

    if ((uintptr_t)addr < (uintptr_t)arena_base ||
        (uintptr_t)addr + len > (uintptr_t)arena_end) {
        return -1;  // Out of arena range
    }

    // 3. Perform madvise
    int ret = madvise(addr, len, MADV_DONTNEED);
    if (ret != 0 && errno == ENOMEM) {
        // ENOMEM: disable madvise for future calls
        g_madvise_disabled = true;
    }

    return ret;
}

// ============================================================================
// Bitmap Helpers: Find Contiguous 1s
// ============================================================================

// Find first contiguous run of 1s in bitmap.
// Sets *start to first bit position, *len to run length.
// Clears found bits from bitmap to allow iteration.
// Returns true if found, false if no more runs.
static bool hz3_bitmap_find_contiguous_ones(uint64_t* bits, uint16_t* start, uint16_t* len) {
    uint16_t total_bits = HZ3_PAGES_PER_SEG;  // 512
    uint16_t pos = 0;

    // Find first 1 bit
    while (pos < total_bits) {
        uint16_t word = pos / 64;
        uint16_t bit = pos % 64;

        if (bits[word] == 0) {
            // Skip entire word
            pos = (word + 1) * 64;
            continue;
        }

        if (bits[word] & (1ULL << bit)) {
            // Found start of run
            *start = pos;
            uint16_t run_len = 0;

            // Count consecutive 1s
            while (pos < total_bits) {
                uint16_t w = pos / 64;
                uint16_t b = pos % 64;

                if (bits[w] & (1ULL << b)) {
                    run_len++;
                    pos++;
                } else {
                    break;
                }
            }

            *len = run_len;
            return true;
        }

        pos++;
    }

    return false;  // No more runs
}

// Clear range in bitmap
static void hz3_bitmap_clear_range(uint64_t* bits, uint16_t start, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = start + i;
        uint16_t word = pos / 64;
        uint16_t bit = pos % 64;
        bits[word] &= ~(1ULL << bit);
    }
}

// Set range in bitmap
static void hz3_bitmap_set_range(uint64_t* bits, uint16_t start, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t pos = start + i;
        uint16_t word = pos / 64;
        uint16_t bit = pos % 64;
        bits[word] |= (1ULL << bit);
    }
}

// ============================================================================
// Candidate Selection
// ============================================================================

static bool hz3_segment_is_purge_candidate(Hz3SegMeta* meta, uint32_t now_epoch) {
    if (!meta) return false;

    // 1. Skip draining segments (S47)
#if HZ3_S47_SEGMENT_QUARANTINE
    if (hz3_s47_is_draining(meta->owner, meta->seg_base)) {
#if HZ3_S57_PURGE_STATS
        atomic_fetch_add(&g_s57b_skipped_draining, 1);
#endif
        return false;
    }
#endif

    // 2. Skip tcache active segments (unless IGNORE_ACTIVE is set for testing)
#if !HZ3_S57_PURGE_IGNORE_ACTIVE
    if (meta == t_hz3_cache.current_seg) {
#if HZ3_S57_PURGE_STATS
        atomic_fetch_add(&g_s57b_skipped_active, 1);
#endif
        return false;
    }
#if HZ3_S56_ACTIVE_SET
    if (meta == t_hz3_cache.active_seg1) {
#if HZ3_S57_PURGE_STATS
        atomic_fetch_add(&g_s57b_skipped_active, 1);
#endif
        return false;
    }
#endif
#endif  // !HZ3_S57_PURGE_IGNORE_ACTIVE

    // 3. Cold check: last_touch_epoch must be old enough
#if HZ3_S57_PURGE_DECAY_EPOCHS > 0
    uint32_t age = now_epoch - meta->last_touch_epoch;
    if (age < HZ3_S57_PURGE_DECAY_EPOCHS) {
#if HZ3_S57_PURGE_STATS
        atomic_fetch_add(&g_s57b_skipped_hot, 1);
#endif
        return false;
    }
#else
    (void)now_epoch;  // suppress unused warning when DECAY_EPOCHS=0
#endif

    return true;
}

// ============================================================================
// Purge Processing
// ============================================================================

// Purge free ranges in segment.
// Returns number of pages purged.
static uint32_t hz3_segment_do_purge(Hz3SegMeta* meta) {
    if (!meta || !meta->seg_base) return 0;

    uint32_t purged_pages = 0;

    // Build target bitmap: free_bits & ~purged_bits (un-purged free pages)
    uint64_t target_bits[HZ3_BITMAP_WORDS];
    for (int i = 0; i < HZ3_BITMAP_WORDS; i++) {
        target_bits[i] = meta->free_bits[i] & ~meta->purged_bits[i];
    }

    // Find and purge contiguous ranges
    uint16_t start, len;
    while (hz3_bitmap_find_contiguous_ones(target_bits, &start, &len)) {
        // Clear from target to allow iteration
        hz3_bitmap_clear_range(target_bits, start, len);

        // Skip small ranges
        if (len < HZ3_S57_PURGE_MIN_RANGE_PAGES) {
#if HZ3_S57_PURGE_STATS
            atomic_fetch_add(&g_s57b_skipped_small, 1);
#endif
            continue;
        }

        // Calculate address and size
        void* addr = (char*)meta->seg_base + start * HZ3_PAGE_SIZE;
        size_t size = len * HZ3_PAGE_SIZE;

        // Perform madvise (with safety checks)
        if (hz3_madvise_safe(addr, size) == 0) {
            // Mark as purged
            hz3_bitmap_set_range(meta->purged_bits, start, len);
            purged_pages += len;

#if HZ3_S57_PURGE_STATS
            atomic_fetch_add(&g_s57b_madvise_calls, 1);
            atomic_fetch_add(&g_s57b_purged_pages, len);
#endif
        }
    }

    return purged_pages;
}

// ============================================================================
// Iterator Callback
// ============================================================================

typedef struct {
    uint32_t now_epoch;
    uint32_t budget_pages;
    uint32_t purged_pages;
} PurgeIterCtx;

static int purge_iter_callback(Hz3SegMeta* meta, void* ctx_ptr) {
    PurgeIterCtx* ctx = (PurgeIterCtx*)ctx_ptr;

    // Check budget
    if (ctx->purged_pages >= ctx->budget_pages) {
        return 1;  // Stop iteration
    }

    // Check if candidate
    if (!hz3_segment_is_purge_candidate(meta, ctx->now_epoch)) {
        return 0;  // Continue
    }

    // Purge this segment
    uint32_t purged = hz3_segment_do_purge(meta);
    ctx->purged_pages += purged;

    return 0;  // Continue
}

// ============================================================================
// Public API
// ============================================================================

void hz3_segment_purge_tick(void) {
#if HZ3_S57_PURGE_STATS
    hz3_s57b_ensure_atexit();
#endif

    // Increment global epoch
    atomic_fetch_add_explicit(&g_purge_epoch, 1, memory_order_relaxed);
    uint32_t now_epoch = atomic_load_explicit(&g_purge_epoch, memory_order_relaxed);

    // Get my shard
    uint8_t my_shard = t_hz3_cache.my_shard;
    if (my_shard >= HZ3_NUM_SHARDS) {
        return;
    }

    // Iterate pack pool with budget
    PurgeIterCtx ctx = {
        .now_epoch = now_epoch,
        .budget_pages = HZ3_S57_PURGE_BUDGET_PAGES,
        .purged_pages = 0
    };

#if HZ3_S57_PURGE_STATS
    atomic_fetch_add(&g_s57b_iterate_calls, 1);
#endif
    uint32_t visited = hz3_pack_iterate(my_shard, purge_iter_callback, &ctx, 0);
#if HZ3_S57_PURGE_STATS
    atomic_fetch_add(&g_s57b_iterate_segments, visited);
#else
    (void)visited;  // suppress unused warning
#endif
}

void hz3_segment_clear_purged_range(Hz3SegMeta* meta, uint16_t start_page, uint16_t pages) {
    if (!meta || pages == 0) return;
    if (start_page + pages > HZ3_PAGES_PER_SEG) return;

    // Clear purged_bits for the allocated range
    // (pages need to be re-purged after next free)
    for (uint16_t i = 0; i < pages; i++) {
        uint16_t pos = start_page + i;
        uint16_t word = pos / 64;
        uint16_t bit = pos % 64;
        meta->purged_bits[word] &= ~(1ULL << bit);
    }
}

void hz3_segment_purge_init_meta(Hz3SegMeta* meta) {
    if (!meta) return;

    // Initialize last_touch_epoch to current epoch
    meta->last_touch_epoch = atomic_load_explicit(&g_purge_epoch, memory_order_relaxed);

    // Initialize purged_bits to all zeros (no pages purged yet)
    memset(meta->purged_bits, 0, sizeof(meta->purged_bits));
}

#endif  // HZ3_S57_PARTIAL_PURGE
