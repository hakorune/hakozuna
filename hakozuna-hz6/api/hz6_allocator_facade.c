#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

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

void hz6_allocator_note_source_alloc(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.source_alloc;
  }
}
