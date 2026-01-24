// hakozuna/hz3/include/hz3_retention_policy.h
//
// S55-2: RetentionPolicyBox (FROZEN) - Admission Control + Open Debt
//
// Purpose:
// - S55-1 OBSERVE showed arena_used_bytes dominates RSS (97-99%)
// - Reduce segment "open frequency" by admission control
// - Hot path: zero cost (event-only at boundaries)
//
// Design:
// - Open Debt: per-shard tracking of "recently opened - freed segments"
// - Watermark: arena_used_slots -> level (L0/L1/L2)
// - Boundary A: hz3_epoch_force() -> light level update
// - Boundary B: segment open -> tries boost + repay (heavy, once only)

#ifndef HZ3_RETENTION_POLICY_H
#define HZ3_RETENTION_POLICY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Retention level (global arena state)
typedef enum {
    HZ3_RETENTION_L0 = 0,  // Normal (no pressure)
    HZ3_RETENTION_L1 = 1,  // SOFT (increase pack tries)
    HZ3_RETENTION_L2 = 2,  // HARD (repay before open)
} Hz3RetentionLevel;

// ============================================================================
// API (minimal)
// ============================================================================

// Called at epoch boundary (hz3_epoch_force)
// - Reads arena_used_slots
// - Updates global level (L0/L1/L2) based on watermarks
// - O(1), no heavy reclaim here
void hz3_retention_tick_epoch(void);

// Get current retention level (L0/L1/L2)
// - Thread-safe read (atomic load)
Hz3RetentionLevel hz3_retention_level_get(void);

// Notify: segment opened (debt++)
// - owner: segment owner shard
void hz3_retention_debt_on_open(uint8_t owner);

// Notify: segment freed (debt--, min 0)
// - owner: segment owner shard
void hz3_retention_debt_on_free(uint8_t owner);

// Try repay at L2 (if debt exceeds limit)
// - owner: target shard
// - Returns: 1 if repay succeeded (segment freed), 0 otherwise
int hz3_retention_try_repay(uint8_t owner);

// ============================================================================
// S55-2B: Epoch Repay API
// ============================================================================

// Repay epoch: periodic segment return at L2
// - Called from hz3_epoch_force() after hz3_retention_tick_epoch()
// - Throttled by HZ3_S55_REPAY_EPOCH_INTERVAL (epoch counter)
// - Operates on my_shard only (thread-safe)
// - Progress gated by hz3_arena_free_gen() delta
void hz3_retention_repay_epoch(void);

#ifdef __cplusplus
}
#endif

#endif  // HZ3_RETENTION_POLICY_H
