#include "hz6_route_backend.h"

int hz6_route_backend_compact_tombstones(Hz6RouteBackend* backend,
                                         size_t* moved_count) {
  if (!backend ||
      (backend->kind != HZ6_ROUTE_BACKEND_EXACT_TABLE &&
       backend->kind != HZ6_ROUTE_BACKEND_PAGE_TABLE)) {
    if (moved_count) {
      *moved_count = 0;
    }
    return 0;
  }
  return hz6_route_table_compact_tombstones(&backend->exact_table,
                                            moved_count);
}
