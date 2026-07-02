#include "../src/h8_internal.h"
#include "../src/h8_hz9_segment_entry.h"

#include <stdio.h>
#include <string.h>

#if !defined(H9_SEGMENT_ENTRY_L1)
#error "segment entry smoke requires H9_SEGMENT_ENTRY_L1"
#endif

static int check_class(uint32_t class_id) {
  void* a = NULL;
  void* b = NULL;
  bool owned = false;

  if (!h9_segment_entry_debug_alloc(class_id, &a) || !a ||
      h9_segment_entry_debug_route(a) != H8_ROUTE_VALID) {
    fprintf(stderr, "segment entry alloc/route failed: class=%u\n", class_id);
    return 1;
  }
  memset(a, (int)(0xa0u + class_id), 16u);

  if (h9_segment_entry_debug_route((char*)a + 1) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry interior route accepted: class=%u\n",
            class_id);
    return 2;
  }
  if (h9_segment_entry_debug_free((char*)a + 1, &owned) || !owned) {
    fprintf(stderr, "segment entry interior free mismatch: class=%u\n",
            class_id);
    return 3;
  }
  if (!h9_segment_entry_debug_free(a, &owned) || !owned ||
      h9_segment_entry_debug_route(a) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry exact free failed: class=%u\n", class_id);
    return 4;
  }
  if (h9_segment_entry_debug_free(a, &owned) || !owned) {
    fprintf(stderr, "segment entry double free accepted: class=%u\n", class_id);
    return 5;
  }
  if (!h9_segment_entry_debug_alloc(class_id, &b) || b != a ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_VALID) {
    fprintf(stderr, "segment entry reuse failed: class=%u\n", class_id);
    return 6;
  }
  if (!h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry final free failed: class=%u\n", class_id);
    return 7;
  }
  uint32_t page_id = h9_segment_entry_debug_prepare_active(class_id);
  if (page_id == UINT32_MAX ||
      !h9_segment_entry_debug_cycle_page(page_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry direct page cycle failed: class=%u\n",
            class_id);
    return 8;
  }
  uintptr_t handle = h9_segment_entry_debug_prepare_handle(class_id);
  if (handle == 0u || !h9_segment_entry_debug_cycle_handle(handle, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry handle cycle failed: class=%u\n",
            class_id);
    return 9;
  }
  if (!h9_segment_entry_debug_cycle_tls_handle(class_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls handle cycle failed: class=%u\n",
            class_id);
    return 10;
  }
  if (!h9_segment_entry_debug_alloc_tls_handle(class_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_VALID ||
      !h9_segment_entry_debug_free(b, &owned) || !owned ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls route free failed: class=%u\n",
            class_id);
    return 11;
  }
  if (!h9_segment_entry_debug_alloc_tls_handle(class_id, &b) ||
      h9_segment_entry_debug_free_tls_handle(class_id, (char*)b + 1, &owned) ||
      !owned ||
      !h9_segment_entry_debug_free_tls_handle(class_id, b, &owned) || !owned ||
      h9_segment_entry_debug_free_tls_handle(class_id, b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls local free failed: class=%u\n",
            class_id);
    return 12;
  }
  uint32_t slot = 0u;
  if (!h9_segment_entry_debug_alloc_tls_slot(class_id, &b, &slot) ||
      !h9_segment_entry_debug_free_tls_slot(class_id, slot, &owned) || !owned ||
      h9_segment_entry_debug_free_tls_slot(class_id, slot, &owned) || !owned) {
    fprintf(stderr, "segment entry tls known-slot free failed: class=%u\n",
            class_id);
    return 13;
  }
  if (!h9_segment_entry_debug_cycle_tls_checked(class_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls checked cycle failed: class=%u\n",
            class_id);
    return 14;
  }
  if (!h9_segment_entry_debug_cycle_tls_checked_touch(class_id, 17u, true,
                                                      &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls checked touch failed: class=%u\n",
            class_id);
    return 15;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_cache(class_id, 19u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls cache cycle failed: class=%u\n",
            class_id);
    return 16;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_ledger(class_id, 23u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls ledger cycle failed: class=%u\n",
            class_id);
    return 17;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_ledger_body(class_id, 29u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls ledger body failed: class=%u\n",
            class_id);
    return 18;
  }
  return 0;
}

static int check_cold_multiclass_start(void) {
  void* a = NULL;
  void* b = NULL;
  if (!h9_segment_entry_debug_alloc(0u, &a) ||
      !h9_segment_entry_debug_alloc(1u, &b) || !a || !b ||
      h9_segment_entry_debug_page_count() != 2u) {
    fprintf(stderr, "segment entry cold multiclass start failed\n");
    h9_segment_entry_debug_reset();
    return 9;
  }
  h9_segment_entry_debug_reset();
  return 0;
}

int main(void) {
  if (check_cold_multiclass_start() != 0) {
    return 9;
  }
  h9_segment_entry_debug_reset();
  for (uint32_t class_id = 0u; class_id < H8_MEDIUM_CLASS_COUNT; ++class_id) {
    int rc = check_class(class_id);
    if (rc != 0) {
      h9_segment_entry_debug_reset();
      return rc;
    }
  }
  if (h9_segment_entry_debug_page_count() != H8_MEDIUM_CLASS_COUNT) {
    fprintf(stderr, "segment entry page count mismatch: %u\n",
            h9_segment_entry_debug_page_count());
    h9_segment_entry_debug_reset();
    return 10;
  }
  h9_segment_entry_debug_reset();
  puts("hz9_segment_entry_smoke ok");
  return 0;
}
