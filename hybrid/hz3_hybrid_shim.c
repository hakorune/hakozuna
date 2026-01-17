#define _GNU_SOURCE

/*
 * hz3_hybrid_shim.c - Day 8 Hybrid LD_PRELOAD shim
 *
 * Routes allocations to the best allocator:
 * - 16B-2048B: hakozuna small (hz_malloc)
 * - 2049B-4095B: hakozuna large (hz_large_alloc)
 * - 4KB-32KB: hz3 (hz3_malloc) - our winning range!
 * - >32KB: hakozuna large (hz_large_alloc)
 *
 * free() uses hz3_segmap_get + sc_tag to determine ownership.
 *
 * BOOTSTRAP MODE:
 * To avoid pthread/TLS deadlock during early initialization (e.g., when
 * pthread_create internally calls malloc), we use a bootstrap allocator
 * that falls back to libc malloc via RTLD_NEXT.
 */

#include "hz3.h"
#include "hz3_types.h"
#include "hz3_segmap.h"
#include "hz3_sc.h"

// hakozuna headers
#include "hz_export.h"
#include "hz_size_class.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

#ifndef HZ3_HYBRID_FREE_FAST_REJECT
#define HZ3_HYBRID_FREE_FAST_REJECT 1
#endif

#ifndef HZ3_HYBRID_STATS
#define HZ3_HYBRID_STATS 0
#endif

#if HZ3_HYBRID_STATS
static _Atomic uint64_t g_hybrid_free_calls = 0;
static _Atomic uint64_t g_hybrid_free_fast_reject = 0;
static _Atomic uint64_t g_hybrid_free_segmap_get_calls = 0;
static _Atomic uint64_t g_hybrid_free_hz3_hits = 0;

static inline void hybrid_stats_inc(_Atomic uint64_t* c) {
    atomic_fetch_add_explicit(c, 1, memory_order_relaxed);
}

static void hybrid_stats_dump(void) {
    fprintf(stderr,
            "hybrid_stats: free_calls=%llu free_fast_reject=%llu free_segmap_get_calls=%llu free_hz3_hits=%llu\n",
            (unsigned long long)atomic_load_explicit(&g_hybrid_free_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hybrid_free_fast_reject, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hybrid_free_segmap_get_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_hybrid_free_hz3_hits, memory_order_relaxed));
}
#endif

/*
 * STATIC BUFFER BOOTSTRAP ALLOCATOR
 *
 * Problem: pthread_create calls malloc internally, but our allocators use TLS.
 * TLS isn't ready during pthread initialization, causing deadlock.
 *
 * Solution: Use a static buffer for the first few allocations. This requires
 * NO external dependencies (no TLS, no dlsym, no libc). Only after a
 * constructor runs do we switch to the real allocators.
 *
 * State machine:
 * 0 = bootstrap mode (use static buffer)
 * 1 = ready (use hybrid allocator)
 */
static _Atomic int g_hybrid_ready = 0;

// Static bootstrap buffer - large enough to survive libc/pthread early init.
// This is .bss, so it should be mostly demand-zero pages.
#define BOOTSTRAP_BUF_SIZE (1024 * 1024)
static char g_bootstrap_buf[BOOTSTRAP_BUF_SIZE] __attribute__((aligned(16)));
static _Atomic size_t g_bootstrap_offset = 0;

// Track bootstrap buffer range for free() routing
#define BOOTSTRAP_BUF_START ((uintptr_t)g_bootstrap_buf)
#define BOOTSTRAP_BUF_END   (BOOTSTRAP_BUF_START + BOOTSTRAP_BUF_SIZE)

// Check if pointer is from bootstrap buffer
static inline int is_bootstrap_ptr(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (addr >= BOOTSTRAP_BUF_START && addr < BOOTSTRAP_BUF_END);
}

typedef struct {
    uint32_t magic;
    uint32_t size;
} HybridBootstrapHdr;

#define HYBRID_BOOTSTRAP_HDR_MAGIC 0x485A3342u  // "HZ3B"

static inline uintptr_t hz_align_up_uintptr(uintptr_t v, size_t align) {
    return (v + (uintptr_t)align - 1u) & ~((uintptr_t)align - 1u);
}

static HybridBootstrapHdr* bootstrap_hdr_from_ptr(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    if (addr < BOOTSTRAP_BUF_START + sizeof(uintptr_t)) {
        return NULL;
    }
    if (addr >= BOOTSTRAP_BUF_END) {
        return NULL;
    }

    uintptr_t hdr_addr = *((uintptr_t*)(addr - sizeof(uintptr_t)));
    if (hdr_addr < BOOTSTRAP_BUF_START || hdr_addr + sizeof(HybridBootstrapHdr) > BOOTSTRAP_BUF_END) {
        return NULL;
    }

    HybridBootstrapHdr* hdr = (HybridBootstrapHdr*)hdr_addr;
    if (hdr->magic != HYBRID_BOOTSTRAP_HDR_MAGIC) {
        return NULL;
    }
    return hdr;
}

static size_t bootstrap_usable_size(void* ptr) {
    HybridBootstrapHdr* hdr = bootstrap_hdr_from_ptr(ptr);
    return hdr ? (size_t)hdr->size : 0;
}

// Allocate from static buffer (bump allocator, never freed)
static void* bootstrap_alloc_aligned(size_t alignment, size_t size) {
    if (alignment < 16) alignment = 16;
    if ((alignment & (alignment - 1u)) != 0) return NULL;

    // Ensure we can always store the back-pointer just before the returned ptr.
    size_t need = sizeof(HybridBootstrapHdr) + sizeof(uintptr_t);
    if (size > (SIZE_MAX - need - alignment)) return NULL;
    need += size + alignment;

    size_t bump = (need + 15u) & ~((size_t)15u);
    size_t offset = atomic_fetch_add(&g_bootstrap_offset, bump);
    if (offset + need > BOOTSTRAP_BUF_SIZE) {
        return NULL;
    }

    uintptr_t base = (uintptr_t)(g_bootstrap_buf + offset);
    HybridBootstrapHdr* hdr = (HybridBootstrapHdr*)base;
    hdr->magic = HYBRID_BOOTSTRAP_HDR_MAGIC;
    hdr->size = (uint32_t)((size > UINT32_MAX) ? UINT32_MAX : size);

    uintptr_t payload = (uintptr_t)(hdr + 1);
    uintptr_t p = hz_align_up_uintptr(payload + sizeof(uintptr_t), alignment);
    *((uintptr_t*)(p - sizeof(uintptr_t))) = (uintptr_t)hdr;
    return (void*)p;
}

static void* bootstrap_alloc(size_t size) {
    if (size == 0) size = 16;
    return bootstrap_alloc_aligned(16, size);
}

// Constructor: mark allocator as ready AFTER TLS is initialized
__attribute__((constructor(101)))  // Run early but after libc init
static void hybrid_init_constructor(void) {
    // At this point, TLS should be ready
    atomic_store(&g_hybrid_ready, 1);
#if HZ3_HYBRID_STATS
    atexit(hybrid_stats_dump);
#endif
}

static inline int hybrid_is_ready(void) {
    return atomic_load(&g_hybrid_ready) == 1;
}

// External hakozuna functions
extern void* hz_malloc(size_t size);
extern void  hz_free(void* ptr);
extern void* hz_large_alloc(size_t size);
extern size_t hz_usable_size(const void* ptr);

// hz_realloc implementation (since we don't link hz_shim.o)
static void* hz_realloc_impl(void* ptr, size_t size) {
    if (!ptr) return hz_malloc(size);
    if (size == 0) {
        hz_free(ptr);
        return NULL;
    }

    size_t old_size = hz_usable_size(ptr);
    if (old_size == 0) {
        // Unknown pointer - this shouldn't happen in hybrid mode
        // since we route all frees through our dispatcher
        return NULL;
    }

    // Allocate new, copy, free old
    void* newptr;
    if (size > HZ_SIZE_CLASS_MAX) {
        newptr = hz_large_alloc(size);
    } else {
        newptr = hz_malloc(size);
    }
    if (!newptr) return NULL;

    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(newptr, ptr, copy_size);
    hz_free(ptr);
    return newptr;
}

// External hz3 functions (declared in hz3.h)
// hz3_malloc, hz3_free, hz3_calloc, hz3_realloc, hz3_usable_size

// Routing constants: hz3 handles 4KB-32KB
#define HZ3_HYBRID_MIN  HZ3_SC_MIN_SIZE   // 4096
#define HZ3_HYBRID_MAX  HZ3_SC_MAX_SIZE   // 32768

// Helper: check if pointer belongs to hz3 (segmap + sc_tag)
static inline int hz3_hybrid_is_hz3_ptr(void* ptr) {
    Hz3SegMeta* meta = hz3_segmap_get(ptr);
    if (meta) {
        uint32_t page_idx = ((uintptr_t)ptr & (HZ3_SEG_SIZE - 1)) >> HZ3_PAGE_SHIFT;
        return (meta->sc_tag[page_idx] != 0);
    }
    return 0;
}

// malloc: route by size (with bootstrap fallback)
HZ_EXPORT void* malloc(size_t size) {
    // Bootstrap mode: use static buffer until constructor runs
    if (!hybrid_is_ready()) {
        return bootstrap_alloc(size);
    }

    // hz3 range: 4KB-32KB
    if (size >= HZ3_HYBRID_MIN && size <= HZ3_HYBRID_MAX) {
        return hz3_malloc(size);
    }
    // hakozuna: small (16-2048) or large (>2048)
    if (size > HZ_SIZE_CLASS_MAX) {
        return hz_large_alloc(size);
    }
    return hz_malloc(size);
}

// free: determine ownership via segmap + sc_tag (with bootstrap fallback)
HZ_EXPORT void free(void* ptr) {
    if (!ptr) return;

    if (!hybrid_is_ready()) {
        // During early init, avoid touching hz/hz3 state (TLS/pthread).
        // We only "free" bootstrap pointers (which are never actually freed).
        return;
    }

    // Bootstrap pointers are from static buffer - never freed
    if (is_bootstrap_ptr(ptr)) {
        return;  // Static memory, nothing to do
    }

#if HZ3_HYBRID_STATS
    hybrid_stats_inc(&g_hybrid_free_calls);
#endif

#if HZ3_HYBRID_FREE_FAST_REJECT
    if (!hz3_segmap_has_l2(ptr)) {
#if HZ3_HYBRID_STATS
        hybrid_stats_inc(&g_hybrid_free_fast_reject);
#endif
        hz_free(ptr);
        return;
    }
#endif

    // Normal operation: route by ownership
#if HZ3_HYBRID_STATS
    hybrid_stats_inc(&g_hybrid_free_segmap_get_calls);
#endif
    if (hz3_hybrid_is_hz3_ptr(ptr)) {
#if HZ3_HYBRID_STATS
        hybrid_stats_inc(&g_hybrid_free_hz3_hits);
#endif
        hz3_free(ptr);
    } else {
        hz_free(ptr);
    }
}

// calloc: overflow check, then route via malloc (with bootstrap fallback)
HZ_EXPORT void* calloc(size_t nmemb, size_t size) {
    if (nmemb && size > (SIZE_MAX / nmemb)) {
        return NULL;
    }
    size_t total = nmemb * size;

    // Bootstrap mode: use static buffer (already zeroed on first use)
    if (!hybrid_is_ready()) {
        void* p = bootstrap_alloc(total);
        if (p) memset(p, 0, total);
        return p;
    }

    void* p = malloc(total);
    // Compiler barrier prevents optimizer from reordering malloc result.
    // Without this, -O2/-O3 can cause hangs in certain call patterns.
    __asm__ __volatile__("" ::: "memory");
    if (p) {
        memset(p, 0, total);
    }
    return p;
}

// realloc: cross-allocator aware (with bootstrap fallback)
HZ_EXPORT void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    if (!hybrid_is_ready()) {
        // Early init: only support bootstrap pointers safely.
        if (is_bootstrap_ptr(ptr)) {
            size_t old_size = bootstrap_usable_size(ptr);
            void* newptr = bootstrap_alloc(size);
            if (newptr) {
                size_t copy_size = (old_size < size) ? old_size : size;
                if (copy_size) memcpy(newptr, ptr, copy_size);
            }
            return newptr;
        }
        // Unknown pointer during early init: allocate new and leak old.
        return bootstrap_alloc(size);
    }

    // Bootstrap pointer: allocate new, copy, return (can't free static memory)
    if (is_bootstrap_ptr(ptr)) {
        void* newptr = malloc(size);  // Uses routing logic
        if (newptr) {
            size_t old_size = bootstrap_usable_size(ptr);
            size_t copy_size = (old_size < size) ? old_size : size;
            if (copy_size) memcpy(newptr, ptr, copy_size);
        }
        return newptr;  // Old bootstrap memory is leaked (acceptable for early init)
    }

    // Determine source allocator
    int from_hz3 = hz3_hybrid_is_hz3_ptr(ptr);

    // Determine target allocator
    int to_hz3 = (size >= HZ3_HYBRID_MIN && size <= HZ3_HYBRID_MAX);

    // Same allocator: delegate directly
    if (from_hz3 && to_hz3) {
        return hz3_realloc(ptr, size);
    }
    if (!from_hz3 && !to_hz3) {
        return hz_realloc_impl(ptr, size);
    }

    // Cross-allocator: copy and move
    size_t old_size = from_hz3 ? hz3_usable_size(ptr) : hz_usable_size(ptr);
    if (old_size == 0) {
        // Unknown pointer - delegate to hakozuna (safe fallback)
        return hz_realloc_impl(ptr, size);
    }

    void* newptr;
    if (to_hz3) {
        newptr = hz3_malloc(size);
    } else if (size > HZ_SIZE_CLASS_MAX) {
        newptr = hz_large_alloc(size);
    } else {
        newptr = hz_malloc(size);
    }

    if (newptr) {
        size_t copy_size = (old_size < size) ? old_size : size;
        memcpy(newptr, ptr, copy_size);
        if (from_hz3) {
            hz3_free(ptr);
        } else {
            hz_free(ptr);
        }
    }
    return newptr;
}

HZ_EXPORT size_t malloc_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }

    if (!hybrid_is_ready()) {
        return 0;
    }

    if (is_bootstrap_ptr(ptr)) {
        return 0;
    }

    return hz3_hybrid_is_hz3_ptr(ptr) ? hz3_usable_size(ptr) : hz_usable_size(ptr);
}

// posix_memalign: use malloc for small alignments, large_alloc for big ones
HZ_EXPORT int posix_memalign(void** memptr, size_t alignment, size_t size) {
    extern void* hz_large_aligned_alloc(size_t alignment, size_t size);

    // Validate alignment
    if ((alignment & (alignment - 1u)) != 0 || alignment < sizeof(void*)) {
        return EINVAL;
    }

    if (!hybrid_is_ready()) {
        void* p = bootstrap_alloc_aligned(alignment, size);
        if (!p) return ENOMEM;
        *memptr = p;
        return 0;
    }

    void* p;
    if (alignment <= 16) {
        // malloc provides 16-byte alignment naturally
        p = malloc(size);
    } else {
        // Large alignment needs special handling
        p = hz_large_aligned_alloc(alignment, size);
    }

    if (!p) {
        return ENOMEM;
    }
    *memptr = p;
    return 0;
}

// aligned_alloc: similar to posix_memalign
HZ_EXPORT void* aligned_alloc(size_t alignment, size_t size) {
    extern void* hz_large_aligned_alloc(size_t alignment, size_t size);

    // Validate alignment (power of 2)
    if ((alignment & (alignment - 1u)) != 0) {
        return NULL;
    }

    // C11 requires size to be multiple of alignment
    if (alignment && (size % alignment) != 0) {
        return NULL;
    }

    if (!hybrid_is_ready()) {
        return bootstrap_alloc_aligned(alignment, size);
    }

    if (alignment <= 16) {
        return malloc(size);
    }
    return hz_large_aligned_alloc(alignment, size);
}
