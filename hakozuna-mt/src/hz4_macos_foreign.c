#if defined(__APPLE__)

#include <malloc/malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hz4_config.h"
#include "hz4_large.h"
#include "hz4_mid.h"
#include "hz4_os.h"
#include "hz4_page.h"
#include "hz4_sizeclass.h"
#include "hz4_tcache.h"
#include "hz4_macos_foreign.h"

#ifndef HZ4_MACOS_FOREIGN_SAFE
#define HZ4_MACOS_FOREIGN_SAFE 0
#endif

static inline malloc_zone_t* hz4_macos_zone_from_ptr(const void* ptr) {
    if (!ptr) {
        return NULL;
    }
    return malloc_zone_from_ptr(ptr);
}

static inline uint32_t hz4_macos_head_magic(void* ptr) {
    uintptr_t base = (uintptr_t)ptr & ~(HZ4_PAGE_SIZE - 1u);
    return *(uint32_t*)base;
}

static inline int hz4_macos_owned_page_magic(void* ptr) {
    if (!ptr) {
        return 0;
    }
    hz4_page_t* page = hz4_page_from_ptr(ptr);
    return page && page->magic == HZ4_PAGE_MAGIC;
}

static inline int hz4_macos_ptr_looks_owned(void* ptr) {
    if (!ptr) {
        return 0;
    }
    if (hz4_macos_aligned_decode(ptr, NULL, NULL)) {
        return 1;
    }
    if (hz4_large_header_valid(ptr)) {
        return 1;
    }

    uint32_t head_magic = hz4_macos_head_magic(ptr);
    if (head_magic == HZ4_MID_MAGIC || head_magic == HZ4_LARGE_MAGIC) {
        return 1;
    }
    return hz4_macos_owned_page_magic(ptr);
}

static __thread int g_hz4_system_malloc_size_query = 0;

static size_t hz4_macos_system_malloc_size(void* ptr) {
    if (!ptr) {
        return 0;
    }
    if (g_hz4_system_malloc_size_query) {
        return 0;
    }
    g_hz4_system_malloc_size_query = 1;

    size_t out = 0;
    malloc_zone_t* zone = hz4_macos_zone_from_ptr(ptr);
    if (zone && zone->size) {
        out = zone->size(zone, ptr);
    }
    if (out == 0) {
        zone = malloc_default_zone();
        if (zone && zone->size) {
            out = zone->size(zone, ptr);
        }
    }

    g_hz4_system_malloc_size_query = 0;
    return out;
}

static void hz4_macos_system_free(void* ptr) {
    if (!ptr) {
        return;
    }

    malloc_zone_t* zone = hz4_macos_zone_from_ptr(ptr);
    if (!zone || !zone->free) {
        zone = malloc_default_zone();
    }
    if (zone && zone->free) {
        zone->free(zone, ptr);
    }
}

static void* hz4_macos_system_realloc(void* ptr, size_t size) {
    if (!ptr) {
        return NULL;
    }

    malloc_zone_t* zone = hz4_macos_zone_from_ptr(ptr);
    if (!zone || !zone->realloc) {
        zone = malloc_default_zone();
    }
    if (zone && zone->realloc) {
        return zone->realloc(zone, ptr, size);
    }
    if (!zone || !zone->malloc || !zone->free) {
        return NULL;
    }

    size_t old_size = 0;
    if (zone->size) {
        old_size = zone->size(zone, ptr);
    }
    void* next = zone->malloc(zone, size);
    if (!next) {
        return NULL;
    }

    size_t copy = old_size < size ? old_size : size;
    if (copy) {
        memcpy(next, ptr, copy);
    }
    zone->free(zone, ptr);
    return next;
}

size_t hz4_macos_foreign_malloc_size(void* ptr,
                                     hz4_macos_foreign_size_kind_t* kind_out,
                                     uint32_t* magic_out) {
    if (kind_out) {
        *kind_out = HZ4_MACOS_FOREIGN_SIZE_NULL;
    }
    if (magic_out) {
        *magic_out = 0;
    }
    if (!ptr) {
        return 0;
    }

    {
        size_t requested = 0;
        if (hz4_macos_aligned_decode(ptr, NULL, &requested)) {
            if (kind_out) {
                *kind_out = HZ4_MACOS_FOREIGN_SIZE_HZ4;
            }
            if (magic_out) {
                *magic_out = (uint32_t)HZ4_ALIGNED_HDR_MAGIC;
            }
            return requested;
        }
    }

    if (hz4_large_header_valid(ptr)) {
        size_t out = hz4_large_usable_size(ptr);
        if (kind_out) {
            *kind_out = out != 0 ? HZ4_MACOS_FOREIGN_SIZE_HZ4 : HZ4_MACOS_FOREIGN_SIZE_SEGMENT_UNKNOWN;
        }
        if (magic_out) {
            *magic_out = HZ4_LARGE_MAGIC;
        }
        return out;
    }

    uint32_t head_magic = hz4_macos_head_magic(ptr);
    if (head_magic == HZ4_MID_MAGIC) {
        size_t out = hz4_mid_usable_size(ptr);
        if (kind_out) {
            *kind_out = out != 0 ? HZ4_MACOS_FOREIGN_SIZE_HZ4 : HZ4_MACOS_FOREIGN_SIZE_SEGMENT_UNKNOWN;
        }
        if (magic_out) {
            *magic_out = head_magic;
        }
        return out;
    }
    if (head_magic == HZ4_LARGE_MAGIC) {
        size_t out = hz4_large_usable_size(ptr);
        if (kind_out) {
            *kind_out = out != 0 ? HZ4_MACOS_FOREIGN_SIZE_HZ4 : HZ4_MACOS_FOREIGN_SIZE_SEGMENT_UNKNOWN;
        }
        if (magic_out) {
            *magic_out = head_magic;
        }
        return out;
    }

    hz4_page_t* page = hz4_page_from_ptr(ptr);
    if (page && page->magic == HZ4_PAGE_MAGIC) {
        size_t out = hz4_small_usable_size(ptr);
        if (kind_out) {
            *kind_out = out != 0 ? HZ4_MACOS_FOREIGN_SIZE_HZ4 : HZ4_MACOS_FOREIGN_SIZE_SEGMENT_UNKNOWN;
        }
        if (magic_out) {
            *magic_out = page->magic;
        }
        return out;
    }

    if (kind_out) {
        *kind_out = HZ4_MACOS_FOREIGN_SIZE_SYSTEM;
    }
    if (magic_out) {
        *magic_out = head_magic;
    }
    return hz4_macos_system_malloc_size(ptr);
}

size_t hz4_macos_foreign_usable_size(void* ptr) {
    return hz4_macos_foreign_malloc_size(ptr, NULL, NULL);
}

void hz4_macos_foreign_free(void* ptr) {
    void* raw = NULL;
    if (hz4_macos_aligned_decode(ptr, &raw, NULL)) {
        hz4_free(raw);
        return;
    }

#if HZ4_MACOS_FOREIGN_SAFE
    if (ptr && !hz4_macos_ptr_looks_owned(ptr)) {
        hz4_macos_system_free(ptr);
        return;
    }
#endif

    hz4_free(ptr);
}

void* hz4_macos_foreign_realloc(void* ptr, size_t size) {
    if (!ptr) {
        return hz4_malloc(size);
    }
    if (size == 0) {
#if HZ4_MACOS_FOREIGN_SAFE
        if (!hz4_macos_ptr_looks_owned(ptr)) {
            hz4_macos_system_free(ptr);
            return NULL;
        }
#endif
        hz4_free(ptr);
        return NULL;
    }

#if HZ4_MACOS_FOREIGN_SAFE
    if (!hz4_macos_ptr_looks_owned(ptr)) {
        return hz4_macos_system_realloc(ptr, size);
    }
#endif

    size_t old_size = hz4_macos_foreign_usable_size(ptr);
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

#endif  // __APPLE__
