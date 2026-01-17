// hz3_release_boundary.c - S65 ReleaseBoundaryBox implementation
//
// The ONLY entry point for free_bits updates when HZ3_S65_RELEASE_BOUNDARY=1.

#include "hz3_release_boundary.h"
#include "hz3_seg_hdr.h"
#include "hz3_segment.h"
#include "hz3_os_purge.h"
#include "hz3_arena.h"
#include "hz3_types.h"
#include "hz3_owner_excl.h"

#include <stdio.h>
#include <stdatomic.h>

#if HZ3_S65_RELEASE_BOUNDARY

// ============================================================================
// Stats (global atomics for thread safety)
// ============================================================================

#if HZ3_S65_STATS
static _Atomic(uint64_t) g_s65_boundary_calls = 0;
static _Atomic(uint64_t) g_s65_boundary_pages = 0;
static _Atomic(uint64_t) g_s65_boundary_fail_page0 = 0;
static _Atomic(uint64_t) g_s65_boundary_fail_magic = 0;
static _Atomic(uint64_t) g_s65_boundary_fail_kind = 0;
static _Atomic(uint64_t) g_s65_boundary_fail_range = 0;
static _Atomic(uint64_t) g_s65_boundary_reason_small = 0;
static _Atomic(uint64_t) g_s65_boundary_reason_sub4k = 0;
static _Atomic(uint64_t) g_s65_boundary_reason_medium = 0;
#endif

// ============================================================================
// Forward declarations (ledger, if enabled)
// ============================================================================

#if HZ3_S65_RELEASE_LEDGER
extern void hz3_s65_ledger_enqueue(void* seg_base, uint32_t page_idx, uint32_t pages, Hz3ReleaseReason reason);
#endif

// ============================================================================
// Main boundary function
// ============================================================================

int hz3_release_range_definitely_free(
    struct Hz3SegHdr* hdr,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason)
{
    // Safety check 1: page_idx != 0 (header page never freed)
    if (page_idx == 0) {
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_boundary_fail_page0, 1, memory_order_relaxed);
#endif
        return -1;
    }

    // Safety check 2: magic validation
    if (hdr->magic != HZ3_SEG_HDR_MAGIC) {
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_boundary_fail_magic, 1, memory_order_relaxed);
#endif
        return -1;
    }

    // Safety check 3: kind validation
    if (hdr->kind != HZ3_SEG_KIND_SMALL) {
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_boundary_fail_kind, 1, memory_order_relaxed);
#endif
        return -1;
    }

    // Safety check 4: arena range check
    void* seg_base = (void*)hdr;
    size_t seg_size = HZ3_SEG_SIZE;
    if (!hz3_os_in_arena_range(seg_base, seg_size)) {
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_boundary_fail_range, 1, memory_order_relaxed);
#endif
        return -1;
    }

    // Bounds check: page_idx + pages must not exceed segment
    if (page_idx + pages > HZ3_PAGES_PER_SEG) {
        return -1;
    }

    // CollisionGuardBox: Hz3SegHdr free_bits/free_pages updates are "my_shard-only".
    // When shard collision is tolerated, multiple writers can race here.
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(hdr->owner);

    // Update free_bits (mark pages as free)
    hz3_bitmap_mark_free(hdr->free_bits, page_idx, pages);

    // Update free_pages counter
    hdr->free_pages += (uint16_t)pages;

    // PTAG clear at release boundary:
    // - Prevent false positives for S79 overwrite tracing on legitimate page reuse.
    // - Make "page is free" explicit for PageTagMap (SSOT boundary).
#if (HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE)
    uint32_t seg_base_page_idx = 0;
    if (hz3_arena_page_index_fast(seg_base, &seg_base_page_idx)) {
#if HZ3_SMALL_V2_PTAG_ENABLE
        if (g_hz3_page_tag) {
            for (uint32_t i = 0; i < pages; i++) {
                hz3_pagetag_clear(seg_base_page_idx + page_idx + i);
            }
        }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
        if (g_hz3_page_tag32) {
            for (uint32_t i = 0; i < pages; i++) {
                hz3_pagetag32_clear(seg_base_page_idx + page_idx + i);
            }
        }
#endif
    }
#endif

#if HZ3_S110_META_ENABLE
    // S110: clear page_bin_plus1 at release boundary (store-release for consistency)
    for (uint32_t i = 0; i < pages; i++) {
        atomic_store_explicit(&hdr->page_bin_plus1[page_idx + i], 0, memory_order_release);
    }
#endif

    // Stats: reason tracking
#if HZ3_S65_STATS
    atomic_fetch_add_explicit(&g_s65_boundary_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_boundary_pages, pages, memory_order_relaxed);
    switch (reason) {
        case HZ3_RELEASE_SMALL_PAGE_RETIRE:
            atomic_fetch_add_explicit(&g_s65_boundary_reason_small, pages, memory_order_relaxed);
            break;
        case HZ3_RELEASE_SUB4K_RUN_RETIRE:
            atomic_fetch_add_explicit(&g_s65_boundary_reason_sub4k, pages, memory_order_relaxed);
            break;
        case HZ3_RELEASE_MEDIUM_RUN_RECLAIM:
            atomic_fetch_add_explicit(&g_s65_boundary_reason_medium, pages, memory_order_relaxed);
            break;
    }
#else
    (void)reason;
#endif

    // Enqueue to ledger for deferred purge (if ledger enabled)
#if HZ3_S65_RELEASE_LEDGER
    hz3_s65_ledger_enqueue(seg_base, page_idx, pages, reason);
#endif

    // NOTE: When HZ3_S65_RELEASE_LEDGER=0, we do NOT call madvise here.
    // Box separation: boundary only updates free_bits, ledger owns purge.

    hz3_owner_excl_release(excl_token);
    return 0;
}

// ============================================================================
// Meta version (for medium runs)
// ============================================================================

int hz3_release_range_definitely_free_meta(
    Hz3SegMeta* meta,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason)
{
    // Safety check 1: meta != NULL
    if (!meta) {
        return -1;
    }

    // Safety check 2: meta->seg_base != NULL
    void* seg_base = meta->seg_base;
    if (!seg_base) {
        return -1;
    }

    // Bounds check: page_idx + pages must not exceed segment
    // NOTE: page_idx == 0 is ALLOWED for medium (no header page)
    if (page_idx + pages > HZ3_PAGES_PER_SEG) {
        return -1;
    }

    // Safety check 3: arena range check (seg_base only)
    if (!hz3_os_in_arena_range(seg_base, HZ3_SEG_SIZE)) {
#if HZ3_S65_STATS
        atomic_fetch_add_explicit(&g_s65_boundary_fail_range, 1, memory_order_relaxed);
#endif
        return -1;
    }

    // CollisionGuardBox: Hz3SegMeta free_bits/free_pages updates are "my_shard-only".
    // When shard collision is tolerated, multiple writers can race here.
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(meta->owner);

    // Update free_bits (mark pages as free)
    hz3_bitmap_mark_free(meta->free_bits, page_idx, pages);

    // Update free_pages counter
    meta->free_pages += (uint16_t)pages;

    // Clear sc_tag for the freed pages
    for (uint32_t i = 0; i < pages; i++) {
        meta->sc_tag[page_idx + i] = 0;
    }

    // PTAG clear at release boundary (meta variant).
#if (HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE)
    uint32_t seg_base_page_idx = 0;
    if (hz3_arena_page_index_fast(seg_base, &seg_base_page_idx)) {
#if HZ3_SMALL_V2_PTAG_ENABLE
        if (g_hz3_page_tag) {
            for (uint32_t i = 0; i < pages; i++) {
                hz3_pagetag_clear(seg_base_page_idx + page_idx + i);
            }
        }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
        if (g_hz3_page_tag32) {
            for (uint32_t i = 0; i < pages; i++) {
                hz3_pagetag32_clear(seg_base_page_idx + page_idx + i);
            }
        }
#endif
    }
#endif

    // Stats: reason tracking
#if HZ3_S65_STATS
    atomic_fetch_add_explicit(&g_s65_boundary_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s65_boundary_pages, pages, memory_order_relaxed);
    switch (reason) {
        case HZ3_RELEASE_SMALL_PAGE_RETIRE:
            atomic_fetch_add_explicit(&g_s65_boundary_reason_small, pages, memory_order_relaxed);
            break;
        case HZ3_RELEASE_SUB4K_RUN_RETIRE:
            atomic_fetch_add_explicit(&g_s65_boundary_reason_sub4k, pages, memory_order_relaxed);
            break;
        case HZ3_RELEASE_MEDIUM_RUN_RECLAIM:
            atomic_fetch_add_explicit(&g_s65_boundary_reason_medium, pages, memory_order_relaxed);
            break;
    }
#else
    (void)reason;
#endif

    // Enqueue to ledger for deferred purge (if ledger enabled)
#if HZ3_S65_RELEASE_LEDGER
    hz3_s65_ledger_enqueue(seg_base, page_idx, pages, reason);
#endif

    hz3_owner_excl_release(excl_token);
    return 0;
}

// ============================================================================
// Stats dump (atexit)
// ============================================================================

void hz3_s65_release_boundary_dump_stats(void) {
#if HZ3_S65_STATS
    uint64_t calls = atomic_load_explicit(&g_s65_boundary_calls, memory_order_relaxed);
    uint64_t pages = atomic_load_explicit(&g_s65_boundary_pages, memory_order_relaxed);
    uint64_t fail_page0 = atomic_load_explicit(&g_s65_boundary_fail_page0, memory_order_relaxed);
    uint64_t fail_magic = atomic_load_explicit(&g_s65_boundary_fail_magic, memory_order_relaxed);
    uint64_t fail_kind = atomic_load_explicit(&g_s65_boundary_fail_kind, memory_order_relaxed);
    uint64_t fail_range = atomic_load_explicit(&g_s65_boundary_fail_range, memory_order_relaxed);
    uint64_t reason_small = atomic_load_explicit(&g_s65_boundary_reason_small, memory_order_relaxed);
    uint64_t reason_sub4k = atomic_load_explicit(&g_s65_boundary_reason_sub4k, memory_order_relaxed);
    uint64_t reason_medium = atomic_load_explicit(&g_s65_boundary_reason_medium, memory_order_relaxed);

    fprintf(stderr, "[HZ3_S65_BOUNDARY] calls=%lu pages=%lu\n", calls, pages);
    fprintf(stderr, "  fail: page0=%lu magic=%lu kind=%lu range=%lu\n",
            fail_page0, fail_magic, fail_kind, fail_range);
    fprintf(stderr, "  reason: small=%lu sub4k=%lu medium=%lu\n",
            reason_small, reason_sub4k, reason_medium);
#endif
}

#endif // HZ3_S65_RELEASE_BOUNDARY
