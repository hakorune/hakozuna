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

/* free_fn is either hz10_free (cast to match) or a libc-free wrapper, so
 * the same inbox plumbing serves both mech=hz10 and mech=system_malloc. */
static void hz10_bench_inbox_drain(Hz10BenchInbox* box,
                                  void (*free_fn)(void*)) {
  hz10_platform_mutex_lock(&box->lock);
  for (size_t i = 0; i < box->count; ++i) {
    free_fn(box->items[i]);
  }
  box->count = 0;
  hz10_platform_mutex_unlock(&box->lock);
}

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

static long hz10_bench_maxrss_kb(void) {
  struct rusage usage;
  return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
}

/*
 * Optional, opt-in (HZ10_DUMP_CLASS_STATS=1): aggregates Box 6's per-class
 * eviction/reclaim counters (src/hz10_class_pages.h, read via
 * hz10_public_entry_class_list_stats()) across every worker thread and
 * class, right before each thread's TLS state disappears at pthread_join.
 * Added specifically to check the main_r50/main_r90 RSS finding in
 * current_task.md against a real workload instead of only the isolated
 * unit tests: does this row's classes ever actually reach
 * HZ10_CLASS_PAGES_SCAN_LIMIT and trigger an eviction, and if so, how
 * often is the evicted page idle (reclaimed) vs not? Off by default --
 * zero cost for every other bench run through this file. */
static int hz10_bench_dump_class_stats;
static _Atomic(uint64_t) hz10_bench_class_length_sum;
static _Atomic(uint64_t) hz10_bench_class_eviction_sum;
static _Atomic(uint64_t) hz10_bench_class_reclaimed_sum;

static void hz10_bench_collect_class_stats(void) {
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    uint32_t length = 0u;
    uint64_t eviction_count = 0u;
    uint64_t eviction_reclaimed_count = 0u;
    hz10_public_entry_class_list_stats(c, &length, &eviction_count,
                                       &eviction_reclaimed_count);
    atomic_fetch_add_explicit(&hz10_bench_class_length_sum, length,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_class_eviction_sum, eviction_count,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&hz10_bench_class_reclaimed_sum,
                              eviction_reclaimed_count, memory_order_relaxed);
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
    atomic_store_explicit(&hz10_bench_class_length_sum, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&hz10_bench_class_eviction_sum, 0u,
                          memory_order_relaxed);
    atomic_store_explicit(&hz10_bench_class_reclaimed_sum, 0u,
                          memory_order_relaxed);
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
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  uint64_t ops = iters * (uint64_t)threads;
  printf(
      "hz10_public_entry mech=%s threads=%u iters_per_thread=%llu ops=%llu "
      "run=%u/%u min_size=%zu max_size=%zu remote_pct=%u seconds=%.6f "
      "ops_per_s=%.2f post_rss_kb=%ld\n",
      mech, threads, (unsigned long long)iters, (unsigned long long)ops, run,
      runs, min_size, max_size, remote_pct, seconds, (double)ops / seconds,
      hz10_bench_maxrss_kb());

  if (use_hz10 && hz10_bench_dump_class_stats) {
    printf(
        "hz10_public_entry_class_stats mech=%s run=%u/%u "
        "total_length_sum=%llu total_eviction_count=%llu "
        "total_eviction_reclaimed_count=%llu\n",
        mech, run, runs,
        (unsigned long long)atomic_load_explicit(
            &hz10_bench_class_length_sum, memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(
            &hz10_bench_class_eviction_sum, memory_order_relaxed),
        (unsigned long long)atomic_load_explicit(
            &hz10_bench_class_reclaimed_sum, memory_order_relaxed));
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
