#include "hz6_route_backend.h"
#include "hz6_route_backend_util.h"

Hz6RouteResult hz6_route_backend_lookup_page_table_exact(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr);

Hz6RouteResult hz6_route_backend_lookup_page_table(
    const Hz6RouteBackend* backend,
    const void* ptr) {
  if (!backend || !backend->exact_table.entries || !ptr ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    return hz6_route_miss();
  }

  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t page_addr =
      hz6_route_backend_align_down(addr, backend->page_granularity);
  return hz6_route_backend_lookup_page_table_exact(backend, addr, page_addr);
}
