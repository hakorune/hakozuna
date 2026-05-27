#include "../api/hz6_allocator.h"
#include "../fronts/large/hz6_large128_front.h"
#include "../fronts/local2p/hz6_local2p_front.h"
#include "../fronts/midpage/hz6_midpage_front.h"
#include "../include/hz6_contract.h"
#include "../route/hz6_route.h"

#include <stdio.h>

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-allocator-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  int foreign = 0;

  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, HZ6_PROFILE_SPEED);
  if (!expect(allocator.profile.transfer_first == 1, "allocator profile")) {
    return 1;
  }

  void* allocated = hz6_malloc(&allocator, 48);
  if (!expect(allocated != NULL, "allocator malloc") ||
      !expect(hz6_owns(&allocator, allocated), "allocator owns exact") ||
      !expect(hz6_owns(&allocator, (unsigned char*)allocated + 1),
              "allocator owns invalid interior")) {
    return 1;
  }
  hz6_free(&allocator, (unsigned char*)allocated + 1);
  hz6_free(&allocator, allocated);
  hz6_free(&allocator, allocated);
  hz6_free(&allocator, &foreign);
  void* reused = hz6_malloc(&allocator, 48);
  if (!expect(reused == allocated, "allocator frontcache reuse")) {
    return 1;
  }
  hz6_free(&allocator, reused);
  Hz6StatsSnapshot stats = hz6_stats_snapshot(&allocator);
  if (!expect(stats.route_valid == 3, "stats valid") ||
      !expect(stats.route_invalid == 2, "stats invalid") ||
      !expect(stats.route_miss == 1, "stats miss") ||
      !expect(stats.source_alloc == 1, "stats source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&allocator);

  Hz6Allocator local2p_allocator;
  hz6_allocator_init_with_profile(&local2p_allocator, HZ6_PROFILE_REMOTE);
  if (!expect(local2p_allocator.transfer_backend.kind ==
                  HZ6_TRANSFER_BACKEND_SHARDED_CACHE,
              "remote profile sharded transfer")) {
    return 1;
  }
  if (!expect(hz6_transfer_backend_capacity(
                  &local2p_allocator.transfer_backend) ==
                  HZ6_TRANSFER_CACHE_CAPACITY,
              "remote profile transfer capacity")) {
    return 1;
  }
  void* local2p_object = hz6_malloc(&local2p_allocator, HZ6_LOCAL2P_BYTES);
  if (!expect(local2p_object != NULL, "local2p malloc")) {
    return 1;
  }
  Hz6RouteResult local2p_route =
      hz6_route_backend_lookup(&local2p_allocator.route_backend,
                               local2p_object);
  Hz6ObjectDescriptor* local2p_descriptor =
      (Hz6ObjectDescriptor*)local2p_route.descriptor;
  if (!expect(local2p_route.kind == HZ6_ROUTE_VALID,
              "local2p route valid") ||
      !expect(local2p_route.front_id == HZ6_FRONT_LOCAL2P,
              "local2p route front") ||
      !expect(local2p_route.class_id == HZ6_LOCAL2P_CLASS_ID,
              "local2p route class") ||
      !expect(local2p_descriptor != NULL, "local2p descriptor") ||
      !expect(hz6_owner_equal(local2p_descriptor->owner,
                              local2p_allocator.owner.token),
              "local2p owner assigned") ||
      !expect(hz6_free_remote(&local2p_allocator, local2p_object),
              "local2p remote free")) {
    return 1;
  }
  if (!expect(local2p_descriptor->state == HZ6_STATE_TRANSFER_FREE,
              "local2p transfer state") ||
      !expect(local2p_descriptor->owner.slot == 0 &&
                  local2p_descriptor->owner.generation == 0,
              "local2p transfer owner clear")) {
    return 1;
  }
  void* local2p_reused = hz6_malloc(&local2p_allocator, HZ6_LOCAL2P_BYTES);
  if (!expect(local2p_reused == local2p_object, "local2p transfer reuse")) {
    return 1;
  }
  if (!expect(hz6_owner_equal(local2p_descriptor->owner,
                              local2p_allocator.owner.token),
              "local2p owner restored")) {
    return 1;
  }
  hz6_free(&local2p_allocator, local2p_reused);
  Hz6StatsSnapshot local2p_stats = hz6_stats_snapshot(&local2p_allocator);
  if (!expect(local2p_stats.transfer_push == 1, "local2p transfer push") ||
      !expect(local2p_stats.transfer_pop == 1, "local2p transfer pop") ||
      !expect(local2p_stats.source_alloc == 1, "local2p source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&local2p_allocator);

  Hz6Allocator rss_capacity_allocator;
  hz6_allocator_init_with_profile(&rss_capacity_allocator, HZ6_PROFILE_RSS);
  if (!expect(hz6_transfer_backend_capacity(
                  &rss_capacity_allocator.transfer_backend) ==
                  rss_capacity_allocator.profile.transfer_capacity,
              "rss profile transfer capacity") ||
      !expect(rss_capacity_allocator.transfer_backend.kind ==
                  HZ6_TRANSFER_BACKEND_SINGLE_CACHE,
              "rss profile single transfer")) {
    return 1;
  }
  hz6_allocator_destroy(&rss_capacity_allocator);

  Hz6Allocator midpage_allocator;
  hz6_allocator_init_with_profile(&midpage_allocator, HZ6_PROFILE_REMOTE);
  void* midpage_object = hz6_malloc(&midpage_allocator, 16384);
  if (!expect(midpage_object != NULL, "midpage malloc")) {
    return 1;
  }
  Hz6RouteResult midpage_route =
      hz6_route_backend_lookup(&midpage_allocator.route_backend,
                               midpage_object);
  if (!expect(midpage_route.kind == HZ6_ROUTE_VALID,
              "midpage route valid") ||
      !expect(midpage_route.front_id == HZ6_FRONT_MIDPAGE,
              "midpage route front") ||
      !expect(midpage_route.class_id == HZ6_MIDPAGE_CLASS_ID,
              "midpage route class") ||
      !expect(hz6_free_remote(&midpage_allocator, midpage_object),
              "midpage remote free")) {
    return 1;
  }
  void* midpage_reused = hz6_malloc(&midpage_allocator, 16384);
  if (!expect(midpage_reused == midpage_object, "midpage transfer reuse")) {
    return 1;
  }
  hz6_free(&midpage_allocator, midpage_reused);
  Hz6StatsSnapshot midpage_stats = hz6_stats_snapshot(&midpage_allocator);
  if (!expect(midpage_stats.transfer_push == 1, "midpage transfer push") ||
      !expect(midpage_stats.transfer_pop == 1, "midpage transfer pop") ||
      !expect(midpage_stats.source_alloc == 1, "midpage source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&midpage_allocator);

  Hz6Allocator large_allocator;
  hz6_allocator_init_with_profile(&large_allocator, HZ6_PROFILE_REMOTE);
  void* large_object = hz6_malloc(&large_allocator, 70000);
  if (!expect(large_object != NULL, "large128 malloc")) {
    return 1;
  }
  Hz6RouteResult large_route =
      hz6_route_backend_lookup(&large_allocator.route_backend, large_object);
  if (!expect(large_route.kind == HZ6_ROUTE_VALID, "large128 route valid") ||
      !expect(large_route.front_id == HZ6_FRONT_LARGE,
              "large128 route front") ||
      !expect(large_route.class_id == HZ6_LARGE128_CLASS_ID,
              "large128 route class")) {
    return 1;
  }
  Hz6ObjectDescriptor* large_descriptor =
      (Hz6ObjectDescriptor*)large_route.descriptor;
  if (!expect(large_descriptor != NULL, "large128 descriptor") ||
      !expect(large_descriptor->source_kind == HZ6_SOURCE_OS_PAGED,
              "large128 os paged source kind") ||
      !expect(large_descriptor->source_bytes == HZ6_LARGE128_BYTES,
              "large128 source bytes") ||
      !expect(hz6_free_remote(&large_allocator, large_object),
              "large128 remote free")) {
    return 1;
  }
  void* large_reused = hz6_malloc(&large_allocator, 70000);
  if (!expect(large_reused == large_object, "large128 transfer reuse")) {
    return 1;
  }
  hz6_free(&large_allocator, large_reused);
  Hz6StatsSnapshot large_stats = hz6_stats_snapshot(&large_allocator);
  if (!expect(large_stats.transfer_push == 1, "large128 transfer push") ||
      !expect(large_stats.transfer_pop == 1, "large128 transfer pop") ||
      !expect(large_stats.source_alloc == 1, "large128 source alloc")) {
    return 1;
  }
  if (!expect(hz6_malloc(&large_allocator, HZ6_LARGE128_BYTES + 1) == NULL,
              "unsupported over-large allocation")) {
    return 1;
  }
  hz6_allocator_destroy(&large_allocator);

  Hz6Allocator remote_allocator;
  hz6_allocator_init_with_profile(&remote_allocator, HZ6_PROFILE_REMOTE);
  void* remote_object = hz6_malloc(&remote_allocator, 128);
  if (!expect(remote_object != NULL, "remote allocator malloc") ||
      !expect(hz6_free_remote(&remote_allocator, remote_object),
              "remote transfer free") ||
      !expect(!hz6_free_remote(&remote_allocator, remote_object),
              "remote double-free rejected")) {
    return 1;
  }
  void* transfer_reused = hz6_malloc(&remote_allocator, 128);
  if (!expect(transfer_reused == remote_object, "transfer reuse")) {
    return 1;
  }
  hz6_free(&remote_allocator, transfer_reused);
  Hz6StatsSnapshot remote_stats = hz6_stats_snapshot(&remote_allocator);
  if (!expect(remote_stats.route_valid == 3, "remote stats valid") ||
      !expect(remote_stats.route_invalid == 1, "remote stats invalid") ||
      !expect(remote_stats.transfer_push == 1, "remote stats push") ||
      !expect(remote_stats.transfer_pop == 1, "remote stats pop") ||
      !expect(remote_stats.source_alloc == 1, "remote stats source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&remote_allocator);

  Hz6Allocator orphan_allocator;
  hz6_allocator_init_with_profile(&orphan_allocator, HZ6_PROFILE_SPEED);
  void* orphan_object = hz6_malloc(&orphan_allocator, 48);
  if (!expect(orphan_object != NULL, "orphan allocator malloc")) {
    return 1;
  }
  Hz6RouteResult orphan_route =
      hz6_route_backend_lookup(&orphan_allocator.route_backend, orphan_object);
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
              "orphan release") ||
      !expect(!hz6_owns(&orphan_allocator, orphan_object),
              "orphan route released") ||
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
      hz6_route_backend_lookup(&profile_scavenge_allocator.route_backend,
                               profile_cached);
  Hz6ObjectDescriptor* profile_scavenge_descriptor =
      (Hz6ObjectDescriptor*)profile_scavenge_route.descriptor;
  hz6_free(&profile_scavenge_allocator, profile_cached);
  if (!expect(profile_scavenge_descriptor != NULL,
              "profile scavenge descriptor") ||
      !expect(profile_scavenge_descriptor->state == HZ6_STATE_LOCAL_FREE,
              "profile scavenge starts local free") ||
      !expect(hz6_allocator_scavenge_profile(&profile_scavenge_allocator) == 1,
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
  hz6_allocator_init_with_profile(&adopt_source, HZ6_PROFILE_SPEED);
  hz6_allocator_init_with_profile(&adopt_target, HZ6_PROFILE_SPEED);
  void* adopt_object = hz6_malloc(&adopt_source, 48);
  if (!expect(adopt_object != NULL, "adopt source malloc")) {
    return 1;
  }
  Hz6RouteResult adopt_source_route =
      hz6_route_backend_lookup(&adopt_source.route_backend, adopt_object);
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

  printf("hz6-r1-allocator-smoke ok\n");
  return 0;
}
