// hakozuna/hz3/src/hz3_retention_policy.c
//
// S55-2: RetentionPolicyBox (FROZEN) - Admission Control + Open Debt
//
// Implementation:
// - Open Debt: per-shard atomic tracking (opened - freed segments)
// - Watermark: arena_used_slots -> level (L0/L1/L2) with hysteresis
// - Boundary A: epoch tick (light level update, O(1))
// - Boundary B: segment open (tries boost + repay if HARD)

#define _GNU_SOURCE

#include "hz3_retention_policy.h"
#include "hz3_config.h"
#include "hz3_arena.h"
#include "hz3_segment.h"
#include "hz3_segment_packing.h"
#include "hz3_segment_quarantine.h"
#include "hz3_mem_budget.h"
#include "hz3_tcache.h"  // for t_hz3_cache.my_shard
#include "hz3_owner_lease.h"
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#if HZ3_S55_RETENTION_FROZEN && !HZ3_SHIM_FORWARD_ONLY

// ============================================================================
// Global State
// ============================================================================

// Current retention level (atomic, global)
static _Atomic int g_retention_level = HZ3_RETENTION_L0;

// Open debt per shard (atomic)
// - debt[owner] = recently opened segments - freed segments (min 0)
static _Atomic uint32_t g_retention_debt[HZ3_NUM_SHARDS];

// Watermark parameters (bytes)
// - Default scales with HZ3_ARENA_SIZE, but can be overridden via hz3_config.h:
//   HZ3_S55_WM_SOFT_BYTES / HZ3_S55_WM_HARD_BYTES / HZ3_S55_WM_HYST_BYTES
//
// NOTE: These are compared against `used_slots * HZ3_SEG_SIZE` (segment usage
// proxy), not RSS.

// ============================================================================
// API Implementation
// ============================================================================

void hz3_retention_tick_epoch(void) {
    // Read arena usage
    uint32_t used_slots = hz3_arena_used_slots();
    size_t used_bytes = (size_t)used_slots * HZ3_SEG_SIZE;

    // Get current level
    int old_level = atomic_load_explicit(&g_retention_level, memory_order_relaxed);

    // Watermark logic with hysteresis
    int new_level = old_level;

    if (old_level == HZ3_RETENTION_L0) {
        // L0 -> L1 or L2
        if (used_bytes >= HZ3_S55_WM_HARD_BYTES) {
            new_level = HZ3_RETENTION_L2;
        } else if (used_bytes >= HZ3_S55_WM_SOFT_BYTES) {
            new_level = HZ3_RETENTION_L1;
        }
    } else if (old_level == HZ3_RETENTION_L1) {
        // L1 -> L0 or L2
        if (used_bytes >= HZ3_S55_WM_HARD_BYTES) {
            new_level = HZ3_RETENTION_L2;
        } else if (used_bytes < HZ3_S55_WM_SOFT_BYTES - HZ3_S55_WM_HYST_BYTES) {
            new_level = HZ3_RETENTION_L0;
        }
    } else if (old_level == HZ3_RETENTION_L2) {
        // L2 -> L1 or L0
        if (used_bytes < HZ3_S55_WM_HARD_BYTES - HZ3_S55_WM_HYST_BYTES) {
            new_level = HZ3_RETENTION_L1;
        }
        if (used_bytes < HZ3_S55_WM_SOFT_BYTES - HZ3_S55_WM_HYST_BYTES) {
            new_level = HZ3_RETENTION_L0;
        }
    }

    // Update level if changed
    if (new_level != old_level) {
        atomic_store_explicit(&g_retention_level, new_level, memory_order_relaxed);

        // Log level transition (SSOT: one-shot)
        fprintf(stderr, "[HZ3_S55_FROZEN] level=%d->%d used_slots=%u used_bytes=%zu\n",
                old_level, new_level, used_slots, used_bytes);
    }
}

Hz3RetentionLevel hz3_retention_level_get(void) {
    return (Hz3RetentionLevel)atomic_load_explicit(&g_retention_level, memory_order_relaxed);
}

void hz3_retention_debt_on_open(uint8_t owner) {
    if (owner >= HZ3_NUM_SHARDS) return;
    atomic_fetch_add_explicit(&g_retention_debt[owner], 1, memory_order_relaxed);
}

void hz3_retention_debt_on_free(uint8_t owner) {
    if (owner >= HZ3_NUM_SHARDS) return;

    // Decrement with lower bound 0
    uint32_t current = atomic_load_explicit(&g_retention_debt[owner], memory_order_relaxed);
    while (current > 0) {
        if (atomic_compare_exchange_weak_explicit(
                &g_retention_debt[owner], &current, current - 1,
                memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
}

int hz3_retention_try_repay(uint8_t owner) {
    // Get current level
    Hz3RetentionLevel level = hz3_retention_level_get();
    if (level != HZ3_RETENTION_L2) {
        return 0;  // Only repay at L2
    }

    // Check debt limit
    uint32_t debt = atomic_load_explicit(&g_retention_debt[owner], memory_order_relaxed);
    uint32_t debt_limit = HZ3_S55_DEBT_LIMIT_L2;
    if (debt <= debt_limit) {
        return 0;  // Debt is within limit
    }

#if HZ3_S47_SEGMENT_QUARANTINE
    // Attempt repay: S47 compact hard (once only)
    uint32_t free_gen_before = hz3_arena_free_gen();  // Use free_gen (not used_slots)
    hz3_s47_compact_hard(owner);
    uint32_t free_gen_after = hz3_arena_free_gen();

    if (free_gen_after > free_gen_before) {
        // Repay succeeded: segment freed
        uint32_t freed = free_gen_after - free_gen_before;

        // Log throttling: max 1 per second
        static _Atomic uint64_t g_last_repay_log_ns = 0;
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        uint64_t last_ns = atomic_load_explicit(&g_last_repay_log_ns, memory_order_relaxed);

        if (now_ns - last_ns > 1000000000ULL) {  // 1 second
            if (atomic_compare_exchange_strong_explicit(
                    &g_last_repay_log_ns, &last_ns, now_ns,
                    memory_order_relaxed, memory_order_relaxed)) {
                fprintf(stderr, "[HZ3_S55_FROZEN] repay=1 freed_segments=%u debt=%u\n",
                        freed, debt);
            }
        }
        return 1;
    }
#endif

    return 0;
}

// ============================================================================
// S55-2B: Epoch Repay Implementation
// ============================================================================

// CRITICAL: Use global atomic epoch counter (not TLS)
// - TLS epoch creates per-thread time axis → epoch throttle becomes inconsistent
// - Global epoch ensures all threads share common timeline
static _Atomic uint64_t g_repay_epoch_counter = 0;

// DEPRECATED: Old TLS epoch (kept for compatibility, but not used for time)
// - Only use for local throttling if needed
HZ3_TLS uint64_t t_repay_epoch_counter_deprecated = 0;

// Log throttling: max 1 per second (same pattern as S55-2)
static _Atomic uint64_t g_last_repay_epoch_log_ns = 0;

void hz3_retention_repay_epoch(void) {
    // 1. L2 level check
    Hz3RetentionLevel level = hz3_retention_level_get();
    if (level != HZ3_RETENTION_L2) {
        return;  // Only repay at L2
    }

    // 2. Increment global epoch (atomic, all threads share)
    // CRITICAL: Increment BEFORE interval check (so epoch always advances in L2)
    uint64_t current_epoch = atomic_fetch_add_explicit(&g_repay_epoch_counter, 1, memory_order_relaxed) + 1;

    // 3. Throttle: skip if not at interval
    if ((current_epoch % HZ3_S55_REPAY_EPOCH_INTERVAL) != 0) {
        // Early return, but epoch already incremented
        return;  // Not this epoch for repay work
    }

    // 3. Progress tracking: before
    uint32_t gen_before = hz3_arena_free_gen();

    // 4. Repay operations (my_shard only, light->heavy->light order)
    uint8_t my_shard = t_hz3_cache.my_shard;
    int collided = (hz3_shard_live_count(my_shard) > 1);

    // 4.1. S47 compact hard (light: aggressive compression + drain)
#if HZ3_S47_SEGMENT_QUARANTINE
    // Collision note:
    // - Under threads>shards, OwnerLease becomes contended and long holds hurt tail latency.
    // - Prefer smaller work chunks during collision.
    if (collided) {
        hz3_s47_compact_soft(my_shard);
    } else {
        hz3_s47_compact_hard(my_shard);
    }
#endif

    // 4.2. Budgeted segment scan (heavy but O(budget) fixed cost)
#if HZ3_MEM_BUDGET_ENABLE
    // Collision note:
    // - hz3_segment_free() is heavy (madvise/arena lock), and holding OwnerLease around it
    //   raises hold_us_max under oversubscription.
    // - Skip segment-free repay work during collision. (RSS work remains available via
    //   non-collision runs or via explicit mem-budget boundaries.)
    if (!collided) {
        hz3_mem_budget_reclaim_segments_budget(HZ3_SEG_SIZE, HZ3_S55_REPAY_SCAN_SLOTS_BUDGET);
    }
#endif

    // 4.3. Medium runs reclaim (DEFERRED: 第2段に落とす、まず無し)
    // - gen_delta==0が続く場合のみ、さらに間引きをかけて実行
    // - Example: if ((t_repay_epoch_counter % (HZ3_S55_REPAY_EPOCH_INTERVAL*4))==0)
    //             hz3_mem_budget_reclaim_medium_runs_budget(256);

    // 5. Progress tracking: after
    uint32_t gen_after = hz3_arena_free_gen();
    uint32_t gen_delta = gen_after - gen_before;

    // 5.5. S55-3: Medium runs reclaim with subrelease (phase 2)
    // - Enqueue: throttled (gen_delta==0 gate optional, INTERVAL*MULT period)
#if HZ3_S55_3_MEDIUM_SUBRELEASE
    uint64_t s55_3_period = (uint64_t)HZ3_S55_REPAY_EPOCH_INTERVAL *
                            (uint64_t)HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT;
    if (s55_3_period == 0) {
        s55_3_period = 1;
    }

    // Enqueue: throttled by gen_delta gate (optional) and period
    // - Use current_epoch (global atomic) not TLS
#if HZ3_S55_3_REQUIRE_GEN_DELTA0
    if (gen_delta == 0 &&
        (current_epoch % s55_3_period) == 0) {
#else
    if ((current_epoch % s55_3_period) == 0) {
#endif
        // S55-3: Immediate madvise (subrelease)
        // Collision note:
        // - This path can be syscall-heavy; keep it off during collision to avoid
        //   exacerbating lease contention.
        if (!collided) {
            hz3_mem_budget_reclaim_medium_runs_subrelease(HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES);
        }
    }
#endif

    // 6. Log epoch progress (throttled: max 1 per second)
    // CRITICAL: Always log (even when gen_delta==0) to show epoch is alive
    // - gen_delta==0 is expected when S47/segment reclaim has no work
    // - Without this log, operators may incorrectly think epoch stopped
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    uint64_t last_ns = atomic_load_explicit(&g_last_repay_epoch_log_ns, memory_order_relaxed);

    if (now_ns - last_ns > 1000000000ULL) {  // 1 second
        if (atomic_compare_exchange_strong_explicit(
                &g_last_repay_epoch_log_ns, &last_ns, now_ns,
                memory_order_relaxed, memory_order_relaxed)) {
            fprintf(stderr, "[HZ3_S55_REPAY_EPOCH] gen_delta=%u\n", gen_delta);
        }
    }

    // No retry: immediate exit regardless of progress
}

#endif  // HZ3_S55_RETENTION_FROZEN && !HZ3_SHIM_FORWARD_ONLY
