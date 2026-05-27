#include "hz6_route_backend.h"
#include "hz6_route_backend_util.h"

void hz6_route_backend_init_exact(Hz6RouteBackend* backend,
                                  Hz6RouteEntry* entries,
                                  size_t capacity) {
  if (!backend) {
    return;
  }
  backend->kind = HZ6_ROUTE_BACKEND_EXACT_TABLE;
  backend->exact_entries = entries;
  backend->page_granularity = 0;
  hz6_route_table_init(&backend->exact_table, entries, capacity);
}

void hz6_route_backend_init_page_table(Hz6RouteBackend* backend,
                                       Hz6RouteEntry* entries,
                                       size_t capacity) {
  hz6_route_backend_init_page_table_with_granularity(
      backend, entries, capacity, HZ6_ROUTE_PAGE_GRANULARITY);
}

void hz6_route_backend_init_page_table_with_granularity(
    Hz6RouteBackend* backend,
    Hz6RouteEntry* entries,
    size_t capacity,
    size_t page_granularity) {
  if (!backend) {
    return;
  }
  backend->kind = HZ6_ROUTE_BACKEND_PAGE_TABLE;
  backend->exact_entries = entries;
  backend->page_granularity =
      hz6_route_backend_valid_granularity(page_granularity)
          ? page_granularity
          : HZ6_ROUTE_PAGE_GRANULARITY;
  hz6_route_table_init(&backend->exact_table, entries, capacity);
}
