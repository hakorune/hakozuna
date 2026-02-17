// hz4_mid_batch_cache.inc - Mid batch-cache boundary (included from hz4_mid.c)
#if HZ4_MID_FREE_BATCH_BOX
static __thread void* g_mid_free_batch_head[HZ4_MID_SC_COUNT];
static __thread uint16_t g_mid_free_batch_n[HZ4_MID_SC_COUNT];
static __thread int g_hz4_mid_free_batch_tls_registered;
static pthread_key_t g_hz4_mid_free_batch_tls_key;
static pthread_once_t g_hz4_mid_free_batch_tls_once = PTHREAD_ONCE_INIT;

static inline void hz4_mid_free_batch_flush_sc_locked(uint16_t sc) {
    void* list = g_mid_free_batch_head[sc];
    if (!list) {
        return;
    }
    uint16_t n = g_mid_free_batch_n[sc];
    g_mid_free_batch_head[sc] = NULL;
    g_mid_free_batch_n[sc] = 0;
    uint32_t flushed = 0;
    uint32_t guard_limit = (uint32_t)n + 8u;
    if (guard_limit < 16u) {
        guard_limit = 16u;
    }

#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_free_batch_flush);
    hz4_mid_stats_add(&g_hz4_mid_stats_free_batch_flush_objs, n);
#else
    (void)n;
#endif

    while (list) {
        void* obj = list;
        list = hz4_obj_get_next(obj);
        flushed++;
        if (flushed > guard_limit) {
            HZ4_FAIL("hz4_mid_free_batch_flush_sc_locked: batch list cycle/corruption");
            abort();
        }
        hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)obj & ~(HZ4_PAGE_SIZE - 1));
        if (page->magic != HZ4_MID_MAGIC) {
            HZ4_FAIL("hz4_mid_free_batch_flush_sc_locked: invalid page");
            abort();
        }
        if (page->sc != sc) {
            HZ4_FAIL("hz4_mid_free_batch_flush_sc_locked: size class mismatch");
            abort();
        }

#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
        uintptr_t owner = hz4_mid_owner_tag_load(page);
        if (owner != 0 && owner != hz4_mid_owner_token()) {
            hz4_mid_owner_remote_push(page, obj);
            continue;
        }
#endif

        if (hz4_mid_page_freelist_empty(page)) {
            hz4_mid_bin_prepend_unique_locked(sc, page);
        }
        hz4_mid_page_freelist_push_raw(page, obj);
        hz4_mid_page_freelist_push_count(page, 1);
    }
}

static void hz4_mid_free_batch_flush_all(void) {
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint16_t sc = (uint16_t)i;
        if (!g_mid_free_batch_head[sc]) {
            continue;
        }
        hz4_mid_sc_lock_acquire(sc);
        hz4_mid_free_batch_flush_sc_locked(sc);
        hz4_mid_sc_lock_release(sc);
    }
}

static void hz4_mid_free_batch_tls_destructor(void* value) {
    (void)value;
    hz4_mid_free_batch_flush_all();
}

static void hz4_mid_free_batch_tls_init(void) {
    (void)pthread_key_create(&g_hz4_mid_free_batch_tls_key, hz4_mid_free_batch_tls_destructor);
}

static inline void hz4_mid_free_batch_tls_register_once(void) {
    if (g_hz4_mid_free_batch_tls_registered) {
        return;
    }
    pthread_once(&g_hz4_mid_free_batch_tls_once, hz4_mid_free_batch_tls_init);
    (void)pthread_setspecific(g_hz4_mid_free_batch_tls_key, (void*)1);
    g_hz4_mid_free_batch_tls_registered = 1;
}

static inline void hz4_mid_free_batch_push(uint16_t sc, void* ptr) {
    hz4_mid_free_batch_tls_register_once();
#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_free_batch_enq);
#endif
    hz4_obj_set_next(ptr, g_mid_free_batch_head[sc]);
    g_mid_free_batch_head[sc] = ptr;
    uint16_t n = (uint16_t)(g_mid_free_batch_n[sc] + 1u);
    g_mid_free_batch_n[sc] = n;
    if (n >= HZ4_MID_FREE_BATCH_LIMIT) {
        hz4_mid_sc_lock_acquire(sc);
        hz4_mid_free_batch_flush_sc_locked(sc);
        hz4_mid_sc_lock_release(sc);
    }
}

#endif

#if HZ4_MID_ALLOC_RUN_CACHE_BOX
static __thread void* g_mid_alloc_run_head[HZ4_MID_SC_COUNT];
static __thread uint16_t g_mid_alloc_run_n[HZ4_MID_SC_COUNT];
static __thread int g_hz4_mid_alloc_run_tls_registered;
static pthread_key_t g_hz4_mid_alloc_run_tls_key;
static pthread_once_t g_hz4_mid_alloc_run_tls_once = PTHREAD_ONCE_INIT;

static inline void hz4_mid_alloc_run_flush_sc_locked(uint16_t sc) {
    void* list = g_mid_alloc_run_head[sc];
    if (!list) {
        return;
    }
    uint16_t n = g_mid_alloc_run_n[sc];
    g_mid_alloc_run_head[sc] = NULL;
    g_mid_alloc_run_n[sc] = 0;
    uint32_t flushed = 0;
    uint32_t guard_limit = (uint32_t)n + 8u;
    if (guard_limit < 16u) {
        guard_limit = 16u;
    }

    while (list) {
        void* obj = list;
        list = hz4_obj_get_next(obj);
        flushed++;
        if (flushed > guard_limit) {
            HZ4_FAIL("hz4_mid_alloc_run_flush_sc_locked: alloc-run list cycle/corruption");
            abort();
        }
        hz4_mid_page_t* page = (hz4_mid_page_t*)((uintptr_t)obj & ~(HZ4_PAGE_SIZE - 1));
        if (page->magic != HZ4_MID_MAGIC) {
            HZ4_FAIL("hz4_mid_alloc_run_flush_sc_locked: invalid page");
            abort();
        }
        if (page->sc != sc) {
            HZ4_FAIL("hz4_mid_alloc_run_flush_sc_locked: size class mismatch");
            abort();
        }

#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
        uintptr_t owner = hz4_mid_owner_tag_load(page);
        if (owner != 0 && owner != hz4_mid_owner_token()) {
            hz4_mid_owner_remote_push(page, obj);
            continue;
        }
#endif

        if (hz4_mid_page_freelist_empty(page)) {
            hz4_mid_bin_prepend_unique_locked(sc, page);
        }
        hz4_mid_page_freelist_push_raw(page, obj);
        hz4_mid_page_freelist_push_count(page, 1);
    }
}

static void hz4_mid_alloc_run_flush_all(void) {
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint16_t sc = (uint16_t)i;
        if (!g_mid_alloc_run_head[sc]) {
            continue;
        }
        hz4_mid_sc_lock_acquire(sc);
        hz4_mid_alloc_run_flush_sc_locked(sc);
        hz4_mid_sc_lock_release(sc);
    }
}

static void hz4_mid_alloc_run_tls_destructor(void* value) {
    (void)value;
    hz4_mid_alloc_run_flush_all();
}

static void hz4_mid_alloc_run_tls_init(void) {
    (void)pthread_key_create(&g_hz4_mid_alloc_run_tls_key, hz4_mid_alloc_run_tls_destructor);
}

static inline void hz4_mid_alloc_run_tls_register_once(void) {
    if (g_hz4_mid_alloc_run_tls_registered) {
        return;
    }
    pthread_once(&g_hz4_mid_alloc_run_tls_once, hz4_mid_alloc_run_tls_init);
    (void)pthread_setspecific(g_hz4_mid_alloc_run_tls_key, (void*)1);
    g_hz4_mid_alloc_run_tls_registered = 1;
}

static inline void* hz4_mid_alloc_run_pop(uint16_t sc) {
    void* obj = g_mid_alloc_run_head[sc];
    if (!obj) {
        return NULL;
    }
    g_mid_alloc_run_head[sc] = hz4_obj_get_next(obj);
    uint16_t n = g_mid_alloc_run_n[sc];
    if (n > 0) {
        g_mid_alloc_run_n[sc] = (uint16_t)(n - 1);
    }
    HZ4_MID_CLEAR_NEXT(obj);
#if HZ4_MID_STATS_B1
    hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_alloc_cache_hit);
#endif
    return obj;
}

static inline void hz4_mid_alloc_run_refill_from_page_locked(uint16_t sc, hz4_mid_page_t* page) {
    if (!page) {
        return;
    }
    uint16_t n = g_mid_alloc_run_n[sc];
    if (n >= HZ4_MID_ALLOC_RUN_CACHE_LIMIT) {
        return;
    }
    if (!HZ4_MID_PAGE_HAS_FREE(page)) {
        return;
    }
    hz4_mid_alloc_run_tls_register_once();

    uint16_t added = 0;
    while (n < HZ4_MID_ALLOC_RUN_CACHE_LIMIT && HZ4_MID_PAGE_HAS_FREE(page)) {
        void* obj = hz4_mid_page_pop(page);
        if (!obj) {
            break;
        }
        hz4_obj_set_next(obj, g_mid_alloc_run_head[sc]);
        g_mid_alloc_run_head[sc] = obj;
        n++;
        added++;
    }
    g_mid_alloc_run_n[sc] = n;

#if HZ4_MID_STATS_B1
    if (added) {
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_alloc_cache_refill);
        hz4_mid_stats_add(&g_hz4_mid_stats_malloc_alloc_cache_refill_objs, added);
    }
#endif
}

#if HZ4_MID_LOCK_BURST_REFILL_BOX
// MidLockBurstRefillBox:
// During lock-path allocation, refill alloc-run cache from additional bin pages
// with bounded scan depth. This keeps lock-path work localized and reversible.
static inline uint16_t hz4_mid_alloc_run_refill_burst_locked(uint16_t sc, hz4_mid_page_t* skip_page) {
    uint16_t n = g_mid_alloc_run_n[sc];
    if (n >= HZ4_MID_ALLOC_RUN_CACHE_LIMIT) {
        return 0;
    }
    hz4_mid_alloc_run_tls_register_once();

    uint16_t added = 0;
    uint16_t scanned = 0;
    hz4_mid_page_t* prev = NULL;
    hz4_mid_page_t* page = g_hz4_mid_bins[sc];

    while (page && scanned < HZ4_MID_LOCK_BURST_SCAN_PAGES && n < HZ4_MID_ALLOC_RUN_CACHE_LIMIT) {
        hz4_mid_page_t* next = page->next;
        if (page == skip_page) {
            prev = page;
            page = next;
            continue;
        }
        scanned++;

        while (n < HZ4_MID_ALLOC_RUN_CACHE_LIMIT && HZ4_MID_PAGE_HAS_FREE(page)) {
            void* obj = hz4_mid_page_pop(page);
            if (!obj) {
                break;
            }
            hz4_obj_set_next(obj, g_mid_alloc_run_head[sc]);
            g_mid_alloc_run_head[sc] = obj;
            n++;
            added++;
        }

        int empty = hz4_mid_page_freelist_empty(page);
        if (empty) {
            hz4_mid_bin_remove_locked(sc, page, prev);
        } else {
            prev = page;
        }
        page = next;
    }

    g_mid_alloc_run_n[sc] = n;

#if HZ4_MID_STATS_B1
    if (added) {
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_lock_burst_calls);
        hz4_mid_stats_add(&g_hz4_mid_stats_malloc_lock_burst_objs, added);
    }
#endif
    return added;
}
#endif

#if HZ4_MID_FREE_BATCH_BOX && HZ4_MID_FREE_BATCH_CONSUME_BOX
static inline uint16_t hz4_mid_alloc_run_refill_from_free_batch(uint16_t sc) {
    uint16_t free_n = g_mid_free_batch_n[sc];
    if (free_n < HZ4_MID_FREE_BATCH_CONSUME_MIN) {
        return 0;
    }

    uint16_t n = g_mid_alloc_run_n[sc];
    if (n >= HZ4_MID_ALLOC_RUN_CACHE_LIMIT) {
        return 0;
    }

    hz4_mid_free_batch_tls_register_once();
    hz4_mid_alloc_run_tls_register_once();

    uint16_t added = 0;
    while (n < HZ4_MID_ALLOC_RUN_CACHE_LIMIT) {
        void* obj = g_mid_free_batch_head[sc];
        if (!obj) {
            break;
        }
        g_mid_free_batch_head[sc] = hz4_obj_get_next(obj);
        if (free_n > 0) {
            free_n--;
        }
        hz4_obj_set_next(obj, g_mid_alloc_run_head[sc]);
        g_mid_alloc_run_head[sc] = obj;
        n++;
        added++;
    }

    g_mid_free_batch_n[sc] = free_n;
    g_mid_alloc_run_n[sc] = n;

#if HZ4_MID_STATS_B1
    if (added) {
        hz4_mid_stats_inc(&g_hz4_mid_stats_malloc_free_batch_refill);
        hz4_mid_stats_add(&g_hz4_mid_stats_malloc_free_batch_refill_objs, added);
    }
#endif
    return added;
}
#endif
#endif
