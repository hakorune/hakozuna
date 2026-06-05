#include "hz6_route_backend.h"

int hz6_route_backend_register_exact(Hz6RouteBackend* backend,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     uint32_t generation,
                                     void* descriptor,
                                     size_t* probe_count) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return 0;
  }
  return hz6_route_register_exact(&backend->exact_table, base, bytes, front_id,
                                  class_id, generation, descriptor,
                                  probe_count);
}

int hz6_route_backend_replace_exact_descriptor(Hz6RouteBackend* backend,
                                               void* base,
                                               size_t bytes,
                                               uint16_t front_id,
                                               uint16_t class_id,
                                               uint32_t old_generation,
                                               void* old_descriptor,
                                               uint32_t new_generation,
                                               void* new_descriptor,
                                               size_t* probe_count) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return 0;
  }
  return hz6_route_replace_exact_descriptor(&backend->exact_table, base, bytes,
                                            front_id, class_id,
                                            old_generation, old_descriptor,
                                            new_generation, new_descriptor,
                                            probe_count);
}

int hz6_route_backend_register_invalid_range(Hz6RouteBackend* backend,
                                             void* base,
                                             size_t bytes,
                                             uint16_t front_id,
                                             uint16_t class_id,
                                             size_t* probe_count) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return 0;
  }
  return hz6_route_register_invalid_range(&backend->exact_table, base, bytes,
                                          front_id, class_id, probe_count);
}
