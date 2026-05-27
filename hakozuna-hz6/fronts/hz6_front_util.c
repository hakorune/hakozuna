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

  Hz6FrontCacheEntry entry;
  if (hz6_frontcache_pop(&allocator->frontcache_bins[class_id], &entry)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (!hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            allocator->owner.token)) {
      return NULL;
    }
    return entry.ptr;
  }

  if (allocator->profile.transfer_first) {
    Hz6TransferObject transfer;
    while (hz6_transfer_backend_pop(&allocator->transfer_backend, class_id,
                                    &transfer)) {
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
