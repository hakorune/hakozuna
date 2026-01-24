#pragma once
// hz3_s65_medium_reclaim.h - S65-2C Medium Epoch Reclaim
//
// Port of S61 dtor reclaim to epoch tick for medium (4KB-32KB) runs.
// Uses boundary API (hz3_release_range_definitely_free_meta) instead of
// direct hz3_segment_free_run() to ensure single entry point for free_bits.

#include "hz3_config.h"

#if HZ3_S65_MEDIUM_RECLAIM

// Called from epoch tick (after S64 retire scan, before S65 ledger purge).
// Drains central bins and returns runs to segment via boundary API.
//
// Algorithm:
// 1. Pop runs from central bins (sc=7→0 for syscall efficiency)
// 2. Convert obj → seg_base → meta → page_idx
// 3. Call hz3_release_range_definitely_free_meta()
// 4. Update pack pool (hz3_pack_on_free)
//
// Budget: HZ3_S65_MEDIUM_RECLAIM_BUDGET_RUNS (runs, not pages)
void hz3_s65_medium_reclaim_tick(void);

#else

// Stub when disabled
static inline void hz3_s65_medium_reclaim_tick(void) {}

#endif  // HZ3_S65_MEDIUM_RECLAIM
