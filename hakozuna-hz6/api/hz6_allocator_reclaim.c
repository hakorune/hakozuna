#include "hz6_allocator.h"

static Hz6OwnerToken hz6_allocator_reclaim_owner_none(void) {
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
  source_descriptor->owner = hz6_allocator_reclaim_owner_none();
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
