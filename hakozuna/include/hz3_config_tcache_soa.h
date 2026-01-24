// hz3_config_tcache_soa.h - TCache SoA / Bin count / Sparse remote (S28-S41)
// Part of hz3_config.h split for maintainability
#pragma once

// ============================================================================
// Bin count policy (S28, S38, S122)
// ============================================================================
//
// Rationale:
// - Count 周りのフラグが増えて複雑化しやすいので、入口を 1つに畳む。
//
// Values:
// - 0: COUNT_U16 (default)
// - 1: COUNT_U32
// - 2: LAZY_COUNT (no hot updates, O(n) walk in event path)
// - 3: SMALL_NOCOUNT + COUNT_U16
// - 4: SMALL_NOCOUNT + COUNT_U32
// - 5: SPLIT_COUNT (S122: low 4bit in head, hi updated 1/16)

// Legacy flags (for backward compatibility, normalized below)
#ifndef HZ3_SMALL_BIN_NOCOUNT
#define HZ3_SMALL_BIN_NOCOUNT 0
#endif

#ifndef HZ3_BIN_LAZY_COUNT
#define HZ3_BIN_LAZY_COUNT 0
#endif

#ifndef HZ3_BIN_COUNT_U32
#define HZ3_BIN_COUNT_U32 1
#endif

#ifndef HZ3_BIN_SPLIT_COUNT
#define HZ3_BIN_SPLIT_COUNT 0
#endif

#ifndef HZ3_BIN_COUNT_POLICY
#if HZ3_BIN_LAZY_COUNT && HZ3_SMALL_BIN_NOCOUNT
#error "invalid config: HZ3_BIN_LAZY_COUNT and HZ3_SMALL_BIN_NOCOUNT are mutually exclusive"
#endif
#if HZ3_BIN_SPLIT_COUNT && HZ3_BIN_LAZY_COUNT
#error "invalid config: HZ3_BIN_SPLIT_COUNT and HZ3_BIN_LAZY_COUNT are mutually exclusive"
#endif
#if HZ3_BIN_SPLIT_COUNT
#define HZ3_BIN_COUNT_POLICY 5
#elif HZ3_BIN_LAZY_COUNT
#define HZ3_BIN_COUNT_POLICY 2
#elif HZ3_SMALL_BIN_NOCOUNT && HZ3_BIN_COUNT_U32
#define HZ3_BIN_COUNT_POLICY 4
#elif HZ3_SMALL_BIN_NOCOUNT
#define HZ3_BIN_COUNT_POLICY 3
#elif HZ3_BIN_COUNT_U32
#define HZ3_BIN_COUNT_POLICY 1
#else
#define HZ3_BIN_COUNT_POLICY 0
#endif
#endif

// Normalize legacy macros based on HZ3_BIN_COUNT_POLICY.
#undef HZ3_SMALL_BIN_NOCOUNT
#undef HZ3_BIN_LAZY_COUNT
#undef HZ3_BIN_COUNT_U32
#undef HZ3_BIN_SPLIT_COUNT
#if HZ3_BIN_COUNT_POLICY == 0
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 0
#define HZ3_BIN_SPLIT_COUNT 0
#elif HZ3_BIN_COUNT_POLICY == 1
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 1
#define HZ3_BIN_SPLIT_COUNT 0
#elif HZ3_BIN_COUNT_POLICY == 2
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 1
#define HZ3_BIN_COUNT_U32 0
#define HZ3_BIN_SPLIT_COUNT 0
#elif HZ3_BIN_COUNT_POLICY == 3
#define HZ3_SMALL_BIN_NOCOUNT 1
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 0
#define HZ3_BIN_SPLIT_COUNT 0
#elif HZ3_BIN_COUNT_POLICY == 4
#define HZ3_SMALL_BIN_NOCOUNT 1
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 1
#define HZ3_BIN_SPLIT_COUNT 0
#elif HZ3_BIN_COUNT_POLICY == 5
// S122: Split Count (low 4bit in head_tag, count_hi updated 1/16)
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 0
#define HZ3_BIN_SPLIT_COUNT 1
#else
#error "invalid HZ3_BIN_COUNT_POLICY value (0-5)"
#endif

// ============================================================================
// Local bins split (S28-2C)
// ============================================================================

// S28-2C: Local bins split (local を bank から分離)
#ifndef HZ3_LOCAL_BINS_SPLIT
#define HZ3_LOCAL_BINS_SPLIT 1
#endif

// S32-1: TLS init check を malloc hot から除去
#ifndef HZ3_TCACHE_INIT_ON_MISS
#define HZ3_TCACHE_INIT_ON_MISS 1
#endif

// ============================================================================
// S40: TCache SoA + Bin Padding (imul 除去)
// ============================================================================
//
// 目的:
// - bank[dst][bin] の offset 計算から imul を消す
// - dst * HZ3_BIN_TOTAL * 16 → dst << (HZ3_BIN_PAD_LOG2 + 3) にする

#ifndef HZ3_TCACHE_SOA
#define HZ3_TCACHE_SOA 1
#endif

#ifndef HZ3_TCACHE_SOA_LOCAL
#define HZ3_TCACHE_SOA_LOCAL 0
#endif

#ifndef HZ3_TCACHE_SOA_BANK
#define HZ3_TCACHE_SOA_BANK 0
#endif

// 旧 HZ3_TCACHE_SOA=1 は LOCAL=1, BANK=1 に展開
#if HZ3_TCACHE_SOA
#undef HZ3_TCACHE_SOA_LOCAL
#undef HZ3_TCACHE_SOA_BANK
#define HZ3_TCACHE_SOA_LOCAL 1
#define HZ3_TCACHE_SOA_BANK 1
#endif

// HZ3_BIN_PAD_LOG2: 0=padding なし, 8=stride=256 (pow2)
#ifndef HZ3_BIN_PAD_LOG2
#define HZ3_BIN_PAD_LOG2 8
#endif

// HZ3_BIN_PAD: computed stride
#if HZ3_BIN_PAD_LOG2
#define HZ3_BIN_PAD (1u << HZ3_BIN_PAD_LOG2)
#else
#define HZ3_BIN_PAD HZ3_BIN_TOTAL
#endif

// S40-2: bank SoA requires padding
#if HZ3_TCACHE_SOA_BANK && !HZ3_BIN_PAD_LOG2
#error "HZ3_TCACHE_SOA_BANK=1 requires HZ3_BIN_PAD_LOG2>0 (e.g. 8) to eliminate imul"
#endif

// ============================================================================
// S41: Sparse RemoteStash (scale lane)
// ============================================================================
//
// Rationale:
// - Dense bank[HZ3_NUM_SHARDS][HZ3_BIN_TOTAL] grows linearly with shards
// - Sparse ring uses constant TLS (~4KB) regardless of shard count

#ifndef HZ3_REMOTE_STASH_SPARSE
#define HZ3_REMOTE_STASH_SPARSE 0
#endif

#ifndef HZ3_REMOTE_STASH_RING_SIZE
// S149: ring=1024 GO (+16.3% on R=90, no R=0 regression, -72% overflow)
#define HZ3_REMOTE_STASH_RING_SIZE 1024
#endif

#if HZ3_REMOTE_STASH_SPARSE
_Static_assert((HZ3_REMOTE_STASH_RING_SIZE & (HZ3_REMOTE_STASH_RING_SIZE - 1)) == 0,
               "HZ3_REMOTE_STASH_RING_SIZE must be power of 2");
_Static_assert(HZ3_NUM_SHARDS <= 255, "requires HZ3_NUM_SHARDS <= 255 (8-bit dst)");
#endif
