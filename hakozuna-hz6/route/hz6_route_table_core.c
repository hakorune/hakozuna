#include "hz6_route.h"

size_t hz6_route_hash_index(uintptr_t base, size_t capacity) {
  if (capacity == 0) {
    return 0;
  }
  uintptr_t x = base >> 4u;
  x ^= x >> 33u;
  x *= (uintptr_t)0xff51afd7ed558ccdULL;
  x ^= x >> 33u;
  x *= (uintptr_t)0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33u;
  return (size_t)(x % capacity);
}

void hz6_route_table_init(Hz6RouteTable* table,
                          Hz6RouteEntry* entries,
                          size_t capacity) {
  if (!table) {
    return;
  }
  table->entries = entries;
  table->capacity = capacity;
  table->active_count = 0;
  table->tombstone_count = 0;
  table->register_used_tombstone = 0;
  table->register_full_probe_with_tombstone = 0;
  for (size_t i = 0; entries && i < capacity; ++i) {
    entries[i].active = 0;
    entries[i].exact_valid = 0;
    entries[i].tombstone = 0;
  }
}
