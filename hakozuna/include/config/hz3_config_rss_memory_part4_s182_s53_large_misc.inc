// S182 tuning knobs (spinlock path only)
// Failed-spin count where thread yielding starts.
#ifndef HZ3_S182_SPIN_YIELD_START
#define HZ3_S182_SPIN_YIELD_START 1024u
#endif

// Yield every N failed spins after HZ3_S182_SPIN_YIELD_START.
#ifndef HZ3_S182_SPIN_YIELD_INTERVAL
#define HZ3_S182_SPIN_YIELD_INTERVAL 1024u
#endif

// Execute a CPU pause/yield hint every N failed spins (0=disabled).
#ifndef HZ3_S182_SPIN_PAUSE_INTERVAL
#define HZ3_S182_SPIN_PAUSE_INTERVAL 0u
#endif

#if HZ3_S182_SPIN_YIELD_INTERVAL < 1
#error "HZ3_S182_SPIN_YIELD_INTERVAL must be >= 1"
#endif

// Adaptive yield mode:
// 0 = fixed interval (legacy)
// 1 = interval doubles every stage (start+interval*(2^k-1), capped by MAX_STAGE)
#ifndef HZ3_S182_SPIN_ADAPTIVE_YIELD
#define HZ3_S182_SPIN_ADAPTIVE_YIELD 0
#endif

#ifndef HZ3_S182_SPIN_ADAPTIVE_MAX_STAGE
#define HZ3_S182_SPIN_ADAPTIVE_MAX_STAGE 4u
#endif

// S53: LargeCacheBudgetBox
#ifndef HZ3_LARGE_CACHE_BUDGET
#define HZ3_LARGE_CACHE_BUDGET 0
#endif

#ifndef HZ3_LARGE_CACHE_SOFT_BYTES
#define HZ3_LARGE_CACHE_SOFT_BYTES (4096ULL << 20)
#endif

#ifndef HZ3_LARGE_CACHE_HARD_BYTES
#define HZ3_LARGE_CACHE_HARD_BYTES (8192ULL << 20)
#endif

#ifndef HZ3_LARGE_CACHE_STATS
#define HZ3_LARGE_CACHE_STATS 0
#endif

