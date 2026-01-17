#pragma once
// hz3_release_boundary.h - S65 ReleaseBoundaryBox
//
// Single entry point for free_bits updates. When HZ3_S65_RELEASE_BOUNDARY=1,
// ALL retire paths (S62/S64 small, sub4k, medium) MUST go through
// hz3_release_range_definitely_free() instead of direct hz3_bitmap_mark_free().

#include <stdint.h>
#include "hz3_config.h"

// Forward declarations
struct Hz3SegHdr;
struct Hz3SegMeta;

// Release reason (for stats/debugging)
typedef enum {
    HZ3_RELEASE_SMALL_PAGE_RETIRE  = 1,
    HZ3_RELEASE_SUB4K_RUN_RETIRE   = 2,
    HZ3_RELEASE_MEDIUM_RUN_RECLAIM = 3,
} Hz3ReleaseReason;

#if HZ3_S65_RELEASE_BOUNDARY

// The ONLY entry point for marking pages as free.
//
// Safety checks:
// - page_idx != 0 (header page never freed)
// - magic/kind/owner validation (Fail-Fast)
// - arena range check (hz3_os_in_arena_range)
//
// When HZ3_S65_RELEASE_LEDGER=1, enqueues to ledger for deferred purge.
// When HZ3_S65_RELEASE_LEDGER=0, does NOT call madvise (box separation).
//
// Returns: 0 on success, -1 on validation failure
int hz3_release_range_definitely_free(
    struct Hz3SegHdr* hdr,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason);

// Meta version for medium runs (Hz3SegMeta instead of Hz3SegHdr)
//
// Differences from hdr version:
// - page_idx == 0 is ALLOWED (medium has no header page)
// - Uses meta->seg_base for arena range check
// - Updates meta->free_bits, meta->free_pages, meta->sc_tag
// - Does NOT call hz3_segment_free_run() (avoids double update)
//
// Returns: 0 on success, -1 on validation failure
int hz3_release_range_definitely_free_meta(
    struct Hz3SegMeta* meta,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason);

// Stats (atexit dump)
void hz3_s65_release_boundary_dump_stats(void);

#else

// Stub when disabled
static inline int hz3_release_range_definitely_free(
    struct Hz3SegHdr* hdr,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason) {
    (void)hdr; (void)page_idx; (void)pages; (void)reason;
    return -1;
}

static inline int hz3_release_range_definitely_free_meta(
    struct Hz3SegMeta* meta,
    uint32_t page_idx,
    uint32_t pages,
    Hz3ReleaseReason reason) {
    (void)meta; (void)page_idx; (void)pages; (void)reason;
    return -1;
}

static inline void hz3_s65_release_boundary_dump_stats(void) {}

#endif // HZ3_S65_RELEASE_BOUNDARY
