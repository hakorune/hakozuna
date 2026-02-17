// hz4_config_remote.h - HZ4 Remote/MT Configuration
// Box Theory: Remote, Inbox, RBUF, Staging settings
#ifndef HZ4_CONFIG_REMOTE_H
#define HZ4_CONFIG_REMOTE_H

// ============================================================================
// Remote Inbox (default ON for stability)
// ============================================================================
#ifndef HZ4_REMOTE_INBOX
#define HZ4_REMOTE_INBOX 1  // default ON (required for -O2 stability)
#endif

// Remote Flush Probe count (required when REMOTE_INBOX=1)
#ifndef HZ4_REMOTE_FLUSH_PROBE
#define HZ4_REMOTE_FLUSH_PROBE 4
#endif

// ============================================================================
// Remote Shards (page.remote_head[] の分割数)
// ============================================================================
#ifndef HZ4_REMOTE_SHARDS
#define HZ4_REMOTE_SHARDS 4  // T=16 の shared-line 競合を軽減
#endif
#if HZ4_REMOTE_SHARDS > 64
#error "HZ4_REMOTE_SHARDS must be <= 64"
#endif

// Remote shard mask hint (optional)
#ifndef HZ4_REMOTE_MASK
#define HZ4_REMOTE_MASK 0  // default OFF (overhead can exceed skip benefit)
#endif

// Page queue mode (seg-internal pageq)
#ifndef HZ4_PAGEQ_ENABLE
#define HZ4_PAGEQ_ENABLE 1
#endif

// PageQ sc-bucket 設定 (collect が該当 bucket のみ走査)
#ifndef HZ4_PAGEQ_BUCKET_SHIFT
#define HZ4_PAGEQ_BUCKET_SHIFT 3  // 8 sc per bucket
#endif

// PageQ drain page-scan budget (per-segment).
// - Bounds worst-case scan cost when a bucket is skewed (many non-matching pages for a given sc).
// - Enabled by default for QuarantineBox builds to avoid pathologically long drains.
#ifndef HZ4_PAGEQ_DRAIN_PAGE_BUDGET
#if HZ4_RSSRETURN_QUARANTINEBOX
#define HZ4_PAGEQ_DRAIN_PAGE_BUDGET 64
#else
#define HZ4_PAGEQ_DRAIN_PAGE_BUDGET 0
#endif
#endif

// ============================================================================
// TLS Remote Buffer (rbuf) 容量
// ============================================================================
#ifndef HZ4_RBUF_CAP
#define HZ4_RBUF_CAP 128  // fixed array size (GO: fallback_rate改善)
#endif

// ============================================================================ 
// Remote flush tuning
// ============================================================================
#ifndef HZ4_REMOTE_FLUSH_THRESHOLD
#define HZ4_REMOTE_FLUSH_THRESHOLD 96  // tuned (Grid Phase B best)
#endif
#if HZ4_REMOTE_FLUSH_THRESHOLD > HZ4_RBUF_CAP
#error "HZ4_REMOTE_FLUSH_THRESHOLD must be <= HZ4_RBUF_CAP"
#endif

// Remote Flush Loop Unroll (R=90 optimization)
#ifndef HZ4_REMOTE_FLUSH_UNROLL
#define HZ4_REMOTE_FLUSH_UNROLL 1  // default ON (GO)
#endif

// Stage5-N23: inbox-only no-clear-next (research / opt-in)
// Skip obj->next clear in remote flush when using inbox path.
#ifndef HZ4_REMOTE_FLUSH_INBOX_NO_CLEAR_NEXT
#define HZ4_REMOTE_FLUSH_INBOX_NO_CLEAR_NEXT 0
#endif

// Stage5-N21 (opt-in): retry pending rbuf entries into remote bump-free-meta
// at flush boundary to reduce obj->next first-touch writes in inbox fallback path.
#ifndef HZ4_REMOTE_FLUSH_RETRY_BUMPMETA
#define HZ4_REMOTE_FLUSH_RETRY_BUMPMETA 0  // default OFF (opt-in for A/B)
#endif

// GuardRemoteFlushCompactBox (opt-in):
// for small rlen band (>FASTPATH_MAX), bypass general bucket path by compact
// per-flush grouping on stack and direct inbox publish.
#ifndef HZ4_REMOTE_FLUSH_COMPACT_BOX
#define HZ4_REMOTE_FLUSH_COMPACT_BOX 0
#endif
#ifndef HZ4_REMOTE_FLUSH_COMPACT_MAX
#define HZ4_REMOTE_FLUSH_COMPACT_MAX 8
#endif
#if HZ4_REMOTE_FLUSH_COMPACT_BOX && !HZ4_REMOTE_INBOX
#error "HZ4_REMOTE_FLUSH_COMPACT_BOX requires HZ4_REMOTE_INBOX=1"
#endif
#if HZ4_REMOTE_FLUSH_COMPACT_BOX && (HZ4_REMOTE_FLUSH_COMPACT_MAX <= HZ4_REMOTE_FLUSH_FASTPATH_MAX)
#error "HZ4_REMOTE_FLUSH_COMPACT_MAX must be > HZ4_REMOTE_FLUSH_FASTPATH_MAX when compact box is enabled"
#endif
#if HZ4_REMOTE_FLUSH_COMPACT_BOX && (HZ4_REMOTE_FLUSH_COMPACT_MAX > HZ4_RBUF_CAP)
#error "HZ4_REMOTE_FLUSH_COMPACT_MAX must be <= HZ4_RBUF_CAP"
#endif
#if HZ4_REMOTE_FLUSH_COMPACT_BOX && (HZ4_REMOTE_FLUSH_COMPACT_MAX > 16)
#error "HZ4_REMOTE_FLUSH_COMPACT_MAX must be <=16"
#endif

// GuardRemoteFlushDirectIndexBox (opt-in):
// for threshold-band flushes (high rlen), bypass hash/probe bucketing and use
// direct owner/sc indexing with TLS stamps.
#ifndef HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX
#define HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX 0
#endif
#ifndef HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN
#define HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN HZ4_REMOTE_FLUSH_THRESHOLD
#endif
#if HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX && !HZ4_REMOTE_INBOX
#error "HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX requires HZ4_REMOTE_INBOX=1"
#endif
#if HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX && defined(HZ4_RBUF_KEY) && !HZ4_RBUF_KEY
#error "HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX requires HZ4_RBUF_KEY=1"
#endif
#if HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX && (HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN <= HZ4_REMOTE_FLUSH_FASTPATH_MAX)
#error "HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN must be > HZ4_REMOTE_FLUSH_FASTPATH_MAX"
#endif
#if HZ4_REMOTE_FLUSH_DIRECT_INDEX_BOX && (HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN > HZ4_RBUF_CAP)
#error "HZ4_REMOTE_FLUSH_DIRECT_INDEX_MIN_RLEN must be <= HZ4_RBUF_CAP"
#endif

// TLS Buckets for remote flush (reduces probe overflow)
#ifndef HZ4_REMOTE_FLUSH_BUCKETS
#define HZ4_REMOTE_FLUSH_BUCKETS 256  // power-of-2 (rf_probe_ovf削減, R50/R90改善)
#endif
#if (HZ4_REMOTE_FLUSH_BUCKETS & (HZ4_REMOTE_FLUSH_BUCKETS - 1)) != 0
#error "HZ4_REMOTE_FLUSH_BUCKETS must be power of two"
#endif

// Remote Direct Inbox: bypass rbuf when threshold reached (R=90 optimization)
// ============================================================================
#ifndef HZ4_REMOTE_DIRECT_INBOX
#define HZ4_REMOTE_DIRECT_INBOX 0  // default OFF (A/B)
#endif

// Remote rbuf keying (store owner/sc in rbuf to avoid meta fetch in flush)
#ifndef HZ4_RBUF_KEY
#define HZ4_RBUF_KEY 1  // default ON (GO: R=50/R=90 improve)
#endif

// Remote free keyed path (reuse owner/sc from caller)
#ifndef HZ4_REMOTE_FREE_KEYED
#define HZ4_REMOTE_FREE_KEYED 1  // default ON (A/B)
#endif

#ifndef HZ4_REMOTE_DIRECT_THRESHOLD
#define HZ4_REMOTE_DIRECT_THRESHOLD HZ4_REMOTE_FLUSH_THRESHOLD  // 128
#endif

// Remote Direct Inbox Period: check flush condition only 1/PERIOD times
// NOTE: HZ4_REMOTE_DIRECT_INBOX_PERIOD must be power-of-two (for & (period-1) optimization)
#ifndef HZ4_REMOTE_DIRECT_INBOX_PERIOD
#define HZ4_REMOTE_DIRECT_INBOX_PERIOD 64  // 1/64 回だけ判定
#endif
#if (HZ4_REMOTE_DIRECT_INBOX_PERIOD & (HZ4_REMOTE_DIRECT_INBOX_PERIOD - 1)) != 0
#error "HZ4_REMOTE_DIRECT_INBOX_PERIOD must be power-of-two"
#endif

// ============================================================================
// Stage5: BumpFreeMeta / RemoteBumpFreeMeta defaults
// ============================================================================
#ifndef HZ4_BUMP_FREE_META
#define HZ4_BUMP_FREE_META 0
#endif
#ifndef HZ4_BUMP_FREE_META_CAP
#define HZ4_BUMP_FREE_META_CAP 64
#endif
#ifndef HZ4_REMOTE_BUMP_FREE_META
#define HZ4_REMOTE_BUMP_FREE_META 0
#endif
#ifndef HZ4_REMOTE_BUMP_FREE_META_CAP
#define HZ4_REMOTE_BUMP_FREE_META_CAP 80
#endif
#ifndef HZ4_REMOTE_BUMP_FREE_META_SHARDS
#define HZ4_REMOTE_BUMP_FREE_META_SHARDS 1
#endif
#ifndef HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
#define HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT 0
#endif
#ifndef HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
#define HZ4_REMOTE_BUMP_FREE_META_TRYLOCK 0
#endif

#if HZ4_BUMP_FREE_META_CAP < 1
#error "HZ4_BUMP_FREE_META_CAP must be >= 1"
#endif
#if HZ4_REMOTE_BUMP_FREE_META_CAP < 1
#error "HZ4_REMOTE_BUMP_FREE_META_CAP must be >= 1"
#endif
#if HZ4_BUMP_FREE_META && !HZ4_POPULATE_BATCH
#error "HZ4_BUMP_FREE_META requires HZ4_POPULATE_BATCH>0"
#endif
#if HZ4_REMOTE_BUMP_FREE_META && !HZ4_POPULATE_BATCH
#error "HZ4_REMOTE_BUMP_FREE_META requires HZ4_POPULATE_BATCH>0"
#endif
#if HZ4_REMOTE_BUMP_FREE_META && !HZ4_BUMP_FREE_META
#error "HZ4_REMOTE_BUMP_FREE_META requires HZ4_BUMP_FREE_META=1"
#endif
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT && !HZ4_REMOTE_BUMP_FREE_META
#error "HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT requires HZ4_REMOTE_BUMP_FREE_META=1"
#endif
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT && HZ4_REMOTE_BUMP_FREE_META_TRYLOCK
#error "HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT and HZ4_REMOTE_BUMP_FREE_META_TRYLOCK are mutually exclusive"
#endif
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS < 1
#error "HZ4_REMOTE_BUMP_FREE_META_SHARDS must be >= 1"
#endif
#if HZ4_REMOTE_BUMP_FREE_META_SHARDS > 1
#if (HZ4_REMOTE_BUMP_FREE_META_SHARDS & (HZ4_REMOTE_BUMP_FREE_META_SHARDS - 1)) != 0
#error "HZ4_REMOTE_BUMP_FREE_META_SHARDS must be power-of-two when > 1"
#endif
#if HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT
#error "HZ4_REMOTE_BUMP_FREE_META_SHARDS is not supported with HZ4_REMOTE_BUMP_FREE_META_PUBCOUNT"
#endif
#endif

// ============================================================================
// Stage5-1: Remote-lite v1a2 (R=90 専用 / R=50 は退行し得る → 既定OFFで lane 隔離)
// - bypass inbox/rbuf; push directly to page.remote_head + pageq_notify
// - notify only on empty->non-empty transition
// ============================================================================
#ifndef HZ4_REMOTE_LITE
#define HZ4_REMOTE_LITE 0  // default OFF (opt-in for A/B)
#endif

#if HZ4_REMOTE_LITE && !HZ4_PAGE_META_SEPARATE
#error "HZ4_REMOTE_LITE requires HZ4_PAGE_META_SEPARATE=1"
#endif

// ============================================================================
// Stage5-2: RemotePageRbufBox v1 (mimalloc-style)
// - bypass inbox/rbuf + segq/pageq
// - remote free: push directly to page.remote_head[]
// - notify: per-owner global page queue (bucketed by sc)
// - owner drain: hz4_collect()/hz4_collect_list() early phase
// ============================================================================
#ifndef HZ4_REMOTE_PAGE_RBUF
#define HZ4_REMOTE_PAGE_RBUF 0  // default OFF (opt-in for A/B)
#endif

#ifndef HZ4_REMOTE_PAGE_RBUF_DRAIN_PAGE_BUDGET
#define HZ4_REMOTE_PAGE_RBUF_DRAIN_PAGE_BUDGET 64  // per collect() call
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN
#define HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN 0  // default OFF (opt-in): drain rbuf only when refill underflow marks demand
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_DEMAND_CREDITS
#define HZ4_REMOTE_PAGE_RBUF_DEMAND_CREDITS 2  // demand tokens added per underflow
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_DEMAND_ALLOW_GATE_ON
#define HZ4_REMOTE_PAGE_RBUF_DEMAND_ALLOW_GATE_ON 1  // bypass demand gate while rbuf_gate_on=1
#endif

#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY 0  // default OFF (opt-in for A/B)
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL
#define HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL 1  // keep queued for N empty collect passes (>=1)
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_MIN_DRAIN
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_MIN_DRAIN 1  // hold only when drained >= N objs
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ONLY
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ONLY 0  // hold only when rbuf gate is ON
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX 0  // 0=disabled, disable hold after N empty drains
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS 0  // 0=disabled, require gate_on for N windows
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT 0  // 0=disabled, fallback% >= HI disables lazy-hold
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT 0  // fallback% <= LO re-enables lazy-hold
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES 1024  // power-of-two
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HOLD_WINDOWS
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HOLD_WINDOWS 2  // flap suppression
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT 0  // 0=disabled, drain0% >= HI disables lazy-hold
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT 0  // drain0% <= LO re-enables lazy-hold
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES 1024  // power-of-two
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HOLD_WINDOWS
#define HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HOLD_WINDOWS 2  // flap suppression
#endif
#if HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL < 1
#error "HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL must be >= 1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL > 255
#error "HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL must be <= 255"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_MIN_DRAIN < 1
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_MIN_DRAIN must be >= 1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX > 255
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX must be <= 255"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS > 255
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS must be <= 255"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 100
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT must be <= 100"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT > 100
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT must be <= 100"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 100
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT must be <= 100"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT > 100
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT must be <= 100"
#endif
#if HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL > 1 && !HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
#error "HZ4_REMOTE_PAGE_RBUF_NOTIFY_INTERVAL>1 requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX > 0 && !HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_EMPTY_STREAK_MAX>0 requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS > 0 && !HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS>0 requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT == 0 && HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT != 0
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT>0"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT == 0 && HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT != 0
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT>0"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0
#if !HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT>0 requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY=1"
#endif
#if !HZ4_REMOTE_BUMP_FREE_META
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT>0 requires HZ4_REMOTE_BUMP_FREE_META=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT >= HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_LO_PCT must be < HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES < 1
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES must be >= 1"
#endif
#if (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES & (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES - 1)) != 0
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_WINDOW_TRIES must be power-of-two"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HOLD_WINDOWS > 255
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HOLD_WINDOWS must be <= 255"
#endif
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0
#if !HZ4_ALLOW_ARCHIVED_BOXES
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT>0 is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#if !HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT>0 requires HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT >= HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_LO_PCT must be < HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES < 1
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES must be >= 1"
#endif
#if (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES & (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES - 1)) != 0
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_WINDOW_TRIES must be power-of-two"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HOLD_WINDOWS > 255
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HOLD_WINDOWS must be <= 255"
#endif
#endif

#if HZ4_REMOTE_PAGE_RBUF && !HZ4_PAGE_META_SEPARATE
#error "HZ4_REMOTE_PAGE_RBUF requires HZ4_PAGE_META_SEPARATE=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN && !HZ4_REMOTE_PAGE_RBUF
#error "HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN requires HZ4_REMOTE_PAGE_RBUF=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_DEMAND_CREDITS < 1 || HZ4_REMOTE_PAGE_RBUF_DEMAND_CREDITS > 255
#error "HZ4_REMOTE_PAGE_RBUF_DEMAND_CREDITS must be in [1,255]"
#endif
#if HZ4_REMOTE_PAGE_RBUF_DEMAND_ALLOW_GATE_ON != 0 && HZ4_REMOTE_PAGE_RBUF_DEMAND_ALLOW_GATE_ON != 1
#error "HZ4_REMOTE_PAGE_RBUF_DEMAND_ALLOW_GATE_ON must be 0 or 1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY && !HZ4_REMOTE_PAGE_RBUF
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY requires HZ4_REMOTE_PAGE_RBUF=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS > 0 && !(HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX)
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ON_DWELL_WINDOWS>0 requires HZ4_REMOTE_PAGE_RBUF_GATEBOX=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ONLY && !(HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX)
#error "HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_GATE_ONLY requires HZ4_REMOTE_PAGE_RBUF_GATEBOX=1"
#endif

// Stage5-2b: RemotePageStagingBox (BatchIt-style micro-batch, per-page)
// - remote free: TLSで {page -> (head, tail, n)} を小さくキャッシュ
// - flush: 1 CAS/list で page.remote_head に押し込み、empty->non-empty の時だけ pageq enqueue
// 目的: RemotePageRbufBox v1 の「1 free = 1 CAS」固定費を削って mimalloc 差分を詰める
#ifndef HZ4_REMOTE_PAGE_STAGING
#define HZ4_REMOTE_PAGE_STAGING 0  // default OFF (opt-in for A/B)
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META
#define HZ4_REMOTE_PAGE_STAGING_META 0  // default OFF (opt-in for A/B)
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_BATCH_PUSH
#define HZ4_REMOTE_PAGE_STAGING_META_BATCH_PUSH 0  // default OFF (opt-in): batch idx flush into rbmf
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_DEFER_FALLBACK
#define HZ4_REMOTE_PAGE_STAGING_META_DEFER_FALLBACK 0  // default OFF (opt-in): defer residual idx fallback
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY
#define HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY 2  // max consecutive defers before fallback
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
#define HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG 0  // default OFF (opt-in): spill residual idx as message instead of obj list
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N
#define HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N 4096  // global fixed pool for idx messages
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N
#define HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N 2  // spill to msg only when residual idx count >= N
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N
#define HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N 16  // per-thread msg-node cache batch size
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ
#define HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ 1  // 1=drain msgq first only when pageq empty
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY
#define HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY 0  // opt-in: with pageq present, drain msgq only when page-drain got==0
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
#define HZ4_REMOTE_PAGE_STAGING_META_MSGPASS 0  // default OFF (archived NO-GO): spill residual idx via owner slot-msgpass
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N
#define HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N 2  // msgpass only when residual idx count >= N
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ
#define HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ 1  // 1=drain msgpass first only when pageq empty
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY
#define HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY 0  // 1=with pageq present, drain msgpass only when page-drain got==0
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_CACHE_N
#define HZ4_REMOTE_PAGE_STAGING_CACHE_N 8  // direct-mapped entries (power-of-two)
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_MAX
#define HZ4_REMOTE_PAGE_STAGING_MAX 2  // micro-batch size per page
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_PERIOD
#define HZ4_REMOTE_PAGE_STAGING_PERIOD 64  // flush-all period (power-of-two, in remote frees)
#endif
#ifndef HZ4_REMOTE_PAGE_STAGING_NO_CLEAR_NEXT
#define HZ4_REMOTE_PAGE_STAGING_NO_CLEAR_NEXT 0  // opt-in: avoid obj->next = NULL stores (can reduce page-faults)
#endif
#if (HZ4_REMOTE_PAGE_STAGING_CACHE_N & (HZ4_REMOTE_PAGE_STAGING_CACHE_N - 1)) != 0
#error "HZ4_REMOTE_PAGE_STAGING_CACHE_N must be power-of-two"
#endif
#if (HZ4_REMOTE_PAGE_STAGING_PERIOD & (HZ4_REMOTE_PAGE_STAGING_PERIOD - 1)) != 0
#error "HZ4_REMOTE_PAGE_STAGING_PERIOD must be power-of-two"
#endif
#if HZ4_REMOTE_PAGE_STAGING && !HZ4_REMOTE_PAGE_RBUF
#error "HZ4_REMOTE_PAGE_STAGING requires HZ4_REMOTE_PAGE_RBUF=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META && !HZ4_REMOTE_PAGE_STAGING
#error "HZ4_REMOTE_PAGE_STAGING_META requires HZ4_REMOTE_PAGE_STAGING=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META && !HZ4_REMOTE_BUMP_FREE_META
#error "HZ4_REMOTE_PAGE_STAGING_META requires HZ4_REMOTE_BUMP_FREE_META=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_BATCH_PUSH && !HZ4_REMOTE_PAGE_STAGING_META
#error "HZ4_REMOTE_PAGE_STAGING_META_BATCH_PUSH requires HZ4_REMOTE_PAGE_STAGING_META=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_DEFER_FALLBACK && !HZ4_REMOTE_PAGE_STAGING_META
#error "HZ4_REMOTE_PAGE_STAGING_META_DEFER_FALLBACK requires HZ4_REMOTE_PAGE_STAGING_META=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY > 255
#error "HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY must be <= 255"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_DEFER_FALLBACK && HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY < 1
#error "HZ4_REMOTE_PAGE_STAGING_META_DEFER_MAX_RETRY must be >= 1 when DEFER_FALLBACK is enabled"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG && !HZ4_REMOTE_PAGE_STAGING_META
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG requires HZ4_REMOTE_PAGE_STAGING_META=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
#if !HZ4_ALLOW_ARCHIVED_BOXES
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N < 1
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N must be >= 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N < 1
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N must be >= 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N > HZ4_REMOTE_PAGE_STAGING_MAX
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N must be <= HZ4_REMOTE_PAGE_STAGING_MAX"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N < 1
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N must be >= 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ != 0 && \
    HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ != 1
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_ON_EMPTY_PAGEQ must be 0 or 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY != 0 && \
    HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY != 1
#error "HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_DRAIN_UNDERFLOW_ONLY must be 0 or 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS && !HZ4_REMOTE_PAGE_STAGING_META
#error "HZ4_REMOTE_PAGE_STAGING_META_MSGPASS requires HZ4_REMOTE_PAGE_STAGING_META=1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
#if !HZ4_ALLOW_ARCHIVED_BOXES
#error "HZ4_REMOTE_PAGE_STAGING_META_MSGPASS is archived (NO-GO). Set HZ4_ALLOW_ARCHIVED_BOXES=1 to enable."
#endif
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N < 1
#error "HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N must be >= 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS && \
    (HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N > HZ4_REMOTE_PAGE_STAGING_MAX)
#error "HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N must be <= HZ4_REMOTE_PAGE_STAGING_MAX"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ != 0 && \
    HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ != 1
#error "HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_ON_EMPTY_PAGEQ must be 0 or 1"
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY != 0 && \
    HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY != 1
#error "HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_DRAIN_UNDERFLOW_ONLY must be 0 or 1"
#endif

// Stage5-N6: RemotePageRbufGateBox (gate RBUF usage based on remote free rate)
// - R=90: RBUF ON (+2% effect)
// - R=50: RBUF OFF (avoid -13% regression)
// Gate uses hysteresis: remote% >= HI -> ON, remote% <= LO -> OFF
#ifndef HZ4_REMOTE_PAGE_RBUF_GATEBOX
#define HZ4_REMOTE_PAGE_RBUF_GATEBOX 0  // default OFF (opt-in for A/B)
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_HI_PCT
#define HZ4_REMOTE_PAGE_RBUF_GATE_HI_PCT 90  // remote% >= HI -> gate ON
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_LO_PCT
#define HZ4_REMOTE_PAGE_RBUF_GATE_LO_PCT 70  // remote% <= LO -> gate OFF
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES
#define HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES 4096  // window size (power-of-two)
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_HOLD_WINDOWS
#define HZ4_REMOTE_PAGE_RBUF_GATE_HOLD_WINDOWS 2  // flap suppression
#endif
// Low-remote bypass: sample local frees for gate window accounting.
// 0 = count every local free (legacy), N>0 = count 1 per 2^N local frees.
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT
#define HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT 0
#endif
#if (HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES & (HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES - 1)) != 0
#error "HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES must be power-of-two"
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT > 15
#error "HZ4_REMOTE_PAGE_RBUF_GATE_LOCAL_SAMPLE_SHIFT must be <= 15"
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATE_HI_PCT <= HZ4_REMOTE_PAGE_RBUF_GATE_LO_PCT
#error "HZ4_REMOTE_PAGE_RBUF_GATE_HI_PCT must be > HZ4_REMOTE_PAGE_RBUF_GATE_LO_PCT"
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATEBOX && !HZ4_REMOTE_PAGE_RBUF
#error "HZ4_REMOTE_PAGE_RBUF_GATEBOX requires HZ4_REMOTE_PAGE_RBUF=1"
#endif

// B40: FastLocalBox (default auto-ON when rbuf-gate is enabled)
// Further reduce local free counter update when gate_off && no remote frees.
// This eliminates most counter traffic for pure-local workloads.
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
#define HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX 1
#else
#define HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX 0
#endif
#endif
#ifndef HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT
#define HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT 6  // count 1 per 64 local frees
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT > 15
#error "HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT must be <= 15"
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX && !(HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX)
#error "HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX requires HZ4_REMOTE_PAGE_RBUF_GATEBOX=1"
#endif
#if HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_BOX && \
    ((1u << HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT) >= HZ4_REMOTE_PAGE_RBUF_GATE_WINDOW_FREES)
#error "HZ4_REMOTE_PAGE_RBUF_GATE_FAST_LOCAL_SHIFT must satisfy (1<<SHIFT) < WINDOW_FREES"
#endif
#if HZ4_REMOTE_FLUSH_RETRY_BUMPMETA && !HZ4_REMOTE_BUMP_FREE_META
#error "HZ4_REMOTE_FLUSH_RETRY_BUMPMETA requires HZ4_REMOTE_BUMP_FREE_META=1"
#endif
#if HZ4_REMOTE_FLUSH_RETRY_BUMPMETA && !HZ4_REMOTE_PAGE_RBUF
#error "HZ4_REMOTE_FLUSH_RETRY_BUMPMETA requires HZ4_REMOTE_PAGE_RBUF=1"
#endif
#if HZ4_REMOTE_FLUSH_RETRY_BUMPMETA && !HZ4_PAGE_META_SEPARATE
#error "HZ4_REMOTE_FLUSH_RETRY_BUMPMETA requires HZ4_PAGE_META_SEPARATE=1"
#endif
// Stage5-3: PressureGate v0 (research / opt-in)
// Use rbuf_gate_on as a cheap pressure signal to gate R90-only paths.
#ifndef HZ4_PRESSURE_GATE
#define HZ4_PRESSURE_GATE 0  // default OFF (opt-in for A/B)
#endif
#if HZ4_PRESSURE_GATE && !(HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX)
#error "HZ4_PRESSURE_GATE requires HZ4_REMOTE_PAGE_RBUF_GATEBOX=1"
#endif

// Stage5-N8: RemoteMetaPadBox (research / opt-in)
// Reduce false sharing by separating remote_head[] (remote CAS traffic) from page queue state.
#ifndef HZ4_REMOTE_META_PADBOX
#define HZ4_REMOTE_META_PADBOX 0  // default OFF (opt-in for A/B)
#endif


#ifndef HZ4_NUM_SHARDS
#define HZ4_NUM_SHARDS 64  // 2の冪 (マスクで高速化: & 63)
#endif
#define HZ4_SHARD_MASK (HZ4_NUM_SHARDS - 1)

// PageQ buckets: round up so SC_MAX doesn't fall outside buckets
#define HZ4_PAGEQ_BUCKETS \
    ((HZ4_SC_MAX + ((1u << HZ4_PAGEQ_BUCKET_SHIFT) - 1)) >> HZ4_PAGEQ_BUCKET_SHIFT)

#endif // HZ4_CONFIG_REMOTE_H
