#ifndef HZ6_ALLOCATOR_TYPES_H
#define HZ6_ALLOCATOR_TYPES_H

#include <stdint.h>

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

typedef int (*Hz6SourceReleaseFn)(void* ptr, size_t bytes);

typedef struct Hz6SourceBlock {
  void* ptr;
  size_t bytes;
  Hz6SourceKind source_kind;
  Hz6SourceReleaseFn source_release;
  Hz6RouteBackend* route_backend;
  size_t ref_count;
  size_t run_slot_bytes;
  uint16_t run_class_id;
  uint16_t run_slot_count;
  uint16_t run_used_count;
  uint16_t run_next_hint;
  uint64_t run_used_bits[HZ6_SOURCE_RUN_BITMAP_WORDS];
  int active;
  int route_registered;
  int run_active;
} Hz6SourceBlock;

typedef struct Hz6ObjectDescriptor {
#if !HZ6_DESCRIPTOR_NO_BACKPTR_L1
  struct Hz6Allocator* allocator;
#endif
  void* ptr;
#if !HZ6_THIN_DESCRIPTOR_L1
  void* source_ptr;
#endif
  Hz6SourceBlock* source_block;
#if !HZ6_THIN_DESCRIPTOR_L1
  Hz6SourceReleaseFn source_release;
#endif
#if !HZ6_DESCRIPTOR_SIDE_OWNER16_L1
  Hz6OwnerToken owner;
#endif
  uint32_t bytes;
#if !HZ6_THIN_DESCRIPTOR_L1
  uint32_t source_bytes;
#endif
  uint32_t generation;
#if HZ6_THIN_DESCRIPTOR_L1
  uint32_t cold_index;
#endif
  uint16_t class_id;
  uint8_t source_kind;
  uint8_t state;
} Hz6ObjectDescriptor;

#if HZ6_THIN_DESCRIPTOR_L1
typedef struct Hz6DescriptorColdSource {
  void* source_ptr;
  Hz6SourceReleaseFn source_release;
  uint32_t source_bytes;
  uint32_t generation;
  uint8_t source_kind;
  uint8_t active;
  uint16_t reserved;
} Hz6DescriptorColdSource;
#endif

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
  size_t next_descriptor_index;
  size_t descriptor_available_count;
  Hz6ObjectDescriptor descriptors[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1
  uint32_t descriptor_side_owner16[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#endif
#if HZ6_THIN_DESCRIPTOR_L1
  size_t next_descriptor_cold_index;
  size_t descriptor_cold_source_current;
  size_t descriptor_cold_source_max;
  Hz6DescriptorColdSource descriptor_cold_sources[HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY];
#endif
  size_t descgov_detached_budget_used;
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
