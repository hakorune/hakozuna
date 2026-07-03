#include "../src/hz10_freelist_page.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_remote_stack.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * HZ10RemoteStackDrain-L0 bench (bench/README.md: THREADS/ITERS/RUNS,
 * no fused-loop, no DCE-prone numbers).
 *
 * One repeat cycle: allocate every slot on the page (owner thread), hand
 * out disjoint slices to THREADS producer threads, time how long all
 * producers take to remote-free their slice concurrently, then time a
 * single owner drain that merges everything back. Repeated REPEAT times to
 * get a stable aggregate. A single-threaded "local" free of the same total
 * op count (reusing Box 2's hz10_freelist_page_free) is reported alongside
 * as the same-run reference point for "cost of going remote", mirroring
 * HZ9's local0-vs-remote-row comparisons.
 *
 * Anti-DCE: every hz10_page_remote_free() call's return value is checked
 * (fail-fast on an unexpected rejection, which cannot happen in this
 * controlled scenario unless something is broken), and each drain's
 * merged count is checked against the expected total.
 */

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

typedef struct ProducerArg {
  Hz10FreelistPage* page;
  void** slots;
  uint32_t begin;
  uint32_t end;
  int failed;
} ProducerArg;

static void* producer_main(void* raw) {
  ProducerArg* arg = (ProducerArg*)raw;
  for (uint32_t i = arg->begin; i < arg->end; ++i) {
    if (!hz10_page_remote_free(arg->page, arg->slots[i],
                              arg->page->generation)) {
      arg->failed = 1;
      return NULL;
    }
  }
  return NULL;
}

/* Returns 0 on success, nonzero on any correctness failure. Adds elapsed
 * nanoseconds for the push phase and the drain phase to *push_ns / *drain_ns
 * and the slots merged by drain to *merged_total. */
static int hz10_bench_one_repeat(Hz10FreelistPage* page, void** slots,
                                uint32_t slot_count, uint32_t threads,
                                pthread_t* thread_ids, ProducerArg* args,
                                uint64_t* push_ns, uint64_t* drain_ns,
                                uint64_t* merged_total) {
  for (uint32_t i = 0; i < slot_count; ++i) {
    slots[i] = hz10_freelist_page_alloc(page);
    if (!slots[i]) {
      fprintf(stderr, "setup: alloc %u failed (page not fully drained?)\n",
              i);
      return 1;
    }
  }

  uint32_t per_thread = slot_count / threads;
  uint64_t start = hz10_platform_now_ns();
  for (uint32_t t = 0; t < threads; ++t) {
    args[t].page = page;
    args[t].slots = slots;
    args[t].begin = t * per_thread;
    args[t].end =
        (t + 1u == threads) ? slot_count : (t + 1u) * per_thread;
    args[t].failed = 0;
    if (pthread_create(&thread_ids[t], NULL, producer_main, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }
  int failed = 0;
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(thread_ids[t], NULL);
    failed |= args[t].failed;
  }
  *push_ns += hz10_platform_now_ns() - start;
  if (failed) {
    fprintf(stderr, "a producer thread had an unexpected remote_free "
                    "rejection\n");
    return 1;
  }

  start = hz10_platform_now_ns();
  uint32_t merged = hz10_page_drain_remote(page);
  *drain_ns += hz10_platform_now_ns() - start;
  if (merged != slot_count) {
    fprintf(stderr, "drain merged %u, expected %u (a push was lost)\n",
            merged, slot_count);
    return 1;
  }
  *merged_total += merged;
  return 0;
}

static int hz10_bench_local_reference(Hz10FreelistPage* page, void** slots,
                                     uint32_t slot_count, uint64_t repeat,
                                     uint64_t* elapsed_ns) {
  uint64_t start = hz10_platform_now_ns();
  for (uint64_t r = 0; r < repeat; ++r) {
    for (uint32_t i = 0; i < slot_count; ++i) {
      slots[i] = hz10_freelist_page_alloc(page);
      if (!slots[i]) {
        fprintf(stderr, "local reference: alloc %u failed on repeat %llu\n",
                i, (unsigned long long)r);
        return 1;
      }
    }
    for (uint32_t i = 0; i < slot_count; ++i) {
      hz10_freelist_page_free(page, slots[i]);
    }
  }
  *elapsed_ns = hz10_platform_now_ns() - start;
  return 0;
}

static void hz10_bench_report(const char* mech, uint32_t threads,
                             uint64_t ops, uint32_t run, uint32_t runs,
                             uint32_t slot_count, uint64_t elapsed_ns) {
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf(
      "hz10_remote_stack_drain mech=%s threads=%u ops=%llu run=%u/%u "
      "slot_count=%u seconds=%.6f ops_per_s=%.2f\n",
      mech, threads, (unsigned long long)ops, run, runs, slot_count, seconds,
      (double)ops / seconds);
}

int main(void) {
  uint32_t slot_size = (uint32_t)hz10_bench_env_u64("SLOT_SIZE", 64ull);
  uint32_t slot_count = (uint32_t)hz10_bench_env_u64("SLOT_COUNT", 1024ull);
  uint32_t threads = (uint32_t)hz10_bench_env_u64("THREADS", 4ull);
  uint64_t repeat = hz10_bench_env_u64("REPEAT", 2000ull);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 1ull);

  if (threads == 0u || slot_count % threads != 0u) {
    fprintf(stderr, "SLOT_COUNT must be a positive multiple of THREADS\n");
    return 1;
  }

  if ((uint64_t)slot_size * (uint64_t)slot_count > (uint64_t)HZ10_PAGE_QUANTUM) {
    fprintf(stderr,
            "SLOT_SIZE * SLOT_COUNT must fit in one %u-byte quantum "
            "(got %llu)\n",
            (unsigned)HZ10_PAGE_QUANTUM,
            (unsigned long long)slot_size * slot_count);
    return 1;
  }

  Hz10FreelistPage* page = hz10_freelist_page_create(slot_size, slot_count);
  if (!page) {
    fprintf(stderr, "setup: hz10_freelist_page_create failed\n");
    return 1;
  }
  void** slots = calloc(slot_count, sizeof(*slots));
  pthread_t* thread_ids = calloc(threads, sizeof(*thread_ids));
  ProducerArg* args = calloc(threads, sizeof(*args));
  if (!slots || !thread_ids || !args) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  int failed = 0;
  for (uint32_t run = 1; run <= runs && !failed; ++run) {
    uint64_t push_ns = 0, drain_ns = 0, merged_total = 0;
    for (uint64_t r = 0; r < repeat && !failed; ++r) {
      failed |= hz10_bench_one_repeat(page, slots, slot_count, threads,
                                     thread_ids, args, &push_ns, &drain_ns,
                                     &merged_total);
    }
    if (!failed) {
      uint64_t ops = repeat * (uint64_t)slot_count;
      hz10_bench_report("remote_push", threads, ops, run, runs, slot_count,
                       push_ns);
      hz10_bench_report("owner_drain", 1u, ops, run, runs, slot_count,
                       drain_ns);
    }

    uint64_t local_ns = 0;
    if (!failed &&
        hz10_bench_local_reference(page, slots, slot_count, repeat,
                                  &local_ns)) {
      failed = 1;
    }
    if (!failed) {
      hz10_bench_report("local_free", 1u, repeat * (uint64_t)slot_count, run,
                       runs, slot_count, local_ns);
    }
  }

  free(args);
  free(thread_ids);
  free(slots);
  hz10_freelist_page_destroy(page);
  return failed ? 1 : 0;
}
