// hz4_config_archived.h - HZ4 Archived (NO-GO) Box Guards
// These boxes are archived and disabled by default.
// Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable them for research.
#ifndef HZ4_CONFIG_ARCHIVED_H
#define HZ4_CONFIG_ARCHIVED_H

#include "hz4_config_core.h"

// ============================================================================
// Archived (NO-GO) Boxes
// ============================================================================
#if !HZ4_ALLOW_ARCHIVED_BOXES

// P3.2: Inbox-only mode
#if HZ4_INBOX_ONLY
#error "HZ4_INBOX_ONLY is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Phase 18: SegAcquireGuardBox
#if HZ4_SEG_ACQ_GUARDBOX
#error "HZ4_SEG_ACQ_GUARDBOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Phase 19: SegAcquireGateBox
#if HZ4_SEG_ACQ_GATEBOX
#error "HZ4_SEG_ACQ_GATEBOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Phase 8: XferCache
#if HZ4_XFER_CACHE
#error "HZ4_XFER_CACHE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Phase 21: CPH No-Decommit Push
#if HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT
#error "HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Remote flush fastpath max
#if HZ4_REMOTE_FLUSH_FASTPATH_MAX != 4
#error "HZ4_REMOTE_FLUSH_FASTPATH_MAX must be 4 (other values archived). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B17: GuardRemoteFlushCompactBox (archived NO-GO)
#if HZ4_REMOTE_FLUSH_COMPACT_BOX
#error "HZ4_REMOTE_FLUSH_COMPACT_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_REMOTE_FLUSH_COMPACT_MAX) && (HZ4_REMOTE_FLUSH_COMPACT_MAX != 8)
#error "HZ4_REMOTE_FLUSH_COMPACT_MAX!=8 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B21-A: SmallActiveOnFirstAllocBox (archived NO-GO)
#if HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX
#error "HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B21-B: FreeRouteSmallFirstBox (archived NO-GO)
#if HZ4_FREE_ROUTE_SMALL_FIRST_BOX
#error "HZ4_FREE_ROUTE_SMALL_FIRST_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B23: LargeHeaderAlignFilterBox (archived NO-GO)
#if HZ4_LARGE_HEADER_ALIGN_FILTER_BOX
#error "HZ4_LARGE_HEADER_ALIGN_FILTER_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_HEADER_ALIGN_FILTER_MASK != 0xFFFu
#error "HZ4_LARGE_HEADER_ALIGN_FILTER_MASK!=0xFFF is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B24: FreeRouteLargeCandidateGuardBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX) && HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX
#error "HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK) && \
    (HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK != 0xFFFu)
#error "HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK!=0xFFF is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B36: FreeRouteLargeValidateFuseBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX) && HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX
#error "HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B38: LocalFreeMetaTcacheFirstBox (archived NO-GO)
#if defined(HZ4_LOCAL_FREE_META_TCACHE_FIRST) && HZ4_LOCAL_FREE_META_TCACHE_FIRST
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B39: MidObjCacheBox (archived NO-GO)
#if defined(HZ4_MID_OBJ_CACHE) && HZ4_MID_OBJ_CACHE
#error "HZ4_MID_OBJ_CACHE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_ST_MID_OBJ_CACHE) && HZ4_ST_MID_OBJ_CACHE
#error "HZ4_ST_MID_OBJ_CACHE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B46: MidDualTierAllocCacheBox (archived NO-GO)
#if defined(HZ4_MID_DUAL_TIER_ALLOC_BOX) && HZ4_MID_DUAL_TIER_ALLOC_BOX
#error "HZ4_MID_DUAL_TIER_ALLOC_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_DUAL_TIER_L0_LIMIT) && (HZ4_MID_DUAL_TIER_L0_LIMIT != 4)
#error "HZ4_MID_DUAL_TIER_L0_LIMIT!=4 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_DUAL_TIER_L0_REFILL_BATCH) && (HZ4_MID_DUAL_TIER_L0_REFILL_BATCH != 2)
#error "HZ4_MID_DUAL_TIER_L0_REFILL_BATCH!=2 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B50: FreeRouteSuperFastSmallLocalBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX) && HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX
#error "HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B51: FreeRouteTLSPageKindCacheBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX) && HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX
#error "HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS) && (HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS != 64)
#error "HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS!=64 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Phase 6: PageTagTable (archived NO-GO)
#if defined(HZ4_PAGE_TAG_TABLE) && HZ4_PAGE_TAG_TABLE
#error "HZ4_PAGE_TAG_TABLE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B60-lite: FreeRoutePagetagE2EBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_PAGETAG_E2E_BOX) && HZ4_FREE_ROUTE_PAGETAG_E2E_BOX
#error "HZ4_FREE_ROUTE_PAGETAG_E2E_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B52: FreeRouteSegmentMapBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_SEGMENT_MAP_BOX) && HZ4_FREE_ROUTE_SEGMENT_MAP_BOX
#error "HZ4_FREE_ROUTE_SEGMENT_MAP_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_FREE_ROUTE_SEGMENT_MAP_L1_BITS) && (HZ4_FREE_ROUTE_SEGMENT_MAP_L1_BITS != 14)
#error "HZ4_FREE_ROUTE_SEGMENT_MAP_L1_BITS!=14 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_FREE_ROUTE_SEGMENT_MAP_L2_BITS) && (HZ4_FREE_ROUTE_SEGMENT_MAP_L2_BITS != 14)
#error "HZ4_FREE_ROUTE_SEGMENT_MAP_L2_BITS!=14 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B53: FreeRoutePageKindMapBox (archived NO-GO)
#if defined(HZ4_FREE_ROUTE_PAGE_KIND_MAP_BOX) && HZ4_FREE_ROUTE_PAGE_KIND_MAP_BOX
#error "HZ4_FREE_ROUTE_PAGE_KIND_MAP_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B54: STMidFreeBatchAnyBox (archived NO-GO)
#if defined(HZ4_ST_MID_FREE_BATCH_ANY_BOX) && HZ4_ST_MID_FREE_BATCH_ANY_BOX
#error "HZ4_ST_MID_FREE_BATCH_ANY_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B55: STMidAllocRunBoostBox (archived NO-GO)
#if defined(HZ4_ST_MID_ALLOC_RUN_BOOST_BOX) && HZ4_ST_MID_ALLOC_RUN_BOOST_BOX
#error "HZ4_ST_MID_ALLOC_RUN_BOOST_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_ST_MID_ALLOC_RUN_CACHE_LIMIT) && (HZ4_ST_MID_ALLOC_RUN_CACHE_LIMIT != 16)
#error "HZ4_ST_MID_ALLOC_RUN_CACHE_LIMIT!=16 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B62: MidRemotePushListBox (archived NO-GO)
#if defined(HZ4_MID_REMOTE_PUSH_LIST_BOX) && HZ4_MID_REMOTE_PUSH_LIST_BOX
#error "HZ4_MID_REMOTE_PUSH_LIST_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_REMOTE_PUSH_LIST_MIN) && (HZ4_MID_REMOTE_PUSH_LIST_MIN != 2)
#error "HZ4_MID_REMOTE_PUSH_LIST_MIN!=2 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B71: MidSpanLiteBox (archived NO-GO)
#if defined(HZ4_MID_SPAN_LITE_BOX) && HZ4_MID_SPAN_LITE_BOX
#error "HZ4_MID_SPAN_LITE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B72: MidSpanLeafRemoteQueueBox (archived NO-GO)
#if defined(HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX) && HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX
#error "HZ4_MID_SPAN_LEAF_REMOTE_QUEUE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B73: MidTransferCacheBox (archived NO-GO)
#if defined(HZ4_MID_TRANSFER_CACHE_BOX) && HZ4_MID_TRANSFER_CACHE_BOX
#error "HZ4_MID_TRANSFER_CACHE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B74: MidLockPathOutlockRefillBox (archived NO-GO)
#if defined(HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX) && HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX
#error "HZ4_MID_LOCKPATH_OUTLOCK_REFILL_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B76: SmallAllocPageInitLiteBox (archived NO-GO)
#if defined(HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX) && HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX
#error "HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Stage5: RemoteFlushNoClearNextBox
#if HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT
#error "HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Stage5: MetaPrefaultBox
#if HZ4_META_PREFAULTBOX
#error "HZ4_META_PREFAULTBOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Stage5: SegCreateLazyInitPagesBox
#if HZ4_SEG_CREATE_LAZY_INIT_PAGES
#error "HZ4_SEG_CREATE_LAZY_INIT_PAGES is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Stage5: BumpFreeMetaAllowUnderGateBox
#if HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE
#error "HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S217: LargeCacheShardBox
#if HZ4_LARGE_CACHE_SHARDBOX
#error "HZ4_LARGE_CACHE_SHARDBOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S218-B5: budgeted rescue knobs (archived NO-GO)
#if HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL != 1
#error "HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL!=1 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF != 1
#error "HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF!=1 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S218-B7: rescue precision-gate knobs (archived NO-GO)
#if HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
#error "HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW != 8
#error "HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW!=8 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT != 25
#error "HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT!=25 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK != 8
#error "HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK!=8 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS != 4
#error "HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS!=4 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S220-B: Large owner-return routing (archived NO-GO)
#if HZ4_S220_LARGE_OWNER_RETURN
#error "HZ4_S220_LARGE_OWNER_RETURN is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES != 3
#error "HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES!=3 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// B14: LargeRemoteFreeBypassSpanCacheBox (archived NO-GO)
#if HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX
#error "HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES != 2
#error "HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES!=2 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S219-D1: pages<=2 lock-shard probe=0 trim (archived NO-GO)
#if HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2 == 0
#error "HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2=0 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S219 follow-up: LargeShardStealBudgetBox (archived NO-GO)
#if HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX
#error "HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// S219 follow-up: LargeShardNonEmptyMaskBox (archived NO-GO)
#if HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX
#error "HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidLockBurstRefillBox (archived NO-GO)
#if HZ4_MID_LOCK_BURST_REFILL_BOX
#error "HZ4_MID_LOCK_BURST_REFILL_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidFreeBatchLockBypassBox (archived NO-GO)
#if defined(HZ4_MID_FREE_BATCH_LOCK_BYPASS_BOX) && HZ4_MID_FREE_BATCH_LOCK_BYPASS_BOX
#error "HZ4_MID_FREE_BATCH_LOCK_BYPASS_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_FREE_BATCH_LOCK_BYPASS_MIN) && (HZ4_MID_FREE_BATCH_LOCK_BYPASS_MIN != 1)
#error "HZ4_MID_FREE_BATCH_LOCK_BYPASS_MIN!=1 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidActiveRunBox (archived NO-GO)
#if defined(HZ4_MID_ACTIVE_RUN) && HZ4_MID_ACTIVE_RUN
#error "HZ4_MID_ACTIVE_RUN is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_ACTIVE_RUN_BATCH) && HZ4_MID_ACTIVE_RUN_BATCH
#error "HZ4_MID_ACTIVE_RUN_BATCH is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_ACTIVE_RUN_BATCH_PAGES) && (HZ4_MID_ACTIVE_RUN_BATCH_PAGES != 4)
#error "HZ4_MID_ACTIVE_RUN_BATCH_PAGES!=4 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_ST_MID_ACTIVE_RUN) && HZ4_ST_MID_ACTIVE_RUN
#error "HZ4_ST_MID_ACTIVE_RUN is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_ST_MID_ACTIVE_RUN_BATCH) && HZ4_ST_MID_ACTIVE_RUN_BATCH
#error "HZ4_ST_MID_ACTIVE_RUN_BATCH is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_ST_MID_ACTIVE_RUN_BATCH_PAGES) && (HZ4_ST_MID_ACTIVE_RUN_BATCH_PAGES != 4)
#error "HZ4_ST_MID_ACTIVE_RUN_BATCH_PAGES!=4 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidOwnerClaimGateBox (archived NO-GO)
#if defined(HZ4_MID_OWNER_CLAIM_GATE_BOX) && HZ4_MID_OWNER_CLAIM_GATE_BOX
#error "HZ4_MID_OWNER_CLAIM_GATE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_OWNER_CLAIM_MIN_STREAK) && (HZ4_MID_OWNER_CLAIM_MIN_STREAK != 3)
#error "HZ4_MID_OWNER_CLAIM_MIN_STREAK!=3 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_OWNER_CLAIM_GATE_SC_MIN) && (HZ4_MID_OWNER_CLAIM_GATE_SC_MIN != 0)
#error "HZ4_MID_OWNER_CLAIM_GATE_SC_MIN!=0 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_OWNER_CLAIM_GATE_SC_MAX) && \
    (HZ4_MID_OWNER_CLAIM_GATE_SC_MAX != ((HZ4_PAGE_SIZE / HZ4_MID_ALIGN) - 1))
#error "HZ4_MID_OWNER_CLAIM_GATE_SC_MAX override is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidTLS tuning boxes (archived NO-GO)
#if defined(HZ4_MID_TLS_CACHE_DEPTH) && (HZ4_MID_TLS_CACHE_DEPTH != 1)
#error "HZ4_MID_TLS_CACHE_DEPTH!=1 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_TLS_CACHE_SC_MAX) && \
    (HZ4_MID_TLS_CACHE_SC_MAX != ((HZ4_PAGE_SIZE / HZ4_MID_ALIGN) - 1))
#error "HZ4_MID_TLS_CACHE_SC_MAX override is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH) && (HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH != 1)
#error "HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH!=1 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidPageCreateSuppressBox (archived NO-GO)
#if defined(HZ4_MID_PAGE_CREATE_SUPPRESS_BOX) && HZ4_MID_PAGE_CREATE_SUPPRESS_BOX
#error "HZ4_MID_PAGE_CREATE_SUPPRESS_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY) && (HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY != 1)
#error "HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY!=1 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidPageCreateOutsideSCLockBox (archived NO-GO)
#if defined(HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX) && HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
#error "HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidPageCreateOutlockLazyInitBox (archived NO-GO)
#if defined(HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_LAZY_INIT_BOX) && \
    HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_LAZY_INIT_BOX
#error "HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_LAZY_INIT_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: MidFreeBatchConsumeAnyOnMissBox / LocalGate (archived NO-GO)
#if defined(HZ4_MID_FREE_BATCH_CONSUME_ANY_ON_MISS_BOX) && \
    HZ4_MID_FREE_BATCH_CONSUME_ANY_ON_MISS_BOX
#error "HZ4_MID_FREE_BATCH_CONSUME_ANY_ON_MISS_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_BOX) && \
    HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_BOX
#error "HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_WINDOW) && \
    (HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_WINDOW != 256)
#error "HZ4_MID_FREE_BATCH_CONSUME_ANY_LOCAL_GATE_WINDOW!=256 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

// Mid follow-up: B63 LockpathFlushOnMissBox (archived NO-GO)
#if defined(HZ4_MID_LOCKPATH_FLUSH_ON_MISS_BOX) && HZ4_MID_LOCKPATH_FLUSH_ON_MISS_BOX
#error "HZ4_MID_LOCKPATH_FLUSH_ON_MISS_BOX is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if defined(HZ4_MID_LOCKPATH_FLUSH_ON_MISS_FIRST_CAP) && (HZ4_MID_LOCKPATH_FLUSH_ON_MISS_FIRST_CAP != 8)
#error "HZ4_MID_LOCKPATH_FLUSH_ON_MISS_FIRST_CAP!=8 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif

#endif // !HZ4_ALLOW_ARCHIVED_BOXES

#endif // HZ4_CONFIG_ARCHIVED_H
