#include "../src/h8_internal.h"

#include <stdio.h>

#if !defined(H9_SEGMENT_LOCAL_CACHE_L0)
#error "segment local cache smoke requires H9_SEGMENT_LOCAL_CACHE_L0"
#endif

static int check_local_flow(uint32_t class_id) {
  h9_segment_local_cache_debug_reset();
  if (h9_segment_local_cache_debug_state_size() == 0u) {
    fprintf(stderr, "segment state has zero size\n");
    return 1;
  }
  if (h9_segment_local_cache_debug_state(class_id) != 0u) {
    fprintf(stderr, "segment did not start LOCAL\n");
    return 2;
  }
  if (!h9_segment_local_cache_debug_put(class_id, 0u) ||
      !h9_segment_local_cache_debug_put(class_id, 1u) ||
      h9_segment_local_cache_debug_put(class_id, 1u)) {
    fprintf(stderr, "segment put/duplicate failed\n");
    return 3;
  }
  uint32_t slot = 99u;
  if (!h9_segment_local_cache_debug_take(class_id, &slot) || slot != 0u) {
    fprintf(stderr, "segment first take failed: %u\n", slot);
    return 4;
  }
  if (!h9_segment_local_cache_debug_free_allocated(class_id, 0u) ||
      h9_segment_local_cache_debug_free_allocated(class_id, 0u)) {
    fprintf(stderr, "segment free allocated transition failed\n");
    return 5;
  }
  if (h9_segment_local_cache_debug_free_bits(class_id) != UINT64_C(3) ||
      h9_segment_local_cache_debug_alloc_bits(class_id) != 0u ||
      h9_segment_local_cache_debug_remote_bits(class_id) != 0u ||
      h9_segment_local_cache_debug_touched_classes() !=
          (UINT64_C(1) << class_id)) {
    fprintf(stderr, "segment local final state mismatch\n");
    return 6;
  }
  return 0;
}

static int check_remote_transition(uint32_t class_id) {
  h9_segment_local_cache_debug_reset();
  if (!h9_segment_local_cache_debug_put(class_id, 0u) ||
      !h9_segment_local_cache_debug_remote_mark(class_id, 0u)) {
    fprintf(stderr, "segment remote transition setup failed\n");
    return 10;
  }
  if (h9_segment_local_cache_debug_state(class_id) != 1u ||
      h9_segment_local_cache_debug_free_bits(class_id) != 0u ||
      h9_segment_local_cache_debug_remote_bits(class_id) != UINT64_C(1)) {
    fprintf(stderr, "segment remote state mismatch\n");
    return 11;
  }
  uint32_t slot = 0u;
  if (h9_segment_local_cache_debug_take(class_id, &slot) ||
      h9_segment_local_cache_debug_put(class_id, 1u) ||
      h9_segment_local_cache_debug_remote_mark(class_id, 0u)) {
    fprintf(stderr, "segment accepted operation after REMOTE_SEEN\n");
    return 12;
  }
  if (!h9_segment_local_cache_debug_remote_mark(class_id, 1u)) {
    fprintf(stderr, "segment rejected second remote pending slot\n");
    return 13;
  }
  uint64_t drained = 0u;
  if (!h9_segment_local_cache_debug_drain_remote(class_id, &drained) ||
      drained != UINT64_C(3) ||
      h9_segment_local_cache_debug_state(class_id) != 2u ||
      h9_segment_local_cache_debug_remote_bits(class_id) != 0u ||
      h9_segment_local_cache_debug_drain_remote(class_id, NULL) ||
      h9_segment_local_cache_debug_put(class_id, 0u)) {
    fprintf(stderr, "segment remote drain state mismatch\n");
    return 14;
  }
  h9_segment_local_cache_debug_reset();
  if (h9_segment_local_cache_debug_drain_remote(class_id, NULL)) {
    fprintf(stderr, "segment drained LOCAL state\n");
    return 15;
  }
  if (!h9_segment_local_cache_debug_retire(class_id) ||
      h9_segment_local_cache_debug_state(class_id) != 2u) {
    fprintf(stderr, "segment direct retire state mismatch\n");
    return 16;
  }
  return 0;
}

static int check_release_all(void) {
  h9_segment_local_cache_debug_reset();
  if (!h9_segment_local_cache_debug_put(4u, 0u) ||
      !h9_segment_local_cache_debug_put(5u, 0u) ||
      !h9_segment_local_cache_debug_remote_mark(5u, 0u)) {
    fprintf(stderr, "segment release-all setup failed\n");
    return 30;
  }
  uint64_t released = h9_segment_local_cache_debug_release_all();
  if (released != ((UINT64_C(1) << 4u) | (UINT64_C(1) << 5u)) ||
      h9_segment_local_cache_debug_touched_classes() != 0u ||
      h9_segment_local_cache_debug_free_bits(4u) != 0u ||
      h9_segment_local_cache_debug_remote_bits(5u) != 0u ||
      h9_segment_local_cache_debug_state(5u) != 0u) {
    fprintf(stderr, "segment release-all state mismatch\n");
    return 31;
  }
  return 0;
}

static int check_class_geometry(void) {
  for (uint32_t class_id = 0u; class_id < H8_MEDIUM_CLASS_COUNT; ++class_id) {
    uint32_t slot_size = 0u;
    uint32_t run_size = 0u;
    uint16_t slot_count = 0u;
    size_t payload_bytes = 0u;
    size_t slack_bytes = 0u;
    if (!h9_segment_local_cache_debug_class_geometry(
            class_id, &slot_size, &run_size, &slot_count) ||
        !h9_segment_local_cache_debug_class_capacity(
            class_id, &payload_bytes, &slack_bytes) ||
        slot_size == 0u || run_size == 0u || slot_count == 0u ||
        slot_count > 64u ||
        payload_bytes != (size_t)slot_size * (size_t)slot_count ||
        payload_bytes + slack_bytes != (size_t)run_size) {
      fprintf(stderr, "segment class geometry invalid: %u\n", class_id);
      return 40;
    }
  }
  if (h9_segment_local_cache_debug_class_geometry(
          H8_MEDIUM_CLASS_COUNT, NULL, NULL, NULL) ||
      h9_segment_local_cache_debug_class_capacity(
          H8_MEDIUM_CLASS_COUNT, NULL, NULL)) {
    fprintf(stderr, "segment accepted invalid geometry class\n");
    return 41;
  }
  return 0;
}

static int check_route_offsets(void) {
  for (uint32_t class_id = 0u; class_id < H8_MEDIUM_CLASS_COUNT; ++class_id) {
    uint32_t slot_size = 0u;
    uint32_t run_size = 0u;
    uint16_t slot_count = 0u;
    size_t payload_bytes = 0u;
    size_t slack_bytes = 0u;
    if (!h9_segment_local_cache_debug_class_geometry(
            class_id, &slot_size, &run_size, &slot_count) ||
        !h9_segment_local_cache_debug_class_capacity(
            class_id, &payload_bytes, &slack_bytes)) {
      fprintf(stderr, "segment route geometry failed: %u\n", class_id);
      return 50;
    }
    if (h9_segment_local_cache_debug_route_offset(class_id, 0u) !=
        H8_ROUTE_VALID) {
      fprintf(stderr, "segment route slot0 failed: %u\n", class_id);
      return 51;
    }
    if (slot_count > 1u &&
        h9_segment_local_cache_debug_route_offset(class_id, slot_size) !=
            H8_ROUTE_VALID) {
      fprintf(stderr, "segment route slot1 failed: %u\n", class_id);
      return 52;
    }
    if (h9_segment_local_cache_debug_route_offset(class_id, 1u) !=
        H8_ROUTE_INVALID) {
      fprintf(stderr, "segment route interior failed: %u\n", class_id);
      return 53;
    }
    if (slack_bytes != 0u &&
        h9_segment_local_cache_debug_route_offset(class_id, payload_bytes) !=
            H8_ROUTE_INVALID) {
      fprintf(stderr, "segment route tail failed: %u\n", class_id);
      return 54;
    }
    if (h9_segment_local_cache_debug_route_offset(class_id, run_size) !=
        H8_ROUTE_MISS) {
      fprintf(stderr, "segment route outside failed: %u\n", class_id);
      return 55;
    }
  }
  if (h9_segment_local_cache_debug_route_offset(H8_MEDIUM_CLASS_COUNT, 0u) !=
      H8_ROUTE_MISS) {
    fprintf(stderr, "segment route accepted invalid class\n");
    return 56;
  }
  return 0;
}

static int check_bound_route(void) {
  const uint32_t class_id = 4u;
  const uintptr_t base = (uintptr_t)0x10000000u;
  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  size_t payload_bytes = 0u;
  size_t slack_bytes = 0u;
  h9_segment_local_cache_debug_reset();
  if (!h9_segment_local_cache_debug_class_geometry(
          class_id, &slot_size, &run_size, &slot_count) ||
      !h9_segment_local_cache_debug_class_capacity(
          class_id, &payload_bytes, &slack_bytes)) {
    fprintf(stderr, "segment bound route geometry failed\n");
    return 60;
  }
  if (h9_segment_local_cache_debug_route_addr(class_id, base) !=
          H8_ROUTE_MISS ||
      h9_segment_local_cache_debug_bind_base(class_id, 0u) ||
      h9_segment_local_cache_debug_bind_base(H8_MEDIUM_CLASS_COUNT, base) ||
      !h9_segment_local_cache_debug_bind_base(class_id, base)) {
    fprintf(stderr, "segment bind precondition failed\n");
    return 61;
  }
  if (h9_segment_local_cache_debug_route_addr(class_id, base) !=
          H8_ROUTE_VALID ||
      h9_segment_local_cache_debug_route_addr(class_id, base + 1u) !=
          H8_ROUTE_INVALID ||
      h9_segment_local_cache_debug_route_addr(class_id, base - 1u) !=
          H8_ROUTE_MISS ||
      h9_segment_local_cache_debug_route_addr(class_id, base + run_size) !=
          H8_ROUTE_MISS) {
    fprintf(stderr, "segment bound route base boundary failed\n");
    return 62;
  }
  if (slot_count > 1u &&
      h9_segment_local_cache_debug_route_addr(class_id, base + slot_size) !=
          H8_ROUTE_VALID) {
    fprintf(stderr, "segment bound route slot1 failed\n");
    return 63;
  }
  if (slack_bytes != 0u &&
      h9_segment_local_cache_debug_route_addr(class_id,
                                              base + payload_bytes) !=
          H8_ROUTE_INVALID) {
    fprintf(stderr, "segment bound route slack failed\n");
    return 64;
  }
  if (!h9_segment_local_cache_debug_remote_mark(class_id, 0u) ||
      h9_segment_local_cache_debug_route_addr(class_id, base) !=
          H8_ROUTE_INVALID) {
    fprintf(stderr, "segment remote route invalidation failed\n");
    return 65;
  }
  uint64_t drained = 0u;
  if (!h9_segment_local_cache_debug_drain_remote(class_id, &drained) ||
      drained != UINT64_C(1) ||
      h9_segment_local_cache_debug_route_addr(class_id, base) !=
          H8_ROUTE_INVALID) {
    fprintf(stderr, "segment retired route invalidation failed\n");
    return 66;
  }
  h9_segment_local_cache_debug_release_all();
  if (h9_segment_local_cache_debug_route_addr(class_id, base) !=
      H8_ROUTE_MISS) {
    fprintf(stderr, "segment release route miss failed\n");
    return 67;
  }
  return 0;
}

int main(void) {
  h8_init();
  int rc = check_class_geometry();
  if (rc != 0) {
    return rc;
  }
  rc = check_route_offsets();
  if (rc != 0) {
    return rc;
  }
  rc = check_bound_route();
  if (rc != 0) {
    return rc;
  }
  rc = check_local_flow(5u);
  if (rc != 0) {
    return rc;
  }
  rc = check_remote_transition(5u);
  if (rc != 0) {
    return rc;
  }
  rc = check_release_all();
  if (rc != 0) {
    return rc;
  }
  if (h9_segment_local_cache_debug_put(H8_MEDIUM_CLASS_COUNT, 0u) ||
      h9_segment_local_cache_debug_put(5u, 64u)) {
    fprintf(stderr, "segment accepted invalid class/slot\n");
    return 20;
  }
  puts("hz9_segment_local_cache_smoke ok");
  return 0;
}
