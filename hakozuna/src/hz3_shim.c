#define _GNU_SOURCE

#include "hz3.h"
#include "hz3_config.h"
#include "hz3_large.h"
#include "hz3_types.h"

#include <errno.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// LD_PRELOAD entry points for hz3.
// Note: Until hz3 is fully implemented, keep HZ3_SHIM_FORWARD_ONLY=1 so this
// library is safe to preload.

static inline int hz3_is_pow2(size_t x) {
    return x && ((x & (x - 1u)) == 0);
}

// ShimNullReturnGuardBox (debug-only)
// mstress (and some benchmarks) can segfault if realloc/malloc returns NULL without being checked.
// This box turns "NULL return" into an immediate, one-shot log (and optional abort) so OOM vs
// corruption is unambiguous.
#ifndef HZ3_SHIM_NULL_RETURN_GUARD
#define HZ3_SHIM_NULL_RETURN_GUARD 0
#endif

#ifndef HZ3_SHIM_NULL_RETURN_GUARD_FAILFAST
#define HZ3_SHIM_NULL_RETURN_GUARD_FAILFAST HZ3_SHIM_NULL_RETURN_GUARD
#endif

#ifndef HZ3_SHIM_NULL_RETURN_GUARD_SHOT
#define HZ3_SHIM_NULL_RETURN_GUARD_SHOT 1
#endif

#if HZ3_SHIM_NULL_RETURN_GUARD
#if HZ3_SHIM_NULL_RETURN_GUARD_SHOT
static _Atomic int g_hz3_shim_null_return_guard_shot = 0;
#else
static _Atomic int g_hz3_shim_null_return_guard_shot __attribute__((unused)) = 0;
#endif

static inline int hz3_shim_null_return_guard_should_log(void) {
#if HZ3_SHIM_NULL_RETURN_GUARD_SHOT
    return (atomic_exchange_explicit(&g_hz3_shim_null_return_guard_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}

static inline void hz3_shim_null_return_guard(const char* where, void* ptr, size_t size) {
    if (!hz3_shim_null_return_guard_should_log()) {
        return;
    }
    fprintf(stderr, "[HZ3_SHIM_NULL_RETURN] where=%s ptr=%p size=%zu errno=%d\n", where, ptr, size, errno);
#if HZ3_SHIM_NULL_RETURN_GUARD_FAILFAST
    abort();
#endif
}
#endif  // HZ3_SHIM_NULL_RETURN_GUARD

void* malloc(size_t size) {
    void* p = hz3_malloc(size);
#if HZ3_SHIM_NULL_RETURN_GUARD
    if (!p && size != 0) {
        hz3_shim_null_return_guard("malloc", NULL, size);
    }
#endif
    return p;
}

void free(void* ptr) {
    hz3_free(ptr);
}

size_t malloc_usable_size(void* ptr) {
    return hz3_usable_size(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    void* p = hz3_calloc(nmemb, size);
#if HZ3_SHIM_NULL_RETURN_GUARD
    size_t total = 0;
    if (nmemb != 0 && size != 0) {
        total = nmemb * size;
        if (total / nmemb != size) {
            total = SIZE_MAX;  // overflow (best-effort)
        }
    }
    if (!p && total != 0) {
        hz3_shim_null_return_guard("calloc", NULL, total);
    }
#endif
    return p;
}

void* realloc(void* ptr, size_t size) {
    void* p = hz3_realloc(ptr, size);
#if HZ3_SHIM_NULL_RETURN_GUARD
    // realloc(ptr, 0) may return NULL (after free) by definition; not an OOM signal.
    if (!p && size != 0) {
        hz3_shim_null_return_guard("realloc", ptr, size);
    }
#endif
    return p;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    if (!hz3_is_pow2(alignment) || alignment < sizeof(void*)) {
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

void* aligned_alloc(size_t alignment, size_t size) {
    if (!hz3_is_pow2(alignment) || alignment == 0) {
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
    if (!hz3_is_pow2(alignment) || alignment < sizeof(void*)) {
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
