#include "../src/h8_hz9_segment_entry_internal.h"

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
  if (!h9_segment_entry_debug_cycle_handle_checked_touch(handle, 13u, true,
                                                         &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry handle checked touch failed: class=%u\n",
            class_id);
    return 10;
  }
  if (!h9_segment_entry_debug_cycle_tls_handle(class_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls handle cycle failed: class=%u\n",
            class_id);
    return 11;
  }
  if (!h9_segment_entry_debug_alloc_tls_handle(class_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_VALID ||
      !h9_segment_entry_debug_free(b, &owned) || !owned ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls route free failed: class=%u\n",
            class_id);
    return 12;
  }
  if (!h9_segment_entry_debug_alloc_tls_handle(class_id, &b) ||
      h9_segment_entry_debug_free_tls_handle(class_id, (char*)b + 1, &owned) ||
      !owned ||
      !h9_segment_entry_debug_free_tls_handle(class_id, b, &owned) || !owned ||
      h9_segment_entry_debug_free_tls_handle(class_id, b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls local free failed: class=%u\n",
            class_id);
    return 13;
  }
  uint32_t slot = 0u;
  if (!h9_segment_entry_debug_alloc_tls_slot(class_id, &b, &slot) ||
      !h9_segment_entry_debug_free_tls_slot(class_id, slot, &owned) || !owned ||
      h9_segment_entry_debug_free_tls_slot(class_id, slot, &owned) || !owned) {
    fprintf(stderr, "segment entry tls known-slot free failed: class=%u\n",
            class_id);
    return 14;
  }
  if (!h9_segment_entry_debug_cycle_tls_checked(class_id, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls checked cycle failed: class=%u\n",
            class_id);
    return 15;
  }
  if (!h9_segment_entry_debug_cycle_tls_checked_touch(class_id, 17u, true,
                                                      &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls checked touch failed: class=%u\n",
            class_id);
    return 16;
  }
  if (!h9_segment_entry_debug_cycle_tls_epoch_body(class_id, 18u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls epoch body failed: class=%u\n",
            class_id);
    return 17;
  }
  if (!h9_segment_entry_debug_cycle_tls_route64_body(class_id, 18u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry tls route64 body failed: class=%u\n",
            class_id);
    return 18;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_cache(class_id, 19u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls cache cycle failed: class=%u\n",
            class_id);
    return 19;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_token_cache(class_id, 21u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls token cache cycle failed: class=%u\n",
            class_id);
    return 20;
  }
  owned = false;
  if (!h9_segment_entry_debug_acquire_tls_token_cache(class_id) ||
      !h9_segment_entry_debug_cycle_tls_token_cache_body(class_id, 22u, true,
                                                         &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned ||
      !h9_segment_entry_debug_retire_tls_token_cache(class_id) ||
      h9_segment_entry_debug_cycle_tls_token_cache_body(class_id, 22u, true,
                                                        &b)) {
    fprintf(stderr, "segment entry tls token cache api failed: class=%u\n",
            class_id);
    return 21;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_ledger(class_id, 23u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls ledger cycle failed: class=%u\n",
            class_id);
    return 22;
  }
  owned = false;
  if (!h9_segment_entry_debug_cycle_tls_ledger_body(class_id, 29u, true, &b) ||
      h9_segment_entry_debug_route(b) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(b, &owned) || !owned) {
    fprintf(stderr, "segment entry tls ledger body failed: class=%u\n",
            class_id);
    return 23;
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

static int check_generation_token(void) {
  void* p = NULL;
  H9SegmentEntryToken token = {0};
  uintptr_t handle = h9_segment_entry_debug_prepare_handle(0u);
  uint32_t generation = h9_segment_entry_debug_handle_generation(handle);
  if (handle == 0u || generation == 0u ||
      !h9_segment_entry_debug_cycle_handle_generation(handle, generation, 31u,
                                                      true, &p)) {
    fprintf(stderr, "segment entry generation token valid check failed\n");
    h9_segment_entry_debug_reset();
    return 22;
  }
  if (!h9_segment_entry_debug_acquire_token(0u, &token) ||
      !h9_segment_entry_debug_token_current(&token)) {
    fprintf(stderr, "segment entry token acquire failed\n");
    h9_segment_entry_debug_reset();
    return 24;
  }
  h9_segment_entry_debug_reset();
  if (h9_segment_entry_debug_cycle_handle_generation(handle, generation, 37u,
                                                     true, &p)) {
    fprintf(stderr, "segment entry stale generation token accepted\n");
    h9_segment_entry_debug_reset();
    return 23;
  }
  if (h9_segment_entry_debug_token_current(&token)) {
    fprintf(stderr, "segment entry stale token remained current\n");
    h9_segment_entry_debug_reset();
    return 25;
  }
  h9_segment_entry_debug_reset();
  return 0;
}

static int check_token_cache_body(void) {
  void* p = NULL;
  bool owned = false;
  uint32_t cache_slot = UINT32_MAX;
  void* cache_ptr = NULL;
  H9SegmentEntryToken token = {0};
  H9SegmentEntryTokenCache cache_state;
  h9_segment_entry_token_cache_reset(&cache_state);
  if (!h9_segment_entry_debug_acquire_token(0u, &token) ||
      !h9_segment_entry_cycle_token_cache_inline(&token, &cache_slot,
                                                 &cache_ptr, 41u, true, &p) ||
      !p || cache_slot == UINT32_MAX || cache_ptr != p) {
    fprintf(stderr, "segment entry token cache cycle failed\n");
    h9_segment_entry_debug_reset();
    return 26;
  }
  if (h9_segment_entry_debug_route(p) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(p, &owned) || !owned) {
    fprintf(stderr, "segment entry token cache fail-closed failed\n");
    h9_segment_entry_debug_reset();
    return 27;
  }
  if (!h9_segment_entry_cycle_token_cache_inline(&token, &cache_slot,
                                                 &cache_ptr, 43u, true, &p) ||
      !p || cache_slot == UINT32_MAX || cache_ptr != p ||
      h9_segment_entry_debug_route(p) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry token cache reuse failed\n");
    h9_segment_entry_debug_reset();
    return 28;
  }
  if (!h9_segment_entry_retire_token_cache_inline(&token, &cache_slot,
                                                  &cache_ptr) ||
      cache_slot != UINT32_MAX || cache_ptr != NULL ||
      h9_segment_entry_debug_route(p) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(p, &owned) || !owned) {
    fprintf(stderr, "segment entry token cache retire failed\n");
    h9_segment_entry_debug_reset();
    return 29;
  }
  if (!h9_segment_entry_debug_alloc(0u, &p) ||
      h9_segment_entry_debug_route(p) != H8_ROUTE_VALID) {
    fprintf(stderr, "segment entry token cache retire reuse failed\n");
    h9_segment_entry_debug_reset();
    return 30;
  }
  h9_segment_entry_debug_reset();
  if (!h9_segment_entry_debug_acquire_token(0u, &cache_state.token) ||
      !h9_segment_entry_cycle_token_cache_state_inline(&cache_state, 47u, true,
                                                       &p) ||
      !p || cache_state.cache_slot == UINT32_MAX ||
      cache_state.cache_ptr != p ||
      h9_segment_entry_debug_route(p) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry token cache state cycle failed\n");
    h9_segment_entry_debug_reset();
    return 31;
  }
  if (h9_segment_entry_debug_free(p, &owned) || !owned ||
      !h9_segment_entry_retire_token_cache_state_inline(&cache_state) ||
      h9_segment_entry_debug_route(p) != H8_ROUTE_INVALID) {
    fprintf(stderr, "segment entry token cache state retire failed\n");
    h9_segment_entry_debug_reset();
    return 32;
  }
  h9_segment_entry_debug_reset();
  h9_segment_entry_token_cache_reset(&cache_state);
  if (!h9_segment_entry_debug_acquire_token(0u, &cache_state.token)) {
    fprintf(stderr, "segment entry token cache split acquire failed\n");
    h9_segment_entry_debug_reset();
    return 33;
  }
  uint32_t slot = UINT32_MAX;
  if (!h9_segment_entry_token_cache_pop_slot_inline(&cache_state, &slot, &p) ||
      slot == UINT32_MAX ||
      !h9_segment_entry_token_cache_push_slot_inline(&cache_state, slot, p) ||
      h9_segment_entry_debug_route(p) != H8_ROUTE_INVALID ||
      h9_segment_entry_debug_free(p, &owned) || !owned ||
      !h9_segment_entry_retire_token_cache_state_inline(&cache_state)) {
    fprintf(stderr, "segment entry token cache split failed\n");
    h9_segment_entry_debug_reset();
    return 34;
  }
  h9_segment_entry_debug_reset();
  return 0;
}

int main(void) {
  if (check_cold_multiclass_start() != 0) {
    return 9;
  }
  if (check_generation_token() != 0) {
    return 24;
  }
  if (check_token_cache_body() != 0) {
    return 26;
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
