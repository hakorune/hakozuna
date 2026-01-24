// ============================================================================
// hz3_hot_helpers.inc - Pointer to page index conversion (0-cost, debug-friendly)
// This file is included by hz3_hot.c (single TU)
// ============================================================================

// Get page index within segment from pointer
static inline size_t hz3_ptr_to_page_idx(void* ptr, void* seg_base) {
    uintptr_t offset = (uintptr_t)ptr - (uintptr_t)seg_base;
    return offset >> HZ3_PAGE_SHIFT;
}

// Get segment base from pointer (2MB aligned)
static inline void* hz3_ptr_to_seg_base(void* ptr) {
    return (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1));
}
