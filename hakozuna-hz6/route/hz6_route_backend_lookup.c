#include "hz6_route_backend.h"

Hz6RouteResult hz6_route_backend_lookup_page_table(
    const Hz6RouteBackend* backend,
    const void* ptr);

Hz6RouteResult hz6_route_backend_lookup(const Hz6RouteBackend* backend,
                                        const void* ptr) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return hz6_route_miss();
  }
  if (backend->kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    return hz6_route_backend_lookup_page_table(backend, ptr);
  }
  return hz6_route_lookup(&backend->exact_table, ptr);
}
