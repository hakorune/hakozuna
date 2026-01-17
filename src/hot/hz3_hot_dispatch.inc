// ============================================================================
// hz3_hot_dispatch.inc - S113 FreeBox + FreeLeafBox dispatch helpers
// This file is included by hz3_hot.c (single TU)
// ============================================================================

// ============================================================================
// S113 FreeBox: SegMath + page_bin_plus1 fast dispatch (PTAG32 fallback)
// ============================================================================
//
// Replaces PTAG32 lookup with direct pointer math:
//   seg_base = ptr & ~(SEG_SIZE-1)  ->  1 AND
//   slot_idx = (seg_base - arena_base) >> SEG_SHIFT  ->  1 SUB + 1 SHIFT
//   Check used[slot_idx]  ->  1 load (cache line shared with allocations)
//   Read page_bin_plus1[page_idx]  ->  1 load from segment header
//
// If any check fails, returns 0 and lets PTAG32 fallback handle it.

#if HZ3_S113_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE
// Returns 1 if handled (seg math hit), 0 otherwise (fallback to PTAG32)
//
// v2 optimizations:
// - Skip used[] check (page_bin_plus1==0 handles unused segments)
// - Use relaxed memory order (arena_base is immutable after init,
//   page_bin_plus1 is written before malloc returns to same thread)
// - Fewer branches, tighter code
static inline int hz3_free_try_s113_segmath(void* ptr) {
    // Step 1: Load arena_base (relaxed: immutable after init)
#if HZ3_PTAG_DSTBIN_TLS
    void* arena_base = t_hz3_cache.arena_base;
    if (__builtin_expect(!arena_base, 0)) {
        arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        t_hz3_cache.arena_base = arena_base;
    }
#else
    void* arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_relaxed);
#endif
    if (__builtin_expect(!arena_base, 0)) {
        return 0;
    }

    // Step 2: Compute seg_base and range check in one
    uintptr_t seg_base = (uintptr_t)ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);
    uintptr_t delta = seg_base - (uintptr_t)arena_base;
    if (__builtin_expect(delta >= HZ3_ARENA_SIZE, 0)) {
        return 0;
    }

    // Step 3: Compute page_idx, skip page 0
    // (page 0 is header, bin_plus1 would be 0 anyway, but skip for clarity)
    size_t page_idx = ((uintptr_t)ptr - seg_base) >> HZ3_PAGE_SHIFT;
    if (__builtin_expect(page_idx == 0, 0)) {
        return 0;
    }

    // Step 4: Read page_bin_plus1 (relaxed: allocation path ensures visibility)
    Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;
    uint16_t bin_plus1 = atomic_load_explicit(&hdr->page_bin_plus1[page_idx], memory_order_relaxed);
    if (__builtin_expect(bin_plus1 == 0, 0)) {
        return 0;  // Unused page or external allocation
    }

    // Step 5: Extract bin and dst
    uint32_t bin = (uint32_t)(bin_plus1 - 1);
    uint8_t dst = hdr->owner;  // Non-atomic: immutable after segment assignment

    // Sanity check
    if (__builtin_expect(bin >= HZ3_BIN_TOTAL || dst >= HZ3_NUM_SHARDS, 0)) {
#if HZ3_S113_FAILFAST
        fprintf(stderr, "[HZ3_S113_BAD] bin=%u dst=%u ptr=%p\n", bin, dst, ptr);
        __builtin_trap();
#endif
        return 0;
    }

    // Step 6: Ensure tcache initialized
    hz3_tcache_ensure_init();

#if HZ3_S69_LIVECOUNT
    // S69: decrement live_count on fast free paths too (avoid overflow)
    hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_S12_V2_STATS
    t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif

    // Step 7: Dispatch to local or remote bin
#if HZ3_LOCAL_BINS_SPLIT
    if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
    } else {
#if HZ3_REMOTE_STASH_SPARSE
        hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
        t_hz3_cache.remote_hint = 1;
#endif
    }
#elif HZ3_TCACHE_SOA_BANK
    hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
    hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
    return 1;
}
#endif  // HZ3_S113_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE

// ============================================================================
// FreeLeafBox: Minimal PTAG32 hit path for leaf optimization
// ============================================================================

// Allow command-line override: -DHZ3_FREE_LEAF_ENABLE=0 for A/B testing
#ifndef HZ3_FREE_LEAF_ENABLE
#if HZ3_ENABLE && !HZ3_SHIM_FORWARD_ONLY && HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE && HZ3_PTAG32_DISPATCH_ENABLE && HZ3_PTAG_DSTBIN_FASTLOOKUP && HZ3_PTAG32_NOINRANGE
#define HZ3_FREE_LEAF_ENABLE 1
#else
#define HZ3_FREE_LEAF_ENABLE 0
#endif
#endif

#if HZ3_FREE_LEAF_ENABLE
// Returns 1 if handled (PTAG32 hit), 0 otherwise
static inline int hz3_free_try_ptag32_leaf(void* ptr) {
    uint32_t tag32 = 0;
    if (!hz3_pagetag32_lookup_hit_fast(ptr, &tag32)) {
        return 0;  // Miss - let slow path handle
    }
    hz3_tcache_ensure_init();

#if HZ3_S69_LIVECOUNT
    // S69: decrement live_count on PTAG32 leaf fast path
    hz3_s69_live_count_dec(ptr);
#endif
#if HZ3_S12_V2_STATS
    t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif

    uint32_t bin = hz3_pagetag32_bin(tag32);
    uint8_t dst = hz3_pagetag32_dst(tag32);

#if HZ3_LOCAL_BINS_SPLIT
    if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(hz3_tcache_get_local_binref(bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
#endif
    } else {
#if HZ3_REMOTE_STASH_SPARSE
        hz3_remote_stash_push(dst, bin, ptr);
#elif HZ3_TCACHE_SOA_BANK
        hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
#if HZ3_DSTBIN_REMOTE_HINT_ENABLE
        t_hz3_cache.remote_hint = 1;
#endif
    }
#elif HZ3_TCACHE_SOA_BANK
    hz3_binref_push(hz3_tcache_get_bank_binref(dst, bin), ptr);
#else
    hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
    return 1;  // Handled
}
#endif  // HZ3_FREE_LEAF_ENABLE
