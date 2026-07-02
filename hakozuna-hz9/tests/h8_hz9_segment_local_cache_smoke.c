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

int main(void) {
  h8_init();
  int rc = check_local_flow(5u);
  if (rc != 0) {
    return rc;
  }
  rc = check_remote_transition(5u);
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
