#include "hz6_allocator_init_internal.h"
#include "../fronts/midpage/hz6_midpage_front.h"

static size_t hz6_allocator_frontcache_effective_capacity(size_t class_id) {
  if (class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
    return HZ6_FRONT_CACHE_MIDPAGE_8K_BIN_CAPACITY;
  }
  if (class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    return HZ6_FRONT_CACHE_MIDPAGE_32K_BIN_CAPACITY;
  }
  return HZ6_FRONT_CACHE_BIN_CAPACITY;
}

void hz6_allocator_init_state_frontcache(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            allocator->frontcache_entries[i],
                            hz6_allocator_frontcache_effective_capacity(i));
  }
}
