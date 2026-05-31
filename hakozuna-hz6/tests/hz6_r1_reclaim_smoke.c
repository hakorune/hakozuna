#include "../api/hz6_allocator.h"

#include <stdio.h>

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-reclaim-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  Hz6Allocator orphan_allocator;
  hz6_allocator_init_with_profile(&orphan_allocator, HZ6_PROFILE_SPEED);
  void* orphan_object = hz6_malloc(&orphan_allocator, 48);
  if (!expect(orphan_object != NULL, "orphan allocator malloc")) {
    return 1;
  }
  Hz6RouteResult orphan_route =
      hz6_allocator_route_lookup(&orphan_allocator, orphan_object);
  Hz6ObjectDescriptor* orphan_descriptor =
      (Hz6ObjectDescriptor*)orphan_route.descriptor;
  if (!expect(orphan_descriptor != NULL, "orphan descriptor") ||
      !expect(orphan_descriptor->state == HZ6_STATE_ACTIVE,
              "orphan starts active")) {
    return 1;
  }
  hz6_allocator_mark_owner_dead(&orphan_allocator);
  if (!expect(orphan_allocator.owner.state == HZ6_OWNER_DEAD,
              "owner marked dead") ||
      !expect(orphan_descriptor->state == HZ6_STATE_ORPHAN,
              "owned descriptor marked orphan") ||
      !expect(hz6_malloc(&orphan_allocator, 48) == NULL,
              "dead owner malloc rejected")) {
    return 1;
  }
  hz6_free(&orphan_allocator, orphan_object);
  Hz6StatsSnapshot orphan_stats = hz6_stats_snapshot(&orphan_allocator);
  if (!expect(orphan_stats.route_invalid == 1, "orphan free invalid")) {
    return 1;
  }
  if (!expect(hz6_allocator_release_orphan(&orphan_allocator, orphan_object),
              "orphan release")) {
    return 1;
  }
  Hz6RouteResult orphan_released_route =
      hz6_allocator_route_lookup(&orphan_allocator, orphan_object);
  if (!expect(orphan_released_route.kind == HZ6_ROUTE_INVALID,
              "orphan exact route released") ||
      !expect(orphan_descriptor->state == HZ6_STATE_DEAD,
              "orphan descriptor dead")) {
    return 1;
  }
  hz6_allocator_destroy(&orphan_allocator);

  Hz6Allocator profile_scavenge_allocator;
  hz6_allocator_init_with_profile(&profile_scavenge_allocator,
                                  HZ6_PROFILE_RSS);
  void* profile_cached = hz6_malloc(&profile_scavenge_allocator, 48);
  if (!expect(profile_cached != NULL, "profile scavenge malloc")) {
    return 1;
  }
  Hz6RouteResult profile_scavenge_route =
      hz6_allocator_route_lookup(&profile_scavenge_allocator, profile_cached);
  Hz6ObjectDescriptor* profile_scavenge_descriptor =
      (Hz6ObjectDescriptor*)profile_scavenge_route.descriptor;
  hz6_free(&profile_scavenge_allocator, profile_cached);
  size_t profile_scavenge_frontcache =
      hz6_allocator_frontcache_count(&profile_scavenge_allocator,
                                     profile_scavenge_descriptor->class_id);
  if (!expect(profile_scavenge_descriptor != NULL,
              "profile scavenge descriptor") ||
      !expect(profile_scavenge_descriptor->state == HZ6_STATE_LOCAL_FREE,
              "profile scavenge starts local free") ||
      !expect(hz6_allocator_frontcache_count(&profile_scavenge_allocator,
                                             profile_scavenge_descriptor
                                                 ->class_id) > 0 ||
                  profile_scavenge_descriptor->state == HZ6_STATE_DEAD,
              "profile scavenge frontcache populated") ||
      !expect(profile_scavenge_descriptor->class_id <
                  HZ6_FRONT_CACHE_CLASS_COUNT ||
                  profile_scavenge_descriptor->state == HZ6_STATE_DEAD,
              "profile scavenge class is cached") ||
      !expect(hz6_allocator_scavenge_profile(&profile_scavenge_allocator) ==
                  profile_scavenge_frontcache,
              "profile scavenge local free") ||
      !expect(!hz6_owns(&profile_scavenge_allocator, profile_cached),
              "profile scavenge route gone") ||
      !expect(profile_scavenge_descriptor->state == HZ6_STATE_DEAD,
              "profile scavenge descriptor dead")) {
    return 1;
  }
  hz6_allocator_destroy(&profile_scavenge_allocator);

  Hz6Allocator adopt_source;
  Hz6Allocator adopt_target;
  hz6_allocator_init_with_profile(&adopt_source, HZ6_PROFILE_STRICT);
  hz6_allocator_init_with_profile(&adopt_target, HZ6_PROFILE_STRICT);
  void* adopt_object = hz6_malloc(&adopt_source, 48);
  if (!expect(adopt_object != NULL, "adopt source malloc")) {
    return 1;
  }
  Hz6RouteResult adopt_source_route =
      hz6_allocator_route_lookup(&adopt_source, adopt_object);
  Hz6ObjectDescriptor* adopt_source_descriptor =
      (Hz6ObjectDescriptor*)adopt_source_route.descriptor;
  hz6_allocator_mark_owner_dead(&adopt_source);
  if (!expect(adopt_source_descriptor != NULL, "adopt source descriptor") ||
      !expect(adopt_source_descriptor->state == HZ6_STATE_ORPHAN,
              "adopt source orphan") ||
      !expect(hz6_allocator_adopt_orphan(&adopt_target,
                                         &adopt_source,
                                         adopt_object),
              "adopt orphan")) {
    return 1;
  }
  void* adopted_reuse = hz6_malloc(&adopt_target, 48);
  if (!expect(adopted_reuse == adopt_object, "adopted reuse") ||
      !expect(!hz6_owns(&adopt_source, adopt_object),
              "adopt source route gone") ||
      !expect(hz6_owns(&adopt_target, adopt_object),
              "adopt target owns object")) {
    return 1;
  }
  hz6_free(&adopt_target, adopted_reuse);
  hz6_allocator_destroy(&adopt_target);
  hz6_allocator_destroy(&adopt_source);

  printf("hz6-r1-reclaim-smoke ok\n");
  return 0;
}
