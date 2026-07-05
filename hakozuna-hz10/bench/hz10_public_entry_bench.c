#include "../src/hz10_class_pages.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_public_entry.h"
#include "../src/hz10_size_class.h"
#include "hz10_public_entry_inbox.h"

#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

/*
 * The first HZ10 bench shaped like HZ8/HZ9/tcmalloc/mimalloc's medium_local0
 * /main_local0/medium_r50/main_r90 rows: a size-RANGE alloc/touch/free loop
 * across THREADS workers, with REMOTE_PCT of frees handed to a different
 * thread's inbox instead of freed locally -- same row-naming convention
 * this whole project's benches already use (e.g. MIN_SIZE=16 MAX_SIZE=32768
 * REMOTE_PCT=0 is a main_local0-style row; REMOTE_PCT=90 is main_r90-style).
 *
 * mech=hz10 uses this module; mech=system_malloc runs the identical loop
 * against plain libc malloc/free as the always-available same-run
 * reference (no external checkout needed, per Box 2's bench precedent).
 * A real same-run HZ8/HZ9/tcmalloc/mimalloc row is HZ10_EXT_ROOT territory
 * (see scripts/run_hz10_pagemap_vs_hz9_same_run.sh's pattern) -- not
 * wired up in this C binary, which only ever links hz10_* and libc.
 */

static void hz10_bench_free_hz10(void* ptr) {
  (void)hz10_free(ptr);
}

static void hz10_bench_free_system(void* ptr) {
  free(ptr);
}

static uint64_t hz10_bench_rng_next(uint64_t* state) {
  uint64_t x = *state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  *state = x;
  return x;
}

static size_t hz10_bench_choose_size(size_t min_size, size_t max_size,
                                    uint64_t* rng) {
  size_t span = max_size - min_size + 1u;
  return min_size + (size_t)(hz10_bench_rng_next(rng) % span);
}

static void hz10_bench_touch(void* p, size_t size) {
  unsigned char* c = (unsigned char*)p;
  c[0] = 0xA5u;
  c[size - 1u] = 0x5Au;
}

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

static int hz10_bench_env_is(const char* name, const char* expected) {
  const char* value = getenv(name);
  return value && strcmp(value, expected) == 0;
}

static long hz10_bench_maxrss_kb(void) {
  struct rusage usage;
  return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
}
static long hz10_bench_current_rss_kb(void) {
  FILE* fp = fopen("/proc/self/status", "r");
  if (!fp) return -1;
  char line[128];
  long rss = -1;
  while (fgets(line, sizeof(line), fp)) {
    if (sscanf(line, "VmRSS: %ld kB", &rss) == 1) break;
  }
  fclose(fp);
  return rss;
}

/*
 * Optional, opt-in (HZ10_DUMP_CLASS_STATS=1): aggregates Box 6's per-class
 * active/retired eviction/reclaim counters (src/hz10_class_pages.h, read
 * via hz10_public_entry_class_list_stats()) across every worker thread and
 * class, right before each thread's TLS state disappears at pthread_join.
 * Added specifically to check the main_r50/main_r90 RSS finding in
 * current_task.md against a real workload instead of only the isolated
 * unit tests: does this row's classes ever actually reach
 * HZ10_CLASS_PAGES_SCAN_LIMIT, and of the resulting `retired` traffic, how
 * much does hz10_class_pages_harvest_retired_capacity() destroy (fully
 * idle) vs. promote back to `active` (partial capacity) vs. leave
 * untouched? Off by default -- zero cost for every other bench run
 * through this file. */
static int hz10_bench_dump_class_stats;
static int hz10_bench_thread_exit_reclaim;
static _Atomic(uint64_t) hz10_bench_active_length_sum;
static _Atomic(uint64_t) hz10_bench_retired_length_sum;
static _Atomic(uint64_t) hz10_bench_max_retired_length;
static _Atomic(uint64_t) hz10_bench_eviction_count_sum;
static _Atomic(uint64_t) hz10_bench_eviction_reclaimed_sum;
static _Atomic(uint64_t) hz10_bench_retired_count_sum;
static _Atomic(uint64_t) hz10_bench_retired_reclaimed_sweep_sum;
static _Atomic(uint64_t) hz10_bench_retired_promoted_sweep_sum;
static _Atomic(uint64_t) hz10_bench_harvest_call_sum;
static _Atomic(uint64_t) hz10_bench_retired_reclaimed_ready_sum;
static _Atomic(uint64_t) hz10_bench_retired_promoted_ready_sum;
static _Atomic(uint64_t) hz10_bench_ready_false_positive_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_hit_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_alloc_fail_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_drain_call_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_drain_fail_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_nonempty_drain_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_slots_merged_sum;
static _Atomic(uint64_t) hz10_bench_active_cache_drain_hit_sum;
static _Atomic(uint64_t) hz10_bench_find_call_sum;
static _Atomic(uint64_t) hz10_bench_find_miss_sum;
static _Atomic(uint64_t) hz10_bench_find_pages_visited_sum;
static _Atomic(uint64_t) hz10_bench_find_drain_call_sum;
static _Atomic(uint64_t) hz10_bench_find_nonempty_drain_sum;
static _Atomic(uint64_t) hz10_bench_find_slots_merged_sum;
static _Atomic(uint64_t) hz10_bench_find_local_hit_sum;
static _Atomic(uint64_t) hz10_bench_find_drain_hit_sum;
static _Atomic(uint64_t) hz10_bench_find_hit_depth_sum;
static _Atomic(uint64_t) hz10_bench_find_miss_pages_visited_sum;
static _Atomic(uint64_t) hz10_bench_find_hit_depth_max;
static _Atomic(uint64_t)
    hz10_bench_find_depth_hist[HZ10_CLASS_PAGES_SCAN_DEPTH_HIST_BUCKETS];
static _Atomic(uint64_t) hz10_bench_active_switch_sum;
static _Atomic(uint64_t) hz10_bench_active_ops_served_sum;
static _Atomic(uint64_t) hz10_bench_active_ops_served_immediate_sum;
static _Atomic(uint64_t) hz10_bench_second_active_check_sum;
static _Atomic(uint64_t) hz10_bench_second_active_hit_sum;
static _Atomic(uint64_t) hz10_bench_thread_reclaim_pages_seen,
    hz10_bench_thread_reclaim_pages_reclaimed, hz10_bench_thread_reclaim_pages_busy,
    hz10_bench_thread_reclaim_pages_deferred_ready,
    hz10_bench_thread_reclaim_pages_deferred_cancel,
    hz10_bench_thread_reclaim_slots_merged, hz10_bench_thread_reclaim_total_ns,
    hz10_bench_thread_reclaim_max_ns;

#define HZ10_BENCH_LOAD(counter) \
  ((unsigned long long)atomic_load_explicit(&(counter), memory_order_relaxed))
#define HZ10_BENCH_ADD(counter, value) \
  atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
#define HZ10_BENCH_ZERO(counter) \
  atomic_store_explicit(&(counter), 0u, memory_order_relaxed)

typedef struct Hz10BenchClassStats {
  _Atomic(uint64_t) active_cache_alloc_fail_count;
  _Atomic(uint64_t) active_cache_drain_fail_count;
  _Atomic(uint64_t) find_call_count;
  _Atomic(uint64_t) find_miss_count;
  _Atomic(uint64_t) find_pages_visited_count;
  _Atomic(uint64_t) find_miss_pages_visited_count;
  _Atomic(uint64_t) find_drain_call_count;
  _Atomic(uint64_t) find_nonempty_drain_count;
  _Atomic(uint64_t) find_hit_depth_sum;
  _Atomic(uint64_t) find_hit_depth_max;
  _Atomic(uint64_t) active_switch_count;
  _Atomic(uint64_t) active_ops_served_sum;
  _Atomic(uint64_t) active_ops_served_immediate_count;
  _Atomic(uint64_t) second_active_check_count;
  _Atomic(uint64_t) second_active_hit_count;
} Hz10BenchClassStats;

static Hz10BenchClassStats hz10_bench_class_stats[HZ10_CLASS_COUNT];

static void hz10_bench_atomic_max_u64(_Atomic(uint64_t)* target,
                                    uint64_t candidate) {
  uint64_t current = atomic_load_explicit(target, memory_order_relaxed);
  while (candidate > current &&
        !atomic_compare_exchange_weak_explicit(target, &current, candidate,
                                               memory_order_relaxed,
                                               memory_order_relaxed)) {
  }
}

static void hz10_bench_collect_class_stats(void) {
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassPageListStats stats;
    hz10_public_entry_class_list_stats(c, &stats);
    atomic_fetch_add_explicit(&hz10_bench_active_length_sum,
                              stats.active_length, memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_retired_length_sum,
                              stats.retired_length, memory_order_relaxed);
    hz10_bench_atomic_max_u64(&hz10_bench_max_retired_length,
                             stats.max_retired_length);
    atomic_fetch_add_explicit(&hz10_bench_eviction_count_sum,
                              stats.eviction_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_eviction_reclaimed_sum,
                              stats.eviction_reclaimed_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_retired_count_sum,
                              stats.retired_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_retired_reclaimed_sweep_sum,
                              stats.retired_reclaimed_by_sweep_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_retired_promoted_sweep_sum,
                              stats.retired_promoted_by_sweep_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_harvest_call_sum,
                              stats.harvest_call_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_retired_reclaimed_ready_sum,
                              stats.retired_reclaimed_by_ready_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_retired_promoted_ready_sum,
                              stats.retired_promoted_by_ready_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_ready_false_positive_sum,
                              stats.ready_false_positive_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_hit_sum,
                              stats.active_cache_hit_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_alloc_fail_sum,
                              stats.active_cache_alloc_fail_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_drain_call_sum,
                              stats.active_cache_drain_call_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_drain_fail_sum,
                              stats.active_cache_drain_fail_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_nonempty_drain_sum,
                              stats.active_cache_nonempty_drain_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_slots_merged_sum,
                              stats.active_cache_slots_merged_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_cache_drain_hit_sum,
                              stats.active_cache_drain_hit_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_call_sum,
                              stats.find_call_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_miss_sum,
                              stats.find_miss_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_pages_visited_sum,
                              stats.find_pages_visited_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_drain_call_sum,
                              stats.find_drain_call_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_nonempty_drain_sum,
                              stats.find_nonempty_drain_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_slots_merged_sum,
                              stats.find_slots_merged_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_local_hit_sum,
                              stats.find_local_hit_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_drain_hit_sum,
                              stats.find_drain_hit_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_hit_depth_sum,
                              stats.find_hit_depth_sum,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_find_miss_pages_visited_sum,
                              stats.find_miss_pages_visited_count,
                              memory_order_relaxed);
    hz10_bench_atomic_max_u64(&hz10_bench_find_hit_depth_max,
                             stats.find_hit_depth_max);
    for (uint32_t i = 0; i < HZ10_CLASS_PAGES_SCAN_DEPTH_HIST_BUCKETS; ++i) {
      atomic_fetch_add_explicit(&hz10_bench_find_depth_hist[i],
                                stats.find_depth_hist[i],
                                memory_order_relaxed);
    }
    Hz10BenchClassStats* class_stats = &hz10_bench_class_stats[c];
    atomic_fetch_add_explicit(&class_stats->active_cache_alloc_fail_count,
                              stats.active_cache_alloc_fail_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->active_cache_drain_fail_count,
                              stats.active_cache_drain_fail_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_call_count,
                              stats.find_call_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_miss_count,
                              stats.find_miss_count, memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_pages_visited_count,
                              stats.find_pages_visited_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_miss_pages_visited_count,
                              stats.find_miss_pages_visited_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_drain_call_count,
                              stats.find_drain_call_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_nonempty_drain_count,
                              stats.find_nonempty_drain_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->find_hit_depth_sum,
                              stats.find_hit_depth_sum,
                              memory_order_relaxed);
    hz10_bench_atomic_max_u64(&class_stats->find_hit_depth_max,
                             stats.find_hit_depth_max);
    atomic_fetch_add_explicit(&hz10_bench_active_switch_sum,
                              stats.active_switch_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_ops_served_sum,
                              stats.active_ops_served_sum,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_active_ops_served_immediate_sum,
                              stats.active_ops_served_immediate_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_second_active_check_sum,
                              stats.second_active_check_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_second_active_hit_sum,
                              stats.second_active_hit_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->active_switch_count,
                              stats.active_switch_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->active_ops_served_sum,
                              stats.active_ops_served_sum,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->active_ops_served_immediate_count,
                              stats.active_ops_served_immediate_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->second_active_check_count,
                              stats.second_active_check_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&class_stats->second_active_hit_count,
                              stats.second_active_hit_count,
                              memory_order_relaxed);
  }
}

static void hz10_bench_collect_thread_reclaim_stats(
    const Hz10PublicEntryThreadReclaimStats* stats, uint64_t elapsed_ns) {
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_pages_seen, stats->pages_seen);
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_pages_reclaimed,
                 stats->pages_reclaimed);
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_pages_busy, stats->pages_busy);
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_pages_deferred_ready,
                 stats->pages_deferred_ready);
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_pages_deferred_cancel,
                 stats->pages_deferred_cancel);
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_slots_merged, stats->slots_merged);
  HZ10_BENCH_ADD(hz10_bench_thread_reclaim_total_ns, elapsed_ns);
  hz10_bench_atomic_max_u64(&hz10_bench_thread_reclaim_max_ns, elapsed_ns);
}

static void hz10_bench_reset_class_stats(void) {
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10BenchClassStats* stats = &hz10_bench_class_stats[c];
    atomic_store_explicit(&stats->active_cache_alloc_fail_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->active_cache_drain_fail_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->find_call_count, 0u, memory_order_relaxed);
    atomic_store_explicit(&stats->find_miss_count, 0u, memory_order_relaxed);
    atomic_store_explicit(&stats->find_pages_visited_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->find_miss_pages_visited_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->find_drain_call_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->find_nonempty_drain_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->find_hit_depth_sum, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->find_hit_depth_max, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->active_switch_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->active_ops_served_sum, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->active_ops_served_immediate_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->second_active_check_count, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&stats->second_active_hit_count, 0u,
                          memory_order_relaxed);
  }
}

static void hz10_bench_print_top_class_stats(const char* mech, uint32_t run,
                                            uint32_t runs) {
  enum { TOP_N = 5 };
  uint32_t top_class[TOP_N] = {0};
  uint64_t top_pages[TOP_N] = {0};

  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    uint64_t pages = atomic_load_explicit(
        &hz10_bench_class_stats[c].find_pages_visited_count,
        memory_order_relaxed);
    for (uint32_t i = 0; i < TOP_N; ++i) {
      if (pages <= top_pages[i]) {
        continue;
      }
      for (uint32_t j = TOP_N - 1u; j > i; --j) {
        top_pages[j] = top_pages[j - 1u];
        top_class[j] = top_class[j - 1u];
      }
      top_pages[i] = pages;
      top_class[i] = c;
      break;
    }
  }

  for (uint32_t i = 0; i < TOP_N; ++i) {
    if (top_pages[i] == 0u) {
      continue;
    }
    uint32_t c = top_class[i];
    Hz10BenchClassStats* stats = &hz10_bench_class_stats[c];
    uint64_t find_calls = atomic_load_explicit(
        &stats->find_call_count, memory_order_relaxed);
    uint64_t find_misses = atomic_load_explicit(
        &stats->find_miss_count, memory_order_relaxed);
    uint64_t hit_depth_sum = atomic_load_explicit(
        &stats->find_hit_depth_sum, memory_order_relaxed);
    uint64_t hit_count = find_calls >= find_misses
                             ? find_calls - find_misses
                             : 0u;
    uint64_t active_switch_count = atomic_load_explicit(
        &stats->active_switch_count, memory_order_relaxed);
    uint64_t active_ops_served_sum = atomic_load_explicit(
        &stats->active_ops_served_sum, memory_order_relaxed);
    uint64_t active_ops_served_immediate_count = atomic_load_explicit(
        &stats->active_ops_served_immediate_count, memory_order_relaxed);
    uint64_t second_active_check_count = atomic_load_explicit(
        &stats->second_active_check_count, memory_order_relaxed);
    uint64_t second_active_hit_count = atomic_load_explicit(
        &stats->second_active_hit_count, memory_order_relaxed);
    printf(
        "hz10_public_entry_class_top mech=%s run=%u/%u rank=%u "
        "class_id=%u slot_size=%u slot_count=%u "
        "active_alloc_fails=%llu active_drain_fails=%llu "
        "find_calls=%llu find_misses=%llu find_pages_visited=%llu "
        "find_miss_pages_visited=%llu find_drain_calls=%llu "
        "find_nonempty_drains=%llu find_hit_depth_sum=%llu "
        "find_hit_depth_max=%llu find_hit_count=%llu "
        "active_switch_count=%llu active_ops_served_sum=%llu "
        "active_ops_served_immediate_count=%llu "
        "second_active_check_count=%llu second_active_hit_count=%llu\n",
        mech, run, runs, i + 1u, c, hz10_size_class_slot_size(c),
        hz10_size_class_slot_count(c),
        (unsigned long long)atomic_load_explicit(
            &stats->active_cache_alloc_fail_count, memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(
            &stats->active_cache_drain_fail_count, memory_order_relaxed),
        (unsigned long long)find_calls, (unsigned long long)find_misses,
        (unsigned long long)top_pages[i],
        (unsigned long long)atomic_load_explicit(
            &stats->find_miss_pages_visited_count, memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(
            &stats->find_drain_call_count, memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(
            &stats->find_nonempty_drain_count, memory_order_relaxed),
        (unsigned long long)hit_depth_sum,
        (unsigned long long)atomic_load_explicit(
            &stats->find_hit_depth_max, memory_order_relaxed),
        (unsigned long long)hit_count,
        (unsigned long long)active_switch_count,
        (unsigned long long)active_ops_served_sum,
        (unsigned long long)active_ops_served_immediate_count,
        (unsigned long long)second_active_check_count,
        (unsigned long long)second_active_hit_count);
  }
}

typedef struct WorkerArg {
  uint32_t tid;
  uint32_t threads;
  size_t min_size;
  size_t max_size;
  uint64_t iters;
  uint32_t remote_pct;
  int use_hz10; /* 1 = hz10_malloc/hz10_free, 0 = libc malloc/free */
  Hz10BenchInbox* inboxes;
  pthread_barrier_t* barrier;
  int failed;
} WorkerArg;

static void* hz10_bench_worker(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  uint64_t rng = 0x9e3779b97f4a7c15ull ^ (uint64_t)(arg->tid + 1u);
  void (*free_fn)(void*) =
      arg->use_hz10 ? hz10_bench_free_hz10 : hz10_bench_free_system;

  for (uint64_t i = 0; i < arg->iters; ++i) {
    if ((i & 31u) == 0u) {
      hz10_bench_inbox_drain(&arg->inboxes[arg->tid], free_fn);
    }
    size_t size = hz10_bench_choose_size(arg->min_size, arg->max_size, &rng);
    void* p = arg->use_hz10 ? hz10_malloc(size) : malloc(size);
    if (!p) {
      fprintf(stderr, "worker %u: alloc failed at iter %llu (size=%zu)\n",
              arg->tid, (unsigned long long)i, size);
      arg->failed = 1;
      return NULL;
    }
    hz10_bench_touch(p, size);

    int remote = arg->threads > 1u &&
                 (uint32_t)(hz10_bench_rng_next(&rng) % 100u) <
                     arg->remote_pct;
    if (remote) {
      uint32_t dst = (arg->tid + 1u +
                     (uint32_t)(hz10_bench_rng_next(&rng) %
                                (uint32_t)(arg->threads - 1u))) %
                    arg->threads;
      hz10_bench_inbox_push(&arg->inboxes[dst], p);
    } else {
      free_fn(p);
    }
  }

  pthread_barrier_wait(arg->barrier);
  hz10_bench_inbox_drain(&arg->inboxes[arg->tid], free_fn);
  pthread_barrier_wait(arg->barrier);
  hz10_bench_inbox_drain(&arg->inboxes[arg->tid], free_fn);

  /* Must read this thread's class-list stats here, before returning --
   * its TLS state (src/hz10_public_entry.c's hz10_class_state) is gone
   * once this thread has exited and the caller has joined it. */
  if (arg->use_hz10 && hz10_bench_dump_class_stats) {
    hz10_bench_collect_class_stats();
  }
  if (arg->use_hz10 && hz10_bench_thread_exit_reclaim) {
    Hz10PublicEntryThreadReclaimStats reclaim_stats;
    uint64_t reclaim_start = hz10_platform_now_ns();
    hz10_public_entry_flush_thread_cache_quiescent(&reclaim_stats);
    hz10_bench_collect_thread_reclaim_stats(
        &reclaim_stats, hz10_platform_now_ns() - reclaim_start);
  }
  return NULL;
}

static int hz10_bench_run(const char* mech, int use_hz10, uint32_t threads,
                         uint64_t iters, size_t min_size, size_t max_size,
                         uint32_t remote_pct, uint32_t run, uint32_t runs) {
  pthread_t* thread_ids = calloc(threads, sizeof(*thread_ids));
  WorkerArg* args = calloc(threads, sizeof(*args));
  Hz10BenchInbox* inboxes = calloc(threads, sizeof(*inboxes));
  pthread_barrier_t barrier;
  if (!thread_ids || !args || !inboxes) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }
  pthread_barrier_init(&barrier, NULL, threads);
  for (uint32_t t = 0; t < threads; ++t) {
    /* capacity: worst case every alloc from every other thread lands here */
    hz10_bench_inbox_init(&inboxes[t], iters * threads + 1u);
  }

  if (use_hz10 && hz10_bench_dump_class_stats) {
    HZ10_BENCH_ZERO(hz10_bench_active_length_sum); HZ10_BENCH_ZERO(hz10_bench_retired_length_sum); HZ10_BENCH_ZERO(hz10_bench_max_retired_length);
    HZ10_BENCH_ZERO(hz10_bench_eviction_count_sum); HZ10_BENCH_ZERO(hz10_bench_eviction_reclaimed_sum); HZ10_BENCH_ZERO(hz10_bench_retired_count_sum);
    HZ10_BENCH_ZERO(hz10_bench_retired_reclaimed_sweep_sum); HZ10_BENCH_ZERO(hz10_bench_retired_promoted_sweep_sum); HZ10_BENCH_ZERO(hz10_bench_harvest_call_sum);
    HZ10_BENCH_ZERO(hz10_bench_retired_reclaimed_ready_sum); HZ10_BENCH_ZERO(hz10_bench_retired_promoted_ready_sum); HZ10_BENCH_ZERO(hz10_bench_ready_false_positive_sum);
    HZ10_BENCH_ZERO(hz10_bench_active_cache_hit_sum); HZ10_BENCH_ZERO(hz10_bench_active_cache_alloc_fail_sum); HZ10_BENCH_ZERO(hz10_bench_active_cache_drain_call_sum);
    HZ10_BENCH_ZERO(hz10_bench_active_cache_drain_fail_sum); HZ10_BENCH_ZERO(hz10_bench_active_cache_nonempty_drain_sum); HZ10_BENCH_ZERO(hz10_bench_active_cache_slots_merged_sum);
    HZ10_BENCH_ZERO(hz10_bench_active_cache_drain_hit_sum); HZ10_BENCH_ZERO(hz10_bench_find_call_sum); HZ10_BENCH_ZERO(hz10_bench_find_miss_sum);
    HZ10_BENCH_ZERO(hz10_bench_find_pages_visited_sum); HZ10_BENCH_ZERO(hz10_bench_find_drain_call_sum); HZ10_BENCH_ZERO(hz10_bench_find_nonempty_drain_sum);
    HZ10_BENCH_ZERO(hz10_bench_find_slots_merged_sum); HZ10_BENCH_ZERO(hz10_bench_find_local_hit_sum); HZ10_BENCH_ZERO(hz10_bench_find_drain_hit_sum);
    HZ10_BENCH_ZERO(hz10_bench_find_hit_depth_sum); HZ10_BENCH_ZERO(hz10_bench_find_miss_pages_visited_sum); HZ10_BENCH_ZERO(hz10_bench_find_hit_depth_max);
    for (uint32_t i = 0; i < HZ10_CLASS_PAGES_SCAN_DEPTH_HIST_BUCKETS; ++i) {
      atomic_store_explicit(&hz10_bench_find_depth_hist[i], 0u,
                            memory_order_relaxed);
    }
    atomic_store_explicit(&hz10_bench_active_switch_sum, 0u, memory_order_relaxed);
    atomic_store_explicit(&hz10_bench_active_ops_served_sum, 0u, memory_order_relaxed);
    atomic_store_explicit(&hz10_bench_active_ops_served_immediate_sum, 0u, memory_order_relaxed);
    atomic_store_explicit(&hz10_bench_second_active_check_sum, 0u, memory_order_relaxed);
    atomic_store_explicit(&hz10_bench_second_active_hit_sum, 0u, memory_order_relaxed);
    hz10_bench_reset_class_stats();
  }
  if (use_hz10 && hz10_bench_thread_exit_reclaim) {
    HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_pages_seen); HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_pages_reclaimed);
    HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_pages_busy); HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_pages_deferred_ready);
    HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_pages_deferred_cancel); HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_slots_merged);
    HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_total_ns); HZ10_BENCH_ZERO(hz10_bench_thread_reclaim_max_ns);
  }
  uint64_t start = hz10_platform_now_ns();
  for (uint32_t t = 0; t < threads; ++t) {
    args[t].tid = t;
    args[t].threads = threads;
    args[t].min_size = min_size;
    args[t].max_size = max_size;
    args[t].iters = iters;
    args[t].remote_pct = remote_pct;
    args[t].use_hz10 = use_hz10;
    args[t].inboxes = inboxes;
    args[t].barrier = &barrier;
    args[t].failed = 0;
    if (pthread_create(&thread_ids[t], NULL, hz10_bench_worker, &args[t]) !=
        0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }
  int failed = 0;
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(thread_ids[t], NULL);
    failed |= args[t].failed;
  }
  uint64_t elapsed_ns = hz10_platform_now_ns() - start;

  for (uint32_t t = 0; t < threads; ++t) {
    hz10_bench_inbox_destroy(&inboxes[t]);
  }
  free(inboxes);
  free(args);
  free(thread_ids);
  pthread_barrier_destroy(&barrier);

  if (failed) {
    return 1;
  }
  uint64_t flush_max_ns = use_hz10 ? HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_max_ns) : 0u;
  uint64_t work_elapsed_ns = elapsed_ns > flush_max_ns ? elapsed_ns - flush_max_ns : elapsed_ns;
  double seconds = (double)elapsed_ns / 1000000000.0, work_seconds = (double)work_elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) { seconds = 1e-9; }
  if (work_seconds <= 0.0) { work_seconds = 1e-9; }
  uint64_t ops = iters * (uint64_t)threads; printf(
      "hz10_public_entry mech=%s threads=%u iters_per_thread=%llu ops=%llu "
      "run=%u/%u inbox=%s min_size=%zu max_size=%zu remote_pct=%u "
      "seconds=%.6f "
      "ops_per_s=%.2f work_loop_seconds=%.6f work_loop_ops_per_s=%.2f "
      "work_loop_plus_flush_seconds=%.6f work_loop_plus_flush_ops_per_s=%.2f "
      "post_rss_kb=%ld current_rss_kb=%ld\n",
      mech, threads, (unsigned long long)iters, (unsigned long long)ops, run,
      runs, hz10_bench_use_spin_inbox ? "spin" : "mutex", min_size, max_size,
      remote_pct, seconds, (double)ops / seconds,
      work_seconds, (double)ops / work_seconds, seconds,
      (double)ops / seconds, hz10_bench_maxrss_kb(), hz10_bench_current_rss_kb());

  if (use_hz10 && hz10_bench_dump_class_stats) {
    printf(
        "hz10_public_entry_class_stats mech=%s run=%u/%u "
        "sweep_budget=%u "
        "active_length_sum=%llu retired_length_sum=%llu "
        "max_retired_length=%llu eviction_count=%llu "
        "eviction_reclaimed_count=%llu retired_count=%llu "
        "retired_reclaimed_by_sweep=%llu retired_promoted_by_sweep=%llu "
        "harvest_call_count=%llu retired_reclaimed_by_ready=%llu "
        "retired_promoted_by_ready=%llu ready_false_positive=%llu "
        "active_cache_hit=%llu active_cache_alloc_fails=%llu "
        "active_cache_drain_calls=%llu active_cache_drain_fails=%llu "
        "active_cache_nonempty_drains=%llu active_cache_slots_merged=%llu "
        "active_cache_drain_hits=%llu find_calls=%llu find_misses=%llu "
        "find_pages_visited=%llu find_drain_calls=%llu "
        "find_nonempty_drains=%llu find_slots_merged=%llu "
        "find_local_hits=%llu find_drain_hits=%llu "
        "find_hit_depth_sum=%llu find_miss_pages_visited=%llu "
        "find_hit_depth_max=%llu "
        "find_depth_h0=%llu find_depth_h1=%llu find_depth_h2_4=%llu "
        "find_depth_h5_16=%llu find_depth_h17_64=%llu "
        "find_depth_h65_128=%llu\n",
        mech, run, runs, HZ10_CLASS_PAGES_SWEEP_BUDGET,
        HZ10_BENCH_LOAD(hz10_bench_active_length_sum),
        HZ10_BENCH_LOAD(hz10_bench_retired_length_sum),
        HZ10_BENCH_LOAD(hz10_bench_max_retired_length),
        HZ10_BENCH_LOAD(hz10_bench_eviction_count_sum),
        HZ10_BENCH_LOAD(hz10_bench_eviction_reclaimed_sum),
        HZ10_BENCH_LOAD(hz10_bench_retired_count_sum),
        HZ10_BENCH_LOAD(hz10_bench_retired_reclaimed_sweep_sum),
        HZ10_BENCH_LOAD(hz10_bench_retired_promoted_sweep_sum),
        HZ10_BENCH_LOAD(hz10_bench_harvest_call_sum),
        HZ10_BENCH_LOAD(hz10_bench_retired_reclaimed_ready_sum),
        HZ10_BENCH_LOAD(hz10_bench_retired_promoted_ready_sum),
        HZ10_BENCH_LOAD(hz10_bench_ready_false_positive_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_hit_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_alloc_fail_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_drain_call_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_drain_fail_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_nonempty_drain_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_slots_merged_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_cache_drain_hit_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_call_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_miss_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_pages_visited_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_drain_call_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_nonempty_drain_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_slots_merged_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_local_hit_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_drain_hit_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_hit_depth_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_miss_pages_visited_sum),
        HZ10_BENCH_LOAD(hz10_bench_find_hit_depth_max),
        HZ10_BENCH_LOAD(hz10_bench_find_depth_hist[0]),
        HZ10_BENCH_LOAD(hz10_bench_find_depth_hist[1]),
        HZ10_BENCH_LOAD(hz10_bench_find_depth_hist[2]),
        HZ10_BENCH_LOAD(hz10_bench_find_depth_hist[3]),
        HZ10_BENCH_LOAD(hz10_bench_find_depth_hist[4]),
        HZ10_BENCH_LOAD(hz10_bench_find_depth_hist[5]));
    printf(
        "hz10_public_entry_two_slot_stats mech=%s run=%u/%u "
        "active_switch_count=%llu active_ops_served_sum=%llu "
        "active_ops_served_immediate_count=%llu "
        "second_active_check_count=%llu second_active_hit_count=%llu\n",
        mech, run, runs,
        HZ10_BENCH_LOAD(hz10_bench_active_switch_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_ops_served_sum),
        HZ10_BENCH_LOAD(hz10_bench_active_ops_served_immediate_sum),
        HZ10_BENCH_LOAD(hz10_bench_second_active_check_sum),
        HZ10_BENCH_LOAD(hz10_bench_second_active_hit_sum));
    if (hz10_bench_thread_exit_reclaim) {
      printf(
          "hz10_public_entry_thread_reclaim mech=%s run=%u/%u "
          "pages_seen=%llu pages_reclaimed=%llu pages_busy=%llu "
          "pages_deferred_ready=%llu pages_deferred_cancel=%llu "
          "slots_merged=%llu flush_total_ns=%llu flush_max_thread_ns=%llu\n",
          mech, run, runs,
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_pages_seen),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_pages_reclaimed),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_pages_busy),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_pages_deferred_ready),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_pages_deferred_cancel),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_slots_merged),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_total_ns),
          HZ10_BENCH_LOAD(hz10_bench_thread_reclaim_max_ns));
    }
    hz10_bench_print_top_class_stats(mech, run, runs);
  }
  return 0;
}

int main(void) {
  uint32_t threads = (uint32_t)hz10_bench_env_u64("THREADS", 4ull);
  uint64_t iters = hz10_bench_env_u64("ITERS", 50000ull);
  size_t min_size = (size_t)hz10_bench_env_u64("MIN_SIZE", 16ull);
  size_t max_size = (size_t)hz10_bench_env_u64("MAX_SIZE", 32768ull);
  uint32_t remote_pct = (uint32_t)hz10_bench_env_u64("REMOTE_PCT", 0ull);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 1ull);
  uint32_t mode = (uint32_t)hz10_bench_env_u64("MODE", 2ull); /* 2 = both */
  hz10_bench_dump_class_stats =
      hz10_bench_env_u64("HZ10_DUMP_CLASS_STATS", 0ull) != 0ull;
  hz10_bench_thread_exit_reclaim =
      hz10_bench_env_u64("HZ10_THREAD_EXIT_RECLAIM", 0ull) != 0ull;
  hz10_bench_use_spin_inbox = hz10_bench_env_is("HZ10_BENCH_INBOX", "spin");
  hz10_bench_spin_inbox_capacity =
      (size_t)hz10_bench_env_u64("HZ10_BENCH_INBOX_CAP", 0ull);
  if (hz10_bench_spin_inbox_capacity != 0u &&
      hz10_bench_spin_inbox_capacity < 1024u) {
    hz10_bench_spin_inbox_capacity = 1024u;
  }

  if (threads == 0u || min_size == 0u || max_size < min_size) {
    fprintf(stderr, "invalid THREADS/MIN_SIZE/MAX_SIZE\n");
    return 1;
  }

  int failed = 0;
  for (uint32_t run = 1; run <= runs && !failed; ++run) {
    if (mode == 0u || mode == 2u) {
      hz10_pagemap_reset_for_tests();
      failed |= hz10_bench_run("hz10", 1, threads, iters, min_size, max_size,
                              remote_pct, run, runs);
    }
    if (!failed && (mode == 1u || mode == 2u)) {
      failed |= hz10_bench_run("system_malloc", 0, threads, iters, min_size,
                              max_size, remote_pct, run, runs);
    }
  }
  return failed ? 1 : 0;
}
