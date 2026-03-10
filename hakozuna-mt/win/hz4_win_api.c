#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hz4_tcache.h"
#include "hz4_mid.h"
#include "hz4_large.h"
#include "hz4_sizeclass.h"
#include "hz4_config.h"
#include "hz4_page.h"

#include "hz4_win_api.h"

typedef struct hz4_win_aligned_hdr {
    uint64_t magic;
    uintptr_t raw;
    uintptr_t cookie;
    size_t requested;
} hz4_win_aligned_hdr_t;

#define HZ4_WIN_ALIGNED_HDR_MAGIC UINT64_C(0x483457494e484452)
#define HZ4_WIN_ALIGNED_HDR_COOKIE UINT64_C(0x9e3779b97f4a7c15)

static inline hz4_win_aligned_hdr_t* hz4_win_aligned_hdr_from_ptr(void* ptr) {
    return (hz4_win_aligned_hdr_t*)((uintptr_t)ptr - sizeof(hz4_win_aligned_hdr_t));
}

static inline uintptr_t hz4_win_aligned_cookie(uintptr_t raw, uintptr_t aligned) {
    return raw ^ aligned ^ (uintptr_t)HZ4_WIN_ALIGNED_HDR_COOKIE;
}

static inline int hz4_win_aligned_decode(void* ptr, void** raw_out, size_t* req_out) {
    if (!ptr) {
        return 0;
    }
    hz4_win_aligned_hdr_t* hdr = hz4_win_aligned_hdr_from_ptr(ptr);
    if (hdr->magic != HZ4_WIN_ALIGNED_HDR_MAGIC) {
        return 0;
    }
    uintptr_t aligned = (uintptr_t)ptr;
    uintptr_t raw = hdr->raw;
    if (raw == 0 || hdr->cookie != hz4_win_aligned_cookie(raw, aligned)) {
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

size_t hz4_win_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    {
        size_t requested = 0;
        if (hz4_win_aligned_decode(ptr, NULL, &requested)) {
            return requested;
        }
    }

    if (hz4_large_header_valid(ptr)) {
        return hz4_large_usable_size(ptr);
    }

    {
        uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1);
        uint32_t head_magic = *(uint32_t*)base;
        if (head_magic == HZ4_MID_MAGIC) {
            return hz4_mid_usable_size(ptr);
        }
        if (head_magic == HZ4_LARGE_MAGIC) {
            return hz4_large_usable_size(ptr);
        }
    }

    {
        hz4_page_t* page = hz4_page_from_ptr(ptr);
        if (hz4_page_valid(page)) {
            return hz4_small_usable_size(ptr);
        }
    }

    HZ4_FAIL("hz4_win_usable_size: unknown pointer");
    abort();
}

void* hz4_win_malloc(size_t size) {
    if (size == 0) {
        size = HZ4_SIZE_MIN;
    }
    return hz4_malloc(size);
}

void hz4_win_free(void* ptr) {
    void* raw = NULL;
    if (hz4_win_aligned_decode(ptr, &raw, NULL)) {
        hz4_free(raw);
        return;
    }
    hz4_free(ptr);
}

void* hz4_win_realloc(void* ptr, size_t size) {
    if (!ptr) {
        return hz4_win_malloc(size);
    }
    if (size == 0) {
        hz4_win_free(ptr);
        return NULL;
    }

    {
        size_t old_size = hz4_win_usable_size(ptr);
        void* next = hz4_win_malloc(size);
        if (!next) {
            return NULL;
        }

        {
            size_t copy = old_size < size ? old_size : size;
            if (copy) {
                memcpy(next, ptr, copy);
            }
        }
        hz4_win_free(ptr);
        return next;
    }
}
