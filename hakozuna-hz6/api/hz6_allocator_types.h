#ifndef HZ6_ALLOCATOR_TYPES_H
#define HZ6_ALLOCATOR_TYPES_H

#include <stdatomic.h>
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

struct Hz6Allocator;

typedef int (*Hz6SourceReleaseFn)(void* ptr, size_t bytes);

typedef struct Hz6SourceBlock {
  void* ptr;
  size_t bytes;
#if HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1
  uint16_t source_state_flags;
#else
  Hz6SourceKind source_kind;
#endif
  Hz6SourceReleaseFn source_release;
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
  Hz6RouteBackend* route_backend;
#endif
  _Atomic size_t ref_count;
  size_t run_slot_bytes;
  uint16_t run_class_id;
  uint16_t run_slot_count;
  uint16_t run_used_count;
  uint16_t run_next_hint;
  uint64_t run_used_bits[HZ6_SOURCE_RUN_BITMAP_WORDS];
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  struct Hz6Allocator* owner_source_storage_allocator;
#endif
#if !HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1
  int active;
  int route_registered;
  int route_shared;
  int run_active;
#endif
} Hz6SourceBlock;

static inline size_t hz6_source_block_ref_count(
    const Hz6SourceBlock* block) {
  return block ? atomic_load_explicit(&block->ref_count,
                                      memory_order_acquire)
               : 0;
}

#if HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1
#define HZ6_SOURCE_BLOCK_FLAG_ACTIVE ((uint16_t)0x0100u)
#define HZ6_SOURCE_BLOCK_FLAG_ROUTE_REGISTERED ((uint16_t)0x0200u)
#define HZ6_SOURCE_BLOCK_FLAG_RUN_ACTIVE ((uint16_t)0x0400u)
#define HZ6_SOURCE_BLOCK_FLAG_ROUTE_SHARED ((uint16_t)0x0800u)
#define HZ6_SOURCE_BLOCK_SOURCE_KIND_MASK ((uint16_t)0x00ffu)

static inline Hz6SourceKind hz6_source_block_source_kind(
    const Hz6SourceBlock* block) {
  return block ? (Hz6SourceKind)(block->source_state_flags &
                                HZ6_SOURCE_BLOCK_SOURCE_KIND_MASK)
               : HZ6_SOURCE_NONE;
}

static inline void hz6_source_block_set_source_kind(Hz6SourceBlock* block,
                                                    Hz6SourceKind kind) {
  if (!block) {
    return;
  }
  block->source_state_flags =
      (uint16_t)((block->source_state_flags &
                  (uint16_t)~HZ6_SOURCE_BLOCK_SOURCE_KIND_MASK) |
                 ((uint16_t)kind & HZ6_SOURCE_BLOCK_SOURCE_KIND_MASK));
}

static inline int hz6_source_block_active(const Hz6SourceBlock* block) {
  return block &&
         (block->source_state_flags & HZ6_SOURCE_BLOCK_FLAG_ACTIVE) != 0;
}

static inline void hz6_source_block_set_active(Hz6SourceBlock* block,
                                               int active) {
  if (!block) {
    return;
  }
  if (active) {
    block->source_state_flags |= HZ6_SOURCE_BLOCK_FLAG_ACTIVE;
  } else {
    block->source_state_flags &=
        (uint16_t)~HZ6_SOURCE_BLOCK_FLAG_ACTIVE;
  }
}

static inline int hz6_source_block_route_registered(
    const Hz6SourceBlock* block) {
  return block &&
         (block->source_state_flags &
          HZ6_SOURCE_BLOCK_FLAG_ROUTE_REGISTERED) != 0;
}

static inline void hz6_source_block_set_route_registered(
    Hz6SourceBlock* block,
    int registered) {
  if (!block) {
    return;
  }
  if (registered) {
    block->source_state_flags |= HZ6_SOURCE_BLOCK_FLAG_ROUTE_REGISTERED;
  } else {
    block->source_state_flags &=
        (uint16_t)~HZ6_SOURCE_BLOCK_FLAG_ROUTE_REGISTERED;
  }
}

static inline int hz6_source_block_route_shared(
    const Hz6SourceBlock* block) {
  return block &&
         (block->source_state_flags & HZ6_SOURCE_BLOCK_FLAG_ROUTE_SHARED) != 0;
}

static inline void hz6_source_block_set_route_shared(
    Hz6SourceBlock* block,
    int shared) {
  if (!block) {
    return;
  }
  if (shared) {
    block->source_state_flags |= HZ6_SOURCE_BLOCK_FLAG_ROUTE_SHARED;
  } else {
    block->source_state_flags &=
        (uint16_t)~HZ6_SOURCE_BLOCK_FLAG_ROUTE_SHARED;
  }
}

static inline int hz6_source_block_run_active(const Hz6SourceBlock* block) {
  return block &&
         (block->source_state_flags & HZ6_SOURCE_BLOCK_FLAG_RUN_ACTIVE) != 0;
}

static inline void hz6_source_block_set_run_active(Hz6SourceBlock* block,
                                                   int active) {
  if (!block) {
    return;
  }
  if (active) {
    block->source_state_flags |= HZ6_SOURCE_BLOCK_FLAG_RUN_ACTIVE;
  } else {
    block->source_state_flags &=
        (uint16_t)~HZ6_SOURCE_BLOCK_FLAG_RUN_ACTIVE;
  }
}
#else
static inline Hz6SourceKind hz6_source_block_source_kind(
    const Hz6SourceBlock* block) {
  return block ? block->source_kind : HZ6_SOURCE_NONE;
}

static inline void hz6_source_block_set_source_kind(Hz6SourceBlock* block,
                                                    Hz6SourceKind kind) {
  if (block) {
    block->source_kind = kind;
  }
}

static inline int hz6_source_block_active(const Hz6SourceBlock* block) {
  return block && block->active;
}

static inline void hz6_source_block_set_active(Hz6SourceBlock* block,
                                               int active) {
  if (block) {
    block->active = active ? 1 : 0;
  }
}

static inline int hz6_source_block_route_registered(
    const Hz6SourceBlock* block) {
  return block && block->route_registered;
}

static inline void hz6_source_block_set_route_registered(
    Hz6SourceBlock* block,
    int registered) {
  if (block) {
    block->route_registered = registered ? 1 : 0;
  }
}

static inline int hz6_source_block_route_shared(
    const Hz6SourceBlock* block) {
  return block && block->route_shared;
}

static inline void hz6_source_block_set_route_shared(
    Hz6SourceBlock* block,
    int shared) {
  if (block) {
    block->route_shared = shared ? 1 : 0;
  }
}

static inline int hz6_source_block_run_active(const Hz6SourceBlock* block) {
  return block && block->run_active;
}

static inline void hz6_source_block_set_run_active(Hz6SourceBlock* block,
                                                   int active) {
  if (block) {
    block->run_active = active ? 1 : 0;
  }
}
#endif

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
#if !(HZ6_DESCRIPTOR_SIDE_OWNER16_L1 || HZ6_DESCRIPTOR_STORAGE_OWNER16_L1)
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
#if HZ6_ROUTE_PACKED_META_L1
  Hz6RouteBytesStorage route_bytes[HZ6_ROUTE_TABLE_CAPACITY];
#endif
  Hz6RouteBackend route_backend;
  Hz6TransferObject transfer_objects[HZ6_TRANSFER_CACHE_CAPACITY];
  Hz6TransferBackend transfer_backend;
  Hz6LargeSpanPool large_span_pool;
  Hz6SourceBlock source_blocks[HZ6_SOURCE_BLOCK_CAPACITY];
  size_t next_descriptor_index;
  size_t descriptor_available_count;
  Hz6ObjectDescriptor descriptors[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1 || HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
  uint32_t descriptor_side_owner16[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#endif
#if HZ6_THIN_DESCRIPTOR_L1
  size_t next_descriptor_cold_index;
  size_t descriptor_cold_source_current;
  size_t descriptor_cold_source_max;
  Hz6DescriptorColdSource descriptor_cold_sources[HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY];
#endif
  size_t descgov_detached_budget_used;
  size_t depot_descriptor_rehome_budget_used;
#if HZ6_DIAGNOSTIC_PROBES
  size_t diagnostic_descriptor_live_current;
  size_t diagnostic_source_block_active_current;
  size_t diagnostic_frontcache_total_current;
#endif
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
