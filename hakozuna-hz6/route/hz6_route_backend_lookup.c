#include "hz6_route_backend.h"

Hz6RouteResult hz6_route_backend_lookup_page_table(
    const Hz6RouteBackend* backend,
    const void* ptr);

Hz6RouteResult hz6_route_backend_lookup_page_table_probe(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count);

Hz6RouteResult hz6_route_backend_lookup_probe(const Hz6RouteBackend* backend,
                                              const void* ptr,
                                              size_t* probe_count) {
  if (probe_count) {
    *probe_count = 0;
  }
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return hz6_route_miss();
  }
  if (backend->kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    return hz6_route_backend_lookup_page_table_probe(backend, ptr, probe_count);
  }
  return hz6_route_lookup_probe(&backend->exact_table, ptr, probe_count);
}

Hz6RouteResult hz6_route_backend_lookup(const Hz6RouteBackend* backend,
                                        const void* ptr) {
  return hz6_route_backend_lookup_probe(backend, ptr, NULL);
}
