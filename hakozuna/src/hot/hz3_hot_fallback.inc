// ============================================================================
// hz3_hot_fallback.inc - RTLD_NEXT fallback functions
// This file is included by hz3_hot.c (single TU)
// ============================================================================

typedef void* (*Hz3MallocFn)(size_t);
typedef void  (*Hz3FreeFn)(void*);
typedef void* (*Hz3CallocFn)(size_t, size_t);
typedef void* (*Hz3ReallocFn)(void*, size_t);

#ifndef HZ3_NEXT_FALLBACK_STATS
#define HZ3_NEXT_FALLBACK_STATS 0
#endif

#if HZ3_NEXT_FALLBACK_STATS
typedef struct {
    _Atomic uint64_t malloc_calls;
    _Atomic uint64_t free_calls;
    _Atomic uint64_t calloc_calls;
    _Atomic uint64_t realloc_calls;
} hz3_next_fallback_stats_t;

static hz3_next_fallback_stats_t g_hz3_next_fallback_stats;

static void hz3_next_fallback_stats_dump(void) {
    uint64_t malloc_calls = atomic_load_explicit(&g_hz3_next_fallback_stats.malloc_calls, memory_order_relaxed);
    uint64_t free_calls = atomic_load_explicit(&g_hz3_next_fallback_stats.free_calls, memory_order_relaxed);
    uint64_t calloc_calls = atomic_load_explicit(&g_hz3_next_fallback_stats.calloc_calls, memory_order_relaxed);
    uint64_t realloc_calls = atomic_load_explicit(&g_hz3_next_fallback_stats.realloc_calls, memory_order_relaxed);
    if ((malloc_calls | free_calls | calloc_calls | realloc_calls) == 0) {
        return;
    }
    fprintf(stderr,
            "[HZ3_NEXT_FALLBACK] malloc=%llu free=%llu calloc=%llu realloc=%llu\n",
            (unsigned long long)malloc_calls,
            (unsigned long long)free_calls,
            (unsigned long long)calloc_calls,
            (unsigned long long)realloc_calls);
}

__attribute__((constructor))
static void hz3_next_fallback_stats_init(void) {
    atexit(hz3_next_fallback_stats_dump);
}

static inline void hz3_next_fallback_malloc_inc(void) {
    atomic_fetch_add_explicit(&g_hz3_next_fallback_stats.malloc_calls, 1, memory_order_relaxed);
}
static inline void hz3_next_fallback_free_inc(void) {
    atomic_fetch_add_explicit(&g_hz3_next_fallback_stats.free_calls, 1, memory_order_relaxed);
}
static inline void hz3_next_fallback_calloc_inc(void) {
    atomic_fetch_add_explicit(&g_hz3_next_fallback_stats.calloc_calls, 1, memory_order_relaxed);
}
static inline void hz3_next_fallback_realloc_inc(void) {
    atomic_fetch_add_explicit(&g_hz3_next_fallback_stats.realloc_calls, 1, memory_order_relaxed);
}
#else
static inline void hz3_next_fallback_malloc_inc(void) {}
static inline void hz3_next_fallback_free_inc(void) {}
static inline void hz3_next_fallback_calloc_inc(void) {}
static inline void hz3_next_fallback_realloc_inc(void) {}
#endif

#if defined(_WIN32)
static inline void* hz3_next_malloc(size_t size) {
    hz3_next_fallback_malloc_inc();
    return malloc(size);
}

static inline void hz3_next_free(void* ptr) {
    hz3_next_fallback_free_inc();
    free(ptr);
}

static inline void* hz3_next_calloc(size_t n, size_t s) {
    hz3_next_fallback_calloc_inc();
    return calloc(n, s);
}

static inline void* hz3_next_realloc(void* p, size_t s) {
    hz3_next_fallback_realloc_inc();
    return realloc(p, s);
}
#else
static inline void* hz3_next_sym(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

static _Atomic(uintptr_t) g_hz3_next_malloc_fn_cache = 0;
static _Atomic(uintptr_t) g_hz3_next_free_fn_cache = 0;
static _Atomic(uintptr_t) g_hz3_next_calloc_fn_cache = 0;
static _Atomic(uintptr_t) g_hz3_next_realloc_fn_cache = 0;

static inline Hz3MallocFn hz3_next_malloc_fn(void) {
    uintptr_t cached = atomic_load_explicit(&g_hz3_next_malloc_fn_cache, memory_order_acquire);
    if (cached != 0) {
        return (Hz3MallocFn)cached;
    }
    Hz3MallocFn fn = (Hz3MallocFn)hz3_next_sym("malloc");
    if (fn) {
        atomic_store_explicit(&g_hz3_next_malloc_fn_cache, (uintptr_t)fn, memory_order_release);
    }
    return fn;
}

static inline Hz3FreeFn hz3_next_free_fn(void) {
    uintptr_t cached = atomic_load_explicit(&g_hz3_next_free_fn_cache, memory_order_acquire);
    if (cached != 0) {
        return (Hz3FreeFn)cached;
    }
    Hz3FreeFn fn = (Hz3FreeFn)hz3_next_sym("free");
    if (fn) {
        atomic_store_explicit(&g_hz3_next_free_fn_cache, (uintptr_t)fn, memory_order_release);
    }
    return fn;
}

static inline Hz3CallocFn hz3_next_calloc_fn(void) {
    uintptr_t cached = atomic_load_explicit(&g_hz3_next_calloc_fn_cache, memory_order_acquire);
    if (cached != 0) {
        return (Hz3CallocFn)cached;
    }
    Hz3CallocFn fn = (Hz3CallocFn)hz3_next_sym("calloc");
    if (fn) {
        atomic_store_explicit(&g_hz3_next_calloc_fn_cache, (uintptr_t)fn, memory_order_release);
    }
    return fn;
}

static inline Hz3ReallocFn hz3_next_realloc_fn(void) {
    uintptr_t cached = atomic_load_explicit(&g_hz3_next_realloc_fn_cache, memory_order_acquire);
    if (cached != 0) {
        return (Hz3ReallocFn)cached;
    }
    Hz3ReallocFn fn = (Hz3ReallocFn)hz3_next_sym("realloc");
    if (fn) {
        atomic_store_explicit(&g_hz3_next_realloc_fn_cache, (uintptr_t)fn, memory_order_release);
    }
    return fn;
}

static inline void* hz3_next_malloc(size_t size) {
    hz3_next_fallback_malloc_inc();
    Hz3MallocFn fn = hz3_next_malloc_fn();
    return fn ? fn(size) : NULL;
}

static inline void hz3_next_free(void* ptr) {
    hz3_next_fallback_free_inc();
    Hz3FreeFn fn = hz3_next_free_fn();
    if (fn) fn(ptr);
}

static inline void* hz3_next_calloc(size_t n, size_t s) {
    hz3_next_fallback_calloc_inc();
    Hz3CallocFn fn = hz3_next_calloc_fn();
    return fn ? fn(n, s) : NULL;
}

static inline void* hz3_next_realloc(void* p, size_t s) {
    hz3_next_fallback_realloc_inc();
    Hz3ReallocFn fn = hz3_next_realloc_fn();
    return fn ? fn(p, s) : NULL;
}
#endif
