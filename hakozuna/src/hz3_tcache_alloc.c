// hz3_tcache_alloc.c - Slow allocation path
// Extracted from hz3_tcache.c for modularization
#define _GNU_SOURCE

#include "hz3_tcache_internal.h"
#include "hz3_segment.h"
#include "hz3_segmap.h"
#include "hz3_tag.h"
#include "hz3_arena.h"
#include "hz3_owner_excl.h"
#include "hz3_owner_lease.h"
#include "hz3_s74_lane_batch.h"
#include "hz3_seg_hdr.h"  // S113: Hz3SegHdr for page_bin_plus1
#if HZ3_S47_SEGMENT_QUARANTINE
#include "hz3_segment_quarantine.h"
#endif
#if HZ3_S49_SEGMENT_PACKING
#include "hz3_segment_packing.h"
#endif
#if HZ3_S55_RETENTION_FROZEN
#include "hz3_retention_policy.h"
#endif

static inline int hz3_tcache_collision_active(void) {
#if HZ3_LANE_SPLIT
    return (hz3_shard_live_count(t_hz3_cache.my_shard) > 1);
#else
    return 0;
#endif
}

static inline Hz3SegMeta* hz3_tcache_current_seg_get(int collision_active) {
#if HZ3_LANE_SPLIT
    if (collision_active) {
        if (!t_hz3_cache.current_seg_base) {
            return NULL;
        }
        return hz3_segmap_get(t_hz3_cache.current_seg_base);
    }
#endif
    (void)collision_active;
    return t_hz3_cache.current_seg;
}

static inline void hz3_tcache_current_seg_set(Hz3SegMeta* meta, int collision_active) {
#if HZ3_LANE_SPLIT
    t_hz3_cache.current_seg_base = meta ? meta->seg_base : NULL;
    if (collision_active) {
        t_hz3_cache.current_seg = NULL;
        return;
    }
#endif
    (void)collision_active;
    t_hz3_cache.current_seg = meta;
}

// ============================================================================
// Day 3: Slow path with segment reuse
// ============================================================================

// Allocate from current segment (or new segment if needed)
static void* hz3_slow_alloc_from_segment_locked(int sc, int collision_active) {
    // Tokens acquired by caller (OwnerLease/OwnerExcl).
    void* result = NULL;

    Hz3SegMeta* meta = hz3_tcache_current_seg_get(collision_active);
    size_t pages = hz3_sc_to_pages(sc);

#if HZ3_S47_SEGMENT_QUARANTINE
    // S47: Skip draining segment (get new one instead)
    if (meta && hz3_s47_is_draining(t_hz3_cache.my_shard, meta->seg_base)) {
        meta = NULL;  // Force new segment allocation
        hz3_tcache_current_seg_set(NULL, collision_active);
    }
#if HZ3_S56_ACTIVE_SET
    // S56-2: Also skip secondary active segment if it is draining.
    if (t_hz3_cache.active_seg1 &&
        hz3_s47_is_draining(t_hz3_cache.my_shard, t_hz3_cache.active_seg1->seg_base)) {
        t_hz3_cache.active_seg1 = NULL;
    }
#endif
#endif

#if HZ3_S56_ACTIVE_SET
    if (collision_active) {
        t_hz3_cache.active_seg1 = NULL;
    }
#endif

#if HZ3_S56_ACTIVE_SET
    // S56-2: Active segment set (slow path only).
    if (!collision_active) {
        Hz3SegMeta* alt = t_hz3_cache.active_seg1;
        if (alt && alt != meta) {
            const uint16_t need = (uint16_t)pages;
            const int meta_ok = (meta && meta->free_pages >= need);
            const int alt_ok = (alt->free_pages >= need);
            if (!meta_ok && alt_ok) {
                t_hz3_cache.active_seg1 = meta;
                meta = alt;
                hz3_tcache_current_seg_set(meta, collision_active);
#if HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
                atomic_fetch_add_explicit(&g_s56_active_choose_alt, 1, memory_order_relaxed);
#endif
            } else if (meta_ok && alt_ok && alt->free_pages < meta->free_pages) {
                t_hz3_cache.active_seg1 = meta;
                meta = alt;
                hz3_tcache_current_seg_set(meta, collision_active);
#if HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
                atomic_fetch_add_explicit(&g_s56_active_choose_alt, 1, memory_order_relaxed);
#endif
            }
        }
    }
#endif

#if HZ3_S49_SEGMENT_PACKING
    // S49-2: Pack pool with retry loop
    int pack_retries = HZ3_S49_PACK_RETRIES;

#if HZ3_S49_PACK_PRESSURE_ONLY
    if (hz3_pack_pressure_active()) {
#endif
        while (pack_retries-- > 0) {
            uint32_t tries = hz3_pack_pressure_active()
                           ? HZ3_S49_PACK_TRIES_HARD
                           : HZ3_S49_PACK_TRIES_SOFT;
            Hz3SegMeta* pack_meta = hz3_pack_try_alloc(t_hz3_cache.my_shard, pages, tries);
            if (!pack_meta) break;

            int pack_start_page = hz3_segment_alloc_run(pack_meta, pages);
            if (pack_start_page >= 0) {
#if HZ3_S56_ACTIVE_SET
                Hz3SegMeta* prev = t_hz3_cache.current_seg;
#endif
                meta = pack_meta;
                hz3_tcache_current_seg_set(meta, collision_active);
#if HZ3_S56_ACTIVE_SET
                if (!collision_active && prev && prev != meta) {
                    t_hz3_cache.active_seg1 = prev;
                }
#endif
                hz3_pack_on_alloc(t_hz3_cache.my_shard, meta);

                uint16_t tag = hz3_tag_make_large(sc);
                for (size_t i = 0; i < pages; i++) {
                    meta->sc_tag[pack_start_page + i] = tag;
                }

                void* run_base = (char*)meta->seg_base + ((size_t)pack_start_page << HZ3_PAGE_SHIFT);

#if HZ3_PTAG16_DISPATCH_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
                uint32_t base_page_idx;
                if (hz3_arena_page_index_fast(run_base, &base_page_idx)) {
#if HZ3_PTAG16_DISPATCH_ENABLE
                    uint16_t ptag = hz3_pagetag_encode_v1(sc, t_hz3_cache.my_shard);
                    for (size_t i = 0; i < pages; i++) {
                        hz3_pagetag_store(base_page_idx + (uint32_t)i, ptag);
                    }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
                    int bin = hz3_bin_index_medium(sc);
                    if (g_hz3_page_tag32) {
                        uint32_t tag32 = hz3_pagetag32_encode(bin, t_hz3_cache.my_shard);
                        for (size_t i = 0; i < pages; i++) {
                            hz3_pagetag32_store(base_page_idx + (uint32_t)i, tag32);
                        }
                    }
#endif
                }
#endif

#if HZ3_S110_META_ENABLE
                // S113: Store bin+1 in segment header for Medium (pack path)
                {
                    Hz3SegHdr* hdr = (Hz3SegHdr*)meta->seg_base;
                    int bin = hz3_bin_index_medium(sc);
                    for (size_t i = 0; i < pages; i++) {
                        atomic_store_explicit(&hdr->page_bin_plus1[pack_start_page + i],
                                              (uint16_t)(bin + 1), memory_order_release);
                    }
                }
#endif

#if HZ3_OOM_SHOT
                atomic_fetch_add_explicit(&g_medium_page_alloc_sc[sc], 1, memory_order_relaxed);
#endif
                result = run_base;
                goto cleanup;
            }

            uint16_t max_contig = hz3_segment_max_contig_free(pack_meta);
            pack_meta->pack_max_fit = (max_contig > HZ3_S49_PACK_MAX_PAGES)
                                    ? HZ3_S49_PACK_MAX_PAGES
                                    : (uint8_t)max_contig;
            hz3_pack_reinsert(t_hz3_cache.my_shard, pack_meta);
        }
#if HZ3_S49_PACK_PRESSURE_ONLY
    }
#endif
#endif  // HZ3_S49_SEGMENT_PACKING

#if HZ3_S55_RETENTION_FROZEN
    Hz3RetentionLevel retention_level = hz3_retention_level_get();

    if (!meta || meta->free_pages < pages) {
        if (retention_level != HZ3_RETENTION_L0) {
#if HZ3_S49_SEGMENT_PACKING
            uint32_t boosted_tries =
                (retention_level == HZ3_RETENTION_L1) ? HZ3_S55_PACK_TRIES_L1 : HZ3_S55_PACK_TRIES_L2;
            Hz3SegMeta* pack_meta = hz3_pack_try_alloc(t_hz3_cache.my_shard, pages, boosted_tries);
            if (pack_meta && pack_meta->free_pages >= pages) {
                meta = pack_meta;
            }
#endif
        }
    }

    if ((!meta || meta->free_pages < pages) && retention_level == HZ3_RETENTION_L2) {
        if (hz3_retention_try_repay(t_hz3_cache.my_shard)) {
#if HZ3_S49_SEGMENT_PACKING
            Hz3SegMeta* pack_meta = hz3_pack_try_alloc(t_hz3_cache.my_shard, pages, HZ3_S49_PACK_TRIES_SOFT);
            if (pack_meta && pack_meta->free_pages >= pages) {
                meta = pack_meta;
            }
#endif
        }
    }
#endif  // HZ3_S55_RETENTION_FROZEN

    if (!meta || meta->free_pages < pages) {
#if HZ3_S56_ACTIVE_SET
        Hz3SegMeta* prev = t_hz3_cache.current_seg;
#endif
        meta = hz3_new_segment(t_hz3_cache.my_shard);
        if (!meta) {
#if HZ3_S47_SEGMENT_QUARANTINE
            for (int retry = 0; retry < 4 && !meta; retry++) {
                hz3_s47_compact_hard(t_hz3_cache.my_shard);
                meta = hz3_new_segment(t_hz3_cache.my_shard);
            }
#endif
            if (!meta) {
#if HZ3_OOM_SHOT
                hz3_medium_oom_dump(sc);
#endif
                goto cleanup;  // result = NULL
            }
        }
        hz3_tcache_current_seg_set(meta, collision_active);
#if HZ3_S56_ACTIVE_SET
        if (!collision_active && prev && prev != meta) {
            t_hz3_cache.active_seg1 = prev;
        }
#endif
#if HZ3_S49_SEGMENT_PACKING
        hz3_pack_on_new(t_hz3_cache.my_shard, meta);
#endif
    }

    int start_page = hz3_segment_alloc_run(meta, pages);
    if (start_page < 0) {
        meta = hz3_new_segment(t_hz3_cache.my_shard);
        if (!meta) {
#if HZ3_S47_SEGMENT_QUARANTINE
            for (int retry = 0; retry < 4 && !meta; retry++) {
                hz3_s47_compact_hard(t_hz3_cache.my_shard);
                meta = hz3_new_segment(t_hz3_cache.my_shard);
            }
#endif
            if (!meta) {
#if HZ3_OOM_SHOT
                hz3_medium_oom_dump(sc);
#endif
                goto cleanup;  // result = NULL
            }
        }
        hz3_tcache_current_seg_set(meta, collision_active);
#if HZ3_S49_SEGMENT_PACKING
        hz3_pack_on_new(t_hz3_cache.my_shard, meta);
#endif
        start_page = hz3_segment_alloc_run(meta, pages);
        if (start_page < 0) {
#if HZ3_OOM_SHOT
            hz3_medium_oom_dump(sc);
#endif
            goto cleanup;  // result = NULL
        }
    }

#if HZ3_S49_SEGMENT_PACKING
    hz3_pack_on_alloc(t_hz3_cache.my_shard, meta);
#endif

    uint16_t tag = hz3_tag_make_large(sc);
    for (size_t i = 0; i < pages; i++) {
        meta->sc_tag[start_page + i] = tag;
    }

    void* run_base = (char*)meta->seg_base + ((size_t)start_page << HZ3_PAGE_SHIFT);

#if HZ3_PTAG16_DISPATCH_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
    uint32_t base_page_idx;
    if (hz3_arena_page_index_fast(run_base, &base_page_idx)) {
#if HZ3_PTAG16_DISPATCH_ENABLE
        uint16_t ptag = hz3_pagetag_encode_v1(sc, t_hz3_cache.my_shard);
        for (size_t i = 0; i < pages; i++) {
            hz3_pagetag_store(base_page_idx + (uint32_t)i, ptag);
        }
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
        int bin = hz3_bin_index_medium(sc);
        if (g_hz3_page_tag32) {
            uint32_t ptag32 = hz3_pagetag32_encode(bin, t_hz3_cache.my_shard);
            for (size_t i = 0; i < pages; i++) {
                hz3_pagetag32_store(base_page_idx + (uint32_t)i, ptag32);
            }
        }
#endif
    }
#endif

#if HZ3_S110_META_ENABLE
    // S113: Store bin+1 in segment header per-page metadata for Medium allocations.
    // This enables S113 fast free dispatch (seg_math + page_bin_plus1).
    {
        Hz3SegHdr* hdr = (Hz3SegHdr*)meta->seg_base;
        int bin = hz3_bin_index_medium(sc);
        for (size_t i = 0; i < pages; i++) {
            atomic_store_explicit(&hdr->page_bin_plus1[start_page + i],
                                  (uint16_t)(bin + 1), memory_order_release);
        }
    }
#endif

#if HZ3_OOM_SHOT
    atomic_fetch_add_explicit(&g_medium_page_alloc_sc[sc], 1, memory_order_relaxed);
#endif
    result = run_base;

cleanup:
    return result;
}

void* hz3_slow_alloc_from_segment(int sc) {
    // OwnerLease -> OwnerExcl: avoid collision races in owner-only path.
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(t_hz3_cache.my_shard);
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(t_hz3_cache.my_shard);
    int collision_active = hz3_tcache_collision_active();
    void* result = hz3_slow_alloc_from_segment_locked(sc, collision_active);
    hz3_owner_excl_release(excl_token);
    hz3_owner_lease_release(lease_token);
    return result;
}

#if HZ3_S74_LANE_BATCH
int hz3_s74_alloc_from_segment_burst(int sc, void** out, int want) {
    if (want <= 0) {
        return 0;
    }

    hz3_s74_stats_lease_call();
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_try_acquire(t_hz3_cache.my_shard);
    if (!lease_token.active) {
        return 0;
    }

    Hz3OwnerExclToken excl_token = hz3_owner_excl_try_acquire(t_hz3_cache.my_shard);
    if (!excl_token.active) {
        hz3_owner_lease_release(lease_token);
        return 0;
    }

    int collision_active = hz3_tcache_collision_active();
    int got = 0;
    for (int i = 0; i < want; i++) {
        void* run = hz3_slow_alloc_from_segment_locked(sc, collision_active);
        if (!run) {
            break;
        }
        out[got++] = run;
    }

    hz3_owner_excl_release(excl_token);
    hz3_owner_lease_release(lease_token);

    if (got > 0) {
        hz3_s74_stats_refill((uint32_t)got);
    }
    return got;
}
#endif  // HZ3_S74_LANE_BATCH

#if HZ3_SPAN_CARVE_ENABLE
// S26-2: Span carve for sc=6,7 (28KB, 32KB)
void* hz3_slow_alloc_span_carve(int sc, Hz3Bin* bin) {
    if (sc < 6) return NULL;  // Early exit before any allocation

    // OwnerLease -> OwnerExcl: avoid collision races in owner-only path.
    Hz3OwnerLeaseToken lease_token = hz3_owner_lease_acquire(t_hz3_cache.my_shard);
    Hz3OwnerExclToken excl_token = hz3_owner_excl_acquire(t_hz3_cache.my_shard);
    void* result = NULL;

    int collision_active = hz3_tcache_collision_active();
    const int carve_count = 4;
    size_t obj_pages = hz3_sc_to_pages(sc);
    size_t span_pages = obj_pages * carve_count;

    Hz3SegMeta* meta = hz3_tcache_current_seg_get(collision_active);
    if (!meta || meta->free_pages < span_pages) {
        meta = hz3_new_segment(t_hz3_cache.my_shard);
        if (!meta) {
#if HZ3_OOM_SHOT
            hz3_medium_oom_dump(sc);
#endif
            goto cleanup;  // result = NULL
        }
        hz3_tcache_current_seg_set(meta, collision_active);
    }

    int start_page = hz3_segment_alloc_run(meta, span_pages);
    if (start_page < 0) {
        meta = hz3_new_segment(t_hz3_cache.my_shard);
        if (!meta) {
#if HZ3_OOM_SHOT
            hz3_medium_oom_dump(sc);
#endif
            goto cleanup;  // result = NULL
        }
        hz3_tcache_current_seg_set(meta, collision_active);
        start_page = hz3_segment_alloc_run(meta, span_pages);
        if (start_page < 0) {
#if HZ3_OOM_SHOT
            hz3_medium_oom_dump(sc);
#endif
            goto cleanup;  // result = NULL
        }
    }

    uint16_t tag = hz3_tag_make_large(sc);
    for (size_t i = 0; i < span_pages; i++) {
        meta->sc_tag[start_page + i] = tag;
    }

#if HZ3_S110_META_ENABLE
    // S113: Store bin+1 for all span pages (span carve path)
    {
        Hz3SegHdr* hdr = (Hz3SegHdr*)meta->seg_base;
        int bin = hz3_bin_index_medium(sc);
        for (size_t i = 0; i < span_pages; i++) {
            atomic_store_explicit(&hdr->page_bin_plus1[start_page + i],
                                  (uint16_t)(bin + 1), memory_order_release);
        }
    }
#endif

    void* first_obj = NULL;
    int bin_idx = hz3_bin_index_medium(sc);

    for (int i = 0; i < carve_count; i++) {
        size_t page_offset = (size_t)start_page + (size_t)i * obj_pages;
        void* obj = (char*)meta->seg_base + (page_offset << HZ3_PAGE_SHIFT);

#if HZ3_PTAG_DSTBIN_ENABLE
        uint32_t base_page_idx;
        if (hz3_arena_page_index_fast(obj, &base_page_idx) && g_hz3_page_tag32) {
            uint32_t ptag32 = hz3_pagetag32_encode(bin_idx, t_hz3_cache.my_shard);
            for (size_t p = 0; p < obj_pages; p++) {
                hz3_pagetag32_store(base_page_idx + (uint32_t)p, ptag32);
            }
        }
#endif
#if HZ3_PTAG16_DISPATCH_ENABLE
        uint32_t base_page_idx2;
        if (hz3_arena_page_index_fast(obj, &base_page_idx2)) {
            uint16_t ptag = hz3_pagetag_encode_v1(sc, t_hz3_cache.my_shard);
            for (size_t p = 0; p < obj_pages; p++) {
                hz3_pagetag_store(base_page_idx2 + (uint32_t)p, ptag);
            }
        }
#endif

        if (i == 0) {
            first_obj = obj;
        } else {
            hz3_bin_push(bin, obj);
        }
    }

#if HZ3_OOM_SHOT
    atomic_fetch_add_explicit(&g_medium_page_alloc_sc[sc], (uint64_t)carve_count,
                              memory_order_relaxed);
#endif
    result = first_obj;

cleanup:
    hz3_owner_excl_release(excl_token);
    hz3_owner_lease_release(lease_token);
    return result;
}
#endif
