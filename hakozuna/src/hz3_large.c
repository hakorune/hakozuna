#define _GNU_SOURCE

#include "hz3_large.h"
#include "hz3_config.h"
#include "hz3_large_debug.h"
#include "hz3_large_internal.h"
#include "hz3_oom.h"
#include "hz3_watch_ptr.h"
#include "hz3_platform.h"

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

static Hz3LargeHdr* g_hz3_large_buckets[HZ3_LARGE_HASH_SIZE];
static hz3_lock_t g_hz3_large_map_locks[HZ3_S181_LARGE_MAP_LOCK_STRIPES] = {
    [0 ... HZ3_S181_LARGE_MAP_LOCK_STRIPES - 1] = HZ3_LOCK_INITIALIZER
};

#if HZ3_S182_LARGE_CACHE_SPINLOCK
static atomic_flag g_hz3_large_cache_spin = ATOMIC_FLAG_INIT;

static inline void hz3_large_spin_pause_hint(void) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#else
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

static inline void hz3_large_spin_thread_yield(void) {
#if defined(_WIN32)
    SwitchToThread();
#else
    sched_yield();
#endif
}

static inline void hz3_large_cache_lock_acquire(void) {
    uint32_t spins = 0;
#if HZ3_S182_SPIN_ADAPTIVE_YIELD
    uint32_t next_yield_at = HZ3_S182_SPIN_YIELD_START;
    uint32_t stage = 0;
#endif
    while (atomic_flag_test_and_set_explicit(&g_hz3_large_cache_spin, memory_order_acquire)) {
        ++spins;
#if HZ3_S182_SPIN_PAUSE_INTERVAL > 0
        if ((spins % HZ3_S182_SPIN_PAUSE_INTERVAL) == 0u) {
            hz3_large_spin_pause_hint();
        }
#endif
#if HZ3_S182_SPIN_ADAPTIVE_YIELD
        if (spins >= next_yield_at) {
            hz3_large_spin_thread_yield();
            if (stage < HZ3_S182_SPIN_ADAPTIVE_MAX_STAGE) {
                ++stage;
            }
            next_yield_at += (HZ3_S182_SPIN_YIELD_INTERVAL << stage);
        }
#else
        if (spins >= HZ3_S182_SPIN_YIELD_START &&
            ((spins - HZ3_S182_SPIN_YIELD_START) % HZ3_S182_SPIN_YIELD_INTERVAL) == 0u) {
            hz3_large_spin_thread_yield();
        }
#endif
    }
}

static inline void hz3_large_cache_lock_release(void) {
    atomic_flag_clear_explicit(&g_hz3_large_cache_spin, memory_order_release);
}
#else
static hz3_lock_t g_hz3_large_lock = HZ3_LOCK_INITIALIZER;

static inline void hz3_large_cache_lock_acquire(void) {
    hz3_lock_acquire(&g_hz3_large_lock);
}

static inline void hz3_large_cache_lock_release(void) {
    hz3_lock_release(&g_hz3_large_lock);
}
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
// S50: Per-class LIFO cache
static Hz3LargeHdr* g_sc_head[HZ3_LARGE_SC_COUNT];
static size_t g_sc_bytes[HZ3_LARGE_SC_COUNT];
static uint32_t g_sc_nodes[HZ3_LARGE_SC_COUNT];
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
static hz3_lock_t g_sc_locks[HZ3_LARGE_SC_COUNT] = {
    [0 ... HZ3_LARGE_SC_COUNT - 1] = HZ3_LOCK_INITIALIZER
};
static _Atomic size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#else
static size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
// Relaxed hint for free-path precheck (S184). Real admission is still checked under lock.
static _Atomic size_t g_total_cached_bytes_hint = 0;
#endif
#if HZ3_S207_TARGETED_REUSE
static _Atomic uint16_t g_sc_reuse_credit[HZ3_LARGE_SC_COUNT];
#endif
#else
// Legacy: single LIFO
static Hz3LargeHdr* g_hz3_large_free_head = NULL;
static size_t g_hz3_large_cached_bytes = 0;
static size_t g_hz3_large_cached_nodes = 0;
#endif
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
static inline size_t hz3_large_total_cached_load(void) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    return atomic_load_explicit(&g_total_cached_bytes, memory_order_relaxed);
#else
    return g_total_cached_bytes;
#endif
}

static inline void hz3_large_total_cached_add(size_t bytes) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    atomic_fetch_add_explicit(&g_total_cached_bytes, bytes, memory_order_relaxed);
#else
    g_total_cached_bytes += bytes;
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
    atomic_fetch_add_explicit(&g_total_cached_bytes_hint, bytes, memory_order_relaxed);
#endif
}

static inline void hz3_large_total_cached_sub(size_t bytes) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    atomic_fetch_sub_explicit(&g_total_cached_bytes, bytes, memory_order_relaxed);
#else
    g_total_cached_bytes -= bytes;
#endif
#if HZ3_S184_LARGE_FREE_PRECHECK
    atomic_fetch_sub_explicit(&g_total_cached_bytes_hint, bytes, memory_order_relaxed);
#endif
}

#if HZ3_S184_LARGE_FREE_PRECHECK
static inline size_t hz3_large_total_cached_hint_load(void) {
    return atomic_load_explicit(&g_total_cached_bytes_hint, memory_order_relaxed);
}
#endif

static inline void hz3_large_sc_lock_acquire(int sc) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_lock_acquire(&g_sc_locks[sc]);
#else
    (void)sc;
#endif
}

static inline void hz3_large_sc_lock_release(int sc) {
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_lock_release(&g_sc_locks[sc]);
#else
    (void)sc;
#endif
}

static inline Hz3LargeHdr* hz3_large_sc_try_pop_locked(int sc, size_t need, int exact_fit) {
    Hz3LargeHdr* hdr = g_sc_head[sc];
    if (!hdr || hdr->magic != HZ3_LARGE_MAGIC) {
        return NULL;
    }
    if (exact_fit ? (hdr->map_size != need) : (hdr->map_size < need)) {
        return NULL;
    }
    g_sc_head[sc] = hdr->next_free;
    g_sc_bytes[sc] -= hdr->map_size;
    g_sc_nodes[sc]--;
    hz3_large_total_cached_sub(hdr->map_size);
    hz3_large_debug_on_cache_remove_locked(hdr);
    return hdr;
}

static inline Hz3LargeHdr* hz3_large_sc_pop_head(int sc) {
    hz3_large_sc_lock_acquire(sc);
    Hz3LargeHdr* hdr = g_sc_head[sc];
    if (hdr) {
        g_sc_head[sc] = hdr->next_free;
        g_sc_bytes[sc] -= hdr->map_size;
        g_sc_nodes[sc]--;
        hz3_large_total_cached_sub(hdr->map_size);
        hz3_large_debug_on_cache_remove_locked(hdr);
    }
    hz3_large_sc_lock_release(sc);
    return hdr;
}

static inline void hz3_large_sc_push_head(int sc, Hz3LargeHdr* hdr) {
    hz3_large_sc_lock_acquire(sc);
    hdr->next_free = g_sc_head[sc];
    g_sc_head[sc] = hdr;
    g_sc_bytes[sc] += hdr->map_size;
    g_sc_nodes[sc]++;
    hz3_large_total_cached_add(hdr->map_size);
    hz3_large_debug_on_cache_insert_locked(hdr);
    hz3_large_sc_lock_release(sc);
}
#endif

#include "hz3_large_cache_policy.inc"

#include "hz3_large_map_ops.inc"

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
static inline int hz3_large_s218_c1_batch_enabled_for_sc(int sc) {
#if HZ3_S218_C1_LARGE_BATCH_MMAP
    return (sc >= HZ3_S218_C1_SC_MIN &&
            sc <= HZ3_S218_C1_SC_MAX &&
            sc < (HZ3_LARGE_SC_COUNT - 1));
#else
    (void)sc;
    return 0;
#endif
}

static inline size_t hz3_large_s218_c1_seed_req_size(size_t class_size, size_t offset) {
    size_t reserve = offset;
#if HZ3_LARGE_CANARY_ENABLE
    reserve += HZ3_LARGE_CANARY_SIZE;
#endif
    if (class_size <= reserve) {
        return 1;
    }
    return class_size - reserve;
}

static Hz3LargeHdr* hz3_large_s218_c1_batch_mmap_alloc(size_t req_size,
                                                       size_t class_size,
                                                       size_t offset,
                                                       int sc,
                                                       size_t align_pad,
                                                       int aligned_alloc) {
#if HZ3_S218_C1_LARGE_BATCH_MMAP
    const size_t blocks = (size_t)HZ3_S218_C1_BATCH_BLOCKS;
    const size_t max_cache_per_batch = (size_t)HZ3_S218_C1_MAX_CACHE_PER_BATCH;
    if (!hz3_large_s218_c1_batch_enabled_for_sc(sc) || blocks < 2 || class_size == 0) {
        return NULL;
    }
    if (class_size > (SIZE_MAX / blocks)) {
        return NULL;
    }
    if (max_cache_per_batch == 0) {
        return NULL;
    }

    size_t hard_cap = hz3_large_hard_cap_for_sc(sc);
    const size_t seed_cap = (size_t)HZ3_S218_C1_SEED_CAP_BYTES;
    if (seed_cap > 0 && hard_cap > seed_cap) {
        hard_cap = seed_cap;
    }
    if (hard_cap < class_size) {
        return NULL;
    }
    // Require room for at least one seeded block; otherwise fallback to normal mmap.
    if ((hz3_large_total_cached_load() + class_size) > hard_cap) {
        return NULL;
    }

    const size_t total = class_size * blocks;
    void* region = hz3_large_os_mmap(total);
    if (region == MAP_FAILED) {
        return NULL;
    }

    uintptr_t user = (uintptr_t)region + offset;
    if (aligned_alloc && align_pad > 0) {
        user = (user + align_pad) & ~(uintptr_t)align_pad;
    }

    Hz3LargeHdr* hdr = (Hz3LargeHdr*)region;
    hdr->magic = HZ3_LARGE_MAGIC;
    hdr->req_size = req_size;
    hdr->map_base = region;
    hdr->map_size = class_size;
    hdr->user_ptr = (void*)user;
    hdr->next = NULL;
    hdr->in_use = 1;
    hdr->next_free = NULL;
#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif

    size_t cached_blocks = 0;
    size_t dropped_blocks = 0;
    size_t seed_req = hz3_large_s218_c1_seed_req_size(class_size, offset);
    void* dropped_bases[(HZ3_S218_C1_BATCH_BLOCKS > 1) ? (HZ3_S218_C1_BATCH_BLOCKS - 1) : 1];
    size_t dropped_count = 0;

#if !HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_large_cache_lock_acquire();
#endif

    for (size_t index = 1; index < blocks; index++) {
        void* extra_base = (void*)((char*)region + (index * class_size));
        Hz3LargeHdr* extra = (Hz3LargeHdr*)extra_base;
        extra->magic = HZ3_LARGE_MAGIC;
        extra->req_size = seed_req;
        extra->map_base = extra_base;
        extra->map_size = class_size;
        extra->user_ptr = (char*)extra_base + offset;
        extra->next = NULL;
        extra->in_use = 0;
        extra->next_free = NULL;
#if HZ3_LARGE_CANARY_ENABLE
        hz3_large_debug_write_canary(extra);
#endif

        int can_cache = (cached_blocks < max_cache_per_batch);
        if (can_cache) {
            can_cache = (hz3_large_total_cached_load() + class_size <= hard_cap);
        }
        if (can_cache) {
            can_cache = hz3_large_reuse_try_admit_locked(sc);
        }
        if (can_cache) {
            hz3_large_sc_push_head(sc, extra);
            cached_blocks++;
        } else {
            dropped_bases[dropped_count++] = extra_base;
            dropped_blocks++;
        }
    }

#if !HZ3_S183_LARGE_CLASS_LOCK_SPLIT
    hz3_large_cache_lock_release();
#endif

    for (size_t index = 0; index < dropped_count; index++) {
        hz3_large_os_munmap(dropped_bases[index], class_size);
    }

    hz3_large_stats_on_s218_c1_batch(cached_blocks, dropped_blocks);
    return hdr;
#else
    (void)req_size;
    (void)class_size;
    (void)offset;
    (void)sc;
    (void)align_pad;
    (void)aligned_alloc;
    return NULL;
#endif
}
#endif

void* hz3_large_alloc(size_t size) {
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_alloc_call();

    if (size == 0) {
        size = 1;
    }

    size_t offset = hz3_large_user_offset();
    if (size > (SIZE_MAX - offset)) {
        return NULL;
    }

#if HZ3_LARGE_CANARY_ENABLE
    if (size > (SIZE_MAX - offset - HZ3_LARGE_CANARY_SIZE)) {
        return NULL;
    }
    size_t need = offset + size + HZ3_LARGE_CANARY_SIZE;
#else
    size_t need = offset + size;
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
    // S50: Round up to class size, then recompute sc from final size
    // This ensures alloc and free use the same class for the same block.
    int sc = hz3_large_sc(need);
    size_t class_size = hz3_large_sc_size(sc);
    if (class_size == 0) {
        // >64MB: キャッシュ対象外、実サイズ使用
        class_size = hz3_page_align(need);
    } else {
        class_size = hz3_page_align(class_size);
        // Recompute sc from page-aligned size to match what free() will compute
        sc = hz3_large_sc(class_size);
    }

    // Cache lookup: O(1) pop (sc < HZ3_LARGE_SC_COUNT-1 のみ)
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT - 1) {
        int try_sc = sc;
        Hz3LargeHdr* hdr = NULL;

#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → sc+2)
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hz3_large_sc_lock_acquire(try_sc);
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            hz3_large_sc_lock_release(try_sc);
            if (hdr) {
                break;
            }
        }
#else
        hz3_large_sc_lock_acquire(sc);
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
        hz3_large_sc_lock_release(sc);
#endif
#else
        hz3_large_cache_lock_acquire();
#if HZ3_S52_LARGE_BESTFIT
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            if (hdr) {
                break;
            }
        }
#else
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
#endif
        hz3_large_cache_lock_release();
#endif

        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_alloc:
#endif
    hz3_large_stats_on_alloc_cache_miss();
    hz3_large_reuse_note_alloc_miss(sc);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    hz3_large_unmap_defer_drain_on_alloc_miss();
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#if HZ3_S193_DEMAND_SCAVENGE
    hz3_large_scavenge_on_mmap_miss(sc);
#endif
#else
    // Legacy: Cache から探す（first-fit）
    hz3_large_cache_lock_acquire();
    Hz3LargeHdr* prev = NULL;
    Hz3LargeHdr* cur = g_hz3_large_free_head;
    while (cur) {
        if (cur->map_size >= need) {
            // 見つかった: cache から外す
            if (prev) {
                prev->next_free = cur->next_free;
            } else {
                g_hz3_large_free_head = cur->next_free;
            }
            g_hz3_large_cached_bytes -= cur->map_size;
            g_hz3_large_cached_nodes--;
            cur->in_use = 1;
            cur->req_size = size;
            cur->next_free = NULL;
            hz3_large_cache_lock_release();

            // bucket に再挿入（usable_size 等で参照される）
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(cur);
#endif
            hz3_large_insert(cur);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", cur->user_ptr, -1, -1);
#endif
            return cur->user_ptr;
        }
        prev = cur;
        cur = cur->next_free;
    }
    hz3_large_cache_lock_release();
#endif
#endif

    Hz3LargeHdr* hdr = NULL;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    hdr = hz3_large_s218_c1_batch_mmap_alloc(size, need, offset, sc, 0, 0);
#endif
    if (!hdr) {
        void* base = hz3_large_os_mmap(need);
        if (base == MAP_FAILED) {
            hz3_oom_note("large_mmap", need, 0);
            return NULL;
        }
        hdr = (Hz3LargeHdr*)base;
        hdr->magic = HZ3_LARGE_MAGIC;
        hdr->req_size = size;
        hdr->map_base = base;
        hdr->map_size = need;
        hdr->user_ptr = (char*)base + offset;
        hdr->next = NULL;
#if HZ3_LARGE_CACHE_ENABLE
        hdr->in_use = 1;
        hdr->next_free = NULL;
#endif
    }

#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif
    hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("large_alloc_mmap", hdr->user_ptr, -1, -1);
#endif
    return hdr->user_ptr;
}

void* hz3_large_aligned_alloc(size_t alignment, size_t size) {
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_alloc_call();

    if (alignment == 0 || (alignment & (alignment - 1u)) != 0) {
        return NULL;
    }
    if (alignment < HZ3_LARGE_ALIGN) {
        alignment = HZ3_LARGE_ALIGN;
    }
    if (size == 0) {
        size = 1;
    }

    size_t offset = hz3_large_user_offset();
    if (size > (SIZE_MAX - offset)) {
        return NULL;
    }
    size_t pad = alignment - 1u;
    if (size > (SIZE_MAX - offset - pad)) {
        return NULL;
    }
#if HZ3_LARGE_CANARY_ENABLE
    if (size > (SIZE_MAX - offset - pad - HZ3_LARGE_CANARY_SIZE)) {
        return NULL;
    }
    size_t need = offset + size + pad + HZ3_LARGE_CANARY_SIZE;
#else
    size_t need = offset + size + pad;
#endif

#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    // S50: Round up to class size, then recompute sc from final size
    // This ensures alloc and free use the same class for the same block.
    int sc = hz3_large_sc(need);
    size_t class_size = hz3_large_sc_size(sc);
    if (class_size == 0) {
        // >64MB: キャッシュ対象外、実サイズ使用
        class_size = hz3_page_align(need);
    } else {
        class_size = hz3_page_align(class_size);
        // Recompute sc from page-aligned size to match what free() will compute
        sc = hz3_large_sc(class_size);
    }

    // Cache lookup: O(1) pop (sc < HZ3_LARGE_SC_COUNT-1 のみ)
    if (sc >= 0 && sc < HZ3_LARGE_SC_COUNT - 1) {
        int try_sc = sc;
        Hz3LargeHdr* hdr = NULL;

#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → ...), aligned_alloc でも同じ方針
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hz3_large_sc_lock_acquire(try_sc);
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            hz3_large_sc_lock_release(try_sc);
            if (hdr) {
                break;
            }
        }
#else
        hz3_large_sc_lock_acquire(sc);
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
        hz3_large_sc_lock_release(sc);
#endif
#else
        hz3_large_cache_lock_acquire();
#if HZ3_S52_LARGE_BESTFIT
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        for (; try_sc <= try_limit; try_sc++) {
            hdr = hz3_large_sc_try_pop_locked(try_sc, class_size, 0);
            if (hdr) {
                break;
            }
        }
#else
        hdr = hz3_large_sc_try_pop_locked(sc, class_size, 1);
#endif
        hz3_large_cache_lock_release();
#endif

        if (hdr) {
            hz3_large_stats_on_alloc_cache_hit();
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 1)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_large_os_munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_aligned;
            }
#endif
            // recalculate user_ptr for new alignment
            uintptr_t start = (uintptr_t)hdr->map_base + offset;
            uintptr_t user = (start + pad) & ~(uintptr_t)pad;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->user_ptr = (void*)user;
            hdr->next_free = NULL;
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_aligned_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_aligned:
#endif
    hz3_large_stats_on_alloc_cache_miss();
    hz3_large_reuse_note_alloc_miss(sc);
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    hz3_large_unmap_defer_drain_on_alloc_miss();
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#if HZ3_S193_DEMAND_SCAVENGE
    hz3_large_scavenge_on_mmap_miss(sc);
#endif
#endif

    Hz3LargeHdr* hdr = NULL;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
    hdr = hz3_large_s218_c1_batch_mmap_alloc(size, need, offset, sc, pad, 1);
#endif
    if (!hdr) {
        void* base = hz3_large_os_mmap(need);
        if (base == MAP_FAILED) {
            hz3_oom_note("large_mmap", need, 0);
            return NULL;
        }

        uintptr_t start = (uintptr_t)base + offset;
        uintptr_t user = (start + pad) & ~(uintptr_t)pad;

        hdr = (Hz3LargeHdr*)base;
        hdr->magic = HZ3_LARGE_MAGIC;
        hdr->req_size = size;
        hdr->map_base = base;
        hdr->map_size = need;
        hdr->user_ptr = (void*)user;
        hdr->next = NULL;
#if HZ3_LARGE_CACHE_ENABLE
        hdr->in_use = 1;
        hdr->next_free = NULL;
#endif
    }

#if HZ3_LARGE_CANARY_ENABLE
    hz3_large_debug_write_canary(hdr);
#endif
    hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_alloc("large_aligned_mmap", hdr->user_ptr, -1, -1);
#endif
    return hdr->user_ptr;
}

int hz3_large_free(void* ptr) {
    if (!ptr) {
        return 0;
    }

    Hz3LargeHdr* hdr = hz3_large_take(ptr);
    if (!hdr) {
        return 0;
    }
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_free_call();
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("large_free", ptr, -1, -1);
#endif
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && (HZ3_S186_LARGE_UNMAP_DEFER || HZ3_S212_LARGE_UNMAP_DEFER_PLUS)
    hz3_large_unmap_defer_drain_budget();
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
    // Safety: validate hdr before accessing map_size
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_size == 0) {
        goto do_munmap;
    }

    int sc = hz3_large_sc(hdr->map_size);

    // >64MB はキャッシュ対象外（巨大ブロックで cap を食い潰すのを防ぐ）
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT - 1) {
        goto do_munmap;
    }

    // NOTE: If we apply madvise(MADV_DONTNEED) for the freed mapping, we MUST
    // not make it visible to other threads (cache lists) until after the
    // madvise is done. Otherwise another thread can reallocate the same mapping
    // and observe its contents being discarded while "in use".
    //
    // Therefore we:
    //   1) decide evict/can_cache + madvise range under lock
    //   2) unlock + (maybe) madvise
    //   3) lock again and insert into cache (evict again if needed)
    // S53/S207-1: hard cap (optionally widened for high-band classes).
    size_t hard_cap = hz3_large_hard_cap_for_sc(sc);
    size_t insert_cap = hz3_large_insert_cap_for_sc(sc, hard_cap);
    size_t evict_budget_left = hz3_large_evict_budget_for_sc(sc);
    int evict_budget_exhausted = 0;

    int can_cache = 0;
#if HZ3_S184_LARGE_FREE_PRECHECK
    int force_lock_admission = hz3_large_reuse_trackable_sc(sc);
#endif

    // S53: soft 判定と madvise 範囲は lock 内で計算してローカルへ退避
#if HZ3_LARGE_CACHE_BUDGET
    int do_madvise = 0;
    uintptr_t madvise_start = 0;
    size_t madvise_size = 0;
#endif

#if HZ3_S184_LARGE_FREE_PRECHECK
    // S184: lock前に relaxed hint で fast admit を先読み。
    // 最終判定は後段の lock 再取得時に必ず行う。
    if (!force_lock_admission) {
        size_t total_hint = hz3_large_total_cached_hint_load();
        size_t projected_hint = total_hint + hdr->map_size;
        if (projected_hint >= total_hint && projected_hint <= hard_cap) {
            can_cache = 1;
#if HZ3_LARGE_CACHE_BUDGET
            do_madvise = (projected_hint > HZ3_LARGE_CACHE_SOFT_BYTES);
            if (do_madvise) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                madvise_start = (start + 4095) & ~(uintptr_t)4095;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (madvise_start < end) {
                    madvise_size = (end - madvise_start) & ~(size_t)4095;
                }
            }
#endif
        }
    }
#endif

    if (!can_cache) {
        hz3_large_cache_lock_acquire();

#if HZ3_S50_LARGE_SCACHE_EVICT
#if HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
        // Cap 超過時: evict してスペースを作る
        // S51: 同一 class を優先しつつ、victim をまとめて lock 外 munmap。
        for (;;) {
            Hz3LargeHdr* victims[HZ3_S185_EVICT_BATCH_MAX];
            size_t vcount = hz3_large_collect_evict_victims_locked(
                sc, hdr->map_size, hard_cap, victims, HZ3_S185_EVICT_BATCH_MAX, &evict_budget_left);
            if (vcount == 0) {
                break;
            }
            hz3_large_cache_lock_release();
            hz3_large_munmap_victims_unlocked(victims, vcount);
            hz3_large_cache_lock_acquire();
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#else
        // Cap 超過時: evict してスペースを作る
        // S51: 同一 class から先に evict（キャッシュ hit 率向上）
        while (hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
               evict_budget_left > 0) {
            Hz3LargeHdr* victim = NULL;
            // First: try same class
            victim = hz3_large_sc_pop_head(sc);
            if (!victim) {
                // Fallback: largest class
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    victim = hz3_large_sc_pop_head(i);
                    if (victim) {
                        break;
                    }
                }
            }
            if (!victim) break;  // evict 対象なし → munmap へ
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();  // hard 発火時のみ出力
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_large_cache_lock_release();
            hz3_large_dispose_victim_unlocked(victim);
            hz3_large_cache_lock_acquire();
            evict_budget_left--;
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#endif
#endif

        can_cache = (hz3_large_total_cached_load() + hdr->map_size <= insert_cap);
#if HZ3_LARGE_CACHE_BUDGET
        if (can_cache && !do_madvise) {
            do_madvise = ((hz3_large_total_cached_load() + hdr->map_size) > HZ3_LARGE_CACHE_SOFT_BYTES);
            if (do_madvise) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                madvise_start = (start + 4095) & ~(uintptr_t)4095;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (madvise_start < end) {
                    madvise_size = (end - madvise_start) & ~(size_t)4095;
                }
            }
        }
#endif

#if HZ3_S191_LARGE_FREE_FAST_INSERT && !HZ3_S207_TARGETED_REUSE
        // S191: If no madvise work is needed, keep the first lock and insert directly.
        if (can_cache) {
            int needs_unlock_for_madvise = 0;
#if HZ3_LARGE_CACHE_BUDGET
            needs_unlock_for_madvise = (do_madvise && madvise_size > 0);
#elif HZ3_S51_LARGE_MADVISE
            needs_unlock_for_madvise = 1;
#endif
            if (!needs_unlock_for_madvise) {
                if (hz3_large_total_cached_load() + hdr->map_size <= insert_cap) {
                    hdr->in_use = 0;
                    hdr->next = NULL;
                    hz3_large_sc_push_head(sc, hdr);
                    hz3_large_stats_on_free_cached();
                    hz3_large_cache_lock_release();
                    return 1;
                }
                can_cache = 0;
            }
        }
#endif

        hz3_large_cache_lock_release();
    }

    if (can_cache) {
        // S53: madvise は “cacheに入れる前” に実行する（reuse race 回避）
        // NOTE: hdr is not visible in any global list at this point.

        // S53: madvise は lock 外で実行
#if HZ3_LARGE_CACHE_BUDGET
#if HZ3_S193_DEMAND_SCAVENGE
        const int run_free_side_purge = 0;
#else
        const int run_free_side_purge = 1;
#endif
        if (run_free_side_purge && do_madvise && madvise_size > 0) {
#if HZ3_S53_THROTTLE
            // S53-2: Throttle check
            int do_actual_madvise = 1;
            uint32_t fc = atomic_fetch_add(&g_hz3_s53_free_counter, 1);
            if ((fc % HZ3_S53_MADVISE_MIN_INTERVAL) != 0) {
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_throttle_skips, 1);
#endif
                do_actual_madvise = 0;
                goto s53_done;
            }

            int64_t npages = (int64_t)(madvise_size >> 12);
            int64_t old_counter = atomic_fetch_sub(&g_hz3_s53_scavenge_counter_pages, npages);
            if (old_counter - npages >= 0) {
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_throttle_skips, 1);
#endif
                do_actual_madvise = 0;
                goto s53_done;
            }

            // 発火
            atomic_store(&g_hz3_s53_scavenge_counter_pages, HZ3_S53_MADVISE_RATE_PAGES);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_throttle_fires, 1);
#endif
s53_done:
            if (do_actual_madvise) {
                hz3_large_soft_purge((void*)madvise_start, madvise_size);
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
                atomic_fetch_add(&g_budget_soft_hits, 1);
#endif
            }
#if HZ3_LARGE_CACHE_STATS
            hz3_large_cache_stats_dump();
#endif
#else
            // THROTTLE=0: 従来通り毎回 madvise
            hz3_large_soft_purge((void*)madvise_start, madvise_size);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
            atomic_fetch_add(&g_budget_soft_hits, 1);
            hz3_large_cache_stats_dump();
#endif
#endif
        }
#elif HZ3_S51_LARGE_MADVISE
#if !HZ3_S193_DEMAND_SCAVENGE
        // S51: 既存の常時 madvise（BUDGET 無効時のみ）
        {
            uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
            uintptr_t aligned_start = (start + 4095) & ~(uintptr_t)4095;  // next page
            uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
            if (aligned_start < end) {
                size_t aligned_size = (end - aligned_start) & ~(size_t)4095;  // page truncate
                if (aligned_size > 0) {
                    hz3_large_soft_purge((void*)aligned_start, aligned_size);
                }
            }
        }
#endif
#endif
        // Finally: insert into cache (under lock). If we can no longer fit,
        // fall back to munmap (safe, but may lose caching benefits).
        hz3_large_cache_lock_acquire();

        // Recompute hard cap under lock (another thread may have changed totals)
        hard_cap = hz3_large_hard_cap_for_sc(sc);
        insert_cap = hz3_large_insert_cap_for_sc(sc, hard_cap);

#if HZ3_S50_LARGE_SCACHE_EVICT
#if HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
        for (;;) {
            Hz3LargeHdr* victims[HZ3_S185_EVICT_BATCH_MAX];
            size_t vcount = hz3_large_collect_evict_victims_locked(
                sc, hdr->map_size, hard_cap, victims, HZ3_S185_EVICT_BATCH_MAX, &evict_budget_left);
            if (vcount == 0) {
                break;
            }
            hz3_large_cache_lock_release();
            hz3_large_munmap_victims_unlocked(victims, vcount);
            hz3_large_cache_lock_acquire();
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#else
        while (hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
               evict_budget_left > 0) {
            Hz3LargeHdr* victim = NULL;
            victim = hz3_large_sc_pop_head(sc);
            if (!victim) {
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    victim = hz3_large_sc_pop_head(i);
                    if (victim) {
                        break;
                    }
                }
            }
            if (!victim) break;
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_large_cache_lock_release();
            hz3_large_dispose_victim_unlocked(victim);
            hz3_large_cache_lock_acquire();
            evict_budget_left--;
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#endif
#endif

        if (hz3_large_total_cached_load() + hdr->map_size <= insert_cap) {
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            if (hz3_large_total_cached_load() + hdr->map_size > hard_cap) {
                hz3_large_stat_inc(&g_s237_slack_admit_hits);
            }
#endif
            if (!hz3_large_reuse_try_admit_locked(sc)) {
                hz3_large_cache_lock_release();
                goto do_munmap;
            }
            hdr->in_use = 0;
            hdr->next = NULL;
            hz3_large_sc_push_head(sc, hdr);
            hz3_large_stats_on_free_cached();
            hz3_large_cache_lock_release();
            return 1;
        }

        hz3_large_cache_lock_release();
        goto do_munmap;
    }

    // Cache できない（cap超過など）: munmap へ
#else
    // Legacy: 上限チェック
    hz3_large_cache_lock_acquire();
    if (g_hz3_large_cached_bytes + hdr->map_size <= HZ3_LARGE_CACHE_MAX_BYTES &&
        g_hz3_large_cached_nodes < HZ3_LARGE_CACHE_MAX_NODES) {
        // Cache に追加
        hdr->in_use = 0;
        hdr->next = NULL;           // bucket list から切断済み
        hdr->next_free = g_hz3_large_free_head;
        g_hz3_large_free_head = hdr;
        g_hz3_large_cached_bytes += hdr->map_size;
        g_hz3_large_cached_nodes++;
        hz3_large_stats_on_free_cached();
        hz3_large_cache_lock_release();
        return 1;
    }
    hz3_large_cache_lock_release();
#endif
#endif

#if HZ3_S50_LARGE_SCACHE && HZ3_LARGE_CACHE_ENABLE
do_munmap:
#endif
    // Cache full または無効: munmap
    hz3_large_stats_on_free_munmap();
    hdr->magic = 0;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    if (hz3_large_try_defer_direct_unmap(hdr)) {
        return 1;
    }
#endif
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(hdr);
    hz3_large_cache_lock_release();
    hz3_large_os_munmap(hdr->map_base, hdr->map_size);
    return 1;
}

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
