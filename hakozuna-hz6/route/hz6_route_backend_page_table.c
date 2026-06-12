#include "hz6_route_backend.h"
#include "hz6_route_backend_util.h"

Hz6RouteResult hz6_route_backend_lookup_page_table_exact(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr);

Hz6RouteResult hz6_route_backend_lookup_page_table_exact_probe(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr,
    size_t* probe_count,
    size_t* exact_probe_count,
    size_t* invalid_probe_count);

Hz6RouteResult hz6_route_backend_lookup_page_table_probe(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count) {
  return hz6_route_backend_lookup_page_table_probe_ex(backend,
                                                      ptr,
                                                      probe_count,
                                                      NULL,
                                                      NULL);
}

Hz6RouteResult hz6_route_backend_lookup_page_table_probe_ex(
    const Hz6RouteBackend* backend,
    const void* ptr,
    size_t* probe_count,
    size_t* exact_probe_count,
    size_t* invalid_probe_count) {
  if (probe_count) {
    *probe_count = 0;
  }
  if (exact_probe_count) {
    *exact_probe_count = 0;
  }
  if (invalid_probe_count) {
    *invalid_probe_count = 0;
  }
  if (!backend || !backend->exact_table.entries || !ptr ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    return hz6_route_miss();
  }

  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t page_addr =
      hz6_route_backend_align_down(addr, backend->page_granularity);
  return hz6_route_backend_lookup_page_table_exact_probe(backend,
                                                        addr,
                                                        page_addr,
                                                        probe_count,
                                                        exact_probe_count,
                                                        invalid_probe_count);
}
