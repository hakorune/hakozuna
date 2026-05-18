// S207-1: high-band cache cap for large path (>64KB classes by default).
// Keeps <=64KB behavior unchanged while relaxing eviction pressure for high classes.
#ifndef HZ3_S207_HIBAND_CACHE
#define HZ3_S207_HIBAND_CACHE 0
#endif

#ifndef HZ3_S207_HIBAND_SC_MIN
#define HZ3_S207_HIBAND_SC_MIN 8
#endif

#ifndef HZ3_S207_HIBAND_HARD_BYTES
#define HZ3_S207_HIBAND_HARD_BYTES (16384ULL << 20)
#endif

#if HZ3_S207_HIBAND_SC_MIN < 0
#error "HZ3_S207_HIBAND_SC_MIN must be >= 0"
#endif

// S207-2: targeted large-cache reuse (miss-driven admission for high-band classes).
// Keep global hard cap unchanged and admit only classes with observed reuse demand.
#ifndef HZ3_S207_TARGETED_REUSE
#define HZ3_S207_TARGETED_REUSE 1
#endif

#ifndef HZ3_S207_REUSE_SC_MIN
#define HZ3_S207_REUSE_SC_MIN 8
#endif

#ifndef HZ3_S207_REUSE_MISS_CREDIT_INC
#define HZ3_S207_REUSE_MISS_CREDIT_INC 4
#endif

#ifndef HZ3_S207_REUSE_ADMIT_COST
#define HZ3_S207_REUSE_ADMIT_COST 1
#endif

#ifndef HZ3_S207_REUSE_CREDIT_MAX
#define HZ3_S207_REUSE_CREDIT_MAX 64
#endif

#ifndef HZ3_S207_REUSE_FLOOR_NODES
#define HZ3_S207_REUSE_FLOOR_NODES 8
#endif

#if HZ3_S207_REUSE_SC_MIN < 0
#error "HZ3_S207_REUSE_SC_MIN must be >= 0"
#endif

#if HZ3_S207_REUSE_MISS_CREDIT_INC < 1
#error "HZ3_S207_REUSE_MISS_CREDIT_INC must be >= 1"
#endif

#if HZ3_S207_REUSE_ADMIT_COST < 1
#error "HZ3_S207_REUSE_ADMIT_COST must be >= 1"
#endif

#if HZ3_S207_REUSE_CREDIT_MAX < HZ3_S207_REUSE_ADMIT_COST
#error "HZ3_S207_REUSE_CREDIT_MAX must be >= HZ3_S207_REUSE_ADMIT_COST"
#endif

#if HZ3_S207_REUSE_FLOOR_NODES < 0
#error "HZ3_S207_REUSE_FLOOR_NODES must be >= 0"
#endif

// S233: hot-band forced admission (default ON)
// Keep S207 targeted reuse globally, but force admit in selected hot large-band
// classes to reduce over-pruning (admit_skip -> mmap/munmap churn).
#ifndef HZ3_S233_HOTBAND_ALWAYS_ADMIT
#define HZ3_S233_HOTBAND_ALWAYS_ADMIT 1
#endif

#ifndef HZ3_S233_SC_MIN
#define HZ3_S233_SC_MIN 8
#endif

#ifndef HZ3_S233_SC_MAX
#define HZ3_S233_SC_MAX 15
#endif

#if HZ3_S233_SC_MIN < 0
#error "HZ3_S233_SC_MIN must be >= 0"
#endif

#if HZ3_S233_SC_MAX < HZ3_S233_SC_MIN
#error "HZ3_S233_SC_MAX must be >= HZ3_S233_SC_MIN"
#endif

// S237: hiband retain + bounded hard-evict on free path.
// Goal: reduce large-path mmap/munmap bursts on sc>=8 while keeping caps bounded.
#ifndef HZ3_S237_HIBAND_RETAIN_TUNE
#define HZ3_S237_HIBAND_RETAIN_TUNE 1
#endif

#ifndef HZ3_S237_SC_MIN
#define HZ3_S237_SC_MIN 8
#endif

#ifndef HZ3_S237_HIBAND_HARD_BYTES
#define HZ3_S237_HIBAND_HARD_BYTES (1024ULL << 20)
#endif

#ifndef HZ3_S237_INSERT_SLACK_BYTES
#define HZ3_S237_INSERT_SLACK_BYTES (128ULL << 20)
#endif

#ifndef HZ3_S237_EVICT_BUDGET
#define HZ3_S237_EVICT_BUDGET 8
#endif

#if HZ3_S237_SC_MIN < 0
#error "HZ3_S237_SC_MIN must be >= 0"
#endif

#if HZ3_S237_HIBAND_HARD_BYTES < 0
#error "HZ3_S237_HIBAND_HARD_BYTES must be >= 0"
#endif

#if HZ3_S237_INSERT_SLACK_BYTES < 0
#error "HZ3_S237_INSERT_SLACK_BYTES must be >= 0"
#endif

#if HZ3_S237_EVICT_BUDGET < 1
#error "HZ3_S237_EVICT_BUDGET must be >= 1"
#endif

// S268: large scache RSS cap.
//
// Diagnostic/RSS policy probe for plateau-like large objects. It bounds
// per-class cache admission for high large-cache classes while leaving the
// hot 64K/page-medium rows unchanged by default.
#ifndef HZ3_S268_LARGE_SCACHE_RSS_CAP
#define HZ3_S268_LARGE_SCACHE_RSS_CAP 0
#endif

#ifndef HZ3_S268_SC_MIN
#define HZ3_S268_SC_MIN 24
#endif

#ifndef HZ3_S268_MAX_NODES_PER_SC
#define HZ3_S268_MAX_NODES_PER_SC 8
#endif

#if HZ3_S268_SC_MIN < 0
#error "HZ3_S268_SC_MIN must be >= 0"
#endif

#if HZ3_S268_MAX_NODES_PER_SC < 1
#error "HZ3_S268_MAX_NODES_PER_SC must be >= 1"
#endif

// S270: large scache idle trim.
//
// Diagnostic/RSS policy probe for plateau-like high-band large objects.
// Unlike S268's hard admission cap, this keeps admitting during active phases
// and trims old cache-tail entries only after a push-only burst suggests that
// the class has gone idle.
#ifndef HZ3_S270_LARGE_SCACHE_IDLE_TRIM
#define HZ3_S270_LARGE_SCACHE_IDLE_TRIM 0
#endif

#ifndef HZ3_S270_SC_MIN
#define HZ3_S270_SC_MIN 24
#endif

#ifndef HZ3_S270_TARGET_NODES_PER_SC
#define HZ3_S270_TARGET_NODES_PER_SC 8
#endif

#ifndef HZ3_S270_TRIGGER_NODES_PER_SC
#define HZ3_S270_TRIGGER_NODES_PER_SC 24
#endif

#ifndef HZ3_S270_IDLE_PUSHES_PER_SC
#define HZ3_S270_IDLE_PUSHES_PER_SC 16
#endif

#ifndef HZ3_S270_TRIM_BATCH_MAX
#define HZ3_S270_TRIM_BATCH_MAX 16
#endif

#ifndef HZ3_S270_REARM_AFTER_TRIM
#define HZ3_S270_REARM_AFTER_TRIM 0
#endif

#ifndef HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS
#define HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS HZ3_S270_LARGE_SCACHE_IDLE_TRIM
#endif

// S270 debug-only list and victim validation.
// This is intentionally separate from STATS so production/research timing lanes
// can keep counters without the heavy list walk/failfast checks.
#ifndef HZ3_S270_DEBUG_VALIDATE
#define HZ3_S270_DEBUG_VALIDATE 0
#endif

#ifndef HZ3_S270_DEBUG_FAILFAST
#define HZ3_S270_DEBUG_FAILFAST HZ3_S270_DEBUG_VALIDATE
#endif

// Debug-only S270 victim quarantine. Detached trim victims are retired but not
// unmapped/deferred. This intentionally leaks memory and exists only to isolate
// S270 trim-dispose side effects from list-detach policy.
#ifndef HZ3_S270_DEBUG_QUARANTINE_VICTIMS
#define HZ3_S270_DEBUG_QUARANTINE_VICTIMS 0
#endif

#if HZ3_S270_SC_MIN < 0
#error "HZ3_S270_SC_MIN must be >= 0"
#endif

#if HZ3_S270_TARGET_NODES_PER_SC < 1
#error "HZ3_S270_TARGET_NODES_PER_SC must be >= 1"
#endif

#if HZ3_S270_TRIGGER_NODES_PER_SC < HZ3_S270_TARGET_NODES_PER_SC
#error "HZ3_S270_TRIGGER_NODES_PER_SC must be >= HZ3_S270_TARGET_NODES_PER_SC"
#endif

#if HZ3_S270_IDLE_PUSHES_PER_SC < 1
#error "HZ3_S270_IDLE_PUSHES_PER_SC must be >= 1"
#endif

#if HZ3_S270_TRIM_BATCH_MAX < 1
#error "HZ3_S270_TRIM_BATCH_MAX must be >= 1"
#endif

#if HZ3_S270_REARM_AFTER_TRIM < 0 || HZ3_S270_REARM_AFTER_TRIM > 1
#error "HZ3_S270_REARM_AFTER_TRIM must be 0 or 1"
#endif

#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS < 0 || HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS > 1
#error "HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS must be 0 or 1"
#endif

#if HZ3_S270_DEBUG_VALIDATE < 0 || HZ3_S270_DEBUG_VALIDATE > 1
#error "HZ3_S270_DEBUG_VALIDATE must be 0 or 1"
#endif

#if HZ3_S270_DEBUG_FAILFAST < 0 || HZ3_S270_DEBUG_FAILFAST > 1
#error "HZ3_S270_DEBUG_FAILFAST must be 0 or 1"
#endif

#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS < 0 || HZ3_S270_DEBUG_QUARANTINE_VICTIMS > 1
#error "HZ3_S270_DEBUG_QUARANTINE_VICTIMS must be 0 or 1"
#endif

#if HZ3_S270_DEBUG_QUARANTINE_VICTIMS && !HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS
#error "HZ3_S270_DEBUG_QUARANTINE_VICTIMS requires HZ3_S270_LARGE_SCACHE_IDLE_TRIM_STATS"
#endif

// S218-A: large supply floor boost.
// Raise per-class floor for high-band classes after enough alloc misses.
// Purpose: reduce mmap/munmap churn when S207 admission becomes too strict.
#ifndef HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST
#define HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST 1
#endif

#ifndef HZ3_S218_SC_MIN
#define HZ3_S218_SC_MIN HZ3_S207_REUSE_SC_MIN
#endif

#ifndef HZ3_S218_MISS_TRIGGER
#define HZ3_S218_MISS_TRIGGER 4096u
#endif

#ifndef HZ3_S218_FLOOR_NODES
#define HZ3_S218_FLOOR_NODES 128
#endif

#if HZ3_S218_SC_MIN < 0
#error "HZ3_S218_SC_MIN must be >= 0"
#endif

#if HZ3_S218_MISS_TRIGGER < 1
#error "HZ3_S218_MISS_TRIGGER must be >= 1"
#endif

#if HZ3_S218_FLOOR_NODES < 1
#error "HZ3_S218_FLOOR_NODES must be >= 1"
#endif

// S218-C1: LargeBatchMmapBox (hz3 large miss-path supply batch)
// Acquire multiple same-class blocks on cache miss and seed class cache.
#ifndef HZ3_S218_C1_LARGE_BATCH_MMAP
#define HZ3_S218_C1_LARGE_BATCH_MMAP 0
#endif

#ifndef HZ3_S218_C1_SC_MIN
#define HZ3_S218_C1_SC_MIN 8
#endif

#ifndef HZ3_S218_C1_SC_MAX
#define HZ3_S218_C1_SC_MAX 15
#endif

#ifndef HZ3_S218_C1_BATCH_BLOCKS
#define HZ3_S218_C1_BATCH_BLOCKS 4
#endif

#ifndef HZ3_S218_C1_SEED_CAP_BYTES
#define HZ3_S218_C1_SEED_CAP_BYTES (512ULL << 20)
#endif

#ifndef HZ3_S218_C1_MAX_CACHE_PER_BATCH
#define HZ3_S218_C1_MAX_CACHE_PER_BATCH 1
#endif

#if HZ3_S218_C1_SC_MIN < 0
#error "HZ3_S218_C1_SC_MIN must be >= 0"
#endif

#if HZ3_S218_C1_SC_MAX < HZ3_S218_C1_SC_MIN
#error "HZ3_S218_C1_SC_MAX must be >= HZ3_S218_C1_SC_MIN"
#endif

#if HZ3_S218_C1_BATCH_BLOCKS < 2
#error "HZ3_S218_C1_BATCH_BLOCKS must be >= 2"
#endif

#if HZ3_S218_C1_SEED_CAP_BYTES < 0
#error "HZ3_S218_C1_SEED_CAP_BYTES must be >= 0"
#endif

#if HZ3_S218_C1_MAX_CACHE_PER_BATCH < 1
#error "HZ3_S218_C1_MAX_CACHE_PER_BATCH must be >= 1"
#endif
