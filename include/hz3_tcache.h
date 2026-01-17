#pragma once

#include "hz3_types.h"
#include "hz3_guardrails.h"
#include "hz3_sc.h"
#include "hz3_small.h"
#if HZ3_SUB4K_ENABLE
#include "hz3_sub4k.h"
#endif

// ============================================================================
// HZ3_OBJ_DEBUG: fail-fast intrusive list pointer checks
// ============================================================================
#ifndef HZ3_OBJ_DEBUG
#define HZ3_OBJ_DEBUG 0
#endif

#if HZ3_OBJ_DEBUG
#include "hz3_arena.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HZ3_OBJ_DEBUG_RA
#define HZ3_OBJ_DEBUG_RA HZ3_OBJ_DEBUG
#endif

static inline int hz3_obj_debug_in_arena(void* p) {
    if (!p) return 1;
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_relaxed);
    if (!base) return 1;  // arena not initialized yet
    return hz3_arena_page_index_fast(p, NULL);
}

static inline uintptr_t hz3_obj_debug_find_so_base(void) {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libhakozuna_hz3") == NULL) {
            continue;
        }
        char* dash = strchr(line, '-');
        if (!dash) {
            continue;
        }
        *dash = '\0';
        uintptr_t base = (uintptr_t)strtoull(line, NULL, 16);
        fclose(f);
        return base;
    }
    fclose(f);
    return 0;
}

static inline void hz3_obj_debug_fail(const char* where, void* obj, void* next, const char* msg) {
    uint32_t page_idx = 0;
    uint32_t tag = 0;
    int have_tag = 0;
#if HZ3_OBJ_DEBUG_RA
    void* ra0 = __builtin_extract_return_addr(__builtin_return_address(0));
#else
    void* ra0 = NULL;
#endif
    if (hz3_arena_page_index_fast(obj, &page_idx)) {
#if HZ3_PTAG_DSTBIN_ENABLE
        tag = hz3_pagetag32_load(page_idx);
        have_tag = (tag != 0);
#endif
    }
    uintptr_t so_base = hz3_obj_debug_find_so_base();
    unsigned long ra_off = 0;
    if (so_base && ra0) {
        ra_off = (unsigned long)((uintptr_t)ra0 - so_base);
    }
    if (have_tag) {
        uint32_t bin = hz3_pagetag32_bin(tag);
        uint8_t dst = hz3_pagetag32_dst(tag);
        fprintf(stderr,
                "[HZ3_OBJ_DEBUG] %s: %s obj=%p next=%p page_idx=%u tag=0x%08x bin=%u dst=%u ra=%p base=%p off=0x%lx\n",
                where, msg, obj, next, page_idx, tag, bin, (unsigned)dst,
                ra0, (void*)so_base, ra_off);
    } else {
        fprintf(stderr, "[HZ3_OBJ_DEBUG] %s: %s obj=%p next=%p page_idx=%u ra=%p base=%p off=0x%lx\n",
                where, msg, obj, next, page_idx, ra0, (void*)so_base, ra_off);
    }
    abort();
}

static inline void hz3_obj_debug_check_ptr(const char* where, void* p) {
    if (!hz3_obj_debug_in_arena(p)) {
        hz3_obj_debug_fail(where, p, NULL, "ptr out of arena");
    }
}

static inline void hz3_obj_debug_check_next(const char* where, void* obj, void* next) {
    if (next == obj) {
        hz3_obj_debug_fail(where, obj, next, "self-loop");
    }
    if (!hz3_obj_debug_in_arena(next)) {
        hz3_obj_debug_fail(where, obj, next, "next out of arena");
    }
}

#define OBJ_CHECK_PTR(where, p) hz3_obj_debug_check_ptr(where, p)
#define OBJ_CHECK_NEXT(where, obj, next) hz3_obj_debug_check_next(where, obj, next)
#else
#define OBJ_CHECK_PTR(where, p) ((void)0)
#define OBJ_CHECK_NEXT(where, obj, next) ((void)0)
#endif

// ============================================================================
// Intrusive linked list pointer operations (0-cost, debug-friendly)
// ============================================================================

// Get next pointer from object (intrusive list: next stored at obj[0])
static inline void* hz3_obj_get_next(void* obj) {
    OBJ_CHECK_PTR("obj_get_next", obj);
    void* next = *(void**)obj;
    OBJ_CHECK_NEXT("obj_get_next", obj, next);
    return next;
}

// Set next pointer in object
static inline void hz3_obj_set_next(void* obj, void* next) {
    OBJ_CHECK_PTR("obj_set_next", obj);
    OBJ_CHECK_NEXT("obj_set_next", obj, next);
    *(void**)obj = next;
}

// ============================================================================
// TLS Bin operations (push/pop)
// ============================================================================

// Push object to bin head (LIFO)
static inline void hz3_bin_push(Hz3Bin* bin, void* obj) {
    hz3_obj_set_next(obj, bin->head);
    bin->head = obj;
#if !HZ3_BIN_LAZY_COUNT
    bin->count++;
#endif
}

// Pop object from bin head (returns NULL if empty)
static inline void* hz3_bin_pop(Hz3Bin* bin) {
    void* obj = bin->head;
    if (obj) {
        bin->head = hz3_obj_get_next(obj);
#if !HZ3_BIN_LAZY_COUNT
        bin->count--;
#endif
    }
    return obj;
}

// ============================================================================
// S40-2: SoA binref push/pop (for HZ3_TCACHE_SOA_LOCAL=1 or HZ3_TCACHE_SOA_BANK=1)
// ============================================================================

#if HZ3_TCACHE_SOA_LOCAL || HZ3_TCACHE_SOA_BANK

#if HZ3_BIN_SPLIT_COUNT
// ============================================================================
// S122: Split Count version - low 4bit in head_tag, count_hi updated 1/16
// ============================================================================

// Helper: pack pointer + count_low into tagged value
static inline uintptr_t hz3_split_pack(void* ptr, uint32_t low) {
    return ((uintptr_t)ptr) | ((uintptr_t)low & HZ3_SPLIT_CNT_MASK);
}

// Helper: extract pointer from tagged value
static inline void* hz3_split_ptr(uintptr_t tagged) {
    return (void*)(tagged & HZ3_SPLIT_PTR_MASK);
}

// Helper: extract count_low from tagged value
static inline uint32_t hz3_split_low(uintptr_t tagged) {
    return (uint32_t)(tagged & HZ3_SPLIT_CNT_MASK);
}

// Get combined count from binref (count_hi << 4 | count_low)
static inline uint32_t hz3_binref_count_get(Hz3BinRef ref) {
    return ((uint32_t)(*ref.count) << HZ3_SPLIT_CNT_SHIFT)
         | hz3_split_low(*ref.head);
}

// Push object to binref head (LIFO) - Split Count version
// RMW on count_hi only when low wraps (1/16 frequency)
static inline void hz3_binref_push(Hz3BinRef ref, void* obj) {
    uintptr_t ht = *ref.head;
    void* old_head = hz3_split_ptr(ht);
    uint32_t low = hz3_split_low(ht);

    hz3_obj_set_next(obj, old_head);

    // count++ : wrap at 16
    low++;
    if (__builtin_expect(low == 16, 0)) {
        (*ref.count)++;
        low = 0;
    }

    *ref.head = hz3_split_pack(obj, low);
}

// Pop object from binref head (returns NULL if empty) - Split Count version
// RMW on count_hi only when low wraps (1/16 frequency)
static inline void* hz3_binref_pop(Hz3BinRef ref) {
    uintptr_t ht = *ref.head;
    void* obj = hz3_split_ptr(ht);
    if (!obj) return NULL;

    uint32_t low = hz3_split_low(ht);
    void* next = hz3_obj_get_next(obj);

    // count-- : wrap at 0
    if (__builtin_expect(low == 0, 0)) {
        (*ref.count)--;
        low = 15;
    } else {
        low--;
    }

    *ref.head = hz3_split_pack(next, low);
    return obj;
}

// Check if binref is empty - Split Count version
static inline int hz3_binref_is_empty(Hz3BinRef ref) {
    return hz3_split_ptr(*ref.head) == NULL;
}

// Prepend linked list to binref (tail->next = old_head) - Split Count version
static inline void hz3_binref_prepend_list(Hz3BinRef ref, void* head, void* tail, uint32_t n) {
    uintptr_t ht = *ref.head;
    void* old_head = hz3_split_ptr(ht);
    uint32_t low = hz3_split_low(ht);

    hz3_obj_set_next(tail, old_head);

    // count += n : compute new low and hi
    uint32_t total = low + n;
    uint32_t hi_delta = total >> HZ3_SPLIT_CNT_SHIFT;
    low = total & (uint32_t)HZ3_SPLIT_CNT_MASK;

    if (hi_delta) {
        *ref.count += (Hz3BinCount)hi_delta;
    }
    *ref.head = hz3_split_pack(head, low);
}

// S117: Prepend an *unlinked* pointer array to binref in LIFO order - Split Count version
static inline void hz3_binref_prepend_ptr_array_lifo(Hz3BinRef ref, void** arr, uint32_t n) {
    if (n == 0) return;
    uintptr_t ht = *ref.head;
    void* old_head = hz3_split_ptr(ht);
    uint32_t low = hz3_split_low(ht);

    // Tail of the resulting list is arr[0] (deepest); link it to old_head.
    hz3_obj_set_next(arr[0], old_head);
    // Build reverse chain so final head becomes arr[n-1].
    for (uint32_t i = 1; i < n; i++) {
        hz3_obj_set_next(arr[i], arr[i - 1]);
    }

    // count += n : compute new low and hi
    uint32_t total = low + n;
    uint32_t hi_delta = total >> HZ3_SPLIT_CNT_SHIFT;
    low = total & (uint32_t)HZ3_SPLIT_CNT_MASK;

    if (hi_delta) {
        *ref.count += (Hz3BinCount)hi_delta;
    }
    *ref.head = hz3_split_pack(arr[n - 1], low);
}

#else  // !HZ3_BIN_SPLIT_COUNT - Original implementation

// Push object to binref head (LIFO)
static inline void hz3_binref_push(Hz3BinRef ref, void* obj) {
    hz3_obj_set_next(obj, *ref.head);
    *ref.head = obj;
#if !HZ3_BIN_LAZY_COUNT
    (*ref.count)++;
#endif
}

// Pop object from binref head (returns NULL if empty)
static inline void* hz3_binref_pop(Hz3BinRef ref) {
    void* obj = *ref.head;
    if (obj) {
        *ref.head = hz3_obj_get_next(obj);
#if !HZ3_BIN_LAZY_COUNT
        (*ref.count)--;
#endif
    }
    return obj;
}

// Check if binref is empty
static inline int hz3_binref_is_empty(Hz3BinRef ref) {
    return *ref.head == NULL;
}

// Prepend linked list to binref (tail->next = old_head)
static inline void hz3_binref_prepend_list(Hz3BinRef ref, void* head, void* tail, uint32_t n) {
    hz3_obj_set_next(tail, *ref.head);
    *ref.head = head;
#if !HZ3_BIN_LAZY_COUNT
    *ref.count += n;
#else
    (void)n;
#endif
}

// S117: Prepend an *unlinked* pointer array to binref in LIFO order.
static inline void hz3_binref_prepend_ptr_array_lifo(Hz3BinRef ref, void** arr, uint32_t n) {
    if (n == 0) return;
    void* old_head = *ref.head;
    // Tail of the resulting list is arr[0] (deepest); link it to old_head.
    hz3_obj_set_next(arr[0], old_head);
    // Build reverse chain so final head becomes arr[n-1].
    for (uint32_t i = 1; i < n; i++) {
        hz3_obj_set_next(arr[i], arr[i - 1]);
    }
    *ref.head = arr[n - 1];
#if !HZ3_BIN_LAZY_COUNT
    *ref.count += n;
#else
    (void)n;
#endif
}

#endif  // HZ3_BIN_SPLIT_COUNT

#endif  // HZ3_TCACHE_SOA_LOCAL || HZ3_TCACHE_SOA_BANK

// S28-2B: nocount 版 push/pop (small hot path 用)
#if HZ3_SMALL_BIN_NOCOUNT
static inline void hz3_small_bin_push(Hz3Bin* bin, void* obj) {
    hz3_obj_set_next(obj, bin->head);
    bin->head = obj;
}

static inline void* hz3_small_bin_pop(Hz3Bin* bin) {
    void* obj = bin->head;
    if (obj) {
        bin->head = hz3_obj_get_next(obj);
    }
    return obj;
}
#else
#define hz3_small_bin_push hz3_bin_push
#define hz3_small_bin_pop hz3_bin_pop
#endif

// Check if bin is empty
static inline int hz3_bin_is_empty(Hz3Bin* bin) {
    return bin->head == NULL;
}

// Bin head accessors (event/slow paths should use these instead of direct field access)
static inline void* hz3_bin_head(const Hz3Bin* bin) {
    return bin->head;
}

static inline void hz3_bin_set_head(Hz3Bin* bin, void* head) {
    bin->head = head;
}

// Bin count accessors (event/slow paths only)
static inline Hz3BinCount hz3_bin_count_get(const Hz3Bin* bin) {
    return bin->count;
}

static inline void hz3_bin_count_set(Hz3Bin* bin, Hz3BinCount value) {
#if !HZ3_BIN_LAZY_COUNT
    bin->count = value;
#else
    (void)bin;
    (void)value;
#endif
}

static inline void hz3_bin_count_add(Hz3Bin* bin, Hz3BinCount delta) {
#if !HZ3_BIN_LAZY_COUNT
    bin->count += delta;
#else
    (void)bin;
    (void)delta;
#endif
}

static inline void hz3_bin_count_sub(Hz3Bin* bin, Hz3BinCount delta) {
#if !HZ3_BIN_LAZY_COUNT
    bin->count -= delta;
#else
    (void)bin;
    (void)delta;
#endif
}

// Clear bin contents (event-only)
static inline void hz3_bin_clear(Hz3Bin* bin) {
    bin->head = NULL;
    hz3_bin_count_set(bin, 0);
}

// Take all objects from bin (event-only)
static inline void* hz3_bin_take_all(Hz3Bin* bin) {
    void* head = bin->head;
    hz3_bin_clear(bin);
    return head;
}

// Prepend a list [head..tail] to bin (event/slow paths)
static inline void hz3_bin_prepend_list(Hz3Bin* bin, void* head, void* tail, uint32_t n) {
    hz3_obj_set_next(tail, bin->head);
    bin->head = head;
    hz3_bin_count_add(bin, (Hz3BinCount)n);
}

// Prepend list for small bins (respects HZ3_SMALL_BIN_NOCOUNT)
static inline void hz3_small_bin_prepend_list(Hz3Bin* bin, void* head, void* tail, uint32_t n) {
    hz3_obj_set_next(tail, bin->head);
    bin->head = head;
#if !HZ3_SMALL_BIN_NOCOUNT && !HZ3_BIN_LAZY_COUNT
    bin->count += (Hz3BinCount)n;
#else
    (void)n;
#endif
}

// S117: Prepend an *unlinked* pointer array to small bin in LIFO order.
// Equivalent to:
//   for i=0..n-1: hz3_small_bin_push(bin, arr[i])
// but updates head/count once. Writes next pointers into objects.
static inline void hz3_small_bin_prepend_ptr_array_lifo(Hz3Bin* bin, void** arr, uint32_t n) {
    if (n == 0) return;
    void* old_head = bin->head;
    hz3_obj_set_next(arr[0], old_head);
    for (uint32_t i = 1; i < n; i++) {
        hz3_obj_set_next(arr[i], arr[i - 1]);
    }
    bin->head = arr[n - 1];
#if !HZ3_SMALL_BIN_NOCOUNT && !HZ3_BIN_LAZY_COUNT
    bin->count += (Hz3BinCount)n;
#endif
}

// ============================================================================
// S99: Refill remainder macro (abstracts SoA/non-SoA difference)
// ============================================================================
// Usage: HZ3_REFILL_REMAINING(batch, got) in hz3_small_v2_alloc_slow()
// Requires: 'ref' (SoA) or 'bin' (non-SoA) in scope, 'batch' array, 'got' count.
#if HZ3_S99_ALLOC_SLOW_PREPEND_LIST
  #if HZ3_TCACHE_SOA_LOCAL
    #define HZ3_REFILL_REMAINING(batch, got) do { \
        if ((got) > 1) { \
            hz3_binref_prepend_list((ref), (batch)[1], (batch)[(got) - 1], (uint32_t)((got) - 1)); \
        } \
    } while(0)
  #else
    #define HZ3_REFILL_REMAINING(batch, got) do { \
        if ((got) > 1) { \
            hz3_small_bin_prepend_list((bin), (batch)[1], (batch)[(got) - 1], (uint32_t)((got) - 1)); \
        } \
    } while(0)
  #endif
#else
  #if HZ3_S117_REFILL_REMAINING_PREPEND_ARRAY
    #if HZ3_TCACHE_SOA_LOCAL
      #define HZ3_REFILL_REMAINING(batch, got) do { \
          if ((got) > 1) { \
              hz3_binref_prepend_ptr_array_lifo((ref), &(batch)[1], (uint32_t)((got) - 1)); \
          } \
      } while(0)
    #else
      #define HZ3_REFILL_REMAINING(batch, got) do { \
          if ((got) > 1) { \
              hz3_small_bin_prepend_ptr_array_lifo((bin), &(batch)[1], (uint32_t)((got) - 1)); \
          } \
      } while(0)
    #endif
  #else
    #if HZ3_TCACHE_SOA_LOCAL
      #define HZ3_REFILL_REMAINING(batch, got) do { \
          for (int _i = 1; _i < (got); _i++) { \
              hz3_binref_push((ref), (batch)[_i]); \
          } \
      } while(0)
    #else
      #define HZ3_REFILL_REMAINING(batch, got) do { \
          for (int _i = 1; _i < (got); _i++) { \
              hz3_small_bin_push((bin), (batch)[_i]); \
          } \
      } while(0)
    #endif
  #endif
#endif

// ============================================================================
// S38-1: Lazy count helpers (event-only, O(cap) walk for trim/drain)
// ============================================================================

#if HZ3_BIN_LAZY_COUNT
// Count list length up to limit (event-only, O(limit))
static inline uint16_t hz3_bin_count_walk(const Hz3Bin* bin, uint16_t limit) {
    uint16_t n = 0;
    void* p = bin->head;
    while (p && n < limit) {
        p = hz3_obj_get_next(p);
        n++;
    }
    return n;
}

// Detach after n elements: keep first n in bin, return head of detached tail.
// Returns NULL if list has <= n elements.
static inline void* hz3_bin_detach_after_n(Hz3Bin* bin, uint16_t n) {
    if (n == 0) {
        void* old_head = bin->head;
        bin->head = NULL;
        return old_head;
    }
    void* prev = bin->head;
    for (uint16_t i = 1; i < n && prev; i++) {
        prev = hz3_obj_get_next(prev);
    }
    if (!prev) return NULL;  // list has < n elements
    void* detached = hz3_obj_get_next(prev);
    hz3_obj_set_next(prev, NULL);  // cut the chain
    return detached;
}
#endif  // HZ3_BIN_LAZY_COUNT

// Convert unified bin index to usable size (bytes). Returns 0 if invalid.
static inline size_t hz3_bin_to_usable_size(uint32_t bin) {
    if (bin < HZ3_SMALL_NUM_SC) {
        return hz3_small_sc_to_size((int)bin);
    }
#if HZ3_SUB4K_ENABLE
    if (bin < HZ3_MEDIUM_BIN_BASE) {
        return hz3_sub4k_sc_to_size((int)(bin - HZ3_SUB4K_BIN_BASE));
    }
#endif
    if (bin < HZ3_BIN_TOTAL) {
        return hz3_sc_to_size((int)(bin - HZ3_MEDIUM_BIN_BASE));
    }
    return 0;
}

// ============================================================================
// TLS Cache
// ============================================================================

// Thread-local cache (defined in hz3_tcache.c)
extern __thread Hz3TCache t_hz3_cache;

// Ensure TLS cache is initialized (call before any bin access)
void hz3_tcache_ensure_init_slow(void);
static inline void hz3_tcache_ensure_init(void) {
    if (__builtin_expect(t_hz3_cache.initialized, 1)) {
        return;
    }
    hz3_tcache_ensure_init_slow();
}

// Shard collision indicator (threads > shards)
int hz3_shard_collision_detected(void);
uint32_t hz3_shard_live_count(uint8_t shard);

// Get bin for size class (assumes initialized)
#if HZ3_PTAG_DSTBIN_ENABLE
static inline int hz3_bin_index_small(int sc) {
    return sc;
}

#if HZ3_SUB4K_ENABLE
static inline int hz3_bin_index_sub4k(int sc) {
    return HZ3_SUB4K_BIN_BASE + sc;
}
#endif

static inline int hz3_bin_index_medium(int sc) {
    return HZ3_MEDIUM_BIN_BASE + sc;
}

// S40-2: LOCAL SoA accessor (only when HZ3_TCACHE_SOA_LOCAL=1)
#if HZ3_TCACHE_SOA_LOCAL
static inline Hz3BinRef hz3_tcache_get_local_binref(uint32_t bin) {
    HZ3_ASSERT_BIN_RANGE(bin);
    return (Hz3BinRef){
        .head  = &t_hz3_cache.local_head[bin],
        .count = &t_hz3_cache.local_count[bin]
    };
}

// ============================================================================
// Event-path helpers for local_head access (abstracts Split Count details)
// ============================================================================
#if HZ3_BIN_SPLIT_COUNT
// S122: Extract pointer from tagged local_head
static inline void* hz3_local_head_get_ptr(uint32_t bin_idx) {
    return hz3_split_ptr(t_hz3_cache.local_head[bin_idx]);
}

// S122: Clear local bin (tagged NULL)
static inline void hz3_local_head_clear(uint32_t bin_idx) {
    t_hz3_cache.local_head[bin_idx] = 0;
    t_hz3_cache.local_count[bin_idx] = 0;
}

#else  // !HZ3_BIN_SPLIT_COUNT
// Non-split-count: direct access
static inline void* hz3_local_head_get_ptr(uint32_t bin_idx) {
    return t_hz3_cache.local_head[bin_idx];
}

static inline void hz3_local_head_clear(uint32_t bin_idx) {
    t_hz3_cache.local_head[bin_idx] = NULL;
#if !HZ3_BIN_LAZY_COUNT
    t_hz3_cache.local_count[bin_idx] = 0;
#endif
}
#endif  // HZ3_BIN_SPLIT_COUNT

#endif  // HZ3_TCACHE_SOA_LOCAL

// S40-2: BANK SoA accessor (only when HZ3_TCACHE_SOA_BANK=1 and not sparse)
#if HZ3_TCACHE_SOA_BANK && !HZ3_REMOTE_STASH_SPARSE
static inline Hz3BinRef hz3_tcache_get_bank_binref(uint8_t dst, uint32_t bin) {
    HZ3_ASSERT_DST_RANGE(dst);
    HZ3_ASSERT_BIN_RANGE(bin);
    return (Hz3BinRef){
        .head  = &t_hz3_cache.bank_head[dst][bin],
        .count = &t_hz3_cache.bank_count[dst][bin]
    };
}
#endif  // HZ3_TCACHE_SOA_BANK && !HZ3_REMOTE_STASH_SPARSE

// S40-2: AoS accessors (when LOCAL or BANK is not SoA)
#if !HZ3_TCACHE_SOA_LOCAL
// AoS accessors (return Hz3Bin*)
	static inline Hz3Bin* hz3_tcache_get_bin(int sc) {
	#if HZ3_LOCAL_BINS_SPLIT
	    // S33: Use unified local_bins (no range check)
	    return &t_hz3_cache.local_bins[hz3_bin_index_medium(sc)];
	#else
	    return &t_hz3_cache.bank[t_hz3_cache.my_shard][hz3_bin_index_medium(sc)];
	#endif
	}

	static inline Hz3Bin* hz3_tcache_get_small_bin(int sc) {
	#if HZ3_LOCAL_BINS_SPLIT
	    // S33: Use unified local_bins (no range check)
	    return &t_hz3_cache.local_bins[hz3_bin_index_small(sc)];
	#else
	    return &t_hz3_cache.bank[t_hz3_cache.my_shard][hz3_bin_index_small(sc)];
	#endif
	}

#if HZ3_SUB4K_ENABLE
	static inline Hz3Bin* hz3_tcache_get_sub4k_bin(int sc) {
	#if HZ3_LOCAL_BINS_SPLIT
	    // S33: Use unified local_bins (no range check)
	    return &t_hz3_cache.local_bins[hz3_bin_index_sub4k(sc)];
	#else
	    return &t_hz3_cache.bank[t_hz3_cache.my_shard][hz3_bin_index_sub4k(sc)];
	#endif
	}
#endif  // HZ3_SUB4K_ENABLE

// S40-2: AoS bank accessor (when BANK is not SoA and not sparse)
#if !HZ3_TCACHE_SOA_BANK && !HZ3_REMOTE_STASH_SPARSE
static inline Hz3Bin* hz3_tcache_get_bank_bin(uint8_t dst, uint32_t bin) {
    HZ3_ASSERT_DST_RANGE(dst);
    HZ3_ASSERT_BIN_RANGE(bin);
    return &t_hz3_cache.bank[dst][bin];
}

#if HZ3_PTAG_DSTBIN_FLAT
static inline Hz3Bin* hz3_tcache_get_bank_bin_flat(uint32_t flat) {
    HZ3_ASSERT_FLAT_RANGE(flat);
    return &t_hz3_cache.bank[0][0] + flat;
}
#endif
#endif  // !HZ3_TCACHE_SOA_BANK && !HZ3_REMOTE_STASH_SPARSE

// S33: bin index から local bin を取得（分岐なし）
#if HZ3_LOCAL_BINS_SPLIT && !HZ3_TCACHE_SOA_LOCAL
static inline Hz3Bin* hz3_tcache_get_local_bin_from_bin_index(uint32_t bin) {
    // S33: Direct array access (no range check, bin comes from PTAG32)
    HZ3_ASSERT_BIN_RANGE(bin);
    return &t_hz3_cache.local_bins[bin];
}
#endif

#endif  // !HZ3_TCACHE_SOA_LOCAL
#else
// Non-PTAG_DSTBIN_ENABLE fallback
#if HZ3_TCACHE_SOA_LOCAL
// SoA layout requires PTAG for proper bin access (Hz3BinRef pattern)
#error "HZ3_TCACHE_SOA_LOCAL=1 requires HZ3_PTAG_DSTBIN_ENABLE=1"
#elif HZ3_LOCAL_BINS_SPLIT
static inline Hz3Bin* hz3_tcache_get_bin(int sc) {
    return &t_hz3_cache.local_bins[HZ3_MEDIUM_BIN_BASE + sc];
}

static inline Hz3Bin* hz3_tcache_get_small_bin(int sc) {
    return &t_hz3_cache.local_bins[sc];
}
#else
static inline Hz3Bin* hz3_tcache_get_bin(int sc) {
    return &t_hz3_cache.bins[sc];
}

static inline Hz3Bin* hz3_tcache_get_small_bin(int sc) {
    return &t_hz3_cache.small_bins[sc];
}
#endif
#endif

// ============================================================================
// Slow path functions (implemented in hz3_tcache.c)
// ============================================================================

// Allocate from slow path (called when bin is empty)
void* hz3_alloc_slow(int sc);

// S46: Global Pressure Box - pressure check and flush handler
#if HZ3_ARENA_PRESSURE_BOX
void hz3_pressure_check_and_flush(void);
#else
static inline void hz3_pressure_check_and_flush(void) {}
#endif

// ============================================================================
// Day 4: Outbox operations (implemented in hz3_tcache.c)
// ============================================================================

// Push object to outbox (flushes automatically when full)
void hz3_outbox_push(uint8_t owner, int sc, void* obj);

// Flush outbox to owner's inbox
void hz3_outbox_flush(uint8_t owner, int sc);

#if HZ3_PTAG_DSTBIN_ENABLE
// S24-1: Forward declarations for inline functions
void hz3_dstbin_flush_remote_budget(uint32_t budget_bins);
void hz3_dstbin_flush_remote_all(void);
#endif

// S41: Sparse RemoteStash push (scale lane)
#if HZ3_REMOTE_STASH_SPARSE
void hz3_remote_stash_push(uint8_t dst, uint32_t bin, void* ptr);
#endif
