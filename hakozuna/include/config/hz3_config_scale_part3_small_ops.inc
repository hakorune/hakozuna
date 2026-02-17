
// S162: Skip hz3_owner_stash_init() call on hot path when TLS is already initialized.
// Safe because tcache init calls hz3_owner_stash_init() once per process.
#ifndef HZ3_S162_OWNER_STASH_INIT_FASTPATH
#define HZ3_S162_OWNER_STASH_INIT_FASTPATH 1
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

// S164: Skip quick empty check when we already got some objects from spill.
// Reduces one load/branch on spill-hit path; A/B required.
#ifndef HZ3_S164_SKIP_QUICK_EMPTY_IF_GOT
  #if HZ3_S180_S112_BOUNDED_COMBO
    #define HZ3_S164_SKIP_QUICK_EMPTY_IF_GOT 1
  #else
    #define HZ3_S164_SKIP_QUICK_EMPTY_IF_GOT 0
  #endif
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
// S204: Larson Diagnostic Instrumentation (default OFF)
// Detailed breakdown of inbox push/drain for root cause analysis.
#ifndef HZ3_S204_LARSON_DIAG
#define HZ3_S204_LARSON_DIAG 0
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
