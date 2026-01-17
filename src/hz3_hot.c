#define _GNU_SOURCE

#include "hz3.h"
#include "hz3_config.h"
#include "hz3_tcache.h"
#include "hz3_large.h"
#include "hz3_segmap.h"
#include "hz3_seg_hdr.h"
#include "hz3_small.h"
#include "hz3_small_v2.h"
#include "hz3_medium_debug.h"
#include "hz3_sub4k.h"
#include "hz3_watch_ptr.h"
#include "hz3_tag.h"
#include "hz3_arena.h"  // Task 3: for hz3_arena_contains_fast()

#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>  // S12-5C: for abort() in FAILFAST mode
#include <stdio.h>

// ============================================================================
// Include split .inc files (single TU maintained)
// ============================================================================

#include "hot/hz3_hot_fallback.inc"
#include "hot/hz3_hot_helpers.inc"
#include "hot/hz3_hot_stats.inc"
#include "hot/hz3_hot_guards.inc"
#include "hot/hz3_hot_medium.inc"
#include "hot/hz3_hot_dispatch.inc"

// ============================================================================
// Hot path implementations
// ============================================================================

void* hz3_malloc(size_t size) {
#if !HZ3_ENABLE || HZ3_SHIM_FORWARD_ONLY
    return hz3_next_malloc(size);
#else
    if (size == 0) {
        size = 1;
    }

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
    if (size <= HZ3_SMALL_MAX_SIZE) {
        void* small = hz3_small_v2_alloc(size);
        if (small) {
            return small;
        }
        return hz3_next_malloc(size);
    }
#else
    if (size <= HZ3_SMALL_MAX_SIZE) {
        void* small = hz3_small_alloc(size);
        if (small) {
            return small;
        }
        return hz3_next_malloc(size);
    }
#endif

#if HZ3_SUB4K_ENABLE
    if (size > HZ3_SMALL_MAX_SIZE && size < HZ3_SC_MIN_SIZE) {
        void* sub4k = hz3_sub4k_alloc(size);
        if (sub4k) {
            return sub4k;
        }
        return hz3_next_malloc(size);
    }
#endif

    if (size < HZ3_SC_MIN_SIZE) {
        size = HZ3_SC_MIN_SIZE;
    }

    // Convert size to size class
    int sc = hz3_sc_from_size(size);
    if (sc < 0) {
        if (size > HZ3_SC_MAX_SIZE) {
            return hz3_large_alloc(size);
        }
        // Out of range, fallback
        return hz3_next_malloc(size);
    }

    // Ensure TLS cache is initialized
#if !HZ3_TCACHE_INIT_ON_MISS
    hz3_tcache_ensure_init();
#endif
    // S32-1: When HZ3_TCACHE_INIT_ON_MISS=1, skip init here.
    // TLS is zero-initialized, so bin->head == NULL -> miss -> slow path -> init there.

#if HZ3_ARENA_PRESSURE_BOX
    // Pressure check: drop to slow path once per epoch (slow path flushes to central)
    {
        uint32_t cur = atomic_load_explicit(&g_hz3_arena_pressure_epoch, memory_order_acquire);
        if (__builtin_expect(cur != 0 && t_hz3_cache.last_pressure_epoch < cur, 0)) {
            return hz3_alloc_slow(sc);
        }
    }
#endif

    // Try to pop from bin (fast path)
#if HZ3_TCACHE_SOA_LOCAL
    void* obj = hz3_binref_pop(hz3_tcache_get_local_binref(hz3_bin_index_medium(sc)));
#else
    Hz3Bin* bin = hz3_tcache_get_bin(sc);
    void* obj = hz3_bin_pop(bin);
#endif
    if (obj) {
#if HZ3_S72_MEDIUM_DEBUG
        hz3_medium_boundary_check_ptr("medium_alloc_fast", obj, sc, t_hz3_cache.my_shard);
#endif
#if HZ3_WATCH_PTR_BOX
        hz3_watch_ptr_on_alloc("medium_alloc_fast", obj, sc, t_hz3_cache.my_shard);
#endif
        return obj;
    }

    // Slow path: allocate new
    return hz3_alloc_slow(sc);
#endif
}

// Slow path: all non-leaf cases (noinline to keep hz3_free small)
__attribute__((noinline))
static void hz3_free_slow(void* ptr);

void hz3_free(void* ptr) {
#if !HZ3_ENABLE || HZ3_SHIM_FORWARD_ONLY
    hz3_next_free(ptr);
#else
    // NULL check
    if (!ptr) {
        return;
    }

#if HZ3_S113_STATS
    // S113-0: Observe seg math potential at hz3_free entry point (before PTAG leaf)
    // This shows what percentage of ALL frees could use seg_math + page_bin_plus1
    {
        // Register atexit dump once
        if (!atomic_exchange_explicit(&g_s113_atexit_registered, 1, memory_order_relaxed)) {
            atexit(hz3_s113_atexit_dump);
        }
        atomic_fetch_add_explicit(&g_s113_calls, 1, memory_order_relaxed);

        void* arena_base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        if (!arena_base) {
            atomic_fetch_add_explicit(&g_s113_seg_math_miss, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s113_fallback_ptag, 1, memory_order_relaxed);
            goto s113_entry_done;
        }

        uintptr_t seg_base = (uintptr_t)ptr & ~((uintptr_t)HZ3_SEG_SIZE - 1);
        uintptr_t delta = seg_base - (uintptr_t)arena_base;
        uint32_t slot_idx = (uint32_t)(delta >> HZ3_SEG_SHIFT);

        if (!hz3_arena_slot_used(slot_idx)) {
            atomic_fetch_add_explicit(&g_s113_seg_math_miss, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s113_fallback_ptag, 1, memory_order_relaxed);
            goto s113_entry_done;
        }
        atomic_fetch_add_explicit(&g_s113_seg_math_hit, 1, memory_order_relaxed);

        Hz3SegHdr* hdr = (Hz3SegHdr*)seg_base;
        if (hdr->magic != HZ3_SEG_HDR_MAGIC) {
            atomic_fetch_add_explicit(&g_s113_fallback_ptag, 1, memory_order_relaxed);
            goto s113_entry_done;
        }

        size_t page_idx = ((uintptr_t)ptr - seg_base) >> HZ3_PAGE_SHIFT;
        if (page_idx == 0) {
            atomic_fetch_add_explicit(&g_s113_page0_skip, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s113_fallback_ptag, 1, memory_order_relaxed);
            goto s113_entry_done;
        }

#if HZ3_S110_META_ENABLE
        uint16_t bin_plus1 = atomic_load_explicit(&hdr->page_bin_plus1[page_idx], memory_order_acquire);
        if (bin_plus1 != 0) {
            atomic_fetch_add_explicit(&g_s113_meta_hit, 1, memory_order_relaxed);
        } else {
            atomic_fetch_add_explicit(&g_s113_meta_zero, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_s113_fallback_ptag, 1, memory_order_relaxed);
        }
#else
        atomic_fetch_add_explicit(&g_s113_meta_zero, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s113_fallback_ptag, 1, memory_order_relaxed);
#endif
    }
s113_entry_done:
    ; // Label needs a statement
#endif

    // S113-1: Try seg math fast path first (if enabled)
#if HZ3_S113_FREE_FAST_ENABLE && HZ3_S110_META_ENABLE
    if (hz3_free_try_s113_segmath(ptr)) {
        return;
    }
    // Fall through to PTAG32 (external allocations, uninitialized pages)
#endif

#if HZ3_FREE_LEAF_ENABLE
    if (hz3_free_try_ptag32_leaf(ptr)) {
        return;
    }
#endif
    hz3_free_slow(ptr);
#endif
}

// Include hz3_free_slow implementation
#include "hot/hz3_hot_free_slow.inc"

void* hz3_calloc(size_t nmemb, size_t size) {
#if !HZ3_ENABLE || HZ3_SHIM_FORWARD_ONLY
    return hz3_next_calloc(nmemb, size);
#else
    // Calculate total size with overflow check
    size_t total;
    if (__builtin_mul_overflow(nmemb, size, &total)) {
        return NULL;
    }

    // Try hz3 allocation
    void* ptr = hz3_malloc(total);
    if (!ptr) {
        return NULL;
    }

    // Zero memory (mmap gives zeroed pages, but be safe for reused allocations)
    __builtin_memset(ptr, 0, total);
    return ptr;
#endif
}

void* hz3_realloc(void* ptr, size_t size) {
#if !HZ3_ENABLE || HZ3_SHIM_FORWARD_ONLY
    return hz3_next_realloc(ptr, size);
#else
    // realloc(NULL, size) == malloc(size)
    if (!ptr) {
        return hz3_malloc(size);
    }

    // realloc(ptr, 0) == free(ptr), return NULL
    if (size == 0) {
        hz3_free(ptr);
        return NULL;
    }

#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_PTAG_DSTBIN_FASTLOOKUP && HZ3_REALLOC_PTAG32
    // S21: PTAG32-first realloc (bin->size via PTAG32)
    uint32_t tag32;
    int in_range = 0;
    if (hz3_pagetag32_lookup_fast(ptr, &tag32, &in_range)) {
        uint32_t bin = hz3_pagetag32_bin(tag32);
        size_t old_size = hz3_bin_to_usable_size(bin);
        if (old_size == 0) {
#if HZ3_PTAG_FAILFAST
            abort();
#else
            return NULL;
#endif
        }
        if (size <= old_size) {
            return ptr;
        }
        void* new_ptr = hz3_malloc(size);
        if (!new_ptr) {
            return NULL;
        }
        size_t copy_size = (size < old_size) ? size : old_size;
        __builtin_memcpy(new_ptr, ptr, copy_size);
        hz3_free(ptr);
        return new_ptr;
    }
    // S21-2: in_range && tag==0 -> fall through to slow path
    // (arena internal but tag not set, don't pass to foreign)
#if HZ3_PTAG_FAILFAST
    if (in_range) {
        abort();
    }
#endif
    // Fall through to slow path (PTAG32 fast path ended)
#endif

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
#if HZ3_SMALL_V2_PTAG_ENABLE
    // S12-4C: PageTagMap hot path (range check + 1 load only)
    uint32_t page_idx;
    if (hz3_arena_page_index_fast(ptr, &page_idx)) {
        uint16_t tag = hz3_pagetag_load(page_idx);
        if (tag != 0) {
            int sc, owner;
            hz3_pagetag_decode(tag, &sc, &owner);
            size_t old_size = hz3_small_sc_to_size(sc);  // tag decode gives size
            if (size <= old_size) {
                return ptr;
            }
            void* new_ptr = hz3_malloc(size);
            if (!new_ptr) {
                return NULL;
            }
            size_t copy_size = (size < old_size) ? size : old_size;
            __builtin_memcpy(new_ptr, ptr, copy_size);
            hz3_small_v2_free_by_tag(ptr, sc, owner);
            return new_ptr;
        }
        // tag == 0: not a small v2 page, fall through to v1 path
    }
#else
    // Task 3: page_hdr based detection (no seg_hdr cold reads)
    uint32_t arena_idx;
    void* arena_base;
    if (hz3_arena_contains_fast(ptr, &arena_idx, &arena_base)) {
        void* page_base = (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
        uint32_t magic = *(uint32_t*)page_base;
        if (magic == HZ3_PAGE_MAGIC) {
            size_t old_size = hz3_small_v2_usable_size(ptr);
            if (size <= old_size) {
                return ptr;
            }
            void* new_ptr = hz3_malloc(size);
            if (!new_ptr) {
                return NULL;
            }
            size_t copy_size = (size < old_size) ? size : old_size;
            __builtin_memcpy(new_ptr, ptr, copy_size);
            hz3_small_v2_free_fast(ptr, page_base);
            return new_ptr;
        }
        // Not a small v2 page - fall through to v1 path
    }
#endif // HZ3_SMALL_V2_PTAG_ENABLE
#endif // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

    // Check if ptr is ours
    Hz3SegMeta* meta = hz3_segmap_get(ptr);
    if (!meta) {
        size_t old_large_size = hz3_large_usable_size(ptr);
        if (old_large_size != 0) {
            if (size > HZ3_SC_MAX_SIZE && size <= old_large_size) {
                return ptr;
            }
            void* new_ptr = hz3_malloc(size);
            if (!new_ptr) {
                return NULL;
            }
            size_t copy_size = (size < old_large_size) ? size : old_large_size;
            __builtin_memcpy(new_ptr, ptr, copy_size);
            hz3_large_free(ptr);
            return new_ptr;
        }
        // Not our allocation, fallback
        return hz3_next_realloc(ptr, size);
    }

    // Get current size class (v1 path)
    void* seg_base = hz3_ptr_to_seg_base(ptr);
    size_t seg_page_idx = hz3_ptr_to_page_idx(ptr, seg_base);
    uint16_t tag = meta->sc_tag[seg_page_idx];

    if (tag == 0) {
        // Unknown, fallback
        return hz3_next_realloc(ptr, size);
    }

    uint8_t kind = hz3_tag_kind(tag);
    int old_sc = hz3_tag_sc(tag);
    size_t old_size = 0;
    if (kind == HZ3_TAG_KIND_SMALL) {
        old_size = hz3_small_sc_to_size(old_sc);
    } else if (kind == HZ3_TAG_KIND_LARGE) {
        old_size = hz3_sc_to_size(old_sc);
    } else {
        return hz3_next_realloc(ptr, size);
    }

    if (size <= old_size) {
        return ptr;
    }

    // Need to reallocate
    void* new_ptr = hz3_malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data (min of old and new size)
    size_t copy_size = (size < old_size) ? size : old_size;
    __builtin_memcpy(new_ptr, ptr, copy_size);

    // Free old
    hz3_free(ptr);

    return new_ptr;
#endif
}

// ============================================================================
// hz3_usable_size: Get usable size of hz3-allocated pointer
// Used by hybrid shim for cross-allocator realloc
// ============================================================================

size_t hz3_usable_size(void* ptr) {
    if (!ptr) {
        return 0;
    }

#if HZ3_PTAG_DSTBIN_ENABLE && HZ3_PTAG_DSTBIN_FASTLOOKUP && HZ3_USABLE_SIZE_PTAG32
    // S21: PTAG32-first usable_size (bin->size via PTAG32)
    uint32_t tag32;
    int in_range = 0;
    if (hz3_pagetag32_lookup_fast(ptr, &tag32, &in_range)) {
        uint32_t bin = hz3_pagetag32_bin(tag32);
        return hz3_bin_to_usable_size(bin);
    }
    // S21-2: in_range && tag==0 -> fall through to slow path
    // (arena internal but tag not set, don't pass to foreign)
#if HZ3_PTAG_FAILFAST
    if (in_range) {
        abort();
    }
#endif
    // Fall through to slow path (PTAG32 fast path ended)
#endif

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
#if HZ3_SMALL_V2_PTAG_ENABLE
    // S12-4C: PageTagMap hot path (range check + 1 load only)
    uint32_t page_idx;
    if (hz3_arena_page_index_fast(ptr, &page_idx)) {
        uint16_t tag = hz3_pagetag_load(page_idx);
        if (tag != 0) {
            int sc, owner;
            (void)owner;  // unused for usable_size
            hz3_pagetag_decode(tag, &sc, &owner);
            return hz3_small_sc_to_size(sc);
        }
        // tag == 0: fallback to page header (PTAG16 tag may be missing in fast lane)
        void* page_base = (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
        uint32_t magic = *(uint32_t*)page_base;
        if (magic == HZ3_PAGE_MAGIC) {
            return hz3_small_v2_usable_size(ptr);
        }
        // Not a small v2 page - fall through to v1 path
    }
#else
    // Task 3: page_hdr based detection (no seg_hdr cold reads)
    uint32_t arena_idx;
    void* arena_base;
    if (hz3_arena_contains_fast(ptr, &arena_idx, &arena_base)) {
        void* page_base = (void*)((uintptr_t)ptr & ~((uintptr_t)HZ3_PAGE_SIZE - 1u));
        uint32_t magic = *(uint32_t*)page_base;
        if (magic == HZ3_PAGE_MAGIC) {
            return hz3_small_v2_usable_size(ptr);
        }
        // Not a small v2 page - fall through to v1 path
    }
#endif // HZ3_SMALL_V2_PTAG_ENABLE
#endif // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

    // Look up segment metadata
    Hz3SegMeta* meta = hz3_segmap_get(ptr);
    if (!meta) {
        return hz3_large_usable_size(ptr);
    }

    // Get page index within segment (v1 path)
    void* seg_base = hz3_ptr_to_seg_base(ptr);
    size_t seg_page_idx = hz3_ptr_to_page_idx(ptr, seg_base);

    // Read tag
    uint16_t tag = meta->sc_tag[seg_page_idx];
    if (tag == 0) {
        return 0;  // Not cached / unknown
    }

    // Convert tag to size class, then to size
    uint8_t kind = hz3_tag_kind(tag);
    int sc = hz3_tag_sc(tag);
    if (kind == HZ3_TAG_KIND_SMALL) {
        return hz3_small_sc_to_size(sc);
    }
    if (kind == HZ3_TAG_KIND_LARGE) {
        return hz3_sc_to_size(sc);
    }
    return 0;
}
