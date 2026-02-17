// hz4_tls.h - HZ4 Thread-Local State
#ifndef HZ4_TLS_H
#define HZ4_TLS_H

#include "hz4_types.h"
#include "hz4_page.h"

#if HZ4_XFER_CACHE
#include "hz4_xfer.h"
#endif

#if HZ4_TRANSFERCACHE
#include "hz4_tcbox.h"
#endif

// ============================================================================
// Tcache Bin (MERGE mode only)
// ============================================================================
#if HZ4_TLS_MERGE
typedef struct hz4_tcache_bin {
    void* head;
    uint32_t count;
#if HZ4_POPULATE_BATCH
    hz4_page_t* bump_page;  // lazy populate target
#endif
#if HZ4_TCACHE_OBJ_CACHE_ON
    void* st_cache[HZ4_TCACHE_OBJ_CACHE_SLOTS_ON];
    uint8_t st_cache_n;
#endif
} hz4_tcache_bin_t;
#endif

// ============================================================================
// Remote Buffer Entry
// ============================================================================
// 固定長配列で stale entry 問題を回避 (hz3 Lane16 の教訓)
typedef struct hz4_rbuf_ent {
    hz4_page_t* page;
    void* obj;
#if HZ4_REMOTE_INBOX && HZ4_RBUF_KEY
    uint8_t owner;
    uint8_t sc;
#endif
} hz4_rbuf_ent_t;

// ============================================================================
// Remote flush buckets (shared struct definitions)
// ============================================================================
typedef struct hz4_flush_bucket {
    hz4_page_t* page;
    void* head;
    void* tail;
    uint32_t n;
} hz4_flush_bucket_t;

#if HZ4_REMOTE_INBOX
typedef struct hz4_inbox_bucket {
    uint8_t owner;
    uint8_t sc;
    void* head;
    void* tail;
} hz4_inbox_bucket_t;
#endif

// ============================================================================
// Carry Slot (remainder from drain_page)
// ============================================================================
typedef struct hz4_carry_slot {
    void*        head;   // freelist head (NULL = empty)
    hz4_page_t*  page;   // source page (carry 元の page)
} hz4_carry_slot_t;

typedef struct hz4_carry {
    uint8_t          n;                          // 使用中スロット数 (0..HZ4_CARRY_SLOTS)
    uint8_t          pad[7];
    hz4_carry_slot_t slot[HZ4_CARRY_SLOTS];      // LIFO stack
} hz4_carry_t;

#if HZ4_DECOMMIT_DELAY_QUEUE
// Phase 2: Decommit Delay Queue (owner-thread only, SPSC)
typedef struct {
    hz4_page_t* head;
    hz4_page_t* tail;
} hz4_decommit_queue_t;
#endif

#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
// StatsStashBox: TLS-local counters for hot path aggregation
typedef struct hz4_os_stats_tls {
    uint64_t collect_calls;
    uint64_t refill_calls;
    uint64_t inbox_consume_calls;
    uint64_t inbox_lite_scan_calls;
    uint64_t inbox_lite_shortcut;
    // max系はTLSに載せない（正確なmax合流が困難）
    uint64_t carry_hit;
    uint64_t carry_miss;
    uint64_t drain_page_calls;
    uint64_t drain_page_objs;
    uint64_t page_used_dec_calls;
    uint64_t remote_flush_calls;
    uint64_t remote_flush_n1;
    uint64_t remote_flush_le4;
    uint64_t remote_flush_probe_ovf;
    uint64_t inbox_push_one_calls;
    uint64_t inbox_push_list_calls;
    uint64_t rf_rlen_1;
    uint64_t rf_rlen_2_4;
    uint64_t rf_rlen_5_8;
    uint64_t rf_rlen_9_16;
    uint64_t rf_rlen_17_32;
    uint64_t rf_rlen_33_64;
    uint64_t rf_rlen_65_127;
    uint64_t rf_rlen_ge_th;
} hz4_os_stats_tls_t;
#endif

#if HZ4_REMOTE_STAGING
// RemoteStagingBox v1 (Safe Option B)
typedef struct {
    void* head;
    void* tail;
    uint8_t owner;
    uint8_t sc;
    uint16_t count;
} hz4_remote_stash_t;
#endif

#if HZ4_REMOTE_PAGE_STAGING
// RemotePageStagingBox v1 (BatchIt-style micro-batch)
// Key: page pointer (direct-mapped). Value: small singly-linked list (head/tail,count).
typedef struct {
    hz4_page_t* page;
#if HZ4_REMOTE_PAGE_STAGING_META
    uint16_t count;
    uint8_t defer_retries;
    uint8_t _pad;
    uint16_t idx[HZ4_REMOTE_PAGE_STAGING_MAX];
#else
    void* head;
    void* tail;
    uint16_t count;
    uint16_t _pad;
#endif
} hz4_remote_page_stash_ent_t;
#endif

// Original merged struct
struct hz4_tls {
#if HZ4_TLS_MERGE
    // ---- Tcache (hot: bins first for locality) ----
    hz4_tcache_bin_t bins[HZ4_SC_MAX];
    hz4_seg_t* cur_seg;
    uint32_t next_page_idx;
    uint8_t owner;
    uint8_t _pad_merge[3];  // alignment padding
#endif

    // ---- Remote Buffer ----
    // 固定長配列: flush 時に page ごとにまとめて publish
    hz4_rbuf_ent_t rbuf[HZ4_RBUF_CAP];
    uint16_t rlen;  // current count in rbuf

#if HZ4_REMOTE_STAGING
    // ---- Remote Staging Box ----
    uint32_t remote_staging_tick;
    hz4_remote_stash_t remote_stash[HZ4_REMOTE_SHARDS];
#endif

#if HZ4_REMOTE_PAGE_STAGING
    // ---- RemotePageStagingBox ----
    uint32_t remote_page_staging_tick;
    hz4_remote_page_stash_ent_t remote_page_stash[HZ4_REMOTE_PAGE_STAGING_CACHE_N];
#endif

#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    // ---- RemotePageRbufGateBox ----
    uint32_t rbuf_gate_total_frees;   // total frees in current window
    uint32_t rbuf_gate_remote_frees;  // remote frees in current window
    uint32_t rbuf_gate_local_tick;    // local free sampling tick
    uint8_t rbuf_gate_on;             // gate state: 0=OFF (use fallback), 1=ON (use RBUF)
    uint8_t rbuf_gate_hold_left;      // windows left before state can flip
    uint8_t rbuf_gate_on_windows;     // consecutive evaluate windows with gate_on=1
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN
    // ---- RemoteDrainDemandBox ----
    uint8_t rbuf_demand_credits[HZ4_SC_MAX];
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY && \
    (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0)
    // ---- N20 fallback-pressure gate (TLS window counters) ----
    uint32_t rbufq_lazy_fb_tries;
    uint32_t rbufq_lazy_fb_fallback;
    uint8_t rbufq_lazy_fb_on;        // 1 while fallback pressure is high
    uint8_t rbufq_lazy_fb_hold_left; // windows left before state can flip
    uint8_t _pad_rbufq_lazy_fb[2];
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY && \
    (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0)
    // ---- N28 drain0-pressure gate (TLS window counters) ----
    uint32_t rbufq_lazy_drain0_tries;
    uint32_t rbufq_lazy_drain0_hits;
    uint8_t rbufq_lazy_drain0_on;        // 1 while drain0 pressure is high
    uint8_t rbufq_lazy_drain0_hold_left; // windows left before state can flip
    uint8_t _pad_rbufq_lazy_drain0[2];
#endif


#if HZ4_REMOTE_INBOX && HZ4_REMOTE_FLUSH_TLS_BUCKETS
    // ---- Remote Flush TLS Buckets (reuse across flush) ----
    uint32_t flush_epoch;
#if HZ4_REMOTE_FLUSH_PAGE_BUCKET
    hz4_flush_bucket_t pbuckets[HZ4_REMOTE_FLUSH_BUCKETS];
    uint32_t pbucket_stamp[HZ4_REMOTE_FLUSH_BUCKETS];
#else
    hz4_inbox_bucket_t ibuckets[HZ4_REMOTE_FLUSH_BUCKETS];
    uint32_t ibucket_stamp[HZ4_REMOTE_FLUSH_BUCKETS];
#if HZ4_REMOTE_FLUSH_ACTIVE
    uint32_t active_idx[HZ4_REMOTE_FLUSH_BUCKETS];
#endif
#endif
#endif

    // ---- Thread ID ----
    uint16_t tid;

    // ---- Carry (remainder) ----
    hz4_carry_t carry[HZ4_SC_MAX];

#if HZ4_REMOTE_INBOX
    // ---- Inbox stash (remainder from inbox consume) ----
    void* inbox_stash[HZ4_SC_MAX];
#endif

#if HZ4_REMOTE_INBOX && HZ4_INBOX_LITE
    // ---- Stage5-1b: Inbox-lite Box (epoch hint) ----
    uint32_t inbox_lite_epoch;
    uint8_t inbox_lite_nonempty;
    uint8_t _pad_inbox_lite[3];
#endif

#if HZ4_DECOMMIT_DELAY_QUEUE
    // ---- Phase 2: Decommit Delay Queue ----
    hz4_decommit_queue_t decommit_queue;
#if HZ4_RSSRETURN
    // RSSReturnBox: Control batch/threshold for decommit queue processing
    uint32_t rssreturn_pending;  // pages in decommit queue (O(1) tracking)
#if HZ4_RSSRETURN_GATEBOX
    uint32_t rssreturn_gate_last;  // last gate fire collect_count
#endif
#if HZ4_RSSRETURN_QUARANTINEBOX
    // RSSReturnQuarantineBox: number of pages currently held in quarantine (per-thread, bounded).
    uint32_t rss_quarantine_inflight;
    // Pages held out of decommit queue while waiting for maturity (bounded by inflight).
    // dqnext is reused as the link while quarantined (page is not in decommit_queue then).
    hz4_page_t* rss_quarantine_head;
    hz4_page_t* rss_quarantine_tail;
#endif
#endif
#if HZ4_RSSRETURN_COOLINGBOX
    hz4_page_t* cool_head;
    hz4_page_t* cool_tail;
    uint32_t cool_count;
#endif
#if HZ4_RSSRETURN && HZ4_RSSRETURN_RELEASEBOX
    uint32_t rssreturn_tokens;
    uint64_t rssreturn_release_last;  // last token refill collect_count (avoid double-add per tick)
#endif
#endif

    // DecommitReusePoolBox archived (NO-GO).

    // ---- Inbox probe tick (P3.2: inbox-only mode) ----
    uint32_t inbox_probe_tick;  // 自然 wrap OK (overflow 無害)

    // ---- Statistics (always present) ----
    uint64_t flush_count;
    uint64_t collect_count;

#if HZ4_TRIM_LITE
    // ---- Stage5-2: Trim-lite Box (amortize trim cost) ----
    uint32_t trim_lite_debt;
#endif

// ---- Phase 18/19: Shared seg_acq epoch tracking (GuardBox/GateBox/CoolPressure) ----
#if HZ4_SEG_ACQ_GUARDBOX || HZ4_SEG_ACQ_GATEBOX || HZ4_RSSRETURN_COOL_PRESSURE_SEG_ACQ
    uint32_t seg_acq_epoch;        // current epoch count (resets on gate/guard success)
    uint32_t seg_acq_epoch_stamp;  // last seen collect_count (for epoch reset detection)
#endif

#if HZ4_SEG_ACQ_GUARDBOX
    // ---- Phase 18: SegAcquireGuardBox ----
    uint32_t seg_acq_guard_last;   // last guard fire collect_count
#endif

#if HZ4_SEG_ACQ_GATEBOX
    // ---- Phase 19: SegAcquireGateBox ----
    uint32_t seg_acq_gate_last;    // last gate fire collect_count
#endif

#if HZ4_RSSRETURN && HZ4_RSSRETURN_PRESSUREGATEBOX
    // ---- PressureGateBox: seg_acq pressure-based quarantine control ----
    uint32_t seg_acq_epoch_last;   // 前回 seg_acq 値
    uint32_t collect_count_last;   // 前回判定時の collect_count
    bool rss_gate_on;              // 現在の Gate 状態
    uint32_t rss_gate_until;       // 状態変更までの hold 期限
    uint32_t seg_acq_delta_max;    // 最大増分（統計用）
#endif

#if HZ4_XFER_CACHE
    // ---- Phase 20: TransferCacheBox node cache ----
    hz4_xfer_batch_t* xfer_node_cache[HZ4_XFER_NODE_CACHE];
    uint8_t xfer_node_count;
#endif

#if HZ4_TRANSFERCACHE
    // ---- Phase 20B: TransferCacheBox node cache ----
    hz4_tcbox_batch_t* tcbox_node_cache[HZ4_TRANSFERCACHE_NODE_CACHE];
    uint8_t tcbox_node_count;
#endif

#if HZ4_STATS
    // ---- Extended Statistics (cold path only) ----
    uint64_t segs_popped;      // segq_pop_all で取得した seg 数
    uint64_t segs_drained;     // 実際に drain した seg 数
    uint64_t segs_requeued;    // finish で再 queue した seg 数
    uint64_t objs_drained;     // drain した object 総数
    uint64_t pages_drained;    // drain した page 数
    uint64_t scan_fallback;    // scan fallback が発動した回数
#endif
#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
    // ---- StatsStashBox: hot path counters (TLS-local) ----
    hz4_os_stats_tls_t stats_tls;
#endif

#if HZ4_SEG_REUSE_POOL
    // ---- SegReuseBox v1: TLS segment reuse pool ----
    hz4_seg_t* seg_reuse_pool[HZ4_SEG_REUSE_POOL_MAX];
    uint32_t seg_reuse_pool_n;
#endif
};

// ============================================================================
// TLS initialization
// ============================================================================
static inline void hz4_tls_init(hz4_tls_t* tls, uint16_t tid) {
    tls->rlen = 0;
    tls->tid = tid;
    tls->inbox_probe_tick = 0;
    tls->flush_count = 0;
    tls->collect_count = 0;
#if HZ4_TRIM_LITE
    tls->trim_lite_debt = 0;
#endif
#if HZ4_REMOTE_INBOX && HZ4_REMOTE_FLUSH_TLS_BUCKETS
    tls->flush_epoch = 0;
#if HZ4_REMOTE_FLUSH_PAGE_BUCKET
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        tls->pbucket_stamp[i] = 0;
    }
#else
    for (uint32_t i = 0; i < HZ4_REMOTE_FLUSH_BUCKETS; i++) {
        tls->ibucket_stamp[i] = 0;
    }
#endif
#endif
#if HZ4_SEG_ACQ_GUARDBOX || HZ4_SEG_ACQ_GATEBOX || HZ4_RSSRETURN_COOL_PRESSURE_SEG_ACQ
    tls->seg_acq_epoch = 0;
    tls->seg_acq_epoch_stamp = 0;
#endif
#if HZ4_SEG_ACQ_GUARDBOX
    tls->seg_acq_guard_last = 0;
#endif
#if HZ4_SEG_ACQ_GATEBOX
    tls->seg_acq_gate_last = 0;
#endif
#if HZ4_RSSRETURN && HZ4_RSSRETURN_PRESSUREGATEBOX
    tls->seg_acq_epoch_last = 0;
    tls->collect_count_last = 0;
    tls->rss_gate_on = false;
    tls->rss_gate_until = 0;
    tls->seg_acq_delta_max = 0;
#endif
#if HZ4_XFER_CACHE
    tls->xfer_node_count = 0;
    for (uint8_t i = 0; i < HZ4_XFER_NODE_CACHE; i++) {
        tls->xfer_node_cache[i] = NULL;
    }
#endif
#if HZ4_TRANSFERCACHE
    tls->tcbox_node_count = 0;
    for (uint8_t i = 0; i < HZ4_TRANSFERCACHE_NODE_CACHE; i++) {
        tls->tcbox_node_cache[i] = NULL;
    }
#endif
    for (uint32_t i = 0; i < HZ4_SC_MAX; i++) {
        tls->carry[i].n = 0;
        for (uint32_t s = 0; s < HZ4_CARRY_SLOTS; s++) {
            tls->carry[i].slot[s].head = NULL;
            tls->carry[i].slot[s].page = NULL;
        }
#if HZ4_REMOTE_INBOX
        tls->inbox_stash[i] = NULL;
#endif
    }
#if HZ4_REMOTE_INBOX && HZ4_INBOX_LITE
    tls->inbox_lite_epoch = 0;
    tls->inbox_lite_nonempty = 0;
#endif
#if HZ4_TLS_MERGE
    // Tcache bins init (cold path via hz4_tls_init_slow)
    for (uint32_t j = 0; j < HZ4_SC_MAX; j++) {
        tls->bins[j].head = NULL;
        tls->bins[j].count = 0;
#if HZ4_POPULATE_BATCH
        tls->bins[j].bump_page = NULL;
#endif
    }
    tls->cur_seg = NULL;
    tls->next_page_idx = HZ4_PAGE_IDX_MIN;
    tls->owner = (uint8_t)hz4_owner_shard(tid);  // 1回だけ計算、以降更新しない
#endif
#if HZ4_DECOMMIT_DELAY_QUEUE
    tls->decommit_queue.head = NULL;
    tls->decommit_queue.tail = NULL;
#if HZ4_RSSRETURN
    tls->rssreturn_pending = 0;
#if HZ4_RSSRETURN_GATEBOX
    tls->rssreturn_gate_last = 0;
#endif
#if HZ4_RSSRETURN_QUARANTINEBOX
    tls->rss_quarantine_inflight = 0;
    tls->rss_quarantine_head = NULL;
    tls->rss_quarantine_tail = NULL;
#endif
#endif
#if HZ4_RSSRETURN_COOLINGBOX
    tls->cool_head = NULL;
    tls->cool_tail = NULL;
    tls->cool_count = 0;
#endif
#if HZ4_RSSRETURN && HZ4_RSSRETURN_RELEASEBOX
    tls->rssreturn_tokens = 0;
    tls->rssreturn_release_last = 0;
#endif
#endif
    // DecommitReusePoolBox archived (NO-GO).
#if HZ4_STATS
    tls->segs_popped = 0;
    tls->segs_drained = 0;
    tls->segs_requeued = 0;
    tls->objs_drained = 0;
    tls->pages_drained = 0;
    tls->scan_fallback = 0;
#endif
#if HZ4_OS_STATS && HZ4_OS_STATS_FAST
    tls->stats_tls = (hz4_os_stats_tls_t){0};
#endif
#if HZ4_SEG_REUSE_POOL
    tls->seg_reuse_pool_n = 0;
#endif
#if HZ4_REMOTE_STAGING
    for (uint32_t i = 0; i < HZ4_REMOTE_SHARDS; i++) {
        tls->remote_stash[i].head = NULL;
        tls->remote_stash[i].tail = NULL;
        tls->remote_stash[i].count = 0;
    }
    tls->remote_staging_tick = 0;
#endif
#if HZ4_REMOTE_PAGE_STAGING
    for (uint32_t i = 0; i < HZ4_REMOTE_PAGE_STAGING_CACHE_N; i++) {
        tls->remote_page_stash[i].page = NULL;
#if HZ4_REMOTE_PAGE_STAGING_META
        tls->remote_page_stash[i].count = 0;
#else
        tls->remote_page_stash[i].head = NULL;
        tls->remote_page_stash[i].tail = NULL;
        tls->remote_page_stash[i].count = 0;
#endif
    }
    tls->remote_page_staging_tick = 0;
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    tls->rbuf_gate_total_frees = 0;
    tls->rbuf_gate_remote_frees = 0;
    tls->rbuf_gate_local_tick = 0;
    tls->rbuf_gate_on = 0;  // start with gate OFF (conservative)
    tls->rbuf_gate_hold_left = 0;
    tls->rbuf_gate_on_windows = 0;
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN
    for (uint32_t i = 0; i < HZ4_SC_MAX; i++) {
        tls->rbuf_demand_credits[i] = 0;
    }
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY && \
    (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_FALLBACK_HI_PCT > 0)
    tls->rbufq_lazy_fb_tries = 0;
    tls->rbufq_lazy_fb_fallback = 0;
    tls->rbufq_lazy_fb_on = 0;
    tls->rbufq_lazy_fb_hold_left = 0;
#endif
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY && \
    (HZ4_REMOTE_PAGE_RBUF_LAZY_NOTIFY_DRAIN0_HI_PCT > 0)
    tls->rbufq_lazy_drain0_tries = 0;
    tls->rbufq_lazy_drain0_hits = 0;
    tls->rbufq_lazy_drain0_on = 0;
    tls->rbufq_lazy_drain0_hold_left = 0;
#endif
    // rbuf は使用時に初期化されるので memset 不要
}

// ============================================================================
// Remote buffer operations (hot path)
// ============================================================================

// Check if buffer needs flush
static inline bool hz4_rbuf_full(hz4_tls_t* tls) {
    return tls->rlen >= HZ4_RBUF_CAP;
}

// Reset buffer after flush
static inline void hz4_rbuf_reset(hz4_tls_t* tls) {
    tls->rlen = 0;
}

// ============================================================================
// Stage5-3: PressureGate v0 (use rbuf gate as pressure signal)
// ============================================================================
static inline bool hz4_pressure_gate_on(hz4_tls_t* tls) {
#if HZ4_PRESSURE_GATE && HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    return tls->rbuf_gate_on != 0;
#else
    (void)tls;
    return false;
#endif
}

static inline void hz4_remote_drain_demand_note_underflow(hz4_tls_t* tls, uint8_t sc) {
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN
    uint8_t cur = tls->rbuf_demand_credits[sc];
    uint8_t add = (uint8_t)HZ4_REMOTE_PAGE_RBUF_DEMAND_CREDITS;
    uint16_t next = (uint16_t)cur + (uint16_t)add;
    tls->rbuf_demand_credits[sc] = (next > 255u) ? 255u : (uint8_t)next;
#else
    (void)tls;
    (void)sc;
#endif
}

static inline bool hz4_remote_drain_demand_try_consume(hz4_tls_t* tls, uint8_t sc) {
#if HZ4_REMOTE_PAGE_RBUF && HZ4_REMOTE_PAGE_RBUF_DEMAND_DRAIN
#if HZ4_REMOTE_PAGE_RBUF_DEMAND_ALLOW_GATE_ON && HZ4_REMOTE_PAGE_RBUF_GATEBOX
    if (tls->rbuf_gate_on) {
        return true;
    }
#endif
    uint8_t cur = tls->rbuf_demand_credits[sc];
    if (cur == 0) {
        return false;
    }
    tls->rbuf_demand_credits[sc] = (uint8_t)(cur - 1u);
    return true;
#else
    (void)tls;
    (void)sc;
    return true;
#endif
}

// ============================================================================
// Boundary API declarations (implemented in hz4_remote_flush.inc)
// ============================================================================
void hz4_remote_flush(hz4_tls_t* tls);

// ============================================================================
// Statistics dump (for benchmarking)
// ============================================================================
#if HZ4_STATS
#include <stdio.h>
static inline void hz4_stats_dump(hz4_tls_t* tls) {
    printf("--- HZ4 TLS Stats (tid=%u) ---\n", tls->tid);
    printf("flush_count:    %lu\n", (unsigned long)tls->flush_count);
    printf("collect_count:  %lu\n", (unsigned long)tls->collect_count);
    printf("segs_popped:    %lu\n", (unsigned long)tls->segs_popped);
    printf("segs_drained:   %lu\n", (unsigned long)tls->segs_drained);
    printf("segs_requeued:  %lu\n", (unsigned long)tls->segs_requeued);
    printf("objs_drained:   %lu\n", (unsigned long)tls->objs_drained);
    printf("pages_drained:  %lu\n", (unsigned long)tls->pages_drained);
    printf("scan_fallback:  %lu\n", (unsigned long)tls->scan_fallback);
}
#endif

#endif // HZ4_TLS_H
