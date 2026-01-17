#pragma once

#include <stddef.h>

#include "hz3_config.h"
#include "hz3_types.h"

// Small size class mapping (16B..2048B, 16B alignment)
// S28-A NO-GO: both v1 (shift) and v2 (unified check) regressed
static inline int hz3_small_sc_from_size(size_t size) {
    if (size == 0) {
        size = 1;
    }
    if (size > HZ3_SMALL_MAX_SIZE) {
        return -1;
    }
    size_t aligned = (size + (HZ3_SMALL_ALIGN - 1u)) & ~(size_t)(HZ3_SMALL_ALIGN - 1u);
    return (int)((aligned / HZ3_SMALL_ALIGN) - 1u);
}

static inline size_t hz3_small_sc_to_size(int sc) {
    return (size_t)(sc + 1) * (size_t)HZ3_SMALL_ALIGN;
}

void* hz3_small_alloc(size_t size);
void  hz3_small_free(void* ptr, int sc);
void  hz3_small_bin_flush(int sc, Hz3Bin* bin);

// S12-5C: Free by tag for unified dispatch (only when V1 PTAG enabled)
#if HZ3_PTAG16_DISPATCH_ENABLE
void hz3_small_free_by_tag(void* ptr, int sc, int owner);
#endif
