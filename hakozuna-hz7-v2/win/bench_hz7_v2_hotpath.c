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

  return 0;
}
