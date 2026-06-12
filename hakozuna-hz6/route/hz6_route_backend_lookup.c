#include "hz6_route_backend.h"

Hz6RouteResult hz6_route_backend_lookup_page_table_probe(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count);

Hz6RouteResult hz6_route_backend_lookup_page_table_probe_ex(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count,
    size_t* exact_hash_probe_count,
    size_t* exact_range_probe_count,
    size_t* exact_page_seed_probe_count,
    size_t* invalid_probe_count);

Hz6RouteResult hz6_route_backend_lookup_probe_ex(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count,
    size_t* exact_hash_probe_count,
    size_t* exact_range_probe_count,
    size_t* exact_page_seed_probe_count,
    size_t* invalid_probe_count);

Hz6RouteResult hz6_route_backend_lookup_exact_probe(
    const Hz6RouteBackend* backend,
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
  return hz6_route_lookup_exact_probe(&backend->exact_table, ptr, probe_count);
}

Hz6RouteResult hz6_route_backend_lookup_exact(const Hz6RouteBackend* backend,
                                              const void* ptr) {
  return hz6_route_backend_lookup_exact_probe(backend, ptr, NULL);
}

Hz6RouteResult hz6_route_backend_lookup_probe(const Hz6RouteBackend* backend,
                                              const void* ptr,
                                              size_t* probe_count) {
  return hz6_route_backend_lookup_probe_ex(backend,
                                           ptr,
                                           probe_count,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL);
}

Hz6RouteResult hz6_route_backend_lookup_probe_ex(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count,
    size_t* exact_hash_probe_count,
    size_t* exact_range_probe_count,
    size_t* exact_page_seed_probe_count,
    size_t* invalid_probe_count) {
  if (probe_count) {
    *probe_count = 0;
  }
  if (exact_hash_probe_count) {
    *exact_hash_probe_count = 0;
  }
  if (exact_range_probe_count) {
    *exact_range_probe_count = 0;
  }
  if (exact_page_seed_probe_count) {
    *exact_page_seed_probe_count = 0;
  }
  if (invalid_probe_count) {
    *invalid_probe_count = 0;
  }
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return hz6_route_miss();
  }
  if (backend->kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    return hz6_route_backend_lookup_page_table_probe_ex(backend,
                                                        ptr,
                                                        probe_count,
                                                        exact_hash_probe_count,
                                                        exact_range_probe_count,
                                                        exact_page_seed_probe_count,
                                                        invalid_probe_count);
  }
  Hz6RouteResult route =
      hz6_route_lookup_probe(&backend->exact_table, ptr, probe_count);
  if (probe_count && exact_hash_probe_count) {
    *exact_hash_probe_count = *probe_count;
  }
  return route;
}

Hz6RouteResult hz6_route_backend_lookup(const Hz6RouteBackend* backend,
                                        const void* ptr) {
  return hz6_route_backend_lookup_probe(backend, ptr, NULL);
}
