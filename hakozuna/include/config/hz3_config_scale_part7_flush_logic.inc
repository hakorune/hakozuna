
#ifndef HZ3_S94_STATS
#define HZ3_S94_STATS 0  // atexit one-shot stats
#endif

// S95: S94 overflow O(1) path (avoid tail scan)
#ifndef HZ3_S95_S94_OVERFLOW_O1
#define HZ3_S95_S94_OVERFLOW_O1 0
#endif

#ifndef HZ3_S95_S94_FAILFAST
#define HZ3_S95_S94_FAILFAST 0
#endif

// ============================================================================
// S96: OwnerStashPush SSOT Box
// ============================================================================
#ifndef HZ3_S96_OWNER_STASH_PUSH_STATS
#define HZ3_S96_OWNER_STASH_PUSH_STATS 0
#endif

#ifndef HZ3_S96_OWNER_STASH_PUSH_SHOT
#define HZ3_S96_OWNER_STASH_PUSH_SHOT 0
#endif

#ifndef HZ3_S96_OWNER_STASH_PUSH_FAILFAST
#define HZ3_S96_OWNER_STASH_PUSH_FAILFAST 0
#endif

// ============================================================================
// S123: CountlessBinsBoundedOpsBox (event-side bounded walk for LAZY_COUNT)
// ============================================================================
// Default OFF (A/B only). Intended to be used with:
//   -DHZ3_BIN_COUNT_POLICY=2 (HZ3_BIN_LAZY_COUNT=1)
#ifndef HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY
#define HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY 0
#endif

#ifndef HZ3_S123_INBOX_DRAIN_FAILFAST
#define HZ3_S123_INBOX_DRAIN_FAILFAST 0
#endif

#ifndef HZ3_S123_TRIM_BOUNDED
#define HZ3_S123_TRIM_BOUNDED 0
#endif

#ifndef HZ3_S123_TRIM_BATCH
#define HZ3_S123_TRIM_BATCH 64
#endif

#ifndef HZ3_S123_S58_OVERAGE_BOUNDED
#define HZ3_S123_S58_OVERAGE_BOUNDED 0
#endif

// ============================================================================
// S136: HotSC-Only TCache Decay (event-only) (ARCHIVED / NO-GO)
// - Stubbed in trunk (kept for A/B compatibility)
// - Archive: hakozuna/hz3/archive/research/s137_small_decay_box/README.md
// ============================================================================
#ifndef HZ3_S136_HOTSC_ONLY
#define HZ3_S136_HOTSC_ONLY 0
#endif

#ifndef HZ3_S136_HOTSC_FIRST
#define HZ3_S136_HOTSC_FIRST 9
#endif

#ifndef HZ3_S136_HOTSC_LAST
#define HZ3_S136_HOTSC_LAST 13
#endif

// ============================================================================
// S137: SmallBinDecayBox (hot SC only, event-only) (ARCHIVED / NO-GO)
// - Stubbed in trunk (kept for A/B compatibility)
// - Archive: hakozuna/hz3/archive/research/s137_small_decay_box/README.md
// ============================================================================
#ifndef HZ3_S137_SMALL_DECAY
#define HZ3_S137_SMALL_DECAY 0
#endif

#ifndef HZ3_S137_SMALL_DECAY_FIRST
#define HZ3_S137_SMALL_DECAY_FIRST 9
#endif

#ifndef HZ3_S137_SMALL_DECAY_LAST
#define HZ3_S137_SMALL_DECAY_LAST 13
#endif

#ifndef HZ3_S137_SMALL_DECAY_TARGET
#define HZ3_S137_SMALL_DECAY_TARGET 0
#endif

#if HZ3_S137_SMALL_DECAY && !HZ3_SMALL_V2_ENABLE
#error "S137 requires HZ3_SMALL_V2_ENABLE=1"
#endif

// S121-F: Pageq Sub-Sharding (ARCHIVED / NO-GO)
// Moved to hz3_config_archive.h. See PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

// ============================================================================
// S97: RemoteStashFlush SSOT Box (sparse ring)
// ============================================================================
// Motivation:
// - `hz3_remote_stash_flush_budget_impl()` is a hot boundary in scale lane.
// - Measure dispatch shape (groups/distinct keys/category) and enable a small
//   A/B to reduce cold stores (tail->next=NULL) when debug list checks are OFF.
#ifndef HZ3_S97_REMOTE_STASH_FLUSH_STATS
#define HZ3_S97_REMOTE_STASH_FLUSH_STATS 0
#endif

// S196: RemoteDispatchPathStatsBox (observe-only, opt-in)
// Count dispatch volume by path (small/sub4k/medium) and medium per-sc shape.
#ifndef HZ3_S196_REMOTE_DISPATCH_STATS
#define HZ3_S196_REMOTE_DISPATCH_STATS 0
#endif

// S197: MediumInboxCASStatsBox (observe-only, opt-in)
// Record inbox push CAS retry shape and per-sc contention.
#ifndef HZ3_S197_MEDIUM_INBOX_CAS_STATS
#define HZ3_S197_MEDIUM_INBOX_CAS_STATS 0
#endif

// S198: InboxDrainHintGateBox (opt-in)
// Skip inbox drain call when all lane heads are empty (hint-based gate).
#ifndef HZ3_S198_INBOX_DRAIN_HINT_GATE
#define HZ3_S198_INBOX_DRAIN_HINT_GATE 0
#endif

// S200: InboxSeqGateBox (opt-in)
// Drain only when inbox sequence has advanced since the last check.
#ifndef HZ3_S200_INBOX_SEQ_GATE
#define HZ3_S200_INBOX_SEQ_GATE 0
#endif

// Strict mode bypasses S198 hint-gate on event drain path.
#ifndef HZ3_S200_INBOX_SEQ_STRICT
#define HZ3_S200_INBOX_SEQ_STRICT 0
#endif

// S201-lite: OwnerSideSpliceBatchBox (opt-in)
// Use list splice on owner-side inbox drain to reduce per-object push overhead.
#ifndef HZ3_S201_MEDIUM_INBOX_SPLICE
#define HZ3_S201_MEDIUM_INBOX_SPLICE 0
#endif

// S202: MediumRemoteDirectCentralBox (opt-in)
// Dispatch medium remote lists directly to owner's central bin (inbox bypass).
#ifndef HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL
#define HZ3_S202_MEDIUM_REMOTE_TO_CENTRAL 0
#endif

// Minimum list size to use S202 direct-central path.
// Keep n==1 on legacy inbox path to avoid over-shifting tiny dispatches.
#ifndef HZ3_S202_MEDIUM_TO_CENTRAL_MIN_N
#define HZ3_S202_MEDIUM_TO_CENTRAL_MIN_N 2
#endif

#if HZ3_S202_MEDIUM_TO_CENTRAL_MIN_N < 1
#error "HZ3_S202_MEDIUM_TO_CENTRAL_MIN_N must be >= 1"
#endif

// S199: MediumDispatchMicroBatchBox (opt-in)
// Batch medium remote dispatch by (dst,sc) to reduce inbox atomic push frequency.
#ifndef HZ3_S199_MEDIUM_DISPATCH_MICROBATCH
#define HZ3_S199_MEDIUM_DISPATCH_MICROBATCH 0
#endif

// Flush threshold by object count per (dst,sc) micro-batch.
#ifndef HZ3_S199_MEDIUM_OUTBOX_FLUSH_N
#define HZ3_S199_MEDIUM_OUTBOX_FLUSH_N 8
#endif
#if HZ3_S199_MEDIUM_OUTBOX_FLUSH_N < 1
#error "HZ3_S199_MEDIUM_OUTBOX_FLUSH_N must be >= 1"
#endif

// S205: OwnerSleepRescueBox (opt-in)
// Rescue medium remote lists to central when owner inbox backlog keeps growing
// without drain progress.
#ifndef HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE
#define HZ3_S205_MEDIUM_OWNER_SLEEP_RESCUE 0
#endif

// Approximate inbox backlog threshold (objects) to trigger rescue.
#ifndef HZ3_S205_BACKLOG_OBJS
#define HZ3_S205_BACKLOG_OBJS 256
#endif

// Number of pushed objects without drain progress before backlog check.
#ifndef HZ3_S205_STALE_PUSHES
#define HZ3_S205_STALE_PUSHES 1024
#endif

// Minimum dispatch list size to allow rescue path.
#ifndef HZ3_S205_MIN_N
#define HZ3_S205_MIN_N 1
#endif

#if HZ3_S205_BACKLOG_OBJS < 1
#error "HZ3_S205_BACKLOG_OBJS must be >= 1"
#endif

#if HZ3_S205_STALE_PUSHES < 1
#error "HZ3_S205_STALE_PUSHES must be >= 1"
#endif

#if HZ3_S205_MIN_N < 1
#error "HZ3_S205_MIN_N must be >= 1"
#endif

// S209: OwnerRescueStrongGateBox (opt-in)
// Route medium remote lists to central only when all of:
// - owner inbox backlog is high,
// - drain progress is stale for enough pushed objects,
// - alloc-side central miss sequence advanced for the same (shard,sc).
#ifndef HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG
#define HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG 0
#endif

#ifndef HZ3_S209_SC_MIN
#define HZ3_S209_SC_MIN 5
#endif

#ifndef HZ3_S209_SC_MAX
#define HZ3_S209_SC_MAX 7
#endif

#ifndef HZ3_S209_BACKLOG_OBJS
#define HZ3_S209_BACKLOG_OBJS 256
#endif

#ifndef HZ3_S209_STALE_PUSHES
#define HZ3_S209_STALE_PUSHES 2048
#endif

#ifndef HZ3_S209_MIN_N
#define HZ3_S209_MIN_N 1
#endif

#ifndef HZ3_S209_MISS_STRIDE
#define HZ3_S209_MISS_STRIDE 8
#endif

#if HZ3_S209_SC_MIN > HZ3_S209_SC_MAX
#error "HZ3_S209_SC_MIN must be <= HZ3_S209_SC_MAX"
#endif

#if HZ3_S209_BACKLOG_OBJS < 1
#error "HZ3_S209_BACKLOG_OBJS must be >= 1"
#endif

#if HZ3_S209_STALE_PUSHES < 1
#error "HZ3_S209_STALE_PUSHES must be >= 1"
#endif

#if HZ3_S209_MIN_N < 1
#error "HZ3_S209_MIN_N must be >= 1"
#endif

#if HZ3_S209_MISS_STRIDE < 1
#error "HZ3_S209_MISS_STRIDE must be >= 1"
#endif

// S210: OwnerRescueLiteBox (opt-in)
// Lighter rescue gate for medium remote path:
// - alloc-side miss sequence advanced for (dst,sc),
// - inbox backlog above threshold,
// - miss-seq stride rate cap.
#ifndef HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
#define HZ3_S210_MEDIUM_OWNER_RESCUE_LITE 1
#endif

#ifndef HZ3_S210_SC_MIN
#define HZ3_S210_SC_MIN 5
#endif

#ifndef HZ3_S210_SC_MAX
#define HZ3_S210_SC_MAX 7
#endif

#ifndef HZ3_S210_BACKLOG_OBJS
#define HZ3_S210_BACKLOG_OBJS 256
#endif

#ifndef HZ3_S210_MIN_N
#define HZ3_S210_MIN_N 1
#endif

#ifndef HZ3_S210_MISS_STRIDE
#define HZ3_S210_MISS_STRIDE 4
#endif

#if HZ3_S210_SC_MIN > HZ3_S210_SC_MAX
#error "HZ3_S210_SC_MIN must be <= HZ3_S210_SC_MAX"
#endif

#if HZ3_S210_BACKLOG_OBJS < 1
#error "HZ3_S210_BACKLOG_OBJS must be >= 1"
#endif

#if HZ3_S210_MIN_N < 1
#error "HZ3_S210_MIN_N must be >= 1"
#endif

#if HZ3_S210_MISS_STRIDE < 1
#error "HZ3_S210_MISS_STRIDE must be >= 1"
#endif

// S206: MediumStealOnMissBox (opt-in)
// On medium alloc slow-path inbox miss, try bounded steal from other shards'
// inboxes before central pop.
#ifndef HZ3_S206_STEAL_ON_MISS
#define HZ3_S206_STEAL_ON_MISS 0
#endif

// Max victim shards scanned per alloc_slow miss.
#ifndef HZ3_S206_STEAL_BUDGET_SHARDS
#define HZ3_S206_STEAL_BUDGET_SHARDS 2
#endif

// Minimum approximate inbox backlog (objects) to attempt steal.
#ifndef HZ3_S206_STEAL_BACKLOG_OBJS
#define HZ3_S206_STEAL_BACKLOG_OBJS 64
#endif

#if HZ3_S206_STEAL_BUDGET_SHARDS < 1
#error "HZ3_S206_STEAL_BUDGET_SHARDS must be >= 1"
#endif

#if HZ3_S206_STEAL_BACKLOG_OBJS < 1
#error "HZ3_S206_STEAL_BACKLOG_OBJS must be >= 1"
#endif

// S206B: MediumDstMixedBatchBox (archived / hard-off in hz3_config_archive.h)
// Producer side: batch medium remote lists by dst-only and push to mixed inbox.
// Consumer side: owner drains mixed inbox and classifies by ptr->sc.
#ifndef HZ3_S206B_MEDIUM_DST_MIXED_BATCH
#define HZ3_S206B_MEDIUM_DST_MIXED_BATCH 0
#endif

// Flush threshold (objects) for per-dst mixed outbox.
#ifndef HZ3_S206B_MIXED_OUTBOX_FLUSH_N
#define HZ3_S206B_MIXED_OUTBOX_FLUSH_N 16
#endif
