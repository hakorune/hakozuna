// ============================================================================
// hz3_hot_fallback.inc - RTLD_NEXT fallback functions
// This file is included by hz3_hot.c (single TU)
// ============================================================================

typedef void* (*Hz3MallocFn)(size_t);
typedef void  (*Hz3FreeFn)(void*);
typedef void* (*Hz3CallocFn)(size_t, size_t);
typedef void* (*Hz3ReallocFn)(void*, size_t);

#if defined(_WIN32)
static inline void* hz3_next_malloc(size_t size) {
    return malloc(size);
}

static inline void hz3_next_free(void* ptr) {
    free(ptr);
}

static inline void* hz3_next_calloc(size_t n, size_t s) {
    return calloc(n, s);
}

static inline void* hz3_next_realloc(void* p, size_t s) {
    return realloc(p, s);
}
#else
static inline void* hz3_next_sym(const char* name) {
    return dlsym(RTLD_NEXT, name);
}

static inline void* hz3_next_malloc(size_t size) {
    Hz3MallocFn fn = (Hz3MallocFn)hz3_next_sym("malloc");
    return fn ? fn(size) : NULL;
}

static inline void hz3_next_free(void* ptr) {
    Hz3FreeFn fn = (Hz3FreeFn)hz3_next_sym("free");
    if (fn) fn(ptr);
}

static inline void* hz3_next_calloc(size_t n, size_t s) {
    Hz3CallocFn fn = (Hz3CallocFn)hz3_next_sym("calloc");
    return fn ? fn(n, s) : NULL;
}

static inline void* hz3_next_realloc(void* p, size_t s) {
    Hz3ReallocFn fn = (Hz3ReallocFn)hz3_next_sym("realloc");
    return fn ? fn(p, s) : NULL;
}
#endif
