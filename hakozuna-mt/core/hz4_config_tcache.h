// hz4_config_tcache.h - HZ4 TCache/TLS Configuration
#ifndef HZ4_CONFIG_TCACHE_H
#define HZ4_CONFIG_TCACHE_H

// B20: Tcache count tracking (statistics only, keep off on production fast path)
#ifndef HZ4_TCACHE_COUNT
#define HZ4_TCACHE_COUNT 0  // default OFF (hot-path fixed-cost reduction)
#endif

// Tcache count fast decrement (pop path)
#ifndef HZ4_TCACHE_COUNT_FASTDEC
// Fastdec is meaningful only when count tracking is enabled.
#if HZ4_TCACHE_COUNT
#define HZ4_TCACHE_COUNT_FASTDEC 1  // default ON (GO)
#else
#define HZ4_TCACHE_COUNT_FASTDEC 0
#endif
#endif
#if HZ4_TCACHE_COUNT_FASTDEC && !HZ4_TCACHE_COUNT
#error "HZ4_TCACHE_COUNT_FASTDEC requires HZ4_TCACHE_COUNT=1"
#endif

// Tcache prefetch locality (0=OFF, 1..3=locality)
#ifndef HZ4_TCACHE_PREFETCH_LOCALITY
#define HZ4_TCACHE_PREFETCH_LOCALITY 3  // default = previous behavior
#endif
#if (HZ4_TCACHE_PREFETCH_LOCALITY < 0) || (HZ4_TCACHE_PREFETCH_LOCALITY > 3)
#error "HZ4_TCACHE_PREFETCH_LOCALITY must be 0..3"
#endif

// ST Tcache prefetch locality override (opt-in, ST only)
// -1 = follow HZ4_TCACHE_PREFETCH_LOCALITY
#ifndef HZ4_ST_TCACHE_PREFETCH_LOCALITY
#define HZ4_ST_TCACHE_PREFETCH_LOCALITY -1
#endif
#if (HZ4_ST_TCACHE_PREFETCH_LOCALITY < -1) || (HZ4_ST_TCACHE_PREFETCH_LOCALITY > 3)
#error "HZ4_ST_TCACHE_PREFETCH_LOCALITY must be -1..3"
#endif
#if (HZ4_ST_TCACHE_PREFETCH_LOCALITY >= 0) && !HZ4_TLS_SINGLE
#error "HZ4_ST_TCACHE_PREFETCH_LOCALITY requires HZ4_TLS_SINGLE=1 (ST only)"
#endif

// Tcache ObjCache Box (opt-in, MT/ST)
// Small per-bin array cache to avoid freelist pointer chasing.
#ifndef HZ4_TCACHE_OBJ_CACHE
#define HZ4_TCACHE_OBJ_CACHE 0  // default OFF (opt-in)
#endif
#ifndef HZ4_TCACHE_OBJ_CACHE_SLOTS
#define HZ4_TCACHE_OBJ_CACHE_SLOTS 8
#endif
// ST-only alias (legacy)
#ifndef HZ4_ST_TCACHE_OBJ_CACHE
#define HZ4_ST_TCACHE_OBJ_CACHE 0  // default OFF (opt-in)
#endif
#ifndef HZ4_ST_TCACHE_OBJ_CACHE_SLOTS
#define HZ4_ST_TCACHE_OBJ_CACHE_SLOTS 8
#endif
#if HZ4_ST_TCACHE_OBJ_CACHE && HZ4_TCACHE_OBJ_CACHE
#error "Use only one of HZ4_ST_TCACHE_OBJ_CACHE or HZ4_TCACHE_OBJ_CACHE"
#endif
#if HZ4_ST_TCACHE_OBJ_CACHE
#define HZ4_TCACHE_OBJ_CACHE_ON 1
#define HZ4_TCACHE_OBJ_CACHE_SLOTS_ON HZ4_ST_TCACHE_OBJ_CACHE_SLOTS
#else
#define HZ4_TCACHE_OBJ_CACHE_ON HZ4_TCACHE_OBJ_CACHE
#define HZ4_TCACHE_OBJ_CACHE_SLOTS_ON HZ4_TCACHE_OBJ_CACHE_SLOTS
#endif
#if HZ4_ST_TCACHE_OBJ_CACHE && !HZ4_TLS_SINGLE
#error "HZ4_ST_TCACHE_OBJ_CACHE requires HZ4_TLS_SINGLE=1 (ST only)"
#endif
#if HZ4_TCACHE_OBJ_CACHE_ON && (HZ4_TCACHE_OBJ_CACHE_SLOTS_ON < 1)
#error "HZ4_TCACHE_OBJ_CACHE_SLOTS must be >=1"
#endif

// ============================================================================
// Phase16 ST/Dist Route FastBox (ST-only)
// ============================================================================
// ST free routing short path (skip owner decode / remote route work).
#ifndef HZ4_ST_ROUTE_FASTBOX
#if defined(HZ4_ST_FREE_ROUTE_BOX)
#define HZ4_ST_ROUTE_FASTBOX HZ4_ST_FREE_ROUTE_BOX
#else
#define HZ4_ST_ROUTE_FASTBOX 0
#endif
#endif
#if HZ4_ST_ROUTE_FASTBOX && !HZ4_TLS_SINGLE
#error "HZ4_ST_ROUTE_FASTBOX requires HZ4_TLS_SINGLE=1 (ST only)"
#endif

// ST refill direct path selector (sc-limited).
#ifndef HZ4_ST_REFILL_DIRECT_BOX
#if defined(HZ4_ST_SMALL_REFILL_DIRECT)
#define HZ4_ST_REFILL_DIRECT_BOX HZ4_ST_SMALL_REFILL_DIRECT
#else
#define HZ4_ST_REFILL_DIRECT_BOX 0
#endif
#endif
#ifndef HZ4_ST_REFILL_DIRECT_SC_MAX
#define HZ4_ST_REFILL_DIRECT_SC_MAX (HZ4_SC_MAX - 1)
#endif
#if HZ4_ST_REFILL_DIRECT_BOX && !HZ4_TLS_SINGLE
#error "HZ4_ST_REFILL_DIRECT_BOX requires HZ4_TLS_SINGLE=1 (ST only)"
#endif
#if HZ4_ST_REFILL_DIRECT_BOX && \
    ((HZ4_ST_REFILL_DIRECT_SC_MAX < HZ4_SC_MIN) || (HZ4_ST_REFILL_DIRECT_SC_MAX >= HZ4_SC_MAX))
#error "HZ4_ST_REFILL_DIRECT_SC_MAX must be in [HZ4_SC_MIN, HZ4_SC_MAX-1]"
#endif

// One-shot ST counters for Phase16 mechanism lock.
#ifndef HZ4_ST_STATS_B1
#define HZ4_ST_STATS_B1 0
#endif

// One-shot small/tcache route counters for guard lane diagnosis.
#ifndef HZ4_SMALL_STATS_B19
#define HZ4_SMALL_STATS_B19 0
#endif

// Local-free metadata path (opt-in legacy/research toggle).
// Keep explicit default to avoid implicit undefined=0 behavior in dependent boxes.
#ifndef HZ4_LOCAL_FREE_META
#define HZ4_LOCAL_FREE_META 0  // default OFF (opt-in)
#endif

// B21: free route small-first legacy dispatch (opt-in).
#ifndef HZ4_FREE_ROUTE_SMALL_FIRST_BOX
#define HZ4_FREE_ROUTE_SMALL_FIRST_BOX 0
#endif

// B24: free route large-candidate guard.
// Skip hz4_large_header_valid() for pointers that cannot be large by
// low-cost alignment checks. Keeps legacy large validation as-is for candidates.
#ifndef HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX
#define HZ4_FREE_ROUTE_LARGE_CANDIDATE_GUARD_BOX 0  // default OFF (opt-in A/B first)
#endif
#ifndef HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK
#define HZ4_FREE_ROUTE_LARGE_CANDIDATE_ALIGN_MASK 0xFFFu  // 4KiB page alignment
#endif

// B25: Safe Arena-First Route Box.
// Use an OS segment registry to detect pointers that are definitely from HZ4
// segment mappings. For those pointers, skip large header validation and route
// directly through small/mid magic checks (safe because page base is mapped).
#ifndef HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX
#define HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX 0  // default OFF (opt-in A/B first)
#endif
#ifndef HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS
#define HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS (1u << 15)  // 32768 entries
#endif
#ifndef HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE
#define HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE 8
#endif
#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX && \
    ((HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS < 1024) || \
     ((HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS & (HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS - 1u)) != 0))
#error "HZ4_FREE_ROUTE_SEGMENT_REGISTRY_SLOTS must be power-of-two and >=1024"
#endif
#if HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX && (HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE < 1)
#error "HZ4_FREE_ROUTE_SEGMENT_REGISTRY_PROBE must be >=1"
#endif

// B26: free-route observability box (one-shot counters at process exit).
#ifndef HZ4_FREE_ROUTE_STATS_B26
#define HZ4_FREE_ROUTE_STATS_B26 0
#endif

// B27: FreeRouteOrderGateBox (thread-local route order gate).
// Dynamically switch legacy free-route order between:
//   large-first      (preserve large-heavy behavior)
//   small/mid-first  (avoid large validate on low-large lanes)
#ifndef HZ4_FREE_ROUTE_ORDER_GATEBOX
#define HZ4_FREE_ROUTE_ORDER_GATEBOX 0  // default OFF (opt-in A/B first)
#endif
#ifndef HZ4_FREE_ROUTE_ORDER_WINDOW
#define HZ4_FREE_ROUTE_ORDER_WINDOW 1024
#endif
#ifndef HZ4_FREE_ROUTE_ORDER_HI_PCT
#define HZ4_FREE_ROUTE_ORDER_HI_PCT 20
#endif
#ifndef HZ4_FREE_ROUTE_ORDER_LO_PCT
#define HZ4_FREE_ROUTE_ORDER_LO_PCT 5
#endif
#ifndef HZ4_FREE_ROUTE_ORDER_HOLD_WINDOWS
#define HZ4_FREE_ROUTE_ORDER_HOLD_WINDOWS 4
#endif
#ifndef HZ4_FREE_ROUTE_ORDER_START_LARGE_FIRST
#define HZ4_FREE_ROUTE_ORDER_START_LARGE_FIRST 1
#endif
// one-shot stats for gate windows/switches (window-end aggregation only)
#ifndef HZ4_FREE_ROUTE_ORDER_GATE_STATS_B27
#define HZ4_FREE_ROUTE_ORDER_GATE_STATS_B27 0
#endif
#if HZ4_FREE_ROUTE_ORDER_GATEBOX && (HZ4_FREE_ROUTE_ORDER_WINDOW < 16)
#error "HZ4_FREE_ROUTE_ORDER_WINDOW must be >=16"
#endif
#if HZ4_FREE_ROUTE_ORDER_GATEBOX && (HZ4_FREE_ROUTE_ORDER_HI_PCT > 100)
#error "HZ4_FREE_ROUTE_ORDER_HI_PCT must be <=100"
#endif
#if HZ4_FREE_ROUTE_ORDER_GATEBOX && (HZ4_FREE_ROUTE_ORDER_LO_PCT > 100)
#error "HZ4_FREE_ROUTE_ORDER_LO_PCT must be <=100"
#endif
#if HZ4_FREE_ROUTE_ORDER_GATEBOX && (HZ4_FREE_ROUTE_ORDER_LO_PCT > HZ4_FREE_ROUTE_ORDER_HI_PCT)
#error "HZ4_FREE_ROUTE_ORDER_LO_PCT must be <= HZ4_FREE_ROUTE_ORDER_HI_PCT"
#endif
#if HZ4_FREE_ROUTE_ORDER_GATEBOX && \
    ((HZ4_FREE_ROUTE_ORDER_START_LARGE_FIRST != 0) && (HZ4_FREE_ROUTE_ORDER_START_LARGE_FIRST != 1))
#error "HZ4_FREE_ROUTE_ORDER_START_LARGE_FIRST must be 0 or 1"
#endif

// B50: Small/Front SuperFast Route Box (opt-in).
// In local-only windows, probe small page first and skip costly large validate
// when pointer is clearly from small page.
#ifndef HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX
#define HZ4_FREE_ROUTE_SUPERFAST_SMALL_LOCAL_BOX 0
#endif

// B51: FreeRouteTLSPageKindCacheBox (opt-in).
// Keep a tiny TLS direct-mapped cache of page-base -> route kind(small/mid)
// and probe it before expensive legacy large validation.
#ifndef HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX
#define HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX 0
#endif
#ifndef HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS
#define HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS 64
#endif
#if HZ4_FREE_ROUTE_PAGETAG_BACKFILL_BOX && \
    ((HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS < 16) || \
     ((HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS & (HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS - 1u)) != 0))
#error "HZ4_FREE_ROUTE_PAGETAG_BACKFILL_SLOTS must be power-of-two and >=16"
#endif

// B35: SmallFreeMetaDirectDecodeBox (opt-in).
// In meta-separated small free route, decode sc/owner directly from page_meta
// instead of PTAG32-lite load+bit decode.
#ifndef HZ4_SMALL_FREE_META_DIRECT_BOX
#define HZ4_SMALL_FREE_META_DIRECT_BOX 0  // default OFF (opt-in A/B first)
#endif
#if HZ4_SMALL_FREE_META_DIRECT_BOX && !HZ4_PAGE_META_SEPARATE
#error "HZ4_SMALL_FREE_META_DIRECT_BOX requires HZ4_PAGE_META_SEPARATE=1"
#endif

// B36: FreeRouteLargeValidateFuseBox (archived NO-GO).
// Kept as reference; non-default enable is blocked by hz4_config_archived.h
// unless HZ4_ALLOW_ARCHIVED_BOXES=1.
#ifndef HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX
#define HZ4_FREE_ROUTE_LARGE_VALIDATE_FUSE_BOX 0
#endif

// B38: LocalFreeMetaTcacheFirstBox (research / opt-in)
// Owner free path: try no-touch tcache obj-cache first, then local_free_meta index push.
// This reduces idx compute overhead on every free while keeping obj->next untouched in fast path.
#ifndef HZ4_LOCAL_FREE_META_TCACHE_FIRST
#define HZ4_LOCAL_FREE_META_TCACHE_FIRST 0  // default OFF (opt-in)
#endif
// Mode:
//   0 = always enable tcache-first (legacy behavior)
//   1 = enable only while rbuf gate is ON
//   2 = enable only while rbuf gate is OFF
#ifndef HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE
#define HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE 0
#endif
// Page select (opt-in):
//   0 = disabled (legacy)
//   1 = enable only on selected pages (used/decommit state)
#ifndef HZ4_LOCAL_FREE_META_TCACHE_FIRST_PAGESEL
#define HZ4_LOCAL_FREE_META_TCACHE_FIRST_PAGESEL 0
#endif
// Minimum used_count (after free-side decrement) to allow tcache-first.
// 1 means "skip empty pages".
#ifndef HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED
#define HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED 1
#endif
// Maximum used_count to allow tcache-first.
// 0 means "no upper bound".
#ifndef HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED
#define HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED 0
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST && !HZ4_LOCAL_FREE_META
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST requires HZ4_LOCAL_FREE_META=1"
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST && !HZ4_TCACHE_OBJ_CACHE_ON
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST requires HZ4_TCACHE_OBJ_CACHE_ON=1"
#endif
#if (HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE < 0) || (HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE > 2)
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE must be 0..2"
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE && !(HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX)
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST_GATE_MODE!=0 requires HZ4_REMOTE_PAGE_RBUF=1 and HZ4_REMOTE_PAGE_RBUF_GATEBOX=1"
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST_PAGESEL && !HZ4_LOCAL_FREE_META_TCACHE_FIRST
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST_PAGESEL requires HZ4_LOCAL_FREE_META_TCACHE_FIRST=1"
#endif
#if (HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED < 0) || (HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED > 65535)
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED must be 0..65535"
#endif
#if (HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED < 0) || (HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED > 65535)
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED must be 0..65535"
#endif
#if HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED && \
    (HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED < HZ4_LOCAL_FREE_META_TCACHE_FIRST_MIN_USED)
#error "HZ4_LOCAL_FREE_META_TCACHE_FIRST_MAX_USED must be >= MIN_USED when enabled"
#endif

// Phase 14 (TagCacheBox) は NO-GO → archive/research に移動済み。

// Phase 12 (Magazine TCache) は NO-GO → archive/research に移動済み。

// P4.1: Collect list mode (eliminate out[] array)
// P4.1b result: +8.6% → GO, default ON
#ifndef HZ4_COLLECT_LIST
#define HZ4_COLLECT_LIST 1  // default ON (GO: +8.6%)
#endif

// Refill tail: single return path via goto (reduce duplicate tcache_pop)
#ifndef HZ4_REFILL_GOTO
#define HZ4_REFILL_GOTO 0  // default OFF (A/B)
#endif


// Phase 13 (BumpSlabBox) は NO-GO → archive/research に移動済み。

// ============================================================================
// Carry slots: multiple remainder slots to reduce requeue/pushback
// ============================================================================
#ifndef HZ4_CARRY_SLOTS
#define HZ4_CARRY_SLOTS 4
#endif

// Carry policy: if carry remains after consume, skip segq to reduce overhead
#ifndef HZ4_CARRY_SKIP_SEGQ
#define HZ4_CARRY_SKIP_SEGQ 1
#endif

#endif // HZ4_CONFIG_TCACHE_H
