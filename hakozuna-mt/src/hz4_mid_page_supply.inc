// hz4_mid_page_supply.inc - Mid page supply boundary (included from hz4_mid.c)
typedef struct hz4_mid_seg {
    struct hz4_mid_seg* next;
    uint32_t next_page;
} hz4_mid_seg_t;

static hz4_mid_seg_t* g_hz4_mid_seg_head = NULL;
static hz4_mid_seg_t* g_hz4_mid_seg_cur = NULL;

#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
// B70: TLS reservation state for amortizing segment lock traffic
typedef struct hz4_mid_page_resv {
    hz4_mid_seg_t* seg;
    uint32_t next_page;
    uint32_t end_page;
} hz4_mid_page_resv_t;
static __thread hz4_mid_page_resv_t g_mid_page_resv;
#endif

#if HZ4_MID_PREFETCHED_BIN_HEAD_BOX
// B37: Try cached hint page before full bin scan.
// Returns page if hint is valid and has free objects, NULL otherwise.
// Predecessor scan is limited to HZ4_MID_PREFETCHED_BIN_HEAD_PREV_SCAN_MAX steps.
static inline hz4_mid_page_t* hz4_mid_bin_try_prefetched_hint_locked(
    uint16_t sc, hz4_mid_page_t** out_prev) {
    hz4_mid_page_t* hint = g_mid_prefetched_bin_head[sc];

    if (!hint) {
        return NULL;  // No hint cached
    }

    // === VALIDATION CHECKS (fail-fast) ===

    // Check 1: sc mismatch
    if (hint->sc != sc) {
        g_mid_prefetched_bin_head[sc] = NULL;
        return NULL;
    }

    // Check 2: Invalid page (magic check)
    if (hint->magic != HZ4_MID_MAGIC) {
        g_mid_prefetched_bin_head[sc] = NULL;
        return NULL;
    }

    // Check 3: Owner-private page
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    if (hz4_mid_owner_tag_load(hint) != 0) {
        g_mid_prefetched_bin_head[sc] = NULL;
        return NULL;
    }
#endif

    // Check 4: Page not in bin
    if (!hint->in_bin) {
        g_mid_prefetched_bin_head[sc] = NULL;
        return NULL;
    }

    // === DRAIN REMOTE FREES (same as full scan) ===
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    if (atomic_load_explicit(&hint->remote_head, memory_order_acquire) != NULL) {
        uint32_t drained = hz4_mid_owner_drain_remote_to_local(hint);
#if HZ4_MID_LOCK_TIME_STATS
        hz4_mid_lock_stats_note_inlock_remote_drain((uint64_t)drained);
#endif
    }
#endif

    // Check 5: Page has free objects
    if (!HZ4_MID_PAGE_HAS_FREE(hint)) {
        g_mid_prefetched_bin_head[sc] = NULL;
        return NULL;
    }

    // === FIND PREDECESSOR (LIMITED SCAN) ===
    // Limit scan to N steps; if hint is too deep, fall back to full scan.
    hz4_mid_page_t* prev = NULL;
    hz4_mid_page_t* cur = g_hz4_mid_bins[sc];
    uint32_t steps = 0;
    while (cur && cur != hint) {
        prev = cur;
        cur = cur->next;
        steps++;
        if (steps > HZ4_MID_PREFETCHED_BIN_HEAD_PREV_SCAN_MAX) {
            // Hint too deep in list; drop it to avoid repeated probe misses.
            g_mid_prefetched_bin_head[sc] = NULL;
            return NULL;
        }
    }

    if (cur != hint) {
        // Hint not found in list
        g_mid_prefetched_bin_head[sc] = NULL;
        return NULL;
    }

    if (out_prev) {
        *out_prev = prev;
    }
    return hint;
}
#endif

// Scan one mid bin under sc lock and return first usable page.
// out_prev is the predecessor of returned page (or last scanned page when miss).
static inline hz4_mid_page_t* hz4_mid_bin_find_usable_locked(uint16_t sc, hz4_mid_page_t** out_prev,
                                                             uint32_t* out_steps) {
    hz4_mid_page_t* prev = NULL;
    hz4_mid_page_t* page = g_hz4_mid_bins[sc];
    uint32_t steps = 0;
    while (page) {
        steps++;
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
        // Owner-private pages should not be in global bins; skip defensively.
        if (hz4_mid_owner_tag_load(page) != 0) {
            prev = page;
            page = page->next;
            continue;
        }
        // Reconcile raced remote frees while page is unowned and lock-protected.
        if (atomic_load_explicit(&page->remote_head, memory_order_acquire) != NULL) {
            uint32_t drained = hz4_mid_owner_drain_remote_to_local(page);
#if HZ4_MID_LOCK_TIME_STATS
            hz4_mid_lock_stats_note_inlock_remote_drain((uint64_t)drained);
#else
            (void)drained;
#endif
        }
#endif
        if (HZ4_MID_PAGE_HAS_FREE(page)) {
            break;
        }
        prev = page;
        page = page->next;
    }
    if (out_prev) {
        *out_prev = prev;
    }
    if (out_steps) {
        *out_steps = steps;
    }
    return page;
}

#if HZ4_MID_PAGE_CREATE_SUPPRESS_BOX
// MidPageCreateSuppressBox:
// On bin miss, drop/reacquire lock and re-scan before creating a new page.
static inline hz4_mid_page_t* hz4_mid_page_create_suppress_retry_locked(
    uint16_t sc, hz4_mid_page_t** out_prev, uint32_t* out_steps_total) {
    hz4_mid_page_t* page = NULL;
    hz4_mid_page_t* prev = NULL;
    uint32_t steps_total = 0;
    for (uint32_t i = 0; i < (uint32_t)HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY; i++) {
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_page_create_suppress_retry);
#endif
        hz4_mid_sc_lock_release(sc);
        hz4_mid_sc_lock_acquire(sc);
        uint32_t steps = 0;
        page = hz4_mid_bin_find_usable_locked(sc, &prev, &steps);
        steps_total += steps;
        if (page) {
#if HZ4_MID_STATS_B1
            hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_page_create_suppress_hit);
#endif
            break;
        }
    }
    if (out_prev) {
        *out_prev = prev;
    }
    if (out_steps_total) {
        *out_steps_total = steps_total;
    }
    return page;
}
#endif

static hz4_mid_page_t* hz4_mid_page_create(uint16_t sc, size_t obj_size) {
    hz4_mid_page_t* page = NULL;  // Single declaration at function start

#if HZ4_MID_PAGE_SUPPLY_RESV_BOX
    // B70: Fail-fast checks for reservation state
    if (g_mid_page_resv.seg) {
        if (g_mid_page_resv.next_page > g_mid_page_resv.end_page) {
            HZ4_FAIL("hz4_mid_page_create: next_page > end_page");
            abort();
        }
        if (g_mid_page_resv.end_page > HZ4_PAGES_PER_SEG) {
            HZ4_FAIL("hz4_mid_page_create: end_page > HZ4_PAGES_PER_SEG");
            abort();
        }
    }

    // B70: Fast path - use TLS reservation if available
    if (g_mid_page_resv.seg && g_mid_page_resv.next_page < g_mid_page_resv.end_page) {
        page = (hz4_mid_page_t*)((uintptr_t)g_mid_page_resv.seg +
                                 ((uintptr_t)g_mid_page_resv.next_page << HZ4_PAGE_SHIFT));
        g_mid_page_resv.next_page++;
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_supply_resv_hit);
#endif
        goto page_init;
    }

    // B70: Refill path - acquire segment lock and reserve chunk
    hz4_mid_seg_lock_acquire();
#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_supply_resv_refill);
#endif

    if (!g_hz4_mid_seg_cur || g_hz4_mid_seg_cur->next_page >= HZ4_PAGES_PER_SEG) {
        hz4_mid_seg_t* seg = (hz4_mid_seg_t*)hz4_os_seg_acquire();
        if (!seg) {
            hz4_mid_seg_lock_release();
            abort();
        }
        seg->next_page = HZ4_PAGE_IDX_MIN;
        seg->next = g_hz4_mid_seg_head;
        g_hz4_mid_seg_head = seg;
        g_hz4_mid_seg_cur = seg;
#if HZ4_MID_STATS_B1
        hz4_mid_stats_inc(&g_hz4_mid_stats_supply_resv_new_seg);
#endif
    }

    // Reserve a chunk of pages
    uint32_t begin = g_hz4_mid_seg_cur->next_page;
    uint32_t chunk = (uint32_t)HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES;
    uint32_t resv_end = begin + chunk;
    if (resv_end > HZ4_PAGES_PER_SEG) {
        resv_end = HZ4_PAGES_PER_SEG;
    }
    g_hz4_mid_seg_cur->next_page = resv_end;

    // Update TLS reservation
    g_mid_page_resv.seg = g_hz4_mid_seg_cur;
    g_mid_page_resv.next_page = begin + 1;  // Return first page, reserve rest
    g_mid_page_resv.end_page = resv_end;

    page = (hz4_mid_page_t*)((uintptr_t)g_hz4_mid_seg_cur +
                             ((uintptr_t)begin << HZ4_PAGE_SHIFT));
#if HZ4_MID_STATS_B1
    hz4_mid_stats_add(&g_hz4_mid_stats_supply_resv_pages, (resv_end - begin));
#endif
    hz4_mid_seg_lock_release();
    goto page_init;

page_init:
#else
    // Original path (B70 disabled) - unchanged
    hz4_mid_seg_lock_acquire();
    if (!g_hz4_mid_seg_cur || g_hz4_mid_seg_cur->next_page >= HZ4_PAGES_PER_SEG) {
        hz4_mid_seg_t* seg = (hz4_mid_seg_t*)hz4_os_seg_acquire();
        if (!seg) {
            abort();
        }
        seg->next_page = HZ4_PAGE_IDX_MIN;
        seg->next = g_hz4_mid_seg_head;
        g_hz4_mid_seg_head = seg;
        g_hz4_mid_seg_cur = seg;
    }
    page = (hz4_mid_page_t*)((uintptr_t)g_hz4_mid_seg_cur +
                             ((uintptr_t)g_hz4_mid_seg_cur->next_page << HZ4_PAGE_SHIFT));
    g_hz4_mid_seg_cur->next_page++;
    hz4_mid_seg_lock_release();
#endif

    // Page init (unchanged)
    page->magic = HZ4_MID_MAGIC;
    page->sc = sc;
    page->_pad0 = 0;
    page->obj_size = (uint32_t)obj_size;
    page->capacity = 0;
    page->free_count = 0;
    page->free = NULL;
    page->next = NULL;
    page->in_bin = 0;
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
    atomic_store_explicit(&page->owner_tag, 0, memory_order_relaxed);
    atomic_store_explicit(&page->remote_head, NULL, memory_order_relaxed);
    atomic_store_explicit(&page->remote_count, 0, memory_order_relaxed);
#endif

    uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_mid_page_t), HZ4_SIZE_ALIGN);
    uintptr_t end = (uintptr_t)page + HZ4_PAGE_SIZE;

#if HZ4_MID_BUMP_INIT
    uint32_t count = (uint32_t)((end - start) / obj_size);
    page->free = NULL;
    page->capacity = count;
    page->free_count = count;
#else
    void* head = NULL;
    uint32_t count = 0;
    for (uintptr_t p = start; p + obj_size <= end; p += obj_size) {
        void* obj = (void*)p;
        hz4_obj_set_next(obj, head);
        head = obj;
        count++;
    }

    page->free = head;
    page->capacity = count;
#if HZ4_MID_FREECOUNT
    page->free_count = count;
#endif
#endif

#if HZ4_PAGE_TAG_TABLE
    // Register page tag for fast lookup (owner_tid=0 for mid)
    uint32_t page_idx;
    if (hz4_pagetag_page_idx(page, &page_idx)) {
        uint32_t tag = hz4_pagetag_encode(HZ4_TAG_KIND_MID, (uint8_t)sc, 0);
        hz4_pagetag_store(page_idx, tag);
    }
#endif

    return page;
}
