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
  free(payload);
  return 0;
}

int main(void) {
  h9_lsp_debug_reset();
  int token_rc = check_pointer_token();
  if (token_rc != 0) {
    h9_lsp_debug_reset();
    return token_rc;
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
