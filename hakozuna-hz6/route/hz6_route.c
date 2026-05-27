#include "hz6_route.h"

void hz6_route_table_init(Hz6RouteTable* table,
                          Hz6RouteEntry* entries,
                          size_t capacity) {
  if (!table) {
    return;
  }
  table->entries = entries;
  table->capacity = capacity;
  for (size_t i = 0; entries && i < capacity; ++i) {
    entries[i].active = 0;
    entries[i].exact_valid = 0;
  }
}

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
  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (entry->active && entry->exact_valid && entry->base == base_addr) {
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
      entry->generation = generation;
      entry->descriptor = descriptor;
      entry->exact_valid = 1;
      entry->active = 1;
      return 1;
    }
  }

  return 0;
}

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

void hz6_route_unregister_exact(Hz6RouteTable* table, void* base) {
  if (!table || !table->entries || !base) {
    return;
  }
  uintptr_t base_addr = (uintptr_t)base;
  for (size_t i = 0; i < table->capacity; ++i) {
    Hz6RouteEntry* entry = &table->entries[i];
    if (entry->active && entry->exact_valid && entry->base == base_addr) {
      entry->active = 0;
      return;
    }
  }
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

Hz6RouteResult hz6_route_lookup(const Hz6RouteTable* table, const void* ptr) {
  if (!table || !table->entries || !ptr) {
    return hz6_route_miss();
  }

  uintptr_t addr = (uintptr_t)ptr;
  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    if (!entry->active || !entry->exact_valid) {
      continue;
    }
    uintptr_t end = entry->base + entry->bytes;
    if (addr == entry->base) {
      return hz6_route_valid(entry->front_id, entry->class_id,
                             entry->generation, entry->descriptor);
    }
    if (addr > entry->base && addr < end) {
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    if (!entry->active || entry->exact_valid) {
      continue;
    }
    uintptr_t end = entry->base + entry->bytes;
    if (addr >= entry->base && addr < end) {
      return hz6_route_invalid(entry->front_id, entry->class_id);
    }
  }

  return hz6_route_miss();
}
