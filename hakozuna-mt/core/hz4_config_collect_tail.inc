// B70: MidPageSupplyReservationBox (default ON)
// Reserve N pages per segment lock acquisition to amortize lock traffic.
#ifndef HZ4_MID_PAGE_SUPPLY_RESV_BOX
#define HZ4_MID_PAGE_SUPPLY_RESV_BOX 1
#endif
#ifndef HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES
#define HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES 16
#endif
#if HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES < 2
#error "HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES must be >= 2"
#endif
#if HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES > HZ4_PAGES_PER_SEG
#error "HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES must be <= HZ4_PAGES_PER_SEG"
#endif

// Mid Stats Box (debug-only, default OFF):
// one-shot atexit counters for mid malloc/free route triage.
#ifndef HZ4_MID_STATS_B1
#define HZ4_MID_STATS_B1 0
#endif
#ifndef HZ4_MID_STATS_B1_SC_HIST
#define HZ4_MID_STATS_B1_SC_HIST 0
#endif
// Mid lock-time stats box (debug-only, default OFF):
// sample lock wait/hold timing and lock-path sub-work to isolate contention source.
#ifndef HZ4_MID_LOCK_TIME_STATS
#define HZ4_MID_LOCK_TIME_STATS 0
#endif
#ifndef HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT
#define HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT 8  // sample 1 / 2^shift lock-path entries
#endif
#ifndef HZ4_MID_LOCK_TIME_STATS_CONTENDED_NS
#define HZ4_MID_LOCK_TIME_STATS_CONTENDED_NS 256u
#endif
#ifndef HZ4_MID_LOCK_TIME_STATS_SC_HIST
#define HZ4_MID_LOCK_TIME_STATS_SC_HIST 0
#endif
#if HZ4_MID_LOCK_TIME_STATS && !HZ4_MID_STATS_B1
#error "HZ4_MID_LOCK_TIME_STATS requires HZ4_MID_STATS_B1=1"
#endif
#if HZ4_MID_LOCK_TIME_STATS && (HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT > 20)
#error "HZ4_MID_LOCK_TIME_STATS_SAMPLE_SHIFT must be <= 20"
#endif

// B37: MidPrefetchedBinHeadBox (opt-in):
// Cache last-hit page hint in lock-path to avoid full bin scan.
#ifndef HZ4_MID_PREFETCHED_BIN_HEAD_BOX
#define HZ4_MID_PREFETCHED_BIN_HEAD_BOX 0  // default OFF (opt-in)
#endif
// Max steps to scan for prev pointer in hint validation.
// If hint is beyond this depth, fall back to full bin scan.
#ifndef HZ4_MID_PREFETCHED_BIN_HEAD_PREV_SCAN_MAX
#define HZ4_MID_PREFETCHED_BIN_HEAD_PREV_SCAN_MAX 2
#endif

// B39: MidObjCacheBox (archived NO-GO, research-only):
// per-thread/per-sc small object cache in mid path to reduce lock-path frequency.
// free side pushes to TLS obj cache first, malloc side pops before lock/bin scan.
#ifndef HZ4_MID_OBJ_CACHE
#define HZ4_MID_OBJ_CACHE 0  // default OFF (opt-in; default gate currently NO-GO)
#endif
#ifndef HZ4_MID_OBJ_CACHE_SLOTS
#define HZ4_MID_OBJ_CACHE_SLOTS 8
#endif
#ifndef HZ4_MID_OBJ_CACHE_BATCH
#define HZ4_MID_OBJ_CACHE_BATCH 0  // default OFF (batch refill kept opt-in)
#endif
#ifndef HZ4_MID_OBJ_CACHE_BATCH_N
#define HZ4_MID_OBJ_CACHE_BATCH_N 4
#endif

// ST-only override (opt-in).
// When enabled, hz4_mid.c uses ST_* values in place of generic MID_* values.
#ifndef HZ4_ST_MID_OBJ_CACHE
#define HZ4_ST_MID_OBJ_CACHE 0
#endif
#ifndef HZ4_ST_MID_OBJ_CACHE_SLOTS
#define HZ4_ST_MID_OBJ_CACHE_SLOTS HZ4_MID_OBJ_CACHE_SLOTS
#endif
#ifndef HZ4_ST_MID_OBJ_CACHE_BATCH
#define HZ4_ST_MID_OBJ_CACHE_BATCH HZ4_MID_OBJ_CACHE_BATCH
#endif
#ifndef HZ4_ST_MID_OBJ_CACHE_BATCH_N
#define HZ4_ST_MID_OBJ_CACHE_BATCH_N HZ4_MID_OBJ_CACHE_BATCH_N
#endif

#if HZ4_MID_OBJ_CACHE && (HZ4_MID_OBJ_CACHE_SLOTS < 1)
#error "HZ4_MID_OBJ_CACHE_SLOTS must be >= 1 when HZ4_MID_OBJ_CACHE=1"
#endif
#if HZ4_MID_OBJ_CACHE && (HZ4_MID_OBJ_CACHE_SLOTS > 64)
#error "HZ4_MID_OBJ_CACHE_SLOTS must be <= 64 when HZ4_MID_OBJ_CACHE=1"
#endif
#if HZ4_MID_OBJ_CACHE_BATCH && !HZ4_MID_OBJ_CACHE
#error "HZ4_MID_OBJ_CACHE_BATCH requires HZ4_MID_OBJ_CACHE=1"
#endif
#if HZ4_MID_OBJ_CACHE_BATCH && (HZ4_MID_OBJ_CACHE_BATCH_N < 1)
#error "HZ4_MID_OBJ_CACHE_BATCH_N must be >= 1 when HZ4_MID_OBJ_CACHE_BATCH=1"
#endif
#if HZ4_MID_OBJ_CACHE_BATCH && (HZ4_MID_OBJ_CACHE_BATCH_N > HZ4_MID_OBJ_CACHE_SLOTS)
#error "HZ4_MID_OBJ_CACHE_BATCH_N must be <= HZ4_MID_OBJ_CACHE_SLOTS"
#endif
#if HZ4_ST_MID_OBJ_CACHE && !HZ4_TLS_SINGLE
#error "HZ4_ST_MID_OBJ_CACHE requires HZ4_TLS_SINGLE=1 (ST only)"
#endif
#if HZ4_ST_MID_OBJ_CACHE && (HZ4_ST_MID_OBJ_CACHE_SLOTS < 1)
#error "HZ4_ST_MID_OBJ_CACHE_SLOTS must be >= 1 when HZ4_ST_MID_OBJ_CACHE=1"
#endif
#if HZ4_ST_MID_OBJ_CACHE && (HZ4_ST_MID_OBJ_CACHE_SLOTS > 64)
#error "HZ4_ST_MID_OBJ_CACHE_SLOTS must be <= 64 when HZ4_ST_MID_OBJ_CACHE=1"
#endif
#if HZ4_ST_MID_OBJ_CACHE_BATCH && !HZ4_ST_MID_OBJ_CACHE
#error "HZ4_ST_MID_OBJ_CACHE_BATCH requires HZ4_ST_MID_OBJ_CACHE=1"
#endif
#if HZ4_ST_MID_OBJ_CACHE_BATCH && (HZ4_ST_MID_OBJ_CACHE_BATCH_N < 1)
#error "HZ4_ST_MID_OBJ_CACHE_BATCH_N must be >= 1 when HZ4_ST_MID_OBJ_CACHE_BATCH=1"
#endif
#if HZ4_ST_MID_OBJ_CACHE_BATCH && (HZ4_ST_MID_OBJ_CACHE_BATCH_N > HZ4_ST_MID_OBJ_CACHE_SLOTS)
#error "HZ4_ST_MID_OBJ_CACHE_BATCH_N must be <= HZ4_ST_MID_OBJ_CACHE_SLOTS"
#endif

// B76: SmallAllocPageInitLiteBox (page初期化軽量化):
// 新規ページ割り当て時にremote系フィールドの初期化をスキップ。
// 新規セグメントから割り当てる場合、remote_head[]等はゼロ初期化済みなので
// 最小限のフィールドのみ初期化すれば十分。
#ifndef HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX
#define HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX 0  // default OFF (opt-in A/B first)
#endif
#ifndef HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT
#define HZ4_SMALL_ALLOC_PAGE_INIT_LITE_VERIFY_SHIFT 8  // 1/256 sampling
#endif
#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX && HZ4_SEG_CREATE_LAZY_INIT_PAGES
#error "HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX is incompatible with HZ4_SEG_CREATE_LAZY_INIT_PAGES (archived NO-GO)"
#endif
#if HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX && HZ4_SEG_REUSE_LAZY_INIT
#error "HZ4_SMALL_ALLOC_PAGE_INIT_LITE_BOX is incompatible with HZ4_SEG_REUSE_LAZY_INIT"
#endif

// ============================================================================
// Force Inline Box (LTO opt-in)
// ============================================================================
// Force always_inline on hot path functions to help LTO cross-module inlining.
#ifndef HZ4_FORCE_INLINE_MALLOC
#define HZ4_FORCE_INLINE_MALLOC 0  // default OFF (opt-in)
#endif

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
