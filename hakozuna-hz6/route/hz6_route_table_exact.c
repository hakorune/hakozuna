#include "hz6_route.h"

int hz6_route_register_exact(Hz6RouteTable* table,
                             void* base,
                             size_t bytes,
                             uint16_t front_id,
                             uint16_t class_id,
                             uint32_t generation,
                             void* descriptor) {
  if (!table || !table->entries || !base || bytes == 0 || !descriptor) {
    return 0;
  }

  uintptr_t base_addr = (uintptr_t)base;
  size_t start = hz6_route_hash_index(base_addr, table->capacity);
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = (start + i) % table->capacity;
    Hz6RouteEntry* entry = &table->entries[index];
    if (entry->active && entry->exact_valid && entry->base == base_addr) {
      return 0;
    }
    if (!entry->active) {
      entry->base = base_addr;
      entry->bytes = bytes;
      entry->front_id = front_id;
      entry->class_id = class_id;
      entry->generation = generation;
      entry->descriptor = descriptor;
      entry->exact_valid = 1;
      entry->active = 1;
      return 1;
    }
  }

  return 0;
}

void hz6_route_unregister_exact(Hz6RouteTable* table, void* base) {
  if (!table || !table->entries || !base) {
    return;
  }
  uintptr_t base_addr = (uintptr_t)base;
  size_t start = hz6_route_hash_index(base_addr, table->capacity);
  for (size_t i = 0; i < table->capacity; ++i) {
    size_t index = (start + i) % table->capacity;
    Hz6RouteEntry* entry = &table->entries[index];
    if (entry->active && entry->exact_valid && entry->base == base_addr) {
      entry->active = 0;
      return;
    }
  }
}
