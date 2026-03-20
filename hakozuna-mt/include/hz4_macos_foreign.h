#ifndef HZ4_MACOS_FOREIGN_H
#define HZ4_MACOS_FOREIGN_H

#include <stddef.h>
#include <stdint.h>

#if defined(__APPLE__)

typedef struct hz4_aligned_hdr {
    uint64_t magic;
    uintptr_t raw;
    uintptr_t cookie;
    size_t requested;
} hz4_aligned_hdr_t;

#define HZ4_ALIGNED_HDR_MAGIC UINT64_C(0x48345a414c4e4844) /* "H4ZALNHD" */
#define HZ4_ALIGNED_HDR_COOKIE UINT64_C(0x9e3779b97f4a7c15)

static inline hz4_aligned_hdr_t* hz4_macos_aligned_hdr_from_ptr(void* ptr) {
    return (hz4_aligned_hdr_t*)((uintptr_t)ptr - sizeof(hz4_aligned_hdr_t));
}

static inline uintptr_t hz4_macos_aligned_cookie(uintptr_t raw, uintptr_t aligned) {
    return raw ^ aligned ^ (uintptr_t)HZ4_ALIGNED_HDR_COOKIE;
}

static inline void hz4_macos_aligned_write(hz4_aligned_hdr_t* hdr,
                                           void* raw,
                                           void* aligned,
                                           size_t requested) {
    hdr->magic = HZ4_ALIGNED_HDR_MAGIC;
    hdr->raw = (uintptr_t)raw;
    hdr->cookie = hz4_macos_aligned_cookie((uintptr_t)raw, (uintptr_t)aligned);
    hdr->requested = requested;
}

static inline int hz4_macos_aligned_decode(void* ptr, void** raw_out, size_t* req_out) {
    if (!ptr) {
        return 0;
    }
    hz4_aligned_hdr_t* hdr = hz4_macos_aligned_hdr_from_ptr(ptr);
    if (hdr->magic != HZ4_ALIGNED_HDR_MAGIC) {
        return 0;
    }
    uintptr_t aligned = (uintptr_t)ptr;
    uintptr_t raw = hdr->raw;
    if (raw == 0 || hdr->cookie != hz4_macos_aligned_cookie(raw, aligned)) {
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

typedef enum hz4_macos_foreign_size_kind {
    HZ4_MACOS_FOREIGN_SIZE_NULL = 0,
    HZ4_MACOS_FOREIGN_SIZE_HZ4 = 1,
    HZ4_MACOS_FOREIGN_SIZE_SEGMENT_UNKNOWN = 2,
    HZ4_MACOS_FOREIGN_SIZE_SYSTEM = 3,
} hz4_macos_foreign_size_kind_t;

size_t hz4_macos_foreign_malloc_size(void* ptr,
                                     hz4_macos_foreign_size_kind_t* kind_out,
                                     uint32_t* magic_out);
size_t hz4_macos_foreign_usable_size(void* ptr);
void   hz4_macos_foreign_free(void* ptr);
void*  hz4_macos_foreign_realloc(void* ptr, size_t size);

#endif  // __APPLE__

#endif  // HZ4_MACOS_FOREIGN_H
