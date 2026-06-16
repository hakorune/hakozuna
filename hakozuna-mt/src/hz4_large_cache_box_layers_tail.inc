#if HZ4_LARGE_LOCK_SHARD_LAYER
static inline int hz4_large_lock_shard_pages_enabled(uint32_t pages) {
    return pages >= (uint32_t)HZ4_LARGE_LOCK_SHARD_MIN_PAGES &&
           pages <= (uint32_t)HZ4_LARGE_LOCK_SHARD_MAX_PAGES;
}

static inline uint32_t hz4_large_lock_shard_self(void) {
    hz4_tls_t* tls = hz4_tls_get();
    return (uint32_t)(tls->owner & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u));
}

static inline uint32_t hz4_large_lock_shard_steal_probe_for_pages(uint32_t pages) {
#if HZ4_S219_LARGE_SPAN_HIBAND
    if (pages <= (uint32_t)HZ4_S219_LARGE_SPAN_PAGES) {
        return (uint32_t)HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2;
    }
#endif
    (void)pages;
    return (uint32_t)HZ4_LARGE_LOCK_SHARD_STEAL_PROBE;
}

static inline uint32_t hz4_large_lock_shard_slots_for_pages(uint32_t pages) {
#if HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX
    if (pages <= (uint32_t)HZ4_S219_LARGE_SPAN_PAGES) {
        return (uint32_t)HZ4_LARGE_LOCK_SHARD_SLOTS_P2;
    }
#else
    (void)pages;
#endif
    return (uint32_t)HZ4_LARGE_LOCK_SHARD_SLOTS;
}

#if HZ4_LARGE_LOCK_SHARD_STICKY_HINT_BOX
static inline int hz4_large_lock_shard_hint_pages_enabled(uint32_t pages) {
    return pages <= (uint32_t)HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES;
}

static inline uint32_t hz4_large_lock_shard_hint_load(uint32_t idx, uint32_t self) {
    if (!g_hz4_large_lock_shard_hint_valid[idx]) {
        return self;
    }
    return ((uint32_t)g_hz4_large_lock_shard_hint_shard[idx]) & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u);
}

static inline void hz4_large_lock_shard_hint_store(uint32_t idx, uint32_t shard) {
    g_hz4_large_lock_shard_hint_shard[idx] = (uint8_t)(shard & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u));
    g_hz4_large_lock_shard_hint_valid[idx] = 1;
}
#else
static inline int hz4_large_lock_shard_hint_pages_enabled(uint32_t pages) {
    (void)pages;
    return 0;
}

static inline uint32_t hz4_large_lock_shard_hint_load(uint32_t idx, uint32_t self) {
    (void)idx;
    return self;
}

static inline void hz4_large_lock_shard_hint_store(uint32_t idx, uint32_t shard) {
    (void)idx;
    (void)shard;
}
#endif

#if HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX
static inline int hz4_large_lock_shard_steal_budget_pages_enabled(uint32_t pages) {
    return pages <= (uint32_t)HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES;
}

static inline void hz4_large_lock_shard_steal_budget_on_success(uint32_t idx, uint32_t pages) {
    if (!hz4_large_lock_shard_steal_budget_pages_enabled(pages)) {
        return;
    }
    g_hz4_large_lock_shard_steal_budget_streak[idx] = 0;
}

static inline int hz4_large_lock_shard_steal_budget_allow(uint32_t idx, uint32_t pages) {
    if (!hz4_large_lock_shard_steal_budget_pages_enabled(pages)) {
        return 1;
    }
    uint8_t cooldown = g_hz4_large_lock_shard_steal_budget_cooldown[idx];
    if (cooldown != 0) {
        g_hz4_large_lock_shard_steal_budget_cooldown[idx] = (uint8_t)(cooldown - 1);
        return 0;
    }
    uint8_t streak = g_hz4_large_lock_shard_steal_budget_streak[idx];
    if (streak < UINT8_MAX) {
        streak = (uint8_t)(streak + 1);
        g_hz4_large_lock_shard_steal_budget_streak[idx] = streak;
    }
    if (streak < (uint8_t)HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK) {
        return 0;
    }
    g_hz4_large_lock_shard_steal_budget_streak[idx] = 0;
    g_hz4_large_lock_shard_steal_budget_cooldown[idx] =
        (uint8_t)HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_COOLDOWN;
    return 1;
}
#else
static inline void hz4_large_lock_shard_steal_budget_on_success(uint32_t idx, uint32_t pages) {
    (void)idx;
    (void)pages;
}

static inline int hz4_large_lock_shard_steal_budget_allow(uint32_t idx, uint32_t pages) {
    (void)idx;
    (void)pages;
    return 1;
}
#endif

#if HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX
static inline int hz4_large_lock_shard_nonempty_mask_pages_enabled(uint32_t pages) {
    return pages <= (uint32_t)HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES;
}

static inline uint64_t hz4_large_lock_shard_nonempty_mask_load(uint32_t idx, uint32_t pages) {
    if (!hz4_large_lock_shard_nonempty_mask_pages_enabled(pages)) {
        return UINT64_MAX;
    }
    return atomic_load_explicit(&g_hz4_large_lock_shard_nonempty_mask[idx], memory_order_acquire);
}

static inline void hz4_large_lock_shard_nonempty_mask_mark(uint32_t idx, uint32_t pages, uint32_t shard) {
    if (!hz4_large_lock_shard_nonempty_mask_pages_enabled(pages)) {
        return;
    }
    uint64_t bit = (UINT64_C(1) << (shard & 63u));
    atomic_fetch_or_explicit(&g_hz4_large_lock_shard_nonempty_mask[idx], bit, memory_order_acq_rel);
}

static inline void hz4_large_lock_shard_nonempty_mask_clear_if_empty(
    uint32_t idx, uint32_t pages, uint32_t shard, uint32_t slots) {
    if (!hz4_large_lock_shard_nonempty_mask_pages_enabled(pages)) {
        return;
    }
    for (uint32_t i = 0; i < slots; i++) {
        void* p = atomic_load_explicit(&g_hz4_large_lock_shard_cache[shard][idx][i], memory_order_acquire);
        if (p != NULL) {
            return;
        }
    }
    uint64_t bit = (UINT64_C(1) << (shard & 63u));
    atomic_fetch_and_explicit(&g_hz4_large_lock_shard_nonempty_mask[idx], ~bit, memory_order_acq_rel);
}
#else
static inline uint64_t hz4_large_lock_shard_nonempty_mask_load(uint32_t idx, uint32_t pages) {
    (void)idx;
    (void)pages;
    return UINT64_MAX;
}

static inline void hz4_large_lock_shard_nonempty_mask_mark(uint32_t idx, uint32_t pages, uint32_t shard) {
    (void)idx;
    (void)pages;
    (void)shard;
}

static inline void hz4_large_lock_shard_nonempty_mask_clear_if_empty(
    uint32_t idx, uint32_t pages, uint32_t shard, uint32_t slots) {
    (void)idx;
    (void)pages;
    (void)shard;
    (void)slots;
}
#endif

static inline void* hz4_large_lock_shard_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_lock_shard_pages_enabled(pages)) {
        return NULL;
    }
    uint32_t steal_probe = hz4_large_lock_shard_steal_probe_for_pages(pages);
    uint32_t slots = hz4_large_lock_shard_slots_for_pages(pages);
    uint32_t idx = pages - 1;
    uint32_t self = hz4_large_lock_shard_self();
    uint32_t hint = self;
    int hint_active = 0;
    uint64_t nonempty_mask = hz4_large_lock_shard_nonempty_mask_load(idx, pages);
    int use_nonempty_mask = (nonempty_mask != UINT64_MAX);
    uint64_t skipped_mask = 0;
#if HZ4_OS_STATS
    int is_p2 = (pages <= (uint32_t)HZ4_S219_LARGE_SPAN_PAGES);
#endif
    if (hz4_large_lock_shard_hint_pages_enabled(pages)) {
        hint = hz4_large_lock_shard_hint_load(idx, self);
        hint_active = (hint != self);
    }
    if (hint_active) {
#if HZ4_OS_STATS
        hz4_os_stats_large_lock_shard_hint_acq_try();
#endif
        for (uint32_t i = 0; i < slots; i++) {
            void* got = atomic_exchange_explicit(&g_hz4_large_lock_shard_cache[hint][idx][i], NULL, memory_order_acq_rel);
            if (got) {
#if HZ4_OS_STATS
                hz4_os_stats_large_lock_shard_hint_acq_hit();
                hz4_os_stats_large_lock_shard_acq_steal_hit();
                if (is_p2) {
                    hz4_os_stats_large_lock_shard_p2_acq_steal_hit();
                }
#endif
                hz4_large_lock_shard_hint_store(idx, hint);
                hz4_large_lock_shard_steal_budget_on_success(idx, pages);
                return got;
            }
        }
        hz4_large_lock_shard_nonempty_mask_clear_if_empty(idx, pages, hint, slots);
    }
    for (uint32_t i = 0; i < slots; i++) {
        void* got = atomic_exchange_explicit(&g_hz4_large_lock_shard_cache[self][idx][i], NULL, memory_order_acq_rel);
        if (got) {
#if HZ4_OS_STATS
            hz4_os_stats_large_lock_shard_acq_self_hit();
            if (is_p2) {
                hz4_os_stats_large_lock_shard_p2_acq_self_hit();
            }
#endif
            hz4_large_lock_shard_hint_store(idx, self);
            hz4_large_lock_shard_steal_budget_on_success(idx, pages);
            return got;
        }
    }
    hz4_large_lock_shard_nonempty_mask_clear_if_empty(idx, pages, self, slots);
    if (steal_probe > 0) {
        int allow_steal = hz4_large_lock_shard_steal_budget_allow(idx, pages);
#if HZ4_OS_STATS
        if (!allow_steal) {
            hz4_os_stats_large_lock_shard_steal_skip_budget();
        }
#endif
        if (allow_steal) {
#if HZ4_OS_STATS
            hz4_os_stats_large_lock_shard_steal_probe();
#endif
            for (uint32_t p = 1; p <= steal_probe; p++) {
                uint32_t shard = (self + p) & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u);
                uint64_t bit = (UINT64_C(1) << (shard & 63u));
                if (hint_active && shard == hint) {
                    continue;
                }
                if (use_nonempty_mask && ((nonempty_mask & bit) == 0)) {
                    skipped_mask |= bit;
#if HZ4_OS_STATS
                    hz4_os_stats_large_lock_shard_mask_skip();
#endif
                    continue;
                }
                for (uint32_t i = 0; i < slots; i++) {
                    void* got = atomic_exchange_explicit(&g_hz4_large_lock_shard_cache[shard][idx][i], NULL, memory_order_acq_rel);
                    if (got) {
#if HZ4_OS_STATS
                        hz4_os_stats_large_lock_shard_acq_steal_hit();
                        hz4_os_stats_large_lock_shard_steal_hit();
                        if (is_p2) {
                            hz4_os_stats_large_lock_shard_p2_acq_steal_hit();
                        }
#endif
                        hz4_large_lock_shard_hint_store(idx, shard);
                        hz4_large_lock_shard_steal_budget_on_success(idx, pages);
                        return got;
                    }
                }
                hz4_large_lock_shard_nonempty_mask_clear_if_empty(idx, pages, shard, slots);
            }
            if (HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_FALLBACK && use_nonempty_mask && skipped_mask != 0) {
#if HZ4_OS_STATS
                hz4_os_stats_large_lock_shard_mask_fallback();
#endif
                for (uint32_t p = 1; p <= steal_probe; p++) {
                    uint32_t shard = (self + p) & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u);
                    uint64_t bit = (UINT64_C(1) << (shard & 63u));
                    if ((skipped_mask & bit) == 0) {
                        continue;
                    }
                    if (hint_active && shard == hint) {
                        continue;
                    }
                    for (uint32_t i = 0; i < slots; i++) {
                        void* got = atomic_exchange_explicit(&g_hz4_large_lock_shard_cache[shard][idx][i], NULL, memory_order_acq_rel);
                        if (got) {
#if HZ4_OS_STATS
                            hz4_os_stats_large_lock_shard_acq_steal_hit();
                            hz4_os_stats_large_lock_shard_steal_hit();
                            hz4_os_stats_large_lock_shard_mask_fallback_hit();
                            if (is_p2) {
                                hz4_os_stats_large_lock_shard_p2_acq_steal_hit();
                            }
#endif
                            hz4_large_lock_shard_hint_store(idx, shard);
                            hz4_large_lock_shard_steal_budget_on_success(idx, pages);
                            return got;
                        }
                    }
                    hz4_large_lock_shard_nonempty_mask_clear_if_empty(idx, pages, shard, slots);
                }
            }
        }
    }
#if HZ4_OS_STATS
    hz4_os_stats_large_lock_shard_acq_miss();
    if (is_p2) {
        hz4_os_stats_large_lock_shard_p2_acq_miss();
    }
#endif
    return NULL;
}

static inline int hz4_large_lock_shard_try_release_seeded(void* base, size_t total, uint32_t seed) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_lock_shard_pages_enabled(pages)) {
        return 0;
    }
    uint32_t steal_probe = hz4_large_lock_shard_steal_probe_for_pages(pages);
    uint32_t slots = hz4_large_lock_shard_slots_for_pages(pages);
    uint32_t idx = pages - 1;
    uint32_t self = seed & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u);
    uint32_t hint = self;
    int hint_active = 0;
#if HZ4_OS_STATS
    int is_p2 = (pages <= (uint32_t)HZ4_S219_LARGE_SPAN_PAGES);
#endif
    if (hz4_large_lock_shard_hint_pages_enabled(pages)) {
        hint = hz4_large_lock_shard_hint_load(idx, self);
        hint_active = (hint != self);
    }
    if (hint_active) {
#if HZ4_OS_STATS
        hz4_os_stats_large_lock_shard_hint_rel_try();
#endif
        for (uint32_t i = 0; i < slots; i++) {
            void* expected = NULL;
            if (atomic_compare_exchange_strong_explicit(
                    &g_hz4_large_lock_shard_cache[hint][idx][i], &expected, base,
                    memory_order_acq_rel, memory_order_acquire)) {
#if HZ4_OS_STATS
                hz4_os_stats_large_lock_shard_hint_rel_hit();
                hz4_os_stats_large_lock_shard_rel_steal_hit();
                if (is_p2) {
                    hz4_os_stats_large_lock_shard_p2_rel_steal_hit();
                }
#endif
                hz4_large_lock_shard_hint_store(idx, hint);
                hz4_large_lock_shard_steal_budget_on_success(idx, pages);
                hz4_large_lock_shard_nonempty_mask_mark(idx, pages, hint);
                return 1;
            }
        }
    }
    for (uint32_t i = 0; i < slots; i++) {
        void* expected = NULL;
        if (atomic_compare_exchange_strong_explicit(
                &g_hz4_large_lock_shard_cache[self][idx][i], &expected, base,
                memory_order_acq_rel, memory_order_acquire)) {
#if HZ4_OS_STATS
            hz4_os_stats_large_lock_shard_rel_self_hit();
            if (is_p2) {
                hz4_os_stats_large_lock_shard_p2_rel_self_hit();
            }
#endif
            hz4_large_lock_shard_hint_store(idx, self);
            hz4_large_lock_shard_steal_budget_on_success(idx, pages);
            hz4_large_lock_shard_nonempty_mask_mark(idx, pages, self);
            return 1;
        }
    }
    if (steal_probe > 0) {
#if HZ4_OS_STATS
        hz4_os_stats_large_lock_shard_steal_probe();
#endif
        for (uint32_t p = 1; p <= steal_probe; p++) {
            uint32_t shard = (self + p) & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1u);
            if (hint_active && shard == hint) {
                continue;
            }
            for (uint32_t i = 0; i < slots; i++) {
                void* expected = NULL;
                if (atomic_compare_exchange_strong_explicit(
                        &g_hz4_large_lock_shard_cache[shard][idx][i], &expected, base,
                        memory_order_acq_rel, memory_order_acquire)) {
#if HZ4_OS_STATS
                    hz4_os_stats_large_lock_shard_rel_steal_hit();
                    hz4_os_stats_large_lock_shard_steal_hit();
                    if (is_p2) {
                        hz4_os_stats_large_lock_shard_p2_rel_steal_hit();
                    }
#endif
                    hz4_large_lock_shard_hint_store(idx, shard);
                    hz4_large_lock_shard_steal_budget_on_success(idx, pages);
                    hz4_large_lock_shard_nonempty_mask_mark(idx, pages, shard);
                    return 1;
                }
            }
        }
    }
#if HZ4_OS_STATS
    hz4_os_stats_large_lock_shard_rel_miss();
    if (is_p2) {
        hz4_os_stats_large_lock_shard_p2_rel_miss();
    }
#endif
    return 0;
}

static inline int hz4_large_lock_shard_try_release(void* base, size_t total) {
    return hz4_large_lock_shard_try_release_seeded(base, total, hz4_large_lock_shard_self());
}

static inline void hz4_large_lock_shard_flush_all(void) {
    for (uint32_t shard = 0; shard < HZ4_LARGE_LOCK_SHARD_SHARDS; shard++) {
        for (uint32_t pages = (uint32_t)HZ4_LARGE_LOCK_SHARD_MIN_PAGES;
             pages <= (uint32_t)HZ4_LARGE_LOCK_SHARD_MAX_PAGES; pages++) {
            uint32_t slots = hz4_large_lock_shard_slots_for_pages(pages);
            uint32_t idx = pages - 1;
            size_t size = ((size_t)pages << HZ4_PAGE_SHIFT);
            for (uint32_t i = 0; i < slots; i++) {
                void* base = atomic_exchange_explicit(
                    &g_hz4_large_lock_shard_cache[shard][idx][i], NULL, memory_order_acq_rel);
                if (base) {
                    hz4_os_large_release(base, size);
                }
            }
        }
    }
}
#endif

#if HZ4_LARGE_OVERFLOW_TLS_BOX
// Per-thread overflow cache, used when global cache for this page-count is full.
static __thread void* g_hz4_large_overflow_tls[HZ4_LARGE_CACHE_MAX_PAGES][HZ4_LARGE_OVERFLOW_TLS_SLOTS];
static __thread uint8_t g_hz4_large_overflow_tls_top[HZ4_LARGE_CACHE_MAX_PAGES];

static inline void* hz4_large_overflow_tls_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        return NULL;
    }
    uint32_t idx = pages - 1;
    uint8_t top = g_hz4_large_overflow_tls_top[idx];
    if (top == 0) {
        return NULL;
    }
    void* base = g_hz4_large_overflow_tls[idx][top - 1];
    g_hz4_large_overflow_tls[idx][top - 1] = NULL;
    g_hz4_large_overflow_tls_top[idx] = (uint8_t)(top - 1);
    return base;
}

static inline int hz4_large_overflow_tls_try_release(void* base, size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_CACHE_MAX_PAGES) {
        return 0;
    }
    uint32_t idx = pages - 1;
    uint8_t top = g_hz4_large_overflow_tls_top[idx];
    if (top >= HZ4_LARGE_OVERFLOW_TLS_SLOTS) {
        return 0;
    }
    g_hz4_large_overflow_tls[idx][top] = base;
    g_hz4_large_overflow_tls_top[idx] = (uint8_t)(top + 1);
    return 1;
}

static inline void hz4_large_overflow_tls_flush_all(void) {
    for (uint32_t pages = 1; pages <= HZ4_LARGE_CACHE_MAX_PAGES; pages++) {
        uint32_t idx = pages - 1;
        while (g_hz4_large_overflow_tls_top[idx] > 0) {
            uint8_t top = g_hz4_large_overflow_tls_top[idx];
            void* base = g_hz4_large_overflow_tls[idx][top - 1];
            g_hz4_large_overflow_tls[idx][top - 1] = NULL;
            g_hz4_large_overflow_tls_top[idx] = (uint8_t)(top - 1);
            if (base) {
                hz4_os_large_release(base, ((size_t)pages << HZ4_PAGE_SHIFT));
            }
        }
    }
}
#endif

#if HZ4_LARGE_HOT_CACHE_LAYER
static inline void* hz4_large_hot_cache_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_HOT_CACHE_MAX_PAGES) {
        return NULL;
    }
    uint32_t idx = pages - 1;
    for (int i = 0; i < HZ4_LARGE_HOT_CACHE_SLOTS; i++) {
        void* got = atomic_exchange_explicit(&g_hz4_large_hot_cache[idx][i], NULL, memory_order_acq_rel);
        if (got) {
            atomic_fetch_sub_explicit(&g_hz4_large_hot_cache_bytes, total, memory_order_relaxed);
            return got;
        }
    }
    return NULL;
}

static inline int hz4_large_hot_cache_try_release(void* base, size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_HOT_CACHE_MAX_PAGES) {
        return 0;
    }
    size_t have = atomic_load_explicit(&g_hz4_large_hot_cache_bytes, memory_order_relaxed);
    if (have >= (size_t)HZ4_LARGE_HOT_CACHE_MAX_BYTES ||
        (size_t)HZ4_LARGE_HOT_CACHE_MAX_BYTES - have < total) {
        return 0;
    }
    uint32_t idx = pages - 1;
    for (int i = 0; i < HZ4_LARGE_HOT_CACHE_SLOTS; i++) {
        void* expected = NULL;
        if (atomic_compare_exchange_strong_explicit(
                &g_hz4_large_hot_cache[idx][i], &expected, base,
                memory_order_acq_rel, memory_order_acquire)) {
            atomic_fetch_add_explicit(&g_hz4_large_hot_cache_bytes, total, memory_order_relaxed);
            return 1;
        }
    }
    return 0;
}
#endif
