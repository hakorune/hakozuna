#pragma once

// ============================================================================
// S61: DtorHardPurgeBox
// ============================================================================
//
// Thread-exit (destructor) hard purge for RSS reduction.
// Drains central pool → segment, then madvise(DONTNEED) fully-free segments.
//
// Key design:
// - my_shard only (no cross-shard locks)
// - Segment-level madvise (not per-run) for efficient syscall usage
// - Cold path only (destructor, no hot path impact)
//
// Safety:
// - Arena bounds check before madvise
// - Budget limits (MAX_PAGES, MAX_CALLS)
// - madvise return value checked (success/fail stats)

#include "hz3_config.h"
#include <stdint.h>

#if HZ3_S61_DTOR_HARD_PURGE

// Thread-exit only (cold). my_shard only.
// Drains central → segment, applies madvise(DONTNEED) to fully-free segments.
void hz3_s61_dtor_hard_purge(void);

#endif  // HZ3_S61_DTOR_HARD_PURGE
