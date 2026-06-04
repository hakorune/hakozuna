#include "hz6_route.h"

int hz6_route_register_invalid_range(Hz6RouteTable* table,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t* probe_count) {
  if (!table || !table->entries || !base || bytes == 0 ||
      bytes > (size_t)UINT32_MAX) {
    return 0;
  }

  uintptr_t base_addr = (uintptr_t)base;
  size_t start = hz6_route_hash_index(base_addr, table->capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(base_addr, table->capacity);
#else
  size_t step = 1;
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
#endif
  size_t tombstone_index = (size_t)-1;
  int tombstone_reused = 0;
  for (size_t i = 0; i < table->capacity; ++i) {
#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step, table->capacity, i);
    Hz6RouteEntry* entry = &table->entries[index];
    if (entry->active && !entry->exact_valid && entry->base == base_addr) {
#if HZ6_DIAGNOSTIC_PROBES
      if (probe_count) {
        *probe_count = probes;
      }
#else
      if (probe_count) {
        *probe_count = 0;
      }
#endif
      return 0;
    }
    if (!entry->active) {
      if (entry->tombstone) {
        if (tombstone_index == (size_t)-1) {
          tombstone_index = index;
          tombstone_reused = 1;
        }
        continue;
      }
      tombstone_index = index;
      tombstone_reused = 0;
      break;
    }
  }

#if HZ6_DIAGNOSTIC_PROBES
  if (probe_count) {
    *probe_count = probes;
  }
#else
  if (probe_count) {
    *probe_count = 0;
  }
#endif
  if (tombstone_index == (size_t)-1) {
    return 0;
  }

  Hz6RouteEntry* entry = &table->entries[tombstone_index];
  if (tombstone_reused) {
#if HZ6_DIAGNOSTIC_PROBES
    ++table->register_used_tombstone;
#endif
#if HZ6_DIAGNOSTIC_PROBES || HZ6_ROUTE_TOMBSTONE_COMPACT_L1
    if (table->tombstone_count != 0) {
      --table->tombstone_count;
    }
#endif
#if HZ6_DIAGNOSTIC_PROBES
    if (probes >= table->capacity) {
      ++table->register_full_probe_with_tombstone;
    }
#endif
  }
  entry->base = base_addr;
  entry->bytes = (uint32_t)bytes;
  entry->front_id = front_id;
  entry->class_id = class_id;
  entry->generation = 0;
  entry->descriptor = NULL;
  entry->exact_valid = 0;
  entry->active = 1;
  entry->tombstone = 0;
#if HZ6_DIAGNOSTIC_PROBES || HZ6_ROUTE_TOMBSTONE_COMPACT_L1
  ++table->active_count;
#endif
  return 1;
}

void hz6_route_unregister_invalid_range(Hz6RouteTable* table,
                                        void* base,
                                        size_t* probe_count) {
  if (!table || !table->entries || !base) {
    return;
  }
  uintptr_t base_addr = (uintptr_t)base;
  size_t start = hz6_route_hash_index(base_addr, table->capacity);
#if HZ6_ROUTE_DOUBLE_HASH_L1
  size_t step = hz6_route_probe_step(base_addr, table->capacity);
#else
  size_t step = 1;
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
#endif
  for (size_t i = 0; i < table->capacity; ++i) {
#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    size_t index = HZ6_ROUTE_PROBE_INDEX(start, step, table->capacity, i);
    Hz6RouteEntry* entry = &table->entries[index];
    if (entry->active && !entry->exact_valid && entry->base == base_addr) {
      entry->active = 0;
      entry->tombstone = 1;
#if HZ6_DIAGNOSTIC_PROBES || HZ6_ROUTE_TOMBSTONE_COMPACT_L1
      if (table->active_count != 0) {
        --table->active_count;
      }
      ++table->tombstone_count;
#endif
#if HZ6_DIAGNOSTIC_PROBES
      if (probe_count) {
        *probe_count = probes;
      }
#else
      (void)probe_count;
#endif
      return;
    }
  }
#if HZ6_DIAGNOSTIC_PROBES
  if (probe_count) {
    *probe_count = probes;
  }
#else
  if (probe_count) {
    *probe_count = 0;
  }
#endif
}
