#ifndef HZ5_LOWPAGE64_P43_SEGMENT_H
#define HZ5_LOWPAGE64_P43_SEGMENT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#ifdef BENCHLAB_HZ5_P43_SEGMENT_SLOTS
#define HZ5_LOWPAGE64_P43_SEGMENT_SLOTS \
  BENCHLAB_HZ5_P43_SEGMENT_SLOTS
#else
#define HZ5_LOWPAGE64_P43_SEGMENT_SLOTS 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_SEGMENT_SIZE
#define HZ5_LOWPAGE64_P43_SEGMENT_SIZE (2u * 1024u * 1024u)
#endif

#ifndef HZ5_LOWPAGE64_P43_SLOT_SIZE
#define HZ5_LOWPAGE64_P43_SLOT_SIZE (128u * 1024u)
#endif

#ifndef HZ5_LOWPAGE64_P43_SLOT_COUNT
#define HZ5_LOWPAGE64_P43_SLOT_COUNT \
  (HZ5_LOWPAGE64_P43_SEGMENT_SIZE / HZ5_LOWPAGE64_P43_SLOT_SIZE)
#endif

/*
 * P43 is split into small research knobs so safety and speed questions do not
 * blur together:
 * - SegmentSlots changes the raw allocation source to 2MiB segment slots.
 * - SlotDecommit/PageNoAccess are safety/RSS probes and must keep metadata out
 *   of inactive data pages.
 * - FastLookup/LocklessLookup only probe pointer-to-slot lookup overhead.
 */
#ifndef HZ5_LOWPAGE64_P43_SLOT_DECOMMIT
#ifdef BENCHLAB_HZ5_P43_SLOT_DECOMMIT
#define HZ5_LOWPAGE64_P43_SLOT_DECOMMIT \
  BENCHLAB_HZ5_P43_SLOT_DECOMMIT
#else
#define HZ5_LOWPAGE64_P43_SLOT_DECOMMIT 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS
#ifdef BENCHLAB_HZ5_P43_DESCRIPTOR_LISTS
#define HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS \
  BENCHLAB_HZ5_P43_DESCRIPTOR_LISTS
#else
#define HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_PAGE_NOACCESS
#ifdef BENCHLAB_HZ5_P43_PAGE_NOACCESS
#define HZ5_LOWPAGE64_P43_PAGE_NOACCESS \
  BENCHLAB_HZ5_P43_PAGE_NOACCESS
#else
#define HZ5_LOWPAGE64_P43_PAGE_NOACCESS 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_REWARM_BATCH
#ifdef BENCHLAB_HZ5_P43_REWARM_BATCH
#define HZ5_LOWPAGE64_P43_REWARM_BATCH \
  BENCHLAB_HZ5_P43_REWARM_BATCH
#else
#define HZ5_LOWPAGE64_P43_REWARM_BATCH 1u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP
#ifdef BENCHLAB_HZ5_P43_COMMITTED_RETAIN_CAP
#define HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP \
  BENCHLAB_HZ5_P43_COMMITTED_RETAIN_CAP
#else
#define HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_TLS_CACHE_CAP
#ifdef BENCHLAB_HZ5_P43_TLS_CACHE_CAP
#define HZ5_LOWPAGE64_P43_TLS_CACHE_CAP \
  BENCHLAB_HZ5_P43_TLS_CACHE_CAP
#else
#define HZ5_LOWPAGE64_P43_TLS_CACHE_CAP 0u
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_FAST_LOOKUP
#ifdef BENCHLAB_HZ5_P43_FAST_LOOKUP
#define HZ5_LOWPAGE64_P43_FAST_LOOKUP \
  BENCHLAB_HZ5_P43_FAST_LOOKUP
#else
#define HZ5_LOWPAGE64_P43_FAST_LOOKUP 0
#endif
#endif

#ifndef HZ5_LOWPAGE64_P43_FAST_LOOKUP_BITS
#define HZ5_LOWPAGE64_P43_FAST_LOOKUP_BITS 12u
#endif

#ifndef HZ5_LOWPAGE64_P43_FAST_LOOKUP_CAP
#define HZ5_LOWPAGE64_P43_FAST_LOOKUP_CAP \
  (1u << HZ5_LOWPAGE64_P43_FAST_LOOKUP_BITS)
#endif

#ifndef HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP
#ifdef BENCHLAB_HZ5_P43_LOCKLESS_LOOKUP
#define HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP \
  BENCHLAB_HZ5_P43_LOCKLESS_LOOKUP
#else
#define HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP 0
#endif
#endif

#if HZ5_LOWPAGE64_P43_SLOT_DECOMMIT && HZ5_LOWPAGE64_P43_PAGE_NOACCESS
#error "P43 SlotDecommit and PageNoAccess are mutually exclusive"
#endif

#if HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP && !HZ5_LOWPAGE64_P43_FAST_LOOKUP
#error "P43 LocklessLookup requires FastLookup"
#endif

#if HZ5_LOWPAGE64_P43_LOCKLESS_LOOKUP && \
    (HZ5_LOWPAGE64_P43_SLOT_DECOMMIT || HZ5_LOWPAGE64_P43_PAGE_NOACCESS)
#error "P43 LocklessLookup is diagnostic-only and cannot run with slot decommit/PageNoAccess"
#endif

typedef struct Hz5Lowpage64P43StatsSnapshot {
  size_t segments_reserved;
  size_t segments_released;
  size_t slot_commits;
  size_t slot_commit_failures;
  size_t slot_decommits;
  size_t slot_decommit_failures;
  size_t tls_hits;
  size_t tls_pushes;
  size_t tls_overflow_pushes;
  size_t global_hits;
  size_t global_pushes;
  size_t cold_hits;
  size_t free_slot_hits;
  size_t lookup_calls;
  size_t lookup_active;
  size_t lookup_nonactive;
  size_t lookup_miss;
  size_t lookup_segments_scanned_total;
  size_t lookup_segments_scanned_max;
  size_t lookup_fast_hits;
  size_t lookup_fast_misses;
  size_t lookup_lockless_hits;
  size_t lookup_lockless_misses;
  size_t find_fast_hits;
  size_t find_segments_scanned_total;
  size_t find_segments_scanned_max;
  size_t va_allocs;
  size_t va_alloc_failures;
  size_t va_releases;
} Hz5Lowpage64P43StatsSnapshot;

#ifndef HZ5_LOWPAGE64_LOOKUP_ENUM
#define HZ5_LOWPAGE64_LOOKUP_ENUM
enum {
  HZ5_LOWPAGE64_LOOKUP_MISS = 0,
  HZ5_LOWPAGE64_LOOKUP_OWNED_ACTIVE = 1,
  HZ5_LOWPAGE64_LOOKUP_OWNED_NONACTIVE = 2
};
#endif

void* hz5_lowpage64_p43_alloc_slot(size_t raw_bytes);
int hz5_lowpage64_p43_release_slot(void* raw);
int hz5_lowpage64_p43_lookup(void* ptr);
int hz5_lowpage64_p43_may_own(void* ptr);
int hz5_lowpage64_p43_active_owns(void* ptr);
void hz5_lowpage64_p43_stats_snapshot(
    Hz5Lowpage64P43StatsSnapshot* snapshot);

#ifdef __cplusplus
}
#endif

#endif
