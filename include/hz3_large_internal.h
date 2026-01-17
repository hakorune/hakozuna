#pragma once

#include "hz3_config.h"

#include <stddef.h>
#include <stdint.h>

#define HZ3_LARGE_ALIGN 16u

typedef struct Hz3LargeHdr {
    uint64_t magic;
    size_t   req_size;
    void*    map_base;
    size_t   map_size;
    void*    user_ptr;
    struct Hz3LargeHdr* next;       // bucket list
#if HZ3_LARGE_CACHE_ENABLE
    uint8_t  in_use;                // 1=allocated, 0=cached
    struct Hz3LargeHdr* next_free;  // cache list
#endif
} Hz3LargeHdr;
