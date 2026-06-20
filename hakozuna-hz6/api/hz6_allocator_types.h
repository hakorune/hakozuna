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
#include "hz6_allocator_owner_inbox_storage_provider.h"

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
#if HZ6_SOURCE_RUN_INLINE_META_L1
  size_t run_slot_bytes;
  uint16_t run_front_id;
  uint16_t run_class_id;
  uint16_t run_slot_count;
  uint16_t run_used_count;
  uint16_t run_next_hint;
  uint64_t run_used_bits[HZ6_SOURCE_RUN_BITMAP_WORDS];
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  uint32_t* run_descriptor_indices;
#else
  uint32_t run_descriptor_indices[HZ6_SOURCE_RUN_MAX_SLOTS];
#endif
#endif
#endif
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  struct Hz6Allocator* owner_source_storage_allocator;
#endif
#if !HZ6_SOURCE_BLOCK_PACKED_FLAGS_L1
  int active;
  int route_registered;
  int route_shared;
  int run_active;
  int decommitted;
#endif
} Hz6SourceBlock;

#if HZ6_PAGE_KIND_FREE_SELECTOR_ACTIVE_L1
typedef struct Hz6PageKindEntry {
  uintptr_t page;
  uint16_t kind;
} Hz6PageKindEntry;
#endif

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
#define HZ6_SOURCE_BLOCK_FLAG_DECOMMITTED ((uint16_t)0x1000u)
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

static inline int hz6_source_block_decommitted(const Hz6SourceBlock* block) {
  return block &&
         (block->source_state_flags & HZ6_SOURCE_BLOCK_FLAG_DECOMMITTED) != 0;
}

static inline void hz6_source_block_set_decommitted(Hz6SourceBlock* block,
                                                    int decommitted) {
  if (!block) {
    return;
  }
  if (decommitted) {
    block->source_state_flags |= HZ6_SOURCE_BLOCK_FLAG_DECOMMITTED;
  } else {
    block->source_state_flags &=
        (uint16_t)~HZ6_SOURCE_BLOCK_FLAG_DECOMMITTED;
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

static inline int hz6_source_block_decommitted(const Hz6SourceBlock* block) {
  return block && block->decommitted;
}

static inline void hz6_source_block_set_decommitted(Hz6SourceBlock* block,
                                                    int decommitted) {
  if (block) {
    block->decommitted = decommitted ? 1 : 0;
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
  size_t bytes_current;
} Hz6LargeSpanPoolBin;

typedef struct Hz6LargeSpanPool {
  Hz6LargeSpanPoolBin bins[HZ6_FRONT_CACHE_CLASS_COUNT];
  size_t bytes_current;
} Hz6LargeSpanPool;

#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
typedef struct Hz6SourceBlockRangeIndexEntry {
  uintptr_t page;
  uint32_t block_index;
  uint8_t active;
  uint8_t reserved[3];
} Hz6SourceBlockRangeIndexEntry;
#endif

#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
typedef struct Hz6ToySmallActiveMapEntry {
  void* ptr;
  Hz6ObjectDescriptor* descriptor;
  uint32_t generation;
  uint16_t class_id;
  uint16_t front_id;
} Hz6ToySmallActiveMapEntry;
#endif

#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
typedef struct Hz6MidPageActiveMapEntry {
  void* ptr;
  Hz6ObjectDescriptor* descriptor;
  uint32_t generation;
  uint16_t class_id;
  uint16_t reserved;
} Hz6MidPageActiveMapEntry;
#endif

#if HZ6_ROUTE_LAST_HIT_CACHE_L1
typedef struct Hz6RouteLastHitCache {
  void* base;
  void* descriptor;
  uint32_t generation;
  uint16_t front_id;
  uint16_t class_id;
} Hz6RouteLastHitCache;
#endif

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
typedef struct Hz6RemotePendingClassInbox {
  atomic_flag lock;
  uint32_t head_index[HZ6_REMOTE_PENDING_FRONT_COUNT];
  uint32_t key_count[HZ6_REMOTE_PENDING_FRONT_COUNT];
  size_t total_count;
} Hz6RemotePendingClassInbox;

typedef enum Hz6RemotePendingSlotState {
  HZ6_REMOTE_PENDING_SLOT_NONE = 0,
  HZ6_REMOTE_PENDING_SLOT_QUEUED = 1,
  HZ6_REMOTE_PENDING_SLOT_CLAIMED = 2
} Hz6RemotePendingSlotState;

#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
typedef struct Hz6RemotePendingExternalTicket {
  void* ptr;
  Hz6ObjectDescriptor* descriptor;
  uint32_t generation;
  uint32_t bytes;
  Hz6OwnerToken owner_token;
  Hz6OwnerToken descriptor_storage_owner_token;
  uint32_t next;
  uint16_t front_id;
  uint16_t class_id;
  uint8_t state;
  uint8_t reserved;
} Hz6RemotePendingExternalTicket;
#endif

typedef struct Hz6RemotePendingStorage {
  Hz6RemotePendingClassInbox
      remote_pending_inbox[HZ6_FRONT_CACHE_CLASS_COUNT];
  uint32_t remote_pending_next[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint8_t remote_pending_slot_state[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  void* remote_pending_published_ptr[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint32_t remote_pending_published_generation[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint32_t remote_pending_published_bytes[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint16_t remote_pending_published_front_id[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint16_t remote_pending_published_class_id[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  Hz6OwnerToken remote_pending_owner_token[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  Hz6RemotePendingExternalTicket
      remote_pending_external_tickets
          [HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY];
  atomic_flag remote_pending_external_lock;
  uint32_t remote_pending_external_free_head;
  uint32_t remote_pending_external_head
      [HZ6_REMOTE_PENDING_FRONT_COUNT][HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint64_t remote_pending_external_nonempty_mask;
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  uint32_t remote_pending_external_dup_index
      [HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY];
#endif
#endif
  _Atomic size_t remote_pending_current;
  _Atomic size_t remote_pending_queued_current;
  _Atomic size_t remote_pending_claimed_current;
  _Atomic size_t remote_pending_total_current;
  _Atomic uint64_t remote_pending_nonempty_mask;
} Hz6RemotePendingStorage;
#endif

struct Hz6Allocator {
  Hz6ProfileConfig profile;
  Hz6OwnerRecord owner;
  Hz6RouteEntry route_entries[HZ6_ROUTE_TABLE_CAPACITY];
#if HZ6_ROUTE_PACKED_META_L1
  Hz6RouteBytesStorage route_bytes[HZ6_ROUTE_TABLE_CAPACITY];
#endif
  Hz6RouteBackend route_backend;
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  atomic_flag route_domain_lock;
#if HZ6_ROUTE_DOMAIN_RWLOCK_L1
  atomic_flag route_domain_writer;
  _Atomic unsigned int route_domain_writer_pending;
  _Atomic unsigned int route_domain_readers;
#endif
  _Atomic unsigned int route_compact_requested;
#endif
#if HZ6_ROUTE_LAST_HIT_CACHE_L1
  Hz6RouteLastHitCache route_last_hit;
#endif
  Hz6TransferObject transfer_objects[HZ6_TRANSFER_CACHE_CAPACITY];
  Hz6TransferBackend transfer_backend;
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  _Atomic uint32_t transfer_phase_demand_epoch;
  _Atomic uint32_t
      transfer_phase_class_demand_epoch[HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint32_t
      transfer_phase_class_occupancy[HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint32_t
      transfer_phase_destination_class_occupancy[HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint32_t
      transfer_phase_origin_fallback_class_occupancy[HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint32_t transfer_phase_destination_occupancy;
  _Atomic uint32_t transfer_phase_origin_fallback_occupancy;
  _Atomic uint32_t transfer_phase_producer_bucket_occupancy[16];
#endif
#if HZ6_REMOTE_FREE_OVERFLOW_L1
  Hz6TransferObject remote_free_overflow_objects
      [HZ6_REMOTE_FREE_OVERFLOW_CAPACITY];
  Hz6TransferCache remote_free_overflow_cache;
#endif
  Hz6LargeSpanPool large_span_pool;
  Hz6SourceBlock source_blocks[HZ6_SOURCE_BLOCK_CAPACITY];
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  Hz6SourceBlockRangeIndexEntry
      source_block_range_index[HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY];
#endif
#if HZ6_SMALL_RUN_ROUTE_ARMED_L1
  size_t small_run_route_range_registered;
#endif
  size_t next_descriptor_index;
  size_t descriptor_available_count;
  Hz6ObjectDescriptor descriptors[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
  atomic_flag remote_pending_storage_lock;
  Hz6OwnerInboxStorageBlock remote_pending_storage_block;
  _Atomic(Hz6RemotePendingStorage*) remote_pending_storage;
#else
  Hz6RemotePendingClassInbox
      remote_pending_inbox[HZ6_FRONT_CACHE_CLASS_COUNT];
  uint32_t remote_pending_next[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint8_t remote_pending_slot_state[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  void* remote_pending_published_ptr[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint32_t remote_pending_published_generation[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint32_t remote_pending_published_bytes[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint16_t remote_pending_published_front_id[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  uint16_t remote_pending_published_class_id[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
  Hz6OwnerToken remote_pending_owner_token[HZ6_OBJECT_DESCRIPTOR_CAPACITY];
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  Hz6RemotePendingExternalTicket
      remote_pending_external_tickets
          [HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY];
  atomic_flag remote_pending_external_lock;
  uint32_t remote_pending_external_free_head;
  uint32_t remote_pending_external_head
      [HZ6_REMOTE_PENDING_FRONT_COUNT][HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint64_t remote_pending_external_nonempty_mask;
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
  uint32_t remote_pending_external_dup_index
      [HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY];
#endif
#endif
  _Atomic size_t remote_pending_current;
  _Atomic size_t remote_pending_queued_current;
  _Atomic size_t remote_pending_claimed_current;
  _Atomic size_t remote_pending_total_current;
  _Atomic uint64_t remote_pending_nonempty_mask;
#endif
#endif
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
#if HZ6_MIDPAGE_32K_COLD_RETIRE_L1
  size_t midpage_32k_cold_retire_cursor;
#endif
#if HZ6_MIDPAGE_32K_COLD_PURGE_L1
  size_t midpage_32k_cold_purge_cursor;
#endif
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  size_t toy_small_active_map_current;
#if HZ6_TOY_SMALL_ACTIVE_MAP_ADDR_ENVELOPE_L1
  uintptr_t toy_small_active_map_min_addr;
  uintptr_t toy_small_active_map_max_addr;
#endif
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_EXTERNAL_L1
  Hz6ToySmallActiveMapEntry* toy_small_active_map;
#else
  Hz6ToySmallActiveMapEntry
      toy_small_active_map[HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY];
#endif
#endif
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  size_t midpage_active_map_current;
#if HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1
  uintptr_t midpage_active_map_min_addr;
  uintptr_t midpage_active_map_max_addr;
#endif
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2
  Hz6MidPageActiveMapEntry* midpage_active_map;
#else
  Hz6MidPageActiveMapEntry
      midpage_active_map[HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY];
#endif
#endif
#if HZ6_PAGE_KIND_FREE_SELECTOR_ACTIVE_L1
  Hz6PageKindEntry page_kind_selector[HZ6_PAGE_KIND_FREE_SELECTOR_CAPACITY];
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t diagnostic_descriptor_live_current;
  size_t diagnostic_source_block_active_current;
  size_t diagnostic_frontcache_total_current;
#endif
  Hz6SourceRegistry source_registry;
#if HZ6_FRONT_CACHE_CLASS_STORAGE_TRIM_L1
  Hz6FrontCacheEntry frontcache_entries_c0
      [HZ6_FRONT_CACHE_CLASS0_STORAGE_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries_c1
      [HZ6_FRONT_CACHE_CLASS1_STORAGE_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries_c2
      [HZ6_FRONT_CACHE_CLASS2_STORAGE_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries_c3
      [HZ6_FRONT_CACHE_CLASS3_STORAGE_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries_c4
      [HZ6_FRONT_CACHE_CLASS4_STORAGE_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries_c5
      [HZ6_FRONT_CACHE_CLASS5_STORAGE_CAPACITY];
  Hz6FrontCacheEntry frontcache_entries_cold
      [HZ6_FRONT_CACHE_CLASS_COUNT > 6u ? HZ6_FRONT_CACHE_CLASS_COUNT - 6u
                                        : 1u]
      [HZ6_FRONT_CACHE_COLD_CLASS_STORAGE_CAPACITY];
#else
  Hz6FrontCacheEntry frontcache_entries[HZ6_FRONT_CACHE_CLASS_COUNT]
                                      [HZ6_FRONT_CACHE_BIN_CAPACITY];
#endif
  Hz6FrontCacheBin frontcache_bins[HZ6_FRONT_CACHE_CLASS_COUNT];
  _Atomic uint32_t remote_backpressure_policy_epoch;
  Hz6StatsSnapshot stats;
};

#ifdef __cplusplus
}
#endif

#endif
