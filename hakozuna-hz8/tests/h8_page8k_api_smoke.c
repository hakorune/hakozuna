#include "../include/h8.h"
#include "../src/h8_internal.h"
#include "../src/h8_medium_page8k_remote.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char* message) {
  fprintf(stderr, "page8k api smoke: %s\n", message);
  return 1;
}

int main(void) {
  h8_init();

  void* page = h8_malloc(8192u);
  if (!page) return fail("8192-byte allocation failed");

  size_t usable = 0;
  bool owned = false;
  if (h8_route(page) != H8_ROUTE_VALID) return fail("exact route is not VALID");
  if (!h8_usable_size_inner(page, &usable, &owned) || !owned ||
      usable != 8192u) {
    return fail("exact usable_size is not 8192");
  }

  void* same = h8_realloc(page, 8192u);
  if (same != page || h8_route(same) != H8_ROUTE_VALID) {
    return fail("same-size realloc did not preserve the page");
  }

  memset(same, 0x5a, 8192u);
  void* grown = h8_realloc(same, 9000u);
  if (!grown || h8_route(same) != H8_ROUTE_INVALID ||
      h8_route(grown) != H8_ROUTE_VALID) {
    return fail("cross-size realloc did not transfer ownership");
  }
  for (size_t i = 0; i < 8192u; ++i) {
    if (((const unsigned char*)grown)[i] != 0x5a) {
      return fail("cross-size realloc did not preserve payload");
    }
  }
  h8_free(grown);

  void* invalid = h8_malloc(8192u);
  if (!invalid) return fail("second 8192-byte allocation failed");
  void* interior = (unsigned char*)invalid + 1u;
  if (h8_route(interior) != H8_ROUTE_INVALID) {
    return fail("interior route is not INVALID");
  }
  usable = 0;
  owned = false;
  if (h8_usable_size_inner(interior, &usable, &owned) || !owned) {
    return fail("interior usable_size did not fail closed");
  }
  errno = 0;
  if (h8_realloc(interior, 9000u) != NULL || errno != EINVAL) {
    return fail("interior realloc did not return EINVAL");
  }
  h8_free(invalid);
  if (h8_route(invalid) != H8_ROUTE_INVALID) {
    return fail("duplicate/stale route is not INVALID");
  }

#if defined(H8_MEDIUM_PAGE8K_RANGE4097_L1)
  void* ranged = h8_malloc(5000u);
  if (!ranged || h8_route(ranged) != H8_ROUTE_VALID) {
    return fail("4097..8192 range allocation is not VALID");
  }
  usable = 0;
  owned = false;
  if (!h8_usable_size_inner(ranged, &usable, &owned) || !owned ||
      usable != 8192u) {
    return fail("range allocation usable_size is not 8192");
  }
  h8_free(ranged);
#if defined(H8_PAGE8K_REMOTE_DIAGNOSTIC)
  H8Page8KRemoteStats page_stats = h8_page8k_remote_stats();
  if (page_stats.range_eligible_alloc != page_stats.range_served_alloc) {
    return fail("range eligible/served attribution disagrees");
  }
  printf("range eligible=%llu served=%llu\n",
         (unsigned long long)page_stats.range_eligible_alloc,
         (unsigned long long)page_stats.range_served_alloc);
#endif
#endif

  puts("page8k api smoke: PASS");
  return 0;
}
