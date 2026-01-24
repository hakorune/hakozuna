// hz4_config.h - HZ4 Configuration Constants
// Box Theory: すべての knob は #ifndef で上書き可能
#ifndef HZ4_CONFIG_H
#define HZ4_CONFIG_H

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

// ============================================================================
// TLS Remote Buffer (rbuf) 容量
// ============================================================================
#ifndef HZ4_RBUF_CAP
#define HZ4_RBUF_CAP 64  // fixed array size
#endif

// ============================================================================
// Remote flush tuning
// ============================================================================
#ifndef HZ4_REMOTE_FLUSH_THRESHOLD
#define HZ4_REMOTE_FLUSH_THRESHOLD HZ4_RBUF_CAP
#endif
#if HZ4_REMOTE_FLUSH_THRESHOLD > HZ4_RBUF_CAP
#error "HZ4_REMOTE_FLUSH_THRESHOLD must be <= HZ4_RBUF_CAP"
#endif

#ifndef HZ4_REMOTE_FLUSH_BUCKETS
#define HZ4_REMOTE_FLUSH_BUCKETS 32  // power-of-2
#endif
#if (HZ4_REMOTE_FLUSH_BUCKETS & (HZ4_REMOTE_FLUSH_BUCKETS - 1)) != 0
#error "HZ4_REMOTE_FLUSH_BUCKETS must be power of two"
#endif

#ifndef HZ4_REMOTE_FLUSH_FASTPATH_MAX
#define HZ4_REMOTE_FLUSH_FASTPATH_MAX 4
#endif

// Page-based bucketing for remote flush (eliminate owner/sc hash collision)
#ifndef HZ4_REMOTE_FLUSH_PAGE_BUCKET
#define HZ4_REMOTE_FLUSH_PAGE_BUCKET 0  // default OFF (A/B test)
#endif

// Probe limit for page bucket collision resolution
#ifndef HZ4_REMOTE_FLUSH_PROBE
#define HZ4_REMOTE_FLUSH_PROBE 4
#endif

// ============================================================================
// Statistics (cold path only)
// ============================================================================
#ifndef HZ4_STATS
#define HZ4_STATS 0  // デフォルト OFF (ベンチ時のみ ON)
#endif

// ============================================================================
// Owner Shard 数 (pending queue の分割数)
// ============================================================================
#ifndef HZ4_NUM_SHARDS
#define HZ4_NUM_SHARDS 64  // 2の冪 (マスクで高速化: & 63)
#endif

#define HZ4_SHARD_MASK (HZ4_NUM_SHARDS - 1)

// ============================================================================
// Size Class Count (carry slots)
// ============================================================================
#ifndef HZ4_SC_MAX
#define HZ4_SC_MAX 128  // 16B aligned classes for 16..2048B
#endif

// PageQ buckets: 切り上げ (SC_MAX が 8 の倍数でなくても bucket 外にならない)
#define HZ4_PAGEQ_BUCKETS \
    ((HZ4_SC_MAX + ((1u << HZ4_PAGEQ_BUCKET_SHIFT) - 1)) >> HZ4_PAGEQ_BUCKET_SHIFT)

// Carry policy: if carry remains after consume, skip segq to reduce overhead
#ifndef HZ4_CARRY_SKIP_SEGQ
#define HZ4_CARRY_SKIP_SEGQ 1
#endif

// Carry slots: multiple remainder slots to reduce requeue/pushback
#ifndef HZ4_CARRY_SLOTS
#define HZ4_CARRY_SLOTS 4
#endif

// Remote inbox: MPSC queue per (owner, sc) for direct remote object collection
// Bypasses segq/pageq for lower overhead in remote-heavy workloads
#ifndef HZ4_REMOTE_INBOX
#define HZ4_REMOTE_INBOX 1  // default ON (P3 inbox lane)
#endif

// P3.2: Inbox-only mode (skip segq/carry most of the time)
#ifndef HZ4_INBOX_ONLY
#define HZ4_INBOX_ONLY 0  // default OFF (A/B)
#endif

// Probe mask: MUST be 2^n - 1 for bitwise AND (e.g., 0xFF = 256回に1回)
#ifndef HZ4_INBOX_SEGQ_PROBE_MASK
#define HZ4_INBOX_SEGQ_PROBE_MASK 0xFF  // 256回に1回
#endif

// P3.4: Inbox splice refill (eliminate double traversal)
#ifndef HZ4_INBOX_SPLICE
#define HZ4_INBOX_SPLICE 0  // default OFF (A/B)
#endif

// P3.5: Tcache count tracking (statistics only, disable for perf)
#ifndef HZ4_TCACHE_COUNT
#define HZ4_TCACHE_COUNT 1  // default ON (A/B: try 0 for perf)
#endif

// P4.1: Collect list mode (eliminate out[] array)
// P4.1b result: +8.6% → GO, default ON
#ifndef HZ4_COLLECT_LIST
#define HZ4_COLLECT_LIST 1  // default ON (GO: +8.6%)
#endif

// ============================================================================
// Page / Segment サイズ
// ============================================================================
#ifndef HZ4_PAGE_SHIFT
#define HZ4_PAGE_SHIFT 16  // 64KB pages
#endif

#ifndef HZ4_SEG_SHIFT
#define HZ4_SEG_SHIFT 20  // 1MB segments
#endif

#define HZ4_PAGE_SIZE (1ULL << HZ4_PAGE_SHIFT)
#define HZ4_SEG_SIZE  (1ULL << HZ4_SEG_SHIFT)
#define HZ4_PAGES_PER_SEG (HZ4_SEG_SIZE / HZ4_PAGE_SIZE)  // = 16

// Page0 予約: segment header と重なるため使用不可
#define HZ4_PAGES_USABLE  (HZ4_PAGES_PER_SEG - 1)         // = 15 (page0 除外)
#define HZ4_PAGE_IDX_MIN  1                                // 最小有効 page_idx

// Bitmap words: HZ4_PAGES_PER_SEG から導出 (bit0 reserved)
#define HZ4_PAGEWORDS ((HZ4_PAGES_PER_SEG + 63) / 64)

// ============================================================================
// Mid alignment
// ============================================================================
#ifndef HZ4_MID_ALIGN
#define HZ4_MID_ALIGN 256u
#endif

// ============================================================================
// Magic 値
// ============================================================================
#define HZ4_PAGE_MAGIC 0x48345047  // "H4PG"
#define HZ4_SEG_MAGIC  0x48345347  // "H4SG"
#define HZ4_MID_MAGIC  0x48344D47  // "H4MG"
#define HZ4_LARGE_MAGIC 0x48344C47 // "H4LG"

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
#define HZ4_COLLECT_OBJ_BUDGET 64
#endif

#ifndef HZ4_COLLECT_SEG_BUDGET
#define HZ4_COLLECT_SEG_BUDGET 4
#endif

// ============================================================================
// Local path optimization flags
// ============================================================================
// Small 側の二重 magic check を省略（release 用）
// HZ4_FAILFAST 時は常に magic check が有効
#ifndef HZ4_LOCAL_MAGIC_CHECK
#define HZ4_LOCAL_MAGIC_CHECK 1  // default ON
#endif

// TLS init check の最適化
#ifndef HZ4_TLS_SINGLE
#define HZ4_TLS_SINGLE 0  // default OFF
#endif

// TLS direct access (eliminate PLT overhead)
// A/B 結果: +8.8% → GO, default ON に切替
#ifndef HZ4_TLS_DIRECT
#define HZ4_TLS_DIRECT 1  // default ON (GO: +8.8%)
#endif

// ============================================================================
// Page Tag Table (eliminate magic routing)
// ============================================================================
#ifndef HZ4_PAGE_TAG_TABLE
#define HZ4_PAGE_TAG_TABLE 0  // default OFF
#endif

// Tag kind constants (kind=0 is reserved for "unknown")
#define HZ4_TAG_KIND_UNKNOWN  0  // tag=0 → fallback path
#define HZ4_TAG_KIND_SMALL    1
#define HZ4_TAG_KIND_MID      2
// Large は tag 登録しない（legacy path 維持）

// Arena size for page tag table (ENV or config で調整可能)
#ifndef HZ4_ARENA_SIZE
#define HZ4_ARENA_SIZE (4ULL << 30)  // 4GB default
#endif

#define HZ4_MAX_PAGES (HZ4_ARENA_SIZE / HZ4_PAGE_SIZE)

// ============================================================================
// Debug flags
// ============================================================================
#ifndef HZ4_FAILFAST
#define HZ4_FAILFAST 0
#endif

// Segment full zeroing (mmap already returns zeroed pages)
#ifndef HZ4_SEG_ZERO_FULL
#define HZ4_SEG_ZERO_FULL 1
#endif

// TLS struct merge (reduce indirection)
// A/B result: TBD
#ifndef HZ4_TLS_MERGE
#define HZ4_TLS_MERGE 0  // default OFF (A/B test)
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

#endif // HZ4_CONFIG_H
