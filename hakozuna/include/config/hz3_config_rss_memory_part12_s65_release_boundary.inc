// S65: Always-On Release (Boundary + Ledger + Coverage)
// ============================================================================
//
// - ReleaseBoundaryBox: Single entry point for free_bits updates
// - ReleaseLedgerBox: Range coalesce + deferred purge
// - CoverageBox: Unify small/sub4k/medium retire paths
// - Idle/BusyGate: Adjust purge aggressiveness based on pressure
//
// When HZ3_S65_RELEASE_BOUNDARY=1, ALL retire paths must go through
// hz3_release_range_definitely_free() - no direct hz3_bitmap_mark_free() calls.

// Master enable for ReleaseBoundaryBox
#ifndef HZ3_S65_RELEASE_BOUNDARY
#define HZ3_S65_RELEASE_BOUNDARY 0
#endif

// Enable ReleaseLedgerBox (requires BOUNDARY)
#ifndef HZ3_S65_RELEASE_LEDGER
#define HZ3_S65_RELEASE_LEDGER 0
#endif

// Enable Coverage expansion (small/sub4k/medium unified retire)
#ifndef HZ3_S65_RELEASE_COVERAGE
#define HZ3_S65_RELEASE_COVERAGE 0
#endif

// Enable Idle/Busy Gate (adjust delay/budget based on pressure)
#ifndef HZ3_S65_IDLE_GATE
#define HZ3_S65_IDLE_GATE 0
#endif

// Ledger ring buffer size (power of 2)
#ifndef HZ3_S65_LEDGER_SIZE
#define HZ3_S65_LEDGER_SIZE 256
#endif

// Max pages to purge per epoch tick
#ifndef HZ3_S65_PURGE_BUDGET_PAGES
#define HZ3_S65_PURGE_BUDGET_PAGES 512
#endif

// Max madvise calls per epoch tick
#ifndef HZ3_S65_PURGE_MAX_CALLS
#define HZ3_S65_PURGE_MAX_CALLS 64
#endif

// Delay epochs before purge (idle mode)
#ifndef HZ3_S65_DELAY_EPOCHS_IDLE
#define HZ3_S65_DELAY_EPOCHS_IDLE 0
#endif

// Delay epochs before purge (busy mode)
#ifndef HZ3_S65_DELAY_EPOCHS_BUSY
#define HZ3_S65_DELAY_EPOCHS_BUSY 4
#endif

// Stats: atexit one-shot dump
#ifndef HZ3_S65_STATS
#define HZ3_S65_STATS 0
#endif

// Coalesce scan depth (how many recent entries to check for merge)
#ifndef HZ3_S65_COALESCE_SCAN_DEPTH
#define HZ3_S65_COALESCE_SCAN_DEPTH 8
#endif
// S65: Validate pages are still free before madvise (prevents purge of live pages)
#ifndef HZ3_S65_LEDGER_VALIDATE_FREE
#define HZ3_S65_LEDGER_VALIDATE_FREE 1
#endif

// S65-2C: Medium epoch reclaim (独立フラグ)
// central から run を pop して boundary API 経由で free_bits を更新
// HZ3_S65_RELEASE_BOUNDARY=1 が前提
#ifndef HZ3_S65_MEDIUM_RECLAIM
#define HZ3_S65_MEDIUM_RECLAIM 0
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_MODE
// 0=auto (purge_only for now), 1=release to seg free_bits (original, repro only), 2=purge_only (madvise + return to central)
#define HZ3_S65_MEDIUM_RECLAIM_MODE 0
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS
#define HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS 512
#endif

// S194: MediumDemandReclaimBox (opt-in)
// Trigger S65 medium reclaim from central-miss boundary instead of only epoch.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND 0
#endif

// Demand trigger range (size-class scope)
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN 5
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX 7
#endif

// Run demand reclaim once per N central-miss events.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE 8
#endif

// Budget cap for demand reclaim (runs per trigger).
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS 64
#endif

// Optional scale by refill want: budget=max(base, want*scale). 0=disabled.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_SCALE
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_SCALE 0
#endif

// Guard: skip demand reclaim while remote-hint is active.
#ifndef HZ3_S65_MEDIUM_RECLAIM_DEMAND_SKIP_REMOTE_HINT
#define HZ3_S65_MEDIUM_RECLAIM_DEMAND_SKIP_REMOTE_HINT 1
#endif

#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN > HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX
#error "HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MIN must be <= HZ3_S65_MEDIUM_RECLAIM_DEMAND_SC_MAX"
#endif

#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE < 1
#error "HZ3_S65_MEDIUM_RECLAIM_DEMAND_MISS_STRIDE must be >= 1"
#endif

#if HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS < 1
#error "HZ3_S65_MEDIUM_RECLAIM_DEMAND_BUDGET_RUNS must be >= 1"
#endif

// S65-2C: Guard release mode when shards collide (multiple threads share shard id).
#ifndef HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD
#define HZ3_S65_MEDIUM_RECLAIM_COLLISION_GUARD 1
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_COLLISION_FAILFAST
#define HZ3_S65_MEDIUM_RECLAIM_COLLISION_FAILFAST 0
#endif

#ifndef HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT
#define HZ3_S65_MEDIUM_RECLAIM_COLLISION_SHOT 1
#endif

// S80: Medium reclaim runtime gate (env: HZ3_S80_MEDIUM_RECLAIM=0/1)
#ifndef HZ3_S80_MEDIUM_RECLAIM_GATE
#define HZ3_S80_MEDIUM_RECLAIM_GATE 0
#endif

#ifndef HZ3_S80_MEDIUM_RECLAIM_DEFAULT
#define HZ3_S80_MEDIUM_RECLAIM_DEFAULT 1
#endif

#ifndef HZ3_S80_MEDIUM_RECLAIM_LOG
#define HZ3_S80_MEDIUM_RECLAIM_LOG 1
#endif

#ifndef HZ3_OS_PURGE_STATS
#define HZ3_OS_PURGE_STATS 0
#endif