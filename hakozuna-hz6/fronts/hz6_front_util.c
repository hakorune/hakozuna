#include "hz6_front_util.h"

#include "../source/hz6_source.h"

void* hz6_front_reuse_or_source(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t bytes) {
  return hz6_front_reuse_or_source_kind(allocator, front_id, class_id, bytes,
                                        HZ6_SOURCE_SYSTEM);
}

void* hz6_front_reuse_or_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind) {
  if (!allocator) {
    return NULL;
  }
  const Hz6OsMemoryOps* source_ops =
      hz6_source_registry_lookup(&allocator->source_registry, source_kind);
  return hz6_front_reuse_or_source_ops(allocator, front_id, class_id, bytes,
                                       source_ops, source_kind);
}

void* hz6_front_reuse_or_prefill_source_kind(Hz6Allocator* allocator,
                                             uint16_t front_id,
                                             uint16_t class_id,
                                             size_t bytes,
                                             Hz6SourceKind source_kind,
                                             size_t count) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0) {
    return NULL;
  }

  void* reused = allocator->profile.transfer_first
                     ? hz6_front_reuse_transfer_or_cached(allocator, class_id)
                     : hz6_front_reuse_cached_or_transfer(allocator,
                                                          class_id);
  if (reused) {
    return reused;
  }

  size_t refill_count = count ? count : 1;
  if (hz6_front_prefill_source_kind(allocator, front_id, class_id, bytes,
                                    source_kind, refill_count) == 0) {
    return hz6_front_reuse_or_source_kind(allocator, front_id, class_id,
                                          bytes, source_kind);
  }

  return allocator->profile.transfer_first
             ? hz6_front_reuse_transfer_or_cached(allocator, class_id)
             : hz6_front_reuse_cached_or_transfer(allocator, class_id);
}

void* hz6_front_reuse_or_source_ops(Hz6Allocator* allocator,
                                    uint16_t front_id,
                                    uint16_t class_id,
                                    size_t bytes,
                                    const Hz6OsMemoryOps* source_ops,
                                    Hz6SourceKind source_kind) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0) {
    return NULL;
  }
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  void* reused = hz6_front_reuse_cached_or_transfer(allocator, class_id);
  if (reused) {
    return reused;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
    return NULL;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = bytes;
  descriptor->source_ptr = ptr;
  descriptor->source_bytes = bytes;
  descriptor->class_id = class_id;
  descriptor->source_kind = source_kind;
  descriptor->source_release = source_ops->release;
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = HZ6_STATE_ACTIVE;
  if (!hz6_route_backend_register_exact(&allocator->route_backend, ptr, bytes,
                                        front_id, class_id,
                                        descriptor->generation, descriptor)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  ++allocator->stats.source_alloc;
  return ptr;
}

static void* hz6_front_reuse_transfer(Hz6Allocator* allocator,
                                      uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !allocator->profile.transfer_first) {
    return NULL;
  }

  Hz6TransferObject transfer;
  size_t home_shard = (size_t)allocator->owner.token.slot;
  while (hz6_transfer_backend_pop_from_shard(
      &allocator->transfer_backend, class_id, home_shard, &transfer)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)transfer.descriptor;
    if (!hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_TRANSFER_FREE, transfer.ptr,
            transfer.generation, allocator->owner.token)) {
      continue;
    }
    ++allocator->stats.transfer_pop;
    return transfer.ptr;
  }

  return NULL;
}

void* hz6_front_reuse_cached_or_transfer(Hz6Allocator* allocator,
                                         uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  while (hz6_frontcache_pop(&allocator->frontcache_bins[class_id], &entry)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            allocator->owner.token)) {
      return entry.ptr;
    }
  }

  if (!allocator->profile.transfer_first) {
    return NULL;
  }

  return hz6_front_reuse_transfer(allocator, class_id);
}

void* hz6_front_reuse_transfer_or_cached(Hz6Allocator* allocator,
                                         uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  if (allocator->profile.transfer_first) {
    void* reused = hz6_front_reuse_transfer(allocator, class_id);
    if (reused) {
      return reused;
    }
  }

  Hz6FrontCacheEntry entry;
  while (hz6_frontcache_pop(&allocator->frontcache_bins[class_id], &entry)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            allocator->owner.token)) {
      return entry.ptr;
    }
  }

  return NULL;
}

void* hz6_front_source_slot_kind(Hz6Allocator* allocator,
                                 uint16_t front_id,
                                 uint16_t class_id,
                                 size_t user_bytes,
                                 size_t source_bytes,
                                 size_t source_offset,
                                 Hz6SourceKind source_kind) {
  if (!allocator) {
    return NULL;
  }
  const Hz6OsMemoryOps* source_ops =
      hz6_source_registry_lookup(&allocator->source_registry, source_kind);
  return hz6_front_source_slot_ops(allocator, front_id, class_id, user_bytes,
                                   source_bytes, source_offset, source_ops,
                                   source_kind);
}

void* hz6_front_source_slot_ops(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t user_bytes,
                                size_t source_bytes,
                                size_t source_offset,
                                const Hz6OsMemoryOps* source_ops,
                                Hz6SourceKind source_kind) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      user_bytes == 0 || source_bytes == 0 || source_offset > source_bytes ||
      user_bytes > source_bytes - source_offset) {
    return NULL;
  }
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }

  void* source_ptr =
      source_ops->reserve(source_bytes, source_ops->allocation_granularity);
  if (!source_ptr) {
    return NULL;
  }

  void* user_ptr = (void*)((unsigned char*)source_ptr + source_offset);
  descriptor->ptr = user_ptr;
  descriptor->bytes = user_bytes;
  descriptor->source_ptr = source_ptr;
  descriptor->source_bytes = source_bytes;
  descriptor->class_id = class_id;
  descriptor->source_kind = source_kind;
  descriptor->source_release = source_ops->release;
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = HZ6_STATE_ACTIVE;

  if (!hz6_route_backend_register_exact(
          &allocator->route_backend, user_ptr, user_bytes, front_id, class_id,
          descriptor->generation, descriptor)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  ++allocator->stats.source_alloc;
  return user_ptr;
}

void* hz6_front_source_block_slot(Hz6Allocator* allocator,
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t user_bytes,
                                  size_t source_offset,
                                  Hz6SourceBlock* source_block) {
  if (!allocator || !source_block || !source_block->active ||
      !source_block->ptr || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      user_bytes == 0 || source_offset > source_block->bytes ||
      user_bytes > source_block->bytes - source_offset) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }
  if (!source_block->route_registered) {
    if (!hz6_route_backend_register_invalid_range(
            &allocator->route_backend, source_block->ptr, source_block->bytes,
            front_id, class_id)) {
      return NULL;
    }
    source_block->route_backend = &allocator->route_backend;
    source_block->route_registered = 1;
  }
  if (!hz6_allocator_retain_source_block(source_block)) {
    return NULL;
  }

  void* user_ptr = (void*)((unsigned char*)source_block->ptr + source_offset);
  descriptor->ptr = user_ptr;
  descriptor->bytes = user_bytes;
  descriptor->source_ptr = source_block->ptr;
  descriptor->source_bytes = source_block->bytes;
  descriptor->source_block = source_block;
  descriptor->class_id = class_id;
  descriptor->source_kind = source_block->source_kind;
  descriptor->source_release = source_block->source_release;
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = HZ6_STATE_ACTIVE;

  if (!hz6_route_backend_register_exact(
          &allocator->route_backend, user_ptr, user_bytes, front_id, class_id,
          descriptor->generation, descriptor)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  return user_ptr;
}

static int hz6_front_prefill_one(Hz6Allocator* allocator,
                                 uint16_t front_id,
                                 uint16_t class_id,
                                 size_t bytes,
                                 const Hz6OsMemoryOps* source_ops,
                                 Hz6SourceKind source_kind) {
  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return 0;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
    return 0;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = bytes;
  descriptor->source_ptr = ptr;
  descriptor->source_bytes = bytes;
  descriptor->class_id = class_id;
  descriptor->source_kind = source_kind;
  descriptor->source_release = source_ops->release;
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = HZ6_STATE_LOCAL_FREE;

  if (!hz6_route_backend_register_exact(&allocator->route_backend, ptr, bytes,
                                        front_id, class_id,
                                        descriptor->generation, descriptor)) {
    hz6_allocator_release_descriptor_source(descriptor);
    return 0;
  }

  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = class_id;
  entry.generation = descriptor->generation;
  if (!hz6_frontcache_push(&allocator->frontcache_bins[class_id], entry)) {
    hz6_route_backend_unregister_exact(&allocator->route_backend, ptr);
    hz6_allocator_release_descriptor_source(descriptor);
    return 0;
  }

  ++allocator->stats.source_alloc;
  return 1;
}

size_t hz6_front_prefill_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind,
                                     size_t count) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0 ||
      count == 0) {
    return 0;
  }

  const Hz6OsMemoryOps* source_ops =
      hz6_source_registry_lookup(&allocator->source_registry, source_kind);
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return 0;
  }

  size_t filled = 0;
  while (filled < count &&
         allocator->frontcache_bins[class_id].count <
             allocator->frontcache_bins[class_id].capacity) {
    if (!hz6_front_prefill_one(allocator, front_id, class_id, bytes,
                               source_ops, source_kind)) {
      break;
    }
    ++filled;
  }
  return filled;
}

int hz6_front_free_local_to_cache(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route,
                                  uint16_t expected_class_id) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != expected_class_id) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }
  if (!hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
    return 0;
  }

  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = descriptor->class_id;
  entry.generation = descriptor->generation;
  if (hz6_frontcache_push(&allocator->frontcache_bins[entry.class_id],
                          entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_DEAD;
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr);
  hz6_allocator_release_descriptor_source(descriptor);
  return 1;
}

int hz6_front_free_remote_to_transfer(Hz6Allocator* allocator,
                                      void* ptr,
                                      Hz6RouteResult route,
                                      uint16_t expected_class_id) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != expected_class_id) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  if (allocator->profile.strict_owner_remote) {
    if (!hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
      return 0;
    }
    descriptor->state = HZ6_STATE_REMOTE_PENDING;
    return 1;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;
  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  descriptor->owner.slot = 0;
  descriptor->owner.generation = 0;
  if (!hz6_transfer_backend_push(&allocator->transfer_backend, object)) {
    descriptor->state = HZ6_STATE_ACTIVE;
    descriptor->owner = allocator->owner.token;
    return 0;
  }

  ++allocator->stats.transfer_push;
  return 1;
}
