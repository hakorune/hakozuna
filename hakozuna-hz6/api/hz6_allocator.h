#ifndef HZ6_ALLOCATOR_H
#define HZ6_ALLOCATOR_H

#include "../frontcache/hz6_frontcache.h"
#include "../include/hz6.h"
#include "../include/hz6_config.h"
#include "../owner/hz6_owner.h"
#include "../policy/hz6_profiles.h"
#include "../route/hz6_route.h"
#include "../transfer/hz6_transfer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6ObjectDescriptor {
  void* ptr;
  size_t bytes;
  uint16_t class_id;
  uint32_t generation;
  Hz6ObjectState state;
} Hz6ObjectDescriptor;

struct Hz6Allocator {
  Hz6ProfileConfig profile;
  Hz6RouteEntry route_entries[HZ6_ROUTE_TABLE_CAPACITY];
  Hz6RouteTable route_table;
  Hz6TransferObject transfer_objects[HZ6_TRANSFER_CACHE_CAPACITY];
  Hz6TransferCache transfer_cache;
  Hz6ObjectDescriptor descriptors[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries[HZ6_FRONT_CACHE_CLASS_COUNT]
                                      [HZ6_FRONT_CACHE_BIN_CAPACITY];
  Hz6FrontCacheBin frontcache_bins[HZ6_FRONT_CACHE_CLASS_COUNT];
  Hz6StatsSnapshot stats;
};

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id);

#ifdef __cplusplus
}
#endif

#endif
