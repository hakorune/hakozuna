// hz4_config_core.h - HZ4 Core Configuration (System/Arch/Global)
// Box Theory: すべての knob は #ifndef で上書き可能
#ifndef HZ4_CONFIG_CORE_H
#define HZ4_CONFIG_CORE_H

// ============================================================================
// System / Arch / Global Settings
// ============================================================================

// Archived Box Guard: NO-GO になった機能を有効にする場合のみ 1 にする
// 詳細は hakozuna/hz4/docs/HZ4_ARCHIVED_BOXES.md を参照
#ifndef HZ4_ALLOW_ARCHIVED_BOXES
#define HZ4_ALLOW_ARCHIVED_BOXES 0
#endif

// Define defaults for archived boxes (0/default).
// NOTE: Some scripts pass `-DXXX=0`; guard checks must be value-based (not `#ifdef`).
#ifndef HZ4_INBOX_ONLY
#define HZ4_INBOX_ONLY 0
#endif
#ifndef HZ4_SEG_ACQ_GUARDBOX
#define HZ4_SEG_ACQ_GUARDBOX 0
#endif
#ifndef HZ4_SEG_ACQ_GATEBOX
#define HZ4_SEG_ACQ_GATEBOX 0
#endif
#ifndef HZ4_XFER_CACHE
#define HZ4_XFER_CACHE 0
#endif
#ifndef HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT
#define HZ4_CPH_PUSH_EMPTY_NO_DECOMMIT 0
#endif
#ifndef HZ4_REMOTE_FLUSH_FASTPATH_MAX
#define HZ4_REMOTE_FLUSH_FASTPATH_MAX 4
#endif
// Stage5: RemoteFlushNoClearNextBox (archived NO-GO)
#ifndef HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT
#define HZ4_REMOTE_FLUSH_NO_CLEAR_NEXT 0
#endif
// Stage5: MetaPrefaultBox (archived NO-GO)
#ifndef HZ4_META_PREFAULTBOX
#define HZ4_META_PREFAULTBOX 0
#endif
// Stage5: SegCreateLazyInitPagesBox (archived NO-GO)
#ifndef HZ4_SEG_CREATE_LAZY_INIT_PAGES
#define HZ4_SEG_CREATE_LAZY_INIT_PAGES 0
#endif
// Stage5: BumpFreeMetaAllowUnderGateBox (archived NO-GO)
#ifndef HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE
#define HZ4_BUMP_FREE_META_ALLOW_UNDER_RBUF_GATE 0
#endif

// TLS direct access (Phase6 GO):
// inline getter + direct __thread access for hz4_tls_get() hot path.
#ifndef HZ4_TLS_DIRECT
#define HZ4_TLS_DIRECT 1
#endif

// SegAcquire budgets (kept even when boxes are archived).
// - Also referenced by non-archived CoolingBox knobs.
#ifndef HZ4_SEG_ACQ_BUDGET
#define HZ4_SEG_ACQ_BUDGET 256  // per-epoch seg_acq budget
#endif
#ifndef HZ4_SEG_ACQ_GUARD_COOLDOWN
#define HZ4_SEG_ACQ_GUARD_COOLDOWN 64  // collect_count delta (minimum between guards)
#endif
#ifndef HZ4_SEG_ACQ_GATE_BUDGET
#define HZ4_SEG_ACQ_GATE_BUDGET 64  // seg_acqがこの回数を超えたら発火
#endif
#ifndef HZ4_SEG_ACQ_GATE_COOLDOWN
#define HZ4_SEG_ACQ_GATE_COOLDOWN 64  // collect_count差分の最小間隔
#endif
#ifndef HZ4_SEG_ACQ_GATE_COLLECT_PASSES
#define HZ4_SEG_ACQ_GATE_COLLECT_PASSES 2  // collectを追加で回す回数
#endif

// ============================================================================
// Page Metadata Configuration (default: SEPARATE=1 for -O2 stability)
// ============================================================================
#ifndef HZ4_PAGE_META_SEPARATE
#define HZ4_PAGE_META_SEPARATE 1  // default ON (required for -O2 stability)
#endif

// ============================================================================
// Core Sizes and Limits
// ============================================================================

// Small size classes (power-of-2 based)
#ifndef HZ4_SC_MIN
#define HZ4_SC_MIN 1  // 8 bytes (sc 0 is sentinel)
#endif
#ifndef HZ4_SC_MAX
#define HZ4_SC_MAX 128  // 16B aligned classes for 16..2048B
#endif

// Page/Segment sizes (must be consistent)
// NOTE: These defaults match the original monolithic hz4_config.h
#ifndef HZ4_PAGE_SHIFT
#define HZ4_PAGE_SHIFT 16  // 64KB pages
#endif
#ifndef HZ4_PAGE_SIZE
#define HZ4_PAGE_SIZE (1ULL << HZ4_PAGE_SHIFT)
#endif

#ifndef HZ4_SEG_SHIFT
#define HZ4_SEG_SHIFT 20  // 1MB segments
#endif
#ifndef HZ4_SEG_SIZE
#define HZ4_SEG_SIZE (1ULL << HZ4_SEG_SHIFT)
#endif

// Derived segment constants (used by seg.h and others)
#define HZ4_PAGES_PER_SEG (HZ4_SEG_SIZE / HZ4_PAGE_SIZE)  // = 16 (1MB/64KB)
#define HZ4_PAGES_USABLE  (HZ4_PAGES_PER_SEG - 1)         // = 15 (page0 reserved)
#define HZ4_PAGE_IDX_MIN  1                              // minimum valid page_idx
#define HZ4_PAGEWORDS ((HZ4_PAGES_PER_SEG + 63) / 64)    // bitmap words for pending bits

// ============================================================================
// Large Path Cache (global per-page-count cache)
// ============================================================================
#ifndef HZ4_LARGE_CACHE_BOX
#define HZ4_LARGE_CACHE_BOX 1
#endif

// Small/medium-heavy lanes call hz4_large_header_valid() frequently from free route.
// Large allocations have header at mmap base (OS page aligned), so we can reject most
// small/mid pointers before touching header memory.
#ifndef HZ4_LARGE_HEADER_ALIGN_FILTER_BOX
#define HZ4_LARGE_HEADER_ALIGN_FILTER_BOX 0  // default OFF (opt-in; A/B gate first)
#endif
#ifndef HZ4_LARGE_HEADER_ALIGN_FILTER_MASK
#define HZ4_LARGE_HEADER_ALIGN_FILTER_MASK 0xFFFu  // 4KiB page mask
#endif
#ifndef HZ4_LARGE_CACHE_SLOTS
#define HZ4_LARGE_CACHE_SLOTS 8
#endif
#ifndef HZ4_LARGE_CACHE_MAX_PAGES
#define HZ4_LARGE_CACHE_MAX_PAGES 16  // covers 64KB..1MB (64KB pages)
#endif
// Large Span Cache Box (opt-in, all lanes)
// Cache multi-page large allocations in TLS buckets (page-count keyed).
#ifndef HZ4_LARGE_SPAN_CACHE
#define HZ4_LARGE_SPAN_CACHE 0  // default OFF (opt-in)
#endif
#ifndef HZ4_LARGE_SPAN_MAX_PAGES
#define HZ4_LARGE_SPAN_MAX_PAGES 16
#endif
#ifndef HZ4_LARGE_SPAN_SLOTS
#define HZ4_LARGE_SPAN_SLOTS 2
#endif
#ifndef HZ4_LARGE_SPAN_CACHE_GATEBOX
#define HZ4_LARGE_SPAN_CACHE_GATEBOX 1  // default ON (when LARGE_SPAN_CACHE=1)
#endif
#ifndef HZ4_LARGE_SPAN_CACHE_GATE_TID_MAX
#define HZ4_LARGE_SPAN_CACHE_GATE_TID_MAX 1  // low-thread IDs only by default
#endif
#ifndef HZ4_LARGE_SPAN_CACHE_GATE_ALLOW_WHEN_RBUF_ON
#define HZ4_LARGE_SPAN_CACHE_GATE_ALLOW_WHEN_RBUF_ON 0
#endif
// ST-only legacy span cache knobs (opt-in)
#ifndef HZ4_ST_LARGE_SPAN_CACHE
#define HZ4_ST_LARGE_SPAN_CACHE 0
#endif
#ifndef HZ4_ST_LARGE_SPAN_MAX_PAGES
#define HZ4_ST_LARGE_SPAN_MAX_PAGES 16
#endif
#ifndef HZ4_ST_LARGE_SPAN_SLOTS
#define HZ4_ST_LARGE_SPAN_SLOTS 2
#endif
#ifndef HZ4_LARGE_OVERFLOW_TLS_BOX
#define HZ4_LARGE_OVERFLOW_TLS_BOX 1  // S214 GO: default ON
#endif
#ifndef HZ4_LARGE_OVERFLOW_TLS_SLOTS
#define HZ4_LARGE_OVERFLOW_TLS_SLOTS 2
#endif
#ifndef HZ4_LARGE_BATCH_ACQUIRE_BOX
#define HZ4_LARGE_BATCH_ACQUIRE_BOX 0  // S216 opt-in: one mmap acquires multiple same-size large blocks
#endif
#ifndef HZ4_LARGE_BATCH_ACQUIRE_BLOCKS
#define HZ4_LARGE_BATCH_ACQUIRE_BLOCKS 8
#endif
#ifndef HZ4_LARGE_BATCH_ACQUIRE_MAX_PAGES
#define HZ4_LARGE_BATCH_ACQUIRE_MAX_PAGES 2  // target 64KB/128KB large band
#endif
#ifndef HZ4_LARGE_BATCH_ACQUIRE_ON_FAILURE_ONLY
#define HZ4_LARGE_BATCH_ACQUIRE_ON_FAILURE_ONLY 1  // S216-v2: trigger batch only after mmap failure path
#endif
#ifndef HZ4_LARGE_CACHE_SHARDBOX
#define HZ4_LARGE_CACHE_SHARDBOX 0  // S217 opt-in: shard global large cache by owner shard
#endif
#ifndef HZ4_LARGE_CACHE_SHARDS
#define HZ4_LARGE_CACHE_SHARDS 8  // power-of-two, <=64
#endif
#ifndef HZ4_LARGE_CACHE_SHARD_STEAL_PROBE
#define HZ4_LARGE_CACHE_SHARD_STEAL_PROBE 2  // number of neighbor shards to probe on miss
#endif
// S218-C2: lock-sharded front cache for large path (keeps global cache as fallback)
#ifndef HZ4_LARGE_LOCK_SHARD_LAYER
#define HZ4_LARGE_LOCK_SHARD_LAYER 1  // S218-C2 GO: default ON
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_SHARDS
#define HZ4_LARGE_LOCK_SHARD_SHARDS 8  // power-of-two, <=64
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_SLOTS
#define HZ4_LARGE_LOCK_SHARD_SLOTS 2
#endif
// P2 LockShard Depth Box (default ON, opt-out available):
// Increase lock-shard slot depth only for pages<=2 band to reduce miss rate on cross128.
#ifndef HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX
#define HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX 1
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_SLOTS_P2
#define HZ4_LARGE_LOCK_SHARD_SLOTS_P2 4
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_SLOTS_MAX
#if HZ4_LARGE_LOCK_SHARD_P2_DEPTH_BOX && (HZ4_LARGE_LOCK_SHARD_SLOTS_P2 > HZ4_LARGE_LOCK_SHARD_SLOTS)
#define HZ4_LARGE_LOCK_SHARD_SLOTS_MAX HZ4_LARGE_LOCK_SHARD_SLOTS_P2
#else
#define HZ4_LARGE_LOCK_SHARD_SLOTS_MAX HZ4_LARGE_LOCK_SHARD_SLOTS
#endif
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_MIN_PAGES
#define HZ4_LARGE_LOCK_SHARD_MIN_PAGES 2  // 128KB band and up
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_MAX_PAGES
#define HZ4_LARGE_LOCK_SHARD_MAX_PAGES 4  // up to 256KB total
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_STEAL_PROBE
#define HZ4_LARGE_LOCK_SHARD_STEAL_PROBE 0  // keep 0 by default (no cross-shard scan)
#endif
// S219: LargeSpanHiBandBox (opt-in, pages<=2)
// Intent: reduce OS acquire/release churn on 64KB/128KB bands.
#ifndef HZ4_S219_LARGE_SPAN_HIBAND
#define HZ4_S219_LARGE_SPAN_HIBAND 1
#endif
#ifndef HZ4_S219_LARGE_SPAN_PAGES
#define HZ4_S219_LARGE_SPAN_PAGES 2
#endif
#ifndef HZ4_S219_LARGE_SPAN_SLOTS
#define HZ4_S219_LARGE_SPAN_SLOTS 32
#endif
#ifndef HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE
#define HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE 2
#endif
// S219 follow-up: use slightly deeper steal probe for 64KB/128KB band (pages<=2).
// Default 3 is tuned from RUNS=10 A/B (cross128 +18.6% with guard-safe behavior).
#ifndef HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2
#define HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE_PAGES2 3
#endif
// LargeShardStickyHintBox (opt-in):
// keep a TLS sticky shard hint (per page-band) and probe order as hint->self->steal.
// default OFF for A/B.
#ifndef HZ4_LARGE_LOCK_SHARD_STICKY_HINT_BOX
#define HZ4_LARGE_LOCK_SHARD_STICKY_HINT_BOX 0
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES
#define HZ4_LARGE_LOCK_SHARD_STICKY_HINT_MAX_PAGES HZ4_LARGE_LOCK_SHARD_MAX_PAGES
#endif
// LargeShardStealBudgetBox (archived NO-GO):
// hard-archived by hz4_config_archived.h (requires HZ4_ALLOW_ARCHIVED_BOXES=1).
#ifndef HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX
#define HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_BOX 0
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES
#define HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MAX_PAGES HZ4_S219_LARGE_SPAN_PAGES
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK
#define HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_MISS_STREAK 2
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_COOLDOWN
#define HZ4_LARGE_LOCK_SHARD_STEAL_BUDGET_COOLDOWN 1
#endif
// LargeShardNonEmptyMaskBox (archived NO-GO):
// hard-archived by hz4_config_archived.h (requires HZ4_ALLOW_ARCHIVED_BOXES=1).
#ifndef HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX
#define HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_BOX 0
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES
#define HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_MAX_PAGES HZ4_S219_LARGE_SPAN_PAGES
#endif
#ifndef HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_FALLBACK
#define HZ4_LARGE_LOCK_SHARD_NONEMPTY_MASK_FALLBACK 1
#endif
#ifndef HZ4_S220_LARGE_MMAP_NOALIGN
#define HZ4_S220_LARGE_MMAP_NOALIGN 1  // default ON: use plain mmap for large acquire (no 64KB trim unmaps)
#endif
#ifndef HZ4_S220_LARGE_OWNER_RETURN
#define HZ4_S220_LARGE_OWNER_RETURN 0  // opt-in: route large remote free to allocating-owner shard
#endif
#ifndef HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES
#define HZ4_S220_LARGE_OWNER_RETURN_MIN_PAGES 3  // narrowed scope: keep <=2 pages on existing S219 path
#endif
#if HZ4_S219_LARGE_SPAN_HIBAND
#undef HZ4_LARGE_SPAN_CACHE
#define HZ4_LARGE_SPAN_CACHE 1
#undef HZ4_LARGE_SPAN_MAX_PAGES
#define HZ4_LARGE_SPAN_MAX_PAGES HZ4_S219_LARGE_SPAN_PAGES
#undef HZ4_LARGE_SPAN_SLOTS
#define HZ4_LARGE_SPAN_SLOTS HZ4_S219_LARGE_SPAN_SLOTS
#undef HZ4_LARGE_SPAN_CACHE_GATEBOX
#define HZ4_LARGE_SPAN_CACHE_GATEBOX 0
#undef HZ4_LARGE_LOCK_SHARD_STEAL_PROBE
#define HZ4_LARGE_LOCK_SHARD_STEAL_PROBE HZ4_S219_LARGE_LOCK_SHARD_STEAL_PROBE
#endif
#if HZ4_LARGE_SPAN_CACHE && (HZ4_LARGE_SPAN_MAX_PAGES < 1 || HZ4_LARGE_SPAN_SLOTS < 1)
#error "HZ4_LARGE_SPAN_* must be >=1"
#endif
#if HZ4_LARGE_SPAN_CACHE && HZ4_LARGE_SPAN_CACHE_GATEBOX && (HZ4_LARGE_SPAN_CACHE_GATE_TID_MAX < 1)
#error "HZ4_LARGE_SPAN_CACHE_GATE_TID_MAX must be >=1"
#endif
// LargePageBinLocklessBox (opt-in):
// Unsharded global page-bin for small large-bands (default: <=2 pages).
// Purpose: reduce lock-shard miss cost on cross-thread heavy 64KB/128KB mixes.
#ifndef HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
#define HZ4_LARGE_PAGEBIN_LOCKLESS_BOX 1  // default ON (2026-02-14): cross128 uplift with MT hold
#endif
#ifndef HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES
#define HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES HZ4_S219_LARGE_SPAN_PAGES
#endif
#ifndef HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS
#define HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS 4
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && (HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES < 1)
#error "HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && (HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES > HZ4_LARGE_CACHE_MAX_PAGES)
#error "HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES must be <= HZ4_LARGE_CACHE_MAX_PAGES"
#endif
#if HZ4_LARGE_PAGEBIN_LOCKLESS_BOX && (HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS < 1)
#error "HZ4_LARGE_PAGEBIN_LOCKLESS_SLOTS must be >=1"
#endif
// LargePageBinBatchSwapBox (B15, opt-in):
// batch publish/acquire for low-page large pagebin to reduce shared atomic frequency.
#ifndef HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX
#define HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX 1  // B15 GO: default ON
#endif
#ifndef HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES
#define HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES
#endif
#ifndef HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N
#define HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N 8
#endif
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX && !HZ4_LARGE_PAGEBIN_LOCKLESS_BOX
#error "HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX requires HZ4_LARGE_PAGEBIN_LOCKLESS_BOX=1"
#endif
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX && (HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES < 1)
#error "HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX && (HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES > HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES)
#error "HZ4_LARGE_PAGEBIN_BATCH_SWAP_MAX_PAGES must be <= HZ4_LARGE_PAGEBIN_LOCKLESS_MAX_PAGES"
#endif
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX && (HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N < 2)
#error "HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N must be >=2"
#endif
#if HZ4_LARGE_PAGEBIN_BATCH_SWAP_BOX && (HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N > 64)
#error "HZ4_LARGE_PAGEBIN_BATCH_SWAP_FLUSH_N must be <=64"
#endif
// LargeRemoteFreeBypassSpanCacheBox (B14, opt-in):
// for remote free in low-page large bands, bypass TLS span/overflow retention and
// publish directly to shared large cache path.
#ifndef HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX
#define HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX 0
#endif
#ifndef HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES
#define HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES 2
#endif
#if HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX && (HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES < 1)
#error "HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES must be >=1"
#endif
#if HZ4_LARGE_REMOTE_BYPASS_SPAN_CACHE_BOX && (HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES > HZ4_LARGE_CACHE_MAX_PAGES)
#error "HZ4_LARGE_REMOTE_BYPASS_MAX_PAGES must be <= HZ4_LARGE_CACHE_MAX_PAGES"
#endif
// LargeExtentCacheBox (B16, default ON / opt-out):
// bounded cache for >1MB large allocations (pages > HZ4_LARGE_CACHE_MAX_PAGES).
// purpose: reduce mmap/munmap churn in high-page large bands.
#ifndef HZ4_LARGE_EXTENT_CACHE_BOX
#define HZ4_LARGE_EXTENT_CACHE_BOX 1
#endif
#ifndef HZ4_LARGE_EXTENT_CACHE_MIN_PAGES
#define HZ4_LARGE_EXTENT_CACHE_MIN_PAGES (HZ4_LARGE_CACHE_MAX_PAGES + 1)
#endif
#ifndef HZ4_LARGE_EXTENT_CACHE_MAX_PAGES
#define HZ4_LARGE_EXTENT_CACHE_MAX_PAGES 256
#endif
#ifndef HZ4_LARGE_EXTENT_CACHE_SLOTS
#define HZ4_LARGE_EXTENT_CACHE_SLOTS 2
#endif
#ifndef HZ4_LARGE_EXTENT_CACHE_MAX_BYTES
#define HZ4_LARGE_EXTENT_CACHE_MAX_BYTES (256ULL << 20)
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX && (HZ4_LARGE_EXTENT_CACHE_MIN_PAGES < (HZ4_LARGE_CACHE_MAX_PAGES + 1))
#error "HZ4_LARGE_EXTENT_CACHE_MIN_PAGES must be >= HZ4_LARGE_CACHE_MAX_PAGES+1"
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX && (HZ4_LARGE_EXTENT_CACHE_MAX_PAGES < HZ4_LARGE_EXTENT_CACHE_MIN_PAGES)
#error "HZ4_LARGE_EXTENT_CACHE_MAX_PAGES must be >= HZ4_LARGE_EXTENT_CACHE_MIN_PAGES"
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX && (HZ4_LARGE_EXTENT_CACHE_SLOTS < 1)
#error "HZ4_LARGE_EXTENT_CACHE_SLOTS must be >=1"
#endif
#if HZ4_LARGE_EXTENT_CACHE_BOX && (HZ4_LARGE_EXTENT_CACHE_MAX_BYTES < (1ULL << 20))
#error "HZ4_LARGE_EXTENT_CACHE_MAX_BYTES must be >=1MiB"
#endif
#if HZ4_ST_LARGE_SPAN_CACHE && !HZ4_TLS_SINGLE
#error "HZ4_ST_LARGE_SPAN_CACHE requires HZ4_TLS_SINGLE=1 (ST only)"
#endif
#if HZ4_ST_LARGE_SPAN_CACHE && (HZ4_ST_LARGE_SPAN_MAX_PAGES < 1 || HZ4_ST_LARGE_SPAN_SLOTS < 1)
#error "HZ4_ST_LARGE_SPAN_* must be >=1"
#endif
#ifndef HZ4_LARGE_HOT_CACHE_LAYER
#define HZ4_LARGE_HOT_CACHE_LAYER 0  // S218-B opt-in: extra cache layer for hot large bands
#endif
#ifndef HZ4_LARGE_HOT_CACHE_MAX_PAGES
#define HZ4_LARGE_HOT_CACHE_MAX_PAGES 2  // target 64KB/128KB large bands
#endif
#ifndef HZ4_LARGE_HOT_CACHE_SLOTS
#define HZ4_LARGE_HOT_CACHE_SLOTS 16
#endif
#ifndef HZ4_LARGE_HOT_CACHE_MAX_BYTES
#define HZ4_LARGE_HOT_CACHE_MAX_BYTES (8ULL << 20)
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_BOX
#define HZ4_LARGE_FAIL_RESCUE_BOX 0  // S218-B3 opt-in: mmap-fail rescue from larger cached blocks
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_SCAN_PAGES
#define HZ4_LARGE_FAIL_RESCUE_SCAN_PAGES 2
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_TRIGGER_STREAK
#define HZ4_LARGE_FAIL_RESCUE_TRIGGER_STREAK 2  // S218-B4: require consecutive mmap fails before rescue
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE
#define HZ4_LARGE_FAIL_RESCUE_PRECISION_GATE 0  // S218-B7 opt-in: skip rescue when recent success ratio is too low
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW
#define HZ4_LARGE_FAIL_RESCUE_GATE_WINDOW 8
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT
#define HZ4_LARGE_FAIL_RESCUE_GATE_MIN_SUCCESS_PCT 25
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK
#define HZ4_LARGE_FAIL_RESCUE_GATE_PRESSURE_STREAK 8  // bypass precision gate under sustained fail pressure
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS
#define HZ4_LARGE_FAIL_RESCUE_GATE_COOLDOWN_FAILS 4
#endif
// S218-B5: budgeted rescue (NO-GO archived)
// Keep defaults at B4-equivalent behavior (interval=1, backoff=1).
#ifndef HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL
#define HZ4_LARGE_FAIL_RESCUE_BUDGET_INTERVAL 1
#endif
#ifndef HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF
#define HZ4_LARGE_FAIL_RESCUE_MAX_BACKOFF 1
#endif

// ============================================================================
// Magic values
// ============================================================================
#define HZ4_PAGE_MAGIC 0x48345047  // "H4PG"
#define HZ4_SEG_MAGIC  0x48345347  // "H4SG"
#define HZ4_MID_MAGIC  0x48344D47  // "H4MG"
#define HZ4_LARGE_MAGIC 0x48344C47 // "H4LG"

// ============================================================================
// Page Tag Table (opt-in, archived NO-GO from Phase 6)
// ============================================================================
// HZ4_PAGE_TAG_TABLE: Enable page tag table for fast routing
// Default OFF - previously NO-GO at -3.1% (Phase 6)
// Tag format: bit 31..28=kind, bit 27..16=sc, bit 15..0=owner_tid
#ifndef HZ4_PAGE_TAG_TABLE
#define HZ4_PAGE_TAG_TABLE 0
#endif
#define HZ4_TAG_KIND_UNKNOWN 0
#define HZ4_TAG_KIND_SMALL   1
#define HZ4_TAG_KIND_MID     2

// Arena configuration for page tag table
// 4GB arena with 64KB pages = 64K pages = 256KB tag table
#ifndef HZ4_ARENA_SIZE
#define HZ4_ARENA_SIZE (4ULL * 1024ULL * 1024ULL * 1024ULL)  // 4GB
#endif
#define HZ4_MAX_PAGES (HZ4_ARENA_SIZE / HZ4_PAGE_SIZE)

// ============================================================================
// Stats and Debugging
// ============================================================================

#ifndef HZ4_STATS
#define HZ4_STATS 0  // default OFF (expensive counters)
#endif
#ifndef HZ4_OS_STATS
#define HZ4_OS_STATS 0  // OS-level stats (page faults, etc)
#endif
#ifndef HZ4_OS_STATS_FAST
#define HZ4_OS_STATS_FAST 0  // Fast path stats (even more expensive)
#endif
#ifndef HZ4_FAILFAST
#define HZ4_FAILFAST 0  // Fail-fast assertions
#endif

// ============================================================================
// Fail-fast Macro
// ============================================================================

#if HZ4_FAILFAST
#include <stdio.h>
#include <stdlib.h>
#define HZ4_FAIL(msg) do { \
    fprintf(stderr, "[HZ4_FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
    abort(); \
} while (0)
#else
#define HZ4_FAIL(msg) ((void)0)
#endif

#endif // HZ4_CONFIG_CORE_H
