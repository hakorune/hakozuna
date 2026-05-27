#include "hz6_route.h"

int hz6_route_register_invalid_range(Hz6RouteTable* table,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id) {
  if (!table || !table->entries || !base || bytes == 0) {
    return 0;
  }

  uintptr_t base_addr = (uintptr_t)base;
  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (entry->active && !entry->exact_valid && entry->base == base_addr) {
      return 0;
    }
  }

  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (!entry->active) {
      entry->base = base_addr;
      entry->bytes = bytes;
      entry->front_id = front_id;
      entry->class_id = class_id;
      entry->generation = 0;
      entry->descriptor = NULL;
      entry->exact_valid = 0;
      entry->active = 1;
      return 1;
    }
  }

  return 0;
}

void hz6_route_unregister_invalid_range(Hz6RouteTable* table, void* base) {
  if (!table || !table->entries || !base) {
    return;
  }
  uintptr_t base_addr = (uintptr_t)base;
  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (entry->active && !entry->exact_valid && entry->base == base_addr) {
      entry->active = 0;
      return;
    }
  }
}
