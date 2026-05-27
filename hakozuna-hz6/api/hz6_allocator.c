#include "hz6_allocator.h"

#include "../source/hz6_source.h"

static size_t hz6_round_class_bytes(size_t size) {
  if (size == 0) {
    return 16;
  }
  if (size <= 16) {
    return 16;
  }
  if (size <= 64) {
    return 64;
  }
  if (size <= 256) {
    return 256;
  }
  if (size <= 1024) {
    return 1024;
  }
  return 4096;
}

static uint16_t hz6_class_id_for_bytes(size_t bytes) {
  if (bytes <= 16) {
    return 0;
  }
  if (bytes <= 64) {
    return 1;
  }
  if (bytes <= 256) {
    return 2;
  }
  if (bytes <= 1024) {
    return 3;
  }
  return 4;
}

static Hz6ObjectDescriptor* hz6_find_free_descriptor(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    if (!allocator->descriptors[i].ptr) {
      return &allocator->descriptors[i];
    }
  }
  return NULL;
}

static int hz6_descriptor_activate(Hz6ObjectDescriptor* descriptor,
                                   Hz6ObjectState expected,
                                   void* ptr,
                                   uint32_t generation) {
  if (!descriptor || descriptor->state != expected) {
    return 0;
  }
  if (descriptor->ptr != ptr || descriptor->generation != generation) {
    return 0;
  }
  descriptor->state = HZ6_STATE_ACTIVE;
  return 1;
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
  allocator->stats.route_valid = 0;
  allocator->stats.route_invalid = 0;
  allocator->stats.route_miss = 0;
  allocator->stats.transfer_push = 0;
  allocator->stats.transfer_pop = 0;
  allocator->stats.source_alloc = 0;
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    allocator->descriptors[i].ptr = NULL;
    allocator->descriptors[i].bytes = 0;
    allocator->descriptors[i].class_id = 0;
    allocator->descriptors[i].generation = 0;
    allocator->descriptors[i].state = HZ6_STATE_DEAD;
  }
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    hz6_frontcache_bin_init(&allocator->frontcache_bins[i],
                            allocator->frontcache_entries[i],
                            HZ6_FRONT_CACHE_BIN_CAPACITY);
  }
  hz6_route_table_init(&allocator->route_table, allocator->route_entries,
                       HZ6_ROUTE_TABLE_CAPACITY);
  hz6_transfer_init(&allocator->transfer_cache, allocator->transfer_objects,
                    HZ6_TRANSFER_CACHE_CAPACITY);
}

void hz6_allocator_destroy(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr) {
      continue;
    }
    hz6_route_unregister_exact(&allocator->route_table, descriptor->ptr);
    hz6_source_system_free(descriptor->ptr);
    descriptor->ptr = NULL;
    descriptor->state = HZ6_STATE_DEAD;
  }
}

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  if (!allocator) {
    return NULL;
  }

  size_t class_bytes = hz6_round_class_bytes(size);
  uint16_t class_id = hz6_class_id_for_bytes(class_bytes);
  if (class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  if (hz6_frontcache_pop(&allocator->frontcache_bins[class_id], &entry)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (!hz6_descriptor_activate(descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr,
                                 entry.generation)) {
      return NULL;
    }
    return entry.ptr;
  }

  if (allocator->profile.transfer_first) {
    Hz6TransferObject transfer;
    while (hz6_transfer_pop(&allocator->transfer_cache, class_id, &transfer)) {
      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)transfer.descriptor;
      if (!hz6_descriptor_activate(descriptor, HZ6_STATE_TRANSFER_FREE,
                                   transfer.ptr, transfer.generation)) {
        continue;
      }
      ++allocator->stats.transfer_pop;
      return transfer.ptr;
    }
  }

  Hz6ObjectDescriptor* descriptor = hz6_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }

  void* ptr = hz6_source_system_alloc(class_bytes);
  if (!ptr) {
    return NULL;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = class_bytes;
  descriptor->class_id = class_id;
  descriptor->generation = 1;
  descriptor->state = HZ6_STATE_ACTIVE;
  if (!hz6_route_register_exact(&allocator->route_table, ptr, class_bytes,
                                HZ6_FRONT_LARGE, class_id,
                                descriptor->generation, descriptor)) {
    hz6_source_system_free(ptr);
    descriptor->ptr = NULL;
    descriptor->state = HZ6_STATE_DEAD;
    return NULL;
  }

  ++allocator->stats.source_alloc;
  return ptr;
}

void hz6_free(Hz6Allocator* allocator, void* ptr) {
  if (!allocator || !ptr) {
    return;
  }

  Hz6RouteResult route = hz6_route_lookup(&allocator->route_table, ptr);
  switch (route.kind) {
    case HZ6_ROUTE_VALID:
      ++allocator->stats.route_valid;
      if (!route.descriptor) {
        ++allocator->stats.route_invalid;
        return;
      }
      {
        Hz6ObjectDescriptor* descriptor =
            (Hz6ObjectDescriptor*)route.descriptor;
        if (descriptor->state != HZ6_STATE_ACTIVE ||
            descriptor->ptr != ptr) {
          ++allocator->stats.route_invalid;
          return;
        }
        descriptor->state = HZ6_STATE_LOCAL_FREE;
        Hz6FrontCacheEntry entry;
        entry.ptr = ptr;
        entry.descriptor = descriptor;
        entry.class_id = descriptor->class_id;
        entry.generation = descriptor->generation;
        if (!hz6_frontcache_push(&allocator->frontcache_bins[entry.class_id],
                                 entry)) {
          descriptor->state = HZ6_STATE_DEAD;
          hz6_route_unregister_exact(&allocator->route_table, ptr);
          hz6_source_system_free(ptr);
          descriptor->ptr = NULL;
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

  Hz6RouteResult route = hz6_route_lookup(&allocator->route_table, ptr);
  if (route.kind == HZ6_ROUTE_MISS) {
    ++allocator->stats.route_miss;
    return 0;
  }
  if (route.kind == HZ6_ROUTE_INVALID || !route.descriptor) {
    ++allocator->stats.route_invalid;
    return 0;
  }

  ++allocator->stats.route_valid;
  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    ++allocator->stats.route_invalid;
    return 0;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;
  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  if (!hz6_transfer_push(&allocator->transfer_cache, object)) {
    descriptor->state = HZ6_STATE_ACTIVE;
    return 0;
  }

  ++allocator->stats.transfer_push;
  return 1;
}

int hz6_owns(Hz6Allocator* allocator, const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }
  Hz6RouteResult route = hz6_route_lookup(&allocator->route_table, ptr);
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
