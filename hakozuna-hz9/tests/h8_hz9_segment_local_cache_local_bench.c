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

typedef struct H9SegmentBenchHeader {
  uint32_t magic;
  uint16_t class_id;
  uint16_t slot;
  uintptr_t base;
} H9SegmentBenchHeader;

#define H9_SEGMENT_BENCH_HEADER_MAGIC 0x48395348u

static bool header_route(uintptr_t user_addr, uint32_t* class_out,
                         uint32_t* slot_out) {
  H9SegmentBenchHeader* header =
      (H9SegmentBenchHeader*)(user_addr - sizeof(H9SegmentBenchHeader));
  if (header->magic != H9_SEGMENT_BENCH_HEADER_MAGIC) {
    return false;
  }
  if (class_out) {
    *class_out = header->class_id;
  }
  if (slot_out) {
    *slot_out = header->slot;
  }
  return true;
}

int main(void) {
  h8_init();
  uint32_t class_id = (uint32_t)env_u64("CLASS_ID", 5u);
  uint64_t iters = env_u64("ITERS", 10000000u);
  bool touch = env_u64("TOUCH", 1u) != 0u;
  uint64_t route_free = env_u64("ROUTE_FREE", 0u);
  bool active_cycle = env_u64("ACTIVE_CYCLE", 0u) != 0u;
  uint64_t active_route = env_u64("ACTIVE_ROUTE", 0u);
  uint64_t route_proof_interval = env_u64("ROUTE_PROOF_INTERVAL", 64u);

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
    if (slot_size > sizeof(H9SegmentBenchHeader)) {
      H9SegmentBenchHeader* header =
          (H9SegmentBenchHeader*)((uintptr_t)payload +
                                  (uintptr_t)slot * (uintptr_t)slot_size);
      header->magic = H9_SEGMENT_BENCH_HEADER_MAGIC;
      header->class_id = (uint16_t)class_id;
      header->slot = (uint16_t)slot;
      header->base = (uintptr_t)payload;
    }
  }
  if ((active_cycle || active_route != 0u) &&
      !h9_segment_local_cache_debug_set_active_class(class_id)) {
    fprintf(stderr, "segment local bench active setup failed\n");
    h8_platform_release(payload, run_size);
    return 6;
  }

  uint64_t ok = 0u;
  uint32_t slot = 0u;
  uintptr_t addr = 0u;
  uintptr_t last_addr = 0u;
  uint32_t last_class = UINT32_MAX;
  uint32_t last_slot = UINT32_MAX;
  bool last_valid = false;
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    bool success = false;
    if (active_route != 0u) {
      if (active_route == 11u) {
        success = h9_segment_local_cache_debug_active_pair_direct(&addr);
        if (success && touch) {
          volatile unsigned char* p = (volatile unsigned char*)addr;
          p[0] = (unsigned char)i;
          p[slot_size - 1u] = (unsigned char)(i >> 8);
        }
        if (!success) {
          fprintf(stderr, "segment local bench cycle failed at iter %llu\n",
                  (unsigned long long)i);
          h8_platform_release(payload, run_size);
          return 5;
        }
        ++ok;
        continue;
      }
      uint32_t routed_class = UINT32_MAX;
      uint32_t routed_slot = UINT32_MAX;
      success = h9_segment_local_cache_debug_active_take_direct(&slot, &addr);
      if (success) {
        last_addr = addr;
        last_class = class_id;
        last_slot = slot;
        last_valid = true;
      }
      if (success && touch && active_route != 7u) {
        volatile unsigned char* p = (volatile unsigned char*)addr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      H8RouteKind route_kind = H8_ROUTE_MISS;
      if (active_route == 4u) {
        route_kind =
            h9_segment_local_cache_debug_route_active_range_addr(
                addr, &routed_class);
        routed_slot = slot;
      } else if (active_route == 5u) {
        route_kind = h9_segment_local_cache_debug_route_active_slot_only_addr(
            addr, &routed_class, &routed_slot);
      } else if (active_route == 6u) {
        if (route_proof_interval != 0u &&
            (i % route_proof_interval) != 0u) {
          route_kind = H8_ROUTE_VALID;
          routed_class = class_id;
          routed_slot = slot;
        } else {
          route_kind = h9_segment_local_cache_debug_route_table_slot_addr(
              addr, &routed_class, &routed_slot);
        }
      } else if (active_route == 7u) {
        if (slot_size <= sizeof(H9SegmentBenchHeader)) {
          success = false;
        } else {
          uintptr_t user_addr = addr + sizeof(H9SegmentBenchHeader);
          if (touch) {
            volatile unsigned char* p = (volatile unsigned char*)user_addr;
            p[0] = (unsigned char)i;
            p[slot_size - sizeof(H9SegmentBenchHeader) - 1u] =
                (unsigned char)(i >> 8);
          }
          route_kind = header_route(user_addr, &routed_class, &routed_slot)
                           ? H8_ROUTE_VALID
                           : H8_ROUTE_INVALID;
        }
      } else if (active_route == 8u) {
        if (last_valid && addr == last_addr) {
          route_kind = H8_ROUTE_VALID;
          routed_class = last_class;
          routed_slot = last_slot;
          last_valid = false;
        } else {
          route_kind = h9_segment_local_cache_debug_route_table_slot_addr(
              addr, &routed_class, &routed_slot);
        }
      } else if (active_route == 9u) {
        route_kind = H8_ROUTE_VALID;
        routed_class = class_id;
        routed_slot = slot;
      } else if (active_route == 10u) {
        route_kind = H8_ROUTE_VALID;
        routed_class = class_id;
        routed_slot = slot;
      } else if (active_route >= 2u) {
        route_kind = h9_segment_local_cache_debug_route_active_slot_addr(
            addr, &routed_class, &routed_slot);
      } else {
        route_kind = h9_segment_local_cache_debug_route_table_slot_addr(
            addr, &routed_class, &routed_slot);
      }
      success = success && route_kind == H8_ROUTE_VALID &&
                routed_class == class_id && routed_slot == slot;
      if (active_route == 10u) {
        success =
            success && h9_segment_local_cache_debug_active_free_direct(slot);
      } else if (active_route >= 3u) {
        success =
            success &&
            h9_segment_local_cache_debug_free_allocated(class_id, slot);
      } else {
        success =
            success &&
            h9_segment_local_cache_debug_free_allocated(routed_class,
                                                        routed_slot);
      }
    } else if (active_cycle) {
      success = h9_segment_local_cache_debug_active_cycle_known(&addr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)addr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
    } else if (route_free == 1u) {
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
    } else if (route_free == 2u) {
      uint32_t routed_class = UINT32_MAX;
      uint32_t routed_slot = UINT32_MAX;
      success = h9_segment_local_cache_debug_take_slot_addr(class_id, &slot,
                                                            &addr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)addr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success =
          success &&
          h9_segment_local_cache_debug_route_table_slot_addr(
              addr, &routed_class, &routed_slot) == H8_ROUTE_VALID &&
          routed_class == class_id && routed_slot == slot &&
          h9_segment_local_cache_debug_free_allocated(routed_class,
                                                      routed_slot);
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
         "route_free=%u active_cycle=%u active_route=%u route_interval=%llu "
         "iters=%llu "
         "seconds=%.6f cycles_per_s=%.2f ops_per_s=%.2f\n",
         class_id, slot_size, run_size, (unsigned)slot_count, payload_bytes,
         slack_bytes, touch ? 1u : 0u, (unsigned)route_free,
         active_cycle ? 1u : 0u, (unsigned)active_route,
         (unsigned long long)route_proof_interval,
         (unsigned long long)ok, elapsed, cycles, cycles * 2.0);

  h8_platform_release(payload, run_size);
  return 0;
}
