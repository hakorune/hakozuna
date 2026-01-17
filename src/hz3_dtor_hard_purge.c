#define _GNU_SOURCE

#include "hz3_dtor_hard_purge.h"

#if HZ3_S61_DTOR_HARD_PURGE

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_central.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_sc.h"
#include "hz3_os_purge.h"
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#include "hz3_dtor_stats.h"
#include "hz3_owner_excl.h"
#include <errno.h>
#include <sys/mman.h>

// ============================================================================
// S61: DtorHardPurgeBox Implementation
// ============================================================================
//
// Thread-exit hard purge: drain central → segment, then madvise fully-free segments.
// Key design:
// - my_shard only (no cross-shard locks)
// - Segment-level madvise (not per-run) for efficient syscall usage
// - Purged set tracking to avoid duplicate madvise

// TLS access
extern __thread Hz3TCache t_hz3_cache;

// ============================================================================
// Stats (global, atomic for multi-thread safety)
// ============================================================================
#if HZ3_S61_DTOR_STATS
HZ3_DTOR_STAT(g_s61_dtor_calls);
HZ3_DTOR_STAT(g_s61_madvise_ok);
HZ3_DTOR_STAT(g_s61_madvise_fail);
HZ3_DTOR_STAT(g_s61_purged_segs);
HZ3_DTOR_STAT(g_s61_purged_pages);
HZ3_DTOR_STAT(g_s61_drained_objs);

HZ3_DTOR_ATEXIT_FLAG(g_s61);

static void hz3_s61_atexit_dump(void) {
    uint32_t dtor_calls = HZ3_DTOR_STAT_LOAD(g_s61_dtor_calls);
    uint32_t madvise_ok = HZ3_DTOR_STAT_LOAD(g_s61_madvise_ok);
    uint32_t madvise_fail = HZ3_DTOR_STAT_LOAD(g_s61_madvise_fail);
    uint32_t purged_segs = HZ3_DTOR_STAT_LOAD(g_s61_purged_segs);
    uint32_t purged_pages = HZ3_DTOR_STAT_LOAD(g_s61_purged_pages);
    uint32_t drained_objs = HZ3_DTOR_STAT_LOAD(g_s61_drained_objs);

    if (dtor_calls > 0 || madvise_ok > 0) {
        fprintf(stderr, "[HZ3_S61_DTOR_PURGE] dtor_calls=%u madvise_ok=%u madvise_fail=%u "
                        "purged_segs=%u purged_pages=%u drained_objs=%u\n",
                dtor_calls, madvise_ok, madvise_fail, purged_segs, purged_pages, drained_objs);
    }
}
#endif

// ============================================================================
// Main implementation
// ============================================================================

// Maximum number of segments we can track in purged set (cold path, stack array OK)
#define S61_PURGED_SET_MAX 256

static inline int hz3_s61_purged_set_contains(void* const* set, uint32_t n, void* seg_base) {
    for (uint32_t i = 0; i < n; i++) {
        if (set[i] == seg_base) {
            return 1;
        }
    }
    return 0;
}

void hz3_s61_dtor_hard_purge(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

#if HZ3_S61_DTOR_STATS
    HZ3_DTOR_STAT_INC(g_s61_dtor_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s61, hz3_s61_atexit_dump);
#endif

    uint8_t my_shard = t_hz3_cache.my_shard;

    // OwnerExcl: Protect segment metadata from collision race
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(my_shard);

    // Budget tracking
    uint32_t pages_budget = HZ3_S61_DTOR_MAX_PAGES;
    uint32_t calls_budget = HZ3_S61_DTOR_MAX_CALLS;

    // Stats
    uint32_t drained_objs = 0;
    uint32_t madvise_ok = 0;
    uint32_t madvise_fail = 0;
    uint32_t purged_segs = 0;
    uint32_t purged_pages = 0;

    // Purged set: track segments we've already madvised (avoid duplicates)
    void* purged_set[S61_PURGED_SET_MAX];
    uint32_t purged_set_count = 0;

    // Drain central bins: sc=7→0 (large to small) for better syscall efficiency
    for (int sc = HZ3_NUM_SC - 1; sc >= 0 && pages_budget > 0 && calls_budget > 0; sc--) {
        void* obj;
        while (pages_budget > 0 && calls_budget > 0 &&
               (obj = hz3_central_pop(my_shard, sc)) != NULL) {

            // Object -> Segment/Page reversal (O(1))
            uintptr_t ptr = (uintptr_t)obj;
            uintptr_t seg_base = ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);

            // Get segment metadata from segmap
            Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
            if (!meta) {
                // Safety: external allocation or unmapped - skip
                continue;
            }

            // Calculate page index and pages for this size class
            uint32_t page_idx = (uint32_t)((ptr - seg_base) >> HZ3_PAGE_SHIFT);
            uint32_t pages = (uint32_t)hz3_sc_to_pages(sc);

            // Boundary check
            if (page_idx + pages > HZ3_PAGES_PER_SEG) {
                // Safety: invalid run bounds - skip
#if HZ3_S61_DTOR_FAILFAST
                fprintf(stderr, "[HZ3_S61_FAILFAST] invalid run bounds: page_idx=%u pages=%u\n",
                        page_idx, pages);
                abort();
#endif
                continue;
            }

            // Return run to segment (marks pages as free, increments free_pages)
            hz3_segment_free_run(meta, page_idx, pages);
#if HZ3_S49_SEGMENT_PACKING
            // Keep pack pool consistent (cold path; OK to call here).
            hz3_pack_on_free(my_shard, meta);
#endif
            drained_objs++;

            // Check if segment is now fully free
            if (meta->free_pages == HZ3_PAGES_PER_SEG) {
                // Check if already purged
                if (!hz3_s61_purged_set_contains(purged_set, purged_set_count, (void*)seg_base)) {
                    // Arena bounds check
                    if (!hz3_os_in_arena_range((void*)seg_base, HZ3_SEG_SIZE)) {
#if HZ3_S61_DTOR_FAILFAST
                        fprintf(stderr, "[HZ3_S61_FAILFAST] segment outside arena: %p\n",
                                (void*)seg_base);
                        abort();
#endif
                        continue;
                    }

                    // madvise entire segment
                    int ret = madvise((void*)seg_base, HZ3_SEG_SIZE, MADV_DONTNEED);
                    if (ret == 0) {
                        madvise_ok++;
                        purged_segs++;
                        purged_pages += HZ3_PAGES_PER_SEG;
                        pages_budget -= (pages_budget >= HZ3_PAGES_PER_SEG) ?
                                        HZ3_PAGES_PER_SEG : pages_budget;
                        calls_budget--;

                        // Add to purged set
                        if (purged_set_count < S61_PURGED_SET_MAX) {
                            purged_set[purged_set_count++] = (void*)seg_base;
                        }
                    } else {
                        madvise_fail++;
#if HZ3_S61_DTOR_FAILFAST
                        fprintf(stderr, "[HZ3_S61_FAILFAST] madvise failed: %p errno=%d\n",
                                (void*)seg_base, errno);
                        abort();
#endif
                    }
                }
            }
        }
    }

#if HZ3_S61_DTOR_STATS
    if (drained_objs > 0) {
        HZ3_DTOR_STAT_ADD(g_s61_drained_objs, drained_objs);
    }
    if (madvise_ok > 0) {
        HZ3_DTOR_STAT_ADD(g_s61_madvise_ok, madvise_ok);
        HZ3_DTOR_STAT_ADD(g_s61_purged_segs, purged_segs);
        HZ3_DTOR_STAT_ADD(g_s61_purged_pages, purged_pages);
    }
    if (madvise_fail > 0) {
        HZ3_DTOR_STAT_ADD(g_s61_madvise_fail, madvise_fail);
    }
#endif

    hz3_owner_excl_release(excl_token);
}


#endif  // HZ3_S61_DTOR_HARD_PURGE
