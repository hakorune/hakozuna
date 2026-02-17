
#if HZ3_S121_G3_BRANCHLESS && !HZ3_S121_G_ATOMIC_PACK
#error "HZ3_S121_G3_BRANCHLESS=1 requires HZ3_S121_G_ATOMIC_PACK=1"
#endif

#if HZ3_S121_G2_FAST_PATH && HZ3_S121_G3_BRANCHLESS
#error "HZ3_S121_G2_FAST_PATH and HZ3_S121_G3_BRANCHLESS are mutually exclusive"
#endif

// S121-L: State transition pre-calculation (push_one micro-opt)
//
// Motivation:
// - perf shows 53.66% of push_one time in state transition calculation
// - Current code recalculates is_empty & is_idle on every CAS retry
// - Most retries don't change the state (concurrent pushes to same page)
//
// Design:
// - Pre-calculate need_notify and new_state before CAS loop
// - Only recalculate when CAS fails AND state actually changed
// - Moves state computation out of hot CAS retry path
//
// Expected effect: +5-10% on R=90% (push_one heavy workload)
// ChatGPT Pro review: "超推し" - highest cost-effectiveness
//
// Requires HZ3_S121_G3_BRANCHLESS (builds on branchless approach)

#ifndef HZ3_S121_L_PRECALC
#define HZ3_S121_L_PRECALC 0  // default OFF for A/B testing
#endif

#if HZ3_S121_L_PRECALC && !HZ3_S121_G3_BRANCHLESS
#error "HZ3_S121_L_PRECALC=1 requires HZ3_S121_G3_BRANCHLESS=1"
#endif

// S121-M: Pageq push batching (micro-opt for push_one notification)
//
// Motivation:
// - Each IDLE→ACTIVE transition triggers hz3_pageq_push() with CAS
// - Consecutive pushes to same (owner, sc) contend on same atomic head
// - Batching reduces CAS count by up to BATCH_SIZE times
//
// Design:
// - TLS buffer holds pending pages for single (owner, sc)
// - On push to different (owner, sc): flush pending first
// - On buffer full: flush and push_list
// - tcache cleanup also flushes pending
//
// Expected effect: +5-15% on remote-heavy (R=90%) workloads
// Risk: Medium (delayed notification, requires flush on cleanup)

#ifndef HZ3_S121_M_PAGEQ_BATCH
#define HZ3_S121_M_PAGEQ_BATCH 0  // default OFF for A/B testing
#endif

#ifndef HZ3_S121_M_BATCH_SIZE
#define HZ3_S121_M_BATCH_SIZE 8  // pages per batch before flush
#endif

#if HZ3_S121_M_PAGEQ_BATCH && !HZ3_S121_PAGE_LOCAL_REMOTE
#error "HZ3_S121_M_PAGEQ_BATCH=1 requires HZ3_S121_PAGE_LOCAL_REMOTE=1"
#endif

// S121-J: Clear state on dequeue (before drain, not after)
//
// Motivation:
// - S121-C clears state after drain, requires double-check + requeue
// - S121-J clears state before drain, eliminates double-check
// - Push side guarantees pageq_push when old_head==NULL (CAS result)
//
// Expected effect:
// - Eliminate double-check (2 loads + 1 CAS)
// - Eliminate requeue path (g_s121_page_requeue → ~0)
// - Reduce instruction count by 7-12 per page drain
// - Conservative: +2-3% on xmalloc-testN, Optimistic: +5-8%
//
// Safety:
// - Conditional exchange (load→exchange) avoids RMW on empty pages
// - No lost wakeup: Push side detects old_head==NULL via CAS result
// - Memory ordering: separate atomics don't sync, but correctness
//   depends on Push notification condition, not ordering guarantee

#ifndef HZ3_S121_J_CLEAR_STATE_ON_DEQ
#define HZ3_S121_J_CLEAR_STATE_ON_DEQ 0  // default OFF for A/B
#endif

#if HZ3_S121_J_CLEAR_STATE_ON_DEQ && !HZ3_S121_PAGE_LOCAL_REMOTE
#error "HZ3_S121_J requires HZ3_S121_PAGE_LOCAL_REMOTE=1"
#endif

// S121-J は他の状態機械と排他
#if HZ3_S121_J_CLEAR_STATE_ON_DEQ && HZ3_S121_F_COOLING_STATE
#error "HZ3_S121_J is incompatible with HZ3_S121_F_COOLING_STATE (archived)"
#endif

#if HZ3_S121_J_CLEAR_STATE_ON_DEQ && HZ3_S121_E_CADENCE_COLLECT
#error "HZ3_S121_J is incompatible with HZ3_S121_E_CADENCE_COLLECT (archived)"
#endif

// S121-H: Budget-limited drain (ARCHIVED / NO-GO)
// Moved to hz3_config_archive.h. See PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

// ============================================================================
// S74: Lane Batch Refill/Flush (slow-path batching to reduce lease contention)
// ============================================================================
//
// Rationale:
// - OwnerLease acquisition cost dominates mstress under shard collisions
// - Batch refill/flush on slow path reduces core-entry frequency

#ifndef HZ3_S74_LANE_BATCH
#define HZ3_S74_LANE_BATCH 0
#endif

#ifndef HZ3_S74_REFILL_BURST
#define HZ3_S74_REFILL_BURST 8
#endif

// ============================================================================
// S203: Medium Remote Counters (A/B measurement)
// ============================================================================
#ifndef HZ3_S203_COUNTERS
#define HZ3_S203_COUNTERS 0
#endif

#ifndef HZ3_S74_FLUSH_BATCH
#define HZ3_S74_FLUSH_BATCH 64
#endif

#ifndef HZ3_S74_STATS
#define HZ3_S74_STATS 0
#endif

// ============================================================================
// S94: SpillLiteBox (consumer-side array for low size classes only)
// ============================================================================
//
// Rationale:
// - S67 full array causes r50 regression due to TLS size (262KB/thread)
// - S94 applies array only to sc < SC_MAX (e.g., 32 classes)
// - Memory: 32 sc * 32 cap * 8B = 8KB (vs S67's 262KB)
// - Hot sc in r90/r50 tends to be low sc, so this should cover main cost
// - S48 is used for sc >= SC_MAX (fallback to linked-list spill)

#ifndef HZ3_S94_SPILL_LITE
#define HZ3_S94_SPILL_LITE 0  // default OFF for A/B
#endif

#ifndef HZ3_S94_SPILL_SC_MAX
#define HZ3_S94_SPILL_SC_MAX 32  // array only for sc < 32
#endif

#ifndef HZ3_S94_SPILL_CAP
#define HZ3_S94_SPILL_CAP 32  // capacity per sc
#endif
