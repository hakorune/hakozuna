// Usage:
//   bench_hz5_midpage_direct [threads] [iters] [working_set] [min_size] [max_size] [pattern] [phase_len] [release_retired_at_end]
//
// pattern:
//   random: random size in [min_size, max_size] (default)
//   phase:  stay in one MidPage class for phase_len allocations, then rotate
//   cycle:  rotate MidPage classes every allocation

#include "hz5_midpagefront.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum Hz5MidPageDirectPattern {
  HZ5_MIDPAGE_DIRECT_RANDOM = 0,
  HZ5_MIDPAGE_DIRECT_PHASE = 1,
  HZ5_MIDPAGE_DIRECT_CYCLE = 2,
} Hz5MidPageDirectPattern;

typedef struct Hz5MidPageDirectRange {
  size_t min_size;
  size_t max_size;
} Hz5MidPageDirectRange;

typedef struct Hz5MidPageDirectArg {
  uint32_t seed;
  size_t iters;
  size_t working_set;
  size_t min_size;
  size_t max_size;
  size_t phase_len;
  size_t range_count;
  Hz5MidPageDirectRange ranges[8];
  Hz5MidPageDirectPattern pattern;
  int release_retired_at_end;
  uint64_t alloc_failures;
  uint64_t free_failures;
  uint64_t released_retired;
} Hz5MidPageDirectArg;

static const Hz5MidPageDirectRange k_hz5_midpage_direct_classes[] = {
    {2049u, 3072u},
    {3073u, 4096u},
    {4097u, 8192u},
    {8193u, 16384u},
    {16385u, 32768u},
};

static inline uint32_t hz5_midpage_direct_lcg(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static uint64_t hz5_midpage_direct_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static uint64_t hz5_midpage_direct_current_rss_kb(void) {
  FILE* f = fopen("/proc/self/statm", "r");
  if (!f) {
    return 0;
  }
  unsigned long resident_pages = 0;
  int scanned = fscanf(f, "%*s %lu", &resident_pages);
  fclose(f);
  if (scanned != 1) {
    return 0;
  }
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    return 0;
  }
  return (uint64_t)resident_pages * (uint64_t)page_size / UINT64_C(1024);
}

static Hz5MidPageDirectPattern hz5_midpage_direct_parse_pattern(
    const char* text) {
  if (!text || strcmp(text, "random") == 0) {
    return HZ5_MIDPAGE_DIRECT_RANDOM;
  }
  if (strcmp(text, "phase") == 0) {
    return HZ5_MIDPAGE_DIRECT_PHASE;
  }
  if (strcmp(text, "cycle") == 0) {
    return HZ5_MIDPAGE_DIRECT_CYCLE;
  }
  return HZ5_MIDPAGE_DIRECT_RANDOM;
}

static const char* hz5_midpage_direct_pattern_name(
    Hz5MidPageDirectPattern pattern) {
  switch (pattern) {
    case HZ5_MIDPAGE_DIRECT_PHASE:
      return "phase";
    case HZ5_MIDPAGE_DIRECT_CYCLE:
      return "cycle";
    case HZ5_MIDPAGE_DIRECT_RANDOM:
    default:
      return "random";
  }
}

static size_t hz5_midpage_direct_collect_ranges(
    size_t min_size,
    size_t max_size,
    Hz5MidPageDirectRange* out,
    size_t out_cap) {
  size_t count = 0;
  for (size_t i = 0;
       i < sizeof(k_hz5_midpage_direct_classes) /
               sizeof(k_hz5_midpage_direct_classes[0]);
       ++i) {
    size_t lo = k_hz5_midpage_direct_classes[i].min_size;
    size_t hi = k_hz5_midpage_direct_classes[i].max_size;
    if (lo < min_size) {
      lo = min_size;
    }
    if (hi > max_size) {
      hi = max_size;
    }
    if (lo <= hi && count < out_cap) {
      out[count].min_size = lo;
      out[count].max_size = hi;
      ++count;
    }
  }
  return count;
}

static size_t hz5_midpage_direct_next_size(Hz5MidPageDirectArg* ta,
                                           size_t iter,
                                           uint32_t* seed) {
  size_t span = (ta->max_size > ta->min_size)
                    ? (ta->max_size - ta->min_size + 1u)
                    : 1u;
  if (ta->pattern == HZ5_MIDPAGE_DIRECT_RANDOM) {
    return ta->min_size + (hz5_midpage_direct_lcg(seed) % span);
  }

  if (ta->range_count == 0u) {
    return ta->min_size + (hz5_midpage_direct_lcg(seed) % span);
  }

  size_t range_index = 0;
  if (ta->pattern == HZ5_MIDPAGE_DIRECT_PHASE) {
    size_t phase_len = ta->phase_len ? ta->phase_len : 4096u;
    range_index = (iter / phase_len) % ta->range_count;
  } else {
    range_index = iter % ta->range_count;
  }
  Hz5MidPageDirectRange range = ta->ranges[range_index];
  size_t range_span = (range.max_size > range.min_size)
                          ? (range.max_size - range.min_size + 1u)
                          : 1u;
  return range.min_size + (hz5_midpage_direct_lcg(seed) % range_span);
}

static void* hz5_midpage_direct_thread(void* arg) {
  Hz5MidPageDirectArg* ta = (Hz5MidPageDirectArg*)arg;
  void** slots = (void**)calloc(ta->working_set, sizeof(void*));
  if (!slots) {
    ta->alloc_failures += ta->iters;
    return NULL;
  }

  uint32_t seed = ta->seed;

  for (size_t i = 0; i < ta->iters; ++i) {
    size_t idx = (size_t)(hz5_midpage_direct_lcg(&seed) % ta->working_set);
    if (slots[idx]) {
      Hz5MidPageFrontFreeResult free_result =
          hz5_midpagefront_free(slots[idx]);
      if (free_result != HZ5_MIDPAGEFRONT_FREE_OK) {
        ++ta->free_failures;
      }
      slots[idx] = NULL;
    }

    size_t size = hz5_midpage_direct_next_size(ta, i, &seed);
    void* ptr = NULL;
    Hz5MidPageFrontAllocResult alloc_result =
        hz5_midpagefront_try_alloc(size, 16u, &ptr);
    if (alloc_result != HZ5_MIDPAGEFRONT_ALLOC_OK || !ptr) {
      ++ta->alloc_failures;
      continue;
    }
    slots[idx] = ptr;
  }

  for (size_t i = 0; i < ta->working_set; ++i) {
    if (!slots[i]) {
      continue;
    }
    Hz5MidPageFrontFreeResult free_result = hz5_midpagefront_free(slots[i]);
    if (free_result != HZ5_MIDPAGEFRONT_FREE_OK) {
      ++ta->free_failures;
    }
  }
  if (ta->release_retired_at_end) {
    ta->released_retired = hz5_midpagefront_release_retired();
  }
  free(slots);
  return NULL;
}

int main(int argc, char** argv) {
  size_t threads = 8;
  size_t iters = 300000;
  size_t working_set = 100;
  size_t min_size = 2049;
  size_t max_size = 32768;
  Hz5MidPageDirectPattern pattern = HZ5_MIDPAGE_DIRECT_RANDOM;
  size_t phase_len = 4096;
  int release_retired_at_end = 0;

  if (argc > 1) {
    threads = (size_t)strtoull(argv[1], NULL, 10);
  }
  if (argc > 2) {
    iters = (size_t)strtoull(argv[2], NULL, 10);
  }
  if (argc > 3) {
    working_set = (size_t)strtoull(argv[3], NULL, 10);
  }
  if (argc > 4) {
    min_size = (size_t)strtoull(argv[4], NULL, 10);
  }
  if (argc > 5) {
    max_size = (size_t)strtoull(argv[5], NULL, 10);
  }
  if (argc > 6) {
    pattern = hz5_midpage_direct_parse_pattern(argv[6]);
  }
  if (argc > 7) {
    phase_len = (size_t)strtoull(argv[7], NULL, 10);
  }
  if (argc > 8) {
    release_retired_at_end = atoi(argv[8]) != 0;
  }

  if (threads == 0) {
    threads = 1;
  }
  if (working_set == 0) {
    working_set = 1;
  }
  if (min_size == 0) {
    min_size = 1;
  }
  if (max_size < min_size) {
    max_size = min_size;
  }
  Hz5MidPageDirectRange ranges[8];
  size_t range_count = hz5_midpage_direct_collect_ranges(
      min_size, max_size, ranges, sizeof(ranges) / sizeof(ranges[0]));

  pthread_t* tids = (pthread_t*)calloc(threads, sizeof(pthread_t));
  Hz5MidPageDirectArg* args =
      (Hz5MidPageDirectArg*)calloc(threads, sizeof(Hz5MidPageDirectArg));
  if (!tids || !args) {
    fprintf(stderr, "bench_hz5_midpage_direct: allocation failure\n");
    free(tids);
    free(args);
    return 1;
  }

  uint64_t start = hz5_midpage_direct_now_ns();
  for (size_t i = 0; i < threads; ++i) {
    args[i].seed = (uint32_t)(0xC0FFEEu + (uint32_t)i * 17u);
    args[i].iters = iters;
    args[i].working_set = working_set;
    args[i].min_size = min_size;
    args[i].max_size = max_size;
    args[i].pattern = pattern;
    args[i].release_retired_at_end = release_retired_at_end;
    args[i].phase_len = phase_len;
    args[i].range_count = range_count;
    for (size_t j = 0; j < range_count; ++j) {
      args[i].ranges[j] = ranges[j];
    }
    if (pthread_create(&tids[i], NULL, hz5_midpage_direct_thread, &args[i]) !=
        0) {
      fprintf(stderr, "bench_hz5_midpage_direct: pthread_create failed\n");
      free(tids);
      free(args);
      return 1;
    }
  }

  uint64_t alloc_failures = 0;
  uint64_t free_failures = 0;
  uint64_t released_retired = 0;
  for (size_t i = 0; i < threads; ++i) {
    pthread_join(tids[i], NULL);
    alloc_failures += args[i].alloc_failures;
    free_failures += args[i].free_failures;
    released_retired += args[i].released_retired;
  }
  uint64_t end = hz5_midpage_direct_now_ns();
  uint64_t current_rss_kb = hz5_midpage_direct_current_rss_kb();

  double sec = (double)(end - start) / 1000000000.0;
  double total_ops = (double)threads * (double)iters;
  double ops_per_s = sec > 0.0 ? total_ops / sec : 0.0;

  printf("[BENCH_HZ5_MIDPAGE_DIRECT] threads=%zu iters=%zu ws=%zu "
         "min_size=%zu max_size=%zu pattern=%s phase_len=%zu "
         "release_retired_at_end=%d\n",
         threads,
         iters,
         working_set,
         min_size,
         max_size,
         hz5_midpage_direct_pattern_name(pattern),
         phase_len,
         release_retired_at_end);
  printf("bench_hz5_midpage_direct: threads=%zu ops=%.0f time=%.6f "
         "ops/s=%.2f\n",
         threads,
         total_ops,
         sec,
         ops_per_s);
  printf("[STATS] alloc_failures=%llu free_failures=%llu "
         "released_retired=%llu current_rss_kb=%llu\n",
         (unsigned long long)alloc_failures,
         (unsigned long long)free_failures,
         (unsigned long long)released_retired,
         (unsigned long long)current_rss_kb);

  free(tids);
  free(args);
  return (alloc_failures == 0 && free_failures == 0) ? 0 : 2;
}
