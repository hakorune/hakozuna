// ============================================================================
// hz3_hot_medium.inc - S12-5C Medium (v1) free by tag helper
// This file is included by hz3_hot.c (single TU)
// ============================================================================

#if HZ3_PTAG16_DISPATCH_ENABLE
// Free medium allocation (4KB+) using PageTagMap decoded sc/owner
// Uses regular bins (not small_bins) for medium size classes
static inline void hz3_medium_free_by_tag(void* ptr, int sc, int owner) {
    if (!ptr || sc < 0 || sc >= HZ3_NUM_SC) {
        return;
    }
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("medium_free_by_tag", ptr, sc, owner);
#endif
#if HZ3_S81_MEDIUM_FREE_GUARD
    hz3_s81_medium_free_guard("medium_free_by_tag", ptr, sc, (uint8_t)owner);
#endif
#if HZ3_S72_MEDIUM_DEBUG
    hz3_medium_boundary_check_ptr("medium_free_by_tag", ptr, sc, owner);
#endif

    hz3_tcache_ensure_init();

    if ((uint8_t)owner == t_hz3_cache.my_shard) {
        // Local: push to TLS bin (fast path)
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_medium(sc)), ptr);
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        hz3_bin_push(bin, ptr);
#endif
        return;
    }

    // Remote: push to outbox (batches to owner's inbox)
    hz3_outbox_push((uint8_t)owner, sc, ptr);
}

#if HZ3_FREE_FASTPATH_SPLIT
// S16-2D: Split v2/v1 medium free into noinline slow-path helpers.
// This reduces live ranges in hz3_free fast path to avoid spills.
__attribute__((noinline)) static void hz3_free_v2_impl(void* ptr, uint16_t tag) {
    int sc, owner;
    hz3_pagetag_decode(tag, &sc, &owner);
    hz3_small_v2_free_by_tag(ptr, sc, owner);
#if HZ3_S12_V2_STATS
    hz3_tcache_ensure_init();
    t_hz3_cache.stats.s12_v2_small_v2_enter++;
#endif
}

__attribute__((noinline)) static void hz3_free_medium_impl(void* ptr, uint16_t tag) {
    int sc, owner;
    hz3_pagetag_decode(tag, &sc, &owner);
    hz3_medium_free_by_tag(ptr, sc, owner);
}
#endif // HZ3_FREE_FASTPATH_SPLIT
#endif
