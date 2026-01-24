#pragma once

#include <stdint.h>
#include <stdatomic.h>

#include "hz3_types.h"
#include "hz3_config.h"

#define HZ3_SEG_HDR_MAGIC 0x485a335345474844ULL  // "HZ3SEGH"
#define HZ3_SEG_KIND_SMALL 1u

typedef struct Hz3SegHdr {
    uint64_t magic;
    uint8_t  kind;
    uint8_t  owner;
    uint16_t reserved;
    uint32_t flags;
    uint16_t free_pages;
    uint16_t reserved2;
    uint32_t reserved3;
    uint64_t free_bits[HZ3_BITMAP_WORDS];
    uint64_t sub4k_run_start_bits[HZ3_BITMAP_WORDS];  // S62-1b: sub4k run start pages

#if HZ3_LANE_T16_R90_PAGE_REMOTE
    _Atomic(uint8_t) t16_qstate;  // 0=idle, 1=queued, 2=processing
    struct Hz3SegHdr* t16_qnext;  // intrusive MPSC queue link
    _Atomic(uint64_t) t16_remote_pending[HZ3_BITMAP_WORDS];  // per-page remote hint bitmap
#endif

#if HZ3_S110_META_ENABLE
    // S110: per-page bin metadata for free fast path (segment-local lookup)
    // 0 = unknown/not-allocated
    // 1..N = bin + 1 (small/sub4k bins)
    // Atomic for future S110-1 free fast path use (store-release/load-acquire)
    _Atomic(uint16_t) page_bin_plus1[HZ3_PAGES_PER_SEG];  // +1024 bytes (default: 512 pages)
#endif
} Hz3SegHdr;

// ptr -> segment header (self-describing); returns NULL if not hz3
Hz3SegHdr* hz3_seg_from_ptr(const void* ptr);

// Allocate a small segment inside arena and return its header
Hz3SegHdr* hz3_seg_alloc_small(uint8_t owner);
