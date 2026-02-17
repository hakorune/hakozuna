// hz4_large.c - HZ4 Large Box (>~64KB, mmap)
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#include "hz4_config.h"
#include "hz4_sizeclass.h"
#include "hz4_os.h"
#include "hz4_large.h"
#include "hz4_tls_init.h"

typedef struct hz4_large_header {
    uint32_t magic;
    uint32_t owner;
    size_t size;
    size_t total;
} hz4_large_header_t;

_Static_assert(
    HZ4_LARGE_HEADER_BYTES ==
        ((((sizeof(hz4_large_header_t)) + (HZ4_SIZE_ALIGN - 1u)) / HZ4_SIZE_ALIGN) * HZ4_SIZE_ALIGN),
    "HZ4_LARGE_HEADER_BYTES must match hz4_large_header_t layout");

#if HZ4_LARGE_CACHE_BOX || HZ4_ST_LARGE_SPAN_CACHE || HZ4_LARGE_SPAN_CACHE
#if HZ4_LARGE_CACHE_BOX
#if HZ4_LARGE_CACHE_SLOTS < 1
#error "HZ4_LARGE_CACHE_SLOTS must be >=1"
#endif
#if HZ4_LARGE_CACHE_MAX_PAGES < 1
#error "HZ4_LARGE_CACHE_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && (HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES < 1)
#error "HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && (HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES > HZ4_LARGE_CACHE_MAX_PAGES)
#error "HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES must be <= HZ4_LARGE_CACHE_MAX_PAGES"
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && (HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS < 1)
#error "HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS must be >=1"
#endif
#if HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX && !HZ4_LARGE_CACHE_BOX
#error "HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX requires HZ4_LARGE_CACHE_BOX=1"
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX && !HZ4_LARGE_CACHE_BOX
#error "HZ4_LARGE_EXTENT_CACHE_BOX requires HZ4_LARGE_CACHE_BOX=1"
#endif
#if HZ4_LARGE_OVERFLOW_TLS_BOX && (HZ4_LARGE_OVERFLOW_TLS_SLOTS < 1)
#error "HZ4_LARGE_OVERFLOW_TLS_SLOTS must be >=1"
#endif
#if HZ4_LARGE_BATCH_ACQUIRE_BOX && (HZ4_LARGE_BATCH_ACQUIRE_BLOCKS < 2)
#error "HZ4_LARGE_BATCH_ACQUIRE_BLOCKS must be >=2"
#endif
#if HZ4_LARGE_BATCH_ACQUIRE_BOX && (HZ4_LARGE_BATCH_ACQUIRE_MAX_PAGES < 1)
#error "HZ4_LARGE_BATCH_ACQUIRE_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_CACHE_SHARDBOX && (HZ4_LARGE_CACHE_SHARDS < 1)
#error "HZ4_LARGE_CACHE_SHARDS must be >=1"
#endif
#if HZ4_LARGE_CACHE_SHARDBOX && ((HZ4_LARGE_CACHE_SHARDS & (HZ4_LARGE_CACHE_SHARDS - 1)) != 0)
#error "HZ4_LARGE_CACHE_SHARDS must be power-of-two"
#endif
#if HZ4_LARGE_CACHE_SHARDBOX && (HZ4_LARGE_CACHE_SHARDS > 64)
#error "HZ4_LARGE_CACHE_SHARDS must be <=64"
#endif
#if HZ4_LARGE_CACHE_SHARDBOX && (HZ4_LARGE_CACHE_SHARD_STEAL_PROBE >= HZ4_LARGE_CACHE_SHARDS)
#error "HZ4_LARGE_CACHE_SHARD_STEAL_PROBE must be < HZ4_LARGE_CACHE_SHARDS"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_SHARDS < 1)
#error "HZ4_LARGE_LOCK_SHARD_SHARDS must be >=1"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && ((HZ4_LARGE_LOCK_SHARD_SHARDS & (HZ4_LARGE_LOCK_SHARD_SHARDS - 1)) != 0)
#error "HZ4_LARGE_LOCK_SHARD_SHARDS must be power-of-two"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_SHARDS > 64)
#error "HZ4_LARGE_LOCK_SHARD_SHARDS must be <=64"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_SLOTS < 1)
#error "HZ4_LARGE_LOCK_SHARD_SLOTS must be >=1"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX && (HZ4_LARGE_LOCK_SHARD_SLOTS_P2 < 1)
#error "HZ4_LARGE_LOCK_SHARD_SLOTS_P2 must be >=1"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX && (HZ4_LARGE_LOCK_SHARD_SLOTS_P2 < HZ4_LARGE_LOCK_SHARD_SLOTS)
#error "HZ4_LARGE_LOCK_SHARD_SLOTS_P2 must be >= HZ4_LARGE_LOCK_SHARD_SLOTS when P2 depth box is enabled"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_SLOTS_MAX < HZ4_LARGE_LOCK_SHARD_SLOTS)
#error "HZ4_LARGE_LOCK_SHARD_SLOTS_MAX must be >= HZ4_LARGE_LOCK_SHARD_SLOTS"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX && (HZ4_LARGE_LOCK_SHARD_SLOTS_MAX < HZ4_LARGE_LOCK_SHARD_SLOTS_P2)
#error "HZ4_LARGE_LOCK_SHARD_SLOTS_MAX must be >= HZ4_LARGE_LOCK_SHARD_SLOTS_P2 when P2 depth box is enabled"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_MIN_PAGES < 1)
#error "HZ4_LARGE_LOCK_SHARD_MIN_PAGES must be >=1"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_MAX_PAGES < HZ4_LARGE_LOCK_SHARD_MIN_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_MAX_PAGES must be >= HZ4_LARGE_LOCK_SHARD_MIN_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_MAX_PAGES > HZ4_LARGE_CACHE_MAX_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_MAX_PAGES must be <= HZ4_LARGE_CACHE_MAX_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_LARGE_LOCK_SHARD_STEAL_PROBE >= HZ4_LARGE_LOCK_SHARD_SHARDS)
#error "HZ4_LARGE_LOCK_SHARD_STEAL_PROBE must be < HZ4_LARGE_LOCK_SHARD_SHARDS"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && (HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2 >= HZ4_LARGE_LOCK_SHARD_SHARDS)
#error "HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2 must be < HZ4_LARGE_LOCK_SHARD_SHARDS"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STICKY_HINT_BOX && (HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES < HZ4_LARGE_LOCK_SHARD_MIN_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES must be >= HZ4_LARGE_LOCK_SHARD_MIN_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STICKY_HINT_BOX && (HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES > HZ4_LARGE_LOCK_SHARD_MAX_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES must be <= HZ4_LARGE_LOCK_SHARD_MAX_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX && (HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES < HZ4_LARGE_LOCK_SHARD_MIN_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES must be >= HZ4_LARGE_LOCK_SHARD_MIN_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX && (HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES > HZ4_LARGE_LOCK_SHARD_MAX_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES must be <= HZ4_LARGE_LOCK_SHARD_MAX_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX && (HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK < 1)
#error "HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK must be >=1"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX && (HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK > 255)
#error "HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK must be <=255"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX && (HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_COOLDOWN > 255)
#error "HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_COOLDOWN must be <=255"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX && (HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES < HZ4_LARGE_LOCK_SHARD_MIN_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES must be >= HZ4_LARGE_LOCK_SHARD_MIN_PAGES"
#endif
#if HZ4_LARGE_LOCK_SHARD_LAYER && HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX && (HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES > HZ4_LARGE_LOCK_SHARD_MAX_PAGES)
#error "HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES must be <= HZ4_LARGE_LOCK_SHARD_MAX_PAGES"
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER && (HZ4_LARGE_HOT_CACHE_MAX_PAGES < 1)
#error "HZ4_LARGE_HOT_CACHE_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER && (HZ4_LARGE_HOT_CACHE_MAX_PAGES > HZ4_LARGE_CACHE_MAX_PAGES)
#error "HZ4_LARGE_HOT_CACHE_MAX_PAGES must be <= HZ4_LARGE_CACHE_MAX_PAGES"
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER && (HZ4_LARGE_HOT_CACHE_SLOTS < 1)
#error "HZ4_LARGE_HOT_CACHE_SLOTS must be >=1"
#endif
#if HZ4_LARGE_HOT_CACHE_LAYER && (HZ4_LARGE_HOT_CACHE_MAX_BYTES < 1)
#error "HZ4_LARGE_HOT_CACHE_MAX_BYTES must be >=1"
#endif
#if HZ4_LARGE_FAIL_RESCUE_BOX && (HZ4_LARGE_FAIL_RESCUE_SCAN_PAGES < 1)
#error "HZ4_LARGE_FAIL_RESCUE_SCAN_PAGES must be >=1"
#endif
#if HZ4_LARGE_FAIL_RESCUE_BOX && (HZ4_LARGE_FAIL_RESCUE_TRIGGER_STREAK < 1)
#error "HZ4_LARGE_FAIL_RESCUE_TRIGGER_STREAK must be >=1"
#endif
#if HZ4_LARGE_FAIL_RESCUE_BOX && HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE && (HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW < 1)
#error "HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW must be >=1"
#endif
#if HZ4_LARGE_FAIL_RESCUE_BOX && HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE && (HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT > 100)
#error "HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT must be <=100"
#endif
#if HZ4_LARGE_FAIL_RESCUE_BOX && HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE && (HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK < 1)
#error "HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK must be >=1"
#endif
#endif


#include "hz4_large_paths.inc"

#endif

static inline size_t hz4_large_hdr_size(void) {
    return hz4_align_up(sizeof(hz4_large_header_t), HZ4_SIZE_ALIGN);
}

static inline size_t hz4_large_total_size(size_t size) {
    size_t hdr = hz4_large_hdr_size();
    size_t total = hdr + size;
    return hz4_align_up(total, HZ4_PAGE_SIZE);
}

static inline int hz4_large_header_try_from_ptr(void* ptr, hz4_large_header_t** out) {
    if (!ptr) {
        return 0;
    }
    size_t hdr = hz4_large_hdr_size();
    uintptr_t addr = (uintptr_t)ptr;
    if ((addr & ((uintptr_t)HZ4_PAGE_SIZE - 1u)) < hdr) {
        return 0;
    }
    uintptr_t haddr = addr - hdr;
#if HZ4_LARGE_HEADER_ALIGN_FILTER_BOX
    // Large header is placed at mmap base (OS page aligned). Reject most
    // small/mid pointers without touching header memory.
    if ((haddr & (uintptr_t)HZ4_LARGE_HEADER_ALIGN_FILTER_MASK) != 0) {
        return 0;
    }
#endif
    hz4_large_header_t* h = (hz4_large_header_t*)haddr;
    if (h->magic != HZ4_LARGE_MAGIC) {
        return 0;
    }
    if (h->total < hdr) {
        return 0;
    }
    if ((h->total & ((size_t)HZ4_PAGE_SIZE - 1u)) != 0) {
        return 0;
    }
    if (h->size > h->total - hdr) {
        return 0;
    }
    if (out) {
        *out = h;
    }
    return 1;
}

int hz4_large_header_valid(void* ptr) {
    return hz4_large_header_try_from_ptr(ptr, NULL);
}

void* hz4_large_malloc(size_t size) {
    size_t hdr = hz4_large_hdr_size();
    size_t total = hz4_large_total_size(size);
#if HZ4_LARGE_SPAN_CACHE
    int allow_large_span_cache = hz4_large_span_cache_enabled();
#endif

#if HZ4_ST_LARGE_SPAN_CACHE
    void* base = hz4_st_large_cache_try_acquire(total);
    if (!base) {
#if HZ4_LARGE_SPAN_CACHE
        if (allow_large_span_cache) {
            base = hz4_large_span_cache_try_acquire(total);
        }
#endif
#if HZ4_LARGE_CACHE_BOX
        if (!base) {
        base = hz4_large_cache_try_acquire(total);
        }
#endif
    }
    if (!base) {
        base = hz4_large_os_acquire_with_retry(total);
    }
#elif HZ4_LARGE_SPAN_CACHE
#if HZ4_LARGE_SPAN_CACHE
    void* base = allow_large_span_cache ? hz4_large_span_cache_try_acquire(total) : NULL;
#else
    void* base = NULL;
#endif
    if (!base) {
#if HZ4_LARGE_CACHE_BOX
        base = hz4_large_cache_try_acquire(total);
#endif
    }
    if (!base) {
        base = hz4_large_os_acquire_with_retry(total);
    }
#elif HZ4_LARGE_CACHE_BOX
    void* base = hz4_large_cache_try_acquire(total);
    if (!base) {
        base = hz4_large_os_acquire_with_retry(total);
    }
#else
    void* base = hz4_large_os_acquire_with_retry(total);
#endif
    if (!base) {
        HZ4_FAIL("hz4_large_malloc: os acquire failed");
        return NULL;
    }

    hz4_large_header_t* h = (hz4_large_header_t*)base;
    hz4_tls_t* tls = hz4_tls_get();
    h->magic = HZ4_LARGE_MAGIC;
    h->owner = (uint32_t)tls->owner;
    h->size = size;
    h->total = total;

    return (void*)((uint8_t*)base + hdr);
}

static inline void hz4_large_free_header_checked(hz4_large_header_t* h) {
#if HZ4_S220_LARGE_OWNER_RETURN || HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX
    int remote_owner = (h->owner != hz4_large_owner_self());
#endif

#if HZ4_S220_LARGE_OWNER_RETURN
    int owner_return_active = remote_owner && hz4_large_owner_return_pages_enabled(h->total);
#else
    int owner_return_active = 0;
#endif

#if HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX
    if (remote_owner) {
        hz4_os_stats_large_remote_free_seen();
    }
    if (!owner_return_active && remote_owner && hz4_large_remote_bypass_pages_enabled(h->total)) {
        uint32_t pages = (uint32_t)(h->total >> HZ4_PAGE_SHIFT);
        hz4_os_stats_large_remote_bypass_spancache();
        if (pages == 1) {
            hz4_os_stats_large_remote_bypass_pages1();
        } else if (pages == 2) {
            hz4_os_stats_large_remote_bypass_pages2();
        }
#if HZ4_LARGE_CACHE_BOX
        if (hz4_large_cache_try_release_with_policy(h, h->total, 0, 0)) {
            hz4_os_stats_large_remote_bypass_cache_hit();
            return;
        }
#endif
        hz4_os_stats_large_remote_bypass_cache_miss();
        return;
    }
#endif

#if HZ4_S220_LARGE_OWNER_RETURN && HZ4_LARGE_CACHE_BOX && HZ4_LARGE_LOCK_SHARD_LAYER
    if (remote_owner) {
        hz4_os_stats_large_owner_remote();
        if (!owner_return_active) {
            hz4_os_stats_large_owner_skip_pages();
        }
    }
    if (owner_return_active) {
        if (hz4_large_lock_shard_try_release_seeded(h, h->total, h->owner)) {
            hz4_os_stats_large_owner_hit();
            return;
        }
        hz4_os_stats_large_owner_miss();
    }
#endif

#if HZ4_ST_LARGE_SPAN_CACHE
    if (!owner_return_active) {
        if (hz4_st_large_cache_try_release(h, h->total)) {
            return;
        }
    }
#endif
#if HZ4_LARGE_SPAN_CACHE
    if (!owner_return_active) {
        if (hz4_large_span_cache_enabled() && hz4_large_span_cache_try_release(h, h->total)) {
            return;
        }
    }
#endif
#if HZ4_LARGE_CACHE_BOX
    if (hz4_large_cache_try_release(h, h->total)) {
        return;
    }
#endif
    hz4_os_large_release(h, h->total);
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
    hz4_large_free_header_checked(h);
}

int hz4_large_try_free(void* ptr) {
    hz4_large_header_t* h = NULL;
    if (!hz4_large_header_try_from_ptr(ptr, &h)) {
        return 0;
    }
    hz4_large_free_header_checked(h);
    return 1;
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
