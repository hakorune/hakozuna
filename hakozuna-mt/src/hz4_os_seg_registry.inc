// hz4_os_seg_registry.inc - B25 segment-registry boundary helpers
#ifndef HZ4_OS_SEG_REGISTRY_INC
#define HZ4_OS_SEG_REGISTRY_INC

#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
#define HZ4_SEGREG_EMPTY ((uintptr_t)0)
#define HZ4_SEGREG_TOMB  ((uintptr_t)1)
static _Atomic(uintptr_t)
    g_hz4_seg_registry[HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS];

static inline uint32_t hz4_seg_registry_hash(uintptr_t seg_base) {
    uint64_t x = (uint64_t)(seg_base >> HZ4_SEG_SHIFT);
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint32_t)x & (HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS - 1u);
}

static inline void hz4_seg_registry_insert(uintptr_t seg_base) {
    uint32_t idx = hz4_seg_registry_hash(seg_base);
    for (uint32_t i = 0; i < (uint32_t)HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE; i++) {
        _Atomic(uintptr_t)* slot =
            &g_hz4_seg_registry[(idx + i) & (HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS - 1u)];
        uintptr_t cur = atomic_load_explicit(slot, memory_order_acquire);
        if (cur == seg_base) {
            return;
        }
        if (cur == HZ4_SEGREG_EMPTY || cur == HZ4_SEGREG_TOMB) {
            if (atomic_compare_exchange_weak_explicit(slot, &cur, seg_base,
                                                      memory_order_release,
                                                      memory_order_acquire)) {
                return;
            }
        }
    }
}

static inline void hz4_seg_registry_remove(uintptr_t seg_base) {
    uint32_t idx = hz4_seg_registry_hash(seg_base);
    for (uint32_t i = 0; i < (uint32_t)HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE; i++) {
        _Atomic(uintptr_t)* slot =
            &g_hz4_seg_registry[(idx + i) & (HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS - 1u)];
        uintptr_t cur = atomic_load_explicit(slot, memory_order_acquire);
        if (cur == HZ4_SEGREG_EMPTY) {
            return;
        }
        if (cur == seg_base) {
            (void)atomic_compare_exchange_strong_explicit(slot, &cur, HZ4_SEGREG_TOMB,
                                                          memory_order_release,
                                                          memory_order_acquire);
            return;
        }
    }
}
#endif

static inline int hz4_os_seg_registry_contains_ptr(const void* ptr) {
#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
    if (!ptr) {
        return 0;
    }
    uintptr_t seg_base = (uintptr_t)ptr & ~((uintptr_t)HZ4_SEG_SIZE - 1u);
    uint32_t idx = hz4_seg_registry_hash(seg_base);
    for (uint32_t i = 0; i < (uint32_t)HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE; i++) {
        uintptr_t cur = atomic_load_explicit(
            &g_hz4_seg_registry[(idx + i) & (HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS - 1u)],
            memory_order_acquire);
        if (cur == seg_base) {
            return 1;
        }
        if (cur == HZ4_SEGREG_EMPTY) {
            return 0;
        }
    }
    return 0;
#else
    (void)ptr;
    return 0;
#endif
}

#endif  // HZ4_OS_SEG_REGISTRY_INC
