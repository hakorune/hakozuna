// hz4_shim.c - HZ4 malloc/free exports (LD_PRELOAD)
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

static size_t hz4_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
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
