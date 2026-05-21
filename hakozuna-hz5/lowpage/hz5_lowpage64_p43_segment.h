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

typedef struct Hz5Lowpage64P43StatsSnapshot {
  size_t segments_reserved;
  size_t segments_released;
  size_t slot_commits;
  size_t slot_commit_failures;
  size_t slot_decommits;
  size_t slot_decommit_failures;
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
