#include "../src/hz10_page_pool.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_pooled_page.h"
#include "../src/hz10_remote_stack.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

/*
 * HZ10BoundedPagePool-L0 bench: the local/remote/RSS matrix
 * (bench/README.md).
 *
 * Same create/alloc/free-or-remote-free/destroy cycle, run twice with only
 * the pool cap changed (CAP vs. 0), so "does pooling help" is isolated to
 * that one parameter rather than comparing different code paths. CAP=0
 * forces hz10_page_pool_release() to actually munmap every single time,
 * i.e. what HZ10 would cost with no pool at all.
 *
 * pooled_local:   create -> alloc all -> free all locally -> destroy
 * unpooled_local: identical cycle, pool cap forced to 0
 * pooled_remote:  create -> alloc all -> THREADS producers remote-free
 *                 their slice -> drain -> destroy
 *
 * RSS (ru_maxrss) is sampled before pooled_local and after unpooled_local
 * to see the cost, in peak resident memory, of the mmap/munmap churn the
 * pool exists to avoid -- reported honestly either way, not assumed.
 */

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

static long hz10_bench_maxrss_kb(void) {
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    return -1;
  }
  return usage.ru_maxrss;
}

static int hz10_bench_local_cycle(uint32_t slot_size, uint32_t slot_count) {
  Hz10FreelistPage* page = hz10_pooled_page_create(slot_size, slot_count);
  if (!page) {
    return 1;
  }
  void** slots = malloc(sizeof(*slots) * slot_count);
  if (!slots) {
    hz10_pooled_page_destroy(page);
    return 1;
  }
  for (uint32_t i = 0; i < slot_count; ++i) {
    slots[i] = hz10_freelist_page_alloc(page);
    if (!slots[i]) {
      free(slots);
      hz10_pooled_page_destroy(page);
      return 1;
    }
  }
  for (uint32_t i = 0; i < slot_count; ++i) {
    hz10_freelist_page_free(page, slots[i]);
  }
  free(slots);
  hz10_pooled_page_destroy(page);
  return 0;
}

typedef struct RemoteProducerArg {
  Hz10FreelistPage* page;
  void** slots;
  uint32_t begin;
  uint32_t end;
  int failed;
} RemoteProducerArg;

static void* remote_producer_main(void* raw) {
  RemoteProducerArg* arg = (RemoteProducerArg*)raw;
  for (uint32_t i = arg->begin; i < arg->end; ++i) {
    if (!hz10_page_remote_free(arg->page, arg->slots[i],
                              arg->page->generation)) {
      arg->failed = 1;
      return NULL;
    }
  }
  return NULL;
}

static int hz10_bench_remote_cycle(uint32_t slot_size, uint32_t slot_count,
                                  uint32_t threads, pthread_t* thread_ids,
                                  RemoteProducerArg* args) {
  Hz10FreelistPage* page = hz10_pooled_page_create(slot_size, slot_count);
  if (!page) {
    return 1;
  }
  void** slots = malloc(sizeof(*slots) * slot_count);
  if (!slots) {
    hz10_pooled_page_destroy(page);
    return 1;
  }
  for (uint32_t i = 0; i < slot_count; ++i) {
    slots[i] = hz10_freelist_page_alloc(page);
    if (!slots[i]) {
      free(slots);
      hz10_pooled_page_destroy(page);
      return 1;
    }
  }

  uint32_t per_thread = slot_count / threads;
  int failed = 0;
  for (uint32_t t = 0; t < threads; ++t) {
    args[t].page = page;
    args[t].slots = slots;
    args[t].begin = t * per_thread;
    args[t].end = (t + 1u == threads) ? slot_count : (t + 1u) * per_thread;
    args[t].failed = 0;
    if (pthread_create(&thread_ids[t], NULL, remote_producer_main,
                       &args[t]) != 0) {
      failed = 1;
    }
  }
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(thread_ids[t], NULL);
    failed |= args[t].failed;
  }
  if (!failed && hz10_page_drain_remote(page) != slot_count) {
    failed = 1;
  }

  free(slots);
  hz10_pooled_page_destroy(page);
  return failed;
}

static void hz10_bench_report(const char* mech, uint64_t ops, uint32_t run,
                             uint32_t runs, uint64_t elapsed_ns,
                             uint32_t cached, uint64_t reused,
                             uint64_t released) {
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf(
      "hz10_bounded_page_pool mech=%s ops=%llu run=%u/%u seconds=%.6f "
      "ops_per_s=%.2f pool_cached=%u pool_reused=%llu pool_released=%llu\n",
      mech, (unsigned long long)ops, run, runs, seconds,
      (double)ops / seconds, cached, (unsigned long long)reused,
      (unsigned long long)released);
}

int main(void) {
  uint32_t slot_size = (uint32_t)hz10_bench_env_u64("SLOT_SIZE", 64ull);
  uint32_t slot_count = (uint32_t)hz10_bench_env_u64("SLOT_COUNT", 64ull);
  uint32_t threads = (uint32_t)hz10_bench_env_u64("THREADS", 4ull);
  uint64_t repeat = hz10_bench_env_u64("REPEAT", 2000ull);
  uint32_t cap =
      (uint32_t)hz10_bench_env_u64("CAP", HZ10_PAGE_POOL_DEFAULT_CAP);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 1ull);

  if (threads == 0u || slot_count % threads != 0u) {
    fprintf(stderr, "SLOT_COUNT must be a positive multiple of THREADS\n");
    return 1;
  }

  pthread_t* thread_ids = calloc(threads, sizeof(*thread_ids));
  RemoteProducerArg* args = calloc(threads, sizeof(*args));
  if (!thread_ids || !args) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  int failed = 0;
  for (uint32_t run = 1; run <= runs && !failed; ++run) {
    long rss_before = hz10_bench_maxrss_kb();

    hz10_page_pool_reset_for_tests();
    hz10_page_pool_set_cap(cap);
    uint64_t start = hz10_platform_now_ns();
    for (uint64_t r = 0; r < repeat && !failed; ++r) {
      failed |= hz10_bench_local_cycle(slot_size, slot_count);
    }
    uint64_t elapsed = hz10_platform_now_ns() - start;
    if (!failed) {
      hz10_bench_report("pooled_local", repeat, run, runs, elapsed,
                       hz10_page_pool_cached_count(),
                       hz10_page_pool_reuse_count(),
                       hz10_page_pool_release_count());
    }

    hz10_page_pool_reset_for_tests();
    hz10_page_pool_set_cap(0u);
    start = hz10_platform_now_ns();
    for (uint64_t r = 0; r < repeat && !failed; ++r) {
      failed |= hz10_bench_local_cycle(slot_size, slot_count);
    }
    elapsed = hz10_platform_now_ns() - start;
    if (!failed) {
      hz10_bench_report("unpooled_local", repeat, run, runs, elapsed,
                       hz10_page_pool_cached_count(),
                       hz10_page_pool_reuse_count(),
                       hz10_page_pool_release_count());
    }
    long rss_after = hz10_bench_maxrss_kb();

    hz10_page_pool_reset_for_tests();
    hz10_page_pool_set_cap(cap);
    start = hz10_platform_now_ns();
    for (uint64_t r = 0; r < repeat && !failed; ++r) {
      failed |= hz10_bench_remote_cycle(slot_size, slot_count, threads,
                                       thread_ids, args);
    }
    elapsed = hz10_platform_now_ns() - start;
    if (!failed) {
      hz10_bench_report("pooled_remote", repeat * (uint64_t)slot_count, run,
                       runs, elapsed, hz10_page_pool_cached_count(),
                       hz10_page_pool_reuse_count(),
                       hz10_page_pool_release_count());
    }

    if (!failed) {
      printf("hz10_bounded_page_pool rss run=%u/%u before_kb=%ld "
             "after_kb=%ld\n",
             run, runs, rss_before, rss_after);
    }
  }

  free(args);
  free(thread_ids);
  return failed ? 1 : 0;
}
