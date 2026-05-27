#include "hz6_allocator_init_internal.h"

void hz6_allocator_init_state_frontcache(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            allocator->frontcache_entries[i],
                            HZ6_FRONT_CACHE_BIN_CAPACITY);
  }
}
