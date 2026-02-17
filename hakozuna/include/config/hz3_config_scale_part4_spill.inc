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
