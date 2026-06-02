#include "hz6_allocator.h"

int hz6_allocator_orphan_adopt_prepare(Hz6Allocator* adopter,
                                       Hz6Allocator* source,
                                       void* ptr,
                                       Hz6RouteResult* source_route,
                                       Hz6ObjectDescriptor** source_descriptor,
                                       Hz6ObjectDescriptor**
                                           adopted_descriptor) {
  if (!adopter || !source || adopter == source || !ptr || !source_route ||
      !source_descriptor || !adopted_descriptor) {
    return 0;
  }
  if (!hz6_owner_is_alive(&adopter->owner, adopter->owner.token)) {
    return 0;
  }

  *source_route = hz6_route_backend_lookup(&source->route_backend, ptr);
  if (source_route->kind != HZ6_ROUTE_VALID || !source_route->descriptor) {
    return 0;
  }

  *source_descriptor = (Hz6ObjectDescriptor*)source_route->descriptor;
  if ((*source_descriptor)->ptr != ptr ||
      (*source_descriptor)->state != HZ6_STATE_ORPHAN ||
      (*source_descriptor)->source_block ||
      (*source_descriptor)->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  *adopted_descriptor = hz6_allocator_find_free_descriptor(adopter);
  if (!*adopted_descriptor) {
    return 0;
  }

  **adopted_descriptor = **source_descriptor;
  (**adopted_descriptor).allocator = adopter;
  (**adopted_descriptor).owner = adopter->owner.token;
  (**adopted_descriptor).state = HZ6_STATE_LOCAL_FREE;
  return 1;
}
