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
