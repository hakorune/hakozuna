#include "hz6_route.h"

Hz6RouteResult hz6_route_lookup_exact_probe(const Hz6RouteTable* table,
                                            const void* ptr,
                                            size_t* probe_count) {
  size_t probes = 0;
  if (!table || !table->entries || !ptr) {
    if (probe_count) {
      *probe_count = probes;
    }
    return hz6_route_miss();
  }

  uintptr_t addr = (uintptr_t)ptr;
  size_t start = hz6_route_hash_index(addr, table->capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(addr, table->capacity);
#else
  size_t step = 1;
#endif
  (void)step;
#if HZ6_ROUTE_LOOP_CARRY_L1 && HZ6_ROUTE_LINEAR_WRAP_L1 && \
    !HZ6_ROUTE_DOUBLE_HASH_L1
  for (size_t i = 0, index = start; i < table->capacity;
       ++i, index = hz6_route_linear_next_index(index, table->capacity)) {
#else
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step, table->capacity, i);
#endif
    const Hz6RouteEntry* entry = &table->entries[index];
    ++probes;
    if (!hz6_route_entry_active(entry)) {
      if (!hz6_route_entry_tombstone(entry)) {
        break;
      }
      continue;
    }
    if (hz6_route_entry_exact_valid(entry) && addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(hz6_route_entry_front_id(entry),
                             hz6_route_entry_class_id(entry),
                             hz6_route_entry_generation(table, index),
                             entry->descriptor);
    }
  }

  if (probe_count) {
    *probe_count = probes;
  }
  return hz6_route_miss();
}

Hz6RouteResult hz6_route_lookup_exact(const Hz6RouteTable* table,
                                      const void* ptr) {
  return hz6_route_lookup_exact_probe(table, ptr, NULL);
}

Hz6RouteResult hz6_route_lookup_probe(const Hz6RouteTable* table,
                                      const void* ptr,
                                      size_t* probe_count) {
  size_t probes = 0;
  if (!table || !table->entries || !ptr) {
    if (probe_count) {
      *probe_count = probes;
    }
    return hz6_route_miss();
  }

  uintptr_t addr = (uintptr_t)ptr;
  size_t start = hz6_route_hash_index(addr, table->capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(addr, table->capacity);
#else
  size_t step = 1;
#endif
  (void)step;
#if HZ6_ROUTE_LOOP_CARRY_L1 && HZ6_ROUTE_LINEAR_WRAP_L1 && \
    !HZ6_ROUTE_DOUBLE_HASH_L1
  for (size_t i = 0, index = start; i < table->capacity;
       ++i, index = hz6_route_linear_next_index(index, table->capacity)) {
#else
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step, table->capacity, i);
#endif
    const Hz6RouteEntry* entry = &table->entries[index];
    ++probes;
    if (!hz6_route_entry_active(entry)) {
      if (!hz6_route_entry_tombstone(entry)) {
        break;
      }
      continue;
    }
    if (hz6_route_entry_exact_valid(entry) && addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(hz6_route_entry_front_id(entry),
                             hz6_route_entry_class_id(entry),
                             hz6_route_entry_generation(table, index),
                             entry->descriptor);
    }
  }

  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    ++probes;
    if (!hz6_route_entry_active(entry) ||
        !hz6_route_entry_exact_valid(entry)) {
      continue;
    }
    uintptr_t end = entry->base + hz6_route_entry_bytes(table, i);
    if (addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(hz6_route_entry_front_id(entry),
                             hz6_route_entry_class_id(entry),
                             hz6_route_entry_generation(table, i),
                             entry->descriptor);
    }
    if (addr > entry->base && addr < end) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_invalid(hz6_route_entry_front_id(entry),
                               hz6_route_entry_class_id(entry));
    }
  }

  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    ++probes;
    if (!hz6_route_entry_active(entry) ||
        hz6_route_entry_exact_valid(entry)) {
      continue;
    }
    uintptr_t end = entry->base + hz6_route_entry_bytes(table, i);
    if (addr >= entry->base && addr < end) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_invalid(hz6_route_entry_front_id(entry),
                               hz6_route_entry_class_id(entry));
    }
  }

  if (probe_count) {
    *probe_count = probes;
  }
  return hz6_route_miss();
}

Hz6RouteResult hz6_route_lookup(const Hz6RouteTable* table, const void* ptr) {
  return hz6_route_lookup_probe(table, ptr, NULL);
}
