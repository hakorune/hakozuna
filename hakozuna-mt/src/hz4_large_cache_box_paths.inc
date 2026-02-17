// Extracted from hz4_large_paths.inc (move-only): cache helpers and rescue gate.

static inline void hz4_large_cache_flush_global_all(void) {
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX
    hz4_large_pagebin_batch_flush_all();
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
    for (uint32_t pages = 1; pages <= HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES; pages++) {
        uint32_t idx = pages - 1;
        size_t size = ((size_t)pages << HZ4_PAGE_SHIFT);
        for (uint32_t i = 0; i < (uint32_t)HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS; i++) {
            void* base = atomic_exchange_explicit(&g_hz4_large_pagebin_cache[idx][i], NULL, memory_order_acq_rel);
            if (base) {
                hz4_os_large_release(base, size);
            }
        }
    }
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER
    hz4_large_lock_shard_flush_all();
#endif
#if HZ4_LARGE_CACHE_SHARDBOX
    for (uint32_t shard = 0; shard < HZ4_LARGE_CACHE_SHARDS; shard++) {
        for (uint32_t pages = 1; pages <= HZ4_LARGE_CACHE_MAX_PAGES; pages++) {
            uint32_t idx = pages - 1;
            size_t size = ((size_t)pages << HZ4_PAGE_SHIFT);
            for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
                void* base = atomic_exchange_explicit(&g_hz4_large_cache[shard][idx][i], NULL, memory_order_acq_rel);
                if (base) {
                    hz4_os_large_release(base, size);
                }
            }
        }
    }
#else
    for (uint32_t pages = 1; pages <= HZ4_LARGE_CACHE_MAX_PAGES; pages++) {
        uint32_t idx = pages - 1;
        size_t size = ((size_t)pages << HZ4_PAGE_SHIFT);
        for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
            void* base = atomic_exchange_explicit(&g_hz4_large_cache[idx][i], NULL, memory_order_acq_rel);
            if (base) {
                hz4_os_large_release(base, size);
            }
        }
    }
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER
    for (uint32_t pages = 1; pages <= HZ4_LARGE_HOT_CACHE_MAX_PAGES; pages++) {
        uint32_t idx = pages - 1;
        size_t size = ((size_t)pages << HZ4_PAGE_SHIFT);
        for (int i = 0; i < HZ4_LARGE_HOT_CACHE_SLOTS; i++) {
            void* base = atomic_exchange_explicit(&g_hz4_large_hot_cache[idx][i], NULL, memory_order_acq_rel);
            if (base) {
                atomic_fetch_sub_explicit(&g_hz4_large_hot_cache_bytes, size, memory_order_relaxed);
                hz4_os_large_release(base, size);
            }
        }
    }
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX
    hz4_large_extent_cache_flush_all();
#endif
}

static inline void* hz4_large_cache_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0) {
        return NULL;
    }
#if HZ4_LARGE_EXTENT_CACHE_BOX
    if (pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        return hz4_large_extent_cache_try_acquire(total);
    }
#else
    if (pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        return NULL;
    }
#endif
    uint32_t idx = pages - 1;
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
    {
        void* got = hz4_large_pagebin_try_acquire(total);
        if (got) {
            return got;
        }
    }
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER
    {
        void* got = hz4_large_lock_shard_try_acquire(total);
        if (got) {
            return got;
        }
    }
#endif
#if HZ4_LARGE_CACHE_SHARDBOX
    uint32_t self = hz4_large_cache_self_shard();
    for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
        void* got = atomic_exchange_explicit(&g_hz4_large_cache[self][idx][i], NULL, memory_order_acq_rel);
        if (got) {
            return got;
        }
    }
#if HZ4_LARGE_OVERFLOW_TLS_BOX
    {
        void* got = hz4_large_overflow_tls_try_acquire(total);
        if (got) {
            return got;
        }
    }
#endif
    for (uint32_t p = 1; p <= HZ4_LARGE_CACHE_SHARD_STEAL_PROBE; p++) {
        uint32_t shard = (self + p) & (HZ4_LARGE_CACHE_SHARDS - 1u);
        for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
            void* got = atomic_exchange_explicit(&g_hz4_large_cache[shard][idx][i], NULL, memory_order_acq_rel);
            if (got) {
                return got;
            }
        }
    }
#if HZ4_LARGE_HOT_CACHE_LAYER
    {
        void* got = hz4_large_hot_cache_try_acquire(total);
        if (got) {
            return got;
        }
    }
#endif
    return NULL;
#else
    for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
        void* got = atomic_exchange_explicit(&g_hz4_large_cache[idx][i], NULL, memory_order_acq_rel);
        if (got) {
            return got;
        }
    }
#if HZ4_LARGE_HOT_CACHE_LAYER
    {
        void* got = hz4_large_hot_cache_try_acquire(total);
        if (got) {
            return got;
        }
    }
#endif
#if HZ4_LARGE_OVERFLOW_TLS_BOX
    return hz4_large_overflow_tls_try_acquire(total);
#else
    return NULL;
#endif
#endif
}

static inline int hz4_large_cache_try_release_with_policy(
    void* base, size_t total, int allow_overflow_tls, int release_on_full) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0) {
        return 0;
    }
#if HZ4_LARGE_EXTENT_CACHE_BOX
    if (pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        return hz4_large_extent_cache_try_release(base, total);
    }
#else
    if (pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        return 0;
    }
#endif
    uint32_t idx = pages - 1;
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
    if (hz4_large_pagebin_try_release(base, total)) {
        return 1;
    }
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER
    if (hz4_large_lock_shard_try_release(base, total)) {
        return 1;
    }
#endif
#if HZ4_LARGE_CACHE_SHARDBOX
    uint32_t self = hz4_large_cache_self_shard();
    for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
        void* prev = atomic_exchange_explicit(&g_hz4_large_cache[self][idx][i], base, memory_order_acq_rel);
        if (prev == NULL) {
            return 1;  // stored successfully
        }
        base = prev;  // keep displaced block and try next slot
    }
#else
    for (int i = 0; i < HZ4_LARGE_CACHE_SLOTS; i++) {
        void* prev = atomic_exchange_explicit(&g_hz4_large_cache[idx][i], base, memory_order_acq_rel);
        if (prev == NULL) {
            return 1;  // stored successfully
        }
        base = prev;  // keep displaced block and try next slot
    }
#endif
#if HZ4_LARGE_OVERFLOW_TLS_BOX
    if (allow_overflow_tls) {
        if (hz4_large_overflow_tls_try_release(base, ((size_t)pages << HZ4_PAGE_SHIFT))) {
            return 1;
        }
    }
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER
    if (hz4_large_hot_cache_try_release(base, ((size_t)pages << HZ4_PAGE_SHIFT))) {
        return 1;
    }
#endif
    // cache full for this page-count; release the final displaced block.
    hz4_os_large_release(base, ((size_t)pages << HZ4_PAGE_SHIFT));
    if (release_on_full) {
        return 1;
    }
    return 0;
}

static inline int hz4_large_cache_try_release(void* base, size_t total) {
    return hz4_large_cache_try_release_with_policy(base, total, 1, 1);
}

#if HZ4_LARGE_FAIL_RESCUE_BOX
static inline void* hz4_large_cache_try_acquire_rescue(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0 || pages >= HZ4_LARGE_CACHE_MAX_PAGES) {
        return NULL;
    }
    uint32_t max_pages = pages + (uint32_t)HZ4_LARGE_FAIL_RESCUE_SCAN_PAGES;
    if (max_pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        max_pages = HZ4_LARGE_CACHE_MAX_PAGES;
    }
    for (uint32_t probe_pages = pages + 1; probe_pages <= max_pages; probe_pages++) {
        size_t probe_total = ((size_t)probe_pages << HZ4_PAGE_SHIFT);
        void* base = hz4_large_cache_try_acquire(probe_total);
        if (!base) {
            continue;
        }
        size_t tail = probe_total - total;
        if (tail != 0) {
            void* tail_base = (void*)((uint8_t*)base + total);
            if (!hz4_large_cache_try_release(tail_base, tail)) {
                hz4_os_large_release(tail_base, tail);
            }
        }
        return base;
    }
    return NULL;
}
#endif

#if HZ4_LARGE_FAIL_RESCUE_BOX && HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
static inline int hz4_large_rescue_gate_allow(uint32_t fail_streak) {
    if (fail_streak >= (uint32_t)HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK) {
        hz4_os_stats_large_rescue_gate_force();
        return 1;
    }
    if (g_hz4_large_rescue_gate_cooldown_tls > 0) {
        g_hz4_large_rescue_gate_cooldown_tls--;
        hz4_os_stats_large_rescue_gate_skip();
        return 0;
    }
    uint32_t attempts = g_hz4_large_rescue_gate_attempts_tls;
    uint32_t success = g_hz4_large_rescue_gate_success_tls;
    if (attempts >= (uint32_t)HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW &&
        success * 100u < attempts * (uint32_t)HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT) {
        g_hz4_large_rescue_gate_cooldown_tls = (uint16_t)HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS;
        hz4_os_stats_large_rescue_gate_skip();
        return 0;
    }
    return 1;
}

static inline void hz4_large_rescue_gate_record(int success) {
    uint32_t attempts = (uint32_t)g_hz4_large_rescue_gate_attempts_tls + 1u;
    uint32_t hits = (uint32_t)g_hz4_large_rescue_gate_success_tls + (success ? 1u : 0u);
    if (attempts > (uint32_t)HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW) {
        attempts = (attempts + 1u) >> 1;
        hits = (hits + 1u) >> 1;
    }
    g_hz4_large_rescue_gate_attempts_tls = (uint16_t)attempts;
    g_hz4_large_rescue_gate_success_tls = (uint16_t)hits;
    if (success) {
        g_hz4_large_rescue_gate_cooldown_tls = 0;
    }
}
#endif
