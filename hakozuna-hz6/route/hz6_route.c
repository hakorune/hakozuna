#include "hz6_route.h"

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
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = (start + i) % table->capacity;
    const Hz6RouteEntry* entry = &table->entries[index];
    ++probes;
    if (!entry->active) {
      if (!entry->tombstone) {
        break;
      }
      continue;
    }
    if (entry->exact_valid && addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(entry->front_id, entry->class_id,
                             entry->generation, entry->descriptor);
    }
  }

  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    ++probes;
    if (!entry->active || !entry->exact_valid) {
      continue;
    }
    uintptr_t end = entry->base + entry->bytes;
    if (addr == entry->base) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_valid(entry->front_id, entry->class_id,
                             entry->generation, entry->descriptor);
    }
    if (addr > entry->base && addr < end) {
      if (probe_count) {
        *probe_count = probes;
      }
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    ++probes;
    if (!entry->active || entry->exact_valid) {
      continue;
    }
    uintptr_t end = entry->base + entry->bytes;
    if (addr >= entry->base && addr < end) {
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

Hz6RouteResult hz6_route_lookup(const Hz6RouteTable* table, const void* ptr) {
  return hz6_route_lookup_probe(table, ptr, NULL);
}
