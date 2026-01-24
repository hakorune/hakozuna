// hz4_os.c - HZ4 OS Box (segment acquire/release)
#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hz4_config.h"
#include "hz4_os.h"
#if HZ4_PAGE_TAG_TABLE
#include "hz4_pagetag.h"
#endif

static void* hz4_os_mmap_aligned(size_t size, size_t align) {
    size_t len = size + align;
    void* base = mmap(NULL, len, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        return NULL;
    }

    uintptr_t addr = (uintptr_t)base;
    uintptr_t aligned = (addr + (align - 1)) & ~(align - 1);

    size_t prefix = (size_t)(aligned - addr);
    size_t suffix = (size_t)((addr + len) - (aligned + size));

    if (prefix) {
        munmap((void*)addr, prefix);
    }
    if (suffix) {
        munmap((void*)(aligned + size), suffix);
    }

    return (void*)aligned;
}

void* hz4_os_seg_acquire(void) {
    void* mem = hz4_os_mmap_aligned((size_t)HZ4_SEG_SIZE, (size_t)HZ4_SEG_SIZE);
    if (!mem) {
        abort();
    }

#if HZ4_PAGE_TAG_TABLE
    // Initialize page tag table on first segment acquire
    // Uses this segment's base as arena_base
    if (!g_hz4_arena_base) {
        hz4_pagetag_init(mem);
    }
#endif

    return mem;
}

void hz4_os_seg_release(void* seg_base) {
    if (!seg_base) {
        return;
    }
    munmap(seg_base, (size_t)HZ4_SEG_SIZE);
}

void* hz4_os_page_acquire(void) {
    void* mem = hz4_os_mmap_aligned((size_t)HZ4_PAGE_SIZE, (size_t)HZ4_PAGE_SIZE);
    if (!mem) {
        abort();
    }
    return mem;
}

void hz4_os_page_release(void* page_base) {
    if (!page_base) {
        return;
    }
    munmap(page_base, (size_t)HZ4_PAGE_SIZE);
}

void* hz4_os_large_acquire(size_t size) {
    void* mem = hz4_os_mmap_aligned(size, (size_t)HZ4_PAGE_SIZE);
    if (!mem) {
        abort();
    }
    return mem;
}

void hz4_os_large_release(void* base, size_t size) {
    if (!base || size == 0) {
        return;
    }
    munmap(base, size);
}
