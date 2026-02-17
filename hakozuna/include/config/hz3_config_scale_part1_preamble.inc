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

