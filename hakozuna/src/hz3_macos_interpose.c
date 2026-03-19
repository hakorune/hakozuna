#if defined(__APPLE__)

#include <errno.h>
#include <malloc/malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "hz3.h"
#include "hz3_arena.h"
#include "hz3_config.h"
#include "hz3_large.h"
#include "hz3_types.h"

static inline int hz3_macos_is_pow2(size_t x) {
    return x && ((x & (x - 1u)) == 0);
}

typedef struct {
    _Atomic uint64_t calls;
    _Atomic uint64_t hz3_hit;
    _Atomic uint64_t system_fallback;
    _Atomic uint64_t arena_unknown;
    _Atomic uint64_t zero_return;
} hz3_macos_malloc_size_stats_t;

static hz3_macos_malloc_size_stats_t g_hz3_macos_malloc_size_stats;

static void hz3_macos_malloc_size_stats_dump(void) {
    uint64_t calls = atomic_load_explicit(&g_hz3_macos_malloc_size_stats.calls, memory_order_relaxed);
    uint64_t hz3_hit = atomic_load_explicit(&g_hz3_macos_malloc_size_stats.hz3_hit, memory_order_relaxed);
    uint64_t system_fallback =
        atomic_load_explicit(&g_hz3_macos_malloc_size_stats.system_fallback, memory_order_relaxed);
    uint64_t arena_unknown =
        atomic_load_explicit(&g_hz3_macos_malloc_size_stats.arena_unknown, memory_order_relaxed);
    uint64_t zero_return = atomic_load_explicit(&g_hz3_macos_malloc_size_stats.zero_return, memory_order_relaxed);
    if (calls == 0) {
        return;
    }
    fprintf(stderr,
            "[HZ3_MACOS_MALLOC_SIZE] calls=%llu hz3_hit=%llu system_fallback=%llu arena_unknown=%llu zero_return=%llu\n",
            (unsigned long long)calls,
            (unsigned long long)hz3_hit,
            (unsigned long long)system_fallback,
            (unsigned long long)arena_unknown,
            (unsigned long long)zero_return);
}

__attribute__((constructor))
static void hz3_macos_malloc_size_stats_init(void) {
    atexit(hz3_macos_malloc_size_stats_dump);
}

static size_t hz3_macos_system_malloc_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    malloc_zone_t* zone = malloc_zone_from_ptr(ptr);
    if (!zone || !zone->size) {
        return 0;
    }
    return zone->size(zone, ptr);
}

size_t malloc_usable_size(void* ptr) {
    atomic_fetch_add_explicit(&g_hz3_macos_malloc_size_stats.calls, 1, memory_order_relaxed);

    if (!ptr) {
        atomic_fetch_add_explicit(&g_hz3_macos_malloc_size_stats.zero_return, 1, memory_order_relaxed);
        return 0;
    }

    size_t out = hz3_usable_size(ptr);
    if (out != 0) {
        atomic_fetch_add_explicit(&g_hz3_macos_malloc_size_stats.hz3_hit, 1, memory_order_relaxed);
        return out;
    }

    if (hz3_arena_contains_fast(ptr, NULL, NULL)) {
        atomic_fetch_add_explicit(&g_hz3_macos_malloc_size_stats.arena_unknown, 1, memory_order_relaxed);
        return 0;
    }

    atomic_fetch_add_explicit(&g_hz3_macos_malloc_size_stats.system_fallback, 1, memory_order_relaxed);
    out = hz3_macos_system_malloc_size(ptr);
    if (out != 0) {
        return out;
    }

    atomic_fetch_add_explicit(&g_hz3_macos_malloc_size_stats.zero_return, 1, memory_order_relaxed);
    return 0;
}

static size_t hz3_macos_malloc_size(void* ptr) {
    return malloc_usable_size(ptr);
}

static void* hz3_macos_malloc(size_t size) {
    return hz3_malloc(size);
}

static void hz3_macos_free(void* ptr) {
    hz3_free(ptr);
}

static void* hz3_macos_calloc(size_t nmemb, size_t size) {
    return hz3_calloc(nmemb, size);
}

static void* hz3_macos_realloc(void* ptr, size_t size) {
    return hz3_realloc(ptr, size);
}

static int hz3_macos_posix_memalign(void** memptr, size_t alignment, size_t size) {
    if (!memptr || !hz3_macos_is_pow2(alignment) || alignment < sizeof(void*)) {
        return EINVAL;
    }

    void* p = (alignment <= HZ3_SMALL_ALIGN) ? hz3_malloc(size)
                                             : hz3_large_aligned_alloc(alignment, size);
    if (!p) {
        return ENOMEM;
    }
    *memptr = p;
    return 0;
}

static void* hz3_macos_aligned_alloc(size_t alignment, size_t size) {
    if (!hz3_macos_is_pow2(alignment) || alignment == 0) {
        errno = EINVAL;
        return NULL;
    }
    if (alignment && (size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }
    if (alignment <= HZ3_SMALL_ALIGN) {
        return hz3_malloc(size);
    }
    void* p = hz3_large_aligned_alloc(alignment, size);
    if (!p) {
        errno = ENOMEM;
    }
    return p;
}

void* memalign(size_t alignment, size_t size) {
    if (!hz3_macos_is_pow2(alignment) || alignment < sizeof(void*)) {
        errno = EINVAL;
        return NULL;
    }
    if (alignment <= HZ3_SMALL_ALIGN) {
        return hz3_malloc(size);
    }
    void* p = hz3_large_aligned_alloc(alignment, size);
    if (!p) {
        errno = ENOMEM;
    }
    return p;
}

#define HZ3_DYLD_INTERPOSE(replacement, replacee) \
    __attribute__((used)) static const struct { const void* replacement; const void* replacee; } \
        _hz3_interpose_##replacement##_##replacee \
        __attribute__((section("__DATA_CONST,__interpose"))) = { \
            (const void*)(uintptr_t)&replacement, (const void*)(uintptr_t)&replacee \
        }

HZ3_DYLD_INTERPOSE(hz3_macos_malloc, malloc);
HZ3_DYLD_INTERPOSE(hz3_macos_free, free);
HZ3_DYLD_INTERPOSE(hz3_macos_calloc, calloc);
HZ3_DYLD_INTERPOSE(hz3_macos_realloc, realloc);
HZ3_DYLD_INTERPOSE(hz3_macos_posix_memalign, posix_memalign);
HZ3_DYLD_INTERPOSE(hz3_macos_aligned_alloc, aligned_alloc);
HZ3_DYLD_INTERPOSE(hz3_macos_malloc_size, malloc_size);

#endif  // __APPLE__
