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
 * Deliberately separate from bench/hz10_public_entry_bench.c and never
 * named main_rNN/main_local0 -- this is NOT a replacement for that bench's
 * locked-in ITERS=500000 RUNS=10 comparison table (current_task.md), it is
 * a diagnostic aimed at one specific question that table cannot answer.
 *
 * What that table's structure hid (found while measuring
 * HZ10RetiredReadyQueue-L0 against it -- see current_task.md's "polling-
 * vs-event-driven" and its wiring follow-up): every worker there runs a
 * FIXED iteration count, then does pthread_barrier_wait + two
 * unconditional full inbox drains before the run ends. Instrumentation
 * (mark/note/push counters, since reverted) showed retired pages for
 * main_r50/r90's dominant pathological class resolve almost entirely
 * during that final unconditional flush -- AFTER the owning thread has
 * already stopped calling hz10_malloc, so neither harvest's budgeted
 * cursor NOR this module's ready queue ever get a chance to observe or
 * act on that resolution within the run's own measurement window. Both
 * mechanisms depend on the owner continuing to check in; a benchmark
 * whose workers stop checking in right as resolution happens cannot see
 * either one's effect, positive or negative.
 *
 * This bench removes that structural blind spot: workers run for
 * RUN_SECONDS of wall-clock time (steady state, not a fixed iteration
 * count), so hz10_malloc keeps being called -- and harvest keeps being
 * given a chance to drain the ready queue -- throughout, not just up to
 * an arbitrary stopping point. Stats are captured TWICE: once the instant
 * each worker's own time-bounded loop ends (before any flush -- what a
 * continuously-running process would actually look like at any given
 * moment) and once more after the same barrier + two-round unconditional
 * flush the other bench uses (to directly show how much of any gap is
 * attributable to that artificial cleanup step versus genuine steady-
 * state behavior). Mid-run checkpoints (approximate: each worker self-
 * reports against its own elapsed-time clock, no synchronizing barrier,
 * since a barrier here would itself distort the steady state being
 * measured) show the trend, not just a single end number.
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

/* One bucket of aggregated Hz10ClassPageListStats fields relevant to
 * this bench's question -- active_length/max_retired_length omitted,
 * not relevant here. */
typedef struct StatBucket {
  _Atomic(uint64_t) active_length;
  _Atomic(uint64_t) retired_length;
  _Atomic(uint64_t) retired_count;
  _Atomic(uint64_t) harvest_call_count;
  _Atomic(uint64_t) retired_reclaimed_by_sweep;
  _Atomic(uint64_t) retired_promoted_by_sweep;
  _Atomic(uint64_t) retired_reclaimed_by_ready;
  _Atomic(uint64_t) retired_promoted_by_ready;
  _Atomic(uint64_t) ready_false_positive;
  _Atomic(uint64_t) ready_push;
} StatBucket;

static void stat_bucket_add(StatBucket* bucket,
                           const Hz10ClassPageListStats* stats) {
  atomic_fetch_add_explicit(&bucket->active_length, stats->active_length,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->retired_length, stats->retired_length,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->retired_count, stats->retired_count,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->harvest_call_count,
                            stats->harvest_call_count, memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->retired_reclaimed_by_sweep,
                            stats->retired_reclaimed_by_sweep_count,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->retired_promoted_by_sweep,
                            stats->retired_promoted_by_sweep_count,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->retired_reclaimed_by_ready,
                            stats->retired_reclaimed_by_ready_count,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->retired_promoted_by_ready,
                            stats->retired_promoted_by_ready_count,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->ready_false_positive,
                            stats->ready_false_positive_count,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&bucket->ready_push, stats->ready_push_count,
                            memory_order_relaxed);
}

static void stat_bucket_print(const char* label, const StatBucket* bucket) {
  printf(
      "  [%s] active_length=%llu retired_length=%llu retired_count=%llu "
      "harvest_calls=%llu "
      "reclaimed_by_sweep=%llu promoted_by_sweep=%llu "
      "reclaimed_by_ready=%llu promoted_by_ready=%llu "
      "ready_false_positive=%llu ready_push=%llu\n",
      label,
      (unsigned long long)atomic_load_explicit(&bucket->active_length,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&bucket->retired_length,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&bucket->retired_count,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&bucket->harvest_call_count,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(
          &bucket->retired_reclaimed_by_sweep, memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(
          &bucket->retired_promoted_by_sweep, memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(
          &bucket->retired_reclaimed_by_ready, memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(
          &bucket->retired_promoted_by_ready, memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&bucket->ready_false_positive,
                                              memory_order_relaxed),
      (unsigned long long)atomic_load_explicit(&bucket->ready_push,
                                              memory_order_relaxed));
}

#define HZ10_STEADY_MAX_CHECKPOINTS 16u

static StatBucket g_steady_state_bucket;
static StatBucket g_post_flush_bucket;
static StatBucket g_checkpoint_buckets[HZ10_STEADY_MAX_CHECKPOINTS];
static uint32_t g_checkpoint_count;
static _Atomic(uint64_t) g_total_ops;

static void collect_into(StatBucket* bucket) {
  for (uint32_t c = 0; c < HZ10_CLASS_COUNT; ++c) {
    Hz10ClassPageListStats stats;
    hz10_public_entry_class_list_stats(c, &stats);
    stat_bucket_add(bucket, &stats);
  }
}

typedef struct WorkerArg {
  uint32_t tid;
  uint32_t threads;
  size_t min_size;
  size_t max_size;
  uint32_t remote_pct;
  uint64_t run_ns;
  Hz10BenchInbox* inboxes;
  pthread_barrier_t* barrier;
} WorkerArg;

static void* worker(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  uint64_t rng = 0x9e3779b97f4a7c15ull ^ (uint64_t)(arg->tid + 1u);
  uint64_t start = hz10_platform_now_ns();
  uint64_t next_checkpoint_ns =
      arg->run_ns / (uint64_t)(g_checkpoint_count + 1u);
  uint32_t checkpoint_idx = 0u;
  uint64_t ops = 0u;

  for (;;) {
    if ((ops & 31u) == 0u) {
      hz10_bench_inbox_drain(&arg->inboxes[arg->tid], hz10_bench_free_hz10);
    }
    uint64_t elapsed = hz10_platform_now_ns() - start;
    if (elapsed >= arg->run_ns) {
      break;
    }
    if (checkpoint_idx < g_checkpoint_count &&
        elapsed >= next_checkpoint_ns) {
      collect_into(&g_checkpoint_buckets[checkpoint_idx]);
      ++checkpoint_idx;
      next_checkpoint_ns =
          arg->run_ns / (uint64_t)(g_checkpoint_count + 1u) *
          (uint64_t)(checkpoint_idx + 1u);
    }

    size_t size = hz10_bench_choose_size(arg->min_size, arg->max_size, &rng);
    void* p = hz10_malloc(size);
    if (!p) {
      fprintf(stderr, "worker %u: alloc failed (size=%zu)\n", arg->tid, size);
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
    ++ops;
  }
  atomic_fetch_add_explicit(&g_total_ops, ops, memory_order_relaxed);

  /* Steady-state snapshot: captured the instant this worker's own
   * time-bounded loop ends, before any barrier or unconditional flush --
   * see the module comment for why this moment matters. */
  collect_into(&g_steady_state_bucket);

  pthread_barrier_wait(arg->barrier);
  hz10_bench_inbox_drain(&arg->inboxes[arg->tid], hz10_bench_free_hz10);
  pthread_barrier_wait(arg->barrier);
  hz10_bench_inbox_drain(&arg->inboxes[arg->tid], hz10_bench_free_hz10);

  /* Post-flush snapshot: after the same unconditional cleanup the other
   * bench always did before ever capturing stats. */
  collect_into(&g_post_flush_bucket);
  return NULL;
}

int main(void) {
  uint32_t threads = (uint32_t)hz10_bench_env_u64("THREADS", 4ull);
  uint64_t run_seconds = hz10_bench_env_u64("RUN_SECONDS", 5ull);
  size_t min_size = (size_t)hz10_bench_env_u64("MIN_SIZE", 16ull);
  size_t max_size = (size_t)hz10_bench_env_u64("MAX_SIZE", 32768ull);
  uint32_t remote_pct = (uint32_t)hz10_bench_env_u64("REMOTE_PCT", 50ull);
  g_checkpoint_count =
      (uint32_t)hz10_bench_env_u64("CHECKPOINTS", 5ull);
  if (g_checkpoint_count > HZ10_STEADY_MAX_CHECKPOINTS) {
    g_checkpoint_count = HZ10_STEADY_MAX_CHECKPOINTS;
  }

  if (threads == 0u || min_size == 0u || max_size < min_size ||
      run_seconds == 0u) {
    fprintf(stderr, "invalid THREADS/MIN_SIZE/MAX_SIZE/RUN_SECONDS\n");
    return 1;
  }

  hz10_pagemap_reset_for_tests();

  uint64_t run_ns = run_seconds * 1000000000ull;
  pthread_t* thread_ids = calloc(threads, sizeof(*thread_ids));
  WorkerArg* args = calloc(threads, sizeof(*args));
  Hz10BenchInbox* inboxes = calloc(threads, sizeof(*inboxes));
  pthread_barrier_t barrier;
  if (!thread_ids || !args || !inboxes) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }
  pthread_barrier_init(&barrier, NULL, threads);
  /* Steady state has no fixed op count to size an inbox against -- use a
   * generously large capacity instead (bounded by how much a single
   * REMOTE_PCT-share of RUN_SECONDS of allocs could plausibly produce). */
  uint64_t inbox_capacity = 200000000ull / (uint64_t)threads + 1024ull;
  for (uint32_t t = 0; t < threads; ++t) {
    hz10_bench_inbox_init(&inboxes[t], (size_t)inbox_capacity);
  }

  printf(
      "hz10_public_entry_steady_state threads=%u run_seconds=%llu "
      "min_size=%zu max_size=%zu remote_pct=%u checkpoints=%u\n",
      threads, (unsigned long long)run_seconds, min_size, max_size,
      remote_pct, g_checkpoint_count);

  uint64_t start = hz10_platform_now_ns();
  for (uint32_t t = 0; t < threads; ++t) {
    args[t].tid = t;
    args[t].threads = threads;
    args[t].min_size = min_size;
    args[t].max_size = max_size;
    args[t].remote_pct = remote_pct;
    args[t].run_ns = run_ns;
    args[t].inboxes = inboxes;
    args[t].barrier = &barrier;
    if (pthread_create(&thread_ids[t], NULL, worker, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(thread_ids[t], NULL);
  }
  uint64_t elapsed_ns = hz10_platform_now_ns() - start;

  for (uint32_t t = 0; t < threads; ++t) {
    hz10_bench_inbox_destroy(&inboxes[t]);
  }
  free(inboxes);
  free(args);
  free(thread_ids);
  pthread_barrier_destroy(&barrier);

  double seconds = (double)elapsed_ns / 1000000000.0;
  uint64_t ops = atomic_load_explicit(&g_total_ops, memory_order_relaxed);
  struct rusage usage;
  long rss_kb = getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
  printf("  wall_seconds=%.3f total_ops=%llu ops_per_s=%.2f post_rss_kb=%ld\n",
        seconds, (unsigned long long)ops, (double)ops / seconds, rss_kb);

  for (uint32_t i = 0; i < g_checkpoint_count; ++i) {
    char label[32];
    snprintf(label, sizeof(label), "checkpoint %u/%u", i + 1u,
            g_checkpoint_count);
    stat_bucket_print(label, &g_checkpoint_buckets[i]);
  }
  stat_bucket_print("steady_state (pre-flush)", &g_steady_state_bucket);
  stat_bucket_print("post_flush", &g_post_flush_bucket);

  return 0;
}
