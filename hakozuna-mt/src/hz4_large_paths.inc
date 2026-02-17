// Internal helper paths extracted from hz4_large.c (move-only split).
#if HZ4_ST_LARGE_SPAN_CACHE
// ST lane: per-page-count small LIFO cache.
static __thread void* g_hz4_st_large_cache[HZ4_ST_LARGE_SPAN_MAX_PAGES][HZ4_ST_LARGE_SPAN_SLOTS];
static __thread uint8_t g_hz4_st_large_cache_top[HZ4_ST_LARGE_SPAN_MAX_PAGES];

static inline uint32_t hz4_st_large_pages(size_t total) {
    return (uint32_t)(total >> HZ4_PAGE_SHIFT);
}

static inline void* hz4_st_large_cache_try_acquire(size_t total) {
    uint32_t pages = hz4_st_large_pages(total);
    if (pages == 0 || pages > HZ4_ST_LARGE_SPAN_MAX_PAGES) {
        return NULL;
    }
    uint32_t idx = pages - 1;
    uint8_t top = g_hz4_st_large_cache_top[idx];
    if (top == 0) {
        return NULL;
    }
    void* base = g_hz4_st_large_cache[idx][top - 1];
    g_hz4_st_large_cache[idx][top - 1] = NULL;
    g_hz4_st_large_cache_top[idx] = (uint8_t)(top - 1);
    return base;
}

static inline int hz4_st_large_cache_try_release(void* base, size_t total) {
    uint32_t pages = hz4_st_large_pages(total);
    if (pages == 0 || pages > HZ4_ST_LARGE_SPAN_MAX_PAGES) {
        return 0;
    }
    uint32_t idx = pages - 1;
    uint8_t top = g_hz4_st_large_cache_top[idx];
    if (top >= HZ4_ST_LARGE_SPAN_SLOTS) {
        return 0;
    }
    g_hz4_st_large_cache[idx][top] = base;
    g_hz4_st_large_cache_top[idx] = (uint8_t)(top + 1);
    return 1;
}
#endif

#if HZ4_LARGE_SPAN_CACHE
// All-lane: per-page-count TLS cache.
static __thread void* g_hz4_large_span_cache[HZ4_LARGE_SPAN_MAX_PAGES][HZ4_LARGE_SPAN_SLOTS];
static __thread uint8_t g_hz4_large_span_cache_top[HZ4_LARGE_SPAN_MAX_PAGES];

static inline int hz4_large_span_cache_enabled(void) {
#if HZ4_LARGE_SPAN_CACHE_GATEBOX
    hz4_tls_t* tls = hz4_tls_get();
    if ((uint32_t)tls->tid > (uint32_t)HZ4_LARGE_SPAN_CACHE_GATE_TID_MAX) {
        return 0;
    }
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
#if !HZ4_LARGE_SPAN_CACHE_GATE_ALLOW_WHEN_RBUF_ON
    if (tls->rbuf_gate_on) {
        return 0;
    }
#endif
#endif
#endif
    return 1;
}

static inline uint32_t hz4_large_pages(size_t total) {
    return (uint32_t)(total >> HZ4_PAGE_SHIFT);
}

static inline void* hz4_large_span_cache_try_acquire(size_t total) {
    uint32_t pages = hz4_large_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_SPAN_MAX_PAGES) {
        return NULL;
    }
    uint32_t idx = pages - 1;
    uint8_t top = g_hz4_large_span_cache_top[idx];
    if (top == 0) {
        return NULL;
    }
    void* base = g_hz4_large_span_cache[idx][top - 1];
    g_hz4_large_span_cache[idx][top - 1] = NULL;
    g_hz4_large_span_cache_top[idx] = (uint8_t)(top - 1);
    return base;
}

static inline int hz4_large_span_cache_try_release(void* base, size_t total) {
    uint32_t pages = hz4_large_pages(total);
    if (pages == 0 || pages > HZ4_LARGE_SPAN_MAX_PAGES) {
        return 0;
    }
    uint32_t idx = pages - 1;
    uint8_t top = g_hz4_large_span_cache_top[idx];
    if (top >= HZ4_LARGE_SPAN_SLOTS) {
        return 0;
    }
    g_hz4_large_span_cache[idx][top] = base;
    g_hz4_large_span_cache_top[idx] = (uint8_t)(top + 1);
    return 1;
}
#endif


#if HZ4_LARGE_CACHE_BOX
#include "hz4_large_cache_box_layers.inc"
#include "hz4_large_cache_box_paths.inc"
#endif

#include "hz4_large_cache_box_retry.inc"

#if HZ4_LARGE_FAIL_RESCUE_BOX
    if (base) {
        g_hz4_large_fail_streak_tls = 0;
    }
#endif
    return base;
}
