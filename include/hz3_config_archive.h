// hz3_config_archive.h - Archived / NO-GO flags
// Part of hz3_config.h split for maintainability
//
// These flags are permanently disabled with #error.
// Kept here for discoverability and to prevent accidental re-enabling.
#pragma once

// ============================================================================
// S28-2A: Bank row base cache (ARCHIVED / NO-GO)
// ============================================================================
// TLS caches &bank[my_shard][0] for faster bin access.
// ARCHIVED: code removed from mainline to reduce flag/branch sprawl.
// Reference: hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md

#ifndef HZ3_TCACHE_BANK_ROW_CACHE
#define HZ3_TCACHE_BANK_ROW_CACHE 0
#endif
#if HZ3_TCACHE_BANK_ROW_CACHE
#error "HZ3_TCACHE_BANK_ROW_CACHE is archived (NO-GO). See hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md"
#endif

// ============================================================================
// S39-2: hz3_free remote cold path (ARCHIVED / NO-GO)
// ============================================================================
// Attempt to mark remote free path as __attribute__((cold)).
// ARCHIVED: code removed from mainline to reduce flag/branch sprawl.
// Reference: hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md

#ifndef HZ3_FREE_REMOTE_COLDIFY
#define HZ3_FREE_REMOTE_COLDIFY 0
#endif
#if HZ3_FREE_REMOTE_COLDIFY
#error "HZ3_FREE_REMOTE_COLDIFY is archived (NO-GO). See hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md"
#endif

// ============================================================================
// S84: Sparse RemoteStash Batch Drain (NO-GO)
// ============================================================================
// Motivation: Batch consecutive (dst,bin) runs to reduce MPSC CAS pressure.
// NO-GO: Regression on r90_pf2 A/B (default OFF).
// Reference: hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md (S84 section)

#ifndef HZ3_S84_REMOTE_STASH_BATCH
#define HZ3_S84_REMOTE_STASH_BATCH 0
#endif
#if HZ3_S84_REMOTE_STASH_BATCH
#error "HZ3_S84_REMOTE_STASH_BATCH is NO-GO. See BUILD_FLAGS_INDEX.md"
#endif

#ifndef HZ3_S84_REMOTE_STASH_BATCH_MAX
#define HZ3_S84_REMOTE_STASH_BATCH_MAX 256
#endif

// ============================================================================
// S87: Small slow-path remote flush (NO-GO)
// ============================================================================
// Motivation: Drain remote stash at small_v2_alloc_slow() entry.
// NO-GO: Overhead exceeded benefit (S88 flush-on-empty preferred).
// Reference: hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md (S87 section)

#ifndef HZ3_S87_SMALL_SLOW_REMOTE_FLUSH
#define HZ3_S87_SMALL_SLOW_REMOTE_FLUSH 0
#endif
#if HZ3_S87_SMALL_SLOW_REMOTE_FLUSH
#error "HZ3_S87_SMALL_SLOW_REMOTE_FLUSH is NO-GO. Use S88 instead. See BUILD_FLAGS_INDEX.md"
#endif

// ============================================================================
// S93 / S93-2: Owner Stash PacketBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: store owner stash as fixed-size pointer arrays to reduce pop-side
// pointer chasing.
// NO-GO: large regression on r90_pf2 (packetize + packet head management costs
// dominate when push shape is effectively n==1).
// Reference: hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md

#ifndef HZ3_S93_OWNER_STASH_PACKET
#define HZ3_S93_OWNER_STASH_PACKET 0
#endif
#if HZ3_S93_OWNER_STASH_PACKET
#error "HZ3_S93_OWNER_STASH_PACKET is archived (NO-GO). See hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md"
#endif

#ifndef HZ3_S93_OWNER_STASH_PACKET_V2
#define HZ3_S93_OWNER_STASH_PACKET_V2 0
#endif
#if HZ3_S93_OWNER_STASH_PACKET_V2
#error "HZ3_S93_OWNER_STASH_PACKET_V2 is archived (NO-GO). See hakozuna/hz3/archive/research/s93_owner_stash_packet_box/README.md"
#endif

#ifndef HZ3_S93_PACKET_K
#define HZ3_S93_PACKET_K 32
#endif

#ifndef HZ3_S93_PACKET_MIN_N
#define HZ3_S93_PACKET_MIN_N 0
#endif

#ifndef HZ3_S93_PACKET_SLAB_BYTES
#define HZ3_S93_PACKET_SLAB_BYTES (64u << 10)
#endif

#ifndef HZ3_S93_STATS
#define HZ3_S93_STATS 0
#endif

#ifndef HZ3_S93_FAILFAST
#define HZ3_S93_FAILFAST 0
#endif

// ============================================================================
// S110-1: Free PTAG Bypass via SegHdr PageMeta (NO-GO)
// ============================================================================
// Motivation: Skip PTAG lookup in hz3_free by using segment header metadata.
// NO-GO: Slower than PTAG32 due to seg lookup overhead (scale lane).
// Reference: hakozuna/hz3/docs/PHASE_HZ3_S110_FREE_SEGHDR_PAGEMETA_FASTPATH_WORK_ORDER.md

#ifndef HZ3_S110_FREE_FAST_ENABLE
#define HZ3_S110_FREE_FAST_ENABLE 0
#endif
#if HZ3_S110_FREE_FAST_ENABLE
#error "HZ3_S110_FREE_FAST_ENABLE is NO-GO. PTAG32 is faster. See S110 work order."
#endif

// ----------------------------------------------------------------------------
// S110 Backward-compatible aliases (deprecated)
// ----------------------------------------------------------------------------
// Legacy flags mapped to new names for migration. Use HZ3_S110_META_ENABLE etc.
#if defined(HZ3_S110_SEGHDR_PAGEMETA_ENABLE) && !defined(HZ3_S110_META_ENABLE)
#define HZ3_S110_META_ENABLE HZ3_S110_SEGHDR_PAGEMETA_ENABLE
#endif
#if defined(HZ3_S110_SEGHDR_PAGEMETA_STATS) && !defined(HZ3_S110_STATS)
#define HZ3_S110_STATS HZ3_S110_SEGHDR_PAGEMETA_STATS
#endif
#if defined(HZ3_S110_SEGHDR_PAGEMETA_FAILFAST) && !defined(HZ3_S110_FAILFAST)
#define HZ3_S110_FAILFAST HZ3_S110_SEGHDR_PAGEMETA_FAILFAST
#endif

// ============================================================================
// S140: Free PTAG32 leaf micro-opts (NO-GO)
// ============================================================================
// Motivation: Reduce hz3_free PTAG32 leaf overhead via fused decode / branch inversion.
// NO-GO: Regression increases with remote ratio (r90 worst). Root causes:
//   - fused decode reduces ILP (sub -> extract dependency chain) and collapses IPC
//   - branch inversion increased branch-miss in remote-heavy runs
// Reference: hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md

#ifndef HZ3_S140_PTAG32_DECODE_FUSED
#define HZ3_S140_PTAG32_DECODE_FUSED 0
#endif
#if HZ3_S140_PTAG32_DECODE_FUSED
#error "HZ3_S140_PTAG32_DECODE_FUSED is NO-GO. See hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md"
#endif

#ifndef HZ3_S140_EXPECT_REMOTE
#define HZ3_S140_EXPECT_REMOTE 0
#endif
#if HZ3_S140_EXPECT_REMOTE
#error "HZ3_S140_EXPECT_REMOTE is NO-GO. See hakozuna/hz3/docs/PHASE_HZ3_S140_PTAG32_LEAF_MICROOPT_NO_GO.md"
#endif

// ============================================================================
// S121-D: Page Packet Notification (NO-GO)
// ============================================================================
// Motivation: Batch page enqueue to reduce pageq CAS frequency (1/K factor).
// NO-GO: Packet management overhead exceeded CAS reduction benefit (-82%).
// Reference: hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

#ifndef HZ3_S121_D_PAGE_PACKET
#define HZ3_S121_D_PAGE_PACKET 0
#endif
#if HZ3_S121_D_PAGE_PACKET
#error "HZ3_S121_D_PAGE_PACKET is archived (NO-GO). See hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md"
#endif

#ifndef HZ3_S121_D_PACKET_K
#define HZ3_S121_D_PACKET_K 4
#endif

#ifndef HZ3_S121_D_N_SLOTS
#define HZ3_S121_D_N_SLOTS 8
#endif

#ifndef HZ3_S121_D_STATS
#define HZ3_S121_D_STATS 0
#endif

// ============================================================================
// S121-E: Case 3 Cadence Collect (NO-GO)
// ============================================================================
// Motivation: Eliminate pageq notification via budget-limited page scan.
// NO-GO: O(n) scan overhead caused -40% regression.
// Reference: hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

#ifndef HZ3_S121_E_CADENCE_COLLECT
#define HZ3_S121_E_CADENCE_COLLECT 0
#endif
#if HZ3_S121_E_CADENCE_COLLECT
#error "HZ3_S121_E_CADENCE_COLLECT is archived (NO-GO). See hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md"
#endif

#ifndef HZ3_S121_E_SCAN_BUDGET
#define HZ3_S121_E_SCAN_BUDGET 16
#endif

#ifndef HZ3_S121_E_PAGELIST_INIT_CAP
#define HZ3_S121_E_PAGELIST_INIT_CAP 64
#endif

#ifndef HZ3_S121_E_STATS
#define HZ3_S121_E_STATS 0
#endif

// ============================================================================
// S121-F: COOLING state / Pageq Sub-Sharding (NO-GO)
// ============================================================================
// Motivation (COOLING): 1-cycle grace period after drain to reduce re-notification.
// Motivation (SHARD): 4-way sub-sharding to reduce CAS contention.
// NO-GO: Pop-side fixed cost increase caused -25% to -35% regression.
// Reference: hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

#ifndef HZ3_S121_F_COOLING_STATE
#define HZ3_S121_F_COOLING_STATE 0
#endif
#if HZ3_S121_F_COOLING_STATE
#error "HZ3_S121_F_COOLING_STATE is archived (NO-GO). See hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md"
#endif

#ifndef HZ3_S121_F_PAGEQ_SHARD
#define HZ3_S121_F_PAGEQ_SHARD 0
#endif
#if HZ3_S121_F_PAGEQ_SHARD
#error "HZ3_S121_F_PAGEQ_SHARD is archived (NO-GO). See hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md"
#endif

#ifndef HZ3_S121_F_NUM_SUBSHARDS
#define HZ3_S121_F_NUM_SUBSHARDS 4
#endif

#ifndef HZ3_S121_F_STATS
#define HZ3_S121_F_STATS 0
#endif

// ============================================================================
// S121-H: Budget-limited drain (NO-GO)
// ============================================================================
// Motivation: Reduce empty page creation rate via max_pages limit per pop_batch.
// NO-GO: Per-page CAS/pop overhead caused -30% regression.
// Reference: hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

#ifndef HZ3_S121_H_BUDGET_DRAIN
#define HZ3_S121_H_BUDGET_DRAIN 0
#endif
#if HZ3_S121_H_BUDGET_DRAIN
#error "HZ3_S121_H_BUDGET_DRAIN is archived (NO-GO). See hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md"
#endif

#ifndef HZ3_S121_H_MAX_PAGES
#define HZ3_S121_H_MAX_PAGES 8
#endif

// ============================================================================
// S128: RemoteStash Defer-MinRun (ARCHIVED / NO-GO)
// ============================================================================
// Was implemented inside hz3_remote_stash_flush_budget_impl() but archived to keep
// the hot boundary maintainable.
// Reference:
// - hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md

#ifndef HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN
#define HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN 0
#endif
#if HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN
#error "HZ3_S128_REMOTE_STASH_DEFER_MIN_RUN is archived (NO-GO). See hakozuna/hz3/archive/research/s128_remote_stash_defer_minrun/README.md"
#endif

#ifndef HZ3_S128_REMOTE_STASH_DEFER_MAX_AGE
#define HZ3_S128_REMOTE_STASH_DEFER_MAX_AGE 2
#endif

#ifndef HZ3_S128_REMOTE_STASH_DEFER_MAX_KEYS
#define HZ3_S128_REMOTE_STASH_DEFER_MAX_KEYS 128
#endif

#ifndef HZ3_S128_STATS
#define HZ3_S128_STATS 0
#endif

#ifndef HZ3_S128_FAILFAST
#define HZ3_S128_FAILFAST 0
#endif

// ============================================================================
// S97: Archived bucketize variants (3/4/5)
// ============================================================================
// S97 keeps only the supported variants in the mainline hot boundary.
// Archived variants are kept as reference snapshots under:
// - hakozuna/hz3/archive/research/s97_bucket_variants/

#if HZ3_S97_REMOTE_STASH_BUCKET == 3 || HZ3_S97_REMOTE_STASH_BUCKET == 4 || HZ3_S97_REMOTE_STASH_BUCKET == 5
#error "HZ3_S97_REMOTE_STASH_BUCKET=3/4/5 is archived (NO-GO). See hakozuna/hz3/archive/research/s97_bucket_variants/README.md"
#endif

// ============================================================================
// S97-6: OwnerStashPushMicroBatchBox (ARCHIVED / NO-GO)
// ============================================================================
// Attempted to micro-batch n==1 remote pushes via TLS staging, but observed:
// - Very low hit rate (avg_batch ~= 1.0), so overhead dominated.
// - Severe scaling regression in some thread/workload mixes (e.g., r50 T=16).
// Reference snapshot:
// - hakozuna/hz3/archive/research/s97_6_owner_stash_push_microbatch/README.md

#ifndef HZ3_S97_6_PUSH_MICROBATCH
#define HZ3_S97_6_PUSH_MICROBATCH 0
#endif
#if HZ3_S97_6_PUSH_MICROBATCH
#error "HZ3_S97_6_PUSH_MICROBATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s97_6_owner_stash_push_microbatch/README.md"
#endif

#ifndef HZ3_S97_6_MAX_BATCH
#define HZ3_S97_6_MAX_BATCH 4
#endif

#ifndef HZ3_S97_6_STATS
#define HZ3_S97_6_STATS 0
#endif
