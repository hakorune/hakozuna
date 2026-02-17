// hz4_shim.c - HZ4 malloc/free exports (LD_PRELOAD)
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hz4_tcache.h"
#include "hz4_mid.h"
#include "hz4_large.h"
#include "hz4_sizeclass.h"
#include "hz4_config.h"
#include "hz4_page.h"

typedef struct hz4_aligned_hdr {
    uint64_t magic;
    uintptr_t raw;
    uintptr_t cookie;
    size_t requested;
} hz4_aligned_hdr_t;

#define HZ4_ALIGNED_HDR_MAGIC UINT64_C(0x48345a414c4e4844) /* "H4ZALNHD" */
#define HZ4_ALIGNED_HDR_COOKIE UINT64_C(0x9e3779b97f4a7c15)

static inline hz4_aligned_hdr_t* hz4_aligned_hdr_from_ptr(void* ptr) {
    return (hz4_aligned_hdr_t*)((uintptr_t)ptr - sizeof(hz4_aligned_hdr_t));
}

static inline uintptr_t hz4_aligned_cookie(uintptr_t raw, uintptr_t aligned) {
    return raw ^ aligned ^ (uintptr_t)HZ4_ALIGNED_HDR_COOKIE;
}

static inline int hz4_aligned_decode(void* ptr, void** raw_out, size_t* req_out) {
    if (!ptr) {
        return 0;
    }
    hz4_aligned_hdr_t* hdr = hz4_aligned_hdr_from_ptr(ptr);
    if (hdr->magic != HZ4_ALIGNED_HDR_MAGIC) {
        return 0;
    }
    uintptr_t aligned = (uintptr_t)ptr;
    uintptr_t raw = hdr->raw;
    if (raw == 0 || hdr->cookie != hz4_aligned_cookie(raw, aligned)) {
        return 0;
    }
    if (raw_out) {
        *raw_out = (void*)raw;
    }
    if (req_out) {
        *req_out = hdr->requested;
    }
    return 1;
}

static size_t hz4_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    {
        size_t requested = 0;
        if (hz4_aligned_decode(ptr, NULL, &requested)) {
            return requested;
        }
    }

    if (hz4_large_header_valid(ptr)) {
        return hz4_large_usable_size(ptr);
    }

    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
    uint32_t head_magic = *(uint32_t*)base;

    if (head_magic == HZ4_MID_MAGIC) {
        return hz4_mid_usable_size(ptr);
    }
    if (head_magic == HZ4_LARGE_MAGIC) {
        return hz4_large_usable_size(ptr);
    }

    hz4_page_t* page = hz4_page_from_ptr(ptr);
    if (hz4_page_valid(page)) {
        return hz4_small_usable_size(ptr);
    }

    HZ4_FAIL("hz4_usable_size: unknown pointer");
    abort();
}

void* malloc(size_t size) {
    if (size == 0) {
        size = HZ4_SIZE_MIN;
    }
    return hz4_malloc(size);
}

void free(void* ptr) {
    void* raw = NULL;
    if (hz4_aligned_decode(ptr, &raw, NULL)) {
        hz4_free(raw);
        return;
    }
    hz4_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
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

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return hz4_malloc(size);
    }
    if (size == 0) {
        hz4_free(ptr);
        return NULL;
    }

    size_t old_size = hz4_usable_size(ptr);
    void* next = hz4_malloc(size);
    if (!next) {
        return NULL;
    }

    size_t copy = old_size < size ? old_size : size;
    if (copy) {
        memcpy(next, ptr, copy);
    }
    hz4_free(ptr);
    return next;
}

size_t malloc_usable_size(void* ptr) {
    return hz4_usable_size(ptr);
}

static inline int hz4_align_ok(size_t alignment) {
    if (alignment < sizeof(void*)) {
        return 0;
    }
    return (alignment & (alignment - 1u)) == 0;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    if (!memptr || !hz4_align_ok(alignment)) {
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

    /* Over-allocate and align manually; free() recovers the original pointer. */
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
    hdr->magic = HZ4_ALIGNED_HDR_MAGIC;
    hdr->raw = (uintptr_t)raw;
    hdr->cookie = hz4_aligned_cookie((uintptr_t)raw, aligned);
    hdr->requested = size;
    *memptr = (void*)aligned;
    return 0;
}

void* memalign(size_t alignment, size_t size) {
    void* p = NULL;
    int rc = posix_memalign(&p, alignment, size);
    if (rc != 0) {
        errno = rc;
        return NULL;
    }
    return p;
}

void* aligned_alloc(size_t alignment, size_t size) {
    if ((alignment & (alignment - 1u)) != 0 || alignment == 0) {
        errno = EINVAL;
        return NULL;
    }
    if ((size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }
    return memalign(alignment, size);
}

void* valloc(size_t size) {
    void* p = NULL;
    int rc = posix_memalign(&p, HZ4_PAGE_SIZE, size);
    if (rc != 0) {
        errno = rc;
        return NULL;
    }
    return p;
}

void* pvalloc(size_t size) {
    size_t page_sz = HZ4_PAGE_SIZE;
    size = (size + page_sz - 1) & ~(page_sz - 1);
    return valloc(size);
}
