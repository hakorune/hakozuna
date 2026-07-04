#include "../src/hz10_class_pages.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_public_entry.h"
#include "../src/hz10_size_class.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

/*
 * Separate from bench/hz10_public_entry_bench.c on purpose, never named
 * main_rNN -- this bench isolates exactly ONE variable the locked-in
 * ITERS=500000 RUNS=10 table cannot isolate: whether that table's
 * "post_rss_kb climbs monotonically run 1->10" finding reflects genuine
 * steady-state growth, or is an artifact of that bench spawning a BRAND
 * NEW set of THREADS worker threads every run and joining (destroying)
 * the previous set -- each of which abandons its entire active+retired
 * per-class page set on exit, since HZ10 has no thread-lifecycle/exit
 * hook (see tests/README.md's "abandoned/TLS-resident pages" note, and
 * current_task.md's "HZ10RetiredReadyQueue-L0 wired into the real path"
 * entry for the measurement that first raised this as the likely real
 * explanation).
 *
 * Design: same workload shape as the locked-in bench (size-range alloc/
 * touch/free loop, REMOTE_PCT share handed to another thread's inbox),
 * same RUNS x ITERS volume -- but THREADS worker threads are created
 * ONCE and never exit until the entire RUNS x ITERS volume is done. Each
 * worker self-reports (into shared atomics) at the end of every ITERS-
 * sized segment, exactly matching the locked-in bench's per-run
 * reporting cadence, so the two tables' post_rss_kb columns are directly
 * comparable row-for-row -- the ONLY structural difference is whether
 * the thread population is fresh or reused each segment. A barrier
 * (sized threads+1, main included) marks each segment boundary purely
 * to keep the printed per-segment stats clean (so a fast worker cannot
 * blend its next segment's counters into the one main is about to
 * print/reset) -- it does NOT gate the actual work loop itself, which
 * runs continuously exactly like a real long-running process would;
 * Important diagnostic caveat: the barrier is NOT allocator-neutral for
 * remote-heavy rows. A fast worker waits at the segment boundary while
 * slower workers can still push remote frees into its inbox; those frees
 * cannot be drained until the next segment starts. The printed
 * boundary_inbox_count/boundary_inbox_max fields expose that backlog.
 * If they are non-zero, this is a boundary-stress workload, not a true
 * continuous steady-state workload.
 */

typedef struct Hz10BenchInbox {
  hz10_platform_mutex_t lock;
  void** items;
  size_t count;
  size_t capacity;
} Hz10BenchInbox;

static void hz10_bench_inbox_init(Hz10BenchInbox* box, size_t capacity) {
  hz10_platform_mutex_init(&box->lock);
  box->items = calloc(capacity, sizeof(*box->items));
  box->count = 0;
  box->capacity = capacity;
}

static void hz10_bench_inbox_destroy(Hz10BenchInbox* box) {
  hz10_platform_mutex_destroy(&box->lock);
  free(box->items);
}

static void hz10_bench_inbox_push(Hz10BenchInbox* box, void* ptr) {
  hz10_platform_mutex_lock(&box->lock);
  if (box->count < box->capacity) {
    box->items[box->count++] = ptr;
  }
  hz10_platform_mutex_unlock(&box->lock);
}

static void hz10_bench_inbox_drain(Hz10BenchInbox* box,
                                  void (*free_fn)(void*)) {
  hz10_platform_mutex_lock(&box->lock);
  for (size_t i = 0; i < box->count; ++i) {
    free_fn(box->items[i]);
  }
  box->count = 0;
  hz10_platform_mutex_unlock(&box->lock);
}

static size_t hz10_bench_inbox_count(const Hz10BenchInbox* box) {
  Hz10BenchInbox* mutable_box = (Hz10BenchInbox*)box;
  hz10_platform_mutex_lock(&mutable_box->lock);
  size_t count = mutable_box->count;
  hz10_platform_mutex_unlock(&mutable_box->lock);
  return count;
}

static void hz10_bench_free_hz10(void* ptr) {
  (void)hz10_free(ptr);
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

/* Shared, reset each segment. Mirrors bench/hz10_public_entry_bench.c's
 * per-run aggregation exactly, so the two reports read the same way. */
static _Atomic(uint64_t) g_active_length_sum;
static _Atomic(uint64_t) g_retired_length_sum;
static _Atomic(uint64_t) g_eviction_count_sum;
static _Atomic(uint64_t) g_retired_count_sum;
static _Atomic(uint64_t) g_harvest_call_sum;
static _Atomic(uint64_t) g_reclaimed_by_sweep_sum;
static _Atomic(uint64_t) g_promoted_by_sweep_sum;
static _Atomic(uint64_t) g_reclaimed_by_ready_sum;
static _Atomic(uint64_t) g_promoted_by_ready_sum;

static void collect_segment_stats(void) {
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassPageListStats stats;
    hz10_public_entry_class_list_stats(c, &stats);
    atomic_fetch_add_explicit(&g_active_length_sum, stats.active_length,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_retired_length_sum, stats.retired_length,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_eviction_count_sum, stats.eviction_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_retired_count_sum, stats.retired_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_harvest_call_sum, stats.harvest_call_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_reclaimed_by_sweep_sum,
                              stats.retired_reclaimed_by_sweep_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_promoted_by_sweep_sum,
                              stats.retired_promoted_by_sweep_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_reclaimed_by_ready_sum,
                              stats.retired_reclaimed_by_ready_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_promoted_by_ready_sum,
                              stats.retired_promoted_by_ready_count,
                              memory_order_relaxed);
  }
}

static void reset_segment_stats(void) {
  atomic_store_explicit(&g_active_length_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_retired_length_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_eviction_count_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_retired_count_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_harvest_call_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_reclaimed_by_sweep_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_promoted_by_sweep_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_reclaimed_by_ready_sum, 0u, memory_order_relaxed);
  atomic_store_explicit(&g_promoted_by_ready_sum, 0u, memory_order_relaxed);
}

typedef struct WorkerArg {
  uint32_t tid;
  uint32_t threads;
  size_t min_size;
  size_t max_size;
  uint64_t iters_per_segment;
  uint32_t segments;
  uint32_t remote_pct;
  Hz10BenchInbox* inboxes;
  pthread_barrier_t* barrier;
  int failed;
} WorkerArg;

static void* worker(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  uint64_t rng = 0x9e3779b97f4a7c15ull ^ (uint64_t)(arg->tid + 1u);

  for (uint32_t seg = 0; seg < arg->segments; ++seg) {
    for (uint64_t i = 0; i < arg->iters_per_segment; ++i) {
      if ((i & 31u) == 0u) {
        hz10_bench_inbox_drain(&arg->inboxes[arg->tid], hz10_bench_free_hz10);
      }
      size_t size =
          hz10_bench_choose_size(arg->min_size, arg->max_size, &rng);
      void* p = hz10_malloc(size);
      if (!p) {
        fprintf(stderr, "worker %u: alloc failed (size=%zu)\n", arg->tid,
                size);
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
        hz10_bench_free_hz10(p);
      }
    }

    /* Segment boundary: deliberately NO unconditional full inbox flush
     * here -- unlike the locked-in bench's per-run cleanup, this thread
     * just keeps going, same as a real long-running process would. Only
     * self-report this thread's own class-list stats (its TLS state is
     * never torn down mid-experiment). Two barrier waits (not one):
     * the first ensures every worker has finished collect_segment_
     * stats() before main reads the totals; the second ensures main has
     * finished printing and resetting before any worker starts the next
     * segment's loop (which will eventually call collect_segment_stats()
     * again and must not race the reset). Negligible overhead -- THREADS
     * is small and this runs `runs` times total, not per-op. */
    collect_segment_stats();
    pthread_barrier_wait(arg->barrier);
    pthread_barrier_wait(arg->barrier);
  }
  return NULL;
}

int main(void) {
  uint32_t threads = (uint32_t)hz10_bench_env_u64("THREADS", 4ull);
  uint64_t iters = hz10_bench_env_u64("ITERS", 500000ull);
  size_t min_size = (size_t)hz10_bench_env_u64("MIN_SIZE", 16ull);
  size_t max_size = (size_t)hz10_bench_env_u64("MAX_SIZE", 32768ull);
  uint32_t remote_pct = (uint32_t)hz10_bench_env_u64("REMOTE_PCT", 50ull);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 10ull);

  if (threads == 0u || min_size == 0u || max_size < min_size ||
      iters == 0u || runs == 0u) {
    fprintf(stderr, "invalid THREADS/MIN_SIZE/MAX_SIZE/ITERS/RUNS\n");
    return 1;
  }

  hz10_pagemap_reset_for_tests();

  pthread_t* thread_ids = calloc(threads, sizeof(*thread_ids));
  WorkerArg* args = calloc(threads, sizeof(*args));
  Hz10BenchInbox* inboxes = calloc(threads, sizeof(*inboxes));
  if (!thread_ids || !args || !inboxes) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, threads + 1u);
  for (uint32_t t = 0; t < threads; ++t) {
    /* Sized for the WHOLE experiment (all segments), not one segment,
     * since these threads and their inboxes live for the entire run. */
    hz10_bench_inbox_init(&inboxes[t],
                         (size_t)(iters * runs * threads + 1u));
  }

  printf(
      "hz10_public_entry_thread_reuse threads=%u iters_per_segment=%llu "
      "segments=%u min_size=%zu max_size=%zu remote_pct=%u\n",
      threads, (unsigned long long)iters, runs, min_size, max_size,
      remote_pct);

  for (uint32_t t = 0; t < threads; ++t) {
    args[t].tid = t;
    args[t].threads = threads;
    args[t].min_size = min_size;
    args[t].max_size = max_size;
    args[t].iters_per_segment = iters;
    args[t].segments = runs;
    args[t].remote_pct = remote_pct;
    args[t].inboxes = inboxes;
    args[t].barrier = &barrier;
    args[t].failed = 0;
    if (pthread_create(&thread_ids[t], NULL, worker, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }

  /* Main is the "+1" party at every segment's pair of barrier waits (see
   * the worker's own comment): the first wait releases once every
   * worker has finished collect_segment_stats() for this segment, the
   * second (after printing+resetting below) tells workers it is safe to
   * proceed into the next segment's loop. */
  uint64_t seg_start = hz10_platform_now_ns();
  for (uint32_t seg = 1; seg <= runs; ++seg) {
    pthread_barrier_wait(&barrier);
    uint64_t now = hz10_platform_now_ns();
    double seconds = (double)(now - seg_start) / 1000000000.0;
    if (seconds <= 0.0) {
      seconds = 1e-9;
    }
    uint64_t ops = iters * (uint64_t)threads;
    struct rusage usage;
    long rss_kb = getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
    size_t boundary_inbox_count = 0u;
    size_t boundary_inbox_max = 0u;
    for (uint32_t t = 0; t < threads; ++t) {
      size_t count = hz10_bench_inbox_count(&inboxes[t]);
      boundary_inbox_count += count;
      if (count > boundary_inbox_max) {
        boundary_inbox_max = count;
      }
    }
    printf(
        "hz10_public_entry_thread_reuse run=%u/%u seconds=%.6f "
        "ops_per_s=%.2f post_rss_kb=%ld active_length_sum=%llu "
        "retired_length_sum=%llu eviction_count=%llu retired_count=%llu "
        "harvest_call_count=%llu reclaimed_by_sweep=%llu "
        "promoted_by_sweep=%llu reclaimed_by_ready=%llu "
        "promoted_by_ready=%llu boundary_inbox_count=%zu "
        "boundary_inbox_max=%zu\n",
        seg, runs, seconds, (double)ops / seconds, rss_kb,
        (unsigned long long)atomic_load_explicit(&g_active_length_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_retired_length_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_eviction_count_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_retired_count_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_harvest_call_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_reclaimed_by_sweep_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_promoted_by_sweep_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_reclaimed_by_ready_sum,
                                                memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(&g_promoted_by_ready_sum,
                                                memory_order_relaxed),
        boundary_inbox_count, boundary_inbox_max);
    reset_segment_stats();
    pthread_barrier_wait(&barrier);
    seg_start = hz10_platform_now_ns();
  }

  int failed = 0;
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(thread_ids[t], NULL);
    failed |= args[t].failed;
  }
  pthread_barrier_destroy(&barrier);
  for (uint32_t t = 0; t < threads; ++t) {
    hz10_bench_inbox_destroy(&inboxes[t]);
  }
  free(inboxes);
  free(args);
  free(thread_ids);

  return failed ? 1 : 0;
}
