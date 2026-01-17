// hz3_small_v2_free.inc - Free functions
// Part of hz3_small_v2.c (single TU split)

void hz3_small_v2_free_remote_slow(void* ptr, int sc, int owner) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)ptr;
    (void)sc;
    (void)owner;
#else
    if (!ptr) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("small_v2:free_remote", ptr, sc, owner);
#endif
#if HZ3_S69_LIVECOUNT
    hz3_s69_live_count_dec(ptr);  // Decrement after validation
#endif

    hz3_small_v2_central_init();
    hz3_small_v2_central_push_list((uint8_t)owner, sc, ptr, ptr, 1);
#endif
}

// Task 3: Fast free using page_hdr only (no seg_hdr read)
void hz3_small_v2_free_fast(void* ptr, void* page_hdr) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)ptr;
    (void)page_hdr;
#else
    if (!ptr || !page_hdr) {
        return;
    }

    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_hdr;
    int sc = (int)ph->sc;
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

    hz3_tcache_ensure_init();

    if (ph->owner == t_hz3_cache.my_shard) {
        hz3_small_v2_free_local_fast(ptr, sc);
        return;
    }

    hz3_small_v2_free_remote_slow(ptr, sc, ph->owner);
#endif
}

// Legacy: kept for compatibility (realloc, etc.)
void hz3_small_v2_free(void* ptr, const Hz3SegHdr* hdr) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)ptr;
    (void)hdr;
#else
    if (!ptr || !hdr) {
        return;
    }

    void* page_base = (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;
    int sc = (int)ph->sc;
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

    hz3_tcache_ensure_init();

    if (hdr->owner == t_hz3_cache.my_shard) {
        hz3_small_v2_free_local_fast(ptr, sc);
        return;
    }

    hz3_small_v2_free_remote_slow(ptr, sc, hdr->owner);
#endif
}

size_t hz3_small_v2_usable_size(void* ptr) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)ptr;
    return 0;
#else
    if (!ptr) {
        return 0;
    }

    void* page_base = (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;
    int sc = (int)ph->sc;
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }
    return hz3_small_sc_to_size(sc);
#endif
}

void hz3_small_v2_bin_flush(int sc, Hz3Bin* bin) {
#if !HZ3_SMALL_V2_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE
    (void)sc;
    (void)bin;
#else
    if (!bin || hz3_bin_is_empty(bin)) {
        return;
    }
    if (sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return;
    }

    hz3_small_v2_central_init();

    void* head = hz3_bin_take_all(bin);
    void* tail;
    uint32_t n;
    hz3_list_find_tail(head, &tail, &n);

    hz3_small_v2_central_push_list(t_hz3_cache.my_shard, sc, head, tail, n);
#endif
}
