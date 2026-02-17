//
// Notes:
// - GO: xmalloc-test +25%, no SSOT regression (2026-01-13)
// - Requires S67-2 TLS spill layout (spill_array/spill_overflow).
// - Requires S99=0 (spill_array objects are not linked via next pointer).
// - Definition is at top of file (forward decl for S99 guard).

#ifndef HZ3_S112_STATS
#define HZ3_S112_STATS 0
#endif

#ifndef HZ3_S112_FAILFAST
#define HZ3_S112_FAILFAST 0
#endif

#if HZ3_S112_FULL_DRAIN_EXCHANGE
#if !HZ3_S44_OWNER_STASH
#error "HZ3_S112_FULL_DRAIN_EXCHANGE=1 requires HZ3_S44_OWNER_STASH=1"
#endif
#if !HZ3_REMOTE_STASH_SPARSE
#error "HZ3_S112_FULL_DRAIN_EXCHANGE=1 requires HZ3_REMOTE_STASH_SPARSE=1"
#endif
#if HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S112_FULL_DRAIN_EXCHANGE=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif
#if !HZ3_S67_SPILL_ARRAY2
#error "HZ3_S112_FULL_DRAIN_EXCHANGE=1 requires HZ3_S67_SPILL_ARRAY2=1"
#endif
#endif

// Safety: S67-2 must be paired with bounded drain OR S112 full drain.
// S112 provides an alternative drain path that doesn't need DRAIN_LIMIT.
#if HZ3_S67_SPILL_ARRAY2 && !HZ3_S67_DRAIN_LIMIT && !HZ3_S112_FULL_DRAIN_EXCHANGE
#error "HZ3_S67_SPILL_ARRAY2=1 requires HZ3_S67_DRAIN_LIMIT=1 or HZ3_S112_FULL_DRAIN_EXCHANGE=1"
#endif

// S67-2 uses smaller CAP for memory efficiency (override S67's 256)
#if HZ3_S67_SPILL_ARRAY2 && !defined(HZ3_S67_SPILL_CAP_OVERRIDE)
#undef HZ3_S67_SPILL_CAP
#define HZ3_S67_SPILL_CAP 64
#endif

// S67-2 replaces both S67 and S48 (mutual exclusion)
#if HZ3_S67_SPILL_ARRAY2
#undef HZ3_S67_SPILL_ARRAY
#define HZ3_S67_SPILL_ARRAY 0
#undef HZ3_S48_OWNER_STASH_SPILL
#define HZ3_S48_OWNER_STASH_SPILL 0
#endif

// S67-2 requires COUNT=0 (same as S48/S67)
#if HZ3_S67_SPILL_ARRAY2 && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S67_SPILL_ARRAY2=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// S67-2 must use bounded drain OR S112 full drain to avoid remainder/overflow paths
#if HZ3_S67_SPILL_ARRAY2 && !HZ3_S67_DRAIN_LIMIT && !HZ3_S112_FULL_DRAIN_EXCHANGE
#error "HZ3_S67_SPILL_ARRAY2=1 requires HZ3_S67_DRAIN_LIMIT=1 or HZ3_S112_FULL_DRAIN_EXCHANGE=1"
#endif

// ============================================================================
// S121: Page-Local Remote (Option A - page worklist instead of object stash)
// ============================================================================
//
// Motivation:
// - Current MPSC object stash concentrates contention at (owner, sc) point.
// - Page-local remote disperses contention across page count.
// - mimalloc uses similar page-local free list + lazy collection.
//
// Architecture:
// - Push: walk object list, prepend each to page->remote_head (MPSC).
//         If state 0→1 (first remote), enqueue page to pageq.
// - Pop:  pageq_pop → atomic_exchange(page->remote_head) → fill out[].
//         Leftover → TLS spill. Lost wakeup prevention via double-check.
//
// Expected improvement: +30-50% on xmalloc-testN (remote-heavy).
//
// Design doc: docs/PHASE_HZ3_S121_PAGE_LOCAL_REMOTE_DESIGN.md

#ifndef HZ3_HZ4_BRIDGE
#define HZ3_HZ4_BRIDGE 0  // research lane: hz4-style page-remote bridge (bench-compatible)
#endif
#ifndef HZ3_LANE_T16_R90_PAGE_REMOTE
#define HZ3_LANE_T16_R90_PAGE_REMOTE 0  // research lane: page-remote + bitmap hint
#endif
#if HZ3_HZ4_BRIDGE
#undef HZ3_LANE_T16_R90_PAGE_REMOTE
#define HZ3_LANE_T16_R90_PAGE_REMOTE 1
#endif
#ifndef HZ3_LANE16_FAILFAST
#define HZ3_LANE16_FAILFAST 0  // debug-only: lane16 list/page corruption guard
#endif
#ifndef HZ3_LANE16_REMOTE_LOCK
#define HZ3_LANE16_REMOTE_LOCK 0  // debug/triage: serialize lane16 page-remote heads
#endif
#ifndef HZ3_LANE16_DUP_GUARD
#define HZ3_LANE16_DUP_GUARD 0  // research: drop duplicate page-remote pushes (prevents cycles)
#endif
#ifndef HZ3_LANE16_DUP_FAILFAST
#define HZ3_LANE16_DUP_FAILFAST 0  // debug-only: per-page duplicate push detector
#endif
#ifndef HZ3_REMOTE_STASH_DUP_FAILFAST
#define HZ3_REMOTE_STASH_DUP_FAILFAST 0  // debug-only: detect duplicate ptrs in remote stash flush
#endif

// Lane16R90 requires RemoteStash (scale lane) + Small v2
#if HZ3_LANE_T16_R90_PAGE_REMOTE
#if !HZ3_REMOTE_STASH_SPARSE
#error "HZ3_LANE_T16_R90_PAGE_REMOTE=1 requires HZ3_REMOTE_STASH_SPARSE=1"
#endif
#if !HZ3_SMALL_V2_ENABLE
#error "HZ3_LANE_T16_R90_PAGE_REMOTE=1 requires HZ3_SMALL_V2_ENABLE=1"
#endif
#if HZ3_S121_PAGE_LOCAL_REMOTE
#error "HZ3_LANE_T16_R90_PAGE_REMOTE is incompatible with HZ3_S121_PAGE_LOCAL_REMOTE"
#endif
#endif

// Page header extension for page-remote (independent of S121)
#ifndef HZ3_PAGE_REMOTE_HEADER
#define HZ3_PAGE_REMOTE_HEADER 0
#endif
#if HZ3_LANE_T16_R90_PAGE_REMOTE || HZ3_S121_PAGE_LOCAL_REMOTE
#undef HZ3_PAGE_REMOTE_HEADER
#define HZ3_PAGE_REMOTE_HEADER 1
#endif

// Page-remote shard count (per-page remote_head array)
#ifndef HZ3_PAGE_REMOTE_SHARDS
#define HZ3_PAGE_REMOTE_SHARDS 2
#endif
#if HZ3_HZ4_BRIDGE && (HZ3_PAGE_REMOTE_SHARDS == 2)
#undef HZ3_PAGE_REMOTE_SHARDS
#define HZ3_PAGE_REMOTE_SHARDS 4
#endif
#if (HZ3_PAGE_REMOTE_SHARDS & (HZ3_PAGE_REMOTE_SHARDS - 1)) != 0
#error "HZ3_PAGE_REMOTE_SHARDS must be power of 2"
#endif

// Lane16R90: bucketize remote list by page (small fixed map)
#ifndef HZ3_T16_REMOTE_BUCKET
#define HZ3_T16_REMOTE_BUCKET 32
#endif
#if (HZ3_T16_REMOTE_BUCKET & (HZ3_T16_REMOTE_BUCKET - 1)) != 0
#error "HZ3_T16_REMOTE_BUCKET must be power of 2"
#endif

// Lane16R90: segment scan budget per pop (per call)
#ifndef HZ3_T16_COLLECT_SEG_BUDGET
#define HZ3_T16_COLLECT_SEG_BUDGET 8
#endif

// Lane16R90: max small segments tracked per thread
#ifndef HZ3_T16_SMALL_SEG_CAP
#define HZ3_T16_SMALL_SEG_CAP 4096
#endif
#if HZ3_T16_SMALL_SEG_CAP > 65535
#error "HZ3_T16_SMALL_SEG_CAP must fit in uint16_t"
#endif

#ifndef HZ3_S121_PAGE_LOCAL_REMOTE
#define HZ3_S121_PAGE_LOCAL_REMOTE 0  // default OFF for A/B testing
#endif

#if HZ3_S121_PAGE_LOCAL_REMOTE
#if !HZ3_S44_OWNER_STASH
#error "HZ3_S121_PAGE_LOCAL_REMOTE=1 requires HZ3_S44_OWNER_STASH=1"
#endif
#endif

#ifndef HZ3_S121_STATS
#define HZ3_S121_STATS 0
#endif

// S121-A: Lazy Enqueue threshold
// Only enqueue page to pageq when remote_count >= threshold.
// WARNING: threshold > 1 can strand objects if pages don't accumulate enough
// pushes before owner tries to pop. Use S121-A2 fallback to handle this.
#ifndef HZ3_S121_LAZY_THRESHOLD
#define HZ3_S121_LAZY_THRESHOLD 1  // 1 = immediate enqueue (no lazy batching)
#endif

// S121-A2: Force enqueue fallback when pageq is empty
// When pop finds pageq empty but needs objects, scan page_tag32 for pages
// with remote_head != NULL && remote_state == 0, and force-enqueue them.
// This resolves the "stranded objects" problem when threshold > 1.
#ifndef HZ3_S121_A2_FORCE_ENQUEUE
#define HZ3_S121_A2_FORCE_ENQUEUE 1  // enabled by default when S121 is on
#endif

// S121-A2: Maximum pages to scan per fallback attempt
// NOTE: This is total pages scanned (including non-matching), not just matching pages.
// Should be high enough to find stranded pages in a sparse arena.
#ifndef HZ3_S121_A2_SCAN_BUDGET
#define HZ3_S121_A2_SCAN_BUDGET 4096
#endif

// S121-D: Page Packet Notification (ARCHIVED / NO-GO)
// Moved to hz3_config_archive.h. See PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

// S121-E: Case 3 Cadence Collect (ARCHIVED / NO-GO)
// Moved to hz3_config_archive.h. See PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

// S121-F: COOLING state (ARCHIVED / NO-GO)
// Moved to hz3_config_archive.h. See PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md

// S121-G: Atomic packing of remote_head + remote_state
//
// Motivation:
// - S121-C push path has 2 atomic ops when old_head==NULL:
//   1. CAS on remote_head to prepend object
//   2. Exchange on remote_state to detect 0→1 transition
// - With objs_per_page=1.4, 84% of pushes trigger notification path.
// - Packing both into single atomic eliminates the extra exchange.
//
// Design (mimalloc xthread_free style):
// - Use lower 2 bits of pointer for state (pointers are 8-byte aligned)
// - remote_tagged = (head_ptr | state_bits)
// - States: 0=IDLE (notify on push), 1=ACTIVE (in pageq/processing)
//
// Push: Single CAS handles prepend + state transition + notification decision
// Pop: Exchange to drain, CAS to transition ACTIVE→IDLE
//
// Expected effect: Eliminate extra atomic_exchange on every empty→non-empty push

#ifndef HZ3_S121_G_ATOMIC_PACK
#define HZ3_S121_G_ATOMIC_PACK 0  // default OFF for A/B testing
#endif

// S121-G2: Fast path optimization for S121-G
//
// Motivation:
// - S121-G regressed -5% due to CAS loop computing state on every retry.
// - Most pushes (>80%) are to non-empty pages where state doesn't change.
// - Split into fast path (non-empty) and slow path (empty+state transition).
//
// Design:
// - Fast path: old_head != NULL → simple CAS without state computation
// - Slow path: old_head == NULL → check state, maybe transition 0→1
//
// Expected effect: Eliminate state computation overhead for majority of pushes

#ifndef HZ3_S121_G2_FAST_PATH
#define HZ3_S121_G2_FAST_PATH 0  // default OFF for A/B testing
#endif

// S121-G3: Branchless state computation for S121-G
//
// Motivation:
// - S121-G/G2 had branch misprediction overhead in CAS loop.
// - Replace conditional with branchless arithmetic.
//
// Design:
// - new_state = old_state | (is_empty & is_idle)
// - need_notify = is_empty & is_idle
// - No branches in hot path (except CAS retry)
//
// Expected effect: Avoid branch misprediction penalty

#ifndef HZ3_S121_G3_BRANCHLESS
#define HZ3_S121_G3_BRANCHLESS 0  // default OFF for A/B testing
#endif

#if HZ3_S121_G_ATOMIC_PACK
#if !HZ3_S121_PAGE_LOCAL_REMOTE
#error "HZ3_S121_G_ATOMIC_PACK=1 requires HZ3_S121_PAGE_LOCAL_REMOTE=1"
#endif
#endif

#if HZ3_S121_G2_FAST_PATH && !HZ3_S121_G_ATOMIC_PACK
#error "HZ3_S121_G2_FAST_PATH=1 requires HZ3_S121_G_ATOMIC_PACK=1"
#endif
