// hz3_config_rss_memory.h - RSS / Memory pressure configuration (S45-S61)
// Part of hz3_config.h split for maintainability
#pragma once

// ============================================================================
// S45: Memory Budget Box (scale lane arena exhaustion recovery)
// ============================================================================

#ifndef HZ3_MEM_BUDGET_ENABLE
#define HZ3_MEM_BUDGET_ENABLE 0
#endif

#ifndef HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES
#define HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES 4096
#endif

// ============================================================================
// S50: Large Cache Size-Class Box
// ============================================================================

#ifndef HZ3_S50_LARGE_SCACHE
#define HZ3_S50_LARGE_SCACHE 1
#endif

#ifndef HZ3_S50_LARGE_SCACHE_EVICT
#define HZ3_S50_LARGE_SCACHE_EVICT 0
#endif

// S51: madvise(MADV_DONTNEED) on cache push
#ifndef HZ3_S51_LARGE_MADVISE
#define HZ3_S51_LARGE_MADVISE 0
#endif

// S52: Best-fit fallback for large cache
#ifndef HZ3_S52_LARGE_BESTFIT
#define HZ3_S52_LARGE_BESTFIT 0
#endif

#ifndef HZ3_S52_BESTFIT_RANGE
#define HZ3_S52_BESTFIT_RANGE 2
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
// S55-2: RetentionPolicyBox FROZEN mode (Admission Control + Open Debt)
// ============================================================================

#ifndef HZ3_S55_RETENTION_FROZEN
#define HZ3_S55_RETENTION_FROZEN 0
#endif

// S55-2: Watermark tuning
#ifndef HZ3_S55_WM_SOFT_BYTES
#define HZ3_S55_WM_SOFT_BYTES (HZ3_ARENA_SIZE / 4)
#endif
#ifndef HZ3_S55_WM_HARD_BYTES
#define HZ3_S55_WM_HARD_BYTES (HZ3_ARENA_SIZE * 35 / 100)
#endif
#ifndef HZ3_S55_WM_HYST_BYTES
#define HZ3_S55_WM_HYST_BYTES (HZ3_ARENA_SIZE / 50)
#endif

// S55-2: Debt/tries tuning
#ifndef HZ3_S55_DEBT_LIMIT_L1
#define HZ3_S55_DEBT_LIMIT_L1 8
#endif
#ifndef HZ3_S55_DEBT_LIMIT_L2
#define HZ3_S55_DEBT_LIMIT_L2 2
#endif
#ifndef HZ3_S55_PACK_TRIES_L1
#define HZ3_S55_PACK_TRIES_L1 16
#endif
#ifndef HZ3_S55_PACK_TRIES_L2
#define HZ3_S55_PACK_TRIES_L2 64
#endif

// ============================================================================
// S55-2B: ReturnPolicyBox - Epoch Repay
// ============================================================================

#ifndef HZ3_S55_REPAY_EPOCH_INTERVAL
#define HZ3_S55_REPAY_EPOCH_INTERVAL 16
#endif

#ifndef HZ3_S55_REPAY_SCAN_SLOTS_BUDGET
#define HZ3_S55_REPAY_SCAN_SLOTS_BUDGET 256
#endif

// ============================================================================
// S55-3: MediumRunSubreleaseBox
// ============================================================================

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE
#define HZ3_S55_3_MEDIUM_SUBRELEASE 0
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES
#define HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES 256
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST
#define HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST 0
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT
#define HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT 1
#endif

#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS
#define HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS 32
#endif

#ifndef HZ3_S55_3_REQUIRE_GEN_DELTA0
#define HZ3_S55_3_REQUIRE_GEN_DELTA0 1
#endif

// ============================================================================
// S45-FOCUS: Focused reclaim for arena exhaustion recovery
// ============================================================================

#ifndef HZ3_S45_FOCUS_RECLAIM
#define HZ3_S45_FOCUS_RECLAIM 0
#endif

#ifndef HZ3_S45_FOCUS_PASSES
#define HZ3_S45_FOCUS_PASSES 4
#endif

#ifndef HZ3_S45_EMERGENCY_FLUSH_REMOTE
#define HZ3_S45_EMERGENCY_FLUSH_REMOTE 0
#endif

#define HZ3_FOCUS_MAX_OBJS 1024
#define HZ3_FOCUS_MAX_SEGS 32

// ============================================================================
// S46: Global Pressure Box (arena exhaustion broadcast)
// ============================================================================

#ifndef HZ3_ARENA_PRESSURE_BOX
#define HZ3_ARENA_PRESSURE_BOX 0
#endif

#ifndef HZ3_OWNER_STASH_FLUSH_BUDGET
#define HZ3_OWNER_STASH_FLUSH_BUDGET 256
#endif

// ============================================================================
// S47: Segment Quarantine (deterministic drain)
// ============================================================================

#ifndef HZ3_S47_SEGMENT_QUARANTINE
#define HZ3_S47_SEGMENT_QUARANTINE 0
#endif

#ifndef HZ3_S47_HEADROOM_SLOTS
#define HZ3_S47_HEADROOM_SLOTS 256
#endif

#ifndef HZ3_S47_SCAN_BUDGET_SOFT
#define HZ3_S47_SCAN_BUDGET_SOFT 512
#endif

#ifndef HZ3_S47_SCAN_BUDGET_HARD
#define HZ3_S47_SCAN_BUDGET_HARD 4096
#endif

#ifndef HZ3_S47_QUARANTINE_MAX_EPOCHS
#define HZ3_S47_QUARANTINE_MAX_EPOCHS 8
#endif

#ifndef HZ3_S47_DRAIN_PASSES_SOFT
#define HZ3_S47_DRAIN_PASSES_SOFT 1
#endif

#ifndef HZ3_S47_DRAIN_PASSES_HARD
#define HZ3_S47_DRAIN_PASSES_HARD 4
#endif

#ifndef HZ3_S47_POLICY_MODE
#define HZ3_S47_POLICY_MODE 0
#endif

#ifndef HZ3_S47_PANIC_WAIT_US
#define HZ3_S47_PANIC_WAIT_US 100
#endif

// S47-2: ArenaGateBox
#ifndef HZ3_S47_ARENA_GATE
#if HZ3_S47_SEGMENT_QUARANTINE
#define HZ3_S47_ARENA_GATE 1
#else
#define HZ3_S47_ARENA_GATE 0
#endif
#endif

#ifndef HZ3_S47_GATE_WAIT_LOOPS
#define HZ3_S47_GATE_WAIT_LOOPS 128
#endif

// S47-3: Pinned segment avoidance
#ifndef HZ3_S47_AVOID_ENABLE
#define HZ3_S47_AVOID_ENABLE 1
#endif

#ifndef HZ3_S47_AVOID_SLOTS
#define HZ3_S47_AVOID_SLOTS 4
#endif

#ifndef HZ3_S47_AVOID_TTL
#define HZ3_S47_AVOID_TTL 8
#endif

#ifndef HZ3_S47_FREEPAGES_WEIGHT
#define HZ3_S47_FREEPAGES_WEIGHT 2
#endif

#ifndef HZ3_S47_SOFT_SKIP_HOPELESS
#define HZ3_S47_SOFT_SKIP_HOPELESS 1
#endif

// ============================================================================
// S49: Segment Packing (existing segment priority)
// ============================================================================

#ifndef HZ3_S49_SEGMENT_PACKING
#define HZ3_S49_SEGMENT_PACKING 0
#endif

#if HZ3_S49_SEGMENT_PACKING
#ifndef HZ3_S49_PACK_PRESSURE_ONLY
#define HZ3_S49_PACK_PRESSURE_ONLY 0
#endif
#ifndef HZ3_S49_PACK_MAX_PAGES
#define HZ3_S49_PACK_MAX_PAGES 8
#endif
#ifndef HZ3_S49_PACK_TRIES_SOFT
#define HZ3_S49_PACK_TRIES_SOFT 4
#endif
#ifndef HZ3_S49_PACK_TRIES_HARD
#define HZ3_S49_PACK_TRIES_HARD 16
#endif
#ifndef HZ3_S49_PACK_TRIES_PANIC
#define HZ3_S49_PACK_TRIES_PANIC 64
#endif
#ifndef HZ3_S49_PACK_TRIES_PER_BUCKET
#define HZ3_S49_PACK_TRIES_PER_BUCKET 2
#endif
#ifndef HZ3_S49_PACK_STATS
#define HZ3_S49_PACK_STATS 0
#endif
#ifndef HZ3_S49_PACK_RETRIES
#define HZ3_S49_PACK_RETRIES 3
#endif
#endif

// ============================================================================
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

// Budget: max pages to purge per epoch tick
#ifndef HZ3_S64_PURGE_BUDGET_PAGES
#define HZ3_S64_PURGE_BUDGET_PAGES 64
#endif

// Budget: max madvise calls per epoch tick
#ifndef HZ3_S64_PURGE_MAX_CALLS
#define HZ3_S64_PURGE_MAX_CALLS 16
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

#ifndef HZ3_S64_TDUMP_BUDGET_OBJS
#define HZ3_S64_TDUMP_BUDGET_OBJS 1024
#endif

// ============================================================================
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
