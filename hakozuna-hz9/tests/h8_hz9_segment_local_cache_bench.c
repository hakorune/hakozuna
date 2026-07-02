#include "../src/h8_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if !defined(H9_SEGMENT_LOCAL_CACHE_L0)
#error "segment local cache bench requires H9_SEGMENT_LOCAL_CACHE_L0"
#endif

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !*value) {
    return fallback;
  }
  char* end = NULL;
  unsigned long long parsed = strtoull(value, &end, 10);
  return end && *end == '\0' ? (uint64_t)parsed : fallback;
}

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

typedef enum SegmentBenchMode {
  SEGMENT_BENCH_BITS = 0,
  SEGMENT_BENCH_BOUND_ADDR = 1,
  SEGMENT_BENCH_KNOWN_ADDR = 2,
  SEGMENT_BENCH_TABLE_ADDR = 3,
  SEGMENT_BENCH_ACTIVE_ADDR = 4,
  SEGMENT_BENCH_FAST_ADDR = 5,
  SEGMENT_BENCH_SLOT_ADDR = 6,
  SEGMENT_BENCH_CYCLE_KNOWN = 7,
  SEGMENT_BENCH_TABLE_SLOT_ADDR = 8
} SegmentBenchMode;

int main(void) {
  h8_init();
  uint32_t class_id = (uint32_t)env_u64("CLASS_ID", 5u);
  uint64_t iters = env_u64("ITERS", 10000000u);
  SegmentBenchMode mode = (SegmentBenchMode)env_u64("MODE", 0u);
  if (env_u64("BOUND_ADDR", 0u) != 0u) {
    mode = SEGMENT_BENCH_BOUND_ADDR;
  }
  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  size_t payload_bytes = 0u;
  size_t slack_bytes = 0u;
  if (!h9_segment_local_cache_debug_class_geometry(
          class_id, &slot_size, &run_size, &slot_count) ||
      !h9_segment_local_cache_debug_class_capacity(
          class_id, &payload_bytes, &slack_bytes)) {
    fprintf(stderr, "segment bench invalid class %u\n", class_id);
    return 1;
  }
  bool uses_base = mode == SEGMENT_BENCH_BOUND_ADDR ||
                   mode == SEGMENT_BENCH_KNOWN_ADDR ||
                   mode == SEGMENT_BENCH_TABLE_ADDR ||
                   mode == SEGMENT_BENCH_ACTIVE_ADDR ||
                   mode == SEGMENT_BENCH_FAST_ADDR ||
                   mode == SEGMENT_BENCH_SLOT_ADDR ||
                   mode == SEGMENT_BENCH_CYCLE_KNOWN ||
                   mode == SEGMENT_BENCH_TABLE_SLOT_ADDR;
  uintptr_t base = (uintptr_t)0x40000000u;
  if (uses_base &&
      !h9_segment_local_cache_debug_bind_base(class_id,
                                              base)) {
    fprintf(stderr, "segment bench bind failed for class %u\n", class_id);
    return 2;
  }
  if (!h9_segment_local_cache_debug_put(class_id, 0u)) {
    fprintf(stderr, "segment bench setup failed for class %u\n", class_id);
    return 3;
  }
  if (mode == SEGMENT_BENCH_ACTIVE_ADDR &&
      !h9_segment_local_cache_debug_set_active_class(class_id)) {
    fprintf(stderr, "segment bench active setup failed for class %u\n",
            class_id);
    return 4;
  }

  uint64_t ok = 0u;
  uint32_t slot = 0u;
  uintptr_t addr = 0u;
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    bool success = false;
    if (mode == SEGMENT_BENCH_BOUND_ADDR) {
      success = h9_segment_local_cache_debug_take_addr(class_id, &addr) &&
                h9_segment_local_cache_debug_free_addr(class_id, addr);
    } else if (mode == SEGMENT_BENCH_FAST_ADDR) {
      success = h9_segment_local_cache_debug_take_addr(class_id, &addr) &&
                h9_segment_local_cache_debug_free_addr_fast(class_id, addr);
    } else if (mode == SEGMENT_BENCH_SLOT_ADDR) {
      success = h9_segment_local_cache_debug_take_slot_addr(class_id, &slot,
                                                            &addr);
      success = success &&
                addr == base + (uintptr_t)slot * (uintptr_t)slot_size &&
                h9_segment_local_cache_debug_free_allocated(class_id, slot);
    } else if (mode == SEGMENT_BENCH_CYCLE_KNOWN) {
      success = h9_segment_local_cache_debug_cycle_known(class_id, &addr);
      success = success && addr >= base;
    } else if (mode == SEGMENT_BENCH_KNOWN_ADDR) {
      success = h9_segment_local_cache_debug_take(class_id, &slot);
      addr = base + (uintptr_t)slot * (uintptr_t)slot_size;
      success = success &&
                addr >= base &&
                h9_segment_local_cache_debug_free_allocated(class_id, slot);
    } else if (mode == SEGMENT_BENCH_TABLE_ADDR) {
      success = h9_segment_local_cache_debug_take(class_id, &slot);
      addr = base + (uintptr_t)slot * (uintptr_t)slot_size;
      uint32_t routed_class = UINT32_MAX;
      success =
          success &&
          h9_segment_local_cache_debug_route_table_addr(addr, &routed_class) ==
              H8_ROUTE_VALID &&
          routed_class == class_id &&
          h9_segment_local_cache_debug_free_allocated(class_id, slot);
    } else if (mode == SEGMENT_BENCH_TABLE_SLOT_ADDR) {
      success = h9_segment_local_cache_debug_take(class_id, &slot);
      addr = base + (uintptr_t)slot * (uintptr_t)slot_size;
      uint32_t routed_class = UINT32_MAX;
      uint32_t routed_slot = UINT32_MAX;
      success =
          success &&
          h9_segment_local_cache_debug_route_table_slot_addr(
              addr, &routed_class, &routed_slot) == H8_ROUTE_VALID &&
          routed_class == class_id && routed_slot == slot &&
          h9_segment_local_cache_debug_free_allocated(routed_class,
                                                      routed_slot);
    } else if (mode == SEGMENT_BENCH_ACTIVE_ADDR) {
      success = h9_segment_local_cache_debug_active_take_addr(&addr) &&
                h9_segment_local_cache_debug_active_free_addr(addr);
    } else {
      success = h9_segment_local_cache_debug_take(class_id, &slot) &&
                h9_segment_local_cache_debug_free_allocated(class_id, slot);
    }
    if (!success) {
      fprintf(stderr, "segment bench transition failed at iter %llu\n",
              (unsigned long long)i);
      return 4;
    }
    ++ok;
  }
  double elapsed = now_seconds() - start;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  double cycles = (double)ok / elapsed;
  const char* mode_name = "bits";
  if (mode == SEGMENT_BENCH_BOUND_ADDR) {
    mode_name = "bound_addr";
  } else if (mode == SEGMENT_BENCH_FAST_ADDR) {
    mode_name = "fast_addr";
  } else if (mode == SEGMENT_BENCH_SLOT_ADDR) {
    mode_name = "slot_addr";
  } else if (mode == SEGMENT_BENCH_CYCLE_KNOWN) {
    mode_name = "cycle_known";
  } else if (mode == SEGMENT_BENCH_KNOWN_ADDR) {
    mode_name = "known_addr";
  } else if (mode == SEGMENT_BENCH_TABLE_ADDR) {
    mode_name = "table_addr";
  } else if (mode == SEGMENT_BENCH_TABLE_SLOT_ADDR) {
    mode_name = "table_slot_addr";
  } else if (mode == SEGMENT_BENCH_ACTIVE_ADDR) {
    mode_name = "active_addr";
  }
  printf("hz9_segment_local_cache_api mode=%s class=%u slot_size=%u run_size=%u "
         "slot_count=%u payload_bytes=%zu slack_bytes=%zu iters=%llu "
         "seconds=%.6f cycles_per_s=%.2f ops_per_s=%.2f\n",
         mode_name, class_id, slot_size, run_size, (unsigned)slot_count,
         payload_bytes, slack_bytes, (unsigned long long)ok, elapsed, cycles,
         cycles * 2.0);
  return 0;
}
