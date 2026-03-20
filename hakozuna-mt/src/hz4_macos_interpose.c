#if defined(__APPLE__)

#include <errno.h>
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hz4_tcache.h"
#include "hz4_mid.h"
#include "hz4_large.h"
#include "hz4_sizeclass.h"
#include "hz4_config.h"
#include "hz4_os.h"
#include "hz4_page.h"
#include "hz4_macos_foreign.h"

#ifndef HZ4_MACOS_INTERPOSE_MALLOC_SIZE
#define HZ4_MACOS_INTERPOSE_MALLOC_SIZE 1
#endif

#ifndef HZ4_MACOS_MALLOC_SIZE_TRACE
#define HZ4_MACOS_MALLOC_SIZE_TRACE 0
#endif

static void hz4_macos_malloc_size_trace(const char* path, void* ptr, int zone_owned, uint32_t magic, size_t out);

typedef struct {
    _Atomic uint64_t calls;
    _Atomic uint64_t hz4_hit;
    _Atomic uint64_t system_fallback;
    _Atomic uint64_t segment_unknown;
    _Atomic uint64_t zero_return;
} hz4_macos_malloc_size_stats_t;

static hz4_macos_malloc_size_stats_t g_hz4_macos_malloc_size_stats;
#if HZ4_MACOS_MALLOC_SIZE_TRACE
static _Atomic uint32_t g_hz4_macos_malloc_size_trace_count = ATOMIC_VAR_INIT(0);
#endif

static void hz4_macos_malloc_size_stats_dump(void) {
    uint64_t calls = atomic_load_explicit(&g_hz4_macos_malloc_size_stats.calls, memory_order_relaxed);
    uint64_t hz4_hit = atomic_load_explicit(&g_hz4_macos_malloc_size_stats.hz4_hit, memory_order_relaxed);
    uint64_t system_fallback =
        atomic_load_explicit(&g_hz4_macos_malloc_size_stats.system_fallback, memory_order_relaxed);
    uint64_t segment_unknown =
        atomic_load_explicit(&g_hz4_macos_malloc_size_stats.segment_unknown, memory_order_relaxed);
    uint64_t zero_return = atomic_load_explicit(&g_hz4_macos_malloc_size_stats.zero_return, memory_order_relaxed);
    if (calls == 0) {
        return;
    }
    fprintf(stderr,
            "[HZ4_MACOS_MALLOC_SIZE] calls=%llu hz4_hit=%llu system_fallback=%llu segment_unknown=%llu zero_return=%llu\n",
            (unsigned long long)calls,
            (unsigned long long)hz4_hit,
            (unsigned long long)system_fallback,
            (unsigned long long)segment_unknown,
            (unsigned long long)zero_return);
}

__attribute__((constructor))
static void hz4_macos_malloc_size_stats_init(void) {
    atexit(hz4_macos_malloc_size_stats_dump);
}

static void hz4_macos_malloc_size_trace(const char* path, void* ptr, int zone_owned, uint32_t magic, size_t out) {
#if HZ4_MACOS_MALLOC_SIZE_TRACE
    uint32_t shot = atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_trace_count, 1, memory_order_relaxed);
    if (shot >= 8) {
        return;
    }
    dprintf(STDERR_FILENO,
            "[HZ4_MACOS_MALLOC_SIZE_TRACE] n=%u path=%s ptr=%p zone=%d magic=0x%08x out=%zu\n",
            (unsigned)shot,
            path,
            ptr,
            zone_owned,
            magic,
            out);
#else
    (void)path;
    (void)ptr;
    (void)zone_owned;
    (void)magic;
    (void)out;
#endif
}

size_t malloc_usable_size(void* ptr) {
    atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.calls, 1, memory_order_relaxed);

    hz4_macos_foreign_size_kind_t kind = HZ4_MACOS_FOREIGN_SIZE_NULL;
    uint32_t magic = 0;
    size_t out = hz4_macos_foreign_malloc_size(ptr, &kind, &magic);

    switch (kind) {
    case HZ4_MACOS_FOREIGN_SIZE_NULL:
        atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.zero_return, 1, memory_order_relaxed);
        return 0;
    case HZ4_MACOS_FOREIGN_SIZE_HZ4:
        if (out != 0) {
            atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.hz4_hit, 1, memory_order_relaxed);
            return out;
        }
        atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.segment_unknown, 1, memory_order_relaxed);
        hz4_macos_malloc_size_trace("segment-unknown", ptr, 0, magic, 0);
        return 0;
    case HZ4_MACOS_FOREIGN_SIZE_SEGMENT_UNKNOWN:
        atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.segment_unknown, 1, memory_order_relaxed);
        hz4_macos_malloc_size_trace("segment-unknown", ptr, 0, magic, 0);
        return 0;
    case HZ4_MACOS_FOREIGN_SIZE_SYSTEM:
        atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.system_fallback, 1, memory_order_relaxed);
        if (out == 0) {
            atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.zero_return, 1, memory_order_relaxed);
            hz4_macos_malloc_size_trace("not-owned-zero", ptr, 1, magic, 0);
        } else {
            hz4_macos_malloc_size_trace("not-owned-system", ptr, 1, magic, out);
        }
        return out;
    }

    atomic_fetch_add_explicit(&g_hz4_macos_malloc_size_stats.zero_return, 1, memory_order_relaxed);
    return 0;
}

static size_t hz4_macos_malloc_size(void* ptr) {
    return malloc_usable_size(ptr);
}

size_t hz4_macos_debug_malloc_size(void* ptr) {
    return malloc_usable_size(ptr);
}

static inline int hz4_macos_align_ok(size_t alignment) {
    if (alignment < sizeof(void*)) {
        return 0;
    }
    return (alignment & (alignment - 1u)) == 0;
}

static void* hz4_macos_malloc(size_t size) {
    if (size == 0) {
        size = HZ4_SIZE_MIN;
    }
    return hz4_malloc(size);
}

static void hz4_macos_free(void* ptr) {
    hz4_macos_foreign_free(ptr);
}

static void* hz4_macos_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return hz4_malloc(0);
    }
    if (nmemb > ((size_t)-1) / size) {
        return NULL;
    }
    size_t total = nmemb * size;
    void* p = hz4_malloc(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}

static void* hz4_macos_realloc(void* ptr, size_t size) {
    return hz4_macos_foreign_realloc(ptr, size);
}

static int hz4_macos_posix_memalign(void** memptr, size_t alignment, size_t size) {
    if (!memptr || !hz4_macos_align_ok(alignment)) {
        return EINVAL;
    }
    if (alignment <= HZ4_SIZE_ALIGN) {
        void* p = hz4_malloc(size);
        if (!p) {
            return ENOMEM;
        }
        *memptr = p;
        return 0;
    }

    size_t overhead = (alignment - 1u) + sizeof(hz4_aligned_hdr_t);
    if (size > (SIZE_MAX - overhead)) {
        return ENOMEM;
    }

    void* raw = hz4_malloc(size + overhead);
    if (!raw) {
        return ENOMEM;
    }

    uintptr_t candidate = (uintptr_t)raw + sizeof(hz4_aligned_hdr_t);
    uintptr_t aligned = (candidate + (alignment - 1u)) & ~(uintptr_t)(alignment - 1u);
    hz4_aligned_hdr_t* hdr = (hz4_aligned_hdr_t*)(aligned - sizeof(hz4_aligned_hdr_t));
    hz4_macos_aligned_write(hdr, raw, (void*)aligned, size);
    *memptr = (void*)aligned;
    return 0;
}

static void* hz4_macos_memalign_impl(size_t alignment, size_t size) {
    void* p = NULL;
    int rc = hz4_macos_posix_memalign(&p, alignment, size);
    if (rc != 0) {
        errno = rc;
        return NULL;
    }
    return p;
}

static void* hz4_macos_aligned_alloc(size_t alignment, size_t size) {
    if ((alignment & (alignment - 1u)) != 0 || alignment == 0) {
        errno = EINVAL;
        return NULL;
    }
    if ((size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }
    return hz4_macos_memalign_impl(alignment, size);
}

static void* hz4_macos_valloc(size_t size) {
    void* p = NULL;
    int rc = hz4_macos_posix_memalign(&p, HZ4_PAGE_SIZE, size);
    if (rc != 0) {
        errno = rc;
        return NULL;
    }
    return p;
}

void* pvalloc(size_t size) {
    size_t page_sz = HZ4_PAGE_SIZE;
    size = (size + page_sz - 1) & ~(page_sz - 1);
    return hz4_macos_valloc(size);
}

void* memalign(size_t alignment, size_t size) {
    return hz4_macos_memalign_impl(alignment, size);
}

#define HZ4_DYLD_INTERPOSE(replacement, replacee) \
    __attribute__((used)) static const struct { const void* replacement; const void* replacee; } \
        _hz4_interpose_##replacement##_##replacee \
        __attribute__((section("__DATA_CONST,__interpose"))) = { \
            (const void*)(uintptr_t)&replacement, (const void*)(uintptr_t)&replacee \
        }

HZ4_DYLD_INTERPOSE(hz4_macos_malloc, malloc);
HZ4_DYLD_INTERPOSE(hz4_macos_free, free);
HZ4_DYLD_INTERPOSE(hz4_macos_calloc, calloc);
HZ4_DYLD_INTERPOSE(hz4_macos_realloc, realloc);
HZ4_DYLD_INTERPOSE(hz4_macos_posix_memalign, posix_memalign);
HZ4_DYLD_INTERPOSE(hz4_macos_aligned_alloc, aligned_alloc);
HZ4_DYLD_INTERPOSE(hz4_macos_valloc, valloc);
#if HZ4_MACOS_INTERPOSE_MALLOC_SIZE
HZ4_DYLD_INTERPOSE(hz4_macos_malloc_size, malloc_size);
#endif

#endif  // __APPLE__
