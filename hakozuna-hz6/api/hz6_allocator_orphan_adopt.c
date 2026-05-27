#include "hz6_allocator.h"

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
  source_descriptor->owner = (Hz6OwnerToken){0};
  source_descriptor->generation = 0;
  source_descriptor->state = HZ6_STATE_DEAD;
  return 1;
}
