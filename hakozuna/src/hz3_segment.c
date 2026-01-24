#define _GNU_SOURCE

#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_arena.h"
#include "hz3_oom.h"
#include "hz3_tcache.h"
#include "hz3_owner_lease.h"
#include "hz3_seg_hdr.h"  // S113: Hz3SegHdr for Medium segment header

#if HZ3_S55_RETENTION_FROZEN
#include "hz3_retention_policy.h"
#endif

#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif

#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

#if HZ3_S49_PACK_STATS
#include <stdatomic.h>
#include <stdio.h>

// S49-2: SSOT alloc_run failure stats (free_pages >= npages but no contiguous run)
static _Atomic(uint32_t) g_alloc_run_fail_by_pages[HZ3_S49_PACK_MAX_PAGES + 1];
static _Atomic(uint32_t) g_alloc_run_fail_total;
#endif

void* hz3_segment_alloc(void** out_reserve_base, size_t* out_reserve_size) {
#if defined(_WIN32)
    // Reserve 2x to guarantee alignment (keep full reservation for later release).
    size_t reserve = HZ3_SEG_SIZE * 2;
    void* reserve_base = VirtualAlloc(NULL, reserve, MEM_RESERVE, PAGE_READWRITE);
    if (!reserve_base) {
        hz3_oom_note("seg_reserve", reserve, 0);
        *out_reserve_base = NULL;
        *out_reserve_size = 0;
        return NULL;
    }

    uintptr_t addr = (uintptr_t)reserve_base;
    uintptr_t aligned_addr = (addr + HZ3_SEG_SIZE - 1) & ~((uintptr_t)HZ3_SEG_SIZE - 1);
    void* aligned = (void*)aligned_addr;

    void* committed = VirtualAlloc(aligned, HZ3_SEG_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (!committed || committed != aligned) {
        VirtualFree(reserve_base, 0, MEM_RELEASE);
        hz3_oom_note("seg_commit", HZ3_SEG_SIZE, 0);
        *out_reserve_base = NULL;
        *out_reserve_size = 0;
        return NULL;
    }

    *out_reserve_base = reserve_base;
    *out_reserve_size = reserve;
    return aligned;
#else
    // Reserve 2x to guarantee alignment
    size_t reserve = HZ3_SEG_SIZE * 2;
    void* mem = mmap(NULL, reserve, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        hz3_oom_note("seg_reserve", reserve, 0);
        *out_reserve_base = NULL;
        *out_reserve_size = 0;
        return NULL;
    }

    // Align to 2MB boundary
    uintptr_t addr = (uintptr_t)mem;
    uintptr_t aligned_addr = (addr + HZ3_SEG_SIZE - 1) & ~((uintptr_t)HZ3_SEG_SIZE - 1);
    void* aligned = (void*)aligned_addr;

    // Trim excess (front and back)
    uintptr_t front = aligned_addr - addr;
    uintptr_t back = reserve - front - HZ3_SEG_SIZE;

    if (front > 0) {
        munmap(mem, front);
    }
    if (back > 0) {
        munmap((char*)aligned + HZ3_SEG_SIZE, back);
    }

    // After trim, reserve info is just the aligned segment
    *out_reserve_base = aligned;
    *out_reserve_size = HZ3_SEG_SIZE;

    return aligned;
#endif
}

// S12-5A: Updated to accept arena_idx for hz3_arena_free()
// S113: Initialize Hz3SegHdr at segment base for page_bin_plus1 support
Hz3SegMeta* hz3_segment_init(void* base, uint8_t owner, void* reserve_base, size_t reserve_size, uint32_t arena_idx) {
    // Allocate SegMeta separately (not in segment, for cache locality separation)
    Hz3SegMeta* meta = mmap(NULL, sizeof(Hz3SegMeta), PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (meta == MAP_FAILED) {
        hz3_oom_note("seg_meta", sizeof(Hz3SegMeta), 0);
        return NULL;
    }

    meta->owner = owner;
    memset(meta->sc_tag, 0, sizeof(meta->sc_tag));
    meta->reserve_base = reserve_base;
    meta->reserve_size = reserve_size;
    meta->arena_idx = arena_idx;  // S12-5A: store arena slot index

    // Day 3: Initialize run tracking
    meta->seg_base = base;

    // S113: Initialize Hz3SegHdr at segment base (page 0) for S110/S113 support.
    // This enables page_bin_plus1 lookup for Medium allocations in hz3_free.
#if HZ3_SEG_SELF_DESC_ENABLE
    {
        Hz3SegHdr* hdr = (Hz3SegHdr*)base;
        hdr->magic = HZ3_SEG_HDR_MAGIC;
        hdr->kind = HZ3_SEG_KIND_SMALL;  // Reuse small kind (S113 treats both the same)
        hdr->owner = owner;
        hdr->reserved = 0;
        hdr->flags = 0;
        hdr->reserved2 = 0;
        hdr->reserved3 = 0;

        // Initialize free_bits: all free except page 0
        for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
            hdr->free_bits[i] = UINT64_MAX;
        }
        hz3_bitmap_mark_used(hdr->free_bits, 0, 1);  // Page 0 = header
        hdr->free_pages = (uint16_t)(HZ3_PAGES_PER_SEG - 1);

        // Zero sub4k bits (not used for Medium)
        for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
            hdr->sub4k_run_start_bits[i] = 0;
        }

#if HZ3_S110_META_ENABLE
        // S110: zero-initialize page_bin_plus1
        for (size_t i = 0; i < HZ3_PAGES_PER_SEG; i++) {
            atomic_store_explicit(&hdr->page_bin_plus1[i], 0, memory_order_relaxed);
        }
#endif
    }

    // Meta tracks free_pages separately (subtract header page)
    meta->free_pages = HZ3_PAGES_PER_SEG - 1;
    // Set all bits to 1, then mark page 0 as used
    for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
        meta->free_bits[i] = ~0ULL;
    }
    hz3_bitmap_mark_used(meta->free_bits, 0, 1);
#else
    // Legacy: all pages free (no Hz3SegHdr)
    meta->free_pages = HZ3_PAGES_PER_SEG;
    for (size_t i = 0; i < HZ3_BITMAP_WORDS; i++) {
        meta->free_bits[i] = ~0ULL;
    }
#endif

    // Register in segmap
    hz3_segmap_set(base, meta);

    return meta;
}

void hz3_segment_free(void* base) {
    // Linearizable teardown: clear segmap and take ownership of meta in one step.
    // Prevents concurrent double-free / UAF if the same seg_base is reclaimed twice.
    Hz3SegMeta* meta = hz3_segmap_take(base);
    if (!meta) return;

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(meta->owner);

    // S12-5A: Get arena_idx before clearing
    uint32_t arena_idx = meta->arena_idx;

    // Safety: if caller frees the current thread's active segment, clear TLS pointers
    // before munmap(meta). This prevents stale Hz3SegMeta* deref later (UAF).
    if (t_hz3_cache.initialized) {
        if (t_hz3_cache.current_seg == meta) {
            t_hz3_cache.current_seg = NULL;
        }
#if HZ3_LANE_SPLIT
        if (t_hz3_cache.current_seg_base == base) {
            t_hz3_cache.current_seg_base = NULL;
        }
#endif
        if (t_hz3_cache.small_current_seg && (void*)t_hz3_cache.small_current_seg == base) {
            t_hz3_cache.small_current_seg = NULL;
        }
#if HZ3_LANE_SPLIT
        if (t_hz3_cache.small_current_seg_base == base) {
            t_hz3_cache.small_current_seg_base = NULL;
        }
#endif
    }

    // S55-2: Debt tracking (freed)
#if HZ3_S55_RETENTION_FROZEN
    hz3_retention_debt_on_free(meta->owner);
#endif

    // S49 safety: ensure meta is not left in the pack pool (stale meta -> UAF).
#if HZ3_S49_SEGMENT_PACKING
    hz3_pack_on_drain(meta->owner, meta);
#endif

    // Clear seg_base for safety
    meta->seg_base = NULL;

    // Then free memory
    void* reserve_base = meta->reserve_base;
    size_t reserve_size = meta->reserve_size;

    // Free meta
    munmap(meta, sizeof(Hz3SegMeta));

    // S12-5A: Free segment via arena (if arena_idx is valid)
    // UINT32_MAX indicates legacy allocation (not from arena)
    if (arena_idx != UINT32_MAX) {
#if HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
        // Clear page tags first (before releasing arena slot)
        hz3_arena_clear_segment_tags(base);
#endif
        hz3_arena_free(arena_idx);
    } else if (reserve_base && reserve_size > 0) {
        // Legacy path: direct munmap
        munmap(reserve_base, reserve_size);
    }

    hz3_owner_lease_release(lease_token);
}

// ============================================================================
// Run allocation (Day 3)
// ============================================================================

// S49-2: Calculate max contiguous free pages (O(PAGES_PER_SEG/64) = O(4) for 256 pages)
// slow path only - used for pack pool bucket selection
uint16_t hz3_segment_max_contig_free(Hz3SegMeta* meta) {
    if (!meta) return 0;

    uint16_t max_run = 0;
    uint16_t current_run = 0;

    for (uint32_t i = 0; i < HZ3_PAGES_PER_SEG; i++) {
        uint32_t word = i / 64;
        uint32_t bit = i % 64;
        uint64_t mask = 1ULL << bit;

        if (meta->free_bits[word] & mask) {
            // Page is free
            current_run++;
            if (current_run > max_run) {
                max_run = current_run;
            }
        } else {
            // Page is allocated
            current_run = 0;
        }
    }
    return max_run;
}

int hz3_segment_alloc_run(Hz3SegMeta* meta, size_t pages) {
    // S-OOM: Support up to 16 pages (64KB) for extended size classes
    if (!meta || pages == 0 || pages > 16) {
        return -1;
    }
    if (meta->free_pages < pages) {
        return -1;
    }

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(meta->owner);
    int ret = -1;

    // Find contiguous free pages
    int start = hz3_bitmap_find_free(meta->free_bits, pages);
    if (start < 0) {
#if HZ3_S49_PACK_STATS
        // S49-2: Record "contiguous shortage" (free_pages >= npages but no contiguous run)
        atomic_fetch_add_explicit(&g_alloc_run_fail_total, 1, memory_order_relaxed);
        if (pages <= HZ3_S49_PACK_MAX_PAGES) {
            atomic_fetch_add_explicit(&g_alloc_run_fail_by_pages[pages], 1, memory_order_relaxed);
        }
#endif
        goto cleanup;
    }

    // Mark as used
    hz3_bitmap_mark_used(meta->free_bits, (size_t)start, pages);
    meta->free_pages -= (uint16_t)pages;

    ret = start;

cleanup:
    hz3_owner_lease_release(lease_token);
    return ret;
}

void hz3_segment_free_run(Hz3SegMeta* meta, size_t start_page, size_t pages) {
    // S-OOM: Support up to 16 pages (64KB) for extended size classes
    if (!meta || pages == 0 || pages > 16) {
        return;
    }
    if (start_page + pages > HZ3_PAGES_PER_SEG) {
        return;
    }

    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(meta->owner);

    // Mark as free
    hz3_bitmap_mark_free(meta->free_bits, start_page, pages);
    meta->free_pages += (uint16_t)pages;

    // Clear tags
    for (size_t i = 0; i < pages; i++) {
        meta->sc_tag[start_page + i] = 0;
    }

#if (HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE)
    // PTAG clear at free boundary:
    // - Prevent false positives for S79 overwrite tracing on legitimate run reuse.
    // - Make "run pages are free" explicit for PageTagMap (SSOT boundary).
    void* run_base = (char*)meta->seg_base + (start_page << HZ3_PAGE_SHIFT);
    uint32_t base_page_idx = 0;
    if (hz3_arena_page_index_fast(run_base, &base_page_idx)) {
#if HZ3_SMALL_V2_PTAG_ENABLE
        if (g_hz3_page_tag) {
            for (size_t i = 0; i < pages; i++) {
                hz3_pagetag_clear(base_page_idx + (uint32_t)i);
            }
        }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
        if (g_hz3_page_tag32) {
            for (size_t i = 0; i < pages; i++) {
                hz3_pagetag32_clear(base_page_idx + (uint32_t)i);
            }
        }
#endif
    }
#endif

#if HZ3_S110_META_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
    // S113: Clear page_bin_plus1 for Medium run free (release boundary)
    {
        Hz3SegHdr* hdr = (Hz3SegHdr*)meta->seg_base;
        for (size_t i = 0; i < pages; i++) {
            atomic_store_explicit(&hdr->page_bin_plus1[start_page + i], 0, memory_order_release);
        }
    }
#endif

    hz3_owner_lease_release(lease_token);
}

Hz3SegMeta* hz3_new_segment(uint8_t owner) {
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(owner);
    // S12-5A: Use arena allocation (same as v2)
    uint32_t arena_idx = 0;
    void* base = hz3_arena_alloc(&arena_idx);
    if (!base) {
        hz3_oom_note("seg_arena", owner, 0);
        hz3_owner_lease_release(lease_token);
        return NULL;
    }
    // reserve_base/size are not used for arena allocations
    // arena_idx is stored in meta for hz3_arena_free()
    Hz3SegMeta* meta = hz3_segment_init(base, owner, base, HZ3_SEG_SIZE, arena_idx);

    if (meta) {
        // S55-2: Debt tracking (opened)
#if HZ3_S55_RETENTION_FROZEN
        hz3_retention_debt_on_open(owner);
#endif
    }

    hz3_owner_lease_release(lease_token);
    return meta;
}
