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
#define HZ3_NUM_SC        16  // size classes: 4KB-64KB
#define HZ3_SMALL_MIN_SIZE 16
// S138: SmallMaxSize Gate A/B - override via HZ3_SMALL_MAX_SIZE_OVERRIDE
#if HZ3_SMALL_MAX_SIZE_OVERRIDE != 0
#define HZ3_SMALL_MAX_SIZE HZ3_SMALL_MAX_SIZE_OVERRIDE
#else
#define HZ3_SMALL_MAX_SIZE 2048
#endif
#define HZ3_SMALL_ALIGN    16

// S122: Split Count requires 16-byte alignment
#if HZ3_BIN_SPLIT_COUNT
_Static_assert(HZ3_SMALL_ALIGN >= 16, "HZ3_BIN_SPLIT_COUNT requires 16-byte alignment");
// Split count constants
#define HZ3_SPLIT_CNT_SHIFT 4
#define HZ3_SPLIT_CNT_MASK  ((uintptr_t)((1u << HZ3_SPLIT_CNT_SHIFT) - 1u))
#define HZ3_SPLIT_PTR_MASK  (~HZ3_SPLIT_CNT_MASK)
#endif
#define HZ3_SMALL_NUM_SC   (HZ3_SMALL_MAX_SIZE / HZ3_SMALL_ALIGN)
#define HZ3_SUB4K_MIN_SIZE (HZ3_SMALL_MAX_SIZE + 1u)
#define HZ3_SUB4K_MAX_SIZE (HZ3_PAGE_SIZE - 1u)
// Number of shards (owner id space).
//
// Notes:
// - This is NOT strictly "max threads". Threads may share a shard id when
//   threads > shards (see hz3_tcache shard assignment).
// - Increasing this increases TLS size (bank/outbox) and global tables.
// - PTAG16 owner encoding requires HZ3_NUM_SHARDS <= 63 unless HZ3_PTAG32_ONLY=1.
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

// S67-2: spill_count is uint8_t, so CAP must fit
#if HZ3_S67_SPILL_ARRAY2 || HZ3_S67_SPILL_ARRAY
_Static_assert(HZ3_S67_SPILL_CAP <= 255, "HZ3_S67_SPILL_CAP must fit in uint8_t");
#endif

// Forward declaration
struct Hz3SegMeta;
struct Hz3SegHdr;
struct Hz3Lane;

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
#if HZ3_BIN_SPLIT_COUNT
    // S122: Split Count - head is tagged pointer (ptr | count_low)
    uintptr_t*   head;   // pointer to tagged head slot
#else
    void**       head;   // pointer to head slot
#endif
    Hz3BinCount* count;  // pointer to count slot (count_hi when SPLIT_COUNT)
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
// Larger batches for smaller objects to amortize slow path overhead.
// S188 opt-in can override sc=5..7 for medium-path A/B.
#if HZ3_S188_MEDIUM_REFILL_TUNE
#define HZ3_REFILL_SC5 HZ3_S188_REFILL_SC5
#define HZ3_REFILL_SC6 HZ3_S188_REFILL_SC6
#define HZ3_REFILL_SC7 HZ3_S188_REFILL_SC7
#else
#define HZ3_REFILL_SC5 3
#define HZ3_REFILL_SC6 3
#define HZ3_REFILL_SC7 3
#endif

// S-OOM: Extended to 16 classes (sc=8..15 for 36KB-64KB)
static const uint8_t HZ3_REFILL_BATCH[16] = {
    12, 8, 4, 4, 4, HZ3_REFILL_SC5, HZ3_REFILL_SC6, HZ3_REFILL_SC7,  // sc=0..7
    3,  3, 2, 2, 2, 2, 2, 2                                            // sc=8..15
};
#undef HZ3_REFILL_SC5
#undef HZ3_REFILL_SC6
#undef HZ3_REFILL_SC7

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
#if HZ3_RBUF_KEY
    uint16_t key;   // (dst<<8)|bin cached at push boundary
    uint8_t  _pad[4];
#else
    uint8_t  _pad[6];
#endif
    void*    ptr;
} Hz3RemoteStashEntry;  // 16 bytes

_Static_assert(sizeof(Hz3RemoteStashEntry) == 16, "must be 16 bytes");

typedef struct {
    Hz3RemoteStashEntry ring[HZ3_REMOTE_STASH_RING_SIZE];
    uint16_t head;
    uint16_t tail;
} Hz3RemoteStashRing;
#endif

// S60: PurgeRangeQueueBox (captures freed ranges for madvise)
#if HZ3_S60_PURGE_RANGE_QUEUE
typedef struct {
    void*    seg_base;    // segment base address
    uint16_t page_idx;    // first page to purge
    uint16_t pages;       // number of pages
} Hz3PurgeRangeEntry;     // 16 bytes

typedef struct {
    Hz3PurgeRangeEntry entries[HZ3_S60_QUEUE_SIZE];
    uint16_t head;
    uint16_t tail;
    uint32_t dropped;     // overflow counter (stats)
    uint32_t queued;      // total queued (stats)
} Hz3PurgeRangeQueue;
#endif


// Day 6: Knobs (TLS snapshot of global knobs)
typedef struct {
    uint8_t refill_batch[HZ3_NUM_SC];
    uint8_t bin_target[HZ3_NUM_SC];
    uint8_t outbox_flush_n;
    // S64: Epoch-based retire/purge knobs (Learning Layer tunable)
    uint16_t s64_retire_budget_objs;   // default: HZ3_S64_RETIRE_BUDGET_OBJS
    uint8_t  s64_purge_delay_epochs;   // default: HZ3_S64_PURGE_DELAY_EPOCHS
    uint8_t  s64_purge_budget_pages;   // default: HZ3_S64_PURGE_BUDGET_PAGES (capped at 255)
    // S64-1: TCacheDumpBox knobs
    uint16_t s64_tdump_budget_objs;    // default: HZ3_S64_TDUMP_BUDGET_OBJS
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
#if HZ3_BIN_SPLIT_COUNT
    // S122: Split Count - head stores (ptr | count_low), count stores count_hi
    uintptr_t   local_head[HZ3_BIN_TOTAL];
#else
    void*       local_head[HZ3_BIN_TOTAL];
#endif
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
    uint16_t remote_flush_credit;   // S173: sparse remote flush demand credit
#if HZ3_PTAG_DSTBIN_TLS
    void*   arena_base;            // TLS snapshot of arena base
    _Atomic(uint32_t)* page_tag32;  // TLS snapshot of PTAG32 base
#endif
#endif
    uint8_t my_shard;
#if HZ3_OWNER_STASH_INSTANCES > 1
    uint32_t owner_stash_seed;  // S144: unique per thread (for push hash)
    uint32_t owner_stash_rr;    // S144: round-robin cursor for pop [0..INSTANCES-1]
#endif
#if HZ3_LANE_SPLIT
    struct Hz3Lane* lane;
#endif
    uint8_t initialized;
#if HZ3_S67_SPILL_ARRAY2
    // S67-2: Two-layer spill (array + overflow list)
    // Array for O(1) batch pop, overflow list for excess
    void* spill_array[HZ3_SMALL_NUM_SC][HZ3_S67_SPILL_CAP];
    uint8_t spill_count[HZ3_SMALL_NUM_SC];
    void* spill_overflow[HZ3_SMALL_NUM_SC];  // linked list head only
#elif HZ3_S67_SPILL_ARRAY
    // S67: Array-based spill (NO-GO, kept for reference)
    void* spill_array[HZ3_SMALL_NUM_SC][HZ3_S67_SPILL_CAP];
    uint8_t spill_count[HZ3_SMALL_NUM_SC];
#elif HZ3_S48_OWNER_STASH_SPILL
    // S48: TLS spill for owner stash leftovers (head only)
    // Used by hz3_owner_stash_pop_batch() to store excess objects
    void* stash_spill[HZ3_SMALL_NUM_SC];
#endif
#if HZ3_S94_SPILL_LITE
    // S94: SpillLite array for low size classes only
    // Memory: SC_MAX * CAP * 8B = 32 * 32 * 8 = 8KB
    // S48 is still used for sc >= SC_MAX (linked-list fallback)
    void* s94_spill_array[HZ3_S94_SPILL_SC_MAX][HZ3_S94_SPILL_CAP];
    uint8_t s94_spill_count[HZ3_S94_SPILL_SC_MAX];
    void* s94_spill_overflow[HZ3_S94_SPILL_SC_MAX];  // linked list overflow head
#if HZ3_S94_SPILL_SC_MAX > HZ3_SMALL_NUM_SC
#error "HZ3_S94_SPILL_SC_MAX must be <= HZ3_SMALL_NUM_SC"
#endif
#endif
    // Day 3: current segment for slow alloc
    struct Hz3SegMeta* current_seg;
#if HZ3_LANE_SPLIT
    void* current_seg_base;
#endif
#if HZ3_S56_ACTIVE_SET
    // S56-2: Secondary active segment for medium carving (slow path only).
    // `current_seg` acts as the primary active segment.
    struct Hz3SegMeta* active_seg1;
#endif
    // Small v2: current segment for small pages
    struct Hz3SegHdr* small_current_seg;
#if HZ3_LANE_SPLIT
    void* small_current_seg_base;
#endif
#if HZ3_LANE_T16_R90_PAGE_REMOTE
    // Lane16R90: per-thread small segment list (arena slot ids)
    uint32_t t16_small_seg_ids[HZ3_T16_SMALL_SEG_CAP];
    uint16_t t16_small_seg_count;
    uint16_t t16_small_seg_cursor;
#endif
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
#if HZ3_S58_TCACHE_DECAY
    // S58-1: Medium bins decay tracking (HZ3_NUM_SC = 16)
    uint16_t s58_max_len[HZ3_NUM_SC];     // dynamic max length (shrinks on overage)
    uint8_t  s58_overage[HZ3_NUM_SC];     // overage count (3 triggers shrink)
    uint16_t s58_lowwater[HZ3_NUM_SC];    // min len observed in epoch interval
#endif
#if HZ3_S60_PURGE_RANGE_QUEUE
    // S60: PurgeRangeQueueBox (captures freed ranges for epoch madvise)
    Hz3PurgeRangeQueue purge_queue;
#endif
#if HZ3_S121_PAGE_LOCAL_REMOTE
    // S121-A2: Scan cursor for stranded page fallback (per size-class)
    uint32_t s121_scan_cursor[HZ3_SMALL_NUM_SC];
#if HZ3_S121_M_PAGEQ_BATCH
    // S121-M: Pageq push batching (pending pages for single owner/sc)
    void*   s121m_pending_pages[HZ3_S121_M_BATCH_SIZE];
    uint8_t s121m_pending_owner;
    uint8_t s121m_pending_sc;
    uint8_t s121m_pending_count;
#endif
#endif
#if HZ3_S145_CENTRAL_LOCAL_CACHE
    // S145-A: Per-sizeclass central pop cache (TLS-local)
    // Reduces atomic exchange contention by batching central pops.
    // Memory: 128 SC * 8B = 1KB (heads) + 128 SC * 2B = 256B (counts) = 1.25KB
    void*    s145_cache_head[HZ3_SMALL_NUM_SC];
    uint16_t s145_cache_count[HZ3_SMALL_NUM_SC];
#endif
} Hz3TCache;

// NOTE: S55-3B (delayed purge queue) is archived. Types removed from mainline.
