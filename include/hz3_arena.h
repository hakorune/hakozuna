#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include "hz3_config.h"
#include "hz3_types.h"
#include "hz3_guardrails.h"

// Reserved arena for self-describing segments
#ifndef HZ3_ARENA_SIZE
#define HZ3_ARENA_SIZE (1ULL << 32)  // 4GB
#endif
// NOTE: scale lane (HZ3_NUM_SHARDS=32) overrides this in Makefile to 16GB.

// Page constants for arena
#define HZ3_ARENA_PAGE_SHIFT 12
#define HZ3_ARENA_PAGE_SIZE  (1u << HZ3_ARENA_PAGE_SHIFT)
#define HZ3_ARENA_MAX_PAGES  (HZ3_ARENA_SIZE >> HZ3_ARENA_PAGE_SHIFT)

// Atomic base/end for lock-free range check (published in init_slow)
extern _Atomic(void*) g_hz3_arena_base;
extern _Atomic(void*) g_hz3_arena_end;

#if HZ3_PTAG_DSTBIN_TLS
// Avoid including hz3_tcache.h in this header: Hz3TCache is defined in hz3_types.h.
extern __thread Hz3TCache t_hz3_cache;
#endif

#if HZ3_SMALL_V2_PTAG_ENABLE
// Page tag array: 0 = not ours, non-zero = encoded (sc, owner, kind)
extern _Atomic(uint16_t)* g_hz3_page_tag;
#endif

#if HZ3_PTAG_DSTBIN_ENABLE
// S17: 32-bit PageTagMap (dst/bin direct)
extern _Atomic(uint32_t)* g_hz3_page_tag32;
#endif

// Slow init: calls pthread_once, use in alloc/segment creation path only
void hz3_arena_init_slow(void);

// Legacy alias for compatibility
static inline void hz3_arena_init(void) { hz3_arena_init_slow(); }

// Fast contains: reads base atomically, returns 0 if arena not yet initialized.
// Does NOT call pthread_once (safe for hot path in free).
HZ3_WARN_UNUSED_RESULT int hz3_arena_contains_fast(const void* ptr, uint32_t* idx_out, void** base_out);

// Legacy contains (calls init_slow, use only in slow paths)
HZ3_WARN_UNUSED_RESULT int hz3_arena_contains(const void* ptr, uint32_t* idx_out, void** base_out);

// Allocate a 2MB slot inside arena (maps memory), returns base or NULL.
void* hz3_arena_alloc(uint32_t* idx_out);

// S12-5A: Free a 2MB slot inside arena (madvise DONTNEED, clears used[])
void hz3_arena_free(uint32_t idx);

// ----------------------------------------------------------------------------
// S45: Arena accessor API (for mem_budget segment scan)
// ----------------------------------------------------------------------------
void* hz3_arena_get_base(void);
uint32_t hz3_arena_slots(void);
int hz3_arena_slot_used(uint32_t idx);

// ----------------------------------------------------------------------------
// S46: Global Pressure Box (arena exhaustion broadcast)
// ----------------------------------------------------------------------------
#if HZ3_ARENA_PRESSURE_BOX
// Pressure epoch: incremented on arena exhaustion, each thread flushes once per epoch
extern _Atomic(uint32_t) g_hz3_arena_pressure_epoch;
#endif

// ----------------------------------------------------------------------------
// S47: Segment Quarantine (arena slot usage tracking)
// ----------------------------------------------------------------------------
#if HZ3_S47_SEGMENT_QUARANTINE
// Accessor for used/total slots (for headroom detection)
uint32_t hz3_arena_used_slots(void);
uint32_t hz3_arena_total_slots(void);
// S47-PolicyBox: Accessor for free_gen (for bounded wait coordination)
uint32_t hz3_arena_free_gen(void);
#endif

#if HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
// S12-4B: Clear page tags for entire segment (call when segment freed/recycled)
void hz3_arena_clear_segment_tags(void* seg_base);
#endif

// ----------------------------------------------------------------------------
// Hot path API: range check only (does NOT read used[], for PageTagMap)
// ----------------------------------------------------------------------------

// Fast page index: range check + page_idx only, does NOT read used[].
// Returns 1 if ptr is within arena range, 0 otherwise.
// This is the minimal check for PageTagMap: range compare only, no memory read.
HZ3_WARN_UNUSED_RESULT static inline int hz3_arena_page_index_fast(const void* ptr, uint32_t* page_idx_out) {
#if HZ3_PTAG_DSTBIN_TLS
    // S114: cache arena_base in TLS to avoid the atomic load + end load.
    // Note: base is immutable after init; on first touch we load with acquire.
    void* base = t_hz3_cache.arena_base;
    if (__builtin_expect(!base, 0)) {
        base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        t_hz3_cache.arena_base = base;
    }
    if (__builtin_expect(!base, 0)) {
        return 0;  // Not initialized yet, false negative is OK
    }
    uintptr_t delta = (uintptr_t)ptr - (uintptr_t)base;
  #if HZ3_ARENA_SIZE == (1ULL << 32)
    if (__builtin_expect((delta >> 32) != 0, 0)) {
  #else
    if (__builtin_expect(delta >= (uintptr_t)HZ3_ARENA_SIZE, 0)) {
  #endif
        return 0;
    }
    if (page_idx_out) {
        *page_idx_out = (uint32_t)(delta >> HZ3_ARENA_PAGE_SHIFT);
    }
    return 1;
#else
    // Default: minimal, conservative range check (no TLS dependency).
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (__builtin_expect(!base, 0)) {
        return 0;  // Not initialized yet, false negative is OK
    }
    void* end = atomic_load_explicit(&g_hz3_arena_end, memory_order_relaxed);
    if (ptr < base || ptr >= end) {
        return 0;  // Outside arena range
    }
    if (page_idx_out) {
        *page_idx_out = (uint32_t)(((uintptr_t)ptr - (uintptr_t)base) >> HZ3_ARENA_PAGE_SHIFT);
    }
    return 1;
#endif
}

#if HZ3_SMALL_V2_PTAG_ENABLE
// ----------------------------------------------------------------------------
// PageTagMap helpers (S12-4 + S12-5B extensions)
// ----------------------------------------------------------------------------

// S12-5B: Static asserts for tag encoding bit widths
#include "hz3_types.h"
_Static_assert(HZ3_SMALL_NUM_SC <= 255, "sc must fit in 8 bits");
#if HZ3_PTAG32_ONLY
_Static_assert(HZ3_NUM_SHARDS <= 255, "owner must fit in 8 bits (PTAG32-only)");
#else
_Static_assert(HZ3_NUM_SHARDS <= 63, "owner must fit in 6 bits");
#endif

// S12-5B Tag encoding with kind:
// - Bits 14-15: kind (2 bits)
//   00 = reserved (tag=0 means not ours)
//   01 = SMALL_V2
//   10 = SMALL_V1_MEDIUM (4KB-32KB)
//   11 = reserved
// - Bits 8-13: owner (6 bits, max 63 shards)
// - Bits 0-7: sc (8 bits, max 255 size classes)
// Note: HZ3_PTAG_LAYOUT_V2=1 uses 0-based encoding (no +1/-1)

#define PTAG_KIND_V2         1
#define PTAG_KIND_V1_MEDIUM  2

#if HZ3_PTAG_LAYOUT_V2
// S16-2: New encoding - sc/owner stored 0-based (no +1)
// kind is non-zero for valid tags, so tag==0 still means "not ours"

static inline uint16_t hz3_pagetag_encode(int sc, int owner) {
    return (uint16_t)(sc | (owner << 8) | (PTAG_KIND_V2 << 14));
}

static inline void hz3_pagetag_decode(uint16_t tag, int* sc, int* owner) {
    *sc = tag & 0xFF;
    *owner = (tag >> 8) & 0x3F;
}

static inline uint16_t hz3_pagetag_encode_v1(int sc, int owner) {
    return (uint16_t)(sc | (owner << 8) | (PTAG_KIND_V1_MEDIUM << 14));
}

static inline void hz3_pagetag_decode_with_kind(uint16_t tag, int* sc, int* owner, int* kind) {
    *kind = tag >> 14;           // top 2 bits, no mask needed
    *owner = (tag >> 8) & 0x3F;  // 6 bits
    *sc = tag & 0xFF;            // 8 bits
}

#else
// Legacy encoding - sc+1, owner+1 (S12-4/S12-5B original)

static inline uint16_t hz3_pagetag_encode(int sc, int owner) {
    return (uint16_t)((sc + 1) | ((owner + 1) << 8) | (PTAG_KIND_V2 << 14));
}

static inline void hz3_pagetag_decode(uint16_t tag, int* sc, int* owner) {
    *sc = (tag & 0xFF) - 1;
    *owner = ((tag >> 8) & 0x3F) - 1;
}

static inline uint16_t hz3_pagetag_encode_v1(int sc, int owner) {
    return (uint16_t)((sc + 1) | ((owner + 1) << 8) | (PTAG_KIND_V1_MEDIUM << 14));
}

static inline void hz3_pagetag_decode_with_kind(uint16_t tag, int* sc, int* owner, int* kind) {
    *kind = (tag >> 14) & 0x3;
    *owner = ((tag >> 8) & 0x3F) - 1;
    *sc = (tag & 0xFF) - 1;
}

#endif // HZ3_PTAG_LAYOUT_V2

// Fast tag load for hot path (relaxed, no fence needed for read-only)
static inline uint16_t hz3_pagetag_load(uint32_t page_idx) {
    return atomic_load_explicit(&g_hz3_page_tag[page_idx], memory_order_relaxed);
}

// Event-only tag set (relaxed store) - for v2 (uses hz3_pagetag_encode)
static inline void hz3_pagetag_set(uint32_t page_idx, int sc, int owner) {
    uint16_t tag = hz3_pagetag_encode(sc, owner);
    atomic_store_explicit(&g_hz3_page_tag[page_idx], tag, memory_order_relaxed);
}

// S12-5B: Direct tag store (relaxed) - for pre-encoded tags
static inline void hz3_pagetag_store(uint32_t page_idx, uint16_t tag) {
    atomic_store_explicit(&g_hz3_page_tag[page_idx], tag, memory_order_relaxed);
}

// Event-only tag clear (relaxed store)
static inline void hz3_pagetag_clear(uint32_t page_idx) {
    atomic_store_explicit(&g_hz3_page_tag[page_idx], 0, memory_order_relaxed);
}
#endif // HZ3_SMALL_V2_PTAG_ENABLE

#if HZ3_PTAG_DSTBIN_ENABLE
// ----------------------------------------------------------------------------
// S17: PTAG dst/bin direct (32-bit)
// ----------------------------------------------------------------------------

#if HZ3_PTAG_DSTBIN_FLAT
_Static_assert((HZ3_BIN_TOTAL * HZ3_NUM_SHARDS) <= 0xFFFFFFFFu,
               "flat bin index must fit in 32 bits");

static inline uint32_t hz3_pagetag32_encode_flat(uint32_t flat) {
    return flat + 1u;
}

static inline uint32_t hz3_pagetag32_encode(int bin, int dst) {
    HZ3_ASSERT_BIN_RANGE(bin);
    HZ3_ASSERT_DST_RANGE(dst);
    uint32_t flat = (uint32_t)dst * (uint32_t)HZ3_BIN_TOTAL + (uint32_t)bin;
    return hz3_pagetag32_encode_flat(flat);
}

static inline uint32_t hz3_pagetag32_flat(uint32_t tag) {
    return tag - 1u;
}

static inline uint32_t hz3_pagetag32_bin(uint32_t tag) {
    return hz3_pagetag32_flat(tag) % (uint32_t)HZ3_BIN_TOTAL;
}

static inline uint8_t hz3_pagetag32_dst(uint32_t tag) {
    return (uint8_t)(hz3_pagetag32_flat(tag) / (uint32_t)HZ3_BIN_TOTAL);
}
#else
_Static_assert(HZ3_BIN_TOTAL <= 65535, "bin index must fit in 16 bits");
_Static_assert(HZ3_NUM_SHARDS <= 255, "dst shard must fit in 8 bits");

static inline uint32_t hz3_pagetag32_encode(int bin, int dst) {
    HZ3_ASSERT_BIN_RANGE(bin);
    HZ3_ASSERT_DST_RANGE(dst);
    return (uint32_t)((uint32_t)(bin + 1) | ((uint32_t)(dst + 1) << 16));
}

static inline uint32_t hz3_pagetag32_bin(uint32_t tag) {
    return (tag & 0xFFFFu) - 1u;
}

static inline uint8_t hz3_pagetag32_dst(uint32_t tag) {
    return (uint8_t)((tag >> 16) & 0xFFu) - 1u;
}
#endif

static inline uint32_t hz3_pagetag32_load(uint32_t page_idx) {
    return atomic_load_explicit(&g_hz3_page_tag32[page_idx], memory_order_relaxed);
}

#if HZ3_S79_PTAG32_STORE_TRACE
void hz3_ptag32_store_trace(uint32_t page_idx, uint32_t old_tag, uint32_t new_tag, const char* where);
#endif
#if HZ3_S92_PTAG32_STORE_LAST
// Capture the latest pagetag32_store event (best-effort, debug-only).
void hz3_ptag32_store_last_capture(uint32_t page_idx, uint32_t old_tag, uint32_t new_tag, void* ra);
// Snapshot the latest pagetag32_store event (best-effort, debug-only).
int hz3_ptag32_store_last_snapshot(uint64_t* seq_out,
                                  uint32_t* page_idx_out,
                                  uint32_t* old_tag_out,
                                  uint32_t* new_tag_out,
                                  void** ra_out,
                                  uintptr_t* so_base_out,
                                  unsigned long* off_out);
#endif
#if HZ3_S88_PTAG32_CLEAR_TRACE
void hz3_ptag32_clear_trace(uint32_t page_idx, uint32_t old_tag, const char* where);
// Snapshot the most recent pagetag32_clear event (best-effort, debug-only).
// Returns 1 if a snapshot exists (seq!=0), else 0.
int hz3_ptag32_clear_last_snapshot(uint64_t* seq_out,
                                  uint32_t* page_idx_out,
                                  uint32_t* old_tag_out,
                                  void** ra_out,
                                  uintptr_t* so_base_out,
                                  unsigned long* off_out);
#if HZ3_S98_PTAG32_CLEAR_MAP
// Lookup clear record for a specific page_idx (best-effort, debug-only).
// Returns 1 if found, else 0.
int hz3_ptag32_clear_map_lookup(uint32_t page_idx,
                                uint64_t* seq_out,
                                uint32_t* old_tag_out,
                                void** ra_out,
                                uintptr_t* so_base_out,
                                unsigned long* off_out);
#endif
#endif

static inline void hz3_pagetag32_store(uint32_t page_idx, uint32_t tag) {
#if HZ3_S79_PTAG32_STORE_TRACE && HZ3_S79_PTAG32_STORE_EXCHANGE
    uint32_t old_tag = atomic_exchange_explicit(&g_hz3_page_tag32[page_idx], tag, memory_order_relaxed);
#if HZ3_S92_PTAG32_STORE_LAST
    hz3_ptag32_store_last_capture(page_idx, old_tag, tag,
                                 __builtin_extract_return_addr(__builtin_return_address(0)));
#endif
    if (old_tag != 0 && old_tag != tag) {
        hz3_ptag32_store_trace(page_idx, old_tag, tag, "pagetag32_store");
    }
#else
#if HZ3_S79_PTAG32_STORE_TRACE || HZ3_S92_PTAG32_STORE_LAST
    uint32_t old_tag0 = atomic_load_explicit(&g_hz3_page_tag32[page_idx], memory_order_relaxed);
#endif
#if HZ3_S79_PTAG32_STORE_TRACE
    if (old_tag0 != 0 && old_tag0 != tag) {
        hz3_ptag32_store_trace(page_idx, old_tag0, tag, "pagetag32_store");
    }
#endif
#if HZ3_S92_PTAG32_STORE_LAST
    hz3_ptag32_store_last_capture(page_idx, old_tag0, tag,
                                 __builtin_extract_return_addr(__builtin_return_address(0)));
#endif
    atomic_store_explicit(&g_hz3_page_tag32[page_idx], tag, memory_order_relaxed);
#endif
}

static inline void hz3_pagetag32_clear(uint32_t page_idx) {
#if HZ3_S88_PTAG32_CLEAR_TRACE
    uint32_t old_tag = atomic_exchange_explicit(&g_hz3_page_tag32[page_idx], 0, memory_order_relaxed);
    if (old_tag != 0) {
        hz3_ptag32_clear_trace(page_idx, old_tag, "pagetag32_clear");
    }
#else
    atomic_store_explicit(&g_hz3_page_tag32[page_idx], 0, memory_order_relaxed);
#endif
}

#if HZ3_PTAG_DSTBIN_FASTLOOKUP
// S18-1: range check + tag load in one helper (false negative OK)
HZ3_WARN_UNUSED_RESULT static inline int hz3_pagetag32_lookup_fast(const void* ptr, uint32_t* tag_out, int* in_range_out) {
    // Acquire load base to synchronize with release store in do_init.
    // S114: when enabled, cache arena_base + tag32 base in TLS.
#if HZ3_PTAG_DSTBIN_TLS
    void* base = t_hz3_cache.arena_base;
    if (__builtin_expect(!base, 0)) {
        base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        t_hz3_cache.arena_base = base;
    }
    _Atomic(uint32_t)* tag32_base = t_hz3_cache.page_tag32;
    if (__builtin_expect(!tag32_base, 0)) {
        tag32_base = g_hz3_page_tag32;
        t_hz3_cache.page_tag32 = tag32_base;
    }
#else
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    _Atomic(uint32_t)* tag32_base = g_hz3_page_tag32;
#endif
    if (__builtin_expect(!base, 0)) {
        if (in_range_out) {
            *in_range_out = 0;
        }
        return 0;
    }
    if (__builtin_expect(!tag32_base, 0)) {
        if (in_range_out) {
            *in_range_out = 0;
        }
        return 0;
    }
    uintptr_t delta = (uintptr_t)ptr - (uintptr_t)base;
#if HZ3_ARENA_SIZE == (1ULL << 32)
    if (__builtin_expect((delta >> 32) != 0, 0)) {
#else
    if (__builtin_expect(delta >= (uintptr_t)HZ3_ARENA_SIZE, 0)) {
#endif
        if (in_range_out) {
            *in_range_out = 0;
        }
        return 0;
    }
	    if (in_range_out) {
	        *in_range_out = 1;
	    }
	    uint32_t page_idx = (uint32_t)(delta >> HZ3_ARENA_PAGE_SHIFT);
#if HZ3_PTAG32_PREFETCH
		    __builtin_prefetch((const void*)&tag32_base[page_idx], 0, 0);
#endif
		    uint32_t tag = atomic_load_explicit(&tag32_base[page_idx], memory_order_relaxed);
		    if (__builtin_expect(tag == 0, 0)) {
		        return 0;
		    }
    if (tag_out) {
        *tag_out = tag;
    }
    return 1;
}

#if HZ3_PTAG32_NOINRANGE
// S28-5: hit-only lookup (no in_range store)
// Returns 1 if hit (tag != 0), 0 if miss.
// Miss case does NOT distinguish arena外 vs arena内・tag==0.
// Caller must use hz3_arena_page_index_fast() for that distinction.
HZ3_WARN_UNUSED_RESULT static inline int hz3_pagetag32_lookup_hit_fast(const void* ptr, uint32_t* tag_out) {
    // Acquire load base to synchronize with release store in do_init.
    // S114: when enabled, cache arena_base + tag32 base in TLS.
#if HZ3_PTAG_DSTBIN_TLS
    void* base = t_hz3_cache.arena_base;
    if (__builtin_expect(!base, 0)) {
        base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
        t_hz3_cache.arena_base = base;
    }
    _Atomic(uint32_t)* tag32_base = t_hz3_cache.page_tag32;
    if (__builtin_expect(!tag32_base, 0)) {
        tag32_base = g_hz3_page_tag32;
        t_hz3_cache.page_tag32 = tag32_base;
    }
#else
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    _Atomic(uint32_t)* tag32_base = g_hz3_page_tag32;
#endif
    if (__builtin_expect(!base, 0)) {
        return 0;
    }
    if (__builtin_expect(!tag32_base, 0)) {
        return 0;
    }
    uintptr_t delta = (uintptr_t)ptr - (uintptr_t)base;
#if HZ3_ARENA_SIZE == (1ULL << 32)
    if (__builtin_expect((delta >> 32) != 0, 0)) {
#else
    if (__builtin_expect(delta >= (uintptr_t)HZ3_ARENA_SIZE, 0)) {
#endif
        return 0;
    }
    uint32_t page_idx = (uint32_t)(delta >> HZ3_ARENA_PAGE_SHIFT);
#if HZ3_PTAG32_PREFETCH
    __builtin_prefetch((const void*)&tag32_base[page_idx], 0, 0);
#endif
    uint32_t tag = atomic_load_explicit(&tag32_base[page_idx], memory_order_relaxed);
    if (__builtin_expect(tag == 0, 0)) {
        return 0;
    }
    if (tag_out) {
        *tag_out = tag;
    }
    return 1;
}
#endif // HZ3_PTAG32_NOINRANGE
#endif // HZ3_PTAG_DSTBIN_FASTLOOKUP
#endif // HZ3_PTAG_DSTBIN_ENABLE
