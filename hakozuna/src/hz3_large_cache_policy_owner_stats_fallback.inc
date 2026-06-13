
#if HZ3_S240_LARGE_OWNER_FRONT && !HZ3_S240_LARGE_OWNER_FRONT_STATS
static inline void hz3_large_s240_register_once(void) {
}

static inline uint8_t hz3_large_s240_current_owner(void) {
    hz3_tcache_ensure_init();
    if (!t_hz3_cache.initialized) {
        return UINT8_MAX;
    }
    return (uint8_t)t_hz3_cache.my_shard;
}

static inline uint8_t hz3_large_s240_normalize_sc(int sc) {
    return (sc >= 0 && sc < HZ3_LARGE_SC_COUNT) ? (uint8_t)sc : UINT8_MAX;
}
#endif

#if !HZ3_S240_LARGE_OWNER_FRONT_STATS
static inline void hz3_s292_on_alloc_req(size_t req_size) {
    (void)req_size;
}
static inline void hz3_s292_on_free_hdr(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_front_pop(size_t req_size, int hit, size_t bytes) {
    (void)req_size;
    (void)hit;
    (void)bytes;
}
static inline void hz3_s292_on_front_push_result(const Hz3LargeHdr* hdr, int success) {
    (void)hdr;
    (void)success;
}
static inline void hz3_s292_on_front_flush_node(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_inbox_push_attempt(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_inbox_push_result(const Hz3LargeHdr* hdr,
                                                 int ok,
                                                 int reject_owner,
                                                 int reject_budget,
                                                 int reject_invalid) {
    (void)hdr;
    (void)ok;
    (void)reject_owner;
    (void)reject_budget;
    (void)reject_invalid;
}
static inline void hz3_s292_on_inbox_pop(const Hz3LargeHdr* hdr, int from_flush) {
    (void)hdr;
    (void)from_flush;
}
static inline void hz3_s292_on_inbox_flush_seen(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_global_fallback(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_global_store(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s292_on_global_munmap(const Hz3LargeHdr* hdr) {
    (void)hdr;
}

#if HZ3_S293_HIGHBAND_GLOBAL_SCACHE_CAP
static inline int hz3_s293_global_admit_under_cap_locked(int sc,
                                                         const Hz3LargeHdr* hdr) {
    if (!hdr ||
        hdr->req_size <= (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES ||
        hdr->req_size > (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES) {
        return 1;
    }
    hz3_large_sc_lock_acquire(sc);
    uint32_t nodes = g_sc_nodes[sc];
    hz3_large_sc_lock_release(sc);
    return nodes < (uint32_t)HZ3_S293_HIGHBAND_GLOBAL_MAX_NODES_PER_SC;
}
#else
static inline int hz3_s293_global_admit_under_cap_locked(int sc,
                                                         const Hz3LargeHdr* hdr) {
    (void)sc;
    (void)hdr;
    return 1;
}
#endif

#if HZ3_S293D_HIGHBAND_GLOBAL_SOFT_CAP
static inline int hz3_s293d_global_admit_under_soft_cap_locked(
    int sc,
    const Hz3LargeHdr* hdr) {
    if (!hdr ||
        hdr->req_size <= (size_t)HZ3_S291_HIGHBAND_MIN_CLASS_BYTES ||
        hdr->req_size > (size_t)HZ3_S291_HIGHBAND_MAX_CLASS_BYTES) {
        return 1;
    }
    hz3_large_sc_lock_acquire(sc);
    uint32_t nodes = g_sc_nodes[sc];
    hz3_large_sc_lock_release(sc);
    if (nodes < (uint32_t)HZ3_S293D_HIGHBAND_GLOBAL_SOFT_NODES_PER_SC) {
        return 1;
    }
    if (nodes >= (uint32_t)HZ3_S293D_HIGHBAND_GLOBAL_HARD_NODES_PER_SC) {
        return 0;
    }
    uint32_t epoch = atomic_fetch_add_explicit(
        &g_s293d_global_admit_epoch[sc],
        1,
        memory_order_relaxed);
    return ((epoch % (uint32_t)HZ3_S293D_HIGHBAND_GLOBAL_ADMIT_STRIDE) == 0);
}
#else
static inline int hz3_s293d_global_admit_under_soft_cap_locked(
    int sc,
    const Hz3LargeHdr* hdr) {
    (void)sc;
    (void)hdr;
    return 1;
}
#endif
#endif

