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

#define HZ3_LARGE_MAGIC 0x485a334c41524745ULL  // "HZ3LARGE"
#define HZ3_LARGE_HASH_BITS 10
#define HZ3_LARGE_HASH_SIZE (1u << HZ3_LARGE_HASH_BITS)

static hz3_lock_t g_hz3_large_lock = HZ3_LOCK_INITIALIZER;
static Hz3LargeHdr* g_hz3_large_buckets[HZ3_LARGE_HASH_SIZE];

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
// S50: Per-class LIFO cache
static Hz3LargeHdr* g_sc_head[HZ3_LARGE_SC_COUNT];
static size_t g_sc_bytes[HZ3_LARGE_SC_COUNT];
static uint32_t g_sc_nodes[HZ3_LARGE_SC_COUNT];
static size_t g_total_cached_bytes = 0;  // 全体の cached bytes
#else
// Legacy: single LIFO
static Hz3LargeHdr* g_hz3_large_free_head = NULL;
static size_t g_hz3_large_cached_bytes = 0;
static size_t g_hz3_large_cached_nodes = 0;
#endif
#endif

// S53: LargeCacheBudgetBox statistics (one-shot)
#if HZ3_LARGE_CACHE_STATS
static _Atomic size_t g_budget_soft_hits = 0;
static _Atomic size_t g_budget_hard_evicts = 0;
static _Atomic size_t g_budget_madvise_bytes = 0;
// S53-2: Throttle stats
static _Atomic size_t g_throttle_skips = 0;
static _Atomic size_t g_throttle_fires = 0;

static void hz3_large_cache_stats_dump(void) {
    static _Atomic int g_stats_dumped = 0;
    if (atomic_exchange(&g_stats_dumped, 1) == 0) {
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE
        fprintf(stderr, "[HZ3_LARGE_CACHE_BUDGET] cached=%zu soft_hits=%zu hard_evicts=%zu "
                "madvise_bytes=%zu throttle_skips=%zu throttle_fires=%zu\n",
                g_total_cached_bytes,
                atomic_load(&g_budget_soft_hits),
                atomic_load(&g_budget_hard_evicts),
                atomic_load(&g_budget_madvise_bytes),
                atomic_load(&g_throttle_skips),
                atomic_load(&g_throttle_fires));
#endif
    }
}
#endif

// S53-2: ThrottleBox state
#if HZ3_LARGE_CACHE_BUDGET && HZ3_S53_THROTTLE
static _Atomic int64_t g_hz3_s53_scavenge_counter_pages = HZ3_S53_MADVISE_RATE_PAGES;
static _Atomic uint32_t g_hz3_s53_free_counter = 0;
#endif

static inline uint32_t hz3_large_hash_ptr(const void* ptr) {
    uintptr_t v = (uintptr_t)ptr;
    v >>= 4;
    v ^= v >> 33;
    v ^= v >> 11;
    return (uint32_t)v & (HZ3_LARGE_HASH_SIZE - 1u);
}

static inline size_t hz3_large_user_offset(void) {
    return (sizeof(Hz3LargeHdr) + (HZ3_LARGE_ALIGN - 1u)) & ~(HZ3_LARGE_ALIGN - 1u);
}

// S50: Page alignment helper
static inline size_t hz3_page_align(size_t n) {
    return (n + 4095) & ~(size_t)4095;
}

#if HZ3_S50_LARGE_SCACHE
// S50: size → class index
// 32KB〜64MB を 8 分割/倍（97 classes）
static inline int hz3_large_sc(size_t size) {
    if (size <= HZ3_LARGE_SC_MIN) return 0;
    if (size > HZ3_LARGE_SC_MAX) return HZ3_LARGE_SC_COUNT - 1;
    // ceil(log2) で bits を取得
    int bits = 64 - __builtin_clzl((uint64_t)(size - 1));
    // 32KB〜64KB (bits=16) が class 0..7 になるよう bits-16
    int base = (bits - 16) * 8;
    if (base < 0) return 0;  // safety clamp
    size_t range_min = 1UL << (bits - 1);
    int sub = (int)((size - range_min) * 8 / range_min);
    if (sub < 0) sub = 0;  // safety clamp
    if (sub > 7) sub = 7;  // クランプ
    int sc = base + sub;
    if (sc < 0) return 0;
    if (sc >= HZ3_LARGE_SC_COUNT) return HZ3_LARGE_SC_COUNT - 1;
    return sc;
}

// S50: class index → size (for mmap allocation)
// Returns the UPPER bound of the class (maximum size that fits in this class)
static inline size_t hz3_large_sc_size(int sc) {
    if (sc < 0) sc = 0;
    if (sc >= HZ3_LARGE_SC_COUNT - 1) return 0;  // >64MB: キャッシュ対象外
    int range = sc / 8;
    int sub = sc % 8;
    size_t base_size = 1UL << (15 + range);  // 32KB * 2^range
    // Upper bound: base + base * (sub + 1) / 8
    return base_size + base_size * (sub + 1) / 8;
}
#endif

static void hz3_large_insert(Hz3LargeHdr* hdr) {
    uint32_t idx = hz3_large_hash_ptr(hdr->user_ptr);
    hz3_lock_acquire(&g_hz3_large_lock);
    hdr->next = g_hz3_large_buckets[idx];
    g_hz3_large_buckets[idx] = hdr;
    hz3_large_debug_on_bucket_insert_locked(hdr);
    hz3_lock_release(&g_hz3_large_lock);
}

static Hz3LargeHdr* hz3_large_take(const void* ptr) {
    uint32_t idx = hz3_large_hash_ptr(ptr);
    hz3_lock_acquire(&g_hz3_large_lock);
    Hz3LargeHdr* prev = NULL;
    Hz3LargeHdr* cur = g_hz3_large_buckets[idx];
    while (cur) {
        // Debug: basic header sanity (one-shot) before deref-heavy checks.
        hz3_large_debug_check_bucket_node_locked(cur);
        if (cur->magic != HZ3_LARGE_MAGIC ||
            cur->map_base == NULL ||
            cur->map_size == 0 ||
            cur->user_ptr == NULL) {
            hz3_large_debug_bad_hdr_locked(cur);
        }
        hz3_large_debug_bad_size_locked(cur);
        if (cur->user_ptr == ptr && cur->magic == HZ3_LARGE_MAGIC) {
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_check_canary_free(cur);
#endif
#if HZ3_LARGE_CACHE_ENABLE
            // in_use==1 のみ許可（double-free 検出）
            if (cur->in_use != 1) {
                hz3_large_debug_bad_inuse(cur);
                hz3_lock_release(&g_hz3_large_lock);
                return NULL;
            }
#endif
            if (prev) {
                prev->next = cur->next;
            } else {
                g_hz3_large_buckets[idx] = cur->next;
            }
            cur->next = NULL;  // bucket list から切断
            hz3_large_debug_on_bucket_remove_locked(cur);
            hz3_lock_release(&g_hz3_large_lock);
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    hz3_lock_release(&g_hz3_large_lock);
    return NULL;
}

static int hz3_large_find_size(const void* ptr, size_t* size_out) {
    uint32_t idx = hz3_large_hash_ptr(ptr);
    hz3_lock_acquire(&g_hz3_large_lock);
    Hz3LargeHdr* cur = g_hz3_large_buckets[idx];
    while (cur) {
        if (cur->user_ptr == ptr && cur->magic == HZ3_LARGE_MAGIC) {
            *size_out = cur->req_size;
            hz3_lock_release(&g_hz3_large_lock);
            return 1;
        }
        cur = cur->next;
    }
    hz3_lock_release(&g_hz3_large_lock);
    return 0;
}

void* hz3_large_alloc(size_t size) {
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
        hz3_lock_acquire(&g_hz3_large_lock);

#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → sc+2)
        int try_sc = sc;
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        Hz3LargeHdr* hdr = NULL;
        for (; try_sc <= try_limit; try_sc++) {
            hdr = g_sc_head[try_sc];
            if (hdr && hdr->magic == HZ3_LARGE_MAGIC && hdr->map_size >= class_size) {
                break;
            }
            hdr = NULL;
        }
#else
        int try_sc = sc;
        Hz3LargeHdr* hdr = g_sc_head[sc];
        if (!(hdr && hdr->magic == HZ3_LARGE_MAGIC && hdr->map_size == class_size)) {
            hdr = NULL;
        }
#endif

        if (hdr) {
            g_sc_head[try_sc] = hdr->next_free;
            g_sc_bytes[try_sc] -= hdr->map_size;
            g_sc_nodes[try_sc]--;
            g_total_cached_bytes -= hdr->map_size;
            hz3_large_debug_on_cache_remove_locked(hdr);
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 0)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_lock_release(&g_hz3_large_lock);
                munmap(hdr->map_base, hdr->map_size);
                goto cache_miss_alloc;
            }
#endif
            hdr->user_ptr = (char*)hdr->map_base + offset;
            hdr->in_use = 1;
            hdr->req_size = size;
            hdr->next_free = NULL;
            hz3_lock_release(&g_hz3_large_lock);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_alloc_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
        hz3_lock_release(&g_hz3_large_lock);
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_alloc:
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#else
    // Legacy: Cache から探す（first-fit）
    hz3_lock_acquire(&g_hz3_large_lock);
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
            hz3_lock_release(&g_hz3_large_lock);

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
    hz3_lock_release(&g_hz3_large_lock);
#endif
#endif

    // Cache miss: mmap
    void* base = mmap(NULL, need, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        hz3_oom_note("large_mmap", need, 0);
        return NULL;
    }

    Hz3LargeHdr* hdr = (Hz3LargeHdr*)base;
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
        hz3_lock_acquire(&g_hz3_large_lock);

#if HZ3_S52_LARGE_BESTFIT
        // S52: best-fit fallback (sc → sc+1 → ...), aligned_alloc でも同じ方針
        int try_sc = sc;
        int try_limit = sc + HZ3_S52_BESTFIT_RANGE;
        if (try_limit >= HZ3_LARGE_SC_COUNT - 1) {
            try_limit = HZ3_LARGE_SC_COUNT - 2;
        }
        Hz3LargeHdr* hdr = NULL;
        for (; try_sc <= try_limit; try_sc++) {
            hdr = g_sc_head[try_sc];
            if (hdr && hdr->magic == HZ3_LARGE_MAGIC && hdr->map_size >= class_size) {
                break;
            }
            hdr = NULL;
        }
#else
        int try_sc = sc;
        Hz3LargeHdr* hdr = g_sc_head[sc];
        if (!(hdr && hdr->magic == HZ3_LARGE_MAGIC && hdr->map_size == class_size)) {
            hdr = NULL;
        }
#endif

        if (hdr) {
            g_sc_head[try_sc] = hdr->next_free;
            g_sc_bytes[try_sc] -= hdr->map_size;
            g_sc_nodes[try_sc]--;
            g_total_cached_bytes -= hdr->map_size;
            hz3_large_debug_on_cache_remove_locked(hdr);
#if HZ3_LARGE_CANARY_ENABLE
            if (!hz3_large_debug_check_canary_cache(hdr, 1)) {
                hz3_large_debug_on_munmap_locked(hdr);
                hz3_lock_release(&g_hz3_large_lock);
                munmap(hdr->map_base, hdr->map_size);
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
            hz3_lock_release(&g_hz3_large_lock);
#if HZ3_LARGE_CANARY_ENABLE
            hz3_large_debug_write_canary(hdr);
#endif
            hz3_large_insert(hdr);
#if HZ3_WATCH_PTR_BOX
            hz3_watch_ptr_on_alloc("large_aligned_cache", hdr->user_ptr, -1, -1);
#endif
            return hdr->user_ptr;
        }
        hz3_lock_release(&g_hz3_large_lock);
    }

#if HZ3_LARGE_CANARY_ENABLE && !HZ3_LARGE_FAILFAST
cache_miss_aligned:
#endif
    // Cache miss: mmap with class_size
    need = class_size;
#endif

    void* base = mmap(NULL, need, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        hz3_oom_note("large_mmap", need, 0);
        return NULL;
    }

    uintptr_t start = (uintptr_t)base + offset;
    uintptr_t user = (start + pad) & ~(uintptr_t)pad;

    Hz3LargeHdr* hdr = (Hz3LargeHdr*)base;
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
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("large_free", ptr, -1, -1);
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
    hz3_lock_acquire(&g_hz3_large_lock);

    // S53: hard cap の決定（MAX_BYTES との整合）
#if HZ3_LARGE_CACHE_BUDGET
    size_t hard_cap = HZ3_LARGE_CACHE_HARD_BYTES;
    if (hard_cap < HZ3_LARGE_CACHE_MAX_BYTES) {
        hard_cap = HZ3_LARGE_CACHE_MAX_BYTES;  // 安全側に倒す
    }
#else
    size_t hard_cap = HZ3_LARGE_CACHE_MAX_BYTES;
#endif

#if HZ3_S50_LARGE_SCACHE_EVICT
    // Cap 超過時: evict してスペースを作る
    // S51: 同一 class から先に evict（キャッシュ hit 率向上）
    while (g_total_cached_bytes + hdr->map_size > hard_cap) {
        Hz3LargeHdr* victim = NULL;
        int victim_sc = -1;
        // First: try same class
        if (g_sc_head[sc]) {
            victim = g_sc_head[sc];
            victim_sc = sc;
        } else {
            // Fallback: largest class
            for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                if (g_sc_head[i]) {
                    victim = g_sc_head[i];
                    victim_sc = i;
                    break;
                }
            }
        }
        if (!victim) break;  // evict 対象なし → munmap へ
        g_sc_head[victim_sc] = victim->next_free;
        g_sc_bytes[victim_sc] -= victim->map_size;
        g_sc_nodes[victim_sc]--;
        g_total_cached_bytes -= victim->map_size;
#if HZ3_LARGE_CACHE_STATS
        atomic_fetch_add(&g_budget_hard_evicts, 1);
        hz3_large_cache_stats_dump();  // hard 発火時のみ出力
#endif
        hz3_large_debug_on_munmap_locked(victim);
        hz3_lock_release(&g_hz3_large_lock);
        victim->magic = 0;
        munmap(victim->map_base, victim->map_size);
        hz3_lock_acquire(&g_hz3_large_lock);
    }
#endif

    int can_cache = (g_total_cached_bytes + hdr->map_size <= hard_cap);

    // S53: soft 判定と madvise 範囲は lock 内で計算してローカルへ退避
#if HZ3_LARGE_CACHE_BUDGET
    int do_madvise = 0;
    uintptr_t madvise_start = 0;
    size_t madvise_size = 0;
    if (can_cache) {
        do_madvise = ((g_total_cached_bytes + hdr->map_size) > HZ3_LARGE_CACHE_SOFT_BYTES);
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

    hz3_lock_release(&g_hz3_large_lock);

    if (can_cache) {
        // S53: madvise は “cacheに入れる前” に実行する（reuse race 回避）
        // NOTE: hdr is not visible in any global list at this point.

        // S53: madvise は lock 外で実行
#if HZ3_LARGE_CACHE_BUDGET
        if (do_madvise && madvise_size > 0) {
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
                madvise((void*)madvise_start, madvise_size, MADV_DONTNEED);
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
            madvise((void*)madvise_start, madvise_size, MADV_DONTNEED);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
            atomic_fetch_add(&g_budget_soft_hits, 1);
            hz3_large_cache_stats_dump();
#endif
#endif
        }
#elif HZ3_S51_LARGE_MADVISE
        // S51: 既存の常時 madvise（BUDGET 無効時のみ）
        {
            uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
            uintptr_t aligned_start = (start + 4095) & ~(uintptr_t)4095;  // next page
            uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
            if (aligned_start < end) {
                size_t aligned_size = (end - aligned_start) & ~(size_t)4095;  // page truncate
                if (aligned_size > 0) {
                    madvise((void*)aligned_start, aligned_size, MADV_DONTNEED);
                }
            }
        }
#endif
        // Finally: insert into cache (under lock). If we can no longer fit,
        // fall back to munmap (safe, but may lose caching benefits).
        hz3_lock_acquire(&g_hz3_large_lock);

        // Recompute hard cap under lock (another thread may have changed totals)
#if HZ3_LARGE_CACHE_BUDGET
        hard_cap = HZ3_LARGE_CACHE_HARD_BYTES;
        if (hard_cap < HZ3_LARGE_CACHE_MAX_BYTES) {
            hard_cap = HZ3_LARGE_CACHE_MAX_BYTES;
        }
#else
        hard_cap = HZ3_LARGE_CACHE_MAX_BYTES;
#endif

#if HZ3_S50_LARGE_SCACHE_EVICT
        while (g_total_cached_bytes + hdr->map_size > hard_cap) {
            Hz3LargeHdr* victim = NULL;
            int victim_sc = -1;
            if (g_sc_head[sc]) {
                victim = g_sc_head[sc];
                victim_sc = sc;
            } else {
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    if (g_sc_head[i]) {
                        victim = g_sc_head[i];
                        victim_sc = i;
                        break;
                    }
                }
            }
            if (!victim) break;
            g_sc_head[victim_sc] = victim->next_free;
            g_sc_bytes[victim_sc] -= victim->map_size;
            g_sc_nodes[victim_sc]--;
            g_total_cached_bytes -= victim->map_size;
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_lock_release(&g_hz3_large_lock);
            victim->magic = 0;
            munmap(victim->map_base, victim->map_size);
            hz3_lock_acquire(&g_hz3_large_lock);
        }
#endif

        if (g_total_cached_bytes + hdr->map_size <= hard_cap) {
            hdr->in_use = 0;
            hdr->next = NULL;
            hdr->next_free = g_sc_head[sc];
            g_sc_head[sc] = hdr;
            g_sc_bytes[sc] += hdr->map_size;
            g_sc_nodes[sc]++;
            g_total_cached_bytes += hdr->map_size;
            hz3_large_debug_on_cache_insert_locked(hdr);
            hz3_lock_release(&g_hz3_large_lock);
            return 1;
        }

        hz3_lock_release(&g_hz3_large_lock);
        goto do_munmap;
    }

    // Cache できない（cap超過など）: munmap へ
#else
    // Legacy: 上限チェック
    hz3_lock_acquire(&g_hz3_large_lock);
    if (g_hz3_large_cached_bytes + hdr->map_size <= HZ3_LARGE_CACHE_MAX_BYTES &&
        g_hz3_large_cached_nodes < HZ3_LARGE_CACHE_MAX_NODES) {
        // Cache に追加
        hdr->in_use = 0;
        hdr->next = NULL;           // bucket list から切断済み
        hdr->next_free = g_hz3_large_free_head;
        g_hz3_large_free_head = hdr;
        g_hz3_large_cached_bytes += hdr->map_size;
        g_hz3_large_cached_nodes++;
        hz3_lock_release(&g_hz3_large_lock);
        return 1;
    }
    hz3_lock_release(&g_hz3_large_lock);
#endif
#endif

#if HZ3_S50_LARGE_SCACHE && HZ3_LARGE_CACHE_ENABLE
do_munmap:
#endif
    // Cache full または無効: munmap
    hdr->magic = 0;
    hz3_lock_acquire(&g_hz3_large_lock);
    hz3_large_debug_on_munmap_locked(hdr);
    hz3_lock_release(&g_hz3_large_lock);
    munmap(hdr->map_base, hdr->map_size);
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
