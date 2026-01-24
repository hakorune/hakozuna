#include "hz3_large_debug.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HZ3_LARGE_DEBUG

#define HZ3_LARGE_CANARY_MAGIC 0x485a3343414e4152ULL     // "HZ3CANAR"
#define HZ3_LARGE_CANARY_PRE_MAGIC 0x485a3343414e5052ULL // "HZ3CANPR"

#if HZ3_LARGE_DEBUG_REGISTRY
#define HZ3_LARGE_REG_CAP ((uint32_t)HZ3_LARGE_DEBUG_REG_CAP)
#define HZ3_LARGE_REG_TOMB ((Hz3LargeHdr*)(uintptr_t)1)

typedef struct Hz3LargeReg {
    Hz3LargeHdr* hdr;
    void* map_base;
    size_t map_size;
    void* user_ptr;
    size_t req_size;
    uint8_t in_bucket;
    uint8_t in_cache;
} Hz3LargeReg;

static Hz3LargeReg g_hz3_large_reg[HZ3_LARGE_REG_CAP];

static inline uint32_t hz3_large_reg_hash(const void* hdr) {
    uintptr_t v = (uintptr_t)hdr;
    v >>= 5;
    v ^= v >> 33;
    v ^= v >> 11;
    return (uint32_t)v & (HZ3_LARGE_REG_CAP - 1u);
}

static Hz3LargeReg* hz3_large_reg_find_locked(const Hz3LargeHdr* hdr) {
    uint32_t idx = hz3_large_reg_hash(hdr);
    for (uint32_t i = 0; i < HZ3_LARGE_REG_CAP; i++) {
        Hz3LargeReg* ent = &g_hz3_large_reg[(idx + i) & (HZ3_LARGE_REG_CAP - 1u)];
        if (ent->hdr == hdr) {
            return ent;
        }
        if (ent->hdr == NULL) {
            return NULL;
        }
    }
    return NULL;
}

static void hz3_large_reg_insert_locked(Hz3LargeHdr* hdr) {
    uint32_t idx = hz3_large_reg_hash(hdr);
    Hz3LargeReg* tomb = NULL;
    for (uint32_t i = 0; i < HZ3_LARGE_REG_CAP; i++) {
        Hz3LargeReg* ent = &g_hz3_large_reg[(idx + i) & (HZ3_LARGE_REG_CAP - 1u)];
        if (ent->hdr == hdr) {
            tomb = ent;
            break;
        }
        if (ent->hdr == HZ3_LARGE_REG_TOMB && tomb == NULL) {
            tomb = ent;
            continue;
        }
        if (ent->hdr == NULL) {
            tomb = tomb ? tomb : ent;
            break;
        }
    }
    if (!tomb) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_reg_full_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_reg_full_shot, 1, memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_LARGE_REG_FULL] cap=%u\n", (unsigned)HZ3_LARGE_REG_CAP);
        }
#endif
        return;
    }
    tomb->hdr = hdr;
    tomb->map_base = hdr->map_base;
    tomb->map_size = hdr->map_size;
    tomb->user_ptr = hdr->user_ptr;
    tomb->req_size = hdr->req_size;
    tomb->in_bucket = 0;
    tomb->in_cache = 0;
}

static void hz3_large_reg_remove_locked(Hz3LargeHdr* hdr) {
    Hz3LargeReg* ent = hz3_large_reg_find_locked(hdr);
    if (ent) {
        ent->hdr = HZ3_LARGE_REG_TOMB;
    }
}

static void hz3_large_reg_set_bucket_locked(Hz3LargeHdr* hdr, int in_bucket) {
    Hz3LargeReg* ent = hz3_large_reg_find_locked(hdr);
    if (ent) {
        ent->in_bucket = (uint8_t)(in_bucket ? 1 : 0);
    }
}

static void hz3_large_reg_set_cache_locked(Hz3LargeHdr* hdr, int in_cache) {
    Hz3LargeReg* ent = hz3_large_reg_find_locked(hdr);
    if (ent) {
        ent->in_cache = (uint8_t)(in_cache ? 1 : 0);
    }
}
#endif

static inline size_t hz3_large_user_offset(void) {
    return (sizeof(Hz3LargeHdr) + (HZ3_LARGE_ALIGN - 1u)) & ~(HZ3_LARGE_ALIGN - 1u);
}

#if HZ3_LARGE_CANARY_ENABLE
static inline uint64_t hz3_large_canary_value(const Hz3LargeHdr* hdr) {
    return HZ3_LARGE_CANARY_MAGIC ^
        (uint64_t)(uintptr_t)hdr ^
        (uint64_t)(uintptr_t)hdr->user_ptr ^
        (uint64_t)hdr->req_size;
}

static inline uint64_t hz3_large_canary_pre_value(const Hz3LargeHdr* hdr) {
    return HZ3_LARGE_CANARY_PRE_MAGIC ^
        (uint64_t)(uintptr_t)hdr ^
        (uint64_t)(uintptr_t)hdr->user_ptr ^
        (uint64_t)hdr->req_size;
}

static inline uint8_t* hz3_large_canary_pre_ptr(const Hz3LargeHdr* hdr) {
    uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
    uintptr_t user = (uintptr_t)hdr->user_ptr;
    if (user < start + HZ3_LARGE_CANARY_SIZE) {
        return NULL;  // not enough padding
    }
    return (uint8_t*)(user - HZ3_LARGE_CANARY_SIZE);
}

static inline uint8_t* hz3_large_canary_ptr(const Hz3LargeHdr* hdr) {
    return (uint8_t*)hdr->user_ptr + hdr->req_size;
}

static inline int hz3_large_check_canary_pre(const Hz3LargeHdr* hdr) {
    uint8_t* pre = hz3_large_canary_pre_ptr(hdr);
    if (!pre) {
        return 1;  // not present
    }
    uint64_t v = 0;
    memcpy(&v, pre, sizeof(v));
    return v == hz3_large_canary_pre_value(hdr);
}

static inline int hz3_large_check_canary(const Hz3LargeHdr* hdr) {
    uint8_t* ptr = hz3_large_canary_ptr(hdr);
    uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
    if ((uintptr_t)ptr + HZ3_LARGE_CANARY_SIZE > end) {
        return 1;  // out of range -> skip
    }
    uint64_t v = 0;
    memcpy(&v, ptr, sizeof(v));
    return v == hz3_large_canary_value(hdr);
}

#if HZ3_LARGE_DEBUG_REGISTRY
static inline int hz3_large_check_canary_pre_reg(const Hz3LargeReg* reg,
                                                 const Hz3LargeHdr* hdr) {
    uintptr_t start = (uintptr_t)reg->map_base + hz3_large_user_offset();
    uintptr_t user = (uintptr_t)reg->user_ptr;
    if (user < start + HZ3_LARGE_CANARY_SIZE) {
        return 1;  // not enough padding
    }
    uintptr_t pre = user - HZ3_LARGE_CANARY_SIZE;
    if (pre + HZ3_LARGE_CANARY_SIZE > (uintptr_t)reg->map_base + reg->map_size) {
        return 1;
    }
    uint64_t v = 0;
    memcpy(&v, (void*)pre, sizeof(v));
    uint64_t expect = HZ3_LARGE_CANARY_PRE_MAGIC ^
        (uint64_t)(uintptr_t)hdr ^
        (uint64_t)(uintptr_t)reg->user_ptr ^
        (uint64_t)reg->req_size;
    return v == expect;
}

static inline int hz3_large_check_canary_post_reg(const Hz3LargeReg* reg,
                                                  const Hz3LargeHdr* hdr) {
    uintptr_t post = (uintptr_t)reg->user_ptr + reg->req_size;
    if (post + HZ3_LARGE_CANARY_SIZE > (uintptr_t)reg->map_base + reg->map_size) {
        return 1;
    }
    uint64_t v = 0;
    memcpy(&v, (void*)post, sizeof(v));
    uint64_t expect = HZ3_LARGE_CANARY_MAGIC ^
        (uint64_t)(uintptr_t)hdr ^
        (uint64_t)(uintptr_t)reg->user_ptr ^
        (uint64_t)reg->req_size;
    return v == expect;
}
#endif
#endif

void hz3_large_debug_on_bucket_insert_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_REGISTRY
    hz3_large_reg_insert_locked(hdr);
    Hz3LargeReg* reg = hz3_large_reg_find_locked(hdr);
    if (reg && reg->in_bucket) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_dup_bucket_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_dup_bucket_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_LARGE_DUP_BUCKET] hdr=%p\n", (void*)hdr);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
    }
    hz3_large_reg_set_bucket_locked(hdr, 1);
#else
    (void)hdr;
#endif
}

void hz3_large_debug_on_bucket_remove_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_REGISTRY
    hz3_large_reg_set_bucket_locked(hdr, 0);
#else
    (void)hdr;
#endif
}

void hz3_large_debug_on_cache_insert_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_REGISTRY
    Hz3LargeReg* reg = hz3_large_reg_find_locked(hdr);
    if (reg && reg->in_cache) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_dup_cache_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_dup_cache_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_LARGE_DUP_CACHE] hdr=%p\n", (void*)hdr);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
    }
    hz3_large_reg_set_cache_locked(hdr, 1);
#else
    (void)hdr;
#endif
}

void hz3_large_debug_on_cache_remove_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_REGISTRY
    hz3_large_reg_set_cache_locked(hdr, 0);
#else
    (void)hdr;
#endif
}

void hz3_large_debug_on_munmap_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_REGISTRY
    hz3_large_reg_remove_locked(hdr);
#else
    (void)hdr;
#endif
}

void hz3_large_debug_check_bucket_node_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_REGISTRY
    Hz3LargeReg* reg = hz3_large_reg_find_locked(hdr);
    if (!reg) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_bad_node_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_bad_node_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_LARGE_BAD_NODE] cur=%p\n", (void*)hdr);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
        return;
    }
    if (reg->in_bucket == 0) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_bucket_stale_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_bucket_stale_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr, "[HZ3_LARGE_BUCKET_STALE] cur=%p\n", (void*)hdr);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
    }
#else
    (void)hdr;
#endif
}

void hz3_large_debug_bad_hdr_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_SHOT
    static _Atomic int g_hz3_large_bad_hdr_shot = 0;
    if (atomic_exchange_explicit(&g_hz3_large_bad_hdr_shot, 1, memory_order_relaxed) == 0) {
        fprintf(stderr,
                "[HZ3_LARGE_BAD_HDR] cur=%p magic=0x%llx map_base=%p map_size=%zu user_ptr=%p\n",
                (void*)hdr, (unsigned long long)hdr->magic, hdr->map_base, hdr->map_size,
                hdr->user_ptr);
    }
#endif

#if HZ3_LARGE_DEBUG_REGISTRY && HZ3_LARGE_CANARY_ENABLE
    {
        Hz3LargeReg* reg = hz3_large_reg_find_locked(hdr);
        if (reg) {
            int pre_ok = hz3_large_check_canary_pre_reg(reg, hdr);
            int post_ok = hz3_large_check_canary_post_reg(reg, hdr);
#if HZ3_LARGE_DEBUG_SHOT
            static _Atomic int g_hz3_large_bad_hdr_reg_shot = 0;
            if (atomic_exchange_explicit(&g_hz3_large_bad_hdr_reg_shot, 1,
                                         memory_order_relaxed) == 0) {
                fprintf(stderr,
                        "[HZ3_LARGE_BAD_HDR_REG] cur=%p user_ptr=%p req=%zu pre=%d post=%d map_base=%p map_size=%zu in_bucket=%u in_cache=%u\n",
                        (void*)hdr, reg->user_ptr, reg->req_size, pre_ok, post_ok,
                        reg->map_base, reg->map_size, (unsigned)reg->in_bucket,
                        (unsigned)reg->in_cache);
            }
#endif
        }
    }
#endif

#if HZ3_LARGE_FAILFAST
    abort();
#endif
}

void hz3_large_debug_bad_size_locked(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_SHOT
    size_t offset = hz3_large_user_offset();
    size_t tail = 0;
#if HZ3_LARGE_CANARY_ENABLE
    tail = HZ3_LARGE_CANARY_SIZE;
#endif
    size_t max_req = 0;
    if (hdr->map_base && hdr->map_size >= offset + tail) {
        max_req = hdr->map_size - offset - tail;
    }
    if (max_req && hdr->req_size > max_req) {
        static _Atomic int g_hz3_large_bad_size_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_bad_size_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr,
                    "[HZ3_LARGE_BAD_SIZE] cur=%p req=%zu max=%zu map_size=%zu\n",
                    (void*)hdr, hdr->req_size, max_req, hdr->map_size);
        }
    }
#if HZ3_LARGE_DEBUG_REGISTRY
    {
        Hz3LargeReg* reg = hz3_large_reg_find_locked(hdr);
        if (reg) {
            size_t reg_max = 0;
            if (reg->map_size >= offset + tail) {
                reg_max = reg->map_size - offset - tail;
            }
            if (reg_max && reg->req_size > reg_max) {
                static _Atomic int g_hz3_large_bad_size_reg_shot = 0;
                if (atomic_exchange_explicit(&g_hz3_large_bad_size_reg_shot, 1,
                                             memory_order_relaxed) == 0) {
                    fprintf(stderr,
                            "[HZ3_LARGE_BAD_SIZE_REG] cur=%p req=%zu max=%zu map_size=%zu\n",
                            (void*)hdr, reg->req_size, reg_max, reg->map_size);
                }
            }
        }
    }
#endif
#else
    (void)hdr;
#endif
}

void hz3_large_debug_bad_inuse(const Hz3LargeHdr* hdr) {
#if HZ3_LARGE_DEBUG_SHOT
    static _Atomic int g_hz3_large_bad_inuse_shot = 0;
    if (atomic_exchange_explicit(&g_hz3_large_bad_inuse_shot, 1, memory_order_relaxed) == 0) {
        fprintf(stderr,
                "[HZ3_LARGE_BAD_INUSE] ptr=%p hdr=%p in_use=%u map_size=%zu user_ptr=%p\n",
                hdr->user_ptr, (void*)hdr, (unsigned)hdr->in_use, hdr->map_size, hdr->user_ptr);
    }
#else
    (void)hdr;
#endif
#if HZ3_LARGE_FAILFAST
    abort();
#endif
}

void hz3_large_debug_write_canary(const Hz3LargeHdr* hdr) {
#if HZ3_LARGE_CANARY_ENABLE
    uint8_t* pre = hz3_large_canary_pre_ptr(hdr);
    if (pre) {
        uint64_t vpre = hz3_large_canary_pre_value(hdr);
        memcpy(pre, &vpre, sizeof(vpre));
    }
    uint8_t* ptr = hz3_large_canary_ptr(hdr);
    uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
    if ((uintptr_t)ptr + HZ3_LARGE_CANARY_SIZE <= end) {
        uint64_t v = hz3_large_canary_value(hdr);
        memcpy(ptr, &v, sizeof(v));
    }
#else
    (void)hdr;
#endif
}

int hz3_large_debug_check_canary_free(const Hz3LargeHdr* hdr) {
#if HZ3_LARGE_CANARY_ENABLE
    int pre_ok = hz3_large_check_canary_pre(hdr);
    if (!pre_ok) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_bad_canary_pre_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_bad_canary_pre_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr,
                    "[HZ3_LARGE_BAD_CANARY_PRE] ptr=%p hdr=%p map_base=%p map_size=%zu req=%zu\n",
                    hdr->user_ptr, (void*)hdr, hdr->map_base, hdr->map_size, hdr->req_size);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
    }

    int post_ok = hz3_large_check_canary(hdr);
    if (!post_ok) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_bad_canary_post_shot = 0;
        if (atomic_exchange_explicit(&g_hz3_large_bad_canary_post_shot, 1,
                                     memory_order_relaxed) == 0) {
            fprintf(stderr,
                    "[HZ3_LARGE_BAD_CANARY_POST] ptr=%p hdr=%p map_base=%p map_size=%zu req=%zu\n",
                    hdr->user_ptr, (void*)hdr, hdr->map_base, hdr->map_size, hdr->req_size);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
    }
    return pre_ok && post_ok;
#else
    (void)hdr;
    return 1;
#endif
}

int hz3_large_debug_check_canary_cache(const Hz3LargeHdr* hdr, int aligned) {
#if HZ3_LARGE_CANARY_ENABLE
    int pre_ok = hz3_large_check_canary_pre(hdr);
    int post_ok = hz3_large_check_canary(hdr);
    if (!pre_ok || !post_ok) {
#if HZ3_LARGE_DEBUG_SHOT
        static _Atomic int g_hz3_large_bad_canary_cache_shot = 0;
        static _Atomic int g_hz3_large_bad_canary_cache_align_shot = 0;
        _Atomic int* shot = aligned ? &g_hz3_large_bad_canary_cache_align_shot
                                    : &g_hz3_large_bad_canary_cache_shot;
        if (atomic_exchange_explicit(shot, 1, memory_order_relaxed) == 0) {
            fprintf(stderr,
                    aligned ? "[HZ3_LARGE_BAD_CANARY_CACHE_ALIGN] hdr=%p map_base=%p map_size=%zu req=%zu\n"
                            : "[HZ3_LARGE_BAD_CANARY_CACHE] hdr=%p map_base=%p map_size=%zu req=%zu\n",
                    (void*)hdr, hdr->map_base, hdr->map_size, hdr->req_size);
        }
#endif
#if HZ3_LARGE_FAILFAST
        abort();
#endif
        return 0;
    }
    return 1;
#else
    (void)hdr;
    (void)aligned;
    return 1;
#endif
}

#endif  // HZ3_LARGE_DEBUG
