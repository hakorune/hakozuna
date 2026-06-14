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

#if HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1
static size_t hz6_allocator_frontcache_storage_capacity(size_t class_id) {
  switch (class_id) {
    case 0:
      return HZ6_FRONT_CACHE_CLASS0_STORAGE_CAPACITY;
    case 1:
      return HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY;
    case 2:
      return HZ6_FRONT_CACHE_CLASS2_STORAGE_CAPACITY;
    case 3:
      return HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY;
    case HZ6_MIDPAGE_8K_CLASS_ID:
      return HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY;
    case HZ6_MIDPAGE_32K_CLASS_ID:
      return HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY;
    default:
      return HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY;
  }
}

static Hz6FrontCacheEntry* hz6_allocator_frontcache_storage(
    Hz6Allocator* allocator,
    size_t class_id) {
  switch (class_id) {
    case 0:
      return allocator->frontcache_entries_c0;
    case 1:
      return allocator->frontcache_entries_c1;
    case 2:
      return allocator->frontcache_entries_c2;
    case 3:
      return allocator->frontcache_entries_c3;
    case HZ6_MIDPAGE_8K_CLASS_ID:
      return allocator->frontcache_entries_c4;
    case HZ6_MIDPAGE_32K_CLASS_ID:
      return allocator->frontcache_entries_c5;
    default:
      if (class_id < 6u || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
        return allocator->frontcache_entries_cold[0];
      }
      return allocator->frontcache_entries_cold[class_id - 6u];
  }
}
#endif

void hz6_allocator_init_state_frontcache(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
#if HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1
    size_t capacity = hz6_allocator_frontcache_effective_capacity(i);
    size_t storage_capacity = hz6_allocator_frontcache_storage_capacity(i);
    if (capacity > storage_capacity) {
      capacity = storage_capacity;
    }
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            hz6_allocator_frontcache_storage(allocator, i),
                            capacity);
#else
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            allocator->frontcache_entries[i],
                            hz6_allocator_frontcache_effective_capacity(i));
#endif
  }
}
