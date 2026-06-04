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

#if HZ6_ROUTE_DOUBLE_HASH_L1
static size_t hz6_route_gcd_size(size_t a, size_t b) {
  while (b != 0) {
    size_t r = a % b;
    a = b;
    b = r;
  }
  return a;
}
#endif

size_t hz6_route_probe_step(uintptr_t base, size_t capacity) {
  if (capacity <= 1) {
    return 1;
  }
#if HZ6_ROUTE_DOUBLE_HASH_L1
  uintptr_t x = base >> 12u;
  x ^= x >> 29u;
  x *= (uintptr_t)0x9e3779b97f4a7c15ULL;
  x ^= x >> 32u;
  size_t step = (size_t)(x % (capacity - 1u)) + 1u;
  while (hz6_route_gcd_size(step, capacity) != 1u) {
    ++step;
    if (step >= capacity) {
      step = 1u;
    }
  }
  return step;
#else
  (void)base;
  return 1;
#endif
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
