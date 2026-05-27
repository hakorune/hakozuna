#ifndef HZ6_ALLOCATOR_H
#define HZ6_ALLOCATOR_H

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
  size_t ref_count;
  int active;
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

struct Hz6Allocator {
  Hz6ProfileConfig profile;
  Hz6OwnerRecord owner;
  Hz6RouteEntry route_entries[HZ6_ROUTE_TABLE_CAPACITY];
  Hz6RouteBackend route_backend;
  Hz6TransferObject transfer_objects[HZ6_TRANSFER_CACHE_CAPACITY];
  Hz6TransferBackend transfer_backend;
  Hz6SourceBlock source_blocks[HZ6_SOURCE_BLOCK_CAPACITY];
  Hz6ObjectDescriptor descriptors[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  Hz6SourceRegistry source_registry;
  Hz6FrontCacheEntry frontcache_entries[HZ6_FRONT_CACHE_CLASS_COUNT]
                                      [HZ6_FRONT_CACHE_BIN_CAPACITY];
  Hz6FrontCacheBin frontcache_bins[HZ6_FRONT_CACHE_CLASS_COUNT];
  Hz6StatsSnapshot stats;
};

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id);

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator);

int hz6_allocator_activate_descriptor(Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner);

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor);

Hz6SourceBlock* hz6_allocator_create_source_block(
    Hz6Allocator* allocator,
    size_t bytes,
    const Hz6OsMemoryOps* source_ops,
    Hz6SourceKind source_kind);

int hz6_allocator_retain_source_block(Hz6SourceBlock* block);

int hz6_allocator_release_source_block(Hz6SourceBlock* block);

void hz6_allocator_mark_owner_dead(Hz6Allocator* allocator);

int hz6_allocator_release_orphan(Hz6Allocator* allocator, void* ptr);

int hz6_allocator_adopt_orphan(Hz6Allocator* adopter,
                               Hz6Allocator* source,
                               void* ptr);

size_t hz6_allocator_scavenge_orphans(Hz6Allocator* allocator,
                                      size_t max_bytes);

size_t hz6_allocator_scavenge_local_free(Hz6Allocator* allocator,
                                         size_t max_bytes);

size_t hz6_allocator_scavenge_profile(Hz6Allocator* allocator);

size_t hz6_allocator_drain_remote_pending(Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
