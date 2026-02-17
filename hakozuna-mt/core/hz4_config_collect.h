// hz4_config_collect.h - HZ4 Collect/Decommit/RSSReturn Configuration
#ifndef HZ4_CONFIG_COLLECT_H
#define HZ4_CONFIG_COLLECT_H

// ============================================================================
// Queue State (segment lifecycle)
// ============================================================================
#define HZ4_QSTATE_IDLE    0
#define HZ4_QSTATE_QUEUED  1
#define HZ4_QSTATE_PROC    2

// ============================================================================
// Budget defaults
// ============================================================================
#ifndef HZ4_COLLECT_OBJ_BUDGET
#define HZ4_COLLECT_OBJ_BUDGET 96  // GO: R=50/+4.9%, R=90/+1.7%
#endif
#ifndef HZ4_COLLECT_OBJ_BUDGET_HI_SC_ENABLE
#define HZ4_COLLECT_OBJ_BUDGET_HI_SC_ENABLE 0  // S218-D1 opt-in: larger collect budget for higher sc
#endif
#ifndef HZ4_COLLECT_OBJ_BUDGET_HI_SC_MIN
#define HZ4_COLLECT_OBJ_BUDGET_HI_SC_MIN 8  // sc>=8 roughly corresponds to >=2KB class boundary
#endif
#ifndef HZ4_COLLECT_OBJ_BUDGET_HI_SC
#define HZ4_COLLECT_OBJ_BUDGET_HI_SC 160
#endif
#if HZ4_COLLECT_OBJ_BUDGET_HI_SC_ENABLE
#if HZ4_COLLECT_OBJ_BUDGET_HI_SC > HZ4_COLLECT_OBJ_BUDGET
#define HZ4_COLLECT_OBJ_BUDGET_MAX HZ4_COLLECT_OBJ_BUDGET_HI_SC
#else
#define HZ4_COLLECT_OBJ_BUDGET_MAX HZ4_COLLECT_OBJ_BUDGET
#endif
#else
#define HZ4_COLLECT_OBJ_BUDGET_MAX HZ4_COLLECT_OBJ_BUDGET
#endif

// Collect prefetch (for list walks in collect path)
#ifndef HZ4_COLLECT_PREFETCH
#define HZ4_COLLECT_PREFETCH 1  // default ON (GO: R=50/R=90 improve)
#endif

#ifndef HZ4_COLLECT_SEG_BUDGET
#define HZ4_COLLECT_SEG_BUDGET 4
#endif

// ============================================================================
// Decommit / Rebuild knobs
// ============================================================================
#ifndef HZ4_PAGE_DECOMMIT
#define HZ4_PAGE_DECOMMIT 0  // default OFF (requires META_SEPARATE)
#endif

#ifndef HZ4_LOCAL_FREE_DECOMMIT
#define HZ4_LOCAL_FREE_DECOMMIT 0  // default OFF (opt-in immediate decommit on local-free)
#endif

#ifndef HZ4_DECOMMIT_OBSERVE
#define HZ4_DECOMMIT_OBSERVE 0  // default OFF (debug prints for decommit path)
#endif

#ifndef HZ4_LOCAL_FREE_OBSERVE
#define HZ4_LOCAL_FREE_OBSERVE 0  // default OFF (debug prints for local free used_count)
#endif

#ifndef HZ4_LOCAL_FREE_FASTDEC
#define HZ4_LOCAL_FREE_FASTDEC 1  // default ON (A/B)
#endif

// B33: SmallFreeUsedDecRelaxedBox.
// Keep local free used_count decrement on the cheapest path when decommit/cph
// ordering constraints are not active.
#ifndef HZ4_ST_FREE_USEDDEC_RELAXED
#define HZ4_ST_FREE_USEDDEC_RELAXED 1  // default ON
#endif
#if HZ4_ST_FREE_USEDDEC_RELAXED && !HZ4_LOCAL_FREE_FASTDEC
#error "HZ4_ST_FREE_USEDDEC_RELAXED requires HZ4_LOCAL_FREE_FASTDEC=1"
#endif

// B21: mark CPH active only on used_count 0->1 transition (opt-in).
// Reduces per-allocation fixed overhead in small local fast path.
#ifndef HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX
#define HZ4_SMALL_ACTIVE_ON_FIRST_ALLOC_BOX 0
#endif

#ifndef HZ4_INBOX_ACCOUNT_FASTDEC
#define HZ4_INBOX_ACCOUNT_FASTDEC 0  // default OFF (A/B)
#endif

#ifndef HZ4_REBUILD_COLD
#define HZ4_REBUILD_COLD 0  // default OFF (A/B)
#endif

#ifndef HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
#define HZ4_TCACHE_PURGE_BEFORE_DECOMMIT 0  // default OFF (opt-in for Phase 1F safety)
#endif

#ifndef HZ4_DECOMMIT_DELAY_QUEUE
#define HZ4_DECOMMIT_DELAY_QUEUE 0  // default OFF (opt-in for Phase 2)
#endif

#ifndef HZ4_DECOMMIT_PROCESS_GUARD
#define HZ4_DECOMMIT_PROCESS_GUARD 1  // default ON (A/B)
#endif

#ifndef HZ4_DECOMMIT_DUEGATEBOX
#define HZ4_DECOMMIT_DUEGATEBOX 0  // default OFF (A/B)
#endif

#ifndef HZ4_DECOMMIT_DELAY_TICKS
#define HZ4_DECOMMIT_DELAY_TICKS 50  // delay in collect_count ticks
#endif

#ifndef HZ4_DECOMMIT_BUDGET_PER_COLLECT
#define HZ4_DECOMMIT_BUDGET_PER_COLLECT 4  // pages processed per collect
#endif

#if HZ4_PAGE_DECOMMIT && !HZ4_PAGE_META_SEPARATE
#error "HZ4_PAGE_DECOMMIT requires HZ4_PAGE_META_SEPARATE=1"
#endif

#if HZ4_LOCAL_FREE_DECOMMIT && !HZ4_PAGE_DECOMMIT
#error "HZ4_LOCAL_FREE_DECOMMIT requires HZ4_PAGE_DECOMMIT=1"
#endif

#if HZ4_TCACHE_PURGE_BEFORE_DECOMMIT && !HZ4_PAGE_DECOMMIT
#error "HZ4_TCACHE_PURGE_BEFORE_DECOMMIT requires HZ4_PAGE_DECOMMIT=1"
#endif

#if HZ4_DECOMMIT_DELAY_QUEUE && !HZ4_PAGE_DECOMMIT
#error "HZ4_DECOMMIT_DELAY_QUEUE requires HZ4_PAGE_DECOMMIT=1"
#endif

// CPH2 + Decommit Queue Integration Flags
#ifndef HZ4_DQ_ENQUEUE_ON_INBOX_ZERO
#define HZ4_DQ_ENQUEUE_ON_INBOX_ZERO 1  // default ON
#endif

#ifndef HZ4_CPH_PREFER_COLD_ON_RSSRETURN
#define HZ4_CPH_PREFER_COLD_ON_RSSRETURN 0  // default OFF (avoid R=90 T=16 hang)
#endif

#ifndef HZ4_CPH_EXTRACT_RELAX_STATE
#define HZ4_CPH_EXTRACT_RELAX_STATE 0  // 0=require HOT, 1=allow extract even if state!=HOT
#endif

#ifndef HZ4_RSSRETURN_QUARANTINEBOX
#define HZ4_RSSRETURN_QUARANTINEBOX 0  // default OFF (rss lane opt-in)
#endif

#ifndef HZ4_RSSRETURN_QUARANTINE_TICKS
#define HZ4_RSSRETURN_QUARANTINE_TICKS 4  // collect_count units
#endif

#ifndef HZ4_RSSRETURN_QUARANTINE_MAX_PAGES
#define HZ4_RSSRETURN_QUARANTINE_MAX_PAGES 8  // per-thread cap
#endif

// ============================================================================
// PressureGateBox: seg_acq pressure-based quarantine control
// ============================================================================
#if HZ4_RSSRETURN
#ifndef HZ4_RSSRETURN_PRESSUREGATEBOX
#define HZ4_RSSRETURN_PRESSUREGATEBOX 0  // default OFF (A/B)
#endif

#ifndef HZ4_RSSRETURN_GATE_TICK_WINDOW
#define HZ4_RSSRETURN_GATE_TICK_WINDOW 64  // 判定間隔（collect_count単位）★デフォルト
// sweep 候補: 32/64/128（Phase A 300k×10 で分散確認）
#endif

#ifndef HZ4_RSSRETURN_GATE_THRESH_SEG_ACQ
#define HZ4_RSSRETURN_GATE_THRESH_SEG_ACQ 1  // ★閾値（window内のseg_acq増分）
// 初期値1: 「このwindowでsegを1回でもacquireしたら圧あり」
// sweep 候補: 1/2/4
#endif

#ifndef HZ4_RSSRETURN_GATE_HOLD_TICKS
#define HZ4_RSSRETURN_GATE_HOLD_TICKS 32  // ★状態保持期間（フラップ防止）
#endif

// Gate 専用 Quarantine パラメータ（既存の #define を上書きしない）
#ifndef HZ4_RSSRETURN_GATE_QUARANTINE_MAX_PAGES
#define HZ4_RSSRETURN_GATE_QUARANTINE_MAX_PAGES 2  // Gate ON 時の最大隔離ページ数
#endif

#ifndef HZ4_RSSRETURN_GATE_QUARANTINE_TICKS
#define HZ4_RSSRETURN_GATE_QUARANTINE_TICKS 1  // Gate ON 時の隔離期間
#endif
#endif

#ifndef HZ4_CPH_DEADLINE_ALIGN_GRACE
#define HZ4_CPH_DEADLINE_ALIGN_GRACE 1  // default ON
#endif
#ifndef HZ4_CPH_SEAL_GRACE
#define HZ4_CPH_SEAL_GRACE 64  // collect_count units
#endif

#ifndef HZ4_CPH_HOT_MAX_PAGES
#define HZ4_CPH_HOT_MAX_PAGES 256  // per sc
#endif

#if HZ4_CPH_2TIER
#ifndef HZ4_CPH_ACTIVE
#define HZ4_CPH_ACTIVE 0
#define HZ4_CPH_SEALING 1
#define HZ4_CPH_HOT 2
#define HZ4_CPH_COLD 3
#endif

#if HZ4_CPH_2TIER && !HZ4_CENTRAL_PAGEHEAP
#error "HZ4_CPH_2TIER requires HZ4_CENTRAL_PAGEHEAP=1"
#endif

#if HZ4_CPH_2TIER && !HZ4_PAGE_META_SEPARATE
#error "HZ4_CPH_2TIER requires HZ4_PAGE_META_SEPARATE=1"
#endif
#endif



// ============================================================================
// Phase 15: RSSReturnBox (epoch + batch + hysteresis)
// ============================================================================
// RSSReturnBox controls "how much/when to return to OS" on top of DelayQueueBox
#ifndef HZ4_RSSRETURN
#if HZ4_DECOMMIT_DELAY_QUEUE && HZ4_PAGE_DECOMMIT && HZ4_PAGE_META_SEPARATE
#define HZ4_RSSRETURN 1  // default ON in RSS lane
#else
#define HZ4_RSSRETURN 0  // default OFF
#endif
#endif

#ifndef HZ4_RSSRETURN_BATCH_PAGES
#define HZ4_RSSRETURN_BATCH_PAGES 8  // max pages per epoch when threshold reached
#endif

#ifndef HZ4_RSSRETURN_THRESH_PAGES
#define HZ4_RSSRETURN_THRESH_PAGES 32  // threshold before batch processing
#endif

#ifndef HZ4_RSSRETURN_SAFETY_PERIOD
#define HZ4_RSSRETURN_SAFETY_PERIOD 256  // fallback: 1 page per PERIOD collects
#endif

// Optional: GateBox for prefer_cold (limits cold-priority attempts)
#ifndef HZ4_RSSRETURN_GATEBOX
#define HZ4_RSSRETURN_GATEBOX 0  // default OFF (A/B)
#endif

#ifndef HZ4_RSSRETURN_GATE_COOLDOWN
#define HZ4_RSSRETURN_GATE_COOLDOWN 256  // collect_count delta between gates
#endif

#ifndef HZ4_RSSRETURN_GATE_MIN_PENDING
#define HZ4_RSSRETURN_GATE_MIN_PENDING 1  // pending >= this to open gate
#endif

// ============================================================================
// Phase 15B: RSSReturn CoolingBox + ReleaseRateBox (A/B, default OFF)
// ============================================================================
#ifndef HZ4_RSSRETURN_COOLINGBOX
#define HZ4_RSSRETURN_COOLINGBOX 0
#endif

#ifndef HZ4_RSSRETURN_COOL_PRESSURE_SEG_ACQ
#define HZ4_RSSRETURN_COOL_PRESSURE_SEG_ACQ 0  // 1: enable seg_acq pressure gating
#endif

#ifndef HZ4_RSSRETURN_COOL_PRESSURE_BUDGET
#define HZ4_RSSRETURN_COOL_PRESSURE_BUDGET HZ4_SEG_ACQ_GATE_BUDGET
#endif

#ifndef HZ4_RSSRETURN_COOL_DELAY_TICKS
#define HZ4_RSSRETURN_COOL_DELAY_TICKS 64
#endif

#ifndef HZ4_RSSRETURN_COOL_SWEEP_BUDGET
#define HZ4_RSSRETURN_COOL_SWEEP_BUDGET 64
#endif

#ifndef HZ4_RSSRETURN_RELEASEBOX
#define HZ4_RSSRETURN_RELEASEBOX 0
#endif

#ifndef HZ4_RSSRETURN_RELEASE_RATE
#define HZ4_RSSRETURN_RELEASE_RATE 1
#endif

#ifndef HZ4_RSSRETURN_RELEASE_PERIOD
#define HZ4_RSSRETURN_RELEASE_PERIOD 64  // 1/N collects. Must be power-of-two.
#endif

#ifndef HZ4_RSSRETURN_RELEASE_BUDGET_MAX
#define HZ4_RSSRETURN_RELEASE_BUDGET_MAX 8
#endif

#if HZ4_RSSRETURN && HZ4_RSSRETURN_RELEASEBOX && ((HZ4_RSSRETURN_RELEASE_PERIOD & (HZ4_RSSRETURN_RELEASE_PERIOD - 1)) != 0)
#error "HZ4_RSSRETURN_RELEASE_PERIOD must be power-of-two"
#endif

// ============================================================================
// Phase 16: DecommitReusePoolBox (avoid seg_acq explosion)
// ============================================================================
// When a page is decommitted (madvise DONTNEED), recycle it in a per-thread pool
// so refill can recommit+rebuild instead of allocating fresh segments.
#ifndef HZ4_DECOMMIT_REUSE_POOL
#define HZ4_DECOMMIT_REUSE_POOL 0  // archived (NO-GO)
#endif
#if HZ4_DECOMMIT_REUSE_POOL
#error "HZ4_DECOMMIT_REUSE_POOL archived (NO-GO). See hakozuna/archive/research/reuse_pool_box/."
#endif

// Phase 16B: SegmentReleaseBox (avoid OOM by releasing fully-empty segments)
#ifndef HZ4_SEG_RELEASE_EMPTY
#if HZ4_PAGE_META_SEPARATE && HZ4_PAGE_DECOMMIT && !HZ4_CENTRAL_PAGEHEAP
#define HZ4_SEG_RELEASE_EMPTY 1  // default ON in RSS lane
#else
#define HZ4_SEG_RELEASE_EMPTY 0
#endif
#endif

// ============================================================================
// SegReuseBox v1: TLS segment reuse pool (reduce page-faults)
// ============================================================================
// Segments that would be released are kept in a TLS pool for reuse.
// This reduces seg_acq and associated page-faults.
// NOTE: Exclusive with RSS lane (HZ4_RSSRETURN). Must have HZ4_SEG_RELEASE_EMPTY enabled.
#ifndef HZ4_SEG_REUSE_POOL
#define HZ4_SEG_REUSE_POOL 0  // default OFF (opt-in for A/B)
#endif

#ifndef HZ4_SEG_REUSE_POOL_MAX
#define HZ4_SEG_REUSE_POOL_MAX 8  // max segments per TLS pool
#endif

#ifndef HZ4_SEG_REUSE_LAZY_INIT
#define HZ4_SEG_REUSE_LAZY_INIT 0  // default OFF (opt-in): skip hz4_seg_init_pages on reuse
#endif

#if HZ4_SEG_REUSE_POOL && !HZ4_SEG_RELEASE_EMPTY
#error "HZ4_SEG_REUSE_POOL requires HZ4_SEG_RELEASE_EMPTY=1"
#endif

// Optional: adapt budget to backlog (pending pages) to avoid "never catches up" under remote-heavy.
// - When pending >= THRESH: budget = clamp(max(BATCH_PAGES, pending / DIV), <= BATCH_MAX_PAGES)
#ifndef HZ4_RSSRETURN_ADAPTIVE_BUDGET
#define HZ4_RSSRETURN_ADAPTIVE_BUDGET 0  // default OFF (A/B)
#endif

#ifndef HZ4_RSSRETURN_BATCH_MAX_PAGES
#define HZ4_RSSRETURN_BATCH_MAX_PAGES 32
#endif

#ifndef HZ4_RSSRETURN_BATCH_DIV
#define HZ4_RSSRETURN_BATCH_DIV 8
#endif

#if HZ4_RSSRETURN && !HZ4_DECOMMIT_DELAY_QUEUE
#error "HZ4_RSSRETURN requires HZ4_DECOMMIT_DELAY_QUEUE=1"
#endif

#if HZ4_RSSRETURN && !HZ4_PAGE_DECOMMIT
#error "HZ4_RSSRETURN requires HZ4_PAGE_DECOMMIT=1"
#endif

#if HZ4_RSSRETURN && HZ4_RSSRETURN_ADAPTIVE_BUDGET && (HZ4_RSSRETURN_BATCH_DIV == 0)
#error "HZ4_RSSRETURN_BATCH_DIV must be >= 1"
#endif

#if HZ4_RSSRETURN && !HZ4_PAGE_META_SEPARATE
#error "HZ4_RSSRETURN requires HZ4_PAGE_META_SEPARATE=1"
#endif

// ============================================================================
// Debug flags
// ============================================================================
#ifndef HZ4_FAILFAST
#define HZ4_FAILFAST 0
#endif

#if HZ4_HANG_OBS && !HZ4_FAILFAST
#error "HZ4_HANG_OBS requires HZ4_FAILFAST=1 (it aborts on hang-like loops)."
#endif
#if HZ4_HANG_OBS
#if (HZ4_HANG_OBS_RECENT & (HZ4_HANG_OBS_RECENT - 1)) != 0
#error "HZ4_HANG_OBS_RECENT must be power-of-two"
#endif
#endif

// Segment full zeroing (mmap already returns zeroed pages)
#ifndef HZ4_SEG_ZERO_FULL
#define HZ4_SEG_ZERO_FULL 0
#endif

// TLS struct merge (reduce indirection)
// A/B result: +1.9% (R=0) → GO
#ifndef HZ4_TLS_MERGE
#define HZ4_TLS_MERGE 1  // default ON (GO)
#endif

// Segment release safety fences (require all relevant knobs to be defined)

// ============================================================================
// Research: Layout/THP Research Box (SegPopulateBox + THP Hint Box)
// ============================================================================
// K1: SegPopulateBox
#ifndef HZ4_SEG_MMAP_POPULATE
#define HZ4_SEG_MMAP_POPULATE 0
#endif

#ifndef HZ4_SEG_MADVISE_POPULATE_WRITE
#define HZ4_SEG_MADVISE_POPULATE_WRITE 0
#endif

// K2: THP Hint Box
#ifndef HZ4_SEG_MADVISE_HUGEPAGE
#define HZ4_SEG_MADVISE_HUGEPAGE 0
#endif

#ifndef HZ4_SEG_MADVISE_NOHUGEPAGE
#define HZ4_SEG_MADVISE_NOHUGEPAGE 0
#endif

#if HZ4_SEG_RELEASE_EMPTY && !HZ4_TCACHE_PURGE_BEFORE_DECOMMIT
#error "HZ4_SEG_RELEASE_EMPTY requires HZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1"
#endif

#if HZ4_SEG_RELEASE_EMPTY && !HZ4_TLS_MERGE
#error "HZ4_SEG_RELEASE_EMPTY requires HZ4_TLS_MERGE=1"
#endif

#if HZ4_SEG_RELEASE_EMPTY && HZ4_PAGE_TAG_TABLE
#error "HZ4_SEG_RELEASE_EMPTY is incompatible with HZ4_PAGE_TAG_TABLE (stale tag risk)"
#endif

#if HZ4_SEG_RELEASE_EMPTY && HZ4_DECOMMIT_REUSE_POOL
#error "HZ4_SEG_RELEASE_EMPTY is incompatible with HZ4_DECOMMIT_REUSE_POOL (stale queue nodes risk)"
#endif

#if HZ4_SEG_RELEASE_EMPTY && HZ4_CENTRAL_PAGEHEAP
#error "HZ4_SEG_RELEASE_EMPTY is incompatible with HZ4_CENTRAL_PAGEHEAP (stale central nodes risk)"
#endif

// ============================================================================
// Phase 20B: TransferCacheBox (hz3 S13-based, lock-free batch stack)
// ============================================================================
#ifndef HZ4_TRANSFERCACHE
#define HZ4_TRANSFERCACHE 0  // default OFF (A/B)
#endif

#ifndef HZ4_TRANSFERCACHE_BATCH
#define HZ4_TRANSFERCACHE_BATCH 32  // objects per batch
#endif

#ifndef HZ4_TRANSFERCACHE_SHARDS
#define HZ4_TRANSFERCACHE_SHARDS 16  // power of two
#endif

#ifndef HZ4_TRANSFERCACHE_NODE_CACHE
#define HZ4_TRANSFERCACHE_NODE_CACHE 8  // per-TLS cache
#endif

// TransferCache GateBox (opt-in, MT lane candidate)
// Gate tcbox trim by remote pressure (rbuf gate) to avoid R50 overhead.
#ifndef HZ4_TRANSFERCACHE_GATEBOX
#define HZ4_TRANSFERCACHE_GATEBOX 0  // default OFF (A/B)
#endif
#ifndef HZ4_TRANSFERCACHE_GATE_ALLOW_WHEN_RBUF_OFF
#define HZ4_TRANSFERCACHE_GATE_ALLOW_WHEN_RBUF_OFF 0
#endif
#ifndef HZ4_TRANSFERCACHE_GATE_POP
#define HZ4_TRANSFERCACHE_GATE_POP 1
#endif

// ============================================================================
// Phase 20: TransferCacheBox (cross-thread batch flow) - ARCHIVED NO-GO
// ============================================================================
#ifndef HZ4_XFER_CACHE
#define HZ4_XFER_CACHE 0  // default OFF (A/B)
#endif

#ifndef HZ4_XFER_BATCH
#define HZ4_XFER_BATCH 32  // batch size (objects)
#endif

#ifndef HZ4_XFER_SHARDS
#define HZ4_XFER_SHARDS 16  // power of two
#endif

#ifndef HZ4_XFER_NODE_CACHE
#define HZ4_XFER_NODE_CACHE 8  // batch nodes cached per thread
#endif

// Stage5-2: Trim-lite Box (perf lane only)
// Move trim/purge fixed cost off the hot path by amortizing across collects.
// default OFF (opt-in). Intended for RSSRETURN=0 builds only.
#ifndef HZ4_TRIM_LITE
#define HZ4_TRIM_LITE 0
#endif
#ifndef HZ4_TRIM_LITE_PERIOD
#define HZ4_TRIM_LITE_PERIOD 64  // collect_count period
#endif
#ifndef HZ4_TRIM_LITE_DEBT_MAX
#define HZ4_TRIM_LITE_DEBT_MAX 64  // object units (approx)
#endif
#if HZ4_TRIM_LITE && HZ4_RSSRETURN
#error "HZ4_TRIM_LITE is perf-lane only (requires HZ4_RSSRETURN=0)"
#endif

// ============================================================================
// Mid Allocator Configuration
// ============================================================================
#ifndef HZ4_MID_ALIGN
#define HZ4_MID_ALIGN 256u
#endif

// Mid max size inline (opt-in)
// Replace hz4_mid_max_size() call with inline computation (avoid PLT in hot path).
#ifndef HZ4_MID_MAX_SIZE_INLINE
#define HZ4_MID_MAX_SIZE_INLINE 0  // default OFF (opt-in)
#endif

// Lock Sharding: Split global mid lock into multiple shards to reduce contention.
#ifndef HZ4_MID_LOCK_SHARDS
#define HZ4_MID_LOCK_SHARDS 16  // default 16 (mid bins lock sharding), power-of-two
#endif

// MidOwnerRemoteQueueBox (opt-in):
// owner thread keeps one private page per-sc (lockless alloc/free on page->free),
// cross-thread free pushes to page->remote_head via atomic CAS only.
// Adopt/release stays on the mid lock boundary.
#ifndef HZ4_MID_OWNER_REMOTE_QUEUE_BOX
#define HZ4_MID_OWNER_REMOTE_QUEUE_BOX 0
#endif

// TLS Local Cache: per-thread per-sizeclass page stash to bypass global lock/bin.
#ifndef HZ4_MID_TLS_CACHE
#define HZ4_MID_TLS_CACHE 1  // default ON
#endif

// Safety gate:
// Without owner-remote queue, cross-thread free can race with lockless TLS-page pop.
// Keep TLS cache OFF by default in that mode; allow explicit unsafe research opt-in.
#ifndef HZ4_MID_TLS_CACHE_UNSAFE_WITHOUT_OWNER
#define HZ4_MID_TLS_CACHE_UNSAFE_WITHOUT_OWNER 0
#endif
#if HZ4_MID_TLS_CACHE && !HZ4_MID_OWNER_REMOTE_QUEUE_BOX && !HZ4_MID_TLS_CACHE_UNSAFE_WITHOUT_OWNER
#undef HZ4_MID_TLS_CACHE
#define HZ4_MID_TLS_CACHE 0
#endif

#ifndef HZ4_MID_TLS_CACHE_DEPTH
#define HZ4_MID_TLS_CACHE_DEPTH 1  // pages per thread per size class
#endif
// NOTE: non-default depth is archived (guarded by hz4_config_archived.h).
#ifndef HZ4_MID_TLS_CACHE_SC_MAX
#define HZ4_MID_TLS_CACHE_SC_MAX ((HZ4_PAGE_SIZE / HZ4_MID_ALIGN) - 1)  // default: all mid classes
#endif
// NOTE: non-default SC gate is archived (guarded by hz4_config_archived.h).
#ifndef HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH
#define HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH 1  // default keeps current behavior (always evict full page)
#endif
// NOTE: non-default full-evict policy is archived (guarded by hz4_config_archived.h).
#if HZ4_MID_TLS_CACHE && (HZ4_MID_TLS_CACHE_DEPTH < 1)
#error "HZ4_MID_TLS_CACHE_DEPTH must be >= 1 when HZ4_MID_TLS_CACHE=1"
#endif
#if HZ4_MID_TLS_CACHE_DEPTH > 255
#error "HZ4_MID_TLS_CACHE_DEPTH must be <= 255"
#endif
#if HZ4_MID_TLS_CACHE && (HZ4_MID_TLS_CACHE_SC_MAX >= (HZ4_PAGE_SIZE / HZ4_MID_ALIGN))
#error "HZ4_MID_TLS_CACHE_SC_MAX out of mid size-class range"
#endif
#if HZ4_MID_TLS_CACHE && (HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH < 1)
#error "HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH must be >= 1"
#endif
#if HZ4_MID_TLS_CACHE && (HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH > HZ4_MID_TLS_CACHE_DEPTH)
#error "HZ4_MID_TLS_CACHE_FULL_EVICT_MIN_DEPTH must be <= HZ4_MID_TLS_CACHE_DEPTH"
#endif

// Mid FreeBatchBox: amortize mid free lock cost by per-thread/per-sc batching.
// default ON.
#ifndef HZ4_MID_FREE_BATCH_BOX
#define HZ4_MID_FREE_BATCH_BOX 1
#endif
#ifndef HZ4_MID_FREE_BATCH_LIMIT
#define HZ4_MID_FREE_BATCH_LIMIT 32
#endif
#if HZ4_MID_FREE_BATCH_BOX && (HZ4_MID_FREE_BATCH_LIMIT < 2)
#error "HZ4_MID_FREE_BATCH_LIMIT must be >= 2 when HZ4_MID_FREE_BATCH_BOX=1"
#endif
// Mid FreeBatchConsumeBox: allow malloc fast path to pop thread-local free-batch objects
// before taking mid shard lock. Objects are staged into alloc-run cache instead
// of direct return to preserve locality. default ON (opt-out available).
#ifndef HZ4_MID_FREE_BATCH_CONSUME_BOX
#define HZ4_MID_FREE_BATCH_CONSUME_BOX 1
#endif
#if HZ4_MID_FREE_BATCH_CONSUME_BOX && !HZ4_MID_FREE_BATCH_BOX
#error "HZ4_MID_FREE_BATCH_CONSUME_BOX requires HZ4_MID_FREE_BATCH_BOX=1"
#endif
#ifndef HZ4_MID_FREE_BATCH_CONSUME_MIN
#define HZ4_MID_FREE_BATCH_CONSUME_MIN 4
#endif
#if HZ4_MID_FREE_BATCH_CONSUME_BOX && (HZ4_MID_FREE_BATCH_CONSUME_MIN < 1)
#error "HZ4_MID_FREE_BATCH_CONSUME_MIN must be >= 1 when HZ4_MID_FREE_BATCH_CONSUME_BOX=1"
#endif
#if HZ4_MID_FREE_BATCH_CONSUME_BOX && (HZ4_MID_FREE_BATCH_CONSUME_MIN > HZ4_MID_FREE_BATCH_LIMIT)
#error "HZ4_MID_FREE_BATCH_CONSUME_MIN must be <= HZ4_MID_FREE_BATCH_LIMIT"
#endif
// Mid AllocRunCacheBox: amortize mid malloc lock cost by per-thread/per-sc stash.
// default ON.
#ifndef HZ4_MID_ALLOC_RUN_CACHE_BOX
#define HZ4_MID_ALLOC_RUN_CACHE_BOX 1
#endif
#ifndef HZ4_MID_ALLOC_RUN_CACHE_LIMIT
#define HZ4_MID_ALLOC_RUN_CACHE_LIMIT 8
#endif
#if HZ4_MID_ALLOC_RUN_CACHE_BOX && (HZ4_MID_ALLOC_RUN_CACHE_LIMIT < 2)
#error "HZ4_MID_ALLOC_RUN_CACHE_LIMIT must be >= 2 when HZ4_MID_ALLOC_RUN_CACHE_BOX=1"
#endif
// B56: ST Mid Local Stack Box (opt-in)
// ST(TLS_SINGLE) only: keep a per-sc fixed array stack of mid objects to
// bypass lock-path and freelist pointer chase for local reuse.
#ifndef HZ4_ST_MID_LOCAL_STACK_BOX
#define HZ4_ST_MID_LOCAL_STACK_BOX 0
#endif
#ifndef HZ4_ST_MID_LOCAL_STACK_SLOTS
#define HZ4_ST_MID_LOCAL_STACK_SLOTS 32
#endif
#if HZ4_ST_MID_LOCAL_STACK_BOX && !HZ4_TLS_SINGLE
#error "HZ4_ST_MID_LOCAL_STACK_BOX requires HZ4_TLS_SINGLE=1"
#endif
#if HZ4_ST_MID_LOCAL_STACK_BOX && (HZ4_ST_MID_LOCAL_STACK_SLOTS < 4)
#error "HZ4_ST_MID_LOCAL_STACK_SLOTS must be >= 4"
#endif
#if HZ4_ST_MID_LOCAL_STACK_BOX && (HZ4_ST_MID_LOCAL_STACK_SLOTS > 255)
#error "HZ4_ST_MID_LOCAL_STACK_SLOTS must be <= 255"
#endif
// B57: Mid Owner Local Stack Box (opt-in)
// MT path only: keep owner-local frees in per-sc TLS array stack to reduce
// lock-path and per-object freelist pointer chase.
#ifndef HZ4_MID_OWNER_LOCAL_STACK_BOX
#define HZ4_MID_OWNER_LOCAL_STACK_BOX 0
#endif
#ifndef HZ4_MID_OWNER_LOCAL_STACK_SLOTS
#define HZ4_MID_OWNER_LOCAL_STACK_SLOTS 16
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_BOX && HZ4_TLS_SINGLE
#error "HZ4_MID_OWNER_LOCAL_STACK_BOX is MT-only (requires HZ4_TLS_SINGLE=0)"
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_BOX && !HZ4_MID_OWNER_REMOTE_QUEUE_BOX
#error "HZ4_MID_OWNER_LOCAL_STACK_BOX requires HZ4_MID_OWNER_REMOTE_QUEUE_BOX=1"
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_BOX && (HZ4_MID_OWNER_LOCAL_STACK_SLOTS < 4)
#error "HZ4_MID_OWNER_LOCAL_STACK_SLOTS must be >= 4"
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_BOX && (HZ4_MID_OWNER_LOCAL_STACK_SLOTS > 255)
#error "HZ4_MID_OWNER_LOCAL_STACK_SLOTS must be <= 255"
#endif
#ifndef HZ4_MID_OWNER_LOCAL_STACK_SC_MAX
#define HZ4_MID_OWNER_LOCAL_STACK_SC_MAX (HZ4_PAGE_SIZE / HZ4_MID_ALIGN)
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_BOX && \
    (HZ4_MID_OWNER_LOCAL_STACK_SC_MAX < 0 || \
     HZ4_MID_OWNER_LOCAL_STACK_SC_MAX > (HZ4_PAGE_SIZE / HZ4_MID_ALIGN))
#error "HZ4_MID_OWNER_LOCAL_STACK_SC_MAX must be in 0..(HZ4_PAGE_SIZE / HZ4_MID_ALIGN)"
#endif
// B58: Mid Owner Local Stack Remote Gate (opt-in)
// When enabled, owner-local stack push/pop gates on remote_count == 0.
// Pages with pending remote objects use the traditional freelist path
// to ensure remote objects are processed promptly.
#ifndef HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE
#define HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE 0
#endif
#if HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE && !HZ4_MID_OWNER_LOCAL_STACK_BOX
#error "HZ4_MID_OWNER_LOCAL_STACK_REMOTE_GATE requires HZ4_MID_OWNER_LOCAL_STACK_BOX=1"
#endif
#ifndef HZ4_MID_OWNER_REMOTE_DRAIN_THRESHOLD
#define HZ4_MID_OWNER_REMOTE_DRAIN_THRESHOLD 8
#endif
#ifndef HZ4_MID_OWNER_REMOTE_DRAIN_PERIOD
#define HZ4_MID_OWNER_REMOTE_DRAIN_PERIOD 64
#endif
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX && (HZ4_MID_OWNER_REMOTE_DRAIN_THRESHOLD < 1)
#error "HZ4_MID_OWNER_REMOTE_DRAIN_THRESHOLD must be >= 1 when HZ4_MID_OWNER_REMOTE_QUEUE_BOX=1"
#endif
#if HZ4_MID_OWNER_REMOTE_QUEUE_BOX && (HZ4_MID_OWNER_REMOTE_DRAIN_PERIOD < 1)
#error "HZ4_MID_OWNER_REMOTE_DRAIN_PERIOD must be >= 1 when HZ4_MID_OWNER_REMOTE_QUEUE_BOX=1"
#endif

// MidLockBurstRefillBox (archived NO-GO):
// on lock-path, refill alloc-run cache from additional bin pages.
// hard-archived by hz4_config_archived.h (requires HZ4_ALLOW_ARCHIVED_BOXES=1).
#ifndef HZ4_MID_LOCK_BURST_REFILL_BOX
#define HZ4_MID_LOCK_BURST_REFILL_BOX 0
#endif
#ifndef HZ4_MID_LOCK_BURST_SCAN_PAGES
#define HZ4_MID_LOCK_BURST_SCAN_PAGES 2
#endif
#ifndef HZ4_MID_LOCK_BURST_MAX_CAPACITY
#define HZ4_MID_LOCK_BURST_MAX_CAPACITY 8
#endif
#if HZ4_MID_LOCK_BURST_REFILL_BOX && !HZ4_MID_ALLOC_RUN_CACHE_BOX
#error "HZ4_MID_LOCK_BURST_REFILL_BOX requires HZ4_MID_ALLOC_RUN_CACHE_BOX=1"
#endif
#if HZ4_MID_LOCK_BURST_REFILL_BOX && (HZ4_MID_LOCK_BURST_SCAN_PAGES < 1)
#error "HZ4_MID_LOCK_BURST_SCAN_PAGES must be >= 1 when HZ4_MID_LOCK_BURST_REFILL_BOX=1"
#endif
#if HZ4_MID_LOCK_BURST_REFILL_BOX && (HZ4_MID_LOCK_BURST_MAX_CAPACITY < 1)
#error "HZ4_MID_LOCK_BURST_MAX_CAPACITY must be >= 1 when HZ4_MID_LOCK_BURST_REFILL_BOX=1"
#endif

// MidPageCreateSuppressBox (archived NO-GO, research opt-in only):
// on lock-path bin miss, drop/reacquire sc lock and re-scan before creating a new page.
// keeps page-create suppression localized to one malloc boundary.
#ifndef HZ4_MID_PAGE_CREATE_SUPPRESS_BOX
#define HZ4_MID_PAGE_CREATE_SUPPRESS_BOX 0
#endif
#ifndef HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY
#define HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY 1
#endif
#if HZ4_MID_PAGE_CREATE_SUPPRESS_BOX && HZ4_MID_LOCK_ELIDE
#error "HZ4_MID_PAGE_CREATE_SUPPRESS_BOX requires HZ4_MID_LOCK_ELIDE=0"
#endif
#if HZ4_MID_PAGE_CREATE_SUPPRESS_BOX && (HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY < 1)
#error "HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY must be >= 1 when HZ4_MID_PAGE_CREATE_SUPPRESS_BOX=1"
#endif
#if HZ4_MID_PAGE_CREATE_SUPPRESS_BOX && (HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY > 4)
#error "HZ4_MID_PAGE_CREATE_SUPPRESS_RETRY must be <= 4 when HZ4_MID_PAGE_CREATE_SUPPRESS_BOX=1"
#endif

// MidPageCreateOutsideSCLockBox (opt-in):
// when lock-path bin miss persists, perform page_create outside sc lock, then re-scan.
#ifndef HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX
#define HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX 0
#endif
#if HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX && HZ4_MID_LOCK_ELIDE
#error "HZ4_MID_PAGE_CREATE_OUTSIDE_SC_LOCK_BOX requires HZ4_MID_LOCK_ELIDE=0"
#endif

// B70: MidPageSupplyReservationBox (default ON)
// Reserve N pages per segment lock acquisition to amortize lock traffic.
#ifndef HZ4_MID_PAGE_SUPPLY_RESV_BOX
#define HZ4_MID_PAGE_SUPPLY_RESV_BOX 1
#endif
#ifndef HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES
#define HZ4_MID_PAGE_SUPPLY_RESV_CHUNK_PAGES 8
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


#endif // HZ4_CONFIG_COLLECT_H
