static void hz3_large_insert(Hz3LargeHdr* hdr) {
    uint32_t idx = hz3_large_hash_ptr(hdr->user_ptr);
    hz3_lock_t* map_lock = hz3_large_map_lock_for_idx(idx);
    hz3_lock_acquire(map_lock);
    hdr->next = g_hz3_large_buckets[idx];
    g_hz3_large_buckets[idx] = hdr;
    hz3_large_debug_on_bucket_insert_locked(hdr);
    hz3_lock_release(map_lock);
}

#if HZ3_S238_STATS
static _Atomic size_t g_hz3_s238_direct_try = 0;
static _Atomic size_t g_hz3_s238_direct_hit = 0;
static _Atomic size_t g_hz3_s238_direct_miss_magic = 0;
static _Atomic size_t g_hz3_s238_direct_miss_userptr = 0;
static _Atomic size_t g_hz3_s238_fallback_hash_hit = 0;
static _Atomic size_t g_hz3_s238_fallback_hash_miss = 0;

static inline void hz3_s238_stats_inc(_Atomic size_t* p) {
    atomic_fetch_add_explicit(p, 1, memory_order_relaxed);
}

static void hz3_s238_stats_dump_final(void) {
    fprintf(stderr,
            "[HZ3_S238] direct_try=%zu direct_hit=%zu direct_miss_magic=%zu "
            "direct_miss_userptr=%zu fallback_hash_hit=%zu fallback_hash_miss=%zu\n",
            atomic_load_explicit(&g_hz3_s238_direct_try, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s238_direct_hit, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s238_direct_miss_magic, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s238_direct_miss_userptr, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s238_fallback_hash_hit, memory_order_relaxed),
            atomic_load_explicit(&g_hz3_s238_fallback_hash_miss, memory_order_relaxed));
}

static inline void hz3_s238_stats_register_once(void) {
    static _Atomic int g_hz3_s238_stats_registered = 0;
    if (atomic_exchange_explicit(&g_hz3_s238_stats_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s238_stats_dump_final);
    }
}
#define HZ3_S238_INC(name) hz3_s238_stats_inc(&(name))
#else
static inline void hz3_s238_stats_inc(_Atomic size_t* p) {
    (void)p;
}
static inline void hz3_s238_stats_register_once(void) {}
#define HZ3_S238_INC(name) ((void)0)
#endif

static inline int hz3_large_take_match_locked(Hz3LargeHdr* cur, const void* ptr) {
    // Debug: basic header sanity (one-shot) before deref-heavy checks.
    hz3_large_debug_check_bucket_node_locked(cur);
    if (cur->magic != HZ3_LARGE_MAGIC ||
        cur->map_base == NULL ||
        cur->map_size == 0 ||
        cur->user_ptr == NULL) {
        hz3_large_debug_bad_hdr_locked(cur);
    }
    hz3_large_debug_bad_size_locked(cur);
    if (cur->user_ptr != ptr || cur->magic != HZ3_LARGE_MAGIC) {
        return 0;
    }
#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_check_canary_free(cur);
#endif
#if HZ3_LARGE_CACHE_ENABLE
    // in_use==1 のみ許可（double-free 検出）
    if (cur->in_use != 1) {
        hz3_large_debug_bad_inuse(cur);
        return -1;
    }
#endif
    return 1;
}

static Hz3LargeHdr* hz3_large_take(const void* ptr) {
    uint32_t idx = hz3_large_hash_ptr(ptr);
    hz3_lock_t* map_lock = hz3_large_map_lock_for_idx(idx);
#if HZ3_S238_LARGE_DIRECT_LOOKUP
    Hz3LargeHdr* cand = NULL;
    size_t offset = hz3_large_user_offset();
    uintptr_t uptr = (uintptr_t)ptr;
    if (uptr >= offset) {
        cand = (Hz3LargeHdr*)(uptr - offset);
    }
    hz3_s238_stats_register_once();
    HZ3_S238_INC(g_hz3_s238_direct_try);
#endif

    hz3_lock_acquire(map_lock);

#if HZ3_S238_LARGE_DIRECT_LOOKUP
    if (cand != NULL) {
        Hz3LargeHdr* prev = NULL;
        Hz3LargeHdr* cur = g_hz3_large_buckets[idx];
        while (cur && cur != cand) {
            prev = cur;
            cur = cur->next;
        }
        if (cur == cand) {
            if (cur->magic != HZ3_LARGE_MAGIC) {
                HZ3_S238_INC(g_hz3_s238_direct_miss_magic);
            } else if (cur->user_ptr != ptr) {
                HZ3_S238_INC(g_hz3_s238_direct_miss_userptr);
            } else {
                int matched = hz3_large_take_match_locked(cur, ptr);
                if (matched < 0) {
                    hz3_lock_release(map_lock);
                    return NULL;
                }
                if (matched > 0) {
                    if (prev) {
                        prev->next = cur->next;
                    } else {
                        g_hz3_large_buckets[idx] = cur->next;
                    }
                    cur->next = NULL;  // bucket list から切断
                    hz3_large_debug_on_bucket_remove_locked(cur);
                    HZ3_S238_INC(g_hz3_s238_direct_hit);
                    hz3_lock_release(map_lock);
                    return cur;
                }
                HZ3_S238_INC(g_hz3_s238_direct_miss_userptr);
            }
        } else {
            HZ3_S238_INC(g_hz3_s238_direct_miss_userptr);
        }
    } else {
        HZ3_S238_INC(g_hz3_s238_direct_miss_userptr);
    }
#endif

    Hz3LargeHdr* prev = NULL;
    Hz3LargeHdr* cur = g_hz3_large_buckets[idx];
    while (cur) {
        int matched = hz3_large_take_match_locked(cur, ptr);
        if (matched < 0) {
            hz3_lock_release(map_lock);
            return NULL;
        }
        if (matched > 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                g_hz3_large_buckets[idx] = cur->next;
            }
            cur->next = NULL;  // bucket list から切断
            hz3_large_debug_on_bucket_remove_locked(cur);
#if HZ3_S238_LARGE_DIRECT_LOOKUP
            HZ3_S238_INC(g_hz3_s238_fallback_hash_hit);
#endif
            hz3_lock_release(map_lock);
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
#if HZ3_S238_LARGE_DIRECT_LOOKUP
    HZ3_S238_INC(g_hz3_s238_fallback_hash_miss);
#endif
    hz3_lock_release(map_lock);
    return NULL;
}

static int hz3_large_find_size(const void* ptr, size_t* size_out) {
    uint32_t idx = hz3_large_hash_ptr(ptr);
    hz3_lock_t* map_lock = hz3_large_map_lock_for_idx(idx);
    hz3_lock_acquire(map_lock);
    Hz3LargeHdr* cur = g_hz3_large_buckets[idx];
    while (cur) {
        if (cur->user_ptr == ptr && cur->magic == HZ3_LARGE_MAGIC) {
            *size_out = cur->req_size;
            hz3_lock_release(map_lock);
            return 1;
        }
        cur = cur->next;
    }
    hz3_lock_release(map_lock);
    return 0;
}
