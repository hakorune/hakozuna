// hz3_config_sub4k.h - Sub4k retire/purge/triage configuration (S62-S63)
// Part of hz3_config.h split for maintainability
#pragma once

// ============================================================================
// S62-0: OBSERVE (stats only, no madvise)
// ============================================================================
// Purpose: count free_bits purge potential before implementing S62-1 PageRetireBox.
// Expected result: free_pages ~ 0 (all pages allocated to small_v2).
// Note: free_pages==0 does not prove retirement is needed (design already implies it);
//       OBSERVE only measures "current free_bits purge potential is small".

#ifndef HZ3_S62_OBSERVE
#define HZ3_S62_OBSERVE 0  // default OFF
#endif
#ifndef HZ3_S62_OBSERVE_DISABLE
#define HZ3_S62_OBSERVE_DISABLE 0
#endif

// ============================================================================
// S62-1: PageRetireBox - retire fully-empty small pages to free_bits
// ============================================================================
// Thread-exit cold path. Does NOT call madvise - just updates free_bits bitmap.
// After this, S62-2 (future) or existing purge logic can madvise the retired pages.
// Scope: small_v2 only (sub4k deferred to S62-1b).

#ifndef HZ3_S62_RETIRE
#define HZ3_S62_RETIRE 0  // default OFF
#endif
#ifndef HZ3_S62_RETIRE_DISABLE
#define HZ3_S62_RETIRE_DISABLE 0
#endif
#ifndef HZ3_S62_RETIRE_MPROTECT
#define HZ3_S62_RETIRE_MPROTECT 0
#endif
#ifndef HZ3_S62_RETIRE_FAILFAST
#define HZ3_S62_RETIRE_FAILFAST 0
#endif

// S62-1 safety: by default, skip S62 retire when remote paths are enabled.
// Rationale: without a global predicate (e.g., S69 live_count), S62 can retire
// pages while objects are still in transit or still-live on other threads.
#ifndef HZ3_S62_RETIRE_SKIP_WHEN_REMOTE
#define HZ3_S62_RETIRE_SKIP_WHEN_REMOTE 1
#endif

// ============================================================================
// S62-1: Stale Fail-Fast Box - detect stale free_bits access
// ============================================================================
// Purpose: Abort immediately when accessing an object on a page marked as free.
// This catches bugs where objects escape to tcache/central after page retirement.
// Only works with S62_RETIRE enabled. Adds cold-path checks only.
#ifndef HZ3_S62_STALE_FAILFAST
#define HZ3_S62_STALE_FAILFAST 0  // default OFF
#endif

// ============================================================================
// S62-1b: Sub4kRunRetireBox - retire fully-empty sub4k 2-page runs to free_bits
// ============================================================================
// Thread-exit cold path. Only updates free_bits (no madvise).
// Scope: sub4k only (2-page runs). Called after S62-1, before S62-2.

#ifndef HZ3_S62_SUB4K_RETIRE
#define HZ3_S62_SUB4K_RETIRE 0  // default OFF
#endif
#ifndef HZ3_S62_SUB4K_RETIRE_DISABLE
#define HZ3_S62_SUB4K_RETIRE_DISABLE 0
#endif

// ============================================================================
// S62-2: DtorSmallPagePurgeBox - purge retired small pages
// ============================================================================
// Thread-exit cold path. Calls madvise(DONTNEED) on free_bits=1 pages.
// Uses consecutive run batching and budget control.
// Requires S62-1 to have retired pages first (or purges initially-free pages).

#ifndef HZ3_S62_PURGE
#define HZ3_S62_PURGE 0  // default OFF
#endif
#ifndef HZ3_S62_PURGE_DISABLE
#define HZ3_S62_PURGE_DISABLE 0
#endif

// S62-2: Budget control (max madvise calls per destructor)
// Prevents runaway madvise loops (same philosophy as S61).
#ifndef HZ3_S62_PURGE_MAX_CALLS
#define HZ3_S62_PURGE_MAX_CALLS 256
#endif

// ============================================================================
// S62-REMOTE-GUARD: Skip S62/S65/S45 when remote paths (xfer/stash) are enabled
// ============================================================================
// Purpose: Prevent S62 retire/purge AND S65 release/reclaim from running
// when xfer/stash are active. This avoids races where pages are modified
// while objects are still in transit through remote free paths.
// S70: Now default OFF - S69 live_count provides safety. Keep as emergency knob.
#ifndef HZ3_S62_REMOTE_GUARD
#define HZ3_S62_REMOTE_GUARD 0  // default OFF - S69 provides safety, keep as emergency knob
#endif

// Sub-flags for finer control (all default to same as S62_REMOTE_GUARD)
#ifndef HZ3_S65_REMOTE_GUARD
#define HZ3_S65_REMOTE_GUARD HZ3_S62_REMOTE_GUARD
#endif
#ifndef HZ3_S45_REMOTE_GUARD
#define HZ3_S45_REMOTE_GUARD HZ3_S62_REMOTE_GUARD
#endif

// ============================================================================
// S62-1G: SingleThreadRetireGate - Allow S62 when total live threads == 1
// ============================================================================
// Purpose: Override HZ3_S62_REMOTE_GUARD when the allocator is in safe
// single-threaded state (total_live_count across all shards == 1).
//
// Rationale:
// - HZ3_S62_REMOTE_GUARD=1 completely disables S62 when remote paths are enabled,
//   preventing RSS reduction even in single-thread scenarios.
// - Single-thread condition (total_live==1) guarantees no objects in transit,
//   making S62 retire/purge safe.
//
// Safety:
// - Thread exit has already flushed all TLS bins (line 185-273 in hz3_tcache.c).
// - Total live count is checked after flush but before shard_live_count decrement.
// - No race: new threads can't have objects in pages being retired.

#ifndef HZ3_S62_SINGLE_THREAD_GATE
#define HZ3_S62_SINGLE_THREAD_GATE 0  // default OFF for A/B testing
#endif

// Fail-fast mode: abort if multi-thread detected when gate is enabled.
// Useful for detecting unexpected thread creation in nominally single-thread environments.
#ifndef HZ3_S62_SINGLE_THREAD_FAILFAST
#define HZ3_S62_SINGLE_THREAD_FAILFAST 0  // default OFF
#endif

// ============================================================================
// S69: SmallPageLiveCountBox - live_count based retirement supplement
// ============================================================================
// When enabled:
// - live_count incremented on alloc (object to user)
// - live_count decremented on free (object from user)
// - S64 retire predicate: count==capacity && live_count==0
// - S62 central-drain retire is disabled
// Safe with xfer/stash because count==capacity proves all objects in central.

#ifndef HZ3_S69_LIVECOUNT
#define HZ3_S69_LIVECOUNT 0  // default OFF until verified
#endif

// Compile-time dependency checks
#if HZ3_S69_LIVECOUNT
  #if !HZ3_SMALL_V2_ENABLE
    #error "S69 requires HZ3_SMALL_V2_ENABLE=1"
  #endif
  #if !HZ3_SEG_SELF_DESC_ENABLE
    #error "S69 requires HZ3_SEG_SELF_DESC_ENABLE=1"
  #endif
  #if !HZ3_S64_RETIRE_SCAN
    #error "S69 requires HZ3_S64_RETIRE_SCAN=1 (S62 is disabled under S69)"
  #endif
#endif

#ifndef HZ3_S69_LIVECOUNT_FAILFAST
#define HZ3_S69_LIVECOUNT_FAILFAST HZ3_S69_LIVECOUNT
#endif

#ifndef HZ3_S69_LIVECOUNT_STATS
#define HZ3_S69_LIVECOUNT_STATS HZ3_S69_LIVECOUNT
#endif

// ============================================================================
// S62-1A: AtExitGate - Execute S62 retire/purge once at process exit
// ============================================================================
// Purpose: For single-thread applications, ensure S62 runs at atexit even if
// thread destructor doesn't execute (main thread exits via process exit).
//
// Design:
// - Register atexit handler in hz3_tcache_ensure_init_slow() (once per process)
// - Handler uses atomic one-shot guard (same pattern as S62 OBSERVE)
// - Uninitialized guard: skip if tcache never initialized
// - Optional single-thread check via HZ3_S62_SINGLE_THREAD_GATE
// - Calls hz3_s62_retire() → hz3_s62_sub4k_retire() → hz3_s62_purge()

#ifndef HZ3_S62_ATEXIT_GATE
#define HZ3_S62_ATEXIT_GATE 0  // default OFF for A/B testing
#endif

// Observability: log execution to stderr (optional, default OFF for release)
#ifndef HZ3_S62_ATEXIT_LOG
#define HZ3_S62_ATEXIT_LOG 0  // default OFF (use 1 for debugging)
#endif

// ============================================================================
// S135-1B: Partial SC Observation - Identify size classes with partial pages
// ============================================================================
// Purpose: Observe which size classes create partial pages (0 < live_count < capacity)
// at process exit. Extends S62_OBSERVE with per-sizeclass partial page counting.
// Output: Single-line report with top 5 size classes by partial page count.
//
// Scope: OBSERVE-only (stats, no madvise), event-only (atexit).
// Requires: HZ3_S69_LIVECOUNT=1 (for live_count field).
//
// Note: S69_LIVECOUNT requires HZ3_S64_RETIRE_SCAN=1. When S135 is enabled,
//       S64 will also be active. This is acceptable for observation purposes.

#ifndef HZ3_S135_PARTIAL_SC_OBS
#define HZ3_S135_PARTIAL_SC_OBS 0  // default OFF
#endif

// Compile-time dependency checks
#if HZ3_S135_PARTIAL_SC_OBS
  #if !HZ3_S69_LIVECOUNT
    #error "S135 requires HZ3_S69_LIVECOUNT=1 (live_count field needed)"
  #endif
  #if !HZ3_S62_OBSERVE
    #error "S135 extends S62_OBSERVE (must be enabled)"
  #endif
#endif

// ============================================================================
// S135-1C: Full Pages / Tail Waste Observation
// ============================================================================
// Purpose: Observe which size classes create tail waste and full pages waste
// at process exit. Extends S135_PARTIAL_SC_OBS to track waste metrics.
// Output: Single-line report with top 5 size classes by waste contribution.
//
// Scope: OBSERVE-only (stats, no madvise), event-only (atexit).
// Requires: HZ3_S135_PARTIAL_SC_OBS=1 (same dependencies as S135-1B).

#ifndef HZ3_S135_FULL_SC_OBS
#define HZ3_S135_FULL_SC_OBS 0  // default OFF
#endif

// Compile-time dependency checks
#if HZ3_S135_FULL_SC_OBS
  #if !HZ3_S135_PARTIAL_SC_OBS
    #error "S135_FULL_SC_OBS requires HZ3_S135_PARTIAL_SC_OBS=1 (extends S135-1B)"
  #endif
#endif
