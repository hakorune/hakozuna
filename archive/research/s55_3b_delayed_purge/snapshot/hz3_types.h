#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include "hz3_config.h"

// Segment constants (can be overridden via -D)
#ifndef HZ3_SEG_SIZE
#define HZ3_SEG_SIZE      (2u << 20)  // 2MB default
#endif
#ifndef HZ3_SEG_SHIFT
#define HZ3_SEG_SHIFT     21          // log2(HZ3_SEG_SIZE)
#endif
#define HZ3_PAGE_SIZE     4096
#define HZ3_PAGE_SHIFT    12
#define HZ3_PAGES_PER_SEG (HZ3_SEG_SIZE >> HZ3_PAGE_SHIFT)  // 512
#define HZ3_NUM_SC        8   // size classes: 4KB-32KB
#define HZ3_SMALL_MIN_SIZE 16
#define HZ3_SMALL_MAX_SIZE 2048
#define HZ3_SMALL_ALIGN    16
#define HZ3_SMALL_NUM_SC   (HZ3_SMALL_MAX_SIZE / HZ3_SMALL_ALIGN)
#define HZ3_SUB4K_MIN_SIZE (HZ3_SMALL_MAX_SIZE + 1u)
#define HZ3_SUB4K_MAX_SIZE (HZ3_PAGE_SIZE - 1u)
// Number of shards (owner id space).
//
// Notes:
// - This is NOT strictly "max threads". Threads may share a shard id when
//   threads > shards (see hz3_tcache shard assignment).
// - Increasing this increases TLS size (bank/outbox) and global tables.
// - Some legacy encodings require HZ3_NUM_SHARDS <= 63 (see hz3_arena.h asserts).
#ifndef HZ3_NUM_SHARDS
#define HZ3_NUM_SHARDS    8   // number of shards
#endif
#define HZ3_BITMAP_WORDS  (HZ3_PAGES_PER_SEG / 64)  // 512/64 = 8
#if HZ3_SUB4K_ENABLE
#define HZ3_SUB4K_BIN_BASE HZ3_SMALL_NUM_SC
#define HZ3_MEDIUM_BIN_BASE (HZ3_SMALL_NUM_SC + HZ3_SUB4K_NUM_SC)
#define HZ3_BIN_TOTAL     (HZ3_MEDIUM_BIN_BASE + HZ3_NUM_SC)
#else
#define HZ3_SUB4K_BIN_BASE HZ3_SMALL_NUM_SC
#define HZ3_MEDIUM_BIN_BASE HZ3_SMALL_NUM_SC
#define HZ3_BIN_TOTAL     (HZ3_SMALL_NUM_SC + HZ3_NUM_SC)
#endif

// S41: Sparse RemoteStash bin count validation (8-bit bin field safety)
#if HZ3_REMOTE_STASH_SPARSE
_Static_assert(HZ3_BIN_TOTAL <= 255, "requires HZ3_BIN_TOTAL <= 255 (8-bit bin)");
#endif

// Forward declaration
struct Hz3SegMeta;
struct Hz3SegHdr;

// Segment metadata (separate allocation, not in segment)
typedef struct Hz3SegMeta {
    uint8_t  owner;                        // shard id
    uint16_t sc_tag[HZ3_PAGES_PER_SEG];    // page_idx -> tag (0=none)
    // munmap tracking
    void*    reserve_base;
    size_t   reserve_size;
    // Day 3: run tracking
    void*    seg_base;                     // segment base address
    uint16_t free_pages;                   // number of free pages
    uint64_t free_bits[HZ3_BITMAP_WORDS];  // bitmap: 1=free, 0=used
    // S12-5A: arena slot index for hz3_arena_free()
    uint32_t arena_idx;
#if HZ3_S49_SEGMENT_PACKING
    // S49: Segment Packing fields
    uint8_t  pack_max_fit;    // max pages that can fit (decremented on failure)
    uint8_t  pack_bucket;     // current bucket index (0 = not in pool)
    uint8_t  pack_in_list;    // 1 if in pack pool, 0 otherwise
    struct Hz3SegMeta* pack_next;  // next in bucket list
#endif
} Hz3SegMeta;

// TLS bin (single-linked list)
// S38-2: count type is configurable (uint16_t or uint32_t)
#if HZ3_BIN_COUNT_U32
typedef uint32_t Hz3BinCount;
#else
typedef uint16_t Hz3BinCount;
#endif

typedef struct {
    void*       head;
    Hz3BinCount count;
} Hz3Bin;

// S38-2: Verify Hz3Bin size doesn't change (16B on 64-bit due to alignment)
_Static_assert(sizeof(Hz3Bin) == 16, "Hz3Bin size must be 16 bytes");

// S40: BinRef for SoA layout (thin struct with head* and count*)
// S40-2: Used when HZ3_TCACHE_SOA_LOCAL=1 or HZ3_TCACHE_SOA_BANK=1
#if HZ3_TCACHE_SOA_LOCAL || HZ3_TCACHE_SOA_BANK
typedef struct {
    void**       head;   // pointer to head slot
    Hz3BinCount* count;  // pointer to count slot
} Hz3BinRef;
#endif

// Hard cap constants (hot path uses only these, learning layer unused in v1)
#define HZ3_BIN_HARD_CAP    16
#define HZ3_DRAIN_TRIGGER   20

// S15-1: sc=0 (4KB) needs thicker bin for mixed workloads
// (2049-4095 requests round up to 4KB, increasing sc=0 demand)
#ifndef HZ3_BIN_CAP_SC0
#define HZ3_BIN_CAP_SC0     32
#endif


// Day 5: Refill batch size per size class (compile-time constant)
// Larger batches for smaller objects to amortize slow path overhead
// S26-1: sc=5,6,7 raised from 2 to 3 for 16KB–32KB supply efficiency
// S26-1B (batch=4) was NO-GO: regression on S25-D and dist=app
static const uint8_t HZ3_REFILL_BATCH[8] = {
    12, 8, 4, 4, 4, 3, 3, 3
};

// Day 4: Outbox for remote free
#define HZ3_OUTBOX_SIZE     32

typedef struct {
    void*   slots[HZ3_OUTBOX_SIZE];
    uint8_t count;
} Hz3OutboxBin;

// S41: Sparse RemoteStash (scale lane)
#if HZ3_REMOTE_STASH_SPARSE
typedef struct {
    uint8_t  dst;
    uint8_t  bin;
    uint8_t  _pad[6];
    void*    ptr;
} Hz3RemoteStashEntry;  // 16 bytes

_Static_assert(sizeof(Hz3RemoteStashEntry) == 16, "must be 16 bytes");

typedef struct {
    Hz3RemoteStashEntry ring[HZ3_REMOTE_STASH_RING_SIZE];
    uint16_t head;
    uint16_t tail;
} Hz3RemoteStashRing;
#endif

// Day 6: Knobs (TLS snapshot of global knobs)
typedef struct {
    uint8_t refill_batch[HZ3_NUM_SC];
    uint8_t bin_target[HZ3_NUM_SC];
    uint8_t outbox_flush_n;
} Hz3Knobs;

// Day 6: Stats for learning (updated only in slow/event path)
typedef struct {
    uint32_t refill_calls[HZ3_NUM_SC];
    uint32_t inbox_drained_objs[HZ3_NUM_SC];
    uint32_t central_pop_hit[HZ3_NUM_SC];
    uint32_t central_pop_miss[HZ3_NUM_SC];
    uint32_t segment_new_count;
    uint32_t bin_trim_excess[HZ3_NUM_SC];
#if HZ3_S12_V2_STATS
    // S12-3C triage: v2 detection counters (TLS, no atomics in hot path)
    uint64_t s12_v2_seg_from_ptr_calls;
    uint64_t s12_v2_seg_from_ptr_hit;
    uint64_t s12_v2_seg_from_ptr_miss;
    uint64_t s12_v2_small_v2_enter;
#endif
#if HZ3_V2_ONLY_STATS
    // S12-8: v2-only PTAG coverage (TLS, no atomics in hot path)
    uint64_t v2_only_tag_load;
    uint64_t v2_only_tag_zero;
    uint64_t v2_only_tag_nonzero;
    uint64_t v2_only_dispatch_v2;
    uint64_t v2_only_dispatch_v1;
#endif
} Hz3Stats;

// S40-2: Static asserts for padding (placed here near HZ3_BIN_PAD usage)
#if HZ3_BIN_PAD_LOG2
_Static_assert((HZ3_BIN_PAD & (HZ3_BIN_PAD - 1)) == 0, "HZ3_BIN_PAD must be power of 2");
_Static_assert(HZ3_BIN_TOTAL <= HZ3_BIN_PAD, "HZ3_BIN_TOTAL must fit in HZ3_BIN_PAD");
#endif

// Thread cache (TLS)
// S40-2: LOCAL and BANK can be independently configured
typedef struct {
    // === Local bins ===
#if HZ3_TCACHE_SOA_LOCAL
    // S40: SoA layout for local bins
    void*       local_head[HZ3_BIN_TOTAL];
    Hz3BinCount local_count[HZ3_BIN_TOTAL];
#else  // AoS for local bins
#if HZ3_LOCAL_BINS_SPLIT
    // S33: Unified local bins (PTAG bin index == array index, no range check)
    Hz3Bin  local_bins[HZ3_BIN_TOTAL];
#else
    Hz3Bin  bins[HZ3_NUM_SC];
    Hz3Bin  small_bins[HZ3_SMALL_NUM_SC];
#endif
#endif  // HZ3_TCACHE_SOA_LOCAL

    // === Bank bins (remote free) ===
#if HZ3_REMOTE_STASH_SPARSE
    // S41: Sparse ring (scale lane)
    Hz3RemoteStashRing remote_stash;
#else
	#if HZ3_PTAG_DSTBIN_ENABLE
	#if HZ3_TCACHE_SOA_BANK
	    // S40: SoA layout for bank bins (+ pow2 padding for imul elimination)
	    void*       bank_head[HZ3_NUM_SHARDS][HZ3_BIN_PAD];
	    Hz3BinCount bank_count[HZ3_NUM_SHARDS][HZ3_BIN_PAD];
	#else  // AoS for bank bins
	    Hz3Bin  bank[HZ3_NUM_SHARDS][HZ3_BIN_TOTAL];
	#endif  // HZ3_TCACHE_SOA_BANK
	#endif  // HZ3_PTAG_DSTBIN_ENABLE
	#endif  // HZ3_REMOTE_STASH_SPARSE

#if HZ3_PTAG_DSTBIN_ENABLE
    // S24: round-robin cursor for budgeted flush
    uint8_t  remote_dst_cursor;
    uint16_t remote_bin_cursor;
    uint8_t  remote_hint;           // "remote bin may exist" flag
#if HZ3_PTAG_DSTBIN_TLS
    void*   arena_base;            // TLS snapshot of arena base
    _Atomic(uint32_t)* page_tag32;  // TLS snapshot of PTAG32 base
#endif
#endif
    uint8_t my_shard;
    uint8_t initialized;
#if HZ3_S48_OWNER_STASH_SPILL
    // S48: TLS spill for owner stash leftovers (head only)
    // Used by hz3_owner_stash_pop_batch() to store excess objects
    void* stash_spill[HZ3_SMALL_NUM_SC];
#endif
    // Day 3: current segment for slow alloc
    struct Hz3SegMeta* current_seg;
    // Small v2: current segment for small pages
    struct Hz3SegHdr* small_current_seg;
    // Day 4: outbox[owner][sc] for remote free
    Hz3OutboxBin outbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];
    // Day 6: knobs snapshot (copied from g_knobs at epoch)
    Hz3Knobs knobs;
    uint32_t knobs_ver;
    // Day 6: epoch counter (event-only)
    uint32_t epoch_counter;
#if HZ3_ARENA_PRESSURE_BOX
    // S46: Global Pressure Box
    uint32_t last_pressure_epoch;  // last seen pressure epoch
    uint8_t  in_pressure_flush;    // re-entrancy guard
#endif
    // Day 6: stats for learning
    Hz3Stats stats;
} Hz3TCache;

// ============================================================================
// S55-3B: Delayed Purge Queue (TLS, SPSC)
// ============================================================================

// Entry: medium run waiting for delayed madvise
typedef struct {
    uintptr_t seg_base;    // segment base address (aligned to HZ3_SEG_SIZE)
    uint16_t  page_idx;    // page index within segment (0..511)
    uint8_t   pages;       // number of pages (1-8 for medium runs, sc 0-7)
    uint8_t   _pad;        // alignment padding
    uint32_t  stamp_epoch; // t_repay_epoch_counter when enqueued
} Hz3DelayedPurgeEntry;    // 16 bytes total

// Static assertion: ensure 16-byte size (cache-line friendly)
_Static_assert(sizeof(Hz3DelayedPurgeEntry) == 16, "Hz3DelayedPurgeEntry must be 16 bytes");

// Queue: TLS ring buffer (SPSC pattern, one per thread)
// - CRITICAL: Both enqueue and dequeue called from hz3_retention_repay_epoch() only
// - No cross-thread access: head/tail are thread-local (no atomics needed)
typedef struct {
    Hz3DelayedPurgeEntry ring[HZ3_S55_3B_QUEUE_SIZE];  // default 256, power-of-2
    // NOTE: Use 32-bit monotonically increasing counters to avoid wraparound
    // when processing is delayed (e.g. large DELAY_EPOCHS).
    uint32_t head;  // enqueue position (non-atomic, TLS SPSC)
    uint32_t tail;  // dequeue position (non-atomic, TLS SPSC)
} Hz3DelayedPurgeQueue;

// Static assertion: ensure QUEUE_SIZE is power-of-2 (bitwise AND optimization)
_Static_assert((HZ3_S55_3B_QUEUE_SIZE & (HZ3_S55_3B_QUEUE_SIZE - 1)) == 0,
               "HZ3_S55_3B_QUEUE_SIZE must be power-of-2");
