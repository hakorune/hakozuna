// hz3_small_v2_s62_api.inc - S62-1 APIs for PageRetireBox
// Part of hz3_small_v2.c (single TU split)

// ============================================================================
// S62-1: APIs for PageRetireBox
// ============================================================================

// S62-1: Pop batch from central bin (exported version)
// Returns count actually popped (0 if bin empty)
uint32_t hz3_small_v2_central_pop_batch_ext(uint8_t shard, int sc, void** out, uint32_t want) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)shard;
    (void)sc;
    (void)out;
    (void)want;
    return 0;
#else
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }
    if (shard >= HZ3_NUM_SHARDS) {
        return 0;
    }
    hz3_small_v2_central_init();
    uint32_t got = (uint32_t)hz3_central_bin_pop_batch(&g_hz3_small_v2_central[shard][sc], out, (int)want);
#if HZ3_S72_BOUNDARY_DEBUG
    for (uint32_t i = 0; i < got; i++) {
        hz3_small_v2_boundary_check_obj("small_v2:central_pop", shard, sc, out[i]);
    }
#endif
    return got;
#endif
}

// S62-1: Get capacity (objects per page) for size class
// Same logic as hz3_small_v2_fill_bin() for accurate retire判定
size_t hz3_small_v2_page_capacity(int sc) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)sc;
    return 0;
#else
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }
    size_t obj_size = hz3_small_sc_to_size(sc);
    uintptr_t start = HZ3_SMALL_V2_PAGE_HDR_SIZE;
    start = (start + (HZ3_SMALL_ALIGN - 1u)) & ~(uintptr_t)(HZ3_SMALL_ALIGN - 1u);
    size_t available = HZ3_PAGE_SIZE - start;
    return available / obj_size;
#endif
}

// S135-1B: Get size class from page header (safe boundary crossing)
// Returns: 1 on success (out_sc set), 0 on failure (invalid magic/bounds)
int hz3_small_v2_page_try_get_sc(void* page_base, uint8_t* out_sc) {
#if !HZ3_S135_PARTIAL_SC_OBS
    (void)page_base;
    (void)out_sc;
    return 0;
#else
    if (!page_base || !out_sc) {
        return 0;
    }
    Hz3SmallV2PageHdr* hdr = (Hz3SmallV2PageHdr*)page_base;

    // Validate magic
    uint32_t magic = atomic_load_explicit(&hdr->magic, memory_order_relaxed);
    if (magic != HZ3_PAGE_MAGIC) {
        return 0;
    }

    // Read size class
    uint8_t sc = hdr->sc;
    if (sc >= HZ3_SMALL_NUM_SC) {
        return 0;  // Out of bounds
    }

    *out_sc = sc;
    return 1;
#endif  // HZ3_S135_PARTIAL_SC_OBS
}

// S135-1C: Get tail waste per page for size class
// Same logic as hz3_small_v2_page_capacity() for consistency
size_t hz3_small_v2_page_tail_waste(int sc) {
#if !HZ3_S135_FULL_SC_OBS
    (void)sc;
    return 0;
#else
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }
    size_t obj_size = hz3_small_sc_to_size(sc);
    size_t capacity = hz3_small_v2_page_capacity(sc);

    // tail_waste = HZ3_PAGE_SIZE - aligned_start - capacity*obj_size
    uintptr_t start = HZ3_SMALL_V2_PAGE_HDR_SIZE;
    start = (start + (HZ3_SMALL_ALIGN - 1u)) & ~(uintptr_t)(HZ3_SMALL_ALIGN - 1u);

    // Safety: capacity*obj_size should never exceed available space
    size_t used = capacity * obj_size;
    if (HZ3_PAGE_SIZE < start + used) {
        return 0;  // Overflow guard
    }

    return HZ3_PAGE_SIZE - start - used;
#endif
}
