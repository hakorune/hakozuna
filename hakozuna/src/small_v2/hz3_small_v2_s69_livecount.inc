// hz3_small_v2_s69_livecount.inc - S69 SmallPageLiveCountBox helpers
// Part of hz3_small_v2.c (single TU split)

// ============================================================================
// S69: SmallPageLiveCountBox helpers
// ============================================================================
#if HZ3_S69_LIVECOUNT

#if HZ3_S69_LIVECOUNT_STATS
static _Atomic uint64_t g_s69_alloc_inc = 0;
static _Atomic uint64_t g_s69_free_dec = 0;
static _Atomic uint64_t g_s69_retire_blocked = 0;  // live>0 at retire check
static _Atomic uint64_t g_s69_skip_page_aligned = 0;  // S70: skipped page-aligned free attempts
static _Atomic int g_s69_atexit_registered = 0;

static void hz3_s69_atexit_dump(void) {
    uint64_t inc = atomic_load_explicit(&g_s69_alloc_inc, memory_order_relaxed);
    uint64_t dec = atomic_load_explicit(&g_s69_free_dec, memory_order_relaxed);
    uint64_t blocked = atomic_load_explicit(&g_s69_retire_blocked, memory_order_relaxed);
    uint64_t skip = atomic_load_explicit(&g_s69_skip_page_aligned, memory_order_relaxed);
    if (inc > 0 || dec > 0 || blocked > 0 || skip > 0) {
        fprintf(stderr, "[HZ3_S69] inc=%lu dec=%lu blocked=%lu skip_aligned=%lu\n",
                inc, dec, blocked, skip);
    }
}

static inline void hz3_s69_stats_register_atexit(void) {
    if (atomic_exchange_explicit(&g_s69_atexit_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s69_atexit_dump);
    }
}
#endif  // HZ3_S69_LIVECOUNT_STATS

void hz3_s69_live_count_inc(void* ptr) {
    uintptr_t page_base = (uintptr_t)ptr & ~(HZ3_PAGE_SIZE - 1);
    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;

#if HZ3_S69_LIVECOUNT_FAILFAST
    // Overflow check: live > capacity is impossible
    uint16_t cur = atomic_load_explicit(&ph->live_count, memory_order_relaxed);
    size_t cap = hz3_small_v2_page_capacity((int)ph->sc);
    if (cur >= cap) {
        fprintf(stderr, "[HZ3_S69_OVERFLOW] ptr=%p sc=%d live=%u cap=%zu\n",
                ptr, (int)ph->sc, cur, cap);
        abort();
    }
#endif

    atomic_fetch_add_explicit(&ph->live_count, 1, memory_order_relaxed);

#if HZ3_S69_LIVECOUNT_STATS
    hz3_s69_stats_register_atexit();
    atomic_fetch_add_explicit(&g_s69_alloc_inc, 1, memory_order_relaxed);
#endif
}

void hz3_s69_live_count_dec(void* ptr) {
    uintptr_t page_base = (uintptr_t)ptr & ~(HZ3_PAGE_SIZE - 1);

    // Skip page-aligned pointers - these are page headers, not user objects
    // This guards against pre-existing bugs in PTAG hot path that accept invalid addresses
    if ((uintptr_t)ptr == page_base) {
#if HZ3_S69_LIVECOUNT_STATS
        atomic_fetch_add_explicit(&g_s69_skip_page_aligned, 1, memory_order_relaxed);
#endif
        return;
    }

    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;

#if HZ3_S69_LIVECOUNT_FAILFAST
    uint16_t old = atomic_fetch_sub_explicit(&ph->live_count, 1, memory_order_acq_rel);
    if (old == 0) {
        fprintf(stderr, "[HZ3_S69_UNDERFLOW] ptr=%p page=%p (double-free?)\n",
                ptr, (void*)page_base);
        abort();
    }
#else
    atomic_fetch_sub_explicit(&ph->live_count, 1, memory_order_acq_rel);
#endif

#if HZ3_S69_LIVECOUNT_STATS
    atomic_fetch_add_explicit(&g_s69_free_dec, 1, memory_order_relaxed);
#endif
}

uint16_t hz3_s69_live_count_read(void* page_base) {
    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;
    return atomic_load_explicit(&ph->live_count, memory_order_acquire);
}

void hz3_s69_retire_blocked_inc(void) {
#if HZ3_S69_LIVECOUNT_STATS
    atomic_fetch_add_explicit(&g_s69_retire_blocked, 1, memory_order_relaxed);
#endif
}

#endif  // HZ3_S69_LIVECOUNT
