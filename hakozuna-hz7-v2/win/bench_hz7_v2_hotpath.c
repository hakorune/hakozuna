#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../hz7.h"

static double h7_seconds(void) {
  LARGE_INTEGER freq;
  LARGE_INTEGER now;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&now);
  return (double)now.QuadPart / (double)freq.QuadPart;
}

static size_t h7_peak_working_set_kb(void) {
  PROCESS_MEMORY_COUNTERS_EX pmc;
  memset(&pmc, 0, sizeof(pmc));
  pmc.cb = sizeof(pmc);
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           (PROCESS_MEMORY_COUNTERS*)&pmc,
                           sizeof(pmc))) {
    return (size_t)(pmc.PeakWorkingSetSize / 1024u);
  }
  return 0;
}

static void h7_touch(void* ptr, size_t size, uint32_t value) {
  unsigned char* bytes = (unsigned char*)ptr;
  if (bytes && size > 0) {
    bytes[0] = (unsigned char)value;
    bytes[size - 1u] = (unsigned char)(value >> 8u);
  }
}

static uint32_t h7_batch_iters_for_size(size_t size, uint32_t requested) {
  uint32_t cap = 262144u;
  if (size >= 32768u) {
    cap = 4096u;
  } else if (size >= 8192u) {
    cap = 8192u;
  }
  return requested < cap ? requested : cap;
}

static uint32_t h7_rng_next(uint32_t* state) {
  *state = *state * 1664525u + 1013904223u;
  return *state;
}

static size_t h7_size_from_range(uint32_t value, size_t min_size, size_t max_size) {
  size_t span = max_size - min_size + 1u;
  return min_size + (size_t)(value % (uint32_t)span);
}

static uint32_t h7_source_family_for_size(size_t size) {
  return size <= 16384u ? 1u : 2u;
}

static void h7_run_malloc_free(const char* label,
                               size_t size,
                               uint32_t iters) {
  uint32_t i;
  uint32_t ok = 0;
  double start = h7_seconds();
  double elapsed;
  for (i = 0; i < iters; ++i) {
    void* ptr = h7_malloc(size);
    if (ptr) {
      h7_touch(ptr, size, i);
      h7_free(ptr);
      ++ok;
    }
  }
  elapsed = h7_seconds() - start;
  printf("hz7_hotpath: op=malloc_free label=%s size=%zu iters=%u ok=%u "
         "time=%.6f pairs/s=%.2f peak_kb=%zu\n",
         label,
         size,
         iters,
         ok,
         elapsed,
         elapsed > 0.0 ? (double)ok / elapsed : 0.0,
         h7_peak_working_set_kb());
}

static void h7_run_route_valid(const char* label,
                               size_t size,
                               uint32_t iters) {
  uint32_t i;
  uint32_t valid = 0;
  void* ptr = h7_malloc(size);
  double start;
  double elapsed;
  if (!ptr) {
    printf("hz7_hotpath: op=route_valid label=%s size=%zu alloc_failed=1\n",
           label,
           size);
    return;
  }
  start = h7_seconds();
  for (i = 0; i < iters; ++i) {
    valid += h7_route(ptr) == H7_ROUTE_VALID;
  }
  elapsed = h7_seconds() - start;
  h7_free(ptr);
  printf("hz7_hotpath: op=route_valid label=%s size=%zu iters=%u ok=%u "
         "time=%.6f ops/s=%.2f peak_kb=%zu\n",
         label,
         size,
         iters,
         valid,
         elapsed,
         elapsed > 0.0 ? (double)iters / elapsed : 0.0,
         h7_peak_working_set_kb());
}

static void h7_run_route_invalid(const char* label,
                                 size_t size,
                                 uint32_t iters) {
  uint32_t i;
  uint32_t invalid = 0;
  void* ptr = h7_malloc(size);
  double start;
  double elapsed;
  if (!ptr) {
    printf("hz7_hotpath: op=route_invalid label=%s size=%zu alloc_failed=1\n",
           label,
           size);
    return;
  }
  h7_free(ptr);
  start = h7_seconds();
  for (i = 0; i < iters; ++i) {
    invalid += h7_route(ptr) == H7_ROUTE_INVALID;
  }
  elapsed = h7_seconds() - start;
  printf("hz7_hotpath: op=route_invalid label=%s size=%zu iters=%u ok=%u "
         "time=%.6f ops/s=%.2f peak_kb=%zu\n",
         label,
         size,
         iters,
         invalid,
         elapsed,
         elapsed > 0.0 ? (double)iters / elapsed : 0.0,
         h7_peak_working_set_kb());
}

static void h7_run_malloc_batch_free(const char* label,
                                     size_t size,
                                     uint32_t iters) {
  uint32_t batch_iters = h7_batch_iters_for_size(size, iters);
  uint32_t i;
  uint32_t ok = 0;
  void** ptrs = (void**)calloc(batch_iters, sizeof(void*));
  double start;
  double elapsed;
  if (!ptrs) {
    printf("hz7_hotpath: op=malloc_batch label=%s size=%zu alloc_failed=1\n",
           label,
           size);
    return;
  }
  start = h7_seconds();
  for (i = 0; i < batch_iters; ++i) {
    ptrs[i] = h7_malloc(size);
    if (ptrs[i]) {
      h7_touch(ptrs[i], size, i);
      ++ok;
    }
  }
  elapsed = h7_seconds() - start;
  for (i = 0; i < batch_iters; ++i) {
    h7_free(ptrs[i]);
  }
  free(ptrs);
  printf("hz7_hotpath: op=malloc_batch label=%s size=%zu iters=%u ok=%u "
         "time=%.6f ops/s=%.2f peak_kb=%zu\n",
         label,
         size,
         batch_iters,
         ok,
         elapsed,
         elapsed > 0.0 ? (double)ok / elapsed : 0.0,
         h7_peak_working_set_kb());
}

static void h7_run_free_batch(const char* label, size_t size, uint32_t iters) {
  uint32_t batch_iters = h7_batch_iters_for_size(size, iters);
  uint32_t i;
  uint32_t ok = 0;
  void** ptrs = (void**)calloc(batch_iters, sizeof(void*));
  double start;
  double elapsed;
  if (!ptrs) {
    printf("hz7_hotpath: op=free_batch label=%s size=%zu alloc_failed=1\n",
           label,
           size);
    return;
  }
  for (i = 0; i < batch_iters; ++i) {
    ptrs[i] = h7_malloc(size);
    if (ptrs[i]) {
      h7_touch(ptrs[i], size, i);
    }
  }
  start = h7_seconds();
  for (i = 0; i < batch_iters; ++i) {
    if (ptrs[i]) {
      h7_free(ptrs[i]);
      ++ok;
    }
  }
  elapsed = h7_seconds() - start;
  free(ptrs);
  printf("hz7_hotpath: op=free_batch label=%s size=%zu iters=%u ok=%u "
         "time=%.6f ops/s=%.2f peak_kb=%zu\n",
         label,
         size,
         batch_iters,
         ok,
         elapsed,
         elapsed > 0.0 ? (double)ok / elapsed : 0.0,
         h7_peak_working_set_kb());
}

static void h7_run_free_retained_loop(const char* label,
                                      size_t size,
                                      uint32_t iters) {
  uint32_t i;
  uint32_t ok = 0;
  void* ptr = h7_malloc(size);
  double start;
  double elapsed;
  if (!ptr) {
    printf("hz7_hotpath: op=free_retained_loop label=%s size=%zu "
           "alloc_failed=1\n",
           label,
           size);
    return;
  }
  h7_free(ptr);
  start = h7_seconds();
  for (i = 0; i < iters; ++i) {
    ptr = h7_malloc(size);
    if (ptr) {
      h7_free(ptr);
      ++ok;
    }
  }
  elapsed = h7_seconds() - start;
  printf("hz7_hotpath: op=free_retained_loop label=%s size=%zu iters=%u ok=%u "
         "time=%.6f pairs/s=%.2f peak_kb=%zu\n",
         label,
         size,
         iters,
         ok,
         elapsed,
         elapsed > 0.0 ? (double)ok / elapsed : 0.0,
         h7_peak_working_set_kb());
}

static void h7_run_mixed_steady(const char* label,
                                size_t min_size,
                                size_t max_size,
                                uint32_t iters,
                                uint32_t live_count) {
  uint32_t i;
  uint32_t rng = 0xC0FFEE11u;
  uint32_t ok = 0;
  uint32_t switches = 0;
  void** ptrs = (void**)calloc(live_count, sizeof(void*));
  size_t* sizes = (size_t*)calloc(live_count, sizeof(size_t));
  double start;
  double elapsed;
  if (!ptrs || !sizes || live_count == 0) {
    printf("hz7_hotpath: op=mixed_steady label=%s size=0 alloc_failed=1\n",
           label);
    free(ptrs);
    free(sizes);
    return;
  }
  for (i = 0; i < live_count; ++i) {
    sizes[i] = h7_size_from_range(h7_rng_next(&rng), min_size, max_size);
    ptrs[i] = h7_malloc(sizes[i]);
    if (ptrs[i]) {
      h7_touch(ptrs[i], sizes[i], i);
    }
  }
  start = h7_seconds();
  for (i = 0; i < iters; ++i) {
    uint32_t slot = h7_rng_next(&rng) % live_count;
    size_t size = h7_size_from_range(h7_rng_next(&rng), min_size, max_size);
    switches += h7_source_family_for_size(sizes[slot]) !=
                h7_source_family_for_size(size);
    h7_free(ptrs[slot]);
    ptrs[slot] = h7_malloc(size);
    sizes[slot] = size;
    if (ptrs[slot]) {
      h7_touch(ptrs[slot], size, i);
      ++ok;
    }
  }
  elapsed = h7_seconds() - start;
  for (i = 0; i < live_count; ++i) {
    h7_free(ptrs[i]);
  }
  free(ptrs);
  free(sizes);
  printf("hz7_hotpath: op=mixed_steady label=%s size=0 iters=%u ok=%u "
         "time=%.6f ops/s=%.2f switches=%u switch_rate=%.6f peak_kb=%zu\n",
         label,
         iters,
         ok,
         elapsed,
         elapsed > 0.0 ? (double)ok / elapsed : 0.0,
         switches,
         iters > 0 ? (double)switches / (double)iters : 0.0,
         h7_peak_working_set_kb());
}

int main(int argc, char** argv) {
  uint32_t iters = 10000000u;
  if (argc > 1) {
    iters = (uint32_t)strtoul(argv[1], 0, 10);
  }

  h7_run_malloc_free("small64", 64u, iters);
  h7_run_malloc_free("span8k", 8192u, iters);
  h7_run_malloc_free("direct32k", 32768u, iters);

  h7_run_route_valid("small64", 64u, iters);
  h7_run_route_valid("span8k", 8192u, iters);
  h7_run_route_valid("direct32k", 32768u, iters);

  h7_run_route_invalid("small64", 64u, iters);
  h7_run_route_invalid("span8k", 8192u, iters);
  h7_run_route_invalid("direct32k", 32768u, iters);

  h7_run_malloc_batch_free("small64", 64u, iters);
  h7_run_malloc_batch_free("span8k", 8192u, iters);
  h7_run_malloc_batch_free("direct32k", 32768u, iters);

  h7_run_free_batch("small64", 64u, iters);
  h7_run_free_batch("span8k", 8192u, iters);
  h7_run_free_batch("direct32k", 32768u, iters);

  h7_run_free_retained_loop("small64", 64u, iters);
  h7_run_free_retained_loop("span8k", 8192u, iters);
  h7_run_free_retained_loop("direct32k", 32768u, iters);

  h7_run_mixed_steady("small_ws400", 16u, 2048u, iters, 400u);
  h7_run_mixed_steady("span_medium_ws400", 4096u, 16384u, iters, 400u);
  h7_run_mixed_steady("direct_medium_ws400", 16385u, 32768u, iters, 400u);
  h7_run_mixed_steady("medium_ws400", 4096u, 32768u, iters, 400u);
  h7_run_mixed_steady("mixed_ws400", 16u, 32768u, iters, 400u);

  return 0;
}
