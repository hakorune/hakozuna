#include "../src/h8_hz9_last_token_integrated_entry.h"
#include "../src/h8_hz9_local_slab_inline_body.h"
#include "../src/h8_hz9_local_slab_pointer_token.h"
#include "../src/h8_hz9_local_slab_route_boundary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)
#error "local slab route boundary smoke requires L0 flag"
#endif

static int expect_route(void* ptr, H8RouteKind expected, const char* label) {
  H9LspRouteResult route = h9_lsp_debug_route(ptr);
  if (route.kind != expected) {
    fprintf(stderr, "%s: route=%d expected=%d reason=%u\n", label,
            (int)route.kind, (int)expected, route.reason);
    return 1;
  }
  return 0;
}

static int check_class(uint32_t class_id) {
  bool owned = false;
  size_t usable = 0u;
  void* p = h9_lsp_debug_alloc(class_id);
  if (!p || expect_route(p, H8_ROUTE_VALID, "exact")) {
    return 1;
  }
  memset(p, (int)(0x51u + class_id), 16u);

  H9LspRouteResult exact = h9_lsp_debug_route(p);
  if (!exact.payload_base || exact.slot_size == 0u || exact.slot_count == 0u) {
    fprintf(stderr, "route payload metadata missing\n");
    return 2;
  }

  void* interior = (char*)p + 1;
  void* tail = (char*)exact.payload_base +
               (size_t)exact.slot_size * (size_t)exact.slot_count;
  int stack_miss = 0;
  if (expect_route(interior, H8_ROUTE_INVALID, "interior") ||
      expect_route(tail, H8_ROUTE_INVALID, "tail") ||
      expect_route(&stack_miss, H8_ROUTE_MISS, "miss")) {
    return 3;
  }

  if (!h9_lsp_debug_usable_size(p, &usable, &owned) || !owned ||
      usable != exact.slot_size) {
    fprintf(stderr, "usable route mismatch usable=%zu slot=%u owned=%d\n",
            usable, exact.slot_size, owned ? 1 : 0);
    return 4;
  }
  if (h9_lsp_debug_usable_size(interior, &usable, &owned) || !owned) {
    fprintf(stderr, "interior usable accepted owned=%d\n", owned ? 1 : 0);
    return 5;
  }

  if (h9_lsp_debug_realloc_in_place(p, exact.slot_size, &owned) != p ||
      !owned) {
    fprintf(stderr, "in-place realloc rejected\n");
    return 6;
  }
  if (h9_lsp_debug_realloc_in_place(p, (size_t)exact.slot_size + 1u,
                                    &owned) ||
      !owned) {
    fprintf(stderr, "oversize realloc accepted\n");
    return 7;
  }

  if (!h9_lsp_debug_free(p, &owned) || !owned ||
      expect_route(p, H8_ROUTE_INVALID, "after-free")) {
    fprintf(stderr, "exact free failed owned=%d\n", owned ? 1 : 0);
    return 8;
  }
  if (h9_lsp_debug_free(p, &owned) || !owned) {
    fprintf(stderr, "double free accepted owned=%d\n", owned ? 1 : 0);
    return 9;
  }
  if (h9_lsp_debug_free(&stack_miss, &owned) || owned) {
    fprintf(stderr, "miss free owned=%d\n", owned ? 1 : 0);
    return 10;
  }
  return 0;
}

static int check_pointer_token(void) {
  void* payload = malloc(64u * 64u);
  if (!payload) {
    return 20;
  }
  H9LspInlinePage page;
  h9_lsp_inline_page_init(&page, (uintptr_t)payload, 64u, 64u);
  page.class_id = 5u;
  H9LspPtrLedger ledger = {0};

  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&page, &slot, &ptr) || !ptr) {
    free(payload);
    return 21;
  }
  h9_lsp_ptr_ledger_insert(&ledger, ptr, &page, slot, 5u);
  if (h9_lsp_ptr_ledger_lookup(&ledger, (char*)ptr + 1) != NULL) {
    fprintf(stderr, "pointer-token accepted interior pointer\n");
    free(payload);
    return 22;
  }
  if (!h9_lsp_ptr_ledger_free_hit(&ledger, ptr)) {
    fprintf(stderr, "pointer-token exact free missed\n");
    free(payload);
    return 23;
  }
  if (h9_lsp_ptr_ledger_free_hit(&ledger, ptr)) {
    fprintf(stderr, "pointer-token double free accepted\n");
    free(payload);
    return 24;
  }

  if (!h9_lsp_inline_alloc_slot(&page, &slot, &ptr) || !ptr) {
    free(payload);
    return 25;
  }
  h9_lsp_ptr_ledger_insert(&ledger, ptr, &page, slot, 5u);
  ++page.generation;
  if (h9_lsp_ptr_ledger_free_hit(&ledger, ptr)) {
    fprintf(stderr, "pointer-token stale generation accepted\n");
    free(payload);
    return 26;
  }
  h9_lsp_ptr_ledger_clear_entry(h9_lsp_ptr_ledger_lookup(&ledger, ptr));
  (void)h9_lsp_inline_free_slot(&page, slot);

  H9LspPtrTokenCache cache = {0};
  page.generation = 1u;
  void* a = NULL;
  void* b = NULL;
  uint32_t slot_a = UINT32_MAX;
  uint32_t slot_b = UINT32_MAX;
  if (!h9_lsp_inline_alloc_slot(&page, &slot_a, &a) ||
      !h9_lsp_inline_alloc_slot(&page, &slot_b, &b)) {
    free(payload);
    return 27;
  }
  h9_lsp_ptr_cache_insert(&cache, a, &page, slot_a, 5u);
  h9_lsp_ptr_cache_insert(&cache, b, &page, slot_b, 5u);
  if (!h9_lsp_ptr_cache_free_hit(&cache, b) ||
      !h9_lsp_ptr_cache_free_hit(&cache, a) ||
      h9_lsp_ptr_cache_free_hit(&cache, a)) {
    fprintf(stderr, "pointer-token cache last/ledger contract failed\n");
    free(payload);
    return 28;
  }

  H9LspPtrLastOnly last_only = {0};
  void* c = NULL;
  void* d = NULL;
  uint32_t slot_c = UINT32_MAX;
  uint32_t slot_d = UINT32_MAX;
  if (!h9_lsp_inline_alloc_slot(&page, &slot_c, &c) ||
      !h9_lsp_inline_alloc_slot(&page, &slot_d, &d)) {
    free(payload);
    return 29;
  }
  h9_lsp_ptr_last_insert(&last_only, c, &page, slot_c, 5u);
  h9_lsp_ptr_last_insert(&last_only, d, &page, slot_d, 5u);
  if (h9_lsp_ptr_last_free_hit(&last_only, c) ||
      !h9_lsp_ptr_last_free_hit(&last_only, d)) {
    fprintf(stderr, "pointer-token last-only contract failed\n");
    free(payload);
    return 30;
  }
  (void)h9_lsp_inline_free_slot(&page, slot_c);
  free(payload);
  return 0;
}

static int check_pointer_token_entry(void) {
  bool owned = false;
  size_t usable = 0u;
  void* ptr = h9_lsp_debug_ptrtoken_alloc(5u);
  if (!ptr || !h9_lsp_debug_ptrtoken_usable_size(ptr, &usable, &owned) ||
      !owned || usable == 0u) {
    fprintf(stderr, "ptrtoken entry usable fast failed\n");
    return 30;
  }
  if (!h9_lsp_debug_ptrtoken_realloc_in_place(ptr, usable, &owned) || !owned) {
    fprintf(stderr, "ptrtoken entry realloc fast failed\n");
    return 31;
  }
  if (h9_lsp_debug_ptrtoken_free((char*)ptr + 1, &owned) || !owned) {
    fprintf(stderr, "ptrtoken entry interior accepted owned=%d\n",
            owned ? 1 : 0);
    return 32;
  }
  if (!h9_lsp_debug_ptrtoken_free(ptr, &owned) || !owned) {
    fprintf(stderr, "ptrtoken entry exact free failed owned=%d\n",
            owned ? 1 : 0);
    return 33;
  }
  if (h9_lsp_debug_ptrtoken_free(ptr, &owned) || !owned) {
    fprintf(stderr, "ptrtoken entry double free accepted owned=%d\n",
            owned ? 1 : 0);
    return 34;
  }

  int stack_miss = 0;
  if (h9_lsp_debug_ptrtoken_free(&stack_miss, &owned) || owned) {
    fprintf(stderr, "ptrtoken entry miss accepted owned=%d\n", owned ? 1 : 0);
    return 35;
  }

  H9LspStats stats = h9_lsp_debug_stats();
  if (stats.ptrtoken_free_fast == 0u || stats.ptrtoken_free_fallback == 0u ||
      stats.ptrtoken_usable_fast == 0u || stats.ptrtoken_realloc_fast == 0u ||
      stats.free_invalid_owned == 0u || stats.free_miss == 0u) {
    fprintf(stderr,
            "ptrtoken entry counters failed fast=%zu fallback=%zu usable=%zu "
            "realloc=%zu invalid=%zu miss=%zu\n",
            stats.ptrtoken_free_fast, stats.ptrtoken_free_fallback,
            stats.ptrtoken_usable_fast, stats.ptrtoken_realloc_fast,
            stats.free_invalid_owned, stats.free_miss);
    return 36;
  }
  return 0;
}

static int check_last_token_entry(void) {
  bool owned = false;
  size_t usable = 0u;
  void* ptr = h9_lsp_debug_lasttoken_alloc(5u);
  if (!ptr) {
    return 40;
  }
  if (!h9_lsp_debug_lasttoken_usable_size(ptr, &usable, &owned) || !owned ||
      usable == 0u) {
    fprintf(stderr, "last-token entry usable fast failed\n");
    return 46;
  }
  if (!h9_lsp_debug_lasttoken_realloc_in_place(ptr, usable, &owned) ||
      !owned) {
    fprintf(stderr, "last-token entry realloc fast failed\n");
    return 47;
  }
  if (h9_lsp_debug_lasttoken_realloc_in_place(ptr, usable + 1u, &owned) ||
      !owned) {
    fprintf(stderr, "last-token entry oversize realloc accepted owned=%d\n",
            owned ? 1 : 0);
    return 48;
  }
  if (h9_lsp_debug_lasttoken_usable_size((char*)ptr + 1, &usable, &owned) ||
      !owned) {
    fprintf(stderr, "last-token entry interior usable accepted owned=%d\n",
            owned ? 1 : 0);
    return 49;
  }
  if (h9_lsp_debug_lasttoken_free((char*)ptr + 1, &owned) || !owned) {
    fprintf(stderr, "last-token entry interior accepted owned=%d\n",
            owned ? 1 : 0);
    return 41;
  }
  if (!h9_lsp_debug_lasttoken_free(ptr, &owned) || !owned) {
    fprintf(stderr, "last-token entry exact fallback free failed owned=%d\n",
            owned ? 1 : 0);
    return 42;
  }
  if (h9_lsp_debug_lasttoken_free(ptr, &owned) || !owned) {
    fprintf(stderr, "last-token entry double free accepted owned=%d\n",
            owned ? 1 : 0);
    return 43;
  }

  ptr = h9_lsp_debug_lasttoken_alloc(5u);
  if (!ptr || !h9_lsp_debug_lasttoken_free(ptr, &owned) || !owned) {
    fprintf(stderr, "last-token entry exact fast free failed owned=%d\n",
            owned ? 1 : 0);
    return 44;
  }
  H9LspStats stats = h9_lsp_debug_stats();
  if (stats.ptrtoken_free_fast == 0u || stats.ptrtoken_free_fallback == 0u ||
      stats.ptrtoken_usable_fast == 0u ||
      stats.ptrtoken_usable_fallback == 0u ||
      stats.ptrtoken_realloc_fast == 0u ||
      stats.ptrtoken_realloc_fallback == 0u || stats.free_invalid_owned == 0u) {
    fprintf(stderr,
            "last-token entry counters failed free=%zu fallback=%zu usable=%zu "
            "usable_fb=%zu realloc=%zu realloc_fb=%zu invalid=%zu\n",
            stats.ptrtoken_free_fast, stats.ptrtoken_free_fallback,
            stats.ptrtoken_usable_fast, stats.ptrtoken_usable_fallback,
            stats.ptrtoken_realloc_fast, stats.ptrtoken_realloc_fallback,
            stats.free_invalid_owned);
    return 45;
  }
  return 0;
}

static int check_integrated_entry(void) {
  void* payload = malloc(64u * 64u);
  if (!payload) {
    return 50;
  }
  H9LspIntegratedEntry entry;
  h9_lsp_integrated_init(&entry, (uintptr_t)payload, 64u, 64u, 5u);

  void* ptr = NULL;
  if (!h9_lsp_integrated_alloc(&entry, &ptr) ||
      !h9_lsp_integrated_free(&entry, ptr)) {
    fprintf(stderr, "integrated exact free failed\n");
    free(payload);
    return 51;
  }
  if (h9_lsp_integrated_free(&entry, ptr)) {
    fprintf(stderr, "integrated double free accepted\n");
    free(payload);
    return 52;
  }

  void* a = NULL;
  void* b = NULL;
  if (!h9_lsp_integrated_alloc(&entry, &a) ||
      !h9_lsp_integrated_alloc(&entry, &b)) {
    free(payload);
    return 53;
  }
  if (h9_lsp_integrated_free(&entry, a) ||
      !h9_lsp_integrated_free(&entry, b)) {
    fprintf(stderr, "integrated non-last fallback contract failed\n");
    free(payload);
    return 54;
  }
  (void)h9_lsp_inline_free_slot(&entry.page, 0u);

  if (entry.fast_hits == 0u || entry.fallback_hits == 0u ||
      entry.state_mismatch != 0u) {
    fprintf(stderr,
            "integrated counters failed fast=%llu fallback=%llu mismatch=%llu\n",
            (unsigned long long)entry.fast_hits,
            (unsigned long long)entry.fallback_hits,
            (unsigned long long)entry.state_mismatch);
    free(payload);
    return 55;
  }
  free(payload);
  return 0;
}

static int check_public_entry(void) {
  bool owned = false;
  int stack_miss = 0;
  void* p = h9_lsp_debug_public_malloc(65536u);
  if (!p || expect_route(p, H8_ROUTE_VALID, "public exact")) {
    return 60;
  }
  if (expect_route((char*)p + 1, H8_ROUTE_INVALID, "public interior")) {
    return 61;
  }
  if (!h9_lsp_debug_public_free(p, &owned) || !owned ||
      expect_route(p, H8_ROUTE_INVALID, "public after-free")) {
    return 62;
  }
  if (h9_lsp_debug_public_free(p, &owned) || !owned) {
    fprintf(stderr, "public double free accepted owned=%d\n", owned ? 1 : 0);
    return 63;
  }
  if (h9_lsp_debug_public_free(&stack_miss, &owned) || owned) {
    fprintf(stderr, "public miss free owned=%d\n", owned ? 1 : 0);
    return 64;
  }
  return 0;
}

static int check_public_entry_nosync(void) {
  bool owned = false;
  int stack_miss = 0;
  void* p = h9_lsp_debug_public_nosync_malloc(65536u);
  if (!p || !h9_lsp_debug_public_nosync_free(p, &owned) || !owned) {
    return 65;
  }
  if (h9_lsp_debug_public_nosync_free(p, &owned) || !owned) {
    fprintf(stderr, "public nosync double free accepted owned=%d\n",
            owned ? 1 : 0);
    return 66;
  }
  void* q = h9_lsp_debug_public_nosync_malloc(65536u);
  if (!q) {
    return 67;
  }
  if (h9_lsp_debug_public_nosync_free((char*)q + 1, &owned) || !owned) {
    fprintf(stderr, "public nosync interior accepted owned=%d\n",
            owned ? 1 : 0);
    return 68;
  }
  if (!h9_lsp_debug_public_nosync_free(q, &owned) || !owned) {
    return 69;
  }
  if (h9_lsp_debug_public_nosync_free(&stack_miss, &owned) || owned) {
    fprintf(stderr, "public nosync miss free owned=%d\n", owned ? 1 : 0);
    return 70;
  }
  return 0;
}

static int check_public_entry_current(void) {
  h9_lsp_debug_public_entry_reset();
  void* p = h9_lsp_debug_public_nosync_malloc(65536u);
  if (!p || !h9_lsp_debug_public_current_free(p)) {
    return 71;
  }
  if (h9_lsp_debug_public_current_free(p)) {
    fprintf(stderr, "public current double free accepted\n");
    return 72;
  }
  void* q = h9_lsp_debug_public_nosync_malloc(65536u);
  if (!q || h9_lsp_debug_public_current_free((char*)q + 1) ||
      !h9_lsp_debug_public_current_free(q)) {
    return 73;
  }
  return 0;
}

static int check_public_remote_pending(void) {
  bool owned = false;
  void* p = h9_lsp_debug_alloc(5u);
  if (!p || !h9_lsp_debug_public_product_free(p, &owned) || !owned) {
    fprintf(stderr, "public remote pending first claim failed owned=%d\n",
            owned ? 1 : 0);
    return 74;
  }
  if (h9_lsp_debug_public_product_free(p, &owned) || !owned) {
    fprintf(stderr, "public remote pending duplicate accepted owned=%d\n",
            owned ? 1 : 0);
    return 75;
  }
  if (h9_lsp_debug_public_product_free((char*)p + 1, &owned) || !owned) {
    fprintf(stderr, "public remote pending interior accepted owned=%d\n",
            owned ? 1 : 0);
    return 76;
  }
  H9LspStats stats = h9_lsp_debug_stats();
  if (stats.remote_pending_claim == 0u ||
      stats.remote_pending_duplicate == 0u ||
      stats.remote_pending_invalid == 0u) {
    fprintf(stderr, "remote pending counters claim=%zu dup=%zu invalid=%zu\n",
            stats.remote_pending_claim, stats.remote_pending_duplicate,
            stats.remote_pending_invalid);
    return 77;
  }
  return 0;
}

int main(void) {
  h9_lsp_debug_reset();
  int token_rc = check_pointer_token();
  if (token_rc != 0) {
    h9_lsp_debug_reset();
    return token_rc;
  }
  int entry_rc = check_pointer_token_entry();
  if (entry_rc != 0) {
    h9_lsp_debug_reset();
    return entry_rc;
  }
  int last_rc = check_last_token_entry();
  if (last_rc != 0) {
    h9_lsp_debug_reset();
    return last_rc;
  }
  int integrated_rc = check_integrated_entry();
  if (integrated_rc != 0) {
    h9_lsp_debug_reset();
    return integrated_rc;
  }
  int public_rc = check_public_entry();
  if (public_rc != 0) {
    h9_lsp_debug_reset();
    return public_rc;
  }
  int public_nosync_rc = check_public_entry_nosync();
  if (public_nosync_rc != 0) {
    h9_lsp_debug_reset();
    return public_nosync_rc;
  }
  int public_current_rc = check_public_entry_current();
  if (public_current_rc != 0) {
    h9_lsp_debug_reset();
    return public_current_rc;
  }
  int public_remote_rc = check_public_remote_pending();
  if (public_remote_rc != 0) {
    h9_lsp_debug_reset();
    return public_remote_rc;
  }
  for (uint32_t class_id = 0u; class_id < 6u; ++class_id) {
    int rc = check_class(class_id);
    if (rc != 0) {
      fprintf(stderr, "class %u failed rc=%d\n", class_id, rc);
      h9_lsp_debug_reset();
      return rc;
    }
  }

  H9LspStats stats = h9_lsp_debug_stats();
  if (stats.route_valid == 0u || stats.route_invalid == 0u ||
      stats.route_miss == 0u || stats.usable_route_valid == 0u ||
      stats.realloc_route_valid == 0u || stats.free_same_owner_local == 0u ||
      stats.free_invalid_owned == 0u || stats.free_miss == 0u) {
    fprintf(stderr,
            "counter gate failed valid=%zu invalid=%zu miss=%zu usable=%zu "
            "realloc=%zu free_local=%zu free_invalid=%zu free_miss=%zu\n",
            stats.route_valid, stats.route_invalid, stats.route_miss,
            stats.usable_route_valid, stats.realloc_route_valid,
            stats.free_same_owner_local, stats.free_invalid_owned,
            stats.free_miss);
    h9_lsp_debug_reset();
    return 11;
  }

  h9_lsp_debug_reset();
  printf("hz9_local_slab_route_boundary_smoke ok\n");
  return 0;
}
