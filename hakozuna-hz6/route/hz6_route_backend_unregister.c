#include "hz6_route_backend.h"

void hz6_route_backend_unregister_exact(Hz6RouteBackend* backend,
                                        void* base) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    return;
  }
  hz6_route_unregister_exact(&backend->exact_table, base);
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
