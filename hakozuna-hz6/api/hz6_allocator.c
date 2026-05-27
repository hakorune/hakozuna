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

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    if (!allocator->descriptors[i].ptr) {
      return &allocator->descriptors[i];
    }
  }
  return NULL;
}

int hz6_allocator_activate_descriptor(Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner) {
  if (!descriptor || descriptor->state != expected) {
    return 0;
  }
  if (descriptor->ptr != ptr || descriptor->generation != generation) {
    return 0;
  }
  descriptor->state = HZ6_STATE_ACTIVE;
  descriptor->owner = owner;
  return 1;
}

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->ptr) {
    return 0;
  }

  int released = 0;
  if (descriptor->source_release) {
    released =
        descriptor->source_release(descriptor->ptr, descriptor->source_bytes);
  } else {
    released = hz6_source_system_release(descriptor->ptr, descriptor->bytes);
  }

  descriptor->ptr = NULL;
  descriptor->bytes = 0;
  descriptor->source_bytes = 0;
  descriptor->class_id = 0;
  descriptor->source_kind = HZ6_SOURCE_NONE;
  descriptor->source_release = NULL;
  descriptor->owner = hz6_owner_token_none();
  descriptor->generation = 0;
  descriptor->state = HZ6_STATE_DEAD;
  return released;
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
        descriptor->state == HZ6_STATE_LOCAL_FREE) {
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

  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr);
  return hz6_allocator_release_descriptor_source(descriptor);
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

    size_t bytes = descriptor->source_bytes ? descriptor->source_bytes
                                            : descriptor->bytes;
    if (!hz6_scavenge_can_release(&budget, bytes)) {
      continue;
    }

    hz6_route_backend_unregister_exact(&allocator->route_backend,
                                       descriptor->ptr);
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

    size_t bytes = descriptor->source_bytes ? descriptor->source_bytes
                                            : descriptor->bytes;
    if (!hz6_scavenge_can_release(&budget, bytes)) {
      continue;
    }

    if (!hz6_frontcache_remove(&allocator->frontcache_bins[descriptor->class_id],
                               descriptor->ptr,
                               descriptor,
                               descriptor->generation,
                               NULL)) {
      continue;
    }

    hz6_route_backend_unregister_exact(&allocator->route_backend,
                                       descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
    hz6_scavenge_account_release(&budget, bytes);
  }

  return budget.objects_released;
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
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    allocator->descriptors[i].ptr = NULL;
    allocator->descriptors[i].bytes = 0;
    allocator->descriptors[i].source_bytes = 0;
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
  hz6_route_backend_init_exact(&allocator->route_backend,
                               allocator->route_entries,
                               HZ6_ROUTE_TABLE_CAPACITY);
  if (allocator->profile.transfer_shards > 1) {
    hz6_transfer_backend_init_sharded(&allocator->transfer_backend,
                                      allocator->transfer_objects,
                                      HZ6_TRANSFER_CACHE_CAPACITY,
                                      allocator->profile.transfer_shards);
  } else {
    hz6_transfer_backend_init_single(&allocator->transfer_backend,
                                     allocator->transfer_objects,
                                     HZ6_TRANSFER_CACHE_CAPACITY);
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
    hz6_route_backend_unregister_exact(&allocator->route_backend,
                                       descriptor->ptr);
    hz6_allocator_release_descriptor_source(descriptor);
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
