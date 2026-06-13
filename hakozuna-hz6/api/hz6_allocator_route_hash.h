#ifndef HZ6_ALLOCATOR_ROUTE_HASH_H
#define HZ6_ALLOCATOR_ROUTE_HASH_H

#include "hz6_allocator.h"

#include <stdint.h>

static inline size_t hz6_route_directory_index(uintptr_t base,
                                               size_t capacity) {
  uintptr_t x = base >> 4;
  x ^= x >> 33;
  x *= (uintptr_t)0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= (uintptr_t)0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return (size_t)(x % capacity);
}

#endif
