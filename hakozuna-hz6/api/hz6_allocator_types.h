#ifndef HZ6_ALLOCATOR_TYPES_H
#define HZ6_ALLOCATOR_TYPES_H

#include "../frontcache/hz6_frontcache.h"
#include "../frontcache/hz6_size_class.h"
#include "../include/hz6.h"
#include "../include/hz6_config.h"
#include "../owner/hz6_owner.h"
#include "../policy/hz6_profiles.h"
#include "../route/hz6_route.h"
#include "../route/hz6_route_backend.h"
#include "../scavenge/hz6_scavenge.h"
#include "../source/hz6_source.h"
#include "../source/hz6_source_registry.h"
#include "../transfer/hz6_transfer.h"
#include "../transfer/hz6_transfer_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6SourceBlock {
  void* ptr;
  size_t bytes;
  Hz6SourceKind source_kind;
  int (*source_release)(void* ptr, size_t bytes);
  Hz6RouteBackend* route_backend;
  size_t ref_count;
  int active;
  int route_registered;
} Hz6SourceBlock;

typedef struct Hz6ObjectDescriptor {
  void* ptr;
  size_t bytes;
  void* source_ptr;
  size_t source_bytes;
  Hz6SourceBlock* source_block;
  uint16_t class_id;
  Hz6SourceKind source_kind;
  int (*source_release)(void* ptr, size_t bytes);
  Hz6OwnerToken owner;
  uint32_t generation;
  Hz6ObjectState state;
} Hz6ObjectDescriptor;

typedef struct Hz6LargeSpanPoolBin {
  Hz6ObjectDescriptor* descriptors[HZ6_TRANSFER_CACHE_CAPACITY];
  size_t count;
} Hz6LargeSpanPoolBin;

typedef struct Hz6LargeSpanPool {
  Hz6LargeSpanPoolBin bins[HZ6_FRONT_CACHE_CLASS_COUNT];
} Hz6LargeSpanPool;

struct Hz6Allocator {
  Hz6ProfileConfig profile;
  Hz6OwnerRecord owner;
  Hz6RouteEntry route_entries[HZ6_ROUTE_TABLE_CAPACITY];
  Hz6RouteBackend route_backend;
  Hz6TransferObject transfer_objects[HZ6_TRANSFER_CACHE_CAPACITY];
  Hz6TransferBackend transfer_backend;
  Hz6LargeSpanPool large_span_pool;
  Hz6SourceBlock source_blocks[HZ6_SOURCE_BLOCK_CAPACITY];
  Hz6ObjectDescriptor descriptors[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  Hz6SourceRegistry source_registry;
  Hz6FrontCacheEntry frontcache_entries[HZ6_FRONT_CACHE_CLASS_COUNT]
                                      [HZ6_FRONT_CACHE_BIN_CAPACITY];
  Hz6FrontCacheBin frontcache_bins[HZ6_FRONT_CACHE_CLASS_COUNT];
  Hz6StatsSnapshot stats;
};

#ifdef __cplusplus
}
#endif

#endif
