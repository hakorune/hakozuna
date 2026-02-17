// hz4_mid_freelist_boundary.inc - Mid FreeList boundary helpers
// Keep all page->free pointer mutations in one box to reduce race-prone direct writes.
static inline int hz4_mid_page_freelist_empty(const hz4_mid_page_t* page) {
#if HZ4_MID_FREECOUNT
    return page->free_count == 0;
#else
    return page->free == NULL;
#endif
}

static inline void hz4_mid_page_freelist_push_raw(hz4_mid_page_t* page, void* obj) {
    hz4_obj_set_next(obj, page->free);
    page->free = obj;
}

static inline void hz4_mid_page_freelist_push_count(hz4_mid_page_t* page, uint32_t n) {
    if (n == 0) {
        return;
    }
#if HZ4_MID_FREECOUNT
    page->free_count += n;
#else
    (void)page;
#endif
}

static inline void hz4_mid_page_freelist_push_list_raw(hz4_mid_page_t* page, void* head, void* tail) {
    if (!head || !tail) {
        return;
    }
    hz4_obj_set_next(tail, page->free);
    page->free = head;
}

static inline void hz4_mid_page_freelist_pop_count_dec(hz4_mid_page_t* page) {
#if HZ4_MID_FREECOUNT
    if (page->free_count > 0) {
        page->free_count--;
    }
#else
    (void)page;
#endif
}

static inline void* hz4_mid_page_pop(hz4_mid_page_t* page) {
    void* obj = page->free;
    if (obj) {
        page->free = hz4_obj_get_next(obj);
        HZ4_MID_CLEAR_NEXT(obj);
        hz4_mid_page_freelist_pop_count_dec(page);
        return obj;
    }
#if HZ4_MID_BUMP_INIT
    if ((uint32_t)page->_pad0 < page->capacity) {
        uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN);
        obj = (void*)(start + ((uintptr_t)page->_pad0 * (uintptr_t)page->obj_size));
        page->_pad0++;
        hz4_mid_page_freelist_pop_count_dec(page);
        return obj;
    }
#endif
    return NULL;
}
