// hz3_small_v2_collision.inc - Collision helpers for shard management
// Part of hz3_small_v2.c (single TU split)

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

static inline int hz3_small_v2_collision_active(void) {
#if HZ3_LANE_SPLIT
    return (hz3_shard_live_count(t_hz3_cache.my_shard) > 1);
#else
    return 0;
#endif
}

static inline Hz3SegHdr* hz3_small_v2_current_hdr_get(int collision_active) {
#if HZ3_LANE_SPLIT
    if (collision_active) {
        Hz3SegHdr* hdr = t_hz3_cache.small_current_seg_base
                           ? hz3_seg_from_ptr(t_hz3_cache.small_current_seg_base)
                           : NULL;
        if (!hdr) {
            return NULL;
        }
        if (hdr->owner != t_hz3_cache.my_shard) {
            return NULL;
        }
        return hdr;
    }
#endif
    (void)collision_active;
    return t_hz3_cache.small_current_seg;
}

static inline void hz3_small_v2_current_hdr_set(Hz3SegHdr* hdr, int collision_active) {
#if HZ3_LANE_SPLIT
    t_hz3_cache.small_current_seg_base = hdr ? (void*)hdr : NULL;
    if (collision_active) {
        t_hz3_cache.small_current_seg = NULL;
        return;
    }
#endif
    (void)collision_active;
    t_hz3_cache.small_current_seg = hdr;
}

#endif  // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
