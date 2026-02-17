// S56-1: Pack Pool Best-Fit
// ============================================================================

#ifndef HZ3_S56_PACK_BESTFIT
#define HZ3_S56_PACK_BESTFIT 0
#endif

#if HZ3_S56_PACK_BESTFIT
#ifndef HZ3_S56_PACK_BESTFIT_K
#define HZ3_S56_PACK_BESTFIT_K 2
#endif
#ifndef HZ3_S56_PACK_BESTFIT_STATS
#define HZ3_S56_PACK_BESTFIT_STATS 0
#endif
#endif

// ============================================================================
// S56-2: Active Segment Set
// ============================================================================

#ifndef HZ3_S56_ACTIVE_SET
#define HZ3_S56_ACTIVE_SET 0
#endif

#if HZ3_S56_ACTIVE_SET
#ifndef HZ3_S56_ACTIVE_SET_K
#define HZ3_S56_ACTIVE_SET_K 2
#endif
#ifndef HZ3_S56_ACTIVE_SET_STATS
#define HZ3_S56_ACTIVE_SET_STATS 0
#endif
#endif

// ============================================================================
// S58: TCache Decay Box
// ============================================================================

#ifndef HZ3_S58_TCACHE_DECAY
#define HZ3_S58_TCACHE_DECAY 0
#endif

#if HZ3_S58_TCACHE_DECAY
#ifndef HZ3_S58_MAX_OVERAGES
#define HZ3_S58_MAX_OVERAGES 3
#endif
#ifndef HZ3_S58_RECLAIM_BUDGET
#define HZ3_S58_RECLAIM_BUDGET 64
#endif
#ifndef HZ3_S58_STATS
#define HZ3_S58_STATS 1
#endif
#ifndef HZ3_S58_INITIAL_MAX_LEN
#define HZ3_S58_INITIAL_MAX_LEN 32
#endif
#endif

// ============================================================================
// S60: PurgeRangeQueueBox
// ============================================================================

#ifndef HZ3_S60_PURGE_RANGE_QUEUE
#define HZ3_S60_PURGE_RANGE_QUEUE 0
#endif

#if HZ3_S60_PURGE_RANGE_QUEUE
#ifndef HZ3_S60_QUEUE_SIZE
#define HZ3_S60_QUEUE_SIZE 256
#endif
#ifndef HZ3_S60_BUDGET_CALLS
#define HZ3_S60_BUDGET_CALLS 32
#endif
#ifndef HZ3_S60_BUDGET_PAGES
#define HZ3_S60_BUDGET_PAGES 512
#endif
#ifndef HZ3_S60_STATS
#define HZ3_S60_STATS 1
#endif
#endif

// ============================================================================
// S61: DtorHardPurgeBox (thread-exit hard purge)
// ============================================================================

#ifndef HZ3_S61_DTOR_HARD_PURGE
#define HZ3_S61_DTOR_HARD_PURGE 0
#endif

#if HZ3_S61_DTOR_HARD_PURGE
#ifndef HZ3_S61_DTOR_MAX_PAGES
#define HZ3_S61_DTOR_MAX_PAGES 65536
#endif
#ifndef HZ3_S61_DTOR_MAX_CALLS
#define HZ3_S61_DTOR_MAX_CALLS 256
#endif
#ifndef HZ3_S61_DTOR_STATS
#define HZ3_S61_DTOR_STATS 1
#endif
#ifndef HZ3_S61_DTOR_FAILFAST
#define HZ3_S61_DTOR_FAILFAST 0
#endif
#endif

// ============================================================================
