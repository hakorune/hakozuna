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

// S113: SegMath + page_bin_plus1 fast dispatch
// Enabled by default when S110_META_ENABLE is on.
#ifndef HZ3_S113_FREE_FAST_ENABLE
#define HZ3_S113_FREE_FAST_ENABLE (HZ3_S110_META_ENABLE)
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

// ============================================================================
// S154: Spill Overflow Prefetch (owner_stash_pop_batch optimization)
// ============================================================================
//
// Motivation:
// - Spill overflow walk (lines 43-49, 289-294) has pointer chasing
// - Prefetch next pointer to hide memory latency
// - Expected: +0.5-1.5% on xmalloc-test (remote-heavy workloads)
//
// Design:
// - Add __builtin_prefetch(hz3_obj_get_next(ov), 0, 3) in spill_overflow loop
// - Distance 1 fixed (simple optimization, low risk)
//
// Results (2026-01-19):
// - xmalloc-test: +7.06% (1.925s → 1.789s)
// - SSOT: small -2.00%, medium +4.00%, mixed +0.30% (all pass -3% threshold)
// - GO: Enable by default
//
#ifndef HZ3_S154_SPILL_PREFETCH
#define HZ3_S154_SPILL_PREFETCH 1  // GO (2026-01-19): +7.06% on xmalloc-test
#endif
