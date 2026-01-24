// hz3_small_v2_fill.inc - Fill bin from page
// Part of hz3_small_v2.c (single TU split)

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

#if HZ3_TCACHE_SOA_LOCAL
#if HZ3_BIN_SPLIT_COUNT
// S122: SoA + Split Count version - use Hz3BinRef
static uint32_t hz3_small_v2_fill_binref(Hz3BinRef ref, int sc, void* page_base) {
    size_t obj_size = hz3_small_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page_base + HZ3_SMALL_V2_PAGE_HDR_SIZE;
    start = (start + (HZ3_SMALL_ALIGN - 1u)) & ~(uintptr_t)(HZ3_SMALL_ALIGN - 1u);

    size_t available = HZ3_PAGE_SIZE - (start - (uintptr_t)page_base);
    size_t count = available / obj_size;
    if (count == 0) {
        return 0;
    }

    char* cur = (char*)start;
    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    // cur is tail, start is head
    hz3_binref_prepend_list(ref, (void*)start, cur, (uint32_t)count);
    return (uint32_t)count;
}
#else
// S40: SoA version - directly take head pointer
static uint32_t hz3_small_v2_fill_bin_head(void** head_ptr, int sc, void* page_base) {
    size_t obj_size = hz3_small_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page_base + HZ3_SMALL_V2_PAGE_HDR_SIZE;
    start = (start + (HZ3_SMALL_ALIGN - 1u)) & ~(uintptr_t)(HZ3_SMALL_ALIGN - 1u);

    size_t available = HZ3_PAGE_SIZE - (start - (uintptr_t)page_base);
    size_t count = available / obj_size;
    if (count == 0) {
        return 0;
    }

    char* cur = (char*)start;
    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    hz3_obj_set_next(cur, *head_ptr);
    *head_ptr = (void*)start;
    return (uint32_t)count;
}
#endif  // HZ3_BIN_SPLIT_COUNT
#else
static void hz3_small_v2_fill_bin(Hz3Bin* bin, int sc, void* page_base) {
    size_t obj_size = hz3_small_sc_to_size(sc);
    uintptr_t start = (uintptr_t)page_base + HZ3_SMALL_V2_PAGE_HDR_SIZE;
    start = (start + (HZ3_SMALL_ALIGN - 1u)) & ~(uintptr_t)(HZ3_SMALL_ALIGN - 1u);

    size_t available = HZ3_PAGE_SIZE - (start - (uintptr_t)page_base);
    size_t count = available / obj_size;
    if (count == 0) {
        return;
    }

    char* cur = (char*)start;
    for (size_t i = 0; i + 1 < count; i++) {
        hz3_obj_set_next(cur, cur + obj_size);
        cur += obj_size;
    }
    hz3_small_bin_prepend_list(bin, (void*)start, cur, (uint32_t)count);
}
#endif

#endif  // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
