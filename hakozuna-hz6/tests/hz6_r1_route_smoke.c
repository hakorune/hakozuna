#include "../include/hz6_contract.h"
#include "../route/hz6_route.h"
#include "../route/hz6_route_backend.h"

#include <stdio.h>

typedef struct SmokeDescriptor {
  int marker;
} SmokeDescriptor;

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-route-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  Hz6RouteEntry route_entries[4];
  Hz6RouteTable route_table;
  hz6_route_table_init(&route_table, route_entries, 4);

  SmokeDescriptor descriptor;
  descriptor.marker = 42;

  unsigned char object[256];
  void* base = (void*)object;

  if (!expect(hz6_route_register_exact(&route_table, base, sizeof(object),
                                       HZ6_FRONT_LOCAL2P, 7, 11,
                                       &descriptor, NULL),
              "route register")) {
    return 1;
  }

  Hz6RouteResult exact = hz6_route_lookup(&route_table, base);
  if (!expect(exact.kind == HZ6_ROUTE_VALID, "exact pointer valid") ||
      !expect(exact.front_id == HZ6_FRONT_LOCAL2P, "front id") ||
      !expect(exact.class_id == 7, "class id") ||
      !expect(exact.generation == 11, "generation") ||
      !expect(exact.descriptor == &descriptor, "descriptor")) {
    return 1;
  }

  Hz6RouteResult interior = hz6_route_lookup(&route_table, object + 16);
  if (!expect(interior.kind == HZ6_ROUTE_INVALID,
              "interior pointer invalid")) {
    return 1;
  }

  int foreign = 0;
  Hz6RouteResult miss = hz6_route_lookup(&route_table, &foreign);
  if (!expect(miss.kind == HZ6_ROUTE_MISS, "foreign pointer miss")) {
    return 1;
  }

  Hz6RouteEntry backend_entries[2];
  Hz6RouteBackend route_backend;
  hz6_route_backend_init_exact(&route_backend, backend_entries, 2);
  if (!expect(hz6_route_backend_register_exact(
                  &route_backend, base, sizeof(object), HZ6_FRONT_LOCAL2P, 7,
                  12, &descriptor, NULL),
              "route backend register")) {
    return 1;
  }
  Hz6RouteResult backend_exact =
      hz6_route_backend_lookup(&route_backend, base);
  Hz6RouteResult backend_interior =
      hz6_route_backend_lookup(&route_backend, object + 8);
  if (!expect(backend_exact.kind == HZ6_ROUTE_VALID,
              "route backend valid") ||
      !expect(backend_exact.generation == 12, "route backend generation") ||
      !expect(backend_interior.kind == HZ6_ROUTE_INVALID,
              "route backend invalid")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&route_backend, base, NULL);
  if (!expect(hz6_route_backend_lookup(&route_backend, base).kind ==
                  HZ6_ROUTE_MISS,
              "route backend unregister")) {
    return 1;
  }

  unsigned char route_run[512];
  SmokeDescriptor route_run_descriptor;
  route_run_descriptor.marker = 77;
  Hz6RouteEntry range_entries[4];
  Hz6RouteTable range_table;
  hz6_route_table_init(&range_table, range_entries, 4);
  if (!expect(hz6_route_register_invalid_range(
                  &range_table, route_run, sizeof(route_run),
                  HZ6_FRONT_MIDPAGE, 4, NULL),
              "route invalid range register") ||
      !expect(hz6_route_lookup(&range_table, route_run + 128).kind ==
                  HZ6_ROUTE_INVALID,
              "route invalid range lookup") ||
      !expect(hz6_route_register_exact(
                  &range_table, route_run + 128, 64, HZ6_FRONT_MIDPAGE, 4,
                  21, &route_run_descriptor, NULL),
              "route exact inside invalid range") ||
      !expect(hz6_route_lookup(&range_table, route_run + 128).kind ==
                  HZ6_ROUTE_VALID,
              "route exact wins over invalid range") ||
      !expect(hz6_route_lookup(&range_table, route_run + 129).kind ==
                  HZ6_ROUTE_INVALID,
              "route exact interior remains invalid")) {
    return 1;
  }
  hz6_route_unregister_exact(&range_table, route_run + 128, NULL);
  if (!expect(hz6_route_lookup(&range_table, route_run + 128).kind ==
                  HZ6_ROUTE_INVALID,
              "route invalid range remains after exact unregister")) {
    return 1;
  }
  hz6_route_unregister_invalid_range(&range_table, route_run, NULL);
  if (!expect(hz6_route_lookup(&range_table, route_run + 128).kind ==
                  HZ6_ROUTE_MISS,
              "route invalid range unregister")) {
    return 1;
  }

  Hz6RouteEntry page_backend_entries[4];
  Hz6RouteBackend page_backend;
  hz6_route_backend_init_page_table_with_granularity(
      &page_backend, page_backend_entries, 4, 64);
  if (!expect(page_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE,
              "page route backend kind") ||
      !expect(page_backend.page_granularity == 64,
              "page route backend granularity") ||
      !expect(hz6_route_backend_register_exact(
                  &page_backend, base, sizeof(object), HZ6_FRONT_LOCAL2P, 7,
                  13, &descriptor, NULL),
              "page route backend register")) {
    return 1;
  }
  Hz6RouteResult page_exact = hz6_route_backend_lookup(&page_backend, base);
  Hz6RouteResult page_interior =
      hz6_route_backend_lookup(&page_backend, object + 24);
  Hz6RouteResult page_end =
      hz6_route_backend_lookup(&page_backend, object + sizeof(object));
  if (!expect(page_exact.kind == HZ6_ROUTE_VALID,
              "page route backend valid") ||
      !expect(page_exact.generation == 13, "page route backend generation") ||
      !expect(page_interior.kind == HZ6_ROUTE_INVALID,
              "page route backend invalid") ||
      !expect(page_end.kind == HZ6_ROUTE_MISS,
              "page route backend end miss")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&page_backend, base, NULL);
  if (!expect(hz6_route_backend_lookup(&page_backend, base).kind ==
                  HZ6_ROUTE_MISS,
              "page route backend unregister")) {
    return 1;
  }
  if (!expect(hz6_route_backend_register_invalid_range(
                  &page_backend, route_run, sizeof(route_run),
                  HZ6_FRONT_MIDPAGE, 4, NULL),
              "page route backend invalid range register") ||
      !expect(hz6_route_backend_lookup(&page_backend, route_run + 192).kind ==
                  HZ6_ROUTE_INVALID,
              "page route backend invalid range lookup") ||
      !expect(hz6_route_backend_register_exact(
                  &page_backend, route_run + 192, 64, HZ6_FRONT_MIDPAGE, 4,
                  23, &route_run_descriptor, NULL),
              "page route backend exact inside invalid range") ||
      !expect(hz6_route_backend_lookup(&page_backend, route_run + 192).kind ==
                  HZ6_ROUTE_VALID,
              "page route backend exact range priority") ||
      !expect(hz6_route_backend_lookup(&page_backend, route_run + 193).kind ==
                  HZ6_ROUTE_INVALID,
              "page route backend exact interior invalid")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&page_backend, route_run + 192, NULL);
  if (!expect(hz6_route_backend_lookup(&page_backend, route_run + 192).kind ==
                  HZ6_ROUTE_INVALID,
              "page route backend invalid range remains after exact unregister")) {
    return 1;
  }
  hz6_route_backend_unregister_invalid_range(&page_backend, route_run, NULL);
  if (!expect(hz6_route_backend_lookup(&page_backend, route_run + 192).kind ==
                  HZ6_ROUTE_MISS,
              "page route backend invalid range unregister")) {
    return 1;
  }

  Hz6RouteEntry range_backend_entries[4];
  Hz6RouteBackend range_backend;
  hz6_route_backend_init_exact(&range_backend, range_backend_entries, 4);
  if (!expect(hz6_route_backend_register_invalid_range(
                  &range_backend, route_run, sizeof(route_run),
                  HZ6_FRONT_MIDPAGE, 4, NULL),
              "route backend invalid range register") ||
      !expect(hz6_route_backend_register_exact(
                  &range_backend, route_run + 256, 64, HZ6_FRONT_MIDPAGE, 4,
                  22, &route_run_descriptor, NULL),
              "route backend exact inside invalid range") ||
      !expect(hz6_route_backend_lookup(&range_backend, route_run + 256).kind ==
                  HZ6_ROUTE_VALID,
              "route backend exact range priority") ||
      !expect(hz6_route_backend_lookup(&range_backend, route_run + 320).kind ==
                  HZ6_ROUTE_INVALID,
              "route backend range end invalid")) {
    return 1;
  }
  hz6_route_backend_unregister_invalid_range(&range_backend, route_run, NULL);
  hz6_route_backend_unregister_exact(&range_backend, route_run + 256, NULL);
  if (!expect(hz6_route_backend_lookup(&range_backend, route_run + 256).kind ==
                  HZ6_ROUTE_MISS,
              "route backend invalid range cleanup")) {
    return 1;
  }

  printf("hz6-r1-route-smoke ok\n");
  return 0;
}
