// hz3_config_scale.h - Scale lane configuration (S42-S44)
// Transfer cache, owner stash, etc.
// Part of hz3_config.h split for maintainability
#pragma once

// Forward declaration for S112 (used by S99 guard below).
// S112 Full Drain Exchange is enabled by default for scale lane (SPARSE).
#ifndef HZ3_S112_FULL_DRAIN_EXCHANGE
  #if HZ3_REMOTE_STASH_SPARSE
    #define HZ3_S112_FULL_DRAIN_EXCHANGE 1
  #else
    #define HZ3_S112_FULL_DRAIN_EXCHANGE 0
  #endif
#endif

// Forward declaration for S67-2 (used by S93-2 guard below).
// S67-2 TLS spill layout is enabled by default when S112 is enabled.
#ifndef HZ3_S67_SPILL_ARRAY2
  #if HZ3_S112_FULL_DRAIN_EXCHANGE
    #define HZ3_S67_SPILL_ARRAY2 1
  #else
    #define HZ3_S67_SPILL_ARRAY2 0
  #endif
#endif

// ============================================================================
// S42: Small Transfer Cache (scale lane remote-heavy optimization)
// ============================================================================
//
// Rationale:
// - T=32/R=50/16-2048 で small central (mutex+list) がボトルネック
// - transfer cache で remote push を吸収し、central mutex 負荷を削減

#ifndef HZ3_S42_SMALL_XFER
#define HZ3_S42_SMALL_XFER 0
#endif

#ifndef HZ3_S42_SMALL_XFER_SLOTS
#define HZ3_S42_SMALL_XFER_SLOTS 64
#endif
#ifndef HZ3_S42_SMALL_XFER_DISABLE
#define HZ3_S42_SMALL_XFER_DISABLE 0
#endif

// S42-1: MicroOpt for hz3_small_xfer_pop_batch() walk loop (compile-time only)
// Default OFF: require A/B (must not regress r50 and R=0 fixed-cost).
#ifndef HZ3_S42_XFER_POP_PREFETCH
#define HZ3_S42_XFER_POP_PREFETCH 0
#endif

// 1 = prefetch next only (no extra deref), 2 = also prefetch next->next (A/B required).
#ifndef HZ3_S42_XFER_POP_PREFETCH_DIST
#define HZ3_S42_XFER_POP_PREFETCH_DIST 1
#endif

// ============================================================================
// S44: Owner Shared Stash (scale lane remote-heavy optimization)
// ============================================================================
//
// Rationale:
// - Remote free objects go to owner's stash (MPSC, no mutex on push)
// - Alloc miss pops from own stash before central (mutex avoidance)

#ifndef HZ3_S44_OWNER_STASH
#define HZ3_S44_OWNER_STASH 0
#endif

// Push/pop can be controlled separately for A/B
#ifndef HZ3_S44_OWNER_STASH_PUSH
#define HZ3_S44_OWNER_STASH_PUSH (HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE)
#endif

#ifndef HZ3_S44_OWNER_STASH_POP
#define HZ3_S44_OWNER_STASH_POP (HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE)
#endif

// Drain limit
#ifndef HZ3_S44_DRAIN_LIMIT
#define HZ3_S44_DRAIN_LIMIT 32
#endif

// Max objects per bin before overflow
#ifndef HZ3_S44_STASH_MAX_OBJS
#define HZ3_S44_STASH_MAX_OBJS 256
#endif

// S44-3: Owner Stash optimization flags
#ifndef HZ3_S44_OWNER_STASH_COUNT
#define HZ3_S44_OWNER_STASH_COUNT 0
#endif

#ifndef HZ3_S44_OWNER_STASH_FASTPOP
#define HZ3_S44_OWNER_STASH_FASTPOP 1
#endif
#ifndef HZ3_S44_OWNER_STASH_DISABLE
#define HZ3_S44_OWNER_STASH_DISABLE 0
#endif

// Compile-time guard: FASTPOP requires COUNT=0
#if HZ3_S44_OWNER_STASH_FASTPOP && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S44_OWNER_STASH_FASTPOP=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// S44-BD: Bounded drain (CAS partial detach instead of atomic_exchange full drain)
// Reduces cache pollution in r90-like constant-push environments
#ifndef HZ3_S44_BOUNDED_DRAIN
#define HZ3_S44_BOUNDED_DRAIN 0  // default OFF for A/B
#endif

// Fixed slack to add to need (max_take = need + BOUNDED_EXTRA, clamped to need + 64)
#ifndef HZ3_S44_BOUNDED_EXTRA
#define HZ3_S44_BOUNDED_EXTRA 32
#endif

// S44-PF: Prefetch next pointer in pop_batch walk loop
#ifndef HZ3_S44_PREFETCH
#define HZ3_S44_PREFETCH 1  // default ON (GO in perf A/B)
#endif

// S44-PF distance: how many nodes ahead to prefetch
// 1 = prefetch next only, 2 = also prefetch next->next (A/B required)
#ifndef HZ3_S44_PREFETCH_DIST
#define HZ3_S44_PREFETCH_DIST 1
#endif

// S44-4: MicroOptBox candidates for hz3_owner_stash_pop_batch() (compile-time only)
// Default OFF: require A/B (must not regress r50).
#ifndef HZ3_S44_4_STATS
#define HZ3_S44_4_STATS 0
#endif
#ifndef HZ3_S44_4_WALK_NOPREV
#define HZ3_S44_4_WALK_NOPREV 0
#endif

// S44-4: Reuse EPF head hint to avoid the post-spill quick empty load on the
// common non-empty path, while still skipping the atomic drain when the hint
// is NULL (confirmed by a relaxed load).
// GO (Safe Combined): r90 +2.8%, r50 ±0.0%, R=0 +5.7% (2026-01-13)
#ifndef HZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT
#define HZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT 1
#endif

// S44-4: Skip the post-spill quick empty check (trade 1 relaxed load/branch for
// always executing the atomic drain, which will still detect empty via NULL).
// Default OFF: A/B required (could hurt workloads where stash is often empty).
#ifndef HZ3_S44_4_SKIP_QUICK_EMPTY_CHECK
#define HZ3_S44_4_SKIP_QUICK_EMPTY_CHECK 0
#endif

// S44-4: Remove the `if(next)` branch around prefetch in the walk loop (x86-only).
// Default OFF: A/B required; keep portable default conservative.
// GO (Safe Combined): r90 +2.8%, r50 ±0.0%, R=0 +5.7% (2026-01-13)
#ifndef HZ3_S44_4_WALK_PREFETCH_UNCOND
#define HZ3_S44_4_WALK_PREFETCH_UNCOND 1
#endif

// S44-4 EPF: Early prefetch stash head before spill checks (hide future walk latency).
// GO: r90 +9.2%, r50 +2.1% (2026-01-13)
#ifndef HZ3_S44_4_EARLY_PREFETCH
#define HZ3_S44_4_EARLY_PREFETCH 1
#endif
#ifndef HZ3_S44_4_EARLY_PREFETCH_DIST
#define HZ3_S44_4_EARLY_PREFETCH_DIST 1
#endif

// ============================================================================
// S98: small_v2_push_remote_list Stats (research, SSOT observation)
// ============================================================================
//
// Purpose:
// - Observe shape of hz3_small_v2_push_remote_list() calls to identify
//   optimization opportunities (n=1 dominance, local vs remote, S44 overflow).
// - Stats-only, no behavior change. Default OFF.
//
#ifndef HZ3_S98_PUSH_REMOTE_STATS
#define HZ3_S98_PUSH_REMOTE_STATS 0
#endif

// ============================================================================
// S99: small_v2_alloc_slow MicroOptBox (refill push loop)
// ============================================================================
//
// Idea:
// - In hz3_small_v2_alloc_slow(), when we refill a batch (stash/central),
//   the remainder objects are currently pushed one-by-one into the TLS bin.
// - For contiguous pops, the nodes are already linked; we can prepend the list
//   with a single tail->next store (and a single count add).
//
// GO (20runs, interleaved, warmup): r50 +4.63%, r90 +8.29%, R=0 -0.92% (within ±1%),
// dist within ±1% (2026-01-14). Safe to enable for scale lane default.
//
// EXCEPTION: S112 uses spill_array which stores individual pointers (not linked
// via next). S99's prepend_list assumes batch[] is linked, so S99 must be OFF
// when S112 is enabled.
#ifndef HZ3_S99_ALLOC_SLOW_PREPEND_LIST
  #if HZ3_S112_FULL_DRAIN_EXCHANGE
    #define HZ3_S99_ALLOC_SLOW_PREPEND_LIST 0
  #else
    #define HZ3_S99_ALLOC_SLOW_PREPEND_LIST 1
  #endif
#endif

// ============================================================================
// S111: RemotePushN1LeafBox (n==1 fastpath for push_remote_list)
// ============================================================================
//
// Motivation:
// - S98 stats show n1_pct=100% in remote-heavy workloads (r90, r50).
// - hz3_small_v2_push_remote_list() has fixed call/branch overhead for n>1
//   that is unnecessary for the dominant n==1 case.
//
// Design:
// - When n==1, use simplified push_one() functions for S44/S42/central
//   instead of the full push_list() path.
// - push_one() avoids tail manipulation and has fewer branches.
//
// Important:
// - DO NOT call hz3_remote_stash_push() from push_remote_list() (loop danger).
//
// A/B policy:
// - GO: enable for scale lane default (remote-heavy micro win, no dist regressions observed)
#ifndef HZ3_S111_REMOTE_PUSH_N1
  #if HZ3_REMOTE_STASH_SPARSE
    #define HZ3_S111_REMOTE_PUSH_N1 1
  #else
    #define HZ3_S111_REMOTE_PUSH_N1 0
  #endif
#endif

#ifndef HZ3_S111_REMOTE_PUSH_N1_STATS
#define HZ3_S111_REMOTE_PUSH_N1_STATS 0
#endif

#ifndef HZ3_S111_REMOTE_PUSH_N1_FAILFAST
#define HZ3_S111_REMOTE_PUSH_N1_FAILFAST 0
#endif

// ============================================================================
// S88: Small slow-path flush-on-empty (scale lane)
// ============================================================================
//
// Motivation:
// - S87 (flush at slow-path entry) was NO-GO due to overhead.
// - Only flush when small slow-path cache lookup returns empty and we're about to page-allocate.
// - Expected to reduce page alloc rate (S85) with less overhead than S87.
//
#ifndef HZ3_S88_SMALL_FLUSH_ON_EMPTY
#define HZ3_S88_SMALL_FLUSH_ON_EMPTY (HZ3_REMOTE_STASH_SPARSE && HZ3_PTAG_DSTBIN_ENABLE)  // scale lane default ON (GO)
#endif

// Budget for the one-shot flush attempt (sparse ring: entries, not bins).
#ifndef HZ3_S88_SMALL_FLUSH_BUDGET
#define HZ3_S88_SMALL_FLUSH_BUDGET 128
#endif

// ============================================================================
// S48: Owner Stash Spill (TLS leftover cache)
// ============================================================================

#ifndef HZ3_S48_OWNER_STASH_SPILL
#define HZ3_S48_OWNER_STASH_SPILL 1
#endif

// Compile-time guard: SPILL is incompatible with COUNT
#if HZ3_S48_OWNER_STASH_SPILL && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S48_OWNER_STASH_SPILL=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// ============================================================================
// S67: Spill Array (replaces S48 linked-list spill with O(1) array)
// ============================================================================
//
// Rationale (S66 perf analysis):
// - hz3_owner_stash_pop_batch was 28.24% of CPU
// - TLS spill linked-list walk was the hotspot (33.36% in loop)
// - Array-based spill enables O(1) batch pop via memcpy
//
// Trade-off: ~11KB more TLS memory per thread (128 SC * 12 CAP * 8 bytes)

#ifndef HZ3_S67_SPILL_ARRAY
#define HZ3_S67_SPILL_ARRAY 0  // temporarily disabled for comparison
#endif

#ifndef HZ3_S67_SPILL_CAP
#define HZ3_S67_SPILL_CAP 256  // match HZ3_S44_STASH_MAX_OBJS to avoid overflow
#endif

// S67 replaces S48 (mutual exclusion)
#if HZ3_S67_SPILL_ARRAY
#undef HZ3_S48_OWNER_STASH_SPILL
#define HZ3_S48_OWNER_STASH_SPILL 0
#endif

// S67 also requires COUNT=0 (same as S48)
#if HZ3_S67_SPILL_ARRAY && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S67_SPILL_ARRAY=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// Optional: overflow one-shot debug log
#ifndef HZ3_S67_SPILL_OVERFLOW_SHOT
#define HZ3_S67_SPILL_OVERFLOW_SHOT 0
#endif

#ifndef HZ3_S67_STATS
#define HZ3_S67_STATS 0
#endif

#ifndef HZ3_S67_FAILFAST
#define HZ3_S67_FAILFAST 0
#endif

// ============================================================================
// S67-2: Two-Layer Spill (array + overflow list)
// ============================================================================
//
// Rationale (S67 failure analysis):
// - S67 pushed overflow back to owner stash via CAS
// - CAS prepend requires hz3_list_find_tail() = O(n) → 10.5s regression
// - S67-2: overflow goes to TLS list (head only, O(1) push)
//
// Structure:
// - spill_array[SC][CAP]: O(1) batch pop via memcpy
// - spill_overflow[SC]: linked list head for excess (O(1) push)
// - No push-back to owner stash (eliminates tail-find)

#ifndef HZ3_S67_SPILL_ARRAY2
  // Defined above (forward declaration).
#endif

// S67-4: Bounded drain (limit stash drain to avoid overflow/rollback)
#ifndef HZ3_S67_DRAIN_LIMIT
#define HZ3_S67_DRAIN_LIMIT 0
#endif

// ============================================================================
// S112: Full Drain Exchange (replace bounded drain CAS loop)
// ============================================================================
//
// Motivation:
// - S67-4 bounded drain uses CAS retry + O(n) re-walk on failure.
// - S112 uses atomic_exchange to drain the stash in O(1) (mimalloc-like),
//   and keeps leftovers in TLS spill (array + overflow list).
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

// S97: Bucketize/merge within one flush_budget window by (dst,bin), turning
// repeated (dst,bin) dispatches into a single `push_list(n>1)` call.
// =0: OFF (default)
// =1: S97-1 (hash + open addressing)
// =2: S97-2 (direct-map + stamp, probe-less)
// =3: S97-3 (direct-map sparse-set, stamp-less)
// =4: S97-4 (direct-map + touched reset)
// =5: S97-5 (direct-map flat slot; epoch+idx in one table)
// =6: S97-8 (table-less radix sort + group; stack-only, no TLS tables)
#ifndef HZ3_S97_REMOTE_STASH_BUCKET
#define HZ3_S97_REMOTE_STASH_BUCKET 0
#endif
#if HZ3_S97_REMOTE_STASH_BUCKET > 6
#error "HZ3_S97_REMOTE_STASH_BUCKET must be 0..6"
#endif

// Max number of distinct keys held per bucketization round.
// When exceeded, current buckets are dispatched and the map is reset.
#ifndef HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS
#define HZ3_S97_REMOTE_STASH_BUCKET_MAX_KEYS 128
#endif

// Skip `hz3_obj_set_next(ptr, NULL)` when draining sparse ring entries.
// Safe in release (push_list will overwrite tail->next), but incompatible with
// `HZ3_LIST_FAILFAST` / debug list checkers that require tail->next==NULL.
#ifndef HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL
#define HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL 0
#endif

// S97-6 (OwnerStashPushMicroBatchBox) was archived (NO-GO). See hz3_config_archive.h.

// S128 (RemoteStash Defer-MinRun) was archived (NO-GO). See hz3_config_archive.h.

// S94 is mutually exclusive with S67/S67-2
#if HZ3_S94_SPILL_LITE && (HZ3_S67_SPILL_ARRAY || HZ3_S67_SPILL_ARRAY2)
#error "S94 and S67/S67-2 are mutually exclusive"
#endif

// ============================================================================
// S110: SegHdr PerPage FreeMeta (segment-local metadata)
// ============================================================================
//
// Motivation:
// - hz3_free PTAG lookup (global table) is a bottleneck (40.83% vs mimalloc 32.65%)
// - mimalloc uses segment-local metadata for better cache locality
//
// Status (scale lane):
// - S110-0: GO (meta hit-rate 100% observed)
// - S110-1 (PTAG bypass in hz3_free): NO-GO (slower than PTAG32 due to seg lookup overhead)
//
// New field in Hz3SegHdr:
//   _Atomic(uint16_t) page_bin_plus1[HZ3_PAGES_PER_SEG]
//   0 = unknown/not-allocated, 1..N = bin + 1
//
// Enable the metadata field and its write/clear boundaries.
// Default OFF (research opt-in only).
#ifndef HZ3_S110_META_ENABLE
#define HZ3_S110_META_ENABLE 0
#endif

// Enable atexit one-shot stats (observation; safe even when FREE_FAST is OFF).
#ifndef HZ3_S110_STATS
#define HZ3_S110_STATS 0
#endif

// Debug-only: fail fast on inconsistent meta/bin/dst in S110 paths.
#ifndef HZ3_S110_FAILFAST
#define HZ3_S110_FAILFAST 0
#endif

// S110_FREE_FAST_ENABLE and deprecated aliases moved to hz3_config_archive.h

// ============================================================================
// S142: Lock-Free MPSC Central Bin (pthread_mutex overhead elimination)
// ============================================================================
//
// Motivation:
// - perf shows pthread_mutex overhead:
//   - hz3_small_xfer (mutex): 6.12%
//   - hz3_central_bin (mutex): 5.60%
//   - Total: 12.11%
//
// Design:
// - MPSC confirmed: Pop is always from my_shard owner (TLS t_hz3_cache.my_shard)
// - Push uses CAS loop (multiple producers from remote free)
// - Pop uses atomic_exchange + push-back remainder
//
// Expected effect: 12.11% → ~1% (net gain +10%)
//
// Phases:
// - S142-A: hz3_central_bin lock-free (Phase 1) - GO (2026-01-17)
// - S142-B: hz3_small_xfer lock-free (Phase 2) - GO (2026-01-17)
//
// Results (S142-A: central lock-free):
// - T=8 RANDOM: +11.8%, r90: T=8 +5.9%, T=16 +25.7%, T=32 +56.3%
// - R=0 fixed cost: -0.6% (noise)
//
// Results (S142-B: xfer lock-free):
// - R=0 T=8: +1.9% (no regression)
// - r90 T=8: +41%, r90 T=32: +105%

#ifndef HZ3_S142_CENTRAL_LOCKFREE
// Default OFF at header level; scale/p32 lanes enable via Makefile.
#define HZ3_S142_CENTRAL_LOCKFREE 0
#endif

// Debug: fail fast on count underflow (indicates bug)
#ifndef HZ3_S142_FAILFAST
#define HZ3_S142_FAILFAST 0
#endif

// Compile-time guard: S142 is incompatible with S86 shadow (needs mutex)
#if HZ3_S142_CENTRAL_LOCKFREE && HZ3_S86_CENTRAL_SHADOW
#error "HZ3_S142_CENTRAL_LOCKFREE=1 requires HZ3_S86_CENTRAL_SHADOW=0"
#endif

// Phase 2: Transfer cache lock-free
#ifndef HZ3_S142_XFER_LOCKFREE
// Default OFF at header level; scale/p32 lanes enable via Makefile.
#define HZ3_S142_XFER_LOCKFREE 0
#endif
