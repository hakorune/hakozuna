// hz4_large.c - HZ4 Large Box (>~64KB, mmap)
#include <stdint.h>
#include <stdlib.h>

#include "hz4_config.h"
#include "hz4_sizeclass.h"
#include "hz4_os.h"
#include "hz4_large.h"

typedef struct hz4_large_header {
    uint32_t magic;
    uint32_t _pad0;
    size_t size;
    size_t total;
} hz4_large_header_t;

static inline size_t hz4_large_hdr_size(void) {
    return hz4_align_up(sizeof(hz4_large_header_t), HZ4_SIZE_ALIGN);
}

static inline size_t hz4_large_total_size(size_t size) {
    size_t hdr = hz4_large_hdr_size();
    size_t total = hdr + size;
    return hz4_align_up(total, HZ4_PAGE_SIZE);
}

void* hz4_large_malloc(size_t size) {
    size_t hdr = hz4_large_hdr_size();
    size_t total = hz4_large_total_size(size);

    void* base = hz4_os_large_acquire(total);
    if (!base) {
        abort();
    }

    hz4_large_header_t* h = (hz4_large_header_t*)base;
    h->magic = HZ4_LARGE_MAGIC;
    h->size = size;
    h->total = total;

    return (void*)((uint8_t*)base + hdr);
}

void hz4_large_free(void* ptr) {
    if (!ptr) {
        return;
    }

    // A案: header は ptr 直前に固定配置
    size_t hdr = hz4_large_hdr_size();
    hz4_large_header_t* h = (hz4_large_header_t*)((uint8_t*)ptr - hdr);
    if (h->magic != HZ4_LARGE_MAGIC) {
        HZ4_FAIL("hz4_large_free: invalid header");
        abort();
    }

    hz4_os_large_release(h, h->total);
}

size_t hz4_large_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    // A案: header は ptr 直前に固定配置
    size_t hdr = hz4_large_hdr_size();
    hz4_large_header_t* h = (hz4_large_header_t*)((uint8_t*)ptr - hdr);
    if (h->magic != HZ4_LARGE_MAGIC) {
        HZ4_FAIL("hz4_large_usable_size: invalid header");
        abort();
    }
    return h->size;
}
