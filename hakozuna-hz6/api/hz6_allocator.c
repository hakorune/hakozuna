#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"
#include "../fronts/toy/hz6_toy_front.h"

static uint32_t g_hz6_allocator_owner_slot_seed = 1;

static Hz6OwnerToken hz6_owner_token_none(void) {
  Hz6OwnerToken token;
  token.slot = 0;
  token.generation = 0;
  return token;
}

static size_t hz6_allocator_descriptor_scavenge_cost(
    const Hz6ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return 0;
  }
  return descriptor->source_block ? descriptor->bytes
                                  : (descriptor->source_bytes
                                         ? descriptor->source_bytes
                                         : descriptor->bytes);
}

Hz6OwnerToken hz6_allocator_owner_token(const Hz6Allocator* allocator) {
  return allocator ? allocator->owner.token : hz6_owner_token_none();
}

int hz6_allocator_debug_set_owner_slot(Hz6Allocator* allocator,
                                       uint32_t slot) {
  if (!allocator) {
    return 0;
  }
  allocator->owner.token.slot = slot;
  return 1;
}

int hz6_allocator_frontcache_push(Hz6Allocator* allocator,
                                  uint16_t class_id,
                                  Hz6FrontCacheEntry entry) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return hz6_frontcache_push(&allocator->frontcache_bins[class_id], entry);
}

int hz6_allocator_frontcache_pop(Hz6Allocator* allocator,
                                 uint16_t class_id,
                                 Hz6FrontCacheEntry* out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return hz6_frontcache_pop(&allocator->frontcache_bins[class_id], out);
}

int hz6_allocator_frontcache_remove(Hz6Allocator* allocator,
                                    uint16_t class_id,
                                    void* ptr,
                                    void* descriptor,
                                    uint32_t generation,
                                    Hz6FrontCacheEntry* removed) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return hz6_frontcache_remove(&allocator->frontcache_bins[class_id],
                               ptr,
                               descriptor,
                               generation,
                               removed);
}

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

void hz6_allocator_mark_owner_dead(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  Hz6OwnerToken old_owner = allocator->owner.token;
  allocator->owner.state = HZ6_OWNER_DEAD;

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || !hz6_owner_equal(descriptor->owner, old_owner)) {
      continue;
    }
    if (descriptor->state == HZ6_STATE_ACTIVE ||
        descriptor->state == HZ6_STATE_LOCAL_FREE ||
        descriptor->state == HZ6_STATE_REMOTE_PENDING) {
      descriptor->state = HZ6_STATE_ORPHAN;
    }
  }
}

int hz6_allocator_release_orphan(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind != HZ6_ROUTE_VALID || !route.descriptor) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->ptr != ptr || descriptor->state != HZ6_STATE_ORPHAN) {
    return 0;
  }

  hz6_allocator_route_unregister_exact(allocator, ptr);
  return hz6_allocator_release_descriptor_source(descriptor);
}

int hz6_allocator_adopt_orphan(Hz6Allocator* adopter,
                               Hz6Allocator* source,
                               void* ptr) {
  if (!adopter || !source || adopter == source || !ptr) {
    return 0;
  }
  if (!hz6_owner_is_alive(&adopter->owner, adopter->owner.token)) {
    return 0;
  }

  Hz6RouteResult source_route =
      hz6_route_backend_lookup(&source->route_backend, ptr);
  if (source_route.kind != HZ6_ROUTE_VALID || !source_route.descriptor) {
    return 0;
  }

  Hz6ObjectDescriptor* source_descriptor =
      (Hz6ObjectDescriptor*)source_route.descriptor;
  if (source_descriptor->ptr != ptr ||
      source_descriptor->state != HZ6_STATE_ORPHAN ||
      source_descriptor->source_block ||
      source_descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  Hz6ObjectDescriptor* adopted_descriptor =
      hz6_allocator_find_free_descriptor(adopter);
  if (!adopted_descriptor) {
    return 0;
  }

  *adopted_descriptor = *source_descriptor;
  adopted_descriptor->owner = adopter->owner.token;
  adopted_descriptor->state = HZ6_STATE_LOCAL_FREE;

  if (!hz6_route_backend_register_exact(
          &adopter->route_backend,
          ptr,
          adopted_descriptor->bytes,
          source_route.front_id,
          source_route.class_id,
          adopted_descriptor->generation,
          adopted_descriptor)) {
    *adopted_descriptor = (Hz6ObjectDescriptor){0};
    return 0;
  }

  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = adopted_descriptor;
  entry.class_id = adopted_descriptor->class_id;
  entry.generation = adopted_descriptor->generation;
  if (!hz6_allocator_frontcache_push(adopter, entry.class_id, entry)) {
    hz6_allocator_route_unregister_exact(adopter, ptr);
    *adopted_descriptor = (Hz6ObjectDescriptor){0};
    return 0;
  }

  hz6_allocator_route_unregister_exact(source, ptr);
  source_descriptor->ptr = NULL;
  source_descriptor->bytes = 0;
  source_descriptor->source_ptr = NULL;
  source_descriptor->source_bytes = 0;
  source_descriptor->source_block = NULL;
  source_descriptor->class_id = 0;
  source_descriptor->source_kind = HZ6_SOURCE_NONE;
  source_descriptor->source_release = NULL;
  source_descriptor->owner = hz6_owner_token_none();
  source_descriptor->generation = 0;
  source_descriptor->state = HZ6_STATE_DEAD;
  return 1;
}

size_t hz6_allocator_scavenge_orphans(Hz6Allocator* allocator,
                                      size_t max_bytes) {
  if (!allocator || max_bytes == 0) {
    return 0;
  }

  Hz6ScavengeBudget budget;
  hz6_scavenge_budget_init(&budget, max_bytes);

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || descriptor->state != HZ6_STATE_ORPHAN) {
      continue;
    }

    size_t bytes = hz6_allocator_descriptor_scavenge_cost(descriptor);
    if (!hz6_scavenge_can_release(&budget, bytes)) {
      continue;
    }

    hz6_allocator_route_unregister_exact(allocator, descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
    hz6_scavenge_account_release(&budget, bytes);
  }

  return budget.objects_released;
}

size_t hz6_allocator_scavenge_local_free(Hz6Allocator* allocator,
                                         size_t max_bytes) {
  if (!allocator || max_bytes == 0) {
    return 0;
  }

  Hz6ScavengeBudget budget;
  hz6_scavenge_budget_init(&budget, max_bytes);

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || descriptor->state != HZ6_STATE_LOCAL_FREE ||
        descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
      continue;
    }

    size_t bytes = hz6_allocator_descriptor_scavenge_cost(descriptor);
    if (!hz6_scavenge_can_release(&budget, bytes)) {
      continue;
    }

    if (!hz6_allocator_frontcache_remove(allocator,
                                         descriptor->class_id,
                                         descriptor->ptr,
                                         descriptor,
                                         descriptor->generation,
                                         NULL)) {
      continue;
    }

    hz6_allocator_route_unregister_exact(allocator, descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
    hz6_scavenge_account_release(&budget, bytes);
  }

  return budget.objects_released;
}

size_t hz6_allocator_scavenge_profile(Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }

  size_t released = 0;
  size_t orphan_budget =
      hz6_profile_scavenge_orphan_budget(&allocator->profile);
  if (orphan_budget != 0) {
    released += hz6_allocator_scavenge_orphans(allocator, orphan_budget);
  }
  size_t local_free_budget =
      hz6_profile_scavenge_local_free_budget(&allocator->profile);
  if (local_free_budget != 0) {
    released += hz6_allocator_scavenge_local_free(allocator,
                                                  local_free_budget);
  }
  return released;
}

size_t hz6_allocator_drain_remote_pending(Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return 0;
  }

  size_t drained = 0;
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr || descriptor->state != HZ6_STATE_REMOTE_PENDING ||
        descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
      continue;
    }
    if (!hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
      continue;
    }

    Hz6FrontCacheEntry entry;
    entry.ptr = descriptor->ptr;
    entry.descriptor = descriptor;
    entry.class_id = descriptor->class_id;
    entry.generation = descriptor->generation;
    if (!hz6_allocator_frontcache_push(allocator, entry.class_id, entry)) {
      continue;
    }

    descriptor->state = HZ6_STATE_LOCAL_FREE;
    ++drained;
  }
  return drained;
}

size_t hz6_allocator_prefill_size(Hz6Allocator* allocator,
                                  size_t size,
                                  size_t count) {
  if (!allocator || size == 0 || count == 0) {
    return 0;
  }
  return hz6_front_prefill_for_allocation(allocator, size, 16, count);
}

size_t hz6_allocator_prefill_front(Hz6Allocator* allocator,
                                   uint16_t front_id,
                                   size_t count) {
  return hz6_allocator_prefill_front_class(allocator, front_id, 0, count);
}

size_t hz6_allocator_prefill_front_class(Hz6Allocator* allocator,
                                         uint16_t front_id,
                                         uint16_t class_id,
                                         size_t count) {
  if (!allocator || count == 0) {
    return 0;
  }
  return hz6_front_prefill_by_id_class(allocator, front_id, class_id, count);
}

Hz6ProfileId hz6_allocator_profile_id(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.id : HZ6_PROFILE_STRICT;
}

int hz6_allocator_profile_transfer_first(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.transfer_first != 0 : 0;
}

int hz6_allocator_profile_strict_owner_remote(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.strict_owner_remote != 0 : 0;
}

Hz6SourceKind hz6_allocator_profile_source_kind(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.source_kind : HZ6_SOURCE_NONE;
}

size_t hz6_allocator_profile_source_refill_batch(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  return allocator ? hz6_profile_source_refill_batch(&allocator->profile,
                                                     front_id, class_id)
                   : 0;
}

size_t hz6_allocator_profile_transfer_capacity(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.transfer_capacity : 0;
}

const Hz6OsMemoryOps* hz6_allocator_source_ops(
    const Hz6Allocator* allocator,
    Hz6SourceKind source_kind) {
  return allocator ? hz6_source_registry_lookup(&allocator->source_registry,
                                                source_kind)
                   : NULL;
}

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
  return hz6_route_backend_lookup(&allocator->route_backend, ptr);
}

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr) {
  if (!allocator || !ptr) {
    return;
  }
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr);
}

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor) {
  if (!allocator || !base || bytes == 0) {
    return 0;
  }
  return hz6_route_backend_register_exact(&allocator->route_backend,
                                          base,
                                          bytes,
                                          front_id,
                                          class_id,
                                          generation,
                                          descriptor);
}

Hz6RouteBackendKind hz6_allocator_route_backend_kind(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return HZ6_ROUTE_BACKEND_EXACT_TABLE;
  }
  return allocator->route_backend.kind;
}

size_t hz6_allocator_route_page_granularity(const Hz6Allocator* allocator) {
  return allocator ? allocator->route_backend.page_granularity : 0;
}

Hz6TransferBackendKind hz6_allocator_transfer_backend_kind(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return HZ6_TRANSFER_BACKEND_SINGLE_CACHE;
  }
  return allocator->transfer_backend.kind;
}

size_t hz6_allocator_transfer_capacity(const Hz6Allocator* allocator) {
  return allocator
             ? hz6_transfer_backend_capacity(&allocator->transfer_backend)
             : 0;
}

size_t hz6_allocator_transfer_count(const Hz6Allocator* allocator) {
  return allocator ? hz6_transfer_backend_count(&allocator->transfer_backend)
                   : 0;
}

size_t hz6_allocator_transfer_count_class(const Hz6Allocator* allocator,
                                          uint16_t class_id) {
  return allocator
             ? hz6_transfer_backend_count_class(&allocator->transfer_backend,
                                                class_id)
             : 0;
}

size_t hz6_allocator_transfer_shard_count_at(const Hz6Allocator* allocator,
                                             size_t shard_index) {
  return allocator ? hz6_transfer_backend_shard_count_at(
                         &allocator->transfer_backend, shard_index)
                   : 0;
}

size_t hz6_allocator_transfer_shard_capacity_at(
    const Hz6Allocator* allocator,
    size_t shard_index) {
  return allocator ? hz6_transfer_backend_shard_capacity_at(
                         &allocator->transfer_backend, shard_index)
                   : 0;
}

int hz6_allocator_transfer_push(Hz6Allocator* allocator,
                                Hz6TransferObject object) {
  if (!allocator) {
    return 0;
  }
  size_t producer_shard = hz6_profile_transfer_producer_shard(
      &allocator->profile, allocator->owner.token.slot, object.class_id);
  return hz6_transfer_backend_push_to_shard(&allocator->transfer_backend,
                                            object,
                                            producer_shard);
}

int hz6_allocator_transfer_pop(Hz6Allocator* allocator,
                               uint16_t class_id,
                               Hz6TransferObject* out) {
  if (!allocator || !out) {
    return 0;
  }
  size_t home_shard = hz6_profile_transfer_consumer_shard(
      &allocator->profile, allocator->owner.token.slot, class_id);
  return hz6_transfer_backend_pop_from_shard(&allocator->transfer_backend,
                                             class_id,
                                             home_shard,
                                             out);
}

void hz6_allocator_note_source_alloc(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.source_alloc;
  }
}

void hz6_allocator_note_transfer_push(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.transfer_push;
  }
}

void hz6_allocator_note_transfer_pop(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.transfer_pop;
  }
}

void hz6_allocator_init(Hz6Allocator* allocator) {
  hz6_allocator_init_with_profile(allocator, HZ6_PROFILE_STRICT);
}

void hz6_allocator_init_with_profile(Hz6Allocator* allocator,
                                     Hz6ProfileId profile_id) {
  if (!allocator) {
    return;
  }
  allocator->profile = hz6_profile_config(profile_id);
  allocator->owner.token.slot = g_hz6_allocator_owner_slot_seed++;
  allocator->owner.token.generation = 1;
  allocator->owner.state = HZ6_OWNER_ALIVE;
  allocator->stats.route_valid = 0;
  allocator->stats.route_invalid = 0;
  allocator->stats.route_miss = 0;
  allocator->stats.transfer_push = 0;
  allocator->stats.transfer_pop = 0;
  allocator->stats.source_alloc = 0;
  hz6_source_registry_init(&allocator->source_registry);
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    allocator->source_blocks[i].ptr = NULL;
    allocator->source_blocks[i].bytes = 0;
    allocator->source_blocks[i].source_kind = HZ6_SOURCE_NONE;
    allocator->source_blocks[i].source_release = NULL;
    allocator->source_blocks[i].route_backend = NULL;
    allocator->source_blocks[i].ref_count = 0;
    allocator->source_blocks[i].active = 0;
    allocator->source_blocks[i].route_registered = 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    allocator->descriptors[i].ptr = NULL;
    allocator->descriptors[i].bytes = 0;
    allocator->descriptors[i].source_ptr = NULL;
    allocator->descriptors[i].source_bytes = 0;
    allocator->descriptors[i].source_block = NULL;
    allocator->descriptors[i].class_id = 0;
    allocator->descriptors[i].source_kind = HZ6_SOURCE_NONE;
    allocator->descriptors[i].source_release = NULL;
    allocator->descriptors[i].owner = hz6_owner_token_none();
    allocator->descriptors[i].generation = 0;
    allocator->descriptors[i].state = HZ6_STATE_DEAD;
  }
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            allocator->frontcache_entries[i],
                            HZ6_FRONT_CACHE_BIN_CAPACITY);
  }
  switch (allocator->profile.route_backend_policy) {
    case HZ6_ROUTE_POLICY_PAGE_TABLE:
      hz6_route_backend_init_page_table_with_granularity(
          &allocator->route_backend, allocator->route_entries,
          HZ6_ROUTE_TABLE_CAPACITY, allocator->profile.route_page_granularity);
      break;
    case HZ6_ROUTE_POLICY_EXACT_TABLE:
    default:
      hz6_route_backend_init_exact(&allocator->route_backend,
                                   allocator->route_entries,
                                   HZ6_ROUTE_TABLE_CAPACITY);
      break;
  }
  size_t transfer_capacity = allocator->profile.transfer_capacity;
  if (transfer_capacity == 0 ||
      transfer_capacity > HZ6_TRANSFER_CACHE_CAPACITY) {
    transfer_capacity = HZ6_TRANSFER_CACHE_CAPACITY;
  }
  if (allocator->profile.transfer_shards > 1) {
    hz6_transfer_backend_init_sharded(&allocator->transfer_backend,
                                      allocator->transfer_objects,
                                      transfer_capacity,
                                      allocator->profile.transfer_shards);
  } else {
    hz6_transfer_backend_init_single(&allocator->transfer_backend,
                                     allocator->transfer_objects,
                                     transfer_capacity);
  }
}

void hz6_allocator_destroy(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  allocator->owner.state = HZ6_OWNER_DYING;
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr) {
      continue;
    }
    hz6_allocator_route_unregister_exact(allocator, descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
  }
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!block->active || !block->ptr) {
      continue;
    }
    if (block->route_registered && block->route_backend) {
      hz6_route_backend_unregister_invalid_range(block->route_backend,
                                                 block->ptr);
    }
    if (block->source_release) {
      block->source_release(block->ptr, block->bytes);
    } else {
      hz6_source_system_release(block->ptr, block->bytes);
    }
    block->ptr = NULL;
    block->bytes = 0;
    block->source_kind = HZ6_SOURCE_NONE;
    block->source_release = NULL;
    block->route_backend = NULL;
    block->ref_count = 0;
    block->active = 0;
    block->route_registered = 0;
  }
  allocator->owner.state = HZ6_OWNER_DEAD;
}

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  if (!allocator) {
    return NULL;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }

  uint16_t class_id = 0;
  const Hz6FrontOps* front = hz6_front_for_allocation(size, 16, &class_id);
  if (!front || !front->alloc) {
    return NULL;
  }

  return front->alloc(allocator, class_id, size);
}

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  switch (route.kind) {
    case HZ6_ROUTE_VALID:
      ++allocator->stats.route_valid;
      {
        const Hz6FrontOps* front = hz6_front_for_id(route.front_id);
        if (!front || !front->free_tagged ||
            !front->free_tagged(allocator, ptr, route)) {
          ++allocator->stats.route_invalid;
        }
      }
      return;
    case HZ6_ROUTE_INVALID:
      ++allocator->stats.route_invalid;
      return;
    case HZ6_ROUTE_MISS:
    default:
      ++allocator->stats.route_miss;
      return;
  }
}

int hz6_free_remote(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  if (route.kind == HZ6_ROUTE_MISS) {
    ++allocator->stats.route_miss;
    return 0;
  }
  if (route.kind == HZ6_ROUTE_INVALID || !route.descriptor) {
    ++allocator->stats.route_invalid;
    return 0;
  }

  const Hz6FrontOps* front = hz6_front_for_id(route.front_id);
  ++allocator->stats.route_valid;
  if (!front || !front->remote_free_tagged ||
      !front->remote_free_tagged(allocator, ptr, route)) {
    ++allocator->stats.route_invalid;
    return 0;
  }

  return 1;
}

int hz6_owns(Hz6Allocator* allocator, const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }
  Hz6RouteResult route =
      hz6_route_backend_lookup(&allocator->route_backend, ptr);
  return route.kind == HZ6_ROUTE_VALID || route.kind == HZ6_ROUTE_INVALID;
}

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot empty;
  empty.route_valid = 0;
  empty.route_invalid = 0;
  empty.route_miss = 0;
  empty.transfer_push = 0;
  empty.transfer_pop = 0;
  empty.source_alloc = 0;
  return allocator ? allocator->stats : empty;
}
