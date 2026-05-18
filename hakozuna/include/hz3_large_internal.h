#pragma once

#include "hz3_config.h"

#include <stddef.h>
#include <stdint.h>

#define HZ3_LARGE_ALIGN 16u

#if HZ3_S240_LARGE_OWNER_FRONT
#define HZ3_LARGE_F_DIRECT_MAPLESS 0x0001u
#define HZ3_LARGE_F_DIRECT_RETAINED 0x0002u
#endif

#if HZ3_S240_LARGE_OWNER_FRONT
typedef enum Hz3LargeState {
    HZ3_LARGE_STATE_ACTIVE = 1,
    HZ3_LARGE_STATE_FREEING = 2,
    HZ3_LARGE_STATE_FRONT = 3,
    HZ3_LARGE_STATE_REMOTE = 4,
    HZ3_LARGE_STATE_GLOBAL = 5,
    HZ3_LARGE_STATE_MUNMAP = 6,
} Hz3LargeState;
#endif

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
#if HZ3_S240_LARGE_OWNER_FRONT
    _Atomic uint32_t state;
    uint8_t  owner_shard;
    uint8_t  sc;
    uint16_t flags;
    uint32_t direct_slot;
#endif
} Hz3LargeHdr;
