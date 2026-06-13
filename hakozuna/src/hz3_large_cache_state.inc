#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
// S50: Per-class LIFO cache
static Hz3LargeHdr* g_sc_head[HZ3_LARGE_SC_COUNT];
static size_t g_sc_bytes[HZ3_LARGE_SC_COUNT];
static uint32_t g_sc_nodes[HZ3_LARGE_SC_COUNT];
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
static hz3_lock_t g_sc_locks[HZ3_LARGE_SC_COUNT] = {
    [0 ... HZ3_LARGE_SC_COUNT - 1] = HZ3_LOCK_INITIALIZER
};
static _Atomic size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#else
static size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
// Relaxed hint for free-path precheck (S184). Real admission is still checked under lock.
static _Atomic size_t g_total_cached_bytes_hint = 0;
#endif
#if HZ3_S253_LARGE_RSS_RETENTION_OBS
static _Atomic size_t g_s253_global_cached_bytes_peak = 0;

static inline void hz3_s253_update_global_cached_peak(size_t value) {
    size_t cur = atomic_load_explicit(&g_s253_global_cached_bytes_peak,
                                      memory_order_relaxed);
    while (value > cur) {
        if (atomic_compare_exchange_weak_explicit(
                &g_s253_global_cached_bytes_peak,
                &cur,
                value,
                memory_order_relaxed,
                memory_order_relaxed)) {
            break;
        }
    }
}
#endif
#if HZ3_S207_TARGETED_REUSE
static _Atomic uint16_t g_sc_reuse_credit[HZ3_LARGE_SC_COUNT];
#endif
#else
// Legacy: single LIFO
static Hz3LargeHdr* g_hz3_large_free_head = NULL;
static size_t g_hz3_large_cached_bytes = 0;
static size_t g_hz3_large_cached_nodes = 0;
#endif
#endif
