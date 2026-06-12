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
    size_t* probe_count,
    size_t* invalid_probe_count);

static size_t hz6_route_backend_lookup_page_table_exact_seed_probe_count(
    const Hz6RouteBackend* backend,
    uintptr_t seed) {
  size_t probes = 0;
  if (!backend || !backend->exact_table.entries ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    return probes;
  }

  size_t start = hz6_route_hash_index(seed, backend->exact_table.capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(seed, backend->exact_table.capacity);
#else
  size_t step = 1;
#endif
  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step,
                                         backend->exact_table.capacity, i);
    const Hz6RouteEntry* entry = &backend->exact_table.entries[index];
    ++probes;
    if (!hz6_route_entry_active(entry) ||
        !hz6_route_entry_exact_valid(entry)) {
      if (!hz6_route_entry_active(entry) &&
          !hz6_route_entry_tombstone(entry)) {
        break;
      }
      continue;
    }
    if (seed == entry->base) {
      return probes;
    }
  }
  return probes;
}

Hz6RouteResult hz6_route_backend_lookup_page_table_exact_probe(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr,
    size_t* probe_count,
    size_t* exact_hash_probe_count,
    size_t* exact_range_probe_count,
    size_t* exact_page_seed_probe_count,
    size_t* invalid_probe_count) {
  size_t probes = 0;
  size_t exact_hash_probes = 0;
  size_t exact_range_probes = 0;
  if (!backend || !backend->exact_table.entries ||
      !hz6_route_backend_valid_granularity(backend->page_granularity)) {
    if (probe_count) {
      *probe_count = probes;
    }
    if (exact_hash_probe_count) {
      *exact_hash_probe_count = exact_hash_probes;
    }
    if (exact_range_probe_count) {
      *exact_range_probe_count = exact_range_probes;
    }
    if (exact_page_seed_probe_count) {
      *exact_page_seed_probe_count =
          hz6_route_backend_lookup_page_table_exact_seed_probe_count(backend,
                                                                     page_addr);
    }
    if (invalid_probe_count) {
      *invalid_probe_count = 0;
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
    ++exact_hash_probes;
    if (!hz6_route_entry_active(entry) ||
        !hz6_route_entry_exact_valid(entry)) {
      if (!hz6_route_entry_active(entry) &&
          !hz6_route_entry_tombstone(entry)) {
        break;
      }
      continue;
    }
    if (addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      if (exact_hash_probe_count) {
        *exact_hash_probe_count = exact_hash_probes;
      }
      if (exact_range_probe_count) {
        *exact_range_probe_count = 0;
      }
      if (exact_page_seed_probe_count) {
        *exact_page_seed_probe_count =
            hz6_route_backend_lookup_page_table_exact_seed_probe_count(
                backend, page_addr);
      }
      if (invalid_probe_count) {
        *invalid_probe_count = 0;
      }
      return hz6_route_valid(hz6_route_entry_front_id(entry),
                             hz6_route_entry_class_id(entry),
                             hz6_route_entry_generation(&backend->exact_table,
                                                        index),
                             entry->descriptor);
    }
  }

  for (size_t i = 0; i < backend->exact_table.capacity; ++i) {
    const Hz6RouteEntry* entry = &backend->exact_table.entries[i];
    ++probes;
    ++exact_range_probes;
    if (!hz6_route_entry_active(entry) ||
        !hz6_route_entry_exact_valid(entry)) {
      continue;
    }

    uintptr_t entry_begin =
        hz6_route_backend_align_down(entry->base,
                                     backend->page_granularity);
    uint32_t entry_bytes = hz6_route_entry_bytes(&backend->exact_table, i);
    uintptr_t entry_end =
        hz6_route_backend_align_up(entry->base + entry_bytes,
                                   backend->page_granularity);
    if (page_addr < entry_begin || page_addr >= entry_end) {
      continue;
    }

    uintptr_t object_end = entry->base + entry_bytes;
    if (addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      if (exact_hash_probe_count) {
        *exact_hash_probe_count = exact_hash_probes;
      }
      if (exact_range_probe_count) {
        *exact_range_probe_count = exact_range_probes;
      }
      if (exact_page_seed_probe_count) {
        *exact_page_seed_probe_count =
            hz6_route_backend_lookup_page_table_exact_seed_probe_count(
                backend, page_addr);
      }
      if (invalid_probe_count) {
        *invalid_probe_count = 0;
      }
      return hz6_route_valid(hz6_route_entry_front_id(entry),
                             hz6_route_entry_class_id(entry),
                             hz6_route_entry_generation(&backend->exact_table,
                                                        i),
                             entry->descriptor);
    }
    if (addr > entry->base && addr < object_end) {
      if (probe_count) {
        *probe_count = probes;
      }
      if (exact_hash_probe_count) {
        *exact_hash_probe_count = exact_hash_probes;
      }
      if (exact_range_probe_count) {
        *exact_range_probe_count = exact_range_probes;
      }
      if (exact_page_seed_probe_count) {
        *exact_page_seed_probe_count =
            hz6_route_backend_lookup_page_table_exact_seed_probe_count(
                backend, page_addr);
      }
      if (invalid_probe_count) {
        *invalid_probe_count = 0;
      }
      return hz6_route_invalid(hz6_route_entry_front_id(entry),
                               hz6_route_entry_class_id(entry));
    }
  }

  if (probe_count) {
    *probe_count = probes;
  }
  if (exact_hash_probe_count) {
    *exact_hash_probe_count = exact_hash_probes;
  }
  if (exact_range_probe_count) {
    *exact_range_probe_count = exact_range_probes;
  }
  if (exact_page_seed_probe_count) {
    *exact_page_seed_probe_count =
        hz6_route_backend_lookup_page_table_exact_seed_probe_count(backend,
                                                                   page_addr);
  }
  return hz6_route_backend_lookup_page_table_invalid_probe(backend,
                                                          addr,
                                                          page_addr,
                                                          probe_count,
                                                          invalid_probe_count);
}

Hz6RouteResult hz6_route_backend_lookup_page_table_exact(
    const Hz6RouteBackend* backend,
    uintptr_t addr,
    uintptr_t page_addr) {
  return hz6_route_backend_lookup_page_table_exact_probe(backend,
                                                        addr,
                                                        page_addr,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        NULL);
}
