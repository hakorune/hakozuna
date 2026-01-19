#pragma once

// hz3 is developed as a separate track. The LD_PRELOAD shim may exist even when
// hz3 is not fully implemented yet.
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

// v2-only dispatch: use PageTagMap only, no segmap fallback.
// Requires HZ3_SMALL_V2_PTAG_ENABLE=1 and HZ3_SEG_SELF_DESC_ENABLE=1.
#ifndef HZ3_V2_ONLY
#define HZ3_V2_ONLY 0
#endif

#if HZ3_V2_ONLY
// V2-only relies on PTAG32 (dst/bin) as the single in-arena dispatch for free().
// Keep this strict so we don't accidentally compile a "return everything" build.
#if !HZ3_SMALL_V2_PTAG_ENABLE || !HZ3_SEG_SELF_DESC_ENABLE || !HZ3_PTAG_V1_ENABLE
#error "HZ3_V2_ONLY requires HZ3_SMALL_V2_PTAG_ENABLE=1, HZ3_SEG_SELF_DESC_ENABLE=1, HZ3_PTAG_V1_ENABLE=1"
#endif
#if !HZ3_PTAG_DSTBIN_ENABLE || !HZ3_PTAG_DSTBIN_FASTLOOKUP
#error "HZ3_V2_ONLY requires HZ3_PTAG_DSTBIN_ENABLE=1 and HZ3_PTAG_DSTBIN_FASTLOOKUP=1"
#endif
#endif

// v2-only PageTagMap coverage stats (TLS + atexit dump).
#ifndef HZ3_V2_ONLY_STATS
#define HZ3_V2_ONLY_STATS 0
#endif

// Self-describing segment header gate
#ifndef HZ3_SEG_SELF_DESC_ENABLE
#define HZ3_SEG_SELF_DESC_ENABLE 0
#endif

// Debug-only fail-fast for self-describing segments
#ifndef HZ3_SMALL_V2_FAILFAST
#define HZ3_SMALL_V2_FAILFAST 0
#endif

// Small v2 performance statistics (compile-time gated, TLS + atexit dump)
#ifndef HZ3_S12_V2_STATS
#define HZ3_S12_V2_STATS 0
#endif

// Small v2 PageTagMap (arena page-level tag for O(1) ptr→(sc,owner) lookup)
// Requires HZ3_SMALL_V2_ENABLE=1 && HZ3_SEG_SELF_DESC_ENABLE=1
#ifndef HZ3_SMALL_V2_PTAG_ENABLE
#define HZ3_SMALL_V2_PTAG_ENABLE 0
#endif

// S12-5B: PTAG for v1 (4KB-32KB) allocations
// Requires HZ3_SMALL_V2_PTAG_ENABLE=1 (uses same PageTagMap infrastructure)
// When enabled, v1 allocations also set page tags for unified dispatch
#ifndef HZ3_PTAG_V1_ENABLE
#define HZ3_PTAG_V1_ENABLE 0
#endif

// S12-5C: Fail-fast mode for PTAG errors
// When 1: abort() on tag==0 in arena (debug mode)
// When 0: silent no-op (release mode, safer for production)
#ifndef HZ3_PTAG_FAILFAST
#define HZ3_PTAG_FAILFAST 0
#endif

// Guardrails (debug-only). Enables extra range checks / build-time asserts.
#ifndef HZ3_GUARDRAILS
#define HZ3_GUARDRAILS 0
#endif

// OOM observability / fail-fast (init/slow path only)
#ifndef HZ3_OOM_SHOT
#define HZ3_OOM_SHOT 0  // normal
#endif

#ifndef HZ3_OOM_FAILFAST
#define HZ3_OOM_FAILFAST 0
#endif

// S17: PTAG dst/bin direct (hot free without kind/owner decode)
// Requires HZ3_SMALL_V2_PTAG_ENABLE=1
#ifndef HZ3_PTAG_DSTBIN_ENABLE
#define HZ3_PTAG_DSTBIN_ENABLE 0
#endif

// S18-1: PTAG dst/bin fast lookup (range check + tag load in one helper)
#ifndef HZ3_PTAG_DSTBIN_FASTLOOKUP
#define HZ3_PTAG_DSTBIN_FASTLOOKUP 0
#endif

// S18-2: PTAG dst/bin flat index (tag32 == flat+1, no decode/stride)
#ifndef HZ3_PTAG_DSTBIN_FLAT
#define HZ3_PTAG_DSTBIN_FLAT 0
#endif

#if HZ3_SUB4K_ENABLE
#if !HZ3_PTAG_DSTBIN_ENABLE
#error "HZ3_SUB4K_ENABLE requires HZ3_PTAG_DSTBIN_ENABLE=1"
#endif
#if !HZ3_SEG_SELF_DESC_ENABLE
#error "HZ3_SUB4K_ENABLE requires HZ3_SEG_SELF_DESC_ENABLE=1"
#endif
#endif

// S21: PTAG32-first usable_size/realloc (bin->size via PTAG32)
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

// S19-1: PTAG32 THP / TLB hint (init-only, hot 0 instructions)
// Applies MADV_HUGEPAGE to the PTAG32 table mapping when available.
#ifndef HZ3_PTAG32_HUGEPAGE
#define HZ3_PTAG32_HUGEPAGE 0
#endif

// S20-1: Prefetch PTAG32 entry after in-range check (hot adds 1 prefetch op).
// Intended to be used with HZ3_PTAG_DSTBIN_FASTLOOKUP=1.
#ifndef HZ3_PTAG32_PREFETCH
#define HZ3_PTAG32_PREFETCH 0
#endif

// S28-5: PTAG32 lookup hit-path optimization (no in_range store on hit)
// When 1, free hot path uses hz3_pagetag32_lookup_hit_fast() which skips
// the in_range out-param write. Miss path does separate range check.
// Note:
// - This is a conditional KEEP: dist=app/uniform improves, tiny is neutral.
// - Keep the header default conservative (0). The hz3 lane enables it via
//   `hakozuna/hz3/Makefile` (HZ3_LDPRELOAD_DEFS) to avoid affecting other builds.
#ifndef HZ3_PTAG32_NOINRANGE
#define HZ3_PTAG32_NOINRANGE 0
#endif

// S24: Budgeted remote bank flush (round-robin, event-only)
// Number of bins to scan per hz3_alloc_slow() call.
#ifndef HZ3_DSTBIN_FLUSH_BUDGET_BINS
#define HZ3_DSTBIN_FLUSH_BUDGET_BINS 32
#endif

// S24-2: Remote hint to skip flush when no remote free occurred
// GO: dist=app +3.4%, uniform no regression (2026-01-02)
#ifndef HZ3_DSTBIN_REMOTE_HINT_ENABLE
#define HZ3_DSTBIN_REMOTE_HINT_ENABLE 1
#endif

// S14: Large allocation cache (mmap reuse)
// When enabled, hz3_large_free() caches mappings instead of munmap
#ifndef HZ3_LARGE_CACHE_ENABLE
#define HZ3_LARGE_CACHE_ENABLE 1
#endif

// Debug: large allocator consistency checks (debug-only A/B)
// Master switch: when OFF, all large debug hooks are disabled.
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

// S14: Maximum bytes to cache (default 512MiB)
#ifndef HZ3_LARGE_CACHE_MAX_BYTES
#define HZ3_LARGE_CACHE_MAX_BYTES (512u << 20)
#endif

// S14: Maximum number of cached nodes (default 64)
#ifndef HZ3_LARGE_CACHE_MAX_NODES
#define HZ3_LARGE_CACHE_MAX_NODES 64
#endif

// S15: Hz3Stats dump (atexit dump of refill_calls, central_pop_miss, etc.)
// For triage: enable to see slow path call counts per sc
#ifndef HZ3_STATS_DUMP
#define HZ3_STATS_DUMP 0
#endif

// S28: Tiny (small v2) slow path stats (event-only, atexit dump)
// For triage: measure slow_rate to determine if bottleneck is hot or supply
#ifndef HZ3_TINY_STATS
#define HZ3_TINY_STATS 0
#endif

// S28-2A: Bank row base cache (TLS caches &bank[my_shard][0] for faster bin access)
// ARCHIVED (NO-GO): code removed from mainline to reduce flag/branch sprawl.
// Reference: hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md
#ifndef HZ3_TCACHE_BANK_ROW_CACHE
#define HZ3_TCACHE_BANK_ROW_CACHE 0
#endif
#if HZ3_TCACHE_BANK_ROW_CACHE
#error "HZ3_TCACHE_BANK_ROW_CACHE is archived (NO-GO). See hakozuna/hz3/archive/research/s28_2a_bank_row_cache/README.md"
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
// Intended for stress/CI to catch accidental oversubscription assumptions.
#ifndef HZ3_SHARD_COLLISION_FAILFAST
#define HZ3_SHARD_COLLISION_FAILFAST 0
#endif

// S28-2B: Small bin nocount (hot path から count 更新を除去)
// alloc/free の hot path で bin->count++/-- を省略し store を削減
#ifndef HZ3_SMALL_BIN_NOCOUNT
#define HZ3_SMALL_BIN_NOCOUNT 0
#endif

// S38-1: Bin lazy count (hot path からすべての bin->count 更新を除去)
// HZ3_SMALL_BIN_NOCOUNT の全 bin 拡張版。
// event-only (trim/flush/drain) は head ベースの判定に置換。
#ifndef HZ3_BIN_LAZY_COUNT
#define HZ3_BIN_LAZY_COUNT 0
#endif

// S38-2: bin->count を uint16_t → uint32_t に変更（同挙動のままコード生成改善）
// subw → subl でレイテンシ/依存改善を狙う（Hz3Bin サイズは16Bのまま不変）
#ifndef HZ3_BIN_COUNT_U32
#define HZ3_BIN_COUNT_U32 1
#endif

// S39-2: hz3_free の remote 経路を cold 化
// ARCHIVED (NO-GO): code removed from mainline to reduce flag/branch sprawl.
// Reference: hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md
#ifndef HZ3_FREE_REMOTE_COLDIFY
#define HZ3_FREE_REMOTE_COLDIFY 0
#endif
#if HZ3_FREE_REMOTE_COLDIFY
#error "HZ3_FREE_REMOTE_COLDIFY is archived (NO-GO). See hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md"
#endif

// ============================================================================
// Bin count policy (single entrypoint)
// ============================================================================
//
// Rationale:
// - Count 周りのフラグが増えて複雑化しやすいので、入口を 1つに畳む。
// - 既存のフラグ（HZ3_*_NOCOUNT / HZ3_BIN_LAZY_COUNT / HZ3_BIN_COUNT_U32）は互換として残し、
//   最終的に **ここで正規化** して以降のコードは一貫した解釈になるようにする。
//
// Values:
// - 0: COUNT_U16 (default)
// - 1: COUNT_U32
// - 2: LAZY_COUNT (no hot updates)
// - 3: SMALL_NOCOUNT + COUNT_U16
// - 4: SMALL_NOCOUNT + COUNT_U32
#ifndef HZ3_BIN_COUNT_POLICY
#if HZ3_BIN_LAZY_COUNT && HZ3_SMALL_BIN_NOCOUNT
#error "invalid config: HZ3_BIN_LAZY_COUNT and HZ3_SMALL_BIN_NOCOUNT are mutually exclusive"
#endif
#if HZ3_BIN_LAZY_COUNT
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
#if HZ3_BIN_COUNT_POLICY == 0
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 0
#elif HZ3_BIN_COUNT_POLICY == 1
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 1
#elif HZ3_BIN_COUNT_POLICY == 2
#define HZ3_SMALL_BIN_NOCOUNT 0
#define HZ3_BIN_LAZY_COUNT 1
#define HZ3_BIN_COUNT_U32 0
#elif HZ3_BIN_COUNT_POLICY == 3
#define HZ3_SMALL_BIN_NOCOUNT 1
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 0
#elif HZ3_BIN_COUNT_POLICY == 4
#define HZ3_SMALL_BIN_NOCOUNT 1
#define HZ3_BIN_LAZY_COUNT 0
#define HZ3_BIN_COUNT_U32 1
#else
#error "invalid HZ3_BIN_COUNT_POLICY value"
#endif

	// S28-2C: Local bins split (local を bank から分離)
	// local alloc/free は small_bins[]/bins[] を使い、浅い TLS オフセットに
	#ifndef HZ3_LOCAL_BINS_SPLIT
#define HZ3_LOCAL_BINS_SPLIT 1
#endif

// S32-1: TLS init check を malloc hot から除去（miss/slow 入口でのみ init）
// When 1: malloc hot hit では ensure_init() を呼ばず、miss/slow path の入口で ensure_init_slow()
// Note: free は引き続き ensure_init() を呼ぶ（init前のdst routing等の安全のため）
#ifndef HZ3_TCACHE_INIT_ON_MISS
#define HZ3_TCACHE_INIT_ON_MISS 1
#endif

// S16-2: PageTag layout v2 (0-based sc/owner encoding)
// When 1: sc/owner stored as 0-based (no +1), decode avoids -1 operations
// When 0: legacy encoding (sc+1, owner+1)
#ifndef HZ3_PTAG_LAYOUT_V2
#define HZ3_PTAG_LAYOUT_V2 0
#endif

// S16-2D: Split free fast/slow to reduce spills (A/B gate)
#ifndef HZ3_FREE_FASTPATH_SPLIT
#define HZ3_FREE_FASTPATH_SPLIT 0
#endif

// S26-2: Span carve for sc=6,7 (28KB, 32KB) - allocate larger span, carve into 4 objects
// Reduces segment_alloc_run() calls by 4x for these size classes
#ifndef HZ3_SPAN_CARVE_ENABLE
#define HZ3_SPAN_CARVE_ENABLE 0
#endif

// ============================================================================
// S40: TCache SoA + Bin Padding (imul 除去)
// ============================================================================
//
// 目的:
// - bank[dst][bin] の offset 計算から imul を消す
// - dst * HZ3_BIN_TOTAL * 16 → dst << (HZ3_BIN_PAD_LOG2 + 3) にする
//
// S40-2: 分離フラグ (local/bank を個別に制御)
// - HZ3_TCACHE_SOA_LOCAL: 0=local は AoS 維持, 1=local も SoA
// - HZ3_TCACHE_SOA_BANK: 0=bank は AoS, 1=bank だけ SoA + pad
//
// 旧 HZ3_TCACHE_SOA は両方1の場合のショートカットとして維持
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

// S40-2: bank SoA requires padding (otherwise imul remains)
#if HZ3_TCACHE_SOA_BANK && !HZ3_BIN_PAD_LOG2
#error "HZ3_TCACHE_SOA_BANK=1 requires HZ3_BIN_PAD_LOG2>0 (e.g. 8) to eliminate imul"
#endif

// ============================================================================
// S41: Sparse RemoteStash (scale lane)
// ============================================================================
//
// Rationale:
// - Dense bank[HZ3_NUM_SHARDS][HZ3_BIN_TOTAL] grows linearly with shards
//   (8 shards → 48KB TLS, 32 shards → 194KB TLS)
// - Sparse ring uses constant TLS (~4KB) regardless of shard count
// - Enables HZ3_NUM_SHARDS=32 without TLS bloat
//
// Design:
// - TLS holds ring of {dst, bin, ptr} entries
// - Hot path pushes to ring (emergency flush on overflow)
// - Event-only drain dispatches to existing inbox/central

#ifndef HZ3_REMOTE_STASH_SPARSE
#define HZ3_REMOTE_STASH_SPARSE 0
#endif

#ifndef HZ3_REMOTE_STASH_RING_SIZE
#define HZ3_REMOTE_STASH_RING_SIZE 256  // power of 2
#endif

#if HZ3_REMOTE_STASH_SPARSE
_Static_assert((HZ3_REMOTE_STASH_RING_SIZE & (HZ3_REMOTE_STASH_RING_SIZE - 1)) == 0,
               "HZ3_REMOTE_STASH_RING_SIZE must be power of 2");
_Static_assert(HZ3_NUM_SHARDS <= 255, "requires HZ3_NUM_SHARDS <= 255 (8-bit dst)");
// Note: HZ3_BIN_TOTAL assert moved to hz3_types.h (where HZ3_BIN_TOTAL is defined)
#endif

// ============================================================================
// S42: Small Transfer Cache (scale lane remote-heavy optimization)
// ============================================================================
//
// Rationale:
// - T=32/R=50/16-2048 で small central (mutex+list) がボトルネック
// - transfer cache で remote push を吸収し、central mutex 負荷を削減
// - event-only / slow path なので mutex コスト許容
//
// Design:
// - list 単位で保持（Hz3BatchChain）で lock 回数を最小化
// - push overflow → central fallback
// - pop miss → central fallback

#ifndef HZ3_S42_SMALL_XFER
#define HZ3_S42_SMALL_XFER 0
#endif

#ifndef HZ3_S42_SMALL_XFER_SLOTS
#define HZ3_S42_SMALL_XFER_SLOTS 64
#endif

// ============================================================================
// S44: Owner Shared Stash (scale lane remote-heavy optimization)
// ============================================================================
//
// Rationale:
// - Remote free objects go to owner's stash (MPSC, no mutex on push)
// - Alloc miss pops from own stash before central (mutex avoidance)
// - Insertion order: xfer (S42) -> owner_stash (S44) -> central -> page
//
// Design:
// - MPSC: atomic CAS push (multiple producers), exchange pop (single consumer)
// - Scale lane only (requires HZ3_REMOTE_STASH_SPARSE)
// - Overflow falls back to S42 xfer -> central

#ifndef HZ3_S44_OWNER_STASH
#define HZ3_S44_OWNER_STASH 0
#endif

// Push/pop can be controlled separately for A/B
#ifndef HZ3_S44_OWNER_STASH_PUSH
#define HZ3_S44_OWNER_STASH_PUSH (HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE)
#endif

#ifndef HZ3_S44_OWNER_STASH_POP
#define HZ3_S44_OWNER_STASH_POP (HZ3_S44_OWNER_STASH && HZ3_REMOTE_STASH_SPARSE)
#endif

// Drain limit (default: same as refill batch)
#ifndef HZ3_S44_DRAIN_LIMIT
#define HZ3_S44_DRAIN_LIMIT 32
#endif

// Max objects per bin before overflow to S42/central
#ifndef HZ3_S44_STASH_MAX_OBJS
#define HZ3_S44_STASH_MAX_OBJS 256
#endif

// S44-3: Owner Stash optimization flags
// COUNT: Enable approximate count tracking (overflow gate).
// When disabled, removes atomic fetch_add/sub overhead (and STASH_MAX_OBJS gate).
#ifndef HZ3_S44_OWNER_STASH_COUNT
#define HZ3_S44_OWNER_STASH_COUNT 0  // default OFF (S44-3 GO)
#endif

// FASTPOP: O(1) pop_batch fast path when old_head == NULL.
// Avoids hz3_list_find_tail() traversal in common case.
#ifndef HZ3_S44_OWNER_STASH_FASTPOP
#define HZ3_S44_OWNER_STASH_FASTPOP 1  // default ON (S44-3 GO)
#endif

// Compile-time guard: FASTPOP requires COUNT=0
// (fast-path early return skips count update, causing inconsistency)
#if HZ3_S44_OWNER_STASH_FASTPOP && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S44_OWNER_STASH_FASTPOP=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// ============================================================================
// S45: Memory Budget Box (scale lane arena exhaustion recovery)
// ============================================================================
//
// Rationale:
// - 16GB arena で arena_alloc_full が発生（medium run が central に残留）
// - arena_alloc() 失敗直前に segment-level reclaim を試行
// - free_pages == HZ3_PAGES_PER_SEG の segment を解放して arena slot を回収
//
// Phase 1: Segment-level reclaim only
// - No run-level tracking (Phase 2)
// - Minimal hot path impact (reclaim on failure only)

#ifndef HZ3_MEM_BUDGET_ENABLE
#define HZ3_MEM_BUDGET_ENABLE 0
#endif

// Phase 2: Max pages to reclaim per call (runaway prevention)
// Default: 4096 pages = 16MB worth of runs
#ifndef HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES
#define HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES 4096
#endif

// ============================================================================
// S48: Owner Stash Spill (TLS leftover cache)
// ============================================================================
//
// Rationale:
// - xmalloc-testN で hz3_owner_stash_pop_batch が 86.96% のホットスポット
// - S44-3 FASTPOP の CAS が高頻度で失敗（concurrent push が多い）
// - 失敗時の slow path: hz3_list_find_tail() でリスト全走査 → O(N)
//
// Solution:
// - 余った list を stash に戻さず、TLS (stash_spill[sc]) に保持
// - 次回 pop で TLS spill を先に消費 → stash アクセス頻度を削減
// - stash への push-back を完全に回避 → hz3_list_find_tail() 不要
//
// Requires: HZ3_S44_OWNER_STASH=1, HZ3_S44_OWNER_STASH_COUNT=0

#ifndef HZ3_S48_OWNER_STASH_SPILL
#define HZ3_S48_OWNER_STASH_SPILL 1
#endif

// Compile-time guard: SPILL is incompatible with COUNT
// (spill doesn't push back to stash, so count becomes invalid)
#if HZ3_S48_OWNER_STASH_SPILL && HZ3_S44_OWNER_STASH_COUNT
#error "HZ3_S48_OWNER_STASH_SPILL=1 requires HZ3_S44_OWNER_STASH_COUNT=0"
#endif

// ============================================================================
// S50: Large Cache Size-Class Box
// ============================================================================
//
// Rationale:
// - mstressN で hz3 が tc/mi に 50% 負け（mmap 532 回 @ 619 usec/call）
// - 現在の単一 LIFO cache は first-fit で多様なサイズを再利用できない
// - size class ごとの LIFO で O(1) cache hit を実現
//
// Design:
// - 32KB〜64MB を 8 分割/倍（最大内的断片化 12.5%）
// - 97 classes = 12 ranges × 8 sub-classes + 1 overflow (>64MB)
// - alloc: 同一 class から O(1) pop
// - free: 同一 class へ O(1) push
// - >64MB はキャッシュ対象外（巨大ブロックで cap を食い潰すのを防ぐ）

#ifndef HZ3_S50_LARGE_SCACHE
#define HZ3_S50_LARGE_SCACHE 1  // default ON (S50 GO)
#endif

// S50: Cap 超過時に最大 class から1個捨てる
#ifndef HZ3_S50_LARGE_SCACHE_EVICT
#define HZ3_S50_LARGE_SCACHE_EVICT 0
#endif

// S51: madvise(MADV_DONTNEED) on cache push (physical release, virtual retain)
// When enabled, cached blocks release physical memory but keep virtual address.
// This avoids munmap/mmap syscall overhead on cache eviction.
#ifndef HZ3_S51_LARGE_MADVISE
#define HZ3_S51_LARGE_MADVISE 0
#endif

// S52: Best-fit fallback for large cache (borrow from larger classes on miss)
// When exact class is empty, try sc+1..sc+HZ3_S52_BESTFIT_RANGE before mmap.
// Reduces mmap calls at cost of slight internal fragmentation.
#ifndef HZ3_S52_LARGE_BESTFIT
#define HZ3_S52_LARGE_BESTFIT 0
#endif

#ifndef HZ3_S52_BESTFIT_RANGE
#define HZ3_S52_BESTFIT_RANGE 2  // try sc+1 and sc+2
#endif

// S53: LargeCacheBudgetBox (soft/hard 2-stage cap, event-only)
// soft超え: madvise(MADV_DONTNEED) で物理だけ解放（map保持）
// hard超え: evict/munmap（既存動作）
#ifndef HZ3_LARGE_CACHE_BUDGET
#define HZ3_LARGE_CACHE_BUDGET 0
#endif

#ifndef HZ3_LARGE_CACHE_SOFT_BYTES
#define HZ3_LARGE_CACHE_SOFT_BYTES (4096ULL << 20)  // 4GB
#endif

#ifndef HZ3_LARGE_CACHE_HARD_BYTES
#define HZ3_LARGE_CACHE_HARD_BYTES (8192ULL << 20)  // 8GB
#endif

// S53: One-shot stats (soft_hits, hard_evicts, madvise_bytes)
#ifndef HZ3_LARGE_CACHE_STATS
#define HZ3_LARGE_CACHE_STATS 0
#endif

// S53-2: ThrottleBox（tcmalloc式カウンタで madvise 間引き）
// BUDGET=1 の時だけ有効
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
#define HZ3_LARGE_SC_MIN   (32u << 10)   // 32KB
#define HZ3_LARGE_SC_MAX   (64u << 20)   // 64MB
#define HZ3_LARGE_SC_COUNT 97            // 12 ranges × 8 + 1 overflow

// ============================================================================
// S54: SegmentPageScavengeBox OBSERVE mode
// ============================================================================
//
// Rationale:
// - S53-3 mstress RSS reduction was -3.7% (vs expected -14~19%)
// - Hypothesis: segment/medium may hold more RSS than large cache
// - OBSERVE phase: statistics only (no madvise), confirm candidate_pages exist
//
// Design:
// - Event-only execution (S46 pressure handler, zero hot path cost)
// - PackBox API boundary (hz3_pack_observe_owner)
// - Max tracking (not cumulative) for single pressure event insight
// - Opt-in (default OFF, enable via HZ3_LDPRELOAD_DEFS_EXTRA)

// S54: SegmentPageScavengeBox OBSERVE mode (統計のみ、madvise実行なし)
#ifndef HZ3_SEG_SCAVENGE_OBSERVE
#define HZ3_SEG_SCAVENGE_OBSERVE 0
#endif

// S54: Minimum contiguous pages to consider for madvise (128KB = 32 pages)
#ifndef HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES
#define HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES 32  // 128KB (32 * 4KB)
#endif

// ============================================================================
// S55: RetentionPolicyBox OBSERVE mode (統計のみ)
// ============================================================================
//
// Purpose:
// - RSS gap の主因（TLS/central/segment の「保持」）を切り分ける。
// - hot path は汚さず、event-only の観測（one-shot dump）だけを提供する。
//
#ifndef HZ3_S55_RETENTION_OBSERVE
#define HZ3_S55_RETENTION_OBSERVE 0
#endif

// ============================================================================
// S55-2: RetentionPolicyBox FROZEN mode (Admission Control + Open Debt)
// ============================================================================
//
// Rationale:
// - S55-1 OBSERVE: arena_used_bytes dominates (97-99%)
// - Segment "open frequency" is the root cause, not large cache or TLS
// - Admission control: reduce segment opening by boosting pack reuse + repay
//
// Design:
// - Open Debt: per-shard tracking (opened - freed segments)
// - Watermark: arena_used_slots -> level (L0/L1/L2)
// - Boundary A: epoch tick (light level update)
// - Boundary B: segment open (tries boost + repay if HARD)
// - Hot path: zero cost (event-only)
//
// Target:
// - mstress mean RSS -10% (vs baseline)
// - Speed degradation: -2% or less
// - No increase in alloc_failed

#ifndef HZ3_S55_RETENTION_FROZEN
#define HZ3_S55_RETENTION_FROZEN 0
#endif

// S55-2: Watermark tuning (defaults scale with arena size, but scale lane
// usually wants absolute MB-level thresholds because HZ3_ARENA_SIZE is large).
// NOTE: These are compared against `used_slots * HZ3_SEG_SIZE` (arena/segment
// usage proxy), not RSS directly.
#ifndef HZ3_S55_WM_SOFT_BYTES
#define HZ3_S55_WM_SOFT_BYTES (HZ3_ARENA_SIZE / 4)  // 25%
#endif
#ifndef HZ3_S55_WM_HARD_BYTES
#define HZ3_S55_WM_HARD_BYTES (HZ3_ARENA_SIZE * 35 / 100)  // 35%
#endif
#ifndef HZ3_S55_WM_HYST_BYTES
#define HZ3_S55_WM_HYST_BYTES (HZ3_ARENA_SIZE / 50)  // 2%
#endif

// S55-2: Debt/tries tuning (compile-time)
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
// S55-2B: ReturnPolicyBox - Epoch Repay (Budgeted Version)
// ============================================================================
//
// Purpose:
// - S55-2 showed admission control alone is insufficient (-0.8% RSS)
// - Add periodic repayment at epoch boundary when L2
// - Target: mstress mean RSS -10%, speed within -2%
//
// Design:
// - Boundary: hz3_epoch_force() only (no hot path)
// - Condition: HZ3_S55_RETENTION_FROZEN=1 && level==L2
// - Repay: S47 compact + budgeted segment scan (my_shard only)
// - Throttle: epoch interval (not time-based, simpler)
// - Progress: hz3_arena_free_gen() delta (0 -> immediate exit)
//
// Safety (CRITICAL):
// - my_shard only (thread-safe, no data races)
// - Interval throttle (runaway prevention)
// - Budgeted scan (O(SCAN_SLOTS_BUDGET) fixed, no full arena scan)
// - Progress gated (no infinite loop)
// - L2 only (hot path zero cost at L0/L1)
//
// Rationale for budgeted scan:
// - OLD TRAP: hz3_mem_budget_reclaim_segments() = full arena scan (O(32768 slots))
// - NEW SAFE: hz3_mem_budget_reclaim_segments_budget() = O(256-1024) fixed
// - OLD TRAP: reclaim_medium_runs() = up to 4096 pages pop (heavy in epoch)
// - NEW SAFE: deferred to phase 2 (first try without it)

// S55-2B: Repay epoch interval
// - 1 = every epoch (aggressive, may impact speed)
// - 16 = every 16th epoch (conservative, recommended initial value)
// - 32 = every 32nd epoch (fallback if speed regression)
#ifndef HZ3_S55_REPAY_EPOCH_INTERVAL
#define HZ3_S55_REPAY_EPOCH_INTERVAL 16
#endif

// S55-2B: Segment scan budget (arena slots to scan per repay)
// - 256 = conservative (8MB scan @ 32KB/slot, ~256us)
// - 512 = balanced (16MB scan, ~512us)
// - 1024 = aggressive (32MB scan, ~1ms)
// - CRITICAL: This is O(budget) fixed cost, NOT O(used_slots)
#ifndef HZ3_S55_REPAY_SCAN_SLOTS_BUDGET
#define HZ3_S55_REPAY_SCAN_SLOTS_BUDGET 256
#endif

// S55-2B: Medium runs reclaim budget (DEFERRED, phase 2)
// - Use only if gen_delta==0 persists
// - Further throttled: INTERVAL*4 (e.g., every 64th epoch)
// - 256 = conservative (256 pages = 1MB @ 4KB/page)
// #ifndef HZ3_S55_REPAY_MEDIUM_PAGES_BUDGET
// #define HZ3_S55_REPAY_MEDIUM_PAGES_BUDGET 256
// #endif

// ============================================================================
// S55-3: MediumRunSubreleaseBox - Medium Run Subrelease with madvise
// ============================================================================
//
// Purpose:
// - S55-2B showed segment-level reclaim insufficient (gen_delta==0 cases)
// - Add run-level subrelease to OS via madvise(MADV_DONTNEED)
// - Target: mstress mean RSS -10%, speed within -2%
//
// Design:
// - Boundary: hz3_retention_repay_epoch() only (phase 2, no hot path)
// - Condition: HZ3_S55_3_MEDIUM_SUBRELEASE=1 && gen_delta==0 && INTERVAL*4
// - Reclaim: pop from my_shard central, return to segment, madvise to OS
// - Safety: madvise AFTER hz3_segment_free_run (avoid intrusive list corruption)
//
// Scope:
// - Medium runs only (4KB-32KB, size class 0..7)
// - Small (<= 2KB) excluded (intrusive freelist + madvise = bad)
// - Large excluded (S53 handles separately)
//
// Safety (CRITICAL):
// - my_shard only (thread-safe, no data races)
// - Budget throttle (O(budget) fixed cost)
// - INTERVAL*4 throttle (secondary to segment scan)
// - madvise AFTER central_pop + segment_free_run (list integrity)

// S55-3: Enable medium run subrelease with madvise
// - 0 = disabled (default, opt-in)
// - 1 = enabled (use with caution, test RSS impact first)
#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE
#define HZ3_S55_3_MEDIUM_SUBRELEASE 0
#endif

// S55-3: Medium runs reclaim budget (pages to pop per phase 2 call)
// - 64 = very conservative (256KB)
// - 128 = conservative (512KB)
// - 256 = balanced (1MB, recommended initial value)
// - 512 = aggressive (2MB)
// - CRITICAL: This is O(budget) fixed cost, NOT O(total_central)
#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES
#define HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES 256
#endif

// S55-3: Fail-fast on madvise error (debug/research only)
// - 0 = ignore madvise errors (production default)
// - 1 = abort on madvise error (research/debug, finds bugs fast)
#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST
#define HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST 0
#endif

// S55-3: Additional throttle multiplier relative to S55-2B epoch interval.
//
// S55-3 is invoked from `hz3_retention_repay_epoch()` which itself is throttled by
// `HZ3_S55_REPAY_EPOCH_INTERVAL`. This multiplier lets us further thin S55-3 without
// changing S55-2B behavior.
//
// Example:
// - If `HZ3_S55_REPAY_EPOCH_INTERVAL=4`:
//   - MULT=1 -> S55-3 can run on every repay attempt (when `gen_delta==0`)
//   - MULT=4 -> S55-3 runs at most every 16 epochs at L2 (when `gen_delta==0`)
#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT
#define HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT 1
#endif

// S55-3: Max madvise syscall count per reclaim call (throttle)
// - 16 = very conservative (minimize syscall overhead)
// - 32 = balanced (recommended initial value)
// - 64 = aggressive (more RSS reduction, higher overhead)
// - CRITICAL: Prevents hundreds of 1-page madvise calls which degrade speed
#ifndef HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS
#define HZ3_S55_3_MEDIUM_SUBRELEASE_MAX_CALLS 32
#endif

// ============================================================================
// S55-3B: Delayed Medium Subrelease (Purge Delay)
// ============================================================================
//
// Purpose:
// - S55-3 showed low efficiency (7.4%): 782MB madvised → only 57MB RSS reduction
// - Root cause: Returned runs are immediately reused (page faults bring them back)
// - Solution: Queue freed runs with epoch timestamp, only madvise runs that
//   remain free after delay_epochs (avoid wasting syscalls on "hot" runs)
// - Target: mstress mean RSS -10% (vs -6.7% in S55-3)
//
// Design:
// - TLS delayed purge queue (SPSC ring, 256 entries per thread)
// - Enqueue: hz3_segment_free_run() → TLS queue (seg_base, page_idx, pages, t_repay_epoch_counter)
// - Process: epoch boundary → age check → still-free check → madvise or discard
// - Safety: TLS SPSC (no cross-thread access), bitmap check before madvise, MAX_CALLS throttle
//
// Box Theory:
// - Boundary: hz3_retention_repay_epoch() ONLY (both enqueue and process called here)
// - No hot path impact (epoch-only, gen_delta==0 condition)
// - CRITICAL: madvise AFTER bitmap check (avoid touching reused runs)
//
// Concurrent Safety:
// - TLS queue: One per thread, no atomics needed (SPSC guarantee)
// - Epoch stamp: Uses t_repay_epoch_counter (thread-local, no global atomic++)
// - Still-free check: Assumes free_bits updated by my_shard only (scale lane: shards==threads)
//   - CRITICAL: If shard collision possible, this is a data race
//   - Mitigation: scale lane enforces shards==threads, or add lightweight lock around bitmap check

// S55-3B: Enable delayed purge queue (default OFF, opt-in)
// - 0 = disabled (fallback to S55-3 immediate madvise)
// - 1 = enabled (use with caution, test RSS impact first)
#ifndef HZ3_S55_3B_DELAYED_PURGE
#define HZ3_S55_3B_DELAYED_PURGE 0
#endif

// S55-3B: Delay epochs before madvise (aging threshold)
// - 2 = very aggressive (short delay, more madvise)
// - 4 = balanced (recommended initial value)
// - 8 = conservative (longer delay, fewer syscalls)
// - CRITICAL: Too small → same as S55-3 (no effect), too large → delayed RSS reduction
#ifndef HZ3_S55_3B_DELAY_EPOCHS
#define HZ3_S55_3B_DELAY_EPOCHS 4
#endif

// S55-3B: Queue size per shard (power-of-2 for bitwise AND optimization)
// - 128 = minimal (512KB worth of runs, may drop on overflow)
// - 256 = balanced (1MB worth of runs, recommended)
// - 512 = aggressive (2MB worth of runs, more memory overhead)
// - CRITICAL: Must be power-of-2, overflow behavior is drop (not block)
#ifndef HZ3_S55_3B_QUEUE_SIZE
#define HZ3_S55_3B_QUEUE_SIZE 256
#endif

// S55-3B: Drop behavior on queue overflow
// - 0 = drop oldest (default, FIFO eviction)
// - 1 = drop newest (fail enqueue, keep old entries)
#ifndef HZ3_S55_3B_DROP_NEWEST_ON_OVERFLOW
#define HZ3_S55_3B_DROP_NEWEST_ON_OVERFLOW 0
#endif

// S55-3B/S55-3 gate: Require gen_delta==0 to enqueue/process medium subrelease.
// - 1 = only run when segment-level reclaim made no progress (default, conservative)
// - 0 = run even if segment-level reclaim progressed (research / profiling)
#ifndef HZ3_S55_3_REQUIRE_GEN_DELTA0
#define HZ3_S55_3_REQUIRE_GEN_DELTA0 1
#endif

// ============================================================================
// S45-FOCUS: Focused reclaim for arena exhaustion recovery
// ============================================================================
//
// Rationale:
// - reclaim_medium_runs scatters reclaimed pages across many segments
// - No single segment reaches free_pages == 512 (fully free)
// - Focus reclaim on ONE segment to ensure arena slot release
//
// Design:
// - Sample objects from central (multiple size classes)
// - Group by seg_base, pick best (most pages_sum + owner match)
// - Return only best seg's runs to segment, push others back to central
// - If free_pages == 512, call hz3_segment_free() to release arena slot

#ifndef HZ3_S45_FOCUS_RECLAIM
#define HZ3_S45_FOCUS_RECLAIM 0  // default OFF for easy rollback
#endif

// S45-FOCUS: Multi-pass and remote flush (enhanced recovery)
// PASSES: max passes for focus_reclaim loop (0 progress -> break)
#ifndef HZ3_S45_FOCUS_PASSES
#define HZ3_S45_FOCUS_PASSES 4
#endif

// EMERGENCY_FLUSH_REMOTE: flush remote stash to central before focus_reclaim
// Requires HZ3_PTAG_DSTBIN_ENABLE=1
#ifndef HZ3_S45_EMERGENCY_FLUSH_REMOTE
#define HZ3_S45_EMERGENCY_FLUSH_REMOTE 0  // default OFF, scale lane enables
#endif

// S45-FOCUS: Sampling limits
// NOTE: Need enough samples to potentially fill a 512-page segment.
// With medium sc 0-7 (1-8 pages), average ~4.5 pages/obj, need ~114+ objects per segment.
// With 32 potential segments, 512 samples should give ~16 objects per segment.
// Increase to 1024 for better coverage.
#define HZ3_FOCUS_MAX_OBJS 1024  // max objects to sample from central
#define HZ3_FOCUS_MAX_SEGS 32    // max segments to track

// ============================================================================
// S46: Global Pressure Box (arena exhaustion broadcast)
// ============================================================================
//
// Rationale:
// - S45-FOCUS only recovers on the failing thread
// - Other threads still hold objects in local bins / inbox / owner_stash
// - Central pool remains insufficient to free a full segment
//
// Solution:
// - Global pressure epoch triggers ALL threads to flush to central
// - Each thread checks epoch at slow path entry, flushes once per epoch
// - Epoch-based design ensures "flush at least once" guarantee

#ifndef HZ3_ARENA_PRESSURE_BOX
#define HZ3_ARENA_PRESSURE_BOX 0  // default OFF, scale lane enables
#endif

// Budget for owner_stash flush during pressure (per size class)
#ifndef HZ3_OWNER_STASH_FLUSH_BUDGET
#define HZ3_OWNER_STASH_FLUSH_BUDGET 256
#endif

// ============================================================================
// S47: Segment Quarantine (deterministic drain for zero alloc-failed)
// ============================================================================
//
// Rationale:
// - S46 Global Pressure Box still allows 3-9 alloc-failed at R=50 5M iters
// - Root cause: central pop scatters objects across many segments
// - No single segment reaches free_pages == 512 (fully free)
//
// Solution:
// - Focus on ONE segment (draining_seg) until it becomes fully free
// - Quarantine: draining_seg doesn't allocate new runs
// - Drain: only return runs belonging to draining_seg
// - Headroom: trigger early before slot exhaustion
// - TTL: release draining_seg if it doesn't become free within MAX_EPOCHS
//
// Event-only: hot path is not affected (checks only in slow path)

#ifndef HZ3_S47_SEGMENT_QUARANTINE
#define HZ3_S47_SEGMENT_QUARANTINE 0  // default OFF, scale lane enables
#endif

// Headroom: trigger soft compact when used_slots >= total - HEADROOM_SLOTS
#ifndef HZ3_S47_HEADROOM_SLOTS
#define HZ3_S47_HEADROOM_SLOTS 256  // Headroom for preemptive compact
#endif

// Scan budget: how many objects to sample from central
#ifndef HZ3_S47_SCAN_BUDGET_SOFT
#define HZ3_S47_SCAN_BUDGET_SOFT 512
#endif

#ifndef HZ3_S47_SCAN_BUDGET_HARD
#define HZ3_S47_SCAN_BUDGET_HARD 4096
#endif

// TTL: release draining_seg after this many epochs (safety valve)
#ifndef HZ3_S47_QUARANTINE_MAX_EPOCHS
#define HZ3_S47_QUARANTINE_MAX_EPOCHS 8
#endif

// Drain passes: how many times to iterate central drain
#ifndef HZ3_S47_DRAIN_PASSES_SOFT
#define HZ3_S47_DRAIN_PASSES_SOFT 1
#endif

#ifndef HZ3_S47_DRAIN_PASSES_HARD
#define HZ3_S47_DRAIN_PASSES_HARD 4  // Baseline value
#endif

// S47-PolicyBox: adaptive parameter adjustment
// Mode: 0=FROZEN(fixed), 1=OBSERVE(log only), 2=LEARN(adaptive)
#ifndef HZ3_S47_POLICY_MODE
#define HZ3_S47_POLICY_MODE 0  // default FROZEN
#endif

// Bounded wait (usec): max wait time per retry when alloc_full
#ifndef HZ3_S47_PANIC_WAIT_US
#define HZ3_S47_PANIC_WAIT_US 100  // 100 usec
#endif

// S47-2: ArenaGateBox (leader election for thundering herd avoidance)
// When alloc_full, CAS selects one leader to run compaction.
// Followers wait for free_gen change instead of all running compaction.
#ifndef HZ3_S47_ARENA_GATE
#if HZ3_S47_SEGMENT_QUARANTINE
#define HZ3_S47_ARENA_GATE 1  // default ON (only meaningful when S47 is enabled)
#else
#define HZ3_S47_ARENA_GATE 0  // forced OFF when S47 is disabled
#endif
#endif

// Follower wait loop count before self-rescue compaction
#ifndef HZ3_S47_GATE_WAIT_LOOPS
#define HZ3_S47_GATE_WAIT_LOOPS 128  // 64-512 range
#endif

// S47-3: Pinned segment avoidance
// Skip segments that failed to free (pinned by long-lived allocations)
#ifndef HZ3_S47_AVOID_ENABLE
#define HZ3_S47_AVOID_ENABLE 1  // default ON
#endif

// Avoid list size (segments to skip per shard)
#ifndef HZ3_S47_AVOID_SLOTS
#define HZ3_S47_AVOID_SLOTS 4
#endif

// Epochs before segment returns from avoid list
#ifndef HZ3_S47_AVOID_TTL
#define HZ3_S47_AVOID_TTL 8
#endif

// Weight for free_pages in scoring (0=disabled, 2=double weight)
#ifndef HZ3_S47_FREEPAGES_WEIGHT
#define HZ3_S47_FREEPAGES_WEIGHT 2
#endif

// S47-4: Skip soft compaction when max_potential < 512 (hopeless)
#ifndef HZ3_S47_SOFT_SKIP_HOPELESS
#define HZ3_S47_SOFT_SKIP_HOPELESS 1  // default ON
#endif

// ============================================================================
// S49: Segment Packing (existing segment priority)
// ============================================================================
//
// Rationale:
// - New segment allocation increases fragmentation
// - Existing partial segments have unused pages that can be utilized
// - Prioritize packing objects into existing segments before opening new ones
//
// Design:
// - Per-shard partial pool with pages-bucket structure
// - pack_max_fit tracks max contiguous pages, decrements on failure
// - Pressure-only mode (HZ3_S49_PACK_PRESSURE_ONLY) limits overhead
//
// Boundary: slow_alloc_from_segment() entry point only

#ifndef HZ3_S49_SEGMENT_PACKING
#define HZ3_S49_SEGMENT_PACKING 0  // default OFF
#endif

#if HZ3_S49_SEGMENT_PACKING
#ifndef HZ3_S49_PACK_PRESSURE_ONLY
#define HZ3_S49_PACK_PRESSURE_ONLY 0  // S49-2: 常時有効
#endif
#ifndef HZ3_S49_PACK_MAX_PAGES
#define HZ3_S49_PACK_MAX_PAGES 8  // medium 4k-32k 向け
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

// S49-2: Pack stats (debug/observability only)
#ifndef HZ3_S49_PACK_STATS
#define HZ3_S49_PACK_STATS 0  // default OFF
#endif

// S49-2: Pack retry count (alloc_run failure -> try next candidate)
#ifndef HZ3_S49_PACK_RETRIES
#define HZ3_S49_PACK_RETRIES 3
#endif
#endif  // HZ3_S49_SEGMENT_PACKING
