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

static size_t h7_size_from_range(uint32_t value,
                                 size_t min_size,
                                 size_t max_size) {
  size_t span = max_size - min_size + 1u;
  return min_size + (size_t)(value % (uint32_t)span);
}

static uint32_t h7_source_family_for_size(size_t size) {
  return size <= 16384u ? 1u : 2u;
}

#include "bench_hz7_v3_ops.inc"
#include "bench_hz7_v3_rows.inc"

static void h7_run_v3_batch_focus_sequence(uint32_t iters) {
  h7_run_size_rows_malloc_batch_free(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_free_batch(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_malloc_free(kSpanAuditRows, kSpanAuditRowCount, iters);
  h7_run_size_rows_free_retained_loop(kSpanAuditRows, kSpanAuditRowCount, iters);

  h7_run_size_rows_malloc_batch_free(kDirectRetainRows,
                                     kDirectRetainRowCount,
                                     iters);
  h7_run_size_rows_free_batch(kDirectRetainRows, kDirectRetainRowCount, iters);
  h7_run_size_rows_malloc_free(kDirectRetainRows, kDirectRetainRowCount, iters);
  h7_run_size_rows_free_retained_loop(kDirectRetainRows,
                                      kDirectRetainRowCount,
                                      iters);
}

static void h7_batch_focus_keep_helpers(void) {
  void (*keep_ops[])(const char*, size_t, size_t, uint32_t, uint32_t) = {
      h7_run_mixed_steady,
  };
  void (*keep_toggle[])(const char*, size_t, size_t, uint32_t, uint32_t,
                        uint32_t) = {
      h7_run_random_toggle,
  };
  void (*keep_rows[])(const H7SizeRow*, size_t, uint32_t) = {
      h7_run_size_rows_route_valid,
      h7_run_size_rows_route_invalid,
      h7_run_size_rows_route_invariant,
  };
  (void)keep_ops;
  (void)keep_toggle;
  (void)keep_rows;
}

int main(int argc, char** argv) {
  uint32_t iters = 10000000u;
  if (argc > 1) {
    iters = (uint32_t)strtoul(argv[1], 0, 10);
  }

  h7_batch_focus_keep_helpers();
  h7_run_v3_batch_focus_sequence(iters);
  return 0;
}
