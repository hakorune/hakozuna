#include "../src/h8_internal.h"

#include <stdio.h>

#if !defined(H9_STATIC_LOCAL_PAGE_SCAFFOLD_L0)
#error "static local page smoke requires H9_STATIC_LOCAL_PAGE_SCAFFOLD_L0"
#endif

static int check_class(uint32_t class_id) {
  h9_static_local_page_debug_reset();
  if (h9_static_local_page_debug_state_size() == 0u) {
    fprintf(stderr, "static local page state has zero size\n");
    return 1;
  }

  if (!h9_static_local_page_debug_put(class_id, 0u) ||
      !h9_static_local_page_debug_put(class_id, 1u)) {
    fprintf(stderr, "static local page put failed for class %u\n", class_id);
    return 2;
  }
  if (h9_static_local_page_debug_put(class_id, 1u)) {
    fprintf(stderr, "static local page accepted duplicate free bit\n");
    return 3;
  }

  uint32_t slot = 99u;
  if (!h9_static_local_page_debug_take(class_id, &slot) || slot != 0u) {
    fprintf(stderr, "static local page first take failed: slot=%u\n", slot);
    return 4;
  }
  if (!h9_static_local_page_debug_take(class_id, &slot) || slot != 1u) {
    fprintf(stderr, "static local page second take failed: slot=%u\n", slot);
    return 5;
  }
  if (h9_static_local_page_debug_take(class_id, &slot)) {
    fprintf(stderr, "static local page took from empty class\n");
    return 6;
  }

  if (!h9_static_local_page_debug_free_allocated(class_id, 0u) ||
      h9_static_local_page_debug_free_allocated(class_id, 0u)) {
    fprintf(stderr, "static local page allocated/free transition failed\n");
    return 7;
  }
  if (h9_static_local_page_debug_free_bits(class_id) != UINT64_C(1) ||
      h9_static_local_page_debug_alloc_bits(class_id) != UINT64_C(2) ||
      h9_static_local_page_debug_touched_classes() !=
          (UINT64_C(1) << class_id)) {
    fprintf(stderr, "static local page final bit state mismatch\n");
    return 8;
  }
  return 0;
}

int main(void) {
  h8_init();
  int rc = check_class(5u);
  if (rc != 0) {
    return rc;
  }
  if (h9_static_local_page_debug_put(H8_MEDIUM_CLASS_COUNT, 0u) ||
      h9_static_local_page_debug_put(5u, 64u)) {
    fprintf(stderr, "static local page accepted invalid class/slot\n");
    return 20;
  }
  puts("hz9_static_local_page_smoke ok");
  return 0;
}
