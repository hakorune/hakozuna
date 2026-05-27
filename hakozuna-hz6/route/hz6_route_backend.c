#include "hz6_route_backend.h"

Hz6RouteResult hz6_route_backend_lookup_page_table(
    const Hz6RouteBackend* backend,
    const void* ptr);

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

int hz6_route_backend_register_exact(Hz6RouteBackend* backend,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     uint32_t generation,
                                     void* descriptor) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return 0;
  }
  return hz6_route_register_exact(&backend->exact_table, base, bytes, front_id,
                                  class_id, generation, descriptor);
}

void hz6_route_backend_unregister_exact(Hz6RouteBackend* backend,
                                        void* base) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return;
  }
  hz6_route_unregister_exact(&backend->exact_table, base);
}

int hz6_route_backend_register_invalid_range(Hz6RouteBackend* backend,
                                             void* base,
                                             size_t bytes,
                                             uint16_t front_id,
                                             uint16_t class_id) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return 0;
  }
  return hz6_route_register_invalid_range(&backend->exact_table, base, bytes,
                                          front_id, class_id);
}

void hz6_route_backend_unregister_invalid_range(Hz6RouteBackend* backend,
                                                void* base) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return;
  }
  hz6_route_unregister_invalid_range(&backend->exact_table, base);
}

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
