// hz3_small_v2_s72_boundary.inc - S72 Boundary debug helpers
// Part of hz3_small_v2.c (single TU split)

// ============================================================================
// S72: Boundary debug helpers (small_v2 list/object validation)
// ============================================================================
#if HZ3_S72_BOUNDARY_DEBUG

static _Atomic int g_s72_boundary_shot = 0;

static inline int hz3_s72_boundary_should_log(void) {
#if HZ3_S72_BOUNDARY_SHOT
    return (atomic_exchange_explicit(&g_s72_boundary_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}

static void hz3_s72_boundary_report(const char* where,
                                    const char* why,
                                    uint8_t owner,
                                    int sc,
                                    void* obj,
                                    uintptr_t page_base,
                                    uint32_t ph_magic,
                                    uint8_t ph_owner,
                                    uint8_t ph_sc,
                                    uint32_t seg_magic,
                                    uint8_t seg_kind,
                                    uint8_t seg_owner) {
    if (hz3_s72_boundary_should_log()) {
        fprintf(stderr,
                "[HZ3_S72_BOUNDARY] where=%s why=%s owner=%u sc=%d obj=%p page=%p "
                "ph_magic=0x%x ph_owner=%u ph_sc=%u seg_magic=0x%x seg_kind=%u seg_owner=%u\n",
                where, why, (unsigned)owner, sc, obj, (void*)page_base,
                ph_magic, (unsigned)ph_owner, (unsigned)ph_sc,
                seg_magic, (unsigned)seg_kind, (unsigned)seg_owner);
    }
#if HZ3_S72_BOUNDARY_FAILFAST
    abort();
#endif
}

static inline int hz3_s72_boundary_check_obj_impl(const char* where,
                                                  uint8_t owner,
                                                  int sc,
                                                  void* obj) {
    if (!obj) {
        hz3_s72_boundary_report(where, "obj_null", owner, sc, obj, 0, 0, 0, 0, 0, 0, 0);
        return 0;
    }

    uint32_t seg_idx = 0;
    void* seg_base_ptr = NULL;
    if (!hz3_arena_contains_fast(obj, &seg_idx, &seg_base_ptr)) {
        hz3_s72_boundary_report(where, "seg_unused_or_out_of_arena",
                                owner, sc, obj, 0, 0, 0, 0, 0, 0, 0);
        return 0;
    }
    (void)seg_idx;

    uintptr_t page_base = (uintptr_t)obj & ~(HZ3_PAGE_SIZE - 1u);
    uintptr_t offset = (uintptr_t)obj - page_base;
    if (offset == 0) {
        hz3_s72_boundary_report(where, "page_base_ptr", owner, sc, obj, page_base, 0, 0, 0, 0, 0, 0);
        return 0;
    }
    if (offset < HZ3_SMALL_V2_PAGE_HDR_SIZE) {
        hz3_s72_boundary_report(where, "in_page_hdr", owner, sc, obj, page_base, 0, 0, 0, 0, 0, 0);
        return 0;
    }

    uint32_t page_idx = 0;
    if (!hz3_arena_page_index_fast(obj, &page_idx)) {
        hz3_s72_boundary_report(where, "page_idx_fail", owner, sc, obj, page_base, 0, 0, 0, 0, 0, 0);
        return 0;
    }

#if HZ3_PTAG_DSTBIN_ENABLE
    if (g_hz3_page_tag32) {
        uint32_t tag32 = hz3_pagetag32_load(page_idx);
        if (tag32 == 0) {
            hz3_s72_boundary_report(where, "ptag32_zero", owner, sc, obj, page_base,
                                    0, 0, 0, tag32, 0, 0);
            return 0;
        }
        uint32_t bin = hz3_pagetag32_bin(tag32);
        uint8_t dst = hz3_pagetag32_dst(tag32);
        if (bin != (uint32_t)hz3_bin_index_small(sc) || dst != owner) {
            hz3_s72_boundary_report(where, "ptag32_mismatch", owner, sc, obj, page_base,
                                    0, 0, 0, tag32, (uint8_t)bin, dst);
            return 0;
        }
    }
#endif

#if HZ3_SMALL_V2_PTAG_ENABLE
    if (g_hz3_page_tag) {
        uint16_t tag = hz3_pagetag_load(page_idx);
        int tag_sc = -1;
        int tag_owner = -1;
        int tag_kind = -1;
        hz3_pagetag_decode_with_kind(tag, &tag_sc, &tag_owner, &tag_kind);
        if (tag_kind != PTAG_KIND_V2 || tag_sc != sc || tag_owner != (int)owner) {
            hz3_s72_boundary_report(where, "ptag_mismatch", owner, sc, obj, page_base,
                                    tag, (uint8_t)tag_owner, (uint8_t)tag_sc,
                                    0, (uint8_t)tag_kind, 0);
            return 0;
        }
    }
#endif

    (void)seg_base_ptr;
    return 1;
}

void hz3_small_v2_boundary_check_obj(const char* where, uint8_t owner, int sc, void* obj) {
    hz3_s72_boundary_check_obj_impl(where, owner, sc, obj);
}

void hz3_small_v2_boundary_check_list(const char* where, uint8_t owner, int sc,
                                      void* head, void* tail, uint32_t n) {
    (void)tail;
    if (!head || n == 0) {
        return;
    }
    (void)n;
    hz3_s72_boundary_check_obj_impl(where, owner, sc, head);
}

#endif  // HZ3_S72_BOUNDARY_DEBUG
