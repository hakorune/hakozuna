#include "hz6_allocator.h"

size_t hz6_allocator_frontcache_count(const Hz6Allocator* allocator,
                                      uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->frontcache_bins[class_id].count;
}

size_t hz6_allocator_frontcache_capacity(const Hz6Allocator* allocator,
                                         uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->frontcache_bins[class_id].capacity;
}
