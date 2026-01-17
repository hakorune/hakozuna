#define _GNU_SOURCE

#include "hz3_s62_purge.h"

#if HZ3_S62_PURGE

#include "hz3_types.h"
#include "hz3_tcache.h"
#include "hz3_arena.h"
#include "hz3_seg_hdr.h"
#include "hz3_os_purge.h"  // hz3_os_madvise_dontneed_checked()
#include "hz3_dtor_stats.h"
#include "hz3_owner_excl.h"

// ============================================================================
// S62-2: DtorSmallPagePurgeBox - purge retired small pages
// ============================================================================
//
// This pass purges pages marked free in free_bits (via S62-1 retire or never allocated).
// Uses consecutive run batching to reduce syscall count.
// Budget control prevents runaway (same philosophy as S61).
//
// Safety: Only purges free_bits=1 pages (retired or never allocated).
// Live objects are never purged.

// Budget control (same philosophy as S61)
#ifndef HZ3_S62_PURGE_MAX_CALLS
#define HZ3_S62_PURGE_MAX_CALLS 256  // max madvise calls per dtor
#endif

// TLS access
extern __thread Hz3TCache t_hz3_cache;

// ============================================================================
// Stats (global, atomic for multi-thread safety)
// ============================================================================
HZ3_DTOR_STAT(g_s62p_dtor_calls);
HZ3_DTOR_STAT(g_s62p_segs_scanned);
HZ3_DTOR_STAT(g_s62p_candidate_segs);
HZ3_DTOR_STAT(g_s62p_pages_purged);
HZ3_DTOR_STAT(g_s62p_madvise_calls);

HZ3_DTOR_ATEXIT_FLAG(g_s62p);

static void hz3_s62p_atexit_dump(void) {
    uint32_t calls = HZ3_DTOR_STAT_LOAD(g_s62p_dtor_calls);
    uint32_t scanned = HZ3_DTOR_STAT_LOAD(g_s62p_segs_scanned);
    uint32_t candidate = HZ3_DTOR_STAT_LOAD(g_s62p_candidate_segs);
    uint32_t purged = HZ3_DTOR_STAT_LOAD(g_s62p_pages_purged);
    uint32_t madvise_calls = HZ3_DTOR_STAT_LOAD(g_s62p_madvise_calls);
    if (calls > 0 || purged > 0) {
        uint64_t purged_bytes = (uint64_t)purged * HZ3_PAGE_SIZE;
        fprintf(stderr, "[HZ3_S62_PURGE] dtor_calls=%u scanned_segs=%u candidate_segs=%u\n",
                calls, scanned, candidate);
        fprintf(stderr, "  pages_purged=%u madvise_calls=%u purged_bytes=%lu\n",
                purged, madvise_calls, (unsigned long)purged_bytes);
    }
}

// ============================================================================
// Main implementation
// ============================================================================

void hz3_s62_purge(void) {
    if (!t_hz3_cache.initialized) {
        return;
    }

    HZ3_DTOR_STAT_INC(g_s62p_dtor_calls);
    HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s62p, hz3_s62p_atexit_dump);

    uint8_t my_shard = t_hz3_cache.my_shard;

    // CollisionGuardBox:
    // S62 purge scans `hdr->free_bits` and can madvise(DONTNEED) pages.
    // Under shard collision (threads > shards), another thread can concurrently
    // allocate a page from the same segment (mark_used) while we still observe the
    // page as free and madvise it. Serialize with the shard OwnerExcl.
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(my_shard);

    // Get arena info
    void* arena_base = hz3_arena_get_base();
    if (!arena_base) {
        return;  // Arena not initialized
    }
    uint32_t slots = hz3_arena_slots();

    // Local stats for this call
    uint32_t scanned_segs = 0;
    uint32_t candidate_segs = 0;
    uint32_t pages_purged = 0;
    uint32_t madvise_calls = 0;

    // Scan arena slots
    for (uint32_t i = 0; i < slots && madvise_calls < HZ3_S62_PURGE_MAX_CALLS; i++) {
        if (!hz3_arena_slot_used(i)) {
            continue;
        }
        scanned_segs++;

        // Calculate slot base address
        void* slot_base = (char*)arena_base + ((size_t)i << HZ3_SEG_SHIFT);
        Hz3SegHdr* hdr = (Hz3SegHdr*)slot_base;

        // Safety check: verify segment header
        if (hdr->magic != HZ3_SEG_HDR_MAGIC) continue;
        if (hdr->kind != HZ3_SEG_KIND_SMALL) continue;
        if (hdr->owner != my_shard) continue;

        candidate_segs++;

        // Batch consecutive free pages (skip page 0 which is header)
        int run_start = -1;
        for (int page = 1; page <= HZ3_PAGES_PER_SEG && madvise_calls < HZ3_S62_PURGE_MAX_CALLS; page++) {
            int is_free = 0;
            if (page < HZ3_PAGES_PER_SEG) {
                int word = page / 64;
                int bit = page % 64;
                is_free = (hdr->free_bits[word] & (1ULL << bit)) != 0;
            }

            if (is_free) {
                if (run_start < 0) {
                    run_start = page;  // Start new run
                }
            } else {
                if (run_start >= 0) {
                    // End of run -> purge batch
                    int run_len = page - run_start;
                    void* run_addr = (char*)slot_base + ((size_t)run_start << HZ3_PAGE_SHIFT);
                    size_t run_size = (size_t)run_len << HZ3_PAGE_SHIFT;
                    hz3_os_madvise_dontneed_checked(run_addr, run_size);
                    pages_purged += run_len;
                    madvise_calls++;
                    run_start = -1;
                }
            }
        }
    }

    // Aggregate stats
    if (scanned_segs > 0) {
        HZ3_DTOR_STAT_ADD(g_s62p_segs_scanned, scanned_segs);
    }
    if (candidate_segs > 0) {
        HZ3_DTOR_STAT_ADD(g_s62p_candidate_segs, candidate_segs);
    }
    if (pages_purged > 0) {
        HZ3_DTOR_STAT_ADD(g_s62p_pages_purged, pages_purged);
    }
    if (madvise_calls > 0) {
        HZ3_DTOR_STAT_ADD(g_s62p_madvise_calls, madvise_calls);
    }

    hz3_owner_excl_release(excl_token);
}

#endif  // HZ3_S62_PURGE
