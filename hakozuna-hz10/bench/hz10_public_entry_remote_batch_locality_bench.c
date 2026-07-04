#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_public_entry.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

typedef struct Inbox {
  hz10_platform_mutex_t lock;
  void** items;
  size_t count;
  size_t capacity;
} Inbox;

typedef struct LocalityStats {
  uint64_t remote_free_count;
  uint64_t ideal_publish_cas_count;
  uint64_t run_len_sum;
  uint64_t run_len_max;
  uint64_t hist[6]; /* 1, 2, 3-4, 5-8, 9-16, 17+ */
} LocalityStats;

typedef struct WorkerArg {
  uint32_t tid;
  uint32_t threads;
  uint64_t iters;
  uint32_t remote_pct;
  size_t min_size;
  size_t max_size;
  Inbox* inboxes;
  pthread_barrier_t* barrier;
  _Atomic(uint64_t)* remote_free_count;
  _Atomic(uint64_t)* ideal_publish_cas_count;
  _Atomic(uint64_t)* run_len_sum;
  _Atomic(uint64_t)* run_len_max;
  _Atomic(uint64_t)* hist;
  int failed;
} WorkerArg;

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  return value && value[0] ? strtoull(value, NULL, 10) : fallback;
}

static uint64_t rng_next(uint64_t* state) {
  uint64_t x = *state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  *state = x;
  return x;
}

static size_t choose_size(size_t min_size, size_t max_size, uint64_t* rng) {
  return min_size + (size_t)(rng_next(rng) % (max_size - min_size + 1u));
}

static void touch_alloc(void* p, size_t size) {
  unsigned char* c = (unsigned char*)p;
  c[0] = 0xA5u;
  c[size - 1u] = 0x5Au;
}

static void inbox_init(Inbox* inbox, size_t capacity) {
  hz10_platform_mutex_init(&inbox->lock);
  inbox->items = calloc(capacity, sizeof(*inbox->items));
  inbox->count = 0;
  inbox->capacity = capacity;
}

static void inbox_destroy(Inbox* inbox) {
  hz10_platform_mutex_destroy(&inbox->lock);
  free(inbox->items);
}

static void inbox_push(Inbox* inbox, void* ptr) {
  hz10_platform_mutex_lock(&inbox->lock);
  if (inbox->count < inbox->capacity) {
    inbox->items[inbox->count++] = ptr;
  }
  hz10_platform_mutex_unlock(&inbox->lock);
}

static void atomic_max_u64(_Atomic(uint64_t)* target, uint64_t candidate) {
  uint64_t current = atomic_load_explicit(target, memory_order_relaxed);
  while (candidate > current &&
         !atomic_compare_exchange_weak_explicit(target, &current, candidate,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}

static uint32_t hist_index(uint64_t len) {
  if (len <= 1u) return 0u;
  if (len == 2u) return 1u;
  if (len <= 4u) return 2u;
  if (len <= 8u) return 3u;
  if (len <= 16u) return 4u;
  return 5u;
}

static void commit_run(WorkerArg* arg, uint64_t len) {
  if (len == 0u) {
    return;
  }
  atomic_fetch_add_explicit(arg->ideal_publish_cas_count, 1u,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(arg->run_len_sum, len, memory_order_relaxed);
  atomic_max_u64(arg->run_len_max, len);
  atomic_fetch_add_explicit(&arg->hist[hist_index(len)], 1u,
                            memory_order_relaxed);
}

static void drain_inbox(Inbox* inbox, WorkerArg* arg) {
  hz10_platform_mutex_lock(&inbox->lock);
  void* prev_page = NULL;
  uint64_t run_len = 0u;
  for (size_t i = 0; i < inbox->count; ++i) {
    void* ptr = inbox->items[i];
    H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
    void* page = route.kind == H10_ROUTE_VALID ? route.owner : NULL;
    if (page && page == prev_page) {
      run_len += 1u;
    } else {
      commit_run(arg, run_len);
      prev_page = page;
      run_len = page ? 1u : 0u;
    }
    if (page) {
      atomic_fetch_add_explicit(arg->remote_free_count, 1u,
                                memory_order_relaxed);
    }
    (void)hz10_free(ptr);
  }
  commit_run(arg, run_len);
  inbox->count = 0;
  hz10_platform_mutex_unlock(&inbox->lock);
}

static void* worker_main(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  uint64_t rng = 0x9e3779b97f4a7c15ull ^ (uint64_t)(arg->tid + 1u);
  for (uint64_t i = 0; i < arg->iters; ++i) {
    if ((i & 31u) == 0u) {
      drain_inbox(&arg->inboxes[arg->tid], arg);
    }
    size_t size = choose_size(arg->min_size, arg->max_size, &rng);
    void* p = hz10_malloc(size);
    if (!p) {
      arg->failed = 1;
      return NULL;
    }
    touch_alloc(p, size);
    int remote = arg->threads > 1u &&
                 (uint32_t)(rng_next(&rng) % 100u) < arg->remote_pct;
    if (remote) {
      uint32_t dst = (arg->tid + 1u +
                     (uint32_t)(rng_next(&rng) % (arg->threads - 1u))) %
                    arg->threads;
      inbox_push(&arg->inboxes[dst], p);
    } else {
      (void)hz10_free(p);
    }
  }
  pthread_barrier_wait(arg->barrier);
  drain_inbox(&arg->inboxes[arg->tid], arg);
  pthread_barrier_wait(arg->barrier);
  drain_inbox(&arg->inboxes[arg->tid], arg);
  return NULL;
}

static long maxrss_kb(void) {
  struct rusage usage;
  return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
}

static int run_once(uint32_t threads, uint64_t iters, size_t min_size,
                    size_t max_size, uint32_t remote_pct, uint32_t run,
                    uint32_t runs) {
  pthread_t* tids = calloc(threads, sizeof(*tids));
  WorkerArg* args = calloc(threads, sizeof(*args));
  Inbox* inboxes = calloc(threads, sizeof(*inboxes));
  pthread_barrier_t barrier;
  _Atomic(uint64_t) remote_free_count = 0, ideal_cas = 0, run_sum = 0,
                    run_max = 0, hist[6];
  for (uint32_t i = 0; i < 6u; ++i) atomic_init(&hist[i], 0u);
  if (!tids || !args || !inboxes) return 1;
  pthread_barrier_init(&barrier, NULL, threads);
  for (uint32_t t = 0; t < threads; ++t) inbox_init(&inboxes[t], iters * threads + 1u);
  uint64_t start = hz10_platform_now_ns();
  for (uint32_t t = 0; t < threads; ++t) {
    args[t] = (WorkerArg){t, threads, iters, remote_pct, min_size, max_size,
                          inboxes, &barrier, &remote_free_count, &ideal_cas,
                          &run_sum, &run_max, hist, 0};
    if (pthread_create(&tids[t], NULL, worker_main, &args[t]) != 0) return 1;
  }
  int failed = 0;
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(tids[t], NULL);
    failed |= args[t].failed;
  }
  uint64_t elapsed_ns = hz10_platform_now_ns() - start;
  uint64_t remote = atomic_load_explicit(&remote_free_count, memory_order_relaxed);
  uint64_t cas = atomic_load_explicit(&ideal_cas, memory_order_relaxed);
  double reduction = remote ? 1.0 - ((double)cas / (double)remote) : 0.0;
  double avg_run = cas ? (double)atomic_load_explicit(&run_sum, memory_order_relaxed) / (double)cas : 0.0;
  printf("hz10_remote_batch_locality run=%u/%u threads=%u iters_per_thread=%llu "
         "min_size=%zu max_size=%zu remote_pct=%u remote_free_count=%llu "
         "ideal_publish_cas_count=%llu cas_reduction_ceiling=%.6f "
         "avg_same_page_run=%.3f max_same_page_run=%llu "
         "hist_1=%llu hist_2=%llu hist_3_4=%llu hist_5_8=%llu "
         "hist_9_16=%llu hist_17_plus=%llu seconds=%.6f post_rss_kb=%ld\n",
         run, runs, threads, (unsigned long long)iters, min_size, max_size,
         remote_pct, (unsigned long long)remote, (unsigned long long)cas,
         reduction, avg_run,
         (unsigned long long)atomic_load_explicit(&run_max, memory_order_relaxed),
         (unsigned long long)atomic_load_explicit(&hist[0], memory_order_relaxed),
         (unsigned long long)atomic_load_explicit(&hist[1], memory_order_relaxed),
         (unsigned long long)atomic_load_explicit(&hist[2], memory_order_relaxed),
         (unsigned long long)atomic_load_explicit(&hist[3], memory_order_relaxed),
         (unsigned long long)atomic_load_explicit(&hist[4], memory_order_relaxed),
         (unsigned long long)atomic_load_explicit(&hist[5], memory_order_relaxed),
         (double)elapsed_ns / 1000000000.0, maxrss_kb());
  for (uint32_t t = 0; t < threads; ++t) inbox_destroy(&inboxes[t]);
  free(inboxes); free(args); free(tids); pthread_barrier_destroy(&barrier);
  return failed;
}

int main(void) {
  uint32_t threads = (uint32_t)env_u64("THREADS", 4ull);
  uint64_t iters = env_u64("ITERS", 500000ull);
  size_t min_size = (size_t)env_u64("MIN_SIZE", 16ull);
  size_t max_size = (size_t)env_u64("MAX_SIZE", 32768ull);
  uint32_t remote_pct = (uint32_t)env_u64("REMOTE_PCT", 50ull);
  uint32_t runs = (uint32_t)env_u64("RUNS", 10ull);
  if (threads == 0u || min_size == 0u || max_size < min_size || runs == 0u) {
    fprintf(stderr, "invalid bench parameters\n");
    return 1;
  }
  for (uint32_t run = 1; run <= runs; ++run) {
    hz10_pagemap_reset_for_tests();
    if (run_once(threads, iters, min_size, max_size, remote_pct, run, runs)) {
      return 1;
    }
  }
  return 0;
}
