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
