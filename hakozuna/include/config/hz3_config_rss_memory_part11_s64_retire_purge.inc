// S64: Epoch-based Retire/Purge (mimalloc-like purge_delay)
// ============================================================================
// Steady-state RSS reduction via epoch-based retire + delayed purge.
// Unlike S62 (thread-exit only), S64 runs on every epoch tick.

// S64-A: RetireScanBox - scan central for empty pages
#ifndef HZ3_S64_RETIRE_SCAN
#define HZ3_S64_RETIRE_SCAN 0
#endif

// S64-B: PurgeDelayBox - delayed madvise(DONTNEED)
#ifndef HZ3_S64_PURGE_DELAY
#define HZ3_S64_PURGE_DELAY 0
#endif

// Budget: max objects to process per epoch tick
#ifndef HZ3_S64_RETIRE_BUDGET_OBJS
#define HZ3_S64_RETIRE_BUDGET_OBJS 512
#endif

// Delay: epochs to wait before purging retired pages
#ifndef HZ3_S64_PURGE_DELAY_EPOCHS
#define HZ3_S64_PURGE_DELAY_EPOCHS 4
#endif

// Delay (ms): when >0, use monotonic ms delay instead of epoch-based delay
#ifndef HZ3_S64_PURGE_DELAY_MS
#define HZ3_S64_PURGE_DELAY_MS 5
#endif

// Budget: max pages to purge per epoch tick
#ifndef HZ3_S64_PURGE_BUDGET_PAGES
#define HZ3_S64_PURGE_BUDGET_PAGES 64
#endif

// Budget: max madvise calls per epoch tick
#ifndef HZ3_S64_PURGE_MAX_CALLS
#define HZ3_S64_PURGE_MAX_CALLS 16
#endif

// Purge queue: coalesce adjacent ranges at enqueue (reduces madvise calls)
#ifndef HZ3_S64_PURGE_COALESCE
#define HZ3_S64_PURGE_COALESCE 0
#endif

// Purge: re-check free_bits before madvise (skip if reused)
#ifndef HZ3_S64_PURGE_CHECK_FREE
#define HZ3_S64_PURGE_CHECK_FREE 0
#endif

// Immediate purge when delay is zero (debug/experiment)
#ifndef HZ3_S64_PURGE_IMMEDIATE_ON_ZERO
#define HZ3_S64_PURGE_IMMEDIATE_ON_ZERO 0
#endif

// Ring buffer size for purge queue (per-shard, power of 2)
#ifndef HZ3_S64_PURGE_QUEUE_SIZE
#define HZ3_S64_PURGE_QUEUE_SIZE 256
#endif

// Stats: atexit one-shot dump
#ifndef HZ3_S64_STATS
#define HZ3_S64_STATS 0
#endif

// S64-P2: Force purge on queue full (watermark-based emergency purge)
// When queue reaches watermark, bypass delay_epochs and purge immediately
#ifndef HZ3_S64_PURGE_FORCE_ON_FULL
#define HZ3_S64_PURGE_FORCE_ON_FULL 0
#endif

// Watermark threshold (queue length trigger, 0-QUEUE_SIZE)
// Recommended: 75% of QUEUE_SIZE (192 for 256-entry queue)
#ifndef HZ3_S64_PURGE_FORCE_WATERMARK
#define HZ3_S64_PURGE_FORCE_WATERMARK 192
#endif

// Forced purge budget: max madvise calls per trigger
#ifndef HZ3_S64_PURGE_FORCE_MAX_CALLS
#define HZ3_S64_PURGE_FORCE_MAX_CALLS 8
#endif

// Forced purge budget: max pages to purge per trigger
#ifndef HZ3_S64_PURGE_FORCE_MAX_PAGES
#define HZ3_S64_PURGE_FORCE_MAX_PAGES 64
#endif

// S64-1: TCacheDumpBox - supply objects to central for retire
// S71: Auto-enable with retire scan (tcache must dump before retire can collect)
#ifndef HZ3_S64_TCACHE_DUMP
// Collision note:
// - When shard collision is tolerated (HZ3_SHARD_COLLISION_FAILFAST=0), multiple
//   threads can share a shard. We observed mstress memory corruption in that mode
//   with S64 tcache dumping enabled.
// - Default to OFF in collision-tolerant builds; callers can explicitly override
//   with -DHZ3_S64_TCACHE_DUMP=1 for experiments.
#if !HZ3_SHARD_COLLISION_FAILFAST
#define HZ3_S64_TCACHE_DUMP 0
#else
#define HZ3_S64_TCACHE_DUMP HZ3_S64_RETIRE_SCAN
#endif
#endif

// S64-P3: TCacheDump only under pressure (skip dump when no pressure)
#ifndef HZ3_S64_TCACHE_DUMP_PRESSURE_ONLY
#define HZ3_S64_TCACHE_DUMP_PRESSURE_ONLY 0
#endif

// S64-L: TCacheDump Learner - auto switch mode based on pressure signal
#ifndef HZ3_S64_TDUMP_LEARNER
#define HZ3_S64_TDUMP_LEARNER 0
#endif

#ifndef HZ3_S64_TDUMP_BUDGET_OBJS
#define HZ3_S64_TDUMP_BUDGET_OBJS 1024
#endif

// S64-P4: Purge pump on enqueue - purge ready entries opportunistically
#ifndef HZ3_S64_PURGE_PUMP_ON_ENQ
#define HZ3_S64_PURGE_PUMP_ON_ENQ 0
#endif

// Max madvise calls per enqueue (keep tiny)
#ifndef HZ3_S64_PURGE_PUMP_MAX_CALLS
#define HZ3_S64_PURGE_PUMP_MAX_CALLS 1
#endif

// Max pages per enqueue
#ifndef HZ3_S64_PURGE_PUMP_BUDGET_PAGES
#define HZ3_S64_PURGE_PUMP_BUDGET_PAGES 64
#endif

// S64-P5: Drain pending before retire - drain xfer/stash to create retire window
#ifndef HZ3_S64_RETIRE_DRAIN_PENDING
#define HZ3_S64_RETIRE_DRAIN_PENDING 0
#endif

// S64-P5b: after draining pending, skip retire_scan for this SC in this tick
#ifndef HZ3_S64_RETIRE_DRAIN_DEFER
#define HZ3_S64_RETIRE_DRAIN_DEFER 0
#endif

// Limit number of SCs drained per tick (avoid budget dilution)
#ifndef HZ3_S64_RETIRE_DRAIN_MAX_SC_PER_TICK
#define HZ3_S64_RETIRE_DRAIN_MAX_SC_PER_TICK 1
#endif

// Max objects to drain from xfer per SC (when pending detected) - P5b: reduced from 256 to 64
#ifndef HZ3_S64_RETIRE_DRAIN_XFER_BUDGET
#define HZ3_S64_RETIRE_DRAIN_XFER_BUDGET 64
#endif

// Max objects to drain from stash per SC (when pending detected) - P5b: reduced from 256 to 64
#ifndef HZ3_S64_RETIRE_DRAIN_STASH_BUDGET
#define HZ3_S64_RETIRE_DRAIN_STASH_BUDGET 64
#endif

// ============================================================================
