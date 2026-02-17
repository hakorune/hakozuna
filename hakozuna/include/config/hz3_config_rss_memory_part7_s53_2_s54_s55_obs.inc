// S53-2: ThrottleBox
#ifndef HZ3_S53_THROTTLE
#define HZ3_S53_THROTTLE 1
#endif

#ifndef HZ3_S53_MADVISE_RATE_PAGES
#define HZ3_S53_MADVISE_RATE_PAGES 1024
#endif

#ifndef HZ3_S53_MADVISE_MIN_INTERVAL
#define HZ3_S53_MADVISE_MIN_INTERVAL 64
#endif

// S50: Size class parameters
#define HZ3_LARGE_SC_MIN   (32u << 10)
#define HZ3_LARGE_SC_MAX   (64u << 20)
#define HZ3_LARGE_SC_COUNT 97

// ============================================================================
// S54: SegmentPageScavengeBox OBSERVE mode
// ============================================================================

#ifndef HZ3_SEG_SCAVENGE_OBSERVE
#define HZ3_SEG_SCAVENGE_OBSERVE 0
#endif

#ifndef HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES
#define HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES 32
#endif

// ============================================================================
// S55: RetentionPolicyBox OBSERVE mode
// ============================================================================

#ifndef HZ3_S55_RETENTION_OBSERVE
#define HZ3_S55_RETENTION_OBSERVE 0
#endif

// S55-OBS2: Estimate resident bytes for selected metadata via mincore(2).
// Debug/triage only (one-shot); keep default OFF.
#ifndef HZ3_S55_RETENTION_OBSERVE_MINCORE
#define HZ3_S55_RETENTION_OBSERVE_MINCORE 0
#endif

// S134: Trigger epoch from small v2 slow path (refill boundary).
// Motivation: some workloads are small-dominant and may not hit medium slow paths,
// so epoch-only maintenance (S65/S55 observe/etc.) can be starved.
// Default OFF (research opt-in; no hot-path changes).
#ifndef HZ3_S134_EPOCH_ON_SMALL_SLOW
#define HZ3_S134_EPOCH_ON_SMALL_SLOW 0
#endif

// ============================================================================
