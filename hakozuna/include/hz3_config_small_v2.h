// hz3_config_small_v2.h - Small v2 / PTAG / Sub4k configuration
// Part of hz3_config.h split for maintainability
#pragma once

// ============================================================================
// Small v2 (self-describing) configuration
// ============================================================================

// Small v2 (self-describing) gate
#ifndef HZ3_SMALL_V2_ENABLE
#define HZ3_SMALL_V2_ENABLE 0
#endif

// Small v1 gate (legacy path). Default OFF when v2 is available.
#ifndef HZ3_SMALL_V1_ENABLE
#define HZ3_SMALL_V1_ENABLE 0
#endif

// S23: sub-4KB classes (2049..4095) to avoid 4KB rounding.
#ifndef HZ3_SUB4K_ENABLE
#define HZ3_SUB4K_ENABLE 0
#endif

#ifndef HZ3_SUB4K_NUM_SC
#define HZ3_SUB4K_NUM_SC 4
#endif

// ============================================================================
// S138: SmallMaxSize Gate A/B
// ============================================================================
// Purpose: SC 127 tail wasteを削減してRSS改善をテスト
// Context: S135-1Cで SC 127 が最大waste contributor（2032B/page）と判明
// Trade-off: 1025-2048B allocations は Sub4k へ fallback（遅いがRSS優先）
//
// Usage:
//   make -C hakozuna/hz3 clean all_ldpreload_scale  # baseline (2048)
//   make -C hakozuna/hz3 all_ldpreload_scale_s138_1024  # treat (1024)

#ifndef HZ3_SMALL_MAX_SIZE_OVERRIDE
#define HZ3_SMALL_MAX_SIZE_OVERRIDE 0
#endif

// Dependency checks
#if HZ3_SMALL_MAX_SIZE_OVERRIDE != 0
  #if HZ3_SMALL_MAX_SIZE_OVERRIDE < 512
    #error "HZ3_SMALL_MAX_SIZE_OVERRIDE must be >= 512 (minimum viable small range)"
  #endif
  #if HZ3_SMALL_MAX_SIZE_OVERRIDE > 2048
    #error "HZ3_SMALL_MAX_SIZE_OVERRIDE must be <= 2048 (baseline max)"
  #endif
  #if (HZ3_SMALL_MAX_SIZE_OVERRIDE % 16) != 0
    #error "HZ3_SMALL_MAX_SIZE_OVERRIDE must be multiple of 16 (HZ3_SMALL_ALIGN)"
  #endif
#endif

// NOTE: S140 PTAG32 leaf micro-opts are NO-GO and archived in hz3_config_archive.h.

// Self-describing segment header gate
#ifndef HZ3_SEG_SELF_DESC_ENABLE
#define HZ3_SEG_SELF_DESC_ENABLE 0
#endif

// Debug-only fail-fast for self-describing segments
#ifndef HZ3_SMALL_V2_FAILFAST
#define HZ3_SMALL_V2_FAILFAST 0
#endif

// ============================================================================
// S118: Small v2 RefillBatch tuning (alloc_slow refill size)
// ============================================================================
//
// Motivation:
// - `hz3_small_v2_alloc_slow()` refills a local bin via xfer/owner-stash/central.
// - Larger refill batches can reduce refill frequency (and thus pop_batch calls),
//   at the cost of more work per refill and potentially higher cache pressure.
//
// Default:
// - Keep `HZ3_SMALL_V2_REFILL_BATCH=32` as baseline.
// - Use `HZ3_S118_SMALL_V2_REFILL_BATCH` for research A/B (0 = disabled).

#ifndef HZ3_SMALL_V2_REFILL_BATCH
#define HZ3_SMALL_V2_REFILL_BATCH 32
#endif

#ifndef HZ3_S118_SMALL_V2_REFILL_BATCH
#define HZ3_S118_SMALL_V2_REFILL_BATCH 0
#endif

#if HZ3_S118_SMALL_V2_REFILL_BATCH
  #if (HZ3_S118_SMALL_V2_REFILL_BATCH < 8) || (HZ3_S118_SMALL_V2_REFILL_BATCH > 128)
    #error "HZ3_S118_SMALL_V2_REFILL_BATCH out of supported range (8..128)"
  #endif
  #undef HZ3_SMALL_V2_REFILL_BATCH
  #define HZ3_SMALL_V2_REFILL_BATCH HZ3_S118_SMALL_V2_REFILL_BATCH
#endif

// S72: Boundary fail-fast / one-shot (small_v2 list/object validation)
#ifndef HZ3_S72_BOUNDARY_DEBUG
#define HZ3_S72_BOUNDARY_DEBUG 0
#endif

#ifndef HZ3_S72_BOUNDARY_FAILFAST
#define HZ3_S72_BOUNDARY_FAILFAST HZ3_S72_BOUNDARY_DEBUG
#endif

#ifndef HZ3_S72_BOUNDARY_SHOT
#define HZ3_S72_BOUNDARY_SHOT 1
#endif

#ifndef HZ3_S72_BOUNDARY_MAX_WALK
#define HZ3_S72_BOUNDARY_MAX_WALK 64
#endif

// S72-M: Medium boundary debug (PTAG-based, no deref)
#ifndef HZ3_S72_MEDIUM_DEBUG
#define HZ3_S72_MEDIUM_DEBUG 0
#endif

#ifndef HZ3_S72_MEDIUM_FAILFAST
#define HZ3_S72_MEDIUM_FAILFAST HZ3_S72_MEDIUM_DEBUG
#endif

#ifndef HZ3_S72_MEDIUM_SHOT
#define HZ3_S72_MEDIUM_SHOT 1
#endif

// S90: Central push guard (medium) - verify (bin,dst) matches (sc,shard) at push boundary.
// Debug-only: catches cross-sc contamination earlier than alloc-pop mismatch.
#ifndef HZ3_S90_CENTRAL_GUARD
#define HZ3_S90_CENTRAL_GUARD 0
#endif

#ifndef HZ3_S90_CENTRAL_GUARD_FAILFAST
#define HZ3_S90_CENTRAL_GUARD_FAILFAST HZ3_S90_CENTRAL_GUARD
#endif

#ifndef HZ3_S90_CENTRAL_GUARD_SHOT
#define HZ3_S90_CENTRAL_GUARD_SHOT 1
#endif

// S78: PTAG32 guard (bin vs segmap mismatch)
#ifndef HZ3_S78_PTAG32_GUARD
#define HZ3_S78_PTAG32_GUARD 0
#endif

#ifndef HZ3_S78_PTAG32_GUARD_FAILFAST
#define HZ3_S78_PTAG32_GUARD_FAILFAST HZ3_S78_PTAG32_GUARD
#endif

#ifndef HZ3_S78_PTAG32_GUARD_SHOT
#define HZ3_S78_PTAG32_GUARD_SHOT 1
#endif

// S79: PTAG32 store trace (tag overwrite detection)
#ifndef HZ3_S79_PTAG32_STORE_TRACE
#define HZ3_S79_PTAG32_STORE_TRACE 0
#endif

#ifndef HZ3_S92_PTAG32_STORE_LAST
#define HZ3_S92_PTAG32_STORE_LAST 0
#endif

#ifndef HZ3_S79_PTAG32_STORE_EXCHANGE
#define HZ3_S79_PTAG32_STORE_EXCHANGE 0
#endif

#ifndef HZ3_S79_PTAG32_STORE_FAILFAST
#define HZ3_S79_PTAG32_STORE_FAILFAST HZ3_S79_PTAG32_STORE_TRACE
#endif

#ifndef HZ3_S79_PTAG32_STORE_SHOT
#define HZ3_S79_PTAG32_STORE_SHOT 1
#endif

// S88: PTAG32 clear trace (tag clear callsite)
#ifndef HZ3_S88_PTAG32_CLEAR_TRACE
#define HZ3_S88_PTAG32_CLEAR_TRACE 0
#endif

#ifndef HZ3_S98_PTAG32_CLEAR_MAP
#define HZ3_S98_PTAG32_CLEAR_MAP 0
#endif

#ifndef HZ3_S88_PTAG32_CLEAR_FAILFAST
// S88 is a trace box by default; failfast is opt-in to avoid "false crash"
// when clear is a normal operation (e.g., S65 reclaim / boundary clears).
#define HZ3_S88_PTAG32_CLEAR_FAILFAST 0
#endif

#ifndef HZ3_S88_PTAG32_CLEAR_SHOT
#define HZ3_S88_PTAG32_CLEAR_SHOT 1
#endif

#ifndef HZ3_S98_PTAG32_CLEAR_MAP_LOG2
#define HZ3_S98_PTAG32_CLEAR_MAP_LOG2 18
#endif

#ifndef HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2
#define HZ3_S98_PTAG32_CLEAR_MAP_SHARDS_LOG2 8
#endif

// S99: PTAG32 miss guard (free path) - detect in-range ptr with tag32==0.
// This catches "tag cleared while ptr is still live" before mis-dispatch to libc.
#ifndef HZ3_S99_PTAG32_MISS_GUARD
#define HZ3_S99_PTAG32_MISS_GUARD 0
#endif

#ifndef HZ3_S99_PTAG32_MISS_GUARD_FAILFAST
#define HZ3_S99_PTAG32_MISS_GUARD_FAILFAST HZ3_S99_PTAG32_MISS_GUARD
#endif

#ifndef HZ3_S99_PTAG32_MISS_GUARD_SHOT
#define HZ3_S99_PTAG32_MISS_GUARD_SHOT 1
#endif

// S81: Medium free guard (free_bits check for stale runs)
#ifndef HZ3_S81_MEDIUM_FREE_GUARD
#define HZ3_S81_MEDIUM_FREE_GUARD 0
#endif

#ifndef HZ3_S81_MEDIUM_FREE_GUARD_FAILFAST
#define HZ3_S81_MEDIUM_FREE_GUARD_FAILFAST HZ3_S81_MEDIUM_FREE_GUARD
#endif

#ifndef HZ3_S81_MEDIUM_FREE_GUARD_SHOT
#define HZ3_S81_MEDIUM_FREE_GUARD_SHOT 1
#endif

#ifndef HZ3_S81_MEDIUM_FREE_GUARD_DEFAULT
#define HZ3_S81_MEDIUM_FREE_GUARD_DEFAULT 1
#endif

#ifndef HZ3_S81_MEDIUM_FREE_GUARD_LOG
#define HZ3_S81_MEDIUM_FREE_GUARD_LOG 1
#endif

// S82: Stash guard (bin vs ptag32 mismatch at push boundaries)
#ifndef HZ3_S82_STASH_GUARD
#define HZ3_S82_STASH_GUARD 0
#endif

#ifndef HZ3_S82_STASH_GUARD_FAILFAST
#define HZ3_S82_STASH_GUARD_FAILFAST HZ3_S82_STASH_GUARD
#endif

#ifndef HZ3_S82_STASH_GUARD_SHOT
#define HZ3_S82_STASH_GUARD_SHOT 1
#endif

#ifndef HZ3_S82_STASH_GUARD_DEFAULT
#define HZ3_S82_STASH_GUARD_DEFAULT 1
#endif

#ifndef HZ3_S82_STASH_GUARD_LOG
#define HZ3_S82_STASH_GUARD_LOG 1
#endif

#ifndef HZ3_S82_STASH_GUARD_MAX_WALK
#define HZ3_S82_STASH_GUARD_MAX_WALK 16
#endif

// S83: Bin push guard (seg_hdr/segmap validation for ptag32 free)
#ifndef HZ3_S83_BIN_PUSH_GUARD
#define HZ3_S83_BIN_PUSH_GUARD 0
#endif

#ifndef HZ3_S83_BIN_PUSH_GUARD_FAILFAST
#define HZ3_S83_BIN_PUSH_GUARD_FAILFAST HZ3_S83_BIN_PUSH_GUARD
#endif

#ifndef HZ3_S83_BIN_PUSH_GUARD_SHOT
#define HZ3_S83_BIN_PUSH_GUARD_SHOT 1
#endif

#ifndef HZ3_S83_BIN_PUSH_GUARD_DEFAULT
#define HZ3_S83_BIN_PUSH_GUARD_DEFAULT 1
#endif

#ifndef HZ3_S83_BIN_PUSH_GUARD_LOG
#define HZ3_S83_BIN_PUSH_GUARD_LOG 1
#endif

// Small v2 performance statistics (compile-time gated, TLS + atexit dump)
#ifndef HZ3_S12_V2_STATS
#define HZ3_S12_V2_STATS 0
#endif

// ============================================================================
// PageTagMap configuration (S12-S21)
// ============================================================================

// PTAG32-only mode: disallow PTAG16 owner encoding (requires PTAG32 path).
#ifndef HZ3_PTAG32_ONLY
#define HZ3_PTAG32_ONLY 0
#endif

// Small v2 PageTagMap (arena page-level tag for O(1) ptr→(sc,owner) lookup)
#ifndef HZ3_SMALL_V2_PTAG_ENABLE
#define HZ3_SMALL_V2_PTAG_ENABLE 0
#endif

// S12-5B: PTAG for v1 (4KB-32KB) allocations
#ifndef HZ3_PTAG_V1_ENABLE
#define HZ3_PTAG_V1_ENABLE 0
#endif

// S12-5C: Fail-fast mode for PTAG errors
#ifndef HZ3_PTAG_FAILFAST
#define HZ3_PTAG_FAILFAST 0
#endif

// S17: PTAG dst/bin direct (hot free without kind/owner decode)
#ifndef HZ3_PTAG_DSTBIN_ENABLE
#define HZ3_PTAG_DSTBIN_ENABLE 0
#endif

#if HZ3_PTAG32_ONLY
// PTAG32-only uses dst/bin tags for owner routing; disable PTAG16 encodings.
#undef HZ3_SMALL_V2_PTAG_ENABLE
#define HZ3_SMALL_V2_PTAG_ENABLE 0
#undef HZ3_PTAG_V1_ENABLE
#define HZ3_PTAG_V1_ENABLE 0
#if !HZ3_PTAG_DSTBIN_ENABLE
#error "HZ3_PTAG32_ONLY requires HZ3_PTAG_DSTBIN_ENABLE=1"
#endif
#endif

// PTAG dispatch helpers (compile-time)
#ifndef HZ3_PTAG32_DISPATCH_ENABLE
#define HZ3_PTAG32_DISPATCH_ENABLE (HZ3_PTAG_DSTBIN_ENABLE && (HZ3_PTAG_V1_ENABLE || HZ3_PTAG32_ONLY))
#endif

#ifndef HZ3_PTAG16_DISPATCH_ENABLE
#define HZ3_PTAG16_DISPATCH_ENABLE (HZ3_SMALL_V2_PTAG_ENABLE && HZ3_PTAG_V1_ENABLE)
#endif

#ifndef HZ3_PTAG_DISPATCH_ENABLE
#define HZ3_PTAG_DISPATCH_ENABLE (HZ3_PTAG16_DISPATCH_ENABLE || HZ3_PTAG32_DISPATCH_ENABLE)
#endif

// S18-1: PTAG dst/bin fast lookup
#ifndef HZ3_PTAG_DSTBIN_FASTLOOKUP
#define HZ3_PTAG_DSTBIN_FASTLOOKUP 0
#endif

// S18-2: PTAG dst/bin flat index
#ifndef HZ3_PTAG_DSTBIN_FLAT
#define HZ3_PTAG_DSTBIN_FLAT 0
#endif

// S21: PTAG32-first usable_size/realloc
#ifndef HZ3_USABLE_SIZE_PTAG32
#define HZ3_USABLE_SIZE_PTAG32 0
#endif

#ifndef HZ3_REALLOC_PTAG32
#define HZ3_REALLOC_PTAG32 0
#endif

// S17: PTAG dst/bin stats (slow/event only)
#ifndef HZ3_PTAG_DSTBIN_STATS
#define HZ3_PTAG_DSTBIN_STATS 0
#endif

// S19-1: PTAG32 THP / TLB hint
#ifndef HZ3_PTAG32_HUGEPAGE
#define HZ3_PTAG32_HUGEPAGE 0
#endif

// S20-1: Prefetch PTAG32 entry after in-range check
#ifndef HZ3_PTAG32_PREFETCH
#define HZ3_PTAG32_PREFETCH 0
#endif

	// S28-5: PTAG32 lookup hit-path optimization
	#ifndef HZ3_PTAG32_NOINRANGE
	#define HZ3_PTAG32_NOINRANGE 0
	#endif

	// S114: PTAG32 TLS cache (arena_base + ptag32 base snapshots in TLS)
	// Motivation: avoid atomic load of g_hz3_arena_base on hot free/lookup paths.
	#ifndef HZ3_S114_PTAG32_TLS_CACHE
	#define HZ3_S114_PTAG32_TLS_CACHE 0
	#endif

	// Implementation flag used by hot paths (hz3_arena.h / hz3_hot.c).
	#ifndef HZ3_PTAG_DSTBIN_TLS
	#define HZ3_PTAG_DSTBIN_TLS HZ3_S114_PTAG32_TLS_CACHE
	#endif

// S16-2: PageTag layout v2 (0-based sc/owner encoding)
#ifndef HZ3_PTAG_LAYOUT_V2
#define HZ3_PTAG_LAYOUT_V2 0
#endif

// S16-2D: Split free fast/slow
#ifndef HZ3_FREE_FASTPATH_SPLIT
#define HZ3_FREE_FASTPATH_SPLIT 0
#endif

// ============================================================================
// Remote flush configuration (S24)
// ============================================================================

// S24: Budgeted remote bank flush
#ifndef HZ3_DSTBIN_FLUSH_BUDGET_BINS
#define HZ3_DSTBIN_FLUSH_BUDGET_BINS 32
#endif

// S24-2: Remote hint to skip flush when no remote free occurred
#ifndef HZ3_DSTBIN_REMOTE_HINT_ENABLE
#define HZ3_DSTBIN_REMOTE_HINT_ENABLE 1
#endif

// S173: Demand-gated remote flush (opt-in, sparse remote stash only)
// Flush budgeted remote stash only after enough remote push activity.
#ifndef HZ3_S173_DSTBIN_DEMAND_GATE
#define HZ3_S173_DSTBIN_DEMAND_GATE 0
#endif

#ifndef HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT
#define HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT 4
#endif

#ifndef HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP
#define HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP 64
#endif

#ifndef HZ3_S173_DSTBIN_DEMAND_CREDIT_CONSUME
#define HZ3_S173_DSTBIN_DEMAND_CREDIT_CONSUME HZ3_DSTBIN_FLUSH_BUDGET_BINS
#endif

#if HZ3_S173_DSTBIN_DEMAND_GATE && !HZ3_REMOTE_STASH_SPARSE
#error "HZ3_S173_DSTBIN_DEMAND_GATE requires HZ3_REMOTE_STASH_SPARSE=1"
#endif

#if HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT < 1
#error "HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT must be >= 1"
#endif

#if HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP < HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT
#error "HZ3_S173_DSTBIN_DEMAND_CREDIT_CAP must be >= HZ3_S173_DSTBIN_DEMAND_MIN_CREDIT"
#endif

#if HZ3_S173_DSTBIN_DEMAND_CREDIT_CONSUME < 1
#error "HZ3_S173_DSTBIN_DEMAND_CREDIT_CONSUME must be >= 1"
#endif

// ============================================================================
// Large allocation cache (S14)
// ============================================================================

// S14: Large allocation cache (mmap reuse)
#ifndef HZ3_LARGE_CACHE_ENABLE
#define HZ3_LARGE_CACHE_ENABLE 1
#endif

// Debug: large allocator consistency checks
#ifndef HZ3_LARGE_DEBUG
#define HZ3_LARGE_DEBUG 0
#endif

#if HZ3_LARGE_DEBUG
#ifndef HZ3_LARGE_DEBUG_SHOT
#define HZ3_LARGE_DEBUG_SHOT 1
#endif
#ifndef HZ3_LARGE_FAILFAST
#define HZ3_LARGE_FAILFAST 0
#endif
#ifndef HZ3_LARGE_CANARY_ENABLE
#define HZ3_LARGE_CANARY_ENABLE 1
#endif
#ifndef HZ3_LARGE_DEBUG_REGISTRY
#define HZ3_LARGE_DEBUG_REGISTRY 1
#endif
#ifndef HZ3_LARGE_DEBUG_REG_CAP
#define HZ3_LARGE_DEBUG_REG_CAP 8192
#endif
#else
#undef HZ3_LARGE_DEBUG_SHOT
#define HZ3_LARGE_DEBUG_SHOT 0
#undef HZ3_LARGE_FAILFAST
#define HZ3_LARGE_FAILFAST 0
#undef HZ3_LARGE_CANARY_ENABLE
#define HZ3_LARGE_CANARY_ENABLE 0
#undef HZ3_LARGE_DEBUG_REGISTRY
#define HZ3_LARGE_DEBUG_REGISTRY 0
#undef HZ3_LARGE_DEBUG_REG_CAP
#define HZ3_LARGE_DEBUG_REG_CAP 0
#endif

// S14: Maximum bytes/nodes to cache
#ifndef HZ3_LARGE_CACHE_MAX_BYTES
#define HZ3_LARGE_CACHE_MAX_BYTES (512u << 20)
#endif

#ifndef HZ3_LARGE_CACHE_MAX_NODES
#define HZ3_LARGE_CACHE_MAX_NODES 64
#endif

// ============================================================================
// Stats configuration (S15, S28)
// ============================================================================

// S15: Hz3Stats dump
#ifndef HZ3_STATS_DUMP
#define HZ3_STATS_DUMP 0
#endif

// S28: Tiny (small v2) slow path stats
#ifndef HZ3_TINY_STATS
#define HZ3_TINY_STATS 0
#endif

// S26-2: Span carve for sc=6,7 (28KB, 32KB)
#ifndef HZ3_SPAN_CARVE_ENABLE
#define HZ3_SPAN_CARVE_ENABLE 0
#endif

// ============================================================================
// Compile-time validation (SUB4K)
// ============================================================================

#if HZ3_SMALL_V1_ENABLE
#error "HZ3_SMALL_V1_ENABLE is archived and no longer supported"
#endif

#if HZ3_SUB4K_ENABLE
#if !HZ3_PTAG_DSTBIN_ENABLE
#error "HZ3_SUB4K_ENABLE requires HZ3_PTAG_DSTBIN_ENABLE=1"
#endif
#if !HZ3_SEG_SELF_DESC_ENABLE
#error "HZ3_SUB4K_ENABLE requires HZ3_SEG_SELF_DESC_ENABLE=1"
#endif
#endif

// ============================================================================
// S144: OwnerStash InstanceBox (N-way Partitioning)
// ============================================================================
// Purpose: Reduce CAS contention in OwnerStash push path by distributing
// operations across N instances. Each [owner][sc] bin becomes [owner][sc][inst].
//
// Strategy:
// - Push: Hash-based instance selection (load balancing)
// - Pop: Round-robin instance drain (fair, no global contention)
// - Power-of-2 restriction: Fast `& mask` selection
// - Backward compatible: INSTANCES=1 = zero overhead
//
// Memory footprint (64-bit, Hz3OwnerStashBin = 16 bytes):
//   - INST=1:  63 shards × 128 SC × 1  inst × 16B = 129 KB
//   - INST=4:  63 shards × 128 SC × 4  inst × 16B = 516 KB
//   - INST=16: 63 shards × 128 SC × 16 inst × 16B = 2.0 MB
// Limit ≤16 keeps BSS < 2.5 MB (acceptable for server workloads).

#ifndef HZ3_OWNER_STASH_INSTANCES
#define HZ3_OWNER_STASH_INSTANCES 1
#endif

#if HZ3_OWNER_STASH_INSTANCES < 1
  #error "HZ3_OWNER_STASH_INSTANCES must be >= 1"
#endif

#if HZ3_OWNER_STASH_INSTANCES > 16
  #error "HZ3_OWNER_STASH_INSTANCES must be <= 16 (memory budget)"
#endif

#if (HZ3_OWNER_STASH_INSTANCES & (HZ3_OWNER_STASH_INSTANCES - 1)) != 0
  #error "HZ3_OWNER_STASH_INSTANCES must be power of 2 (1/2/4/8/16)"
#endif

#define HZ3_OWNER_STASH_INST_MASK (HZ3_OWNER_STASH_INSTANCES - 1)

// ============================================================================
// S145-A: CentralPop Local Cache Box
// ============================================================================
// Purpose: Reduce atomic exchange hotspot in hz3_small_v2_alloc_slow() by
// adding a TLS per-sizeclass cache layer. When refilling from central, grab
// a batch of items, return 1, and cache the rest locally.
//
// Usage:
//   make -C hakozuna/hz3 clean all_ldpreload_scale  # baseline (OFF)
//   make ... HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S145_CENTRAL_LOCAL_CACHE=1'  # treatment

#ifndef HZ3_S145_CENTRAL_LOCAL_CACHE
#define HZ3_S145_CENTRAL_LOCAL_CACHE 0  // Default OFF (experimental)
#endif

#if HZ3_S145_CENTRAL_LOCAL_CACHE
  #ifndef HZ3_S145_CACHE_BATCH
  #define HZ3_S145_CACHE_BATCH 16  // Items to grab from central in bulk
  #endif
#endif
