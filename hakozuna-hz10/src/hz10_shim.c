#include "hz10_freelist_page.h"
#include "hz10_large_alloc.h"
#include "hz10_page_pool.h"
#include "hz10_pagemap.h"
#include "hz10_public_entry.h"
#include "hz10_public_entry_owner.h"
#include "hz10_size_class.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
#define HZ10_ENABLE_SHIM_THREAD_EXIT_STATS 0
#endif

static _Atomic(uint64_t) hz10_shim_foreign_free_count;
static int hz10_shim_process_exit_stats;
#if HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
static int hz10_shim_thread_exit_stats;
#endif
static int hz10_shim_exit_stats_classes;
static unsigned hz10_shim_census_sec;
#if HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
static pthread_once_t hz10_shim_thread_stats_once = PTHREAD_ONCE_INIT;
static pthread_key_t hz10_shim_thread_stats_key;
static int hz10_shim_thread_stats_key_ready;
static char hz10_shim_thread_stats_marker;
#endif
static _Thread_local int hz10_shim_in_stats_dump;

static int hz10_shim_is_power_of_two(size_t value) {
  return value != 0u && (value & (value - 1u)) == 0u;
}

static void hz10_shim_atfork_prepare(void) {
  hz10_pagemap_atfork_prepare();
  hz10_freelist_page_atfork_prepare();
}

static void hz10_shim_atfork_parent(void) {
  hz10_freelist_page_atfork_parent();
  hz10_pagemap_atfork_parent();
}

static void hz10_shim_atfork_child(void) {
  hz10_freelist_page_atfork_child();
  hz10_pagemap_atfork_child();
}

static int hz10_shim_exit_stats_enabled(void) {
  const char* value = getenv("HZ10_SHIM_EXIT_STATS");
  return value && value[0] == '1' && value[1] == '\0';
}

/* Measurement-only env knobs:
 * - HZ10_SHIM_EXIT_STATS=1 dumps process-exit stats.
 * - HZ10_SHIM_THREAD_EXIT_STATS=1 dumps per-thread stats from a pthread
 *   destructor only in libhz10_thread_stats.so. The default preload lane
 *   compiles this path out and ignores the env knob.
 * - HZ10_SHIM_EXIT_STATS_CLASSES=0 suppresses per-class rows; summaries and
 *   class_totals still print. Default is class rows ON for small probes. */
#if HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
static int hz10_shim_thread_exit_stats_enabled(void) {
  const char* value = getenv("HZ10_SHIM_THREAD_EXIT_STATS");
  return value && value[0] == '1' && value[1] == '\0';
}
#endif

static int hz10_shim_exit_stats_classes_enabled(void) {
  const char* value = getenv("HZ10_SHIM_EXIT_STATS_CLASSES");
  return !(value && value[0] == '0' && value[1] == '\0');
}

static unsigned hz10_shim_census_seconds(void) {
  const char* value = getenv("HZ10_SHIM_CENSUS_SEC");
  if (!value || !value[0]) {
    return 0u;
  }
  char* end = NULL;
  unsigned long parsed = strtoul(value, &end, 10);
  if (end == value || (end && *end != '\0') || parsed == 0ul) {
    return 0u;
  }
  if (parsed > 3600ul) {
    parsed = 3600ul;
  }
  return (unsigned)parsed;
}

static void hz10_shim_write_all(const char* text, size_t len);

static void hz10_shim_writef(const char* fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) {
    return;
  }
  size_t len = (size_t)n;
  if (len >= sizeof(buf)) {
    len = sizeof(buf) - 1u;
  }
  hz10_shim_write_all(buf, len);
}

void hz10_shim_dump_exit_stats(void) {
  if (hz10_shim_in_stats_dump) {
    return;
  }
  hz10_shim_in_stats_dump = 1;
  uint64_t foreign_frees = atomic_load_explicit(&hz10_shim_foreign_free_count,
                                                memory_order_relaxed);
  Hz10FreelistMetadataStats metadata = {0};
  hz10_freelist_metadata_stats(&metadata);

  uint32_t pool_cached = hz10_page_pool_cached_count();
  uint64_t pool_reuse = hz10_page_pool_reuse_count();
  uint64_t pool_release = hz10_page_pool_release_count();
  uint64_t pool_purged = hz10_page_pool_purged_count();

  uint64_t active_pages = 0u;
  uint64_t retired_pages = 0u;
  uint64_t max_retired_pages = 0u;
  uint64_t eviction_count = 0u;
  uint64_t retired_count = 0u;
  uint64_t ready_reclaimed = 0u;
  uint64_t sweep_reclaimed = 0u;
  uint64_t local_free_reclaimed = 0u;
  uint64_t orphan_published = 0u;
  uint64_t orphan_popped = 0u;
  uint64_t orphan_adopted = 0u;
  uint64_t orphan_reject_class = 0u;
  uint64_t orphan_reject_state = 0u;
  uint64_t orphan_reject_no_capacity = 0u;
  uint64_t orphan_repush = 0u;
  uint64_t orphan_depth = 0u;
  uint64_t orphan_max_depth_sum = 0u;

  hz10_shim_writef(
      "hz10_shim_exit_stats summary class_count=%u foreign_frees=%llu "
      "pool_cached=%u pool_cached_bytes=%llu pool_reuse=%llu "
      "pool_release=%llu pool_purged=%llu metadata_slabs=%llu "
      "metadata_slab_bytes=%u metadata_node_bytes=%u "
      "metadata_capacity=%llu metadata_live=%llu metadata_free=%llu\n",
      (unsigned)HZ10_CLASS_COUNT, (unsigned long long)foreign_frees,
      (unsigned)pool_cached,
      (unsigned long long)pool_cached * (unsigned long long)HZ10_PAGE_QUANTUM,
      (unsigned long long)pool_reuse, (unsigned long long)pool_release,
      (unsigned long long)pool_purged,
      (unsigned long long)metadata.slab_count, (unsigned)metadata.slab_bytes,
      (unsigned)metadata.node_bytes,
      (unsigned long long)metadata.node_capacity,
      (unsigned long long)metadata.live_nodes,
      (unsigned long long)metadata.free_nodes);

  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassPageListStats stats = {0};
    hz10_public_entry_class_list_stats(c, &stats);
    if (stats.active_length == 0u && stats.retired_length == 0u &&
        stats.max_retired_length == 0u && stats.eviction_count == 0u &&
        stats.retired_count == 0u) {
      continue;
    }

    uint32_t slot_size = hz10_size_class_slot_size(c);
    uint32_t slot_count = hz10_size_class_slot_count(c);
    active_pages += stats.active_length;
    retired_pages += stats.retired_length;
    max_retired_pages += stats.max_retired_length;
    eviction_count += stats.eviction_count;
    retired_count += stats.retired_count;
    ready_reclaimed += stats.retired_reclaimed_by_ready_count;
    sweep_reclaimed += stats.retired_reclaimed_by_sweep_count;
    local_free_reclaimed += stats.retired_reclaimed_by_local_free_count;

    if (hz10_shim_exit_stats_classes) {
      hz10_shim_writef(
          "hz10_shim_exit_stats class=%u slot_size=%u slot_count=%u "
          "active_pages=%u retired_pages=%u max_retired=%u "
          "evictions=%llu retired=%llu reclaimed_ready=%llu "
          "reclaimed_sweep=%llu reclaimed_local_free=%llu "
          "find_calls=%llu find_misses=%llu find_pages_visited=%llu\n",
          (unsigned)c, (unsigned)slot_size, (unsigned)slot_count,
          (unsigned)stats.active_length, (unsigned)stats.retired_length,
          (unsigned)stats.max_retired_length,
          (unsigned long long)stats.eviction_count,
          (unsigned long long)stats.retired_count,
          (unsigned long long)stats.retired_reclaimed_by_ready_count,
          (unsigned long long)stats.retired_reclaimed_by_sweep_count,
          (unsigned long long)stats.retired_reclaimed_by_local_free_count,
          (unsigned long long)stats.find_call_count,
          (unsigned long long)stats.find_miss_count,
          (unsigned long long)stats.find_pages_visited_count);
    }
  }

  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10OrphanAdoptionClassStats stats = {0};
    hz10_public_entry_orphan_adoption_class_stats(c, &stats);
    if (stats.published_pages == 0u && stats.pop_count == 0u &&
        stats.adopt_count == 0u && stats.reject_class_count == 0u &&
        stats.reject_state_count == 0u &&
        stats.reject_no_capacity_count == 0u && stats.repush_count == 0u &&
        stats.depth == 0u && stats.max_depth == 0u) {
      continue;
    }

    orphan_published += stats.published_pages;
    orphan_popped += stats.pop_count;
    orphan_adopted += stats.adopt_count;
    orphan_reject_class += stats.reject_class_count;
    orphan_reject_state += stats.reject_state_count;
    orphan_reject_no_capacity += stats.reject_no_capacity_count;
    orphan_repush += stats.repush_count;
    orphan_depth += stats.depth;
    orphan_max_depth_sum += stats.max_depth;

    if (hz10_shim_exit_stats_classes) {
      hz10_shim_writef(
          "hz10_shim_orphan_adoption_stats class=%u slot_size=%u "
          "slot_count=%u published=%llu pop=%llu adopted=%llu "
          "reject_class=%llu reject_state=%llu reject_no_capacity=%llu "
          "repush=%llu depth=%llu max_depth=%llu\n",
          (unsigned)c, (unsigned)hz10_size_class_slot_size(c),
          (unsigned)hz10_size_class_slot_count(c),
          (unsigned long long)stats.published_pages,
          (unsigned long long)stats.pop_count,
          (unsigned long long)stats.adopt_count,
          (unsigned long long)stats.reject_class_count,
          (unsigned long long)stats.reject_state_count,
          (unsigned long long)stats.reject_no_capacity_count,
          (unsigned long long)stats.repush_count,
          (unsigned long long)stats.depth,
          (unsigned long long)stats.max_depth);
    }
  }

  hz10_shim_writef(
      "hz10_shim_exit_stats class_totals active_pages=%llu "
      "retired_pages=%llu max_retired_sum=%llu page_bytes=%llu "
      "evictions=%llu retired=%llu reclaimed_ready=%llu "
      "reclaimed_sweep=%llu reclaimed_local_free=%llu\n",
      (unsigned long long)active_pages, (unsigned long long)retired_pages,
      (unsigned long long)max_retired_pages,
      (unsigned long long)(active_pages + retired_pages) *
          (unsigned long long)HZ10_PAGE_QUANTUM,
      (unsigned long long)eviction_count, (unsigned long long)retired_count,
      (unsigned long long)ready_reclaimed,
      (unsigned long long)sweep_reclaimed,
      (unsigned long long)local_free_reclaimed);
  hz10_shim_writef(
      "hz10_shim_orphan_adoption_stats totals published=%llu pop=%llu "
      "adopted=%llu reject_class=%llu reject_state=%llu "
      "reject_no_capacity=%llu repush=%llu depth=%llu max_depth_sum=%llu\n",
      (unsigned long long)orphan_published,
      (unsigned long long)orphan_popped,
      (unsigned long long)orphan_adopted,
      (unsigned long long)orphan_reject_class,
      (unsigned long long)orphan_reject_state,
      (unsigned long long)orphan_reject_no_capacity,
      (unsigned long long)orphan_repush, (unsigned long long)orphan_depth,
      (unsigned long long)orphan_max_depth_sum);
  hz10_shim_in_stats_dump = 0;
}

typedef enum Hz10ShimCensusBucket {
  HZ10_CENSUS_LIVE_ACTIVE = 0,
  HZ10_CENSUS_LIVE_RETIRED = 1,
  HZ10_CENSUS_ORPHAN_ACTIVE = 2,
  HZ10_CENSUS_ORPHAN_RETIRED = 3,
  HZ10_CENSUS_ADOPTED = 4,
  HZ10_CENSUS_UNKNOWN = 5,
  HZ10_CENSUS_BUCKET_COUNT = 6
} Hz10ShimCensusBucket;

typedef struct Hz10ShimCensusClassBucket {
  uint64_t pages;
  uint64_t page_bytes;
  uint64_t slot_capacity;
  uint64_t live_slots;
  uint64_t free_slots;
  uint64_t hidden_free_slots;
  uint64_t util_hist[8];
} Hz10ShimCensusClassBucket;

typedef struct Hz10ShimCensusTotals {
  Hz10ShimCensusClassBucket by_class[HZ10_CENSUS_BUCKET_COUNT]
                                    [HZ10_CLASS_COUNT];
  uint64_t large_pages;
  uint64_t skipped_records;
} Hz10ShimCensusTotals;

static const char* hz10_shim_census_bucket_name(
    Hz10ShimCensusBucket bucket) {
  switch (bucket) {
    case HZ10_CENSUS_LIVE_ACTIVE:
      return "owner_live_active";
    case HZ10_CENSUS_LIVE_RETIRED:
      return "owner_live_retired";
    case HZ10_CENSUS_ORPHAN_ACTIVE:
      return "orphan_unadopted";
    case HZ10_CENSUS_ORPHAN_RETIRED:
      return "orphan_retired";
    case HZ10_CENSUS_ADOPTED:
      return "adopted";
    default:
      return "unknown";
  }
}

static uint32_t hz10_shim_census_popcount_pending(Hz10FreelistPage* page) {
  uint32_t total = 0u;
  for (uint32_t i = 0; i < page->pending_words; ++i) {
    uint64_t word =
        atomic_load_explicit(&page->pending_bits[i], memory_order_acquire);
    total += (uint32_t)__builtin_popcountll(word);
  }
  if (total > page->slot_count) {
    total = page->slot_count;
  }
  return total;
}

static uint32_t hz10_shim_census_util_bucket(uint32_t live_slots,
                                             uint32_t slot_count) {
  if (live_slots == 0u) return 0u;
  if (live_slots == 1u) return 1u;
  if (live_slots <= 3u) return 2u;
  if (live_slots <= 7u) return 3u;
  if (live_slots <= 15u) return 4u;
  if (live_slots <= 63u) return 5u;
  if (live_slots == slot_count) return 7u;
  return 6u;
}

static void hz10_shim_census_add_page(Hz10ShimCensusTotals* totals,
                                      Hz10ShimCensusBucket bucket,
                                      Hz10FreelistPage* page) {
  uint32_t class_id = page->class_id;
  if (class_id >= HZ10_CLASS_COUNT) {
    bucket = HZ10_CENSUS_UNKNOWN;
    class_id = 0u;
  }
  Hz10ShimCensusClassBucket* cell = &totals->by_class[bucket][class_id];
  uint32_t free_count = page->free_count;
  if (free_count > page->slot_count) {
    free_count = page->slot_count;
  }
  uint32_t live_slots = page->slot_count - free_count;
  uint32_t hidden_free = hz10_shim_census_popcount_pending(page);
  cell->pages += 1u;
  cell->page_bytes += HZ10_PAGE_QUANTUM;
  cell->slot_capacity += page->slot_count;
  cell->live_slots += live_slots;
  cell->free_slots += free_count;
  cell->hidden_free_slots += hidden_free;
  cell->util_hist[hz10_shim_census_util_bucket(live_slots,
                                               page->slot_count)] += 1u;
}

static void hz10_shim_dump_census(void) {
  Hz10ShimCensusTotals totals = {0};
  for (uint32_t r = 0; r < HZ10_ROOT_SIZE; ++r) {
    H10Leaf* leaf = hz10_pagemap_leaf_load(r);
    if (!leaf) {
      continue;
    }
    for (uint32_t i = 0; i < HZ10_LEAF_SIZE; ++i) {
      H10PageRecord* rec = &leaf->entries[i];
      uint32_t gen0 = __atomic_load_n(&rec->generation, __ATOMIC_ACQUIRE);
      uint32_t slot_size = __atomic_load_n(&rec->slot_size, __ATOMIC_ACQUIRE);
      if (slot_size == 0u) {
        continue;
      }
      void* owner = __atomic_load_n(&rec->owner, __ATOMIC_RELAXED);
      uint32_t flags = __atomic_load_n(&rec->flags, __ATOMIC_RELAXED);
      uint32_t gen1 = __atomic_load_n(&rec->generation, __ATOMIC_ACQUIRE);
      if (gen0 != gen1 || !owner) {
        totals.skipped_records += 1u;
        continue;
      }
      if (flags != 0u) {
        totals.large_pages += 1u;
        continue;
      }
      Hz10FreelistPage* page = (Hz10FreelistPage*)owner;
      Hz10OwnerRecord* page_owner =
          (Hz10OwnerRecord*)hz10_freelist_page_owner_thread(page);
      uint32_t owner_state = hz10_public_entry_owner_state(page_owner);
      Hz10ShimCensusBucket bucket = HZ10_CENSUS_UNKNOWN;
      if (page->adopted_count != 0u) {
        bucket = HZ10_CENSUS_ADOPTED;
      } else if (owner_state == HZ10_THREAD_OWNER_STATE_LIVE) {
        bucket = page->owner_list_kind == HZ10_FREELIST_PAGE_OWNER_LIST_RETIRED
                     ? HZ10_CENSUS_LIVE_RETIRED
                     : HZ10_CENSUS_LIVE_ACTIVE;
      } else if (owner_state == HZ10_THREAD_OWNER_STATE_EXITED) {
        bucket = page->owner_list_kind == HZ10_FREELIST_PAGE_OWNER_LIST_RETIRED
                     ? HZ10_CENSUS_ORPHAN_RETIRED
                     : HZ10_CENSUS_ORPHAN_ACTIVE;
      }
      hz10_shim_census_add_page(&totals, bucket, page);
    }
  }

  Hz10FreelistMetadataStats metadata = {0};
  hz10_freelist_metadata_stats(&metadata);
  uint32_t pool_cached = hz10_page_pool_cached_count();

  uint64_t total_pages = 0u;
  uint64_t total_page_bytes = 0u;
  for (uint32_t b = 0; b < HZ10_CENSUS_BUCKET_COUNT; ++b) {
    for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
      Hz10ShimCensusClassBucket* cell = &totals.by_class[b][c];
      total_pages += cell->pages;
      total_page_bytes += cell->page_bytes;
    }
  }

  hz10_shim_writef(
      "hz10_shim_census summary pages=%llu page_bytes=%llu "
      "large_pages=%llu skipped_records=%llu pool_cached=%u "
      "pool_cached_bytes=%llu metadata_slabs=%llu metadata_bytes=%llu "
      "metadata_live_nodes=%llu metadata_free_nodes=%llu\n",
      (unsigned long long)total_pages,
      (unsigned long long)total_page_bytes,
      (unsigned long long)totals.large_pages,
      (unsigned long long)totals.skipped_records, (unsigned)pool_cached,
      (unsigned long long)pool_cached * (unsigned long long)HZ10_PAGE_QUANTUM,
      (unsigned long long)metadata.slab_count,
      (unsigned long long)metadata.slab_count *
          (unsigned long long)metadata.slab_bytes,
      (unsigned long long)metadata.live_nodes,
      (unsigned long long)metadata.free_nodes);

  for (uint32_t b = 0; b < HZ10_CENSUS_BUCKET_COUNT; ++b) {
    for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
      Hz10ShimCensusClassBucket* cell = &totals.by_class[b][c];
      if (cell->pages == 0u) {
        continue;
      }
      hz10_shim_writef(
          "hz10_shim_census bucket=%s class=%u slot_size=%u slot_count=%u "
          "pages=%llu page_bytes=%llu slot_capacity=%llu live_slots=%llu "
          "free_slots=%llu hidden_free_slots=%llu "
          "hist_live_0=%llu hist_live_1=%llu hist_live_2_3=%llu "
          "hist_live_4_7=%llu hist_live_8_15=%llu hist_live_16_63=%llu "
          "hist_live_64_plus=%llu hist_live_full=%llu\n",
          hz10_shim_census_bucket_name((Hz10ShimCensusBucket)b),
          (unsigned)c, (unsigned)hz10_size_class_slot_size(c),
          (unsigned)hz10_size_class_slot_count(c),
          (unsigned long long)cell->pages,
          (unsigned long long)cell->page_bytes,
          (unsigned long long)cell->slot_capacity,
          (unsigned long long)cell->live_slots,
          (unsigned long long)cell->free_slots,
          (unsigned long long)cell->hidden_free_slots,
          (unsigned long long)cell->util_hist[0],
          (unsigned long long)cell->util_hist[1],
          (unsigned long long)cell->util_hist[2],
          (unsigned long long)cell->util_hist[3],
          (unsigned long long)cell->util_hist[4],
          (unsigned long long)cell->util_hist[5],
          (unsigned long long)cell->util_hist[6],
          (unsigned long long)cell->util_hist[7]);
    }
  }
}

static void* hz10_shim_census_thread(void* arg) {
  unsigned seconds = (unsigned)(uintptr_t)arg;
  sleep(seconds);
  hz10_shim_dump_census();
  return NULL;
}

#if HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
static void hz10_shim_thread_stats_destructor(void* value) {
  if (!value || !hz10_shim_thread_exit_stats) {
    return;
  }
  /* Dump-only. Do not call hz10_public_entry_flush_thread_cache_quiescent()
   * or destroy pages here: pthread exit is not a proven quiescent boundary
   * for remote frees into this thread's pages. */
  hz10_shim_dump_exit_stats();
}

static void hz10_shim_init_thread_stats_key(void) {
  hz10_shim_thread_stats_key_ready =
      pthread_key_create(&hz10_shim_thread_stats_key,
                         hz10_shim_thread_stats_destructor) == 0;
}

__attribute__((noinline)) static void
hz10_shim_mark_thread_for_stats_slow(void) {
  if (hz10_shim_in_stats_dump) {
    return;
  }
  (void)pthread_once(&hz10_shim_thread_stats_once,
                     hz10_shim_init_thread_stats_key);
  if (hz10_shim_thread_stats_key_ready &&
      pthread_getspecific(hz10_shim_thread_stats_key) == NULL) {
    (void)pthread_setspecific(hz10_shim_thread_stats_key,
                              &hz10_shim_thread_stats_marker);
  }
}

static inline void hz10_shim_mark_thread_for_stats_fast(void) {
  if (__builtin_expect(hz10_shim_thread_exit_stats, 0)) {
    hz10_shim_mark_thread_for_stats_slow();
  }
}
#else
static inline void hz10_shim_mark_thread_for_stats_fast(void) {
}
#endif

__attribute__((constructor)) static void hz10_shim_init(void) {
  hz10_shim_process_exit_stats = hz10_shim_exit_stats_enabled();
#if HZ10_ENABLE_SHIM_THREAD_EXIT_STATS
  hz10_shim_thread_exit_stats = hz10_shim_thread_exit_stats_enabled();
#endif
  hz10_shim_exit_stats_classes = hz10_shim_exit_stats_classes_enabled();
  hz10_shim_census_sec = hz10_shim_census_seconds();
  (void)pthread_atfork(hz10_shim_atfork_prepare, hz10_shim_atfork_parent,
                       hz10_shim_atfork_child);
  if (hz10_shim_process_exit_stats) {
    (void)atexit(hz10_shim_dump_exit_stats);
  }
  if (hz10_shim_census_sec != 0u) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, hz10_shim_census_thread,
                       (void*)(uintptr_t)hz10_shim_census_sec) == 0) {
      (void)pthread_detach(thread);
    }
  }
}

static size_t hz10_shim_request_size(size_t size) {
  return size == 0u ? (size_t)HZ10_MIN_ALIGN : size;
}

static void hz10_shim_write_all(const char* text, size_t len) {
  while (len != 0u) {
    ssize_t n = write(STDERR_FILENO, text, len);
    if (n <= 0) {
      return;
    }
    text += (size_t)n;
    len -= (size_t)n;
  }
}

static void hz10_shim_abort_unknown(const char* api) {
  static const char prefix[] = "hz10 shim: unknown pointer in ";
  static const char suffix[] = "\n";
  hz10_shim_write_all(prefix, sizeof(prefix) - 1u);
  hz10_shim_write_all(api, strlen(api));
  hz10_shim_write_all(suffix, sizeof(suffix) - 1u);
  abort();
}

static int hz10_shim_tolerate_foreign(void) {
  const char* value = getenv("HZ10_SHIM_TOLERATE_FOREIGN");
  return value && value[0] == '1' && value[1] == '\0';
}

static size_t hz10_shim_usable_size_or_abort(void* ptr, const char* api) {
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  if (route.kind != H10_ROUTE_VALID) {
    hz10_shim_abort_unknown(api);
  }
  return (size_t)route.slot_size;
}

static void* hz10_shim_aligned_alloc_impl(size_t alignment, size_t size) {
  size_t request = hz10_shim_request_size(size);
  if (alignment <= (size_t)HZ10_MIN_ALIGN) {
    return hz10_malloc(request);
  }
  /* v0 alignment story: route alignments above the small-class minimum
   * through the large path, which is 64KiB quantum aligned. Requests above
   * that quantum are rejected until a dedicated aligned path exists. */
  if (alignment > (size_t)HZ10_PAGE_QUANTUM) {
    errno = ENOMEM;
    return NULL;
  }
  return hz10_large_alloc(request);
}

static void* hz10_shim_malloc_impl(size_t size) {
  hz10_shim_mark_thread_for_stats_fast();
  return hz10_malloc(hz10_shim_request_size(size));
}

static void hz10_shim_free_impl(void* ptr, const char* api) {
  hz10_shim_mark_thread_for_stats_fast();
  if (!ptr) {
    return;
  }
  if (!hz10_free(ptr)) {
    if (!hz10_shim_tolerate_foreign()) {
      hz10_shim_abort_unknown(api);
    }
    atomic_fetch_add_explicit(&hz10_shim_foreign_free_count, 1u,
                              memory_order_relaxed);
  }
}

uint64_t hz10_shim_tolerated_foreign_free_count(void) {
  return atomic_load_explicit(&hz10_shim_foreign_free_count,
                              memory_order_relaxed);
}

void* malloc(size_t size) {
  return hz10_shim_malloc_impl(size);
}

void free(void* ptr) {
  hz10_shim_free_impl(ptr, "free");
}

void* calloc(size_t count, size_t size) {
  if (count != 0u && size > SIZE_MAX / count) {
    errno = ENOMEM;
    return NULL;
  }
  size_t total = count * size;
  void* ptr = hz10_shim_malloc_impl(total);
  if (ptr) {
    memset(ptr, 0, hz10_shim_request_size(total));
  }
  return ptr;
}

void* realloc(void* ptr, size_t size) {
  if (!ptr) {
    return hz10_shim_malloc_impl(size);
  }
  if (size == 0u) {
    hz10_shim_free_impl(ptr, "realloc");
    return NULL;
  }

  size_t old_size = hz10_shim_usable_size_or_abort(ptr, "realloc");
  if (size <= old_size) {
    return ptr;
  }

  void* next = hz10_shim_malloc_impl(size);
  if (!next) {
    return NULL;
  }
  memcpy(next, ptr, old_size);
  hz10_shim_free_impl(ptr, "realloc");
  return next;
}

size_t malloc_usable_size(void* ptr) {
  if (!ptr) {
    return 0u;
  }
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  return route.kind == H10_ROUTE_VALID ? (size_t)route.slot_size : 0u;
}

int posix_memalign(void** out, size_t alignment, size_t size) {
  if (alignment < sizeof(void*) || !hz10_shim_is_power_of_two(alignment)) {
    return EINVAL;
  }
  void* ptr = hz10_shim_aligned_alloc_impl(alignment, size);
  if (!ptr) {
    return errno == ENOMEM ? ENOMEM : EINVAL;
  }
  *out = ptr;
  return 0;
}

void* aligned_alloc(size_t alignment, size_t size) {
  if (!hz10_shim_is_power_of_two(alignment) ||
      (alignment != 0u && (size % alignment) != 0u)) {
    errno = EINVAL;
    return NULL;
  }
  return hz10_shim_aligned_alloc_impl(alignment, size);
}

void* memalign(size_t alignment, size_t size) {
  if (!hz10_shim_is_power_of_two(alignment)) {
    errno = EINVAL;
    return NULL;
  }
  return hz10_shim_aligned_alloc_impl(alignment, size);
}
