#include "../src/h8_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if !defined(H9_SEGMENT_LOCAL_CACHE_L0)
#error "segment local cache local bench requires H9_SEGMENT_LOCAL_CACHE_L0"
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

int main(void) {
  h8_init();
  uint32_t class_id = (uint32_t)env_u64("CLASS_ID", 5u);
  uint64_t iters = env_u64("ITERS", 10000000u);
  bool touch = env_u64("TOUCH", 1u) != 0u;
  bool route_free = env_u64("ROUTE_FREE", 0u) != 0u;

  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  size_t payload_bytes = 0u;
  size_t slack_bytes = 0u;
  if (!h9_segment_local_cache_debug_class_geometry(
          class_id, &slot_size, &run_size, &slot_count) ||
      !h9_segment_local_cache_debug_class_capacity(
          class_id, &payload_bytes, &slack_bytes)) {
    fprintf(stderr, "segment local bench invalid class %u\n", class_id);
    return 1;
  }

  void* payload = h8_platform_reserve_rw(run_size);
  if (!payload) {
    fprintf(stderr, "segment local bench payload allocation failed\n");
    return 2;
  }
  h9_segment_local_cache_debug_reset();
  if (!h9_segment_local_cache_debug_bind_base(class_id, (uintptr_t)payload)) {
    fprintf(stderr, "segment local bench bind failed\n");
    h8_platform_release(payload, run_size);
    return 3;
  }
  for (uint32_t slot = 0; slot < slot_count; ++slot) {
    if (!h9_segment_local_cache_debug_put(class_id, slot)) {
      fprintf(stderr, "segment local bench put failed at slot %u\n", slot);
      h8_platform_release(payload, run_size);
      return 4;
    }
  }

  uint64_t ok = 0u;
  uint32_t slot = 0u;
  uintptr_t addr = 0u;
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    bool success = false;
    if (route_free) {
      uint32_t routed_class = UINT32_MAX;
      success = h9_segment_local_cache_debug_take_slot_addr(class_id, &slot,
                                                            &addr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)addr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success =
          success &&
          h9_segment_local_cache_debug_route_table_addr(addr, &routed_class) ==
              H8_ROUTE_VALID &&
          routed_class == class_id &&
          h9_segment_local_cache_debug_free_addr_fast(routed_class, addr);
    } else {
      success = h9_segment_local_cache_debug_cycle_known(class_id, &addr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)addr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
    }
    if (!success) {
      fprintf(stderr, "segment local bench cycle failed at iter %llu\n",
              (unsigned long long)i);
      h8_platform_release(payload, run_size);
      return 5;
    }
    ++ok;
  }
  double elapsed = now_seconds() - start;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  double cycles = (double)ok / elapsed;
  printf("hz9_segment_local_cache_local class=%u slot_size=%u run_size=%u "
         "slot_count=%u payload_bytes=%zu slack_bytes=%zu touch=%u "
         "route_free=%u iters=%llu seconds=%.6f cycles_per_s=%.2f "
         "ops_per_s=%.2f\n",
         class_id, slot_size, run_size, (unsigned)slot_count, payload_bytes,
         slack_bytes, touch ? 1u : 0u, route_free ? 1u : 0u,
         (unsigned long long)ok, elapsed, cycles, cycles * 2.0);

  h8_platform_release(payload, run_size);
  return 0;
}
