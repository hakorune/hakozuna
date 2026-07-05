#include "../src/hz10_freelist_page.h"
#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_public_entry.h"
#include "../src/hz10_size_class.h"

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * HZ10LocalPathTrim-L0 (docs/HZ10_SPEED_ATTACK_PLAN_L0.md), measurement
 * phase. HZ10PublicFreeStageCost-L0 found route/validation overhead is
 * close to the same size as the remaining remote-specific cost -- this
 * bench answers the follow-up "Recommended measurement" list for the
 * LOCAL path specifically: fixed-size (not mixed random-size) local0 rows
 * by class, split into alloc-only and free-only phases against a warm
 * working set, plus a cold-vs-warm comparison to separate steady-state
 * substrate cost from one-time page/pagemap setup cost.
 *
 * Unlike bench/hz10_public_entry_stage_cost_bench.c, no cross-thread
 * barrier synchronization is needed here: every mode measured is purely
 * local (REMOTE_PCT=0 by construction, out of scope for this box), so
 * each worker thread's timing is entirely independent of every other
 * thread's progress -- same reasoning as the existing
 * bench/hz10_public_entry_bench.c, which also just joins threads and
 * sums independently-measured wall time.
 *
 * Each worker thread owns a private working set of WORKING_SET pointers,
 * all of the same CLASS_SIZE, and reuses them round after round (a
 * steady-state, already-warm active page, not a fresh one every round)
 * unless WARMUP_ROUNDS=0, in which case the first measured round is
 * whatever a genuinely cold start looks like.
 */

typedef enum LocalMode {
  MODE_ALLOC_FREE = 0, /* combined: free K, then alloc K, timed together */
  MODE_FREE_ONLY,       /* time only the K frees; realloc after, untimed */
  MODE_ALLOC_ONLY,       /* free K untimed first; time only the K allocs */
  MODE_COUNT
} LocalMode;

static const char* mode_name(LocalMode m) {
  switch (m) {
    case MODE_ALLOC_FREE:
      return "alloc_free";
    case MODE_FREE_ONLY:
      return "free_only";
    case MODE_ALLOC_ONLY:
      return "alloc_only";
    default:
      return "unknown";
  }
}

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

typedef struct WorkerArg {
  uint32_t tid;
  size_t class_size;
  uint32_t working_set;
  uint64_t rounds;
  uint32_t warmup_rounds;
  LocalMode mode;
  int use_hz10; /* 0 = system_malloc, for the same-run reference row */
  uint64_t elapsed_ns;
  int failed;
} WorkerArg;

static void* hz10_bench_alloc(int use_hz10, size_t size) {
  return use_hz10 ? hz10_malloc(size) : malloc(size);
}

static void hz10_bench_free(int use_hz10, void* ptr) {
  if (use_hz10) {
    (void)hz10_free(ptr);
  } else {
    free(ptr);
  }
}

static void* worker_main(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  void** ptrs = calloc(arg->working_set, sizeof(*ptrs));
  if (!ptrs) {
    arg->failed = 1;
    return NULL;
  }

  for (uint32_t k = 0; k < arg->working_set; ++k) {
    ptrs[k] = hz10_bench_alloc(arg->use_hz10, arg->class_size);
    if (!ptrs[k]) {
      fprintf(stderr, "local-path: setup alloc failed (tid=%u k=%u size=%zu)\n",
              arg->tid, k, arg->class_size);
      arg->failed = 1;
      return NULL;
    }
    memset(ptrs[k], 0xB6, arg->class_size);
  }

  /* Steady-state warm-up: cycle the whole working set a few times before
   * any timing starts, so the measured rounds see an already-warm active
   * page/route entry rather than the one-time cost of creating it. Set
   * WARMUP_ROUNDS=0 to deliberately measure a cold start instead. */
  for (uint32_t w = 0; w < arg->warmup_rounds; ++w) {
    for (uint32_t k = 0; k < arg->working_set; ++k) {
      hz10_bench_free(arg->use_hz10, ptrs[k]);
    }
    for (uint32_t k = 0; k < arg->working_set; ++k) {
      ptrs[k] = hz10_bench_alloc(arg->use_hz10, arg->class_size);
      if (!ptrs[k]) {
        fprintf(stderr, "local-path: warmup realloc failed (tid=%u)\n",
                arg->tid);
        arg->failed = 1;
        return NULL;
      }
    }
  }

  uint64_t elapsed_ns = 0u;
  for (uint64_t r = 0; r < arg->rounds; ++r) {
    switch (arg->mode) {
      case MODE_ALLOC_FREE: {
        uint64_t start = hz10_platform_now_ns();
        for (uint32_t k = 0; k < arg->working_set; ++k) {
          hz10_bench_free(arg->use_hz10, ptrs[k]);
        }
        for (uint32_t k = 0; k < arg->working_set; ++k) {
          ptrs[k] = hz10_bench_alloc(arg->use_hz10, arg->class_size);
        }
        elapsed_ns += hz10_platform_now_ns() - start;
        break;
      }
      case MODE_FREE_ONLY: {
        uint64_t start = hz10_platform_now_ns();
        for (uint32_t k = 0; k < arg->working_set; ++k) {
          hz10_bench_free(arg->use_hz10, ptrs[k]);
        }
        elapsed_ns += hz10_platform_now_ns() - start;
        for (uint32_t k = 0; k < arg->working_set; ++k) {
          ptrs[k] = hz10_bench_alloc(arg->use_hz10, arg->class_size);
        }
        break;
      }
      case MODE_ALLOC_ONLY: {
        for (uint32_t k = 0; k < arg->working_set; ++k) {
          hz10_bench_free(arg->use_hz10, ptrs[k]);
        }
        uint64_t start = hz10_platform_now_ns();
        for (uint32_t k = 0; k < arg->working_set; ++k) {
          ptrs[k] = hz10_bench_alloc(arg->use_hz10, arg->class_size);
        }
        elapsed_ns += hz10_platform_now_ns() - start;
        break;
      }
      default:
        break;
    }
    for (uint32_t k = 0; k < arg->working_set; ++k) {
      if (!ptrs[k]) {
        fprintf(stderr, "local-path: NULL ptr mid-run (tid=%u mode=%s)\n",
                arg->tid, mode_name(arg->mode));
        arg->failed = 1;
        free(ptrs);
        return NULL;
      }
    }
  }
  arg->elapsed_ns = elapsed_ns;

  for (uint32_t k = 0; k < arg->working_set; ++k) {
    hz10_bench_free(arg->use_hz10, ptrs[k]);
  }
  free(ptrs);
  return NULL;
}

/* Caps working_set so a class with few slots per page (e.g. class 20/21's
 * 2, or the slot_count=1 isolating class) never spans more pages than
 * HZ10_CLASS_PAGES_SCAN_LIMIT (128). Without this, a working set sized for
 * a small class (many slots/page, so a few pages easily hold thousands of
 * live items) would, for a small-slot_count class, create thousands of
 * DISTINCT pages -- eviction-on-add (src/hz10_class_pages.c) pushes most of
 * them into `retired` during setup, and the timed alloc phase then falls
 * through find_with_capacity()'s bounded active-list scan into harvest's
 * budgeted (4-per-call) retired sweep, which cannot keep up and starts
 * paying for genuinely fresh pages every few calls -- measuring known,
 * already-documented pool/scan-limit churn (current_task.md's "slot_count=1
 * isolation gap" and "class_pages bounded scan"), not steady-state warm
 * local-free cost, which is what this box wants to isolate. Capping at a
 * small multiple of the page target keeps every class's working set inside
 * one warm, uncontested active-list window. */
static uint32_t clamp_working_set_to_pages(size_t class_size,
                                          uint32_t working_set) {
  enum { PAGE_TARGET = 32u };
  uint32_t class_id = hz10_size_class_for(class_size);
  if (class_id >= HZ10_CLASS_COUNT) {
    return working_set;
  }
  uint32_t slot_count = hz10_size_class_slot_count(class_id);
  if (slot_count == 0u) {
    return working_set;
  }
  uint64_t max_working_set = (uint64_t)slot_count * PAGE_TARGET;
  if ((uint64_t)working_set > max_working_set) {
    return (uint32_t)max_working_set;
  }
  return working_set;
}

static int run_row(const char* mech, int use_hz10, uint32_t threads,
                   size_t class_size, uint32_t working_set, uint64_t rounds,
                   uint32_t warmup_rounds, LocalMode mode, uint32_t run,
                   uint32_t runs) {
  working_set = clamp_working_set_to_pages(class_size, working_set);
  pthread_t* ids = calloc(threads, sizeof(*ids));
  WorkerArg* args = calloc(threads, sizeof(*args));
  if (!ids || !args) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }
  for (uint32_t t = 0; t < threads; ++t) {
    args[t].tid = t;
    args[t].class_size = class_size;
    args[t].working_set = working_set;
    args[t].rounds = rounds;
    args[t].warmup_rounds = warmup_rounds;
    args[t].mode = mode;
    args[t].use_hz10 = use_hz10;
    if (pthread_create(&ids[t], NULL, worker_main, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }
  int failed = 0;
  uint64_t total_ns = 0u;
  for (uint32_t t = 0; t < threads; ++t) {
    pthread_join(ids[t], NULL);
    failed |= args[t].failed;
    total_ns += args[t].elapsed_ns;
  }
  free(args);
  free(ids);
  if (failed) {
    return 1;
  }

  uint64_t ops = (uint64_t)working_set * rounds * threads;
  /* total_ns sums each thread's own elapsed time; dividing by threads
   * gives the wall-clock-equivalent window ops/s within, matching how
   * bench/hz10_public_entry_bench.c reports ops_per_s (ops over wall
   * time, not over summed per-thread time). */
  double wall_ns = (double)total_ns / (double)threads;
  double seconds = wall_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf("hz10_public_entry_local_path mech=%s mode=%s run=%u/%u threads=%u "
        "class_size=%zu working_set=%u rounds=%llu warmup_rounds=%u "
        "ops=%llu seconds=%.6f ops_per_s=%.2f ns_per_op=%.4f\n",
        mech, mode_name(mode), run, runs, threads, class_size, working_set,
        (unsigned long long)rounds, warmup_rounds, (unsigned long long)ops,
        seconds, (double)ops / seconds, (double)total_ns / (double)ops);
  return 0;
}

int main(void) {
  uint32_t threads = (uint32_t)env_u64("THREADS", 4ull);
  uint32_t working_set = (uint32_t)env_u64("WORKING_SET", 4096ull);
  uint64_t rounds = env_u64("ROUNDS", 2000ull);
  uint32_t warmup_rounds = (uint32_t)env_u64("WARMUP_ROUNDS", 5ull);
  uint32_t runs = (uint32_t)env_u64("RUNS", 5ull);
  uint32_t mode_env = (uint32_t)env_u64("MODE", 3ull); /* 3 = all 3 modes */

  size_t default_sizes[] = {16u, 64u, 24576u, 32768u, 65536u};
  size_t class_size = (size_t)env_u64("CLASS_SIZE", 0ull);

  if (threads == 0u || working_set == 0u || rounds == 0u || runs == 0u) {
    fprintf(stderr, "invalid THREADS/WORKING_SET/ROUNDS/RUNS\n");
    return 1;
  }

  printf("hz10_public_entry_local_path sizeof(Hz10FreelistPage)=%zu "
        "offsetof_base=%zu offsetof_local_free_head=%zu "
        "offsetof_generation=%zu offsetof_owner_thread_token=%zu "
        "(cache-line question: owner_thread_token and the fields\n"
        "hz10_freelist_page_free()/local free route need are all inside "
        "the first %zu bytes -- same first cache line on a 64B-line "
        "machine unless the struct is misaligned)\n",
        sizeof(Hz10FreelistPage), offsetof(Hz10FreelistPage, base),
        offsetof(Hz10FreelistPage, local_free_head),
        offsetof(Hz10FreelistPage, generation),
        offsetof(Hz10FreelistPage, owner_thread_token),
        offsetof(Hz10FreelistPage, owner_thread_token) + sizeof(void*));

  size_t* sizes = &class_size;
  size_t size_count = 1;
  if (class_size == 0u) {
    sizes = default_sizes;
    size_count = sizeof(default_sizes) / sizeof(default_sizes[0]);
  }

  int any_failed = 0;
  for (size_t si = 0; si < size_count; ++si) {
    size_t sz = sizes[si];
    for (uint32_t run = 1u; run <= runs; ++run) {
      for (LocalMode m = 0; m < MODE_COUNT; ++m) {
        if (mode_env != 3u && (uint32_t)m != mode_env) {
          continue;
        }
        any_failed |= run_row("hz10", 1, threads, sz, working_set, rounds,
                             warmup_rounds, m, run, runs);
        any_failed |= run_row("system_malloc", 0, threads, sz, working_set,
                             rounds, warmup_rounds, m, run, runs);
      }
    }
  }
  return any_failed ? 1 : 0;
}
