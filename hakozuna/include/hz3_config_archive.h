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
// S170-A: RemoteFlushUnrollBox (hz4 -> hz3 reverse import, NO-GO)
// ============================================================================
// Motivation: apply 4-way unroll in sparse remote stash flush loops to reduce
// loop/branch fixed cost.
// NO-GO: ABBA long + perf-stat paired recheck did not show stable benefit.
// Archived as hard-off to avoid accidental enable on scale lane.
// Reference: hakozuna/hz3/archive/research/s170_remote_flush_unroll/README.md

#ifndef HZ3_REMOTE_FLUSH_UNROLL
#define HZ3_REMOTE_FLUSH_UNROLL 0
#endif
#if HZ3_REMOTE_FLUSH_UNROLL
#error "HZ3_REMOTE_FLUSH_UNROLL is archived (NO-GO). See hakozuna/hz3/archive/research/s170_remote_flush_unroll/README.md"
#endif

// ============================================================================
// S170-B: RbufKeyBox (hz4 -> hz3 reverse import, NO-GO)
// ============================================================================
// Motivation: store (dst<<8)|bin in sparse stash entry to reduce flush-side
// key recomputation/comparison cost.
// NO-GO: R=50 regression and unstable behavior in final perf-stat recheck.
// Archived as hard-off to avoid accidental enable on scale lane.
// Reference: hakozuna/hz3/archive/research/s170_rbuf_key/README.md

#ifndef HZ3_RBUF_KEY
#define HZ3_RBUF_KEY 0
#endif
#if HZ3_RBUF_KEY
#error "HZ3_RBUF_KEY is archived (NO-GO). See hakozuna/hz3/archive/research/s170_rbuf_key/README.md"
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

// ============================================================================
// S16x: OwnerStash MicroOpt Series (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: Micro-optimizations for hz3_owner_stash_pop_batch().
// NO-GO: All variants showed regression or no benefit.
// Reference: hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md

// S166: CAS fast path for remainder push-back
#ifndef HZ3_S166_S112_REMAINDER_FASTPOP
#define HZ3_S166_S112_REMAINDER_FASTPOP 0
#endif
#if HZ3_S166_S112_REMAINDER_FASTPOP
#error "HZ3_S166_S112_REMAINDER_FASTPOP is archived (NO-GO). See hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md"
#endif

// S167: want==32 loop unrolling
#ifndef HZ3_S167_WANT32_UNROLL
#define HZ3_S167_WANT32_UNROLL 0
#endif
#if HZ3_S167_WANT32_UNROLL
#error "HZ3_S167_WANT32_UNROLL is archived (NO-GO). See hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md"
#endif

#ifndef HZ3_S167_WANT32
#define HZ3_S167_WANT32 32
#endif

// S168: Small remainder (1-2 elements) tail scan skip
#ifndef HZ3_S168_S112_REMAINDER_SMALL_FASTPATH
#define HZ3_S168_S112_REMAINDER_SMALL_FASTPATH 0
#endif
#if HZ3_S168_S112_REMAINDER_SMALL_FASTPATH
#error "HZ3_S168_S112_REMAINDER_SMALL_FASTPATH is archived (NO-GO). See hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md"
#endif

// S171: Bounded drain prebuffer (depends on S169)
#ifndef HZ3_S171_S112_BOUNDED_PREBUF
#define HZ3_S171_S112_BOUNDED_PREBUF 0
#endif
#if HZ3_S171_S112_BOUNDED_PREBUF
#error "HZ3_S171_S112_BOUNDED_PREBUF is archived (NO-GO, S169 dependency). See hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md"
#endif

#ifndef HZ3_S171_PREBUF_MAX
#define HZ3_S171_PREBUF_MAX 1024
#endif

// S172: Bounded drain speculative prefill (depends on S169)
#ifndef HZ3_S172_S112_BOUNDED_PREFILL
#define HZ3_S172_S112_BOUNDED_PREFILL 0
#endif
#if HZ3_S172_S112_BOUNDED_PREFILL
#error "HZ3_S172_S112_BOUNDED_PREFILL is archived (NO-GO, S169 dependency). See hakozuna/hz3/archive/research/s16x_owner_stash_microopt/README.md"
#endif

// ============================================================================
// S183: LargeCacheClassLockSplitBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: Split large cache lock by size-class to reduce global lock
// contention on free/alloc cache paths.
// NO-GO: T4 long improves slightly, but T8 and mixed/dist regress.
// Reference: hakozuna/hz3/archive/research/s183_large_cache_class_lock_split/README.md

#ifndef HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#define HZ3_S183_LARGE_CLASS_LOCK_SPLIT 0
#endif
#if HZ3_S183_LARGE_CLASS_LOCK_SPLIT
#error "HZ3_S183_LARGE_CLASS_LOCK_SPLIT is archived (NO-GO). See hakozuna/hz3/archive/research/s183_large_cache_class_lock_split/README.md"
#endif

// ============================================================================
// S186: LargeUnmapDeferralQueueBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: defer munmap on large-cache eviction path and drain by budget
// at free boundary to reduce eviction hot path fixed cost.
// NO-GO: larson long T4 regressed significantly (mean -3.4%, median -5.0%).
// Reference: hakozuna/hz3/archive/research/s186_large_unmap_deferral_queue/README.md

#ifndef HZ3_S186_LARGE_UNMAP_DEFER
#define HZ3_S186_LARGE_UNMAP_DEFER 0
#endif
#if HZ3_S186_LARGE_UNMAP_DEFER
#error "HZ3_S186_LARGE_UNMAP_DEFER is archived (NO-GO). See hakozuna/hz3/archive/research/s186_large_unmap_deferral_queue/README.md"
#endif

// ============================================================================
// S189: MediumTransferCacheBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: Add per-(shard,sc) medium transfer cache (sc5..7) to reduce
// central miss cost in medium slow path.
// NO-GO: RUNS=21 showed clear regressions on main/guard lanes and Larson.
// Reference: hakozuna/hz3/archive/research/s189_medium_transfer_cache/README.md

#ifndef HZ3_S189_MEDIUM_TRANSFERCACHE
#define HZ3_S189_MEDIUM_TRANSFERCACHE 0
#endif
#if HZ3_S189_MEDIUM_TRANSFERCACHE
#error "HZ3_S189_MEDIUM_TRANSFERCACHE is archived (NO-GO). See hakozuna/hz3/archive/research/s189_medium_transfer_cache/README.md"
#endif

// ============================================================================
// S195: MediumInboxShardBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: split medium inbox into sub-shards to reduce MPSC CAS contention
// on remote-heavy medium free path.
// NO-GO: shards=2/4 failed guard lane clearly; shards=8 improved main lane but
// guard variance stayed near/over adoption gate for single default.
// Reference: hakozuna/hz3/archive/research/s195_medium_inbox_shard/README.md

#if HZ3_S195_MEDIUM_INBOX_SHARDS != 1
#error "HZ3_S195_MEDIUM_INBOX_SHARDS is archived (NO-GO). Keep legacy value 1. See hakozuna/hz3/archive/research/s195_medium_inbox_shard/README.md"
#endif

#if HZ3_S195_MEDIUM_INBOX_STATS
#error "HZ3_S195_MEDIUM_INBOX_STATS is archived with S195. See hakozuna/hz3/archive/research/s195_medium_inbox_shard/README.md"
#endif

// ============================================================================
// S200: InboxSeqGateBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: replace hint-based inbox drain with event-sequence gate
// ("drain only when push sequence advanced").
// NO-GO: RUNS=21 interleaved showed guard regression beyond adoption gate
// (16..2048 r90: -5.104%), and perf spot-check increased cycles/page-faults.
// Reference: hakozuna/hz3/archive/research/s200_inbox_seq_gate/README.md

#ifndef HZ3_S200_INBOX_SEQ_GATE
#define HZ3_S200_INBOX_SEQ_GATE 0
#endif
#if HZ3_S200_INBOX_SEQ_GATE
#error "HZ3_S200_INBOX_SEQ_GATE is archived (NO-GO). See hakozuna/hz3/archive/research/s200_inbox_seq_gate/README.md"
#endif

#ifndef HZ3_S200_INBOX_SEQ_STRICT
#define HZ3_S200_INBOX_SEQ_STRICT 0
#endif
#if HZ3_S200_INBOX_SEQ_STRICT
#error "HZ3_S200_INBOX_SEQ_STRICT is archived with S200. See hakozuna/hz3/archive/research/s200_inbox_seq_gate/README.md"
#endif

// ============================================================================
// S201: OwnerSideSpliceBatchBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce owner-side medium inbox drain fixed cost by replacing
// per-object push with list splice.
// NO-GO: RUNS=21 interleaved failed gate (main +1.147%, guard -1.578%).
// Reference: hakozuna/hz3/archive/research/s201_owner_side_splice_batch/README.md

#ifndef HZ3_S201_MEDIUM_INBOX_SPLICE
#define HZ3_S201_MEDIUM_INBOX_SPLICE 0
#endif
#if HZ3_S201_MEDIUM_INBOX_SPLICE
#error "HZ3_S201_MEDIUM_INBOX_SPLICE is archived (NO-GO). See hakozuna/hz3/archive/research/s201_owner_side_splice_batch/README.md"
#endif

// ============================================================================
// S202: MediumRemoteDirectCentralBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: bypass medium inbox path by dispatching remote medium list
// directly to owner's central bin.
// NO-GO: RUNS=21 interleaved failed all lanes (main/guard/larson negative).
// Reference: hakozuna/hz3/archive/research/s202_medium_remote_direct_central/README.md

#ifndef HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL
#define HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL 0
#endif
#if HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL
#error "HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL is archived (NO-GO). See hakozuna/hz3/archive/research/s202_medium_remote_direct_central/README.md"
#endif

// ============================================================================
// S205: OwnerSleepRescueBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: detect medium inbox backlog with no owner drain progress and
// reroute remote medium list to central.
// NO-GO: RUNS=21 main lane regressed clearly (16..32768 r90: -2.344%).
// Reference: hakozuna/hz3/archive/research/s205_owner_sleep_rescue/README.md

#ifndef HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
#define HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE 0
#endif
#if HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
#error "HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE is archived (NO-GO). See hakozuna/hz3/archive/research/s205_owner_sleep_rescue/README.md"
#endif

// ============================================================================
// S206B: MediumDstMixedBatchBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce remote medium inbox atomic frequency by batching per-dst
// into mixed queues and decoding sc on owner drain.
// NO-GO: main lane throughput regressed heavily and RSS/page-faults increased.
// Reference: hakozuna/hz3/archive/research/s206b_medium_dst_mixed_batch/README.md

#ifndef HZ3_S206B_MEDIUM_DST_MIXED_BATCH
#define HZ3_S206B_MEDIUM_DST_MIXED_BATCH 0
#endif
#if HZ3_S206B_MEDIUM_DST_MIXED_BATCH
#error "HZ3_S206B_MEDIUM_DST_MIXED_BATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s206b_medium_dst_mixed_batch/README.md"
#endif

// ============================================================================
// S208: MediumCentralFloorReserveBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: on medium central-miss boundary, reserve extra segment fallback
// runs into central up to a per-(shard,sc) floor to reduce from_segment share.
// NO-GO: repeated RUNS=21 replays showed unstable sign and guard sensitivity.
// Recent deepcheck reproduced guard gate violation with identical SO hashes.
// Reference: hakozuna/hz3/archive/research/s208_medium_central_floor_reserve/README.md

#ifndef HZ3_S208_MEDIUM_CENTRAL_RESERVE
#define HZ3_S208_MEDIUM_CENTRAL_RESERVE 0
#endif
#if HZ3_S208_MEDIUM_CENTRAL_RESERVE
#error "HZ3_S208_MEDIUM_CENTRAL_RESERVE is archived (NO-GO). See hakozuna/hz3/archive/research/s208_medium_central_floor_reserve/README.md"
#endif

// ============================================================================
// S211: MediumSegmentReserveLiteBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce medium from_segment fallback with lower fixed cost than
// S208 by sparse reserve publish at central-miss boundary.
// NO-GO: after short-sweep retune, RUNS=21 main lane regressed and RSS rose
// sharply; no stable promotion point was found for single-default.
// Reference: hakozuna/hz3/archive/research/s211_medium_segment_reserve_lite/README.md

#ifndef HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
#define HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE 0
#endif
#if HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE
#error "HZ3_S211_MEDIUM_SEGMENT_RESERVE_LITE is archived (NO-GO). See hakozuna/hz3/archive/research/s211_medium_segment_reserve_lite/README.md"
#endif

// ============================================================================
// S220: CPU-local Remote Recycle Queue (RRQ) (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: bypass medium inbox dispatch by reusing remote-freed objects via
// per-CPU queue before inbox/central path.
// NO-GO: replay-stable regression on main lane and catastrophic larson
// regression (inbox batch recycle collapse + slowpath amplification).
// Reference: hakozuna/hz3/archive/research/s220_cpu_rrq/README.md

#ifndef HZ3_S220_CPU_RRQ
#define HZ3_S220_CPU_RRQ 0
#endif
#if HZ3_S220_CPU_RRQ
#error "HZ3_S220_CPU_RRQ is archived (NO-GO). See hakozuna/hz3/archive/research/s220_cpu_rrq/README.md"
#endif

// ============================================================================
// S221: MediumDispatchScopedMinibatchBox (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: batch remote medium dispatch for n==1 dominant streams while
// preserving inbox recycle path and guard behavior.
// NO-GO: main improved, but guard lane failed replay-stably in RUNS=10 screen
// and recheck (`-1.233%`, `-2.486%`); no safe promotion point found.
// Reference: hakozuna/hz3/archive/research/s221_medium_dispatch_minibatch/README.md

#ifndef HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
#define HZ3_S221_MEDIUM_DISPATCH_MINIBATCH 0
#endif
#if HZ3_S221_MEDIUM_DISPATCH_MINIBATCH
#error "HZ3_S221_MEDIUM_DISPATCH_MINIBATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s221_medium_dispatch_minibatch/README.md"
#endif

// ============================================================================
// S223: Medium alloc_slow Dynamic Want (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: increase medium refill "want" after central-miss streak to reduce
// repeated central-miss -> segment fallback churn on sc=5..7.
// NO-GO: RUNS=10 screen showed unstable negative sign on main/guard and failed
// guard gate on recheck (`guard < -1.0%`); no safe promotion point found.
// Reference: hakozuna/hz3/archive/research/s223_medium_dynamic_want/README.md

#ifndef HZ3_S223_DYNAMIC_WANT
#define HZ3_S223_DYNAMIC_WANT 0
#endif
#if HZ3_S223_DYNAMIC_WANT
#error "HZ3_S223_DYNAMIC_WANT is archived (NO-GO). See hakozuna/hz3/archive/research/s223_medium_dynamic_want/README.md"
#endif

// ============================================================================
// S224: Medium Remote local n==1 pair-batch (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce n==1-dominant medium remote publish fixed cost by keeping
// one pending object per (dst,sc) and publishing pair-lists when possible.
// NO-GO: RUNS=10 screen was negative on main and failed guard gate
// (`main -1.889%`, `guard -1.513%`) despite larson positive.
// Reference: hakozuna/hz3/archive/research/s224_medium_n1_pair_batch/README.md

#ifndef HZ3_S224_MEDIUM_N1_PAIR_BATCH
#define HZ3_S224_MEDIUM_N1_PAIR_BATCH 0
#endif
#if HZ3_S224_MEDIUM_N1_PAIR_BATCH
#error "HZ3_S224_MEDIUM_N1_PAIR_BATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s224_medium_n1_pair_batch/README.md"
#endif

// ============================================================================
// S225: Medium Remote n==1 last-key pair-batch (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce n==1 medium remote publish fixed cost with one TLS pending
// slot (last dst/sc) and immediate n==2 publish on key hit.
// NO-GO: RUNS=10 screen failed main gate (`main -1.014%`) even though
// guard/larson were positive, so no safe default promotion point was found.
// Reference: hakozuna/hz3/archive/research/s225_medium_n1_lastkey_pair_batch/README.md

#ifndef HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
#define HZ3_S225_MEDIUM_N1_LASTKEY_BATCH 0
#endif
#if HZ3_S225_MEDIUM_N1_LASTKEY_BATCH
#error "HZ3_S225_MEDIUM_N1_LASTKEY_BATCH is archived (NO-GO). See hakozuna/hz3/archive/research/s225_medium_n1_lastkey_pair_batch/README.md"
#endif

// ============================================================================
// S226: Medium flush-scope bucket3 (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce n==1-dominant medium remote publish fixed cost by
// grouping entries only inside one flush window by (dst,sc5..7).
// NO-GO: RUNS=10 screen was only marginally positive on main while guard
// failed gate (`main +0.466%`, `guard -2.428%`), so no safe default point.
// Reference: hakozuna/hz3/archive/research/s226_medium_flush_bucket3/README.md

#ifndef HZ3_S226_MEDIUM_FLUSH_BUCKET3
#define HZ3_S226_MEDIUM_FLUSH_BUCKET3 0
#endif
#if HZ3_S226_MEDIUM_FLUSH_BUCKET3
#error "HZ3_S226_MEDIUM_FLUSH_BUCKET3 is archived (NO-GO). See hakozuna/hz3/archive/research/s226_medium_flush_bucket3/README.md"
#endif

// ============================================================================
// S228: Central Fast local remainder cache (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce S222 fast-pop repush fixed cost by keeping remainder in
// TLS when shard is exclusive (`live_count==1`), then serving future pops from
// TLS first.
// NO-GO: mechanism worked (`pop_repush/pop_hits` dropped to ~0), but source mix
// worsened (`from_central`/`from_segment` up) and throughput regressed
// replay-stably on screen gate.
// Reference: hakozuna/hz3/archive/research/s228_central_fast_local_remainder/README.md

#ifndef HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
#define HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER 0
#endif
#if HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER
#error "HZ3_S228_CENTRAL_FAST_LOCAL_REMAINDER is archived (NO-GO). See hakozuna/hz3/archive/research/s228_central_fast_local_remainder/README.md"
#endif

// ============================================================================
// S229: Medium alloc_slow central-first (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: for medium hot classes (`sc=5..7`), try central-fast before
// inbox drain to reduce inbox-side fixed cost.
// NO-GO: path engaged but source mix shifted to central-heavy and slowpath
// amplification caused main-lane collapse replay-stably.
// Reference: hakozuna/hz3/archive/research/s229_central_first/README.md

#ifndef HZ3_S229_CENTRAL_FIRST
#define HZ3_S229_CENTRAL_FIRST 0
#endif
#if HZ3_S229_CENTRAL_FIRST
#error "HZ3_S229_CENTRAL_FIRST is archived (NO-GO). See hakozuna/hz3/archive/research/s229_central_first/README.md"
#endif

// ============================================================================
// S230: Medium n==1 -> central-fast direct (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: route medium n==1 remote dispatch directly to central-fast
// (S222) to reduce inbox publish fixed cost under n==1-dominant streams.
// NO-GO: mechanism engaged strongly (`to_central_s230` jumped), but source mix
// shifted to central-heavy + slowpath amplification and main throughput
// collapsed replay-stably on screen gate.
// Reference: hakozuna/hz3/archive/research/s230_medium_n1_to_central_fast/README.md

#ifndef HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
#define HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST 0
#endif
#if HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST
#error "HZ3_S230_MEDIUM_N1_TO_CENTRAL_FAST is archived (NO-GO). See hakozuna/hz3/archive/research/s230_medium_n1_to_central_fast/README.md"
#endif

// ============================================================================
// S231: Medium inbox fast MPSC (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce medium inbox fixed cost by forcing lane=0 push/drain and
// skipping push-seq updates on the same path (`sc=5..7`).
// NO-GO: mechanism did not change the dominant dispatch/source mix and failed
// replay gate on guard (RUNS=10), so no promotion point was found.
// Reference: hakozuna/hz3/archive/research/s231_medium_inbox_fast_mpsc/README.md

#ifndef HZ3_S231_INBOX_FAST_MPSC
#define HZ3_S231_INBOX_FAST_MPSC 0
#endif
#if HZ3_S231_INBOX_FAST_MPSC
#error "HZ3_S231_INBOX_FAST_MPSC is archived (NO-GO). See hakozuna/hz3/archive/research/s231_medium_inbox_fast_mpsc/README.md"
#endif

// ============================================================================
// S232: Large aggressive retain preset (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce hz3 large-path mmap/munmap churn on 64KB..128KB by
// forcing larger retain budget caps.
// NO-GO: large-only lane regressed strongly (`-22.73%`) and churn counters
// worsened (`alloc_cache_misses/free_munmap` increased), so promotion point
// was not found.
// Reference: hakozuna/hz3/archive/research/s232_large_aggressive/README.md

#ifndef HZ3_S232_LARGE_AGGRESSIVE
#define HZ3_S232_LARGE_AGGRESSIVE 0
#endif
#if HZ3_S232_LARGE_AGGRESSIVE
#error "HZ3_S232_LARGE_AGGRESSIVE is archived (NO-GO). See hakozuna/hz3/archive/research/s232_large_aggressive/README.md"
#endif

// ============================================================================
// S234: Central fast partial-pop CAS (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce S222 pop->repush fixed cost by partial CAS pop of only
// `want` nodes from fast head.
// NO-GO: mechanism engaged (`pop_repush=0`, `s234_hits` high) but replay-stable
// regressions on both main and guard lanes.
// Reference: hakozuna/hz3/archive/research/s234_central_fast_partial_pop/README.md

#ifndef HZ3_S234_CENTRAL_FAST_PARTIAL_POP
#define HZ3_S234_CENTRAL_FAST_PARTIAL_POP 0
#endif
#if HZ3_S234_CENTRAL_FAST_PARTIAL_POP
#error "HZ3_S234_CENTRAL_FAST_PARTIAL_POP is archived (NO-GO). See hakozuna/hz3/archive/research/s234_central_fast_partial_pop/README.md"
#endif

// ============================================================================
// S236-C: Pressure-aware BatchIt-lite (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: producer-side pressure-aware batching for medium n==1 stream.
// NO-GO: mechanism engaged but throughput screen gate was not met.
// Reference: hakozuna/hz3/archive/research/s236c_medium_batchit_lite/README.md

#ifndef HZ3_S236_BATCHIT_LITE
#define HZ3_S236_BATCHIT_LITE 0
#endif
#if HZ3_S236_BATCHIT_LITE
#error "HZ3_S236_BATCHIT_LITE is archived (NO-GO). See hakozuna/hz3/archive/research/s236c_medium_batchit_lite/README.md"
#endif

// ============================================================================
// S236-D: Medium mailbox multi-slot (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: reduce mailbox overflow fallback by increasing mailbox slots.
// NO-GO: fallback reduction was real but main throughput gain was too small.
// Reference: hakozuna/hz3/archive/research/s236d_medium_mailbox_multislot/README.md

#ifndef HZ3_S236_MAILBOX_SLOTS
#define HZ3_S236_MAILBOX_SLOTS 1
#endif
#if HZ3_S236_MAILBOX_SLOTS > 1
#error "HZ3_S236_MAILBOX_SLOTS>1 is archived (NO-GO). See hakozuna/hz3/archive/research/s236d_medium_mailbox_multislot/README.md"
#endif

// ============================================================================
// S236-E/F/G: mini central retry/hint series (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: improve mini-refill central capture with retry/hint gates.
// NO-GO: retries had no effective hits and hint gate had no throughput upside.
// Reference:
//   - hakozuna/hz3/archive/research/s236e_mini_central_retry/README.md
//   - hakozuna/hz3/archive/research/s236f_mini_central_retry_streak/README.md
//   - hakozuna/hz3/archive/research/s236g_mini_central_hint_gate/README.md

#ifndef HZ3_S236E_MINI_CENTRAL_RETRY
#define HZ3_S236E_MINI_CENTRAL_RETRY 0
#endif
#if HZ3_S236E_MINI_CENTRAL_RETRY
#error "HZ3_S236E_MINI_CENTRAL_RETRY is archived (NO-GO). See hakozuna/hz3/archive/research/s236e_mini_central_retry/README.md"
#endif

#ifndef HZ3_S236F_MINI_CENTRAL_RETRY
#define HZ3_S236F_MINI_CENTRAL_RETRY 0
#endif
#if HZ3_S236F_MINI_CENTRAL_RETRY
#error "HZ3_S236F_MINI_CENTRAL_RETRY is archived (NO-GO). See hakozuna/hz3/archive/research/s236f_mini_central_retry_streak/README.md"
#endif

#ifndef HZ3_S236G_MINI_CENTRAL_HINT_GATE
#define HZ3_S236G_MINI_CENTRAL_HINT_GATE 0
#endif
#if HZ3_S236G_MINI_CENTRAL_HINT_GATE
#error "HZ3_S236G_MINI_CENTRAL_HINT_GATE is archived (NO-GO). See hakozuna/hz3/archive/research/s236g_mini_central_hint_gate/README.md"
#endif

// ============================================================================
// S236-I: mini inbox second-chance (ARCHIVED / NO-GO)
// ============================================================================
// Motivation: add one extra inbox probe in mini-refill miss path.
// NO-GO: mechanism was almost never engaged; guard/larson gates failed.
// Reference: hakozuna/hz3/archive/research/s236i_mini_inbox_second_chance/README.md

#ifndef HZ3_S236I_MINI_INBOX_SECOND_CHANCE
#define HZ3_S236I_MINI_INBOX_SECOND_CHANCE 0
#endif
#if HZ3_S236I_MINI_INBOX_SECOND_CHANCE
#error "HZ3_S236I_MINI_INBOX_SECOND_CHANCE is archived (NO-GO). See hakozuna/hz3/archive/research/s236i_mini_inbox_second_chance/README.md"
#endif
