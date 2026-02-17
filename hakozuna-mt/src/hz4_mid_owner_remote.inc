// hz4_mid_owner_remote.inc - Mid owner/remote boundary (included from hz4_mid.c)
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX
static __thread hz4_mid_page_t* g_mid_owner_page[HZ4_MID_SC_COUNT];
static __thread uint16_t g_mid_owner_drain_tick[HZ4_MID_SC_COUNT];
static __thread uintptr_t g_hz4_mid_owner_token_anchor;
static __thread int g_hz4_mid_owner_tls_registered;
static pthread_key_t g_hz4_mid_owner_tls_key;
static pthread_once_t g_hz4_mid_owner_tls_once = PTHREAD_ONCE_INIT;

static inline uintptr_t hz4_mid_owner_token(void) {
    return (uintptr_t)&g_hz4_mid_owner_token_anchor;
}

static inline uintptr_t hz4_mid_owner_tag_load(const hz4_mid_page_t* page) {
    return atomic_load_explicit((_Atomic(uintptr_t)*)&page->owner_tag, memory_order_acquire);
}

static inline int hz4_mid_owner_is_self_page(const hz4_mid_page_t* page) {
    return hz4_mid_owner_tag_load(page) == hz4_mid_owner_token();
}

static inline void hz4_mid_owner_tag_store(hz4_mid_page_t* page, uintptr_t tag) {
    atomic_store_explicit(&page->owner_tag, tag, memory_order_release);
}

static inline void hz4_mid_owner_remote_push(hz4_mid_page_t* page, void* obj) {
    void* old = atomic_load_explicit(&page->remote_head, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(obj, old);
        if (atomic_compare_exchange_weak_explicit(
                &page->remote_head, &old, obj, memory_order_release, memory_order_acquire)) {
            break;
        }
    }
    atomic_fetch_add_explicit(&page->remote_count, 1u, memory_order_release);
}

static inline uint32_t hz4_mid_owner_drain_remote_to_local(hz4_mid_page_t* page) {
    void* list = atomic_exchange_explicit(&page->remote_head, NULL, memory_order_acq_rel);
    if (!list) {
        return 0;
    }

    uint32_t n = 0;
    void* tail = NULL;
    void* cur = list;
    uint32_t guard_limit = page->capacity + 8u;
    if (guard_limit < 16u) {
        guard_limit = 16u;
    }
    while (cur) {
        tail = cur;
        cur = hz4_obj_get_next(cur);
        n++;
        if (n > guard_limit) {
            HZ4_FAIL("hz4_mid_owner_drain_remote_to_local: remote list cycle/corruption");
            abort();
        }
    }

    if (tail) {
        hz4_mid_page_freelist_push_list_raw(page, list, tail);
        hz4_mid_page_freelist_push_count(page, n);
    }

    // Best-effort count reconciliation. Exact precision is not required because
    // remote_count is only a drain heuristic; avoid unbounded CAS spinning under
    // heavy concurrent remote pushes.
    uint32_t prev = atomic_load_explicit(&page->remote_count, memory_order_acquire);
    uint32_t next = (prev > n) ? (prev - n) : 0u;
    (void)atomic_compare_exchange_strong_explicit(
        &page->remote_count, &prev, next, memory_order_acq_rel, memory_order_acquire);
    return n;
}

static inline int hz4_mid_owner_should_drain(uint16_t sc, hz4_mid_page_t* page) {
    uint32_t rc = atomic_load_explicit(&page->remote_count, memory_order_acquire);
    if (rc >= (uint32_t)HZ4_MID_OWNER_REMOTE_DRAIN_THRESHOLD) {
        return 1;
    }
    if (rc == 0) {
        return 0;
    }
    uint16_t tick = (uint16_t)(g_mid_owner_drain_tick[sc] + 1u);
    g_mid_owner_drain_tick[sc] = tick;
    return (tick % (uint16_t)HZ4_MID_OWNER_REMOTE_DRAIN_PERIOD) == 0;
}

static inline void hz4_mid_owner_release_page_locked(uint16_t sc, hz4_mid_page_t* page) {
    if (!page) {
        return;
    }
    if (page->magic != HZ4_MID_MAGIC || page->sc != sc) {
        HZ4_FAIL("hz4_mid_owner_release_page_locked: invalid owner page");
        abort();
    }

    uintptr_t self = hz4_mid_owner_token();
    if (hz4_mid_owner_tag_load(page) != self) {
        return;
    }

    // Clear owner first so new frees switch to the lock-protected unowned path.
    hz4_mid_owner_tag_store(page, 0);

    // Drain any pending remote nodes while we still hold the mid lock boundary.
    (void)hz4_mid_owner_drain_remote_to_local(page);

    // Keep released pages reachable from global bins so unowned remote/free traffic
    // can be reclaimed on future lock-path scans.
    hz4_mid_bin_prepend_unique_locked(sc, page);
}

static void hz4_mid_owner_tls_destructor(void* value) {
    (void)value;
#if HZ4_MID_OWNER_LOCAL_STACK_BOX
    uintptr_t self = hz4_mid_owner_token();
#endif
    for (uint32_t i = 0; i < HZ4_MID_SC_COUNT; i++) {
        uint16_t sc = (uint16_t)i;
        hz4_mid_sc_lock_acquire(sc);
#if HZ4_MID_OWNER_LOCAL_STACK_BOX
        hz4_mid_owner_local_stack_flush_sc_locked(sc, self);
#endif
        hz4_mid_page_t* page = g_mid_owner_page[sc];
        if (!page) {
            hz4_mid_sc_lock_release(sc);
            continue;
        }
        hz4_mid_owner_release_page_locked(sc, page);
        hz4_mid_sc_lock_release(sc);
        g_mid_owner_page[sc] = NULL;
        g_mid_owner_drain_tick[sc] = 0;
    }
}

static void hz4_mid_owner_tls_init(void) {
    (void)pthread_key_create(&g_hz4_mid_owner_tls_key, hz4_mid_owner_tls_destructor);
}

static inline void hz4_mid_owner_tls_register_once(void) {
    if (g_hz4_mid_owner_tls_registered) {
        return;
    }
    pthread_once(&g_hz4_mid_owner_tls_once, hz4_mid_owner_tls_init);
    (void)pthread_setspecific(g_hz4_mid_owner_tls_key, (void*)1);
    g_hz4_mid_owner_tls_registered = 1;
}

static inline void hz4_mid_owner_claim_page_locked(uint16_t sc, hz4_mid_page_t* page, hz4_mid_page_t* prev) {
    if (!page) {
        return;
    }
    if (page->magic != HZ4_MID_MAGIC || page->sc != sc) {
        HZ4_FAIL("hz4_mid_owner_claim_page_locked: invalid page");
        abort();
    }

    // Keep owner pages out of global bins to avoid concurrent freelist mutation.
    hz4_mid_bin_remove_locked(sc, page, prev);

    hz4_mid_owner_tag_store(page, hz4_mid_owner_token());
    g_mid_owner_page[sc] = page;
    g_mid_owner_drain_tick[sc] = 0;
}
#endif
