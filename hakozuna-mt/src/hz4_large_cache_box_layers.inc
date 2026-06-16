// Extracted from hz4_large_paths.inc (move-only): large cache layers.
// Global per-page-count LIFO cache (cross-thread friendly).
#if HZ4_LARGE_CACHE_SHARDBOX
static _Atomic(void*) g_hz4_large_cache[HZ4_LARGE_CACHE_SHARDS][HZ4_LARGE_CACHE_MAX_PAGES][HZ4_LARGE_CACHE_SLOTS] = {0};
#else
static _Atomic(void*) g_hz4_large_cache[HZ4_LARGE_CACHE_MAX_PAGES][HZ4_LARGE_CACHE_SLOTS] = {0};
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX
// LargeExtentCacheBox (B16): bounded cache for > HZ4_LARGE_CACHE_MAX_PAGES bands.
enum {
    HZ4_LARGE_EXTENT_CACHE_RANGE_PAGES =
        (HZ4_LARGE_EXTENT_CACHE_MAX_PAGES - HZ4_LARGE_EXTENT_CACHE_MIN_PAGES + 1)
};
static _Atomic(void*)
    g_hz4_large_extent_cache[HZ4_LARGE_EXTENT_CACHE_RANGE_PAGES][HZ4_LARGE_EXTENT_CACHE_SLOTS] = {0};
static _Atomic(size_t) g_hz4_large_extent_cache_bytes = 0;
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
// LargePageBinLocklessBox: unsharded fast page-bin for low-page large bands.
static _Atomic(void*)
    g_hz4_large_pagebin_cache[HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES][HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS] = {0};
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX
// LargePageBinBatchSwapBox (B15): batch publish/acquire to reduce shared atomic frequency.
static _Atomic(void*) g_hz4_large_pagebin_batch_head[HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES] = {0};
static __thread void* g_hz4_large_pagebin_batch_stash[HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES];
static __thread void* g_hz4_large_pagebin_batch_relbuf[HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES][HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N];
static __thread uint8_t g_hz4_large_pagebin_batch_relbuf_n[HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES];
#endif
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER
// S218-C2: sharded front-layer cache for selected large page bands.
static _Atomic(void*) g_hz4_large_lock_shard_cache[HZ4_LARGE_LOCK_SHARD_SHARDS][HZ4_LARGE_CACHE_MAX_PAGES][HZ4_LARGE_LOCK_SHARD_SLOTS_MAX] = {0};
#if HZ4_LARGE_LOCK_SHARD_STICKY_HINT_BOX
// Sticky shard hint by page-band (TLS).
static __thread uint8_t g_hz4_large_lock_shard_hint_shard[HZ4_LARGE_CACHE_MAX_PAGES];
static __thread uint8_t g_hz4_large_lock_shard_hint_valid[HZ4_LARGE_CACHE_MAX_PAGES];
#endif
#if HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX
// Steal budget/cooldown state by page-band (TLS).
static __thread uint8_t g_hz4_large_lock_shard_steal_budget_streak[HZ4_LARGE_CACHE_MAX_PAGES];
static __thread uint8_t g_hz4_large_lock_shard_steal_budget_cooldown[HZ4_LARGE_CACHE_MAX_PAGES];
#endif
#if HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX
// Approximate non-empty shard mask by page-band (global).
static _Atomic(uint64_t) g_hz4_large_lock_shard_nonempty_mask[HZ4_LARGE_CACHE_MAX_PAGES] = {0};
#endif
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER
// Extra global cache layer dedicated to hot large bands (64KB/128KB by default).
static _Atomic(void*) g_hz4_large_hot_cache[HZ4_LARGE_HOT_CACHE_MAX_PAGES][HZ4_LARGE_HOT_CACHE_SLOTS] = {0};
static _Atomic(size_t) g_hz4_large_hot_cache_bytes = 0;
#endif
#if HZ4_LARGE_FAIL_RESCUE_BOX
// S218-B4: trigger rescue only after consecutive mmap failures.
static __thread uint32_t g_hz4_large_fail_streak_tls = 0;
#if HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
// S218-B7: rescue precision gate state (recent success ratio + cooldown).
static __thread uint16_t g_hz4_large_rescue_gate_attempts_tls = 0;
static __thread uint16_t g_hz4_large_rescue_gate_success_tls = 0;
static __thread uint16_t g_hz4_large_rescue_gate_cooldown_tls = 0;
#endif
// S218-B5: budgeted rescue is archived NO-GO.
// Keep state only when non-default archived knobs are explicitly enabled.
#if (HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL > 1) || (HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF > 1)
static __thread uint32_t g_hz4_large_rescue_budget_tls = 0;  // failures since last rescue
static __thread uint32_t g_hz4_large_rescue_interval_tls = HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL;  // current interval
#endif
#endif

static inline uint32_t hz4_large_cache_pages(size_t total) {
    return (uint32_t)(total >> HZ4_PAGE_SHIFT);
}

#if HZ4_LARGE_EXTENT_CACHE_BOX
static inline int hz4_large_extent_cache_pages_enabled(uint32_t pages) {
    return pages >= (uint32_t)HZ4_LARGE_EXTENT_CACHE_MIN_PAGES &&
           pages <= (uint32_t)HZ4_LARGE_EXTENT_CACHE_MAX_PAGES;
}

static inline uint32_t hz4_large_extent_cache_idx(uint32_t pages) {
    return pages - (uint32_t)HZ4_LARGE_EXTENT_CACHE_MIN_PAGES;
}

static inline size_t hz4_large_extent_cache_total_for_pages(uint32_t pages) {
    return ((size_t)pages << HZ4_PAGE_SHIFT);
}

static inline int hz4_large_extent_cache_try_reserve_bytes(size_t bytes) {
    size_t cap = (size_t)HZ4_LARGE_EXTENT_CACHE_MAX_BYTES;
    size_t cur = atomic_load_explicit(&g_hz4_large_extent_cache_bytes, memory_order_relaxed);
    for (;;) {
        if (cur >= cap || cap - cur < bytes) {
            return 0;
        }
        size_t next = cur + bytes;
        if (atomic_compare_exchange_weak_explicit(
                &g_hz4_large_extent_cache_bytes, &cur, next,
                memory_order_acq_rel, memory_order_relaxed)) {
            hz4_os_stats_large_extent_b16_bytes_peak(next);
            return 1;
        }
    }
}

static inline void hz4_large_extent_cache_release_reserved_bytes(size_t bytes) {
    atomic_fetch_sub_explicit(&g_hz4_large_extent_cache_bytes, bytes, memory_order_relaxed);
}

static inline void* hz4_large_extent_cache_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_extent_cache_pages_enabled(pages)) {
        return NULL;
    }
    uint32_t idx = hz4_large_extent_cache_idx(pages);
    size_t size = hz4_large_extent_cache_total_for_pages(pages);
    for (int i = 0; i < HZ4_LARGE_EXTENT_CACHE_SLOTS; i++) {
        void* got = atomic_exchange_explicit(&g_hz4_large_extent_cache[idx][i], NULL, memory_order_acq_rel);
        if (got) {
            hz4_large_extent_cache_release_reserved_bytes(size);
            hz4_os_stats_large_extent_b16_acq_hit();
            return got;
        }
    }
    hz4_os_stats_large_extent_b16_acq_miss();
    return NULL;
}

static inline int hz4_large_extent_cache_try_release(void* base, size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_extent_cache_pages_enabled(pages)) {
        return 0;
    }
    size_t size = hz4_large_extent_cache_total_for_pages(pages);
    if (!hz4_large_extent_cache_try_reserve_bytes(size)) {
        hz4_os_stats_large_extent_b16_rel_drop_cap();
        hz4_os_stats_large_extent_b16_rel_miss();
        return 0;
    }

    uint32_t idx = hz4_large_extent_cache_idx(pages);
    for (int i = 0; i < HZ4_LARGE_EXTENT_CACHE_SLOTS; i++) {
        void* expected = NULL;
        if (atomic_compare_exchange_strong_explicit(
                &g_hz4_large_extent_cache[idx][i], &expected, base,
                memory_order_acq_rel, memory_order_acquire)) {
            hz4_os_stats_large_extent_b16_rel_hit();
            return 1;
        }
    }

    hz4_large_extent_cache_release_reserved_bytes(size);
    hz4_os_stats_large_extent_b16_rel_miss();
    return 0;
}

static inline void hz4_large_extent_cache_flush_all(void) {
    for (uint32_t pages = (uint32_t)HZ4_LARGE_EXTENT_CACHE_MIN_PAGES;
         pages <= (uint32_t)HZ4_LARGE_EXTENT_CACHE_MAX_PAGES; pages++) {
        uint32_t idx = hz4_large_extent_cache_idx(pages);
        size_t size = hz4_large_extent_cache_total_for_pages(pages);
        for (int i = 0; i < HZ4_LARGE_EXTENT_CACHE_SLOTS; i++) {
            void* base = atomic_exchange_explicit(&g_hz4_large_extent_cache[idx][i], NULL, memory_order_acq_rel);
            if (base) {
                hz4_large_extent_cache_release_reserved_bytes(size);
                hz4_os_large_release(base, size);
            }
        }
    }
}
#endif

#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
static inline int hz4_large_pagebin_pages_enabled(uint32_t pages) {
    return pages >= 1 && pages <= (uint32_t)HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES;
}

#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX
static inline int hz4_large_pagebin_batch_pages_enabled(uint32_t pages) {
    return pages >= 1 && pages <= (uint32_t)HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES;
}

static inline void* hz4_large_pagebin_batch_next_get(void* node) {
    return *(void**)node;
}

static inline void hz4_large_pagebin_batch_next_set(void* node, void* next) {
    *(void**)node = next;
}

static inline void hz4_large_pagebin_batch_publish_list(uint32_t idx, void* head, void* tail, uint32_t n) {
    uint32_t retries = 0;
    void* old = atomic_load_explicit(&g_hz4_large_pagebin_batch_head[idx], memory_order_acquire);
    for (;;) {
        hz4_large_pagebin_batch_next_set(tail, old);
        if (atomic_compare_exchange_weak_explicit(&g_hz4_large_pagebin_batch_head[idx], &old, head,
                                                  memory_order_acq_rel, memory_order_acquire)) {
            break;
        }
        retries++;
    }
    hz4_os_stats_large_pagebin_b15_rel_flush_call();
    hz4_os_stats_large_pagebin_b15_rel_flush_objs(n);
    hz4_os_stats_large_pagebin_b15_rel_cas_retry(retries);
}

static inline void hz4_large_pagebin_batch_flush_tls_idx(uint32_t idx) {
    uint8_t n = g_hz4_large_pagebin_batch_relbuf_n[idx];
    if (n == 0) {
        return;
    }
    void* head = g_hz4_large_pagebin_batch_relbuf[idx][0];
    if (!head) {
        g_hz4_large_pagebin_batch_relbuf_n[idx] = 0;
        return;
    }
    void* tail = head;
    for (uint8_t i = 1; i < n; i++) {
        void* node = g_hz4_large_pagebin_batch_relbuf[idx][i];
        hz4_large_pagebin_batch_next_set(tail, node);
        tail = node;
        g_hz4_large_pagebin_batch_relbuf[idx][i] = NULL;
    }
    hz4_large_pagebin_batch_next_set(tail, NULL);
    g_hz4_large_pagebin_batch_relbuf[idx][0] = NULL;
    g_hz4_large_pagebin_batch_relbuf_n[idx] = 0;
    hz4_large_pagebin_batch_publish_list(idx, head, tail, (uint32_t)n);
}

static inline void hz4_large_pagebin_batch_flush_all(void) {
    for (uint32_t pages = 1; pages <= (uint32_t)HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES; pages++) {
        uint32_t idx = pages - 1;
        size_t size = ((size_t)pages << HZ4_PAGE_SHIFT);
        hz4_large_pagebin_batch_flush_tls_idx(idx);

        void* stash = g_hz4_large_pagebin_batch_stash[idx];
        g_hz4_large_pagebin_batch_stash[idx] = NULL;
        while (stash) {
            void* next = hz4_large_pagebin_batch_next_get(stash);
            hz4_os_large_release(stash, size);
            stash = next;
        }

        void* list = atomic_exchange_explicit(&g_hz4_large_pagebin_batch_head[idx], NULL, memory_order_acq_rel);
        while (list) {
            void* next = hz4_large_pagebin_batch_next_get(list);
            hz4_os_large_release(list, size);
            list = next;
        }
    }
}

static inline void* hz4_large_pagebin_batch_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_pagebin_batch_pages_enabled(pages)) {
        return NULL;
    }
    hz4_os_stats_large_pagebin_b15_acq_call();
    uint32_t idx = pages - 1;
    void* node = g_hz4_large_pagebin_batch_stash[idx];
    if (node) {
        g_hz4_large_pagebin_batch_stash[idx] = hz4_large_pagebin_batch_next_get(node);
        hz4_large_pagebin_batch_next_set(node, NULL);
        hz4_os_stats_large_pagebin_b15_acq_stash_hit();
        return node;
    }
    node = atomic_exchange_explicit(&g_hz4_large_pagebin_batch_head[idx], NULL, memory_order_acq_rel);
    if (!node) {
        return NULL;
    }
    hz4_os_stats_large_pagebin_b15_acq_take_list();
    g_hz4_large_pagebin_batch_stash[idx] = hz4_large_pagebin_batch_next_get(node);
    hz4_large_pagebin_batch_next_set(node, NULL);
    return node;
}

static inline int hz4_large_pagebin_batch_try_release(void* base, size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_pagebin_batch_pages_enabled(pages)) {
        return 0;
    }
    uint32_t idx = pages - 1;
    uint8_t n = g_hz4_large_pagebin_batch_relbuf_n[idx];
    if (n >= (uint8_t)HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N) {
        hz4_large_pagebin_batch_flush_tls_idx(idx);
        n = g_hz4_large_pagebin_batch_relbuf_n[idx];
    }
    g_hz4_large_pagebin_batch_relbuf[idx][n] = base;
    n = (uint8_t)(n + 1u);
    g_hz4_large_pagebin_batch_relbuf_n[idx] = n;
    if (n >= (uint8_t)HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N) {
        hz4_large_pagebin_batch_flush_tls_idx(idx);
    }
    return 1;
}
#endif

static inline void* hz4_large_pagebin_try_acquire(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_pagebin_pages_enabled(pages)) {
        return NULL;
    }
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX
    {
        void* got = hz4_large_pagebin_batch_try_acquire(total);
        if (got) {
            hz4_os_stats_large_pagebin_acq_hit();
            return got;
        }
    }
#endif
    uint32_t idx = pages - 1;
    for (uint32_t i = 0; i < (uint32_t)HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS; i++) {
        void* got = atomic_exchange_explicit(&g_hz4_large_pagebin_cache[idx][i], NULL, memory_order_acq_rel);
        if (got) {
            hz4_os_stats_large_pagebin_acq_hit();
            return got;
        }
    }
    hz4_os_stats_large_pagebin_acq_miss();
    return NULL;
}

static inline int hz4_large_pagebin_try_release(void* base, size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    if (!hz4_large_pagebin_pages_enabled(pages)) {
        return 0;
    }
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX
    if (hz4_large_pagebin_batch_try_release(base, total)) {
        hz4_os_stats_large_pagebin_rel_hit();
        return 1;
    }
#endif
    uint32_t idx = pages - 1;
    for (uint32_t i = 0; i < (uint32_t)HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS; i++) {
        void* expected = NULL;
        if (atomic_compare_exchange_strong_explicit(&g_hz4_large_pagebin_cache[idx][i], &expected, base,
                                                    memory_order_acq_rel, memory_order_acquire)) {
            hz4_os_stats_large_pagebin_rel_hit();
            return 1;
        }
    }
    hz4_os_stats_large_pagebin_rel_miss();
    return 0;
}
#endif

#if HZ4_S220_LARGE_OWNER_RETURN || HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX
static inline uint32_t hz4_large_owner_self(void) {
    hz4_tls_t* tls = hz4_tls_get();
    return (uint32_t)tls->owner;
}
#endif

#if HZ4_S220_LARGE_OWNER_RETURN
static inline int hz4_large_owner_return_pages_enabled(size_t total) {
    return ((uint32_t)(total >> HZ4_PAGE_SHIFT)) >= (uint32_t)HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES;
}
#endif

#if HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX
static inline int hz4_large_remote_bypass_pages_enabled(size_t total) {
    uint32_t pages = hz4_large_cache_pages(total);
    return pages >= 1 && pages <= (uint32_t)HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES;
}
#endif

#if HZ4_LARGE_CACHE_SHARDBOX
static inline uint32_t hz4_large_cache_self_shard(void) {
    hz4_tls_t* tls = hz4_tls_get();
    return (uint32_t)(tls->owner & (HZ4_LARGE_CACHE_SHARDS - 1u));
}
#endif


#include "hz4_large_cache_box_layers_tail.inc"
