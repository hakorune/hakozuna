// hz3_config_core.h - Core enables, shim, debug
// Part of hz3_config.h split for maintainability
#pragma once

// ============================================================================
// Core Configuration
// ============================================================================
//
// Configuration layers (SSOT):
//   1) Header defaults (this file): conservative, safe in any build context.
//   2) Build profile (hakozuna/hz3/Makefile): hz3 lane "known-good" defaults.
//   3) A/B overrides: pass diffs via HZ3_LDPRELOAD_DEFS_EXTRA (never replace defs).

// When 0, the shim forwards to the next allocator (RTLD_NEXT) and does not
// change behavior.
#ifndef HZ3_ENABLE
#define HZ3_ENABLE 0
#endif

// Safety: for early bring-up, prefer forwarding unless explicitly enabled.
#ifndef HZ3_SHIM_FORWARD_ONLY
#define HZ3_SHIM_FORWARD_ONLY 1
#endif

// Learning layer is event-only by design (no hot-path reads of global knobs).
#ifndef HZ3_LEARN_ENABLE
#define HZ3_LEARN_ENABLE 0
#endif

// Guardrails (debug-only). Enables extra range checks / build-time asserts.
#ifndef HZ3_GUARDRAILS
#define HZ3_GUARDRAILS 0
#endif

// OOM observability / fail-fast (init/slow path only)
#ifndef HZ3_OOM_SHOT
#define HZ3_OOM_SHOT 0
#endif

#ifndef HZ3_OOM_FAILFAST
#define HZ3_OOM_FAILFAST 0
#endif

// ============================================================================
// S85: small_v2 slow path breakdown stats (event-only, atexit one-shot)
// ============================================================================
//
// Purpose:
// - Break down hz3_small_v2_alloc_slow() into xfer/stash/central/page.
// - Find the next optimization boundary after S44 (owner stash pop).
//
#ifndef HZ3_S85_SMALL_V2_SLOW_STATS
#define HZ3_S85_SMALL_V2_SLOW_STATS 0
#endif

// ============================================================================
// Debug: WatchPtrBox (one-pointer trace for corruption triage)
// ============================================================================
#ifndef HZ3_WATCH_PTR_BOX
#define HZ3_WATCH_PTR_BOX 0
#endif

#ifndef HZ3_WATCH_PTR_FAILFAST
#define HZ3_WATCH_PTR_FAILFAST 0
#endif

#ifndef HZ3_WATCH_PTR_SHOT
#define HZ3_WATCH_PTR_SHOT 1
#endif

// ============================================================================
// Shard assignment / collision observability (init-only)
// ============================================================================
//
// Shard id is stored into PTAG (dst) and used as the "owner" key for inbox/central.
// When threads > HZ3_NUM_SHARDS, shard ids collide by design (multiple threads share
// an owner id). This is typically safe but may affect scalability/locality.
//
// Keep these as compile-time flags (hz3 policy: no runtime env knobs).

// One-shot warning when a shard collision is detected at TLS init.
#ifndef HZ3_SHARD_COLLISION_SHOT
#define HZ3_SHARD_COLLISION_SHOT 0
#endif

// Fail-fast (abort) when a shard collision is detected at TLS init.
#ifndef HZ3_SHARD_COLLISION_FAILFAST
#define HZ3_SHARD_COLLISION_FAILFAST 0
#endif

// ============================================================================
// LaneSplit / OwnerLease (collision-tolerant owner-only boundaries)
// ============================================================================
// LaneSplit: lane-local state (future) with lane0 embedded in shard core.
#ifndef HZ3_LANE_SPLIT
#define HZ3_LANE_SPLIT 0
#endif

// OwnerLease: event-only lease for owner-only operations.
#ifndef HZ3_OWNER_LEASE_ENABLE
#define HZ3_OWNER_LEASE_ENABLE 0
#endif

#ifndef HZ3_OWNER_LEASE_TTL_US
#define HZ3_OWNER_LEASE_TTL_US 500
#endif

#ifndef HZ3_OWNER_LEASE_OPS_BUDGET
#define HZ3_OWNER_LEASE_OPS_BUDGET 1
#endif

#ifndef HZ3_OWNER_LEASE_STATS
#define HZ3_OWNER_LEASE_STATS HZ3_OWNER_LEASE_ENABLE
#endif
