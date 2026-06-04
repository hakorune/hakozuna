#include "hz6_route_backend.h"
#include "hz6_route_backend_util.h"

Hz6RouteResult hz6_route_backend_lookup_page_table_invalid(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr);

Hz6RouteResult hz6_route_backend_lookup_page_table_invalid_probe(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr,
    size_t* probe_count);

Hz6RouteResult hz6_route_backend_lookup_page_table_exact_probe(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr,
    size_t* probe_count) {
  size_t probes = 0;
  if (!backend || !backend->exact_table.entries ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    if (probe_count) {
      *probe_count = probes;
    }
    return hz6_route_miss();
  }

  size_t start = hz6_route_hash_index(addr, backend->exact_table.capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(addr, backend->exact_table.capacity);
#else
  size_t step = 1;
#endif
  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step,
                                         backend->exact_table.capacity, i);
    const Hz6RouteEntry* entry = &backend->exact_table.entries[index];
    ++probes;
    if (!entry->active || !entry->exact_valid) {
      if (!entry->active && !entry->tombstone) {
        break;
      }
      continue;
    }
    if (addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(entry->front_id, entry->class_id,
                             entry->generation, entry->descriptor);
    }
  }

  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    const Hz6RouteEntry* entry = &backend->exact_table.entries[i];
    ++probes;
    if (!entry->active || !entry->exact_valid) {
      continue;
    }

    uintptr_t entry_begin =
        hz6_route_backend_align_down(entry->base,
                                     backend->page_granularity);
    uintptr_t entry_end =
        hz6_route_backend_align_up(entry->base + entry->bytes,
                                   backend->page_granularity);
    if (page_addr < entry_begin || page_addr >= entry_end) {
      continue;
    }

    uintptr_t object_end = entry->base + entry->bytes;
    if (addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(entry->front_id, entry->class_id,
                             entry->generation, entry->descriptor);
    }
    if (addr > entry->base && addr < object_end) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  if (probe_count) {
    *probe_count = probes;
  }
  return hz6_route_backend_lookup_page_table_invalid_probe(backend,
                                                          addr,
                                                          page_addr,
                                                          probe_count);
}

Hz6RouteResult hz6_route_backend_lookup_page_table_exact(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr) {
  return hz6_route_backend_lookup_page_table_exact_probe(backend,
                                                        addr,
                                                        page_addr,
                                                        NULL);
}
