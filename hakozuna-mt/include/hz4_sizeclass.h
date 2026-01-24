// hz4_sizeclass.h - HZ4 Size Class Box (16B aligned, 16..2048B)
#ifndef HZ4_SIZECLASS_H
#define HZ4_SIZECLASS_H

#include <stddef.h>
#include <stdint.h>

#include "hz4_config.h"

#define HZ4_SIZE_ALIGN 16u
#define HZ4_SIZE_MIN   16u
#define HZ4_SIZE_MAX   2048u
#define HZ4_SC_COUNT   (HZ4_SIZE_MAX / HZ4_SIZE_ALIGN)
#define HZ4_SC_INVALID 0xFFu

#if HZ4_SC_MAX < HZ4_SC_COUNT
#error "HZ4_SC_MAX too small for 16..2048B size classes"
#endif

static inline size_t hz4_align_up(size_t v, size_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

static inline uint8_t hz4_size_to_sc(size_t size) {
    if (size == 0) {
        size = HZ4_SIZE_MIN;
    }
    size = hz4_align_up(size, HZ4_SIZE_ALIGN);
    if (size > HZ4_SIZE_MAX) {
        return HZ4_SC_INVALID;
    }
    return (uint8_t)((size / HZ4_SIZE_ALIGN) - 1);
}

static inline size_t hz4_sc_to_size(uint8_t sc) {
    return (size_t)(sc + 1) * HZ4_SIZE_ALIGN;
}

#endif // HZ4_SIZECLASS_H
