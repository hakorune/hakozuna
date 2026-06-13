#define _GNU_SOURCE

#include "hz3_large.h"
#include "hz3_config.h"
#include "hz3_large_debug.h"
#include "hz3_large_internal.h"
#include "hz3_oom.h"
#include "hz3_watch_ptr.h"
#include "hz3_platform.h"
#include "hz3_tcache.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#if HZ3_S182_LARGE_CACHE_SPINLOCK
#if !defined(_WIN32)
#include <sched.h>
#endif
#endif

#define HZ3_LARGE_MAGIC 0x485a334c41524745ULL  // "HZ3LARGE"
#define HZ3_LARGE_HASH_BITS 10
#define HZ3_LARGE_HASH_SIZE (1u << HZ3_LARGE_HASH_BITS)

static inline size_t hz3_large_user_offset(void);
static inline int hz3_large_os_munmap(void* base, size_t bytes);
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
static inline void hz3_s276_direct_clear_retained(Hz3LargeHdr* hdr);
#endif

static Hz3LargeHdr* g_hz3_large_buckets[HZ3_LARGE_HASH_SIZE];
static hz3_lock_t g_hz3_large_map_locks[HZ3_S181_LARGE_MAP_LOCK_STRIPES] = {
    [0 ... HZ3_S181_LARGE_MAP_LOCK_STRIPES - 1] = HZ3_LOCK_INITIALIZER
};

#include "hz3_large_lock.inc"
#include "hz3_large_cache_state.inc"
#include "hz3_large_scache.inc"

#include "hz3_large_cache_policy.inc"

#include "hz3_large_map_ops.inc"

#include "hz3_large_retained_helpers.inc"
#include "hz3_large_batch_mmap.inc"
#include "hz3_large_alloc_path.inc"
#include "hz3_large_aligned_alloc_path.inc"
#include "hz3_large_free_path.inc"

size_t hz3_large_usable_size(const void* ptr) {
    if (!ptr) {
        return 0;
    }

    size_t size = 0;
    if (hz3_large_find_size(ptr, &size)) {
        return size;
    }
    return 0;
}
