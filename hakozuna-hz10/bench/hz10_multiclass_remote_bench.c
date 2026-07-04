#include "../src/hz10_freelist_page.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_remote_stack.h"
#include "../src/hz10_size_class.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Isolates one specific, still-open question from current_task.md's
 * main_r50/main_r90 investigation: is the per-op atomic RMW cost of a
 * remote free (pending-bit fetch_or + Treiber CAS push,
 * src/hz10_remote_stack.c) roughly CONSTANT regardless of how many
 * distinct size classes are concurrently active, or does it get worse as
 * more classes/pages are touched (cache/TLB pressure from spreading
 * across many separate Hz10FreelistPage structs and their backing
 * quanta, one of the last untested hypotheses for why main_r50/r90 sit at
 * ~0.48 of tcmalloc)?
 *
 * Deliberately mirrors bench/hz10_remote_stack_drain_bench.c's shape (the
 * existing, already-validated methodology for measuring remote-free cost
 * without bench-harness contamination): pre-allocate every slot up front
 * (owner thread, single-threaded, cheap), split ownership STATICALLY
 * across producer threads before the timed region starts, then time a
 * pure parallel remote-free burst followed by a single owner drain. This
 * avoids two confounds identified in the main_r50/r90 investigation: (1)
 * bench/hz10_public_entry_bench.c's own Hz10BenchInbox mutex, which
 * dominated raw strace syscall counts equally for every mechanism under
 * test, and (2) fresh-page creation/eviction churn, which is a separate,
 * already-measured cost (quantum reservation, Box 6/Box 4 pool wiring).
 *
 * New here: CLASSES pages (using REAL hz10_size_class slot_size/
 * slot_count shapes, the first CLASSES of the 24 real classes) instead of
 * one, with all their pre-allocated slots pooled into one flat array,
 * shuffled (fixed seed, reproducible) before being split across
 * producer threads -- so each thread's contiguous slice is a realistic
 * mix of classes, not one class per thread. Sweeping CLASSES from 1 up to
 * 24 while holding total ops roughly constant answers the question
 * directly: flat ops/s across the sweep means no compounding effect from
 * class count; a clear downward trend means there is one.
 */

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

/* xorshift64 -- fast, deterministic, good enough to shuffle a flat array
 * without needing a real RNG library. */
static uint64_t hz10_bench_rng_next(uint64_t* state) {
  uint64_t x = *state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  *state = x;
  return x;
}

typedef struct ProducerSlice {
  Hz10FreelistPage** pages;
  uint32_t* slot_page_index;
  void** slots;
  uint32_t begin;
  uint32_t end;
  int failed;
} ProducerSlice;

static void* producer_main(void* raw) {
  ProducerSlice* arg = (ProducerSlice*)raw;
  for (uint32_t i = arg->begin; i < arg->end; ++i) {
    Hz10FreelistPage* page = arg->pages[arg->slot_page_index[i]];
    if (!hz10_page_remote_free(page, arg->slots[i], page->generation)) {
      arg->failed = 1;
      return NULL;
    }
  }
  return NULL;
}

/* One repeat cycle: owner allocates every slot on every page, shuffles the
 * flat (slot, owning-page) pairing, producer threads remote-free their
 * static slice concurrently (timed), then the owner drains every page
 * once (timed). Returns 0 on success. */
static int hz10_bench_one_repeat(Hz10FreelistPage** pages, uint32_t classes,
                                uint32_t* slot_counts, uint32_t total_slots,
                                void** slots, uint32_t* slot_page_index,
                                uint32_t threads, pthread_t* thread_ids,
                                ProducerSlice* args, uint64_t* seed,
                                uint64_t* push_ns, uint64_t* drain_ns,
                                uint64_t* merged_total) {
  uint32_t cursor = 0;
  for (uint32_t p = 0; p < classes; ++p) {
    for (uint32_t s = 0; s < slot_counts[p]; ++s) {
      void* slot = hz10_freelist_page_alloc(pages[p]);
      if (!slot) {
        fprintf(stderr, "setup: alloc failed on class %u slot %u\n", p, s);
        return 1;
      }
      slots[cursor] = slot;
      slot_page_index[cursor] = p;
      ++cursor;
    }
  }

  /* Fisher-Yates shuffle of the (slot, page) pairing so each producer's
   * contiguous slice below is a realistic cross-class mix, not one class
   * per thread. */
  for (uint32_t i = total_slots; i > 1u; --i) {
    uint32_t j = (uint32_t)(hz10_bench_rng_next(seed) % (uint64_t)i);
    uint32_t k = i - 1u;
    void* tmp_slot = slots[k];
    slots[k] = slots[j];
    slots[j] = tmp_slot;
    uint32_t tmp_page = slot_page_index[k];
    slot_page_index[k] = slot_page_index[j];
    slot_page_index[j] = tmp_page;
  }

  uint32_t per_thread = total_slots / threads;
  uint64_t start = hz10_platform_now_ns();
  for (uint32_t t = 0; t < threads; ++t) {
    args[t].pages = pages;
    args[t].slot_page_index = slot_page_index;
    args[t].slots = slots;
    args[t].begin = t * per_thread;
    args[t].end = (t + 1u == threads) ? total_slots : (t + 1u) * per_thread;
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
  uint32_t merged = 0;
  for (uint32_t p = 0; p < classes; ++p) {
    merged += hz10_page_drain_remote(pages[p]);
  }
  *drain_ns += hz10_platform_now_ns() - start;
  if (merged != total_slots) {
    fprintf(stderr, "drain merged %u, expected %u (a push was lost)\n",
            merged, total_slots);
    return 1;
  }
  *merged_total += merged;
  return 0;
}

static void hz10_bench_report(const char* mech, uint32_t classes,
                             uint32_t threads, uint64_t ops, uint32_t run,
                             uint32_t runs, uint64_t elapsed_ns) {
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf("hz10_multiclass_remote mech=%s classes=%u threads=%u ops=%llu "
        "run=%u/%u seconds=%.6f ops_per_s=%.2f\n",
        mech, classes, threads, (unsigned long long)ops, run, runs, seconds,
        (double)ops / seconds);
}

static int hz10_bench_run_classes(uint32_t classes, uint32_t threads,
                                 uint64_t repeat, uint32_t runs) {
  Hz10FreelistPage** pages = calloc(classes, sizeof(*pages));
  uint32_t* slot_counts = calloc(classes, sizeof(*slot_counts));
  pthread_t* thread_ids = calloc(threads, sizeof(*thread_ids));
  ProducerSlice* args = calloc(threads, sizeof(*args));
  if (!pages || !slot_counts || !thread_ids || !args) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  uint32_t total_slots = 0;
  for (uint32_t p = 0; p < classes; ++p) {
    uint32_t slot_size = hz10_size_class_slot_size(p);
    uint32_t slot_count = hz10_size_class_slot_count(p);
    pages[p] = hz10_freelist_page_create(slot_size, slot_count);
    if (!pages[p]) {
      fprintf(stderr, "setup: hz10_freelist_page_create failed for class %u\n",
              p);
      return 1;
    }
    slot_counts[p] = slot_count;
    total_slots += slot_count;
  }
  if (total_slots < threads) {
    fprintf(stderr, "classes=%u too small: total_slots %u < threads %u\n",
            classes, total_slots, threads);
    return 1;
  }

  void** slots = calloc(total_slots, sizeof(*slots));
  uint32_t* slot_page_index = calloc(total_slots, sizeof(*slot_page_index));
  if (!slots || !slot_page_index) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  int failed = 0;
  for (uint32_t run = 1; run <= runs && !failed; ++run) {
    uint64_t seed = 0x2545F4914F6CDD1Dull ^ (uint64_t)classes;
    uint64_t push_ns = 0, drain_ns = 0, merged_total = 0;
    for (uint64_t r = 0; r < repeat && !failed; ++r) {
      failed |= hz10_bench_one_repeat(pages, classes, slot_counts,
                                     total_slots, slots, slot_page_index,
                                     threads, thread_ids, args, &seed,
                                     &push_ns, &drain_ns, &merged_total);
    }
    if (!failed) {
      uint64_t ops = repeat * (uint64_t)total_slots;
      hz10_bench_report("remote_push", classes, threads, ops, run, runs,
                       push_ns);
      hz10_bench_report("owner_drain", classes, 1u, ops, run, runs,
                       drain_ns);
    }
  }

  free(slot_page_index);
  free(slots);
  for (uint32_t p = 0; p < classes; ++p) {
    hz10_freelist_page_destroy(pages[p]);
  }
  free(args);
  free(thread_ids);
  free(slot_counts);
  free(pages);
  return failed ? 1 : 0;
}

int main(void) {
  uint32_t threads = (uint32_t)hz10_bench_env_u64("THREADS", 4ull);
  uint64_t repeat = hz10_bench_env_u64("REPEAT", 200ull);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 1ull);
  uint32_t single_classes = (uint32_t)hz10_bench_env_u64("CLASSES", 0ull);

  if (threads == 0u) {
    fprintf(stderr, "THREADS must be > 0\n");
    return 1;
  }

  uint32_t sweep[] = {1u, 2u, 4u, 8u, 16u, HZ10_CLASS_COUNT};
  uint32_t sweep_count = sizeof(sweep) / sizeof(sweep[0]);

  if (single_classes != 0u) {
    if (single_classes > HZ10_CLASS_COUNT) {
      fprintf(stderr, "CLASSES must be <= %u\n", HZ10_CLASS_COUNT);
      return 1;
    }
    return hz10_bench_run_classes(single_classes, threads, repeat, runs);
  }

  for (uint32_t i = 0; i < sweep_count; ++i) {
    if (hz10_bench_run_classes(sweep[i], threads, repeat, runs)) {
      return 1;
    }
  }
  return 0;
}
