#include "hz6_route_backend.h"
#include "hz6_route_backend_util.h"

Hz6RouteResult hz6_route_backend_lookup_page_table_invalid_probe(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr,
    size_t* probe_count) {
  size_t probes = probe_count ? *probe_count : 0;
  if (!backend || !backend->exact_table.entries ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    if (probe_count) {
      *probe_count = probes;
    }
    return hz6_route_miss();
  }

  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    const Hz6RouteEntry* entry = &backend->exact_table.entries[i];
    ++probes;
    if (!entry->active || entry->exact_valid) {
      continue;
    }

    uintptr_t entry_begin =
        hz6_route_backend_align_down(entry->base,
                                     backend->page_granularity);
    uintptr_t entry_end =
        hz6_route_backend_align_up(entry->base + entry->bytes,
                                   backend->page_granularity);
    if (page_addr >= entry_begin && page_addr < entry_end &&
        addr >= entry->base && addr < entry->base + entry->bytes) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  if (probe_count) {
    *probe_count = probes;
  }
  return hz6_route_miss();
}

Hz6RouteResult hz6_route_backend_lookup_page_table_invalid(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr) {
  return hz6_route_backend_lookup_page_table_invalid_probe(backend,
                                                          addr,
                                                          page_addr,
                                                          NULL);
}
