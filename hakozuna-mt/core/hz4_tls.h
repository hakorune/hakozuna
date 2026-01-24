// hz4_tls.h - HZ4 Thread-Local State
#ifndef HZ4_TLS_H
#define HZ4_TLS_H

#include "hz4_types.h"
#include "hz4_page.h"

// ============================================================================
// Tcache Bin (MERGE mode only)
// ============================================================================
#if HZ4_TLS_MERGE
typedef struct hz4_tcache_bin {
    void* head;
    uint32_t count;
} hz4_tcache_bin_t;
#endif

// ============================================================================
// Remote Buffer Entry
// ============================================================================
// 固定長配列で stale entry 問題を回避 (hz3 Lane16 の教訓)
typedef struct hz4_rbuf_ent {
    hz4_page_t* page;
    void* obj;
} hz4_rbuf_ent_t;

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

    // ---- Thread ID ----
    uint16_t tid;

    // ---- Carry (remainder) ----
    hz4_carry_t carry[HZ4_SC_MAX];

#if HZ4_REMOTE_INBOX
    // ---- Inbox stash (remainder from inbox consume) ----
    void* inbox_stash[HZ4_SC_MAX];
#endif

    // ---- Inbox probe tick (P3.2: inbox-only mode) ----
    uint32_t inbox_probe_tick;  // 自然 wrap OK (overflow 無害)

    // ---- Statistics (always present) ----
    uint64_t flush_count;
    uint64_t collect_count;

#if HZ4_STATS
    // ---- Extended Statistics (cold path only) ----
    uint64_t segs_popped;      // segq_pop_all で取得した seg 数
    uint64_t segs_drained;     // 実際に drain した seg 数
    uint64_t segs_requeued;    // finish で再 queue した seg 数
    uint64_t objs_drained;     // drain した object 総数
    uint64_t pages_drained;    // drain した page 数
    uint64_t scan_fallback;    // scan fallback が発動した回数
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
#if HZ4_TLS_MERGE
    // Tcache bins init (cold path via hz4_tls_init_slow)
    for (uint32_t j = 0; j < HZ4_SC_MAX; j++) {
        tls->bins[j].head = NULL;
        tls->bins[j].count = 0;
    }
    tls->cur_seg = NULL;
    tls->next_page_idx = HZ4_PAGE_IDX_MIN;
    tls->owner = (uint8_t)hz4_owner_shard(tid);  // 1回だけ計算、以降更新しない
#endif
#if HZ4_STATS
    tls->segs_popped = 0;
    tls->segs_drained = 0;
    tls->segs_requeued = 0;
    tls->objs_drained = 0;
    tls->pages_drained = 0;
    tls->scan_fallback = 0;
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
