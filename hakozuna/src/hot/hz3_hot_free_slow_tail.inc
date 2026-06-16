#if !HZ3_PTAG16_DISPATCH_ENABLE
    // v1 fallback path: only needed when PTAG unified dispatch is not enabled
    // When HZ3_PTAG_V1_ENABLE=1, all v1/v2 handled via PageTagMap above

    // Look up segment metadata (lock-free)
    Hz3SegMeta* meta = hz3_segmap_get(ptr);
    if (!meta) {
        hz3_s263_note_slow_segmap_miss();
        if (hz3_large_free(ptr)) {
            return;
        }
        // Not our allocation, fallback
        hz3_next_free(ptr);
        return;
    }
    hz3_s263_note_slow_segmap_hit();

    // Get page index within segment (v1 path)
    void* seg_base = hz3_ptr_to_seg_base(ptr);
    size_t seg_page_idx = hz3_ptr_to_page_idx(ptr, seg_base);

    // Read tag (hot path: read-only)
    uint16_t tag = meta->sc_tag[seg_page_idx];
    if (tag == 0) {
        hz3_s263_note_slow_segmap_tag_zero();
        // Not cached / unknown, fallback
        hz3_next_free(ptr);
        return;
    }

    uint8_t kind = hz3_tag_kind(tag);
    int sc = hz3_tag_sc(tag);
    if (kind == HZ3_TAG_KIND_SMALL) {
        hz3_small_free(ptr, sc);
        return;
    }
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
    if (kind == HZ3_TAG_KIND_MEDIUM_ALIGNED) {
        hz3_tcache_ensure_init();
        uint8_t owner = meta->owner;
        if (owner == t_hz3_cache.my_shard) {
#if HZ3_TCACHE_SOA_LOCAL
            hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_medium_aligned(sc)), ptr);
#elif HZ3_LOCAL_BINS_SPLIT
            hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(hz3_bin_index_medium_aligned(sc)), ptr);
#else
            hz3_central_aligned_push(owner, sc, ptr);
#endif
        } else {
            hz3_central_aligned_push(owner, sc, ptr);
        }
        return;
    }
#endif
    if (kind != HZ3_TAG_KIND_LARGE) {
        hz3_next_free(ptr);
        return;
    }

    // Ensure TLS cache is initialized
    hz3_tcache_ensure_init();

    // Day 4: Local vs Remote free
    uint8_t owner = meta->owner;
    hz3_s263_note_slow_segmap_dispatch(owner);
    if (owner == t_hz3_cache.my_shard) {
        // Local: push to bin (fast path)
#if HZ3_TCACHE_SOA_LOCAL
        hz3_binref_push(hz3_tcache_get_local_binref(hz3_bin_index_medium(sc)), ptr);
#else
        Hz3Bin* bin = hz3_tcache_get_bin(sc);
        hz3_bin_push(bin, ptr);
#endif
    } else {
        // Remote: push to outbox (batches to inbox)
        hz3_outbox_push(owner, sc, ptr);
    }
#endif // !HZ3_PTAG16_DISPATCH_ENABLE
}
