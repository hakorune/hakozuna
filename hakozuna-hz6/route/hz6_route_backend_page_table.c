#include "hz6_route_backend.h"

static uintptr_t hz6_route_backend_align_down(uintptr_t value,
                                              size_t granularity) {
  return value & ~((uintptr_t)granularity - (uintptr_t)1);
}

static uintptr_t hz6_route_backend_align_up(uintptr_t value,
                                            size_t granularity) {
  return (value + (uintptr_t)granularity - (uintptr_t)1) &
         ~((uintptr_t)granularity - (uintptr_t)1);
}

static int hz6_route_backend_valid_granularity(size_t granularity) {
  return granularity != 0 && (granularity & (granularity - (size_t)1)) == 0;
}

static Hz6RouteResult hz6_route_backend_lookup_page_table_impl(
    const Hz6RouteBackend* backend,
    const void* ptr) {
  if (!backend || !backend->exact_table.entries || !ptr ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    return hz6_route_miss();
  }

  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t page_addr =
      hz6_route_backend_align_down(addr, backend->page_granularity);

  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    const Hz6RouteEntry* entry = &backend->exact_table.entries[i];
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
      return hz6_route_valid(entry->front_id, entry->class_id,
                             entry->generation, entry->descriptor);
    }
    if (addr > entry->base && addr < object_end) {
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    const Hz6RouteEntry* entry = &backend->exact_table.entries[i];
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
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  return hz6_route_miss();
}

Hz6RouteResult hz6_route_backend_lookup_page_table(
    const Hz6RouteBackend* backend,
    const void* ptr) {
  return hz6_route_backend_lookup_page_table_impl(backend, ptr);
}
