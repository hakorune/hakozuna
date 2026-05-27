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

static Hz6RouteResult hz6_route_backend_lookup_page_table(
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

void hz6_route_backend_init_page_table(Hz6RouteBackend* backend,
                                       Hz6RouteEntry* entries,
                                       size_t capacity) {
  hz6_route_backend_init_page_table_with_granularity(
      backend, entries, capacity, HZ6_ROUTE_PAGE_GRANULARITY);
}

void hz6_route_backend_init_page_table_with_granularity(
    Hz6RouteBackend* backend,
    Hz6RouteEntry* entries,
    size_t capacity,
    size_t page_granularity) {
  if (!backend) {
    return;
  }
  backend->kind = HZ6_ROUTE_BACKEND_PAGE_TABLE;
  backend->exact_entries = entries;
  backend->page_granularity =
      hz6_route_backend_valid_granularity(page_granularity)
          ? page_granularity
          : HZ6_ROUTE_PAGE_GRANULARITY;
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
