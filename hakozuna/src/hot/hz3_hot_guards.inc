// ============================================================================
// hz3_hot_guards.inc - S78/S81/S83 guard/validation logic
// This file is included by hz3_hot.c (single TU)
// ============================================================================

// ============================================================================
// S78: PTAG32 guard (bin vs segmap mismatch)
// ============================================================================

#if HZ3_S78_PTAG32_GUARD
static _Atomic int g_s78_ptag32_guard_fired = 0;

static inline void hz3_s78_ptag32_guard(const char* where, void* ptr, uint32_t bin, uint8_t dst) {
    int bin_is_medium = (bin >= HZ3_MEDIUM_BIN_BASE && bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC));
    Hz3SegMeta* meta = hz3_segmap_get(ptr);
    int ptr_is_medium = (meta != NULL);

    if (bin_is_medium == ptr_is_medium) {
        return;
    }
    if (HZ3_S78_PTAG32_GUARD_SHOT &&
        atomic_exchange_explicit(&g_s78_ptag32_guard_fired, 1, memory_order_acq_rel) != 0) {
        return;
    }
    fprintf(stderr,
            "[HZ3_S78_PTAG32_GUARD] where=%s ptr=%p bin=%u dst=%u segmap=%p\n",
            (where ? where : "?"), ptr, bin, (unsigned)dst, (void*)meta);
    if (HZ3_S78_PTAG32_GUARD_FAILFAST) {
        abort();
    }
}
#endif

// ============================================================================
// S81: Medium free guard (free_bits check for stale runs)
// ============================================================================

#if HZ3_S81_MEDIUM_FREE_GUARD
static _Atomic int g_s81_medium_free_inited = 0;
static _Atomic int g_s81_medium_free_enabled = HZ3_S81_MEDIUM_FREE_GUARD_DEFAULT;
static _Atomic int g_s81_medium_free_shot = 0;
static _Atomic int g_s81_medium_free_logged = 0;

static inline int hz3_s81_parse_bool(const char* s, int defval) {
    if (!s || !*s) {
        return defval;
    }
    if (s[0] == '0' && s[1] == '\0') {
        return 0;
    }
    return 1;
}

static inline int hz3_s81_medium_free_guard_enabled(void) {
    if (atomic_load_explicit(&g_s81_medium_free_inited, memory_order_acquire) == 0) {
        int enabled = hz3_s81_parse_bool(getenv("HZ3_S81_MEDIUM_FREE_GUARD"),
                                         HZ3_S81_MEDIUM_FREE_GUARD_DEFAULT);
        atomic_store_explicit(&g_s81_medium_free_enabled, enabled, memory_order_release);
        atomic_store_explicit(&g_s81_medium_free_inited, 1, memory_order_release);
#if HZ3_S81_MEDIUM_FREE_GUARD_LOG
        if (!enabled &&
            atomic_exchange_explicit(&g_s81_medium_free_logged, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr, "[HZ3_S81_MEDIUM_FREE_GUARD] disabled via env\n");
        }
#endif
    }
    return atomic_load_explicit(&g_s81_medium_free_enabled, memory_order_acquire);
}

static inline int hz3_s81_free_bits_is_free(const uint64_t* bits, size_t page_idx) {
    size_t word = page_idx / 64;
    size_t bit = page_idx % 64;
    return (bits[word] >> bit) & 1u;
}

static inline int hz3_s81_run_is_free(const uint64_t* bits, size_t page_idx, size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        if (!hz3_s81_free_bits_is_free(bits, page_idx + i)) {
            return 0;
        }
    }
    return 1;
}

static inline void hz3_s81_medium_free_guard(const char* where, void* ptr, int sc, uint8_t dst) {
    if (!hz3_s81_medium_free_guard_enabled()) {
        return;
    }
    if (sc < 0 || sc >= HZ3_NUM_SC) {
        return;
    }
    uintptr_t seg_base = (uintptr_t)ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);
    Hz3SegMeta* meta = hz3_segmap_get((void*)seg_base);
    if (!meta) {
        return;
    }
    size_t page_idx = ((uintptr_t)ptr - seg_base) >> HZ3_PAGE_SHIFT;
    size_t pages = hz3_sc_to_pages(sc);
    if (page_idx + pages > HZ3_PAGES_PER_SEG) {
        return;
    }
    if (!hz3_s81_run_is_free(meta->free_bits, page_idx, pages)) {
        return;
    }
    if (HZ3_S81_MEDIUM_FREE_GUARD_SHOT &&
        atomic_exchange_explicit(&g_s81_medium_free_shot, 1, memory_order_acq_rel) != 0) {
        return;
    }
    fprintf(stderr,
            "[HZ3_S81_MEDIUM_FREE_GUARD] where=%s ptr=%p sc=%d dst=%u seg=%p page=%zu pages=%zu\n",
            (where ? where : "?"), ptr, sc, (unsigned)dst, (void*)seg_base, page_idx, pages);
    if (HZ3_S81_MEDIUM_FREE_GUARD_FAILFAST) {
        abort();
    }
}
#endif

// ============================================================================
// S83: Bin push guard (seg_hdr/segmap validation for ptag32 free)
// ============================================================================

#if HZ3_S83_BIN_PUSH_GUARD
static _Atomic int g_s83_bin_push_inited = 0;
static _Atomic int g_s83_bin_push_enabled = HZ3_S83_BIN_PUSH_GUARD_DEFAULT;
static _Atomic int g_s83_bin_push_shot = 0;
static _Atomic int g_s83_bin_push_logged = 0;

static inline int hz3_s83_parse_bool(const char* s, int defval) {
    if (!s || !*s) {
        return defval;
    }
    if (s[0] == '0' && s[1] == '\0') {
        return 0;
    }
    return 1;
}

static inline int hz3_s83_bin_push_guard_enabled(void) {
    if (atomic_load_explicit(&g_s83_bin_push_inited, memory_order_acquire) == 0) {
        int enabled = hz3_s83_parse_bool(getenv("HZ3_S83_BIN_PUSH_GUARD"),
                                         HZ3_S83_BIN_PUSH_GUARD_DEFAULT);
        atomic_store_explicit(&g_s83_bin_push_enabled, enabled, memory_order_release);
        atomic_store_explicit(&g_s83_bin_push_inited, 1, memory_order_release);
#if HZ3_S83_BIN_PUSH_GUARD_LOG
        if (!enabled &&
            atomic_exchange_explicit(&g_s83_bin_push_logged, 1, memory_order_acq_rel) == 0) {
            fprintf(stderr, "[HZ3_S83_BIN_PUSH_GUARD] disabled via env\n");
        }
#endif
    }
    return atomic_load_explicit(&g_s83_bin_push_enabled, memory_order_acquire);
}

static inline void hz3_s83_bin_push_guard_report(const char* where,
                                                 const char* why,
                                                 void* ptr,
                                                 uint32_t bin,
                                                 uint8_t dst,
                                                 uint32_t page_idx,
                                                 uint32_t tag32,
                                                 void* seg_base) {
    if (HZ3_S83_BIN_PUSH_GUARD_SHOT &&
        atomic_exchange_explicit(&g_s83_bin_push_shot, 1, memory_order_acq_rel) != 0) {
        return;
    }
    fprintf(stderr,
            "[HZ3_S83_BIN_PUSH_GUARD] where=%s why=%s ptr=%p bin=%u dst=%u page_idx=%u tag32=0x%08x seg=%p\n",
            (where ? where : "?"), why, ptr, bin, (unsigned)dst, page_idx, tag32, seg_base);
    if (HZ3_S83_BIN_PUSH_GUARD_FAILFAST) {
        abort();
    }
}

static inline void hz3_s83_bin_push_guard(const char* where, void* ptr, uint32_t bin, uint8_t dst) {
    if (!ptr) {
        return;
    }
    if (!hz3_s83_bin_push_guard_enabled()) {
        return;
    }
    if (!g_hz3_page_tag32) {
        return;
    }

    uint32_t page_idx = 0;
    if (!hz3_arena_page_index_fast(ptr, &page_idx)) {
        return;
    }
    uint32_t tag32 = hz3_pagetag32_load(page_idx);
    void* seg_base = hz3_ptr_to_seg_base(ptr);

    if (bin < HZ3_MEDIUM_BIN_BASE) {
        // small/sub4k: segment must be self-describing
        Hz3SegHdr* hdr = hz3_seg_from_ptr(ptr);
        if (!hdr || hdr->kind != HZ3_SEG_KIND_SMALL) {
            hz3_s83_bin_push_guard_report(where, "seg_hdr_invalid", ptr, bin, dst,
                                          page_idx, tag32, seg_base);
        }
        return;
    }
    if (bin < (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)) {
        // medium: segmap must be present
        Hz3SegMeta* meta = hz3_segmap_get(ptr);
        if (!meta) {
            hz3_s83_bin_push_guard_report(where, "segmap_null", ptr, bin, dst,
                                          page_idx, tag32, seg_base);
        }
        return;
    }
}
#endif
