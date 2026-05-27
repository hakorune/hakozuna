#include "../api/hz6_allocator.h"
#include "../include/hz6_contract.h"

#include <stdio.h>

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-safety-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  int foreign = 0;

  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, HZ6_PROFILE_REMOTE);

  void* p = hz6_malloc(&allocator, 128);
  if (!expect(p != NULL, "malloc")) {
    return 1;
  }

  hz6_free(&allocator, (unsigned char*)p + 1);
  hz6_free(&allocator, &foreign);
  Hz6StatsSnapshot after_bad_free = hz6_stats_snapshot(&allocator);
  if (!expect(after_bad_free.route_invalid == 1, "interior invalid") ||
      !expect(after_bad_free.route_miss == 1, "foreign miss")) {
    return 1;
  }

  hz6_free(&allocator, p);
  hz6_free(&allocator, p);
  Hz6StatsSnapshot after_double_free = hz6_stats_snapshot(&allocator);
  if (!expect(after_double_free.route_invalid == 2,
              "local double-free invalid")) {
    return 1;
  }

  void* reused = hz6_malloc(&allocator, 128);
  if (!expect(reused == p, "reuse after local free")) {
    return 1;
  }
  hz6_free(&allocator, reused);
  hz6_allocator_destroy(&allocator);

  Hz6Allocator remote_allocator;
  hz6_allocator_init_with_profile(&remote_allocator, HZ6_PROFILE_REMOTE);
  void* remote = hz6_malloc(&remote_allocator, 128);
  if (!expect(remote != NULL, "remote malloc") ||
      !expect(hz6_free_remote(&remote_allocator, remote),
              "remote free") ||
      !expect(!hz6_free_remote(&remote_allocator, remote),
              "remote double-free rejected")) {
    return 1;
  }
  Hz6StatsSnapshot remote_stats = hz6_stats_snapshot(&remote_allocator);
  if (!expect(remote_stats.route_invalid == 1,
              "remote double-free invalid")) {
    return 1;
  }
  void* remote_reused = hz6_malloc(&remote_allocator, 128);
  if (!expect(remote_reused == remote, "remote transfer reuse")) {
    return 1;
  }
  hz6_free(&remote_allocator, remote_reused);
  hz6_allocator_destroy(&remote_allocator);

  Hz6Allocator orphan_allocator;
  hz6_allocator_init_with_profile(&orphan_allocator, HZ6_PROFILE_SPEED);
  void* orphan = hz6_malloc(&orphan_allocator, 48);
  if (!expect(orphan != NULL, "orphan malloc")) {
    return 1;
  }
  Hz6RouteResult route =
      hz6_route_backend_lookup(&orphan_allocator.route_backend, orphan);
  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (!expect(route.kind == HZ6_ROUTE_VALID, "orphan route valid") ||
      !expect(descriptor != NULL, "orphan descriptor")) {
    return 1;
  }

  hz6_allocator_mark_owner_dead(&orphan_allocator);
  if (!expect(orphan_allocator.owner.state == HZ6_OWNER_DEAD,
              "owner dead") ||
      !expect(descriptor->state == HZ6_STATE_ORPHAN,
              "descriptor orphan") ||
      !expect(hz6_malloc(&orphan_allocator, 48) == NULL,
              "dead owner malloc rejected")) {
    return 1;
  }

  hz6_free(&orphan_allocator, orphan);
  Hz6StatsSnapshot orphan_stats = hz6_stats_snapshot(&orphan_allocator);
  if (!expect(orphan_stats.route_invalid == 1, "orphan free invalid") ||
      !expect(hz6_allocator_release_orphan(&orphan_allocator, orphan),
              "orphan release") ||
      !expect(!hz6_owns(&orphan_allocator, orphan),
              "orphan route gone") ||
      !expect(descriptor->state == HZ6_STATE_DEAD,
              "orphan descriptor dead")) {
    return 1;
  }
  hz6_allocator_destroy(&orphan_allocator);

  Hz6Allocator scavenge_allocator;
  hz6_allocator_init_with_profile(&scavenge_allocator, HZ6_PROFILE_SPEED);
  void* scavenged = hz6_malloc(&scavenge_allocator, 48);
  if (!expect(scavenged != NULL, "scavenge malloc")) {
    return 1;
  }
  Hz6RouteResult scavenge_route =
      hz6_route_backend_lookup(&scavenge_allocator.route_backend, scavenged);
  Hz6ObjectDescriptor* scavenge_descriptor =
      (Hz6ObjectDescriptor*)scavenge_route.descriptor;
  if (!expect(scavenge_descriptor != NULL, "scavenge descriptor")) {
    return 1;
  }
  hz6_allocator_mark_owner_dead(&scavenge_allocator);
  if (!expect(hz6_allocator_scavenge_orphans(&scavenge_allocator, 1) == 0,
              "scavenge budget too small") ||
      !expect(scavenge_descriptor->state == HZ6_STATE_ORPHAN,
              "scavenge keeps orphan over budget") ||
      !expect(hz6_allocator_scavenge_orphans(&scavenge_allocator, 128) == 1,
              "scavenge releases orphan") ||
      !expect(!hz6_owns(&scavenge_allocator, scavenged),
              "scavenge route gone") ||
      !expect(scavenge_descriptor->state == HZ6_STATE_DEAD,
              "scavenge descriptor dead")) {
    return 1;
  }
  hz6_allocator_destroy(&scavenge_allocator);

  Hz6Allocator local_scavenge_allocator;
  hz6_allocator_init_with_profile(&local_scavenge_allocator,
                                  HZ6_PROFILE_SPEED);
  void* local_scavenged = hz6_malloc(&local_scavenge_allocator, 48);
  if (!expect(local_scavenged != NULL, "local scavenge malloc")) {
    return 1;
  }
  Hz6RouteResult local_scavenge_route =
      hz6_route_backend_lookup(&local_scavenge_allocator.route_backend,
                               local_scavenged);
  Hz6ObjectDescriptor* local_scavenge_descriptor =
      (Hz6ObjectDescriptor*)local_scavenge_route.descriptor;
  if (!expect(local_scavenge_descriptor != NULL,
              "local scavenge descriptor")) {
    return 1;
  }
  hz6_free(&local_scavenge_allocator, local_scavenged);
  if (!expect(local_scavenge_descriptor->state == HZ6_STATE_LOCAL_FREE,
              "local scavenge starts local free") ||
      !expect(hz6_allocator_scavenge_local_free(&local_scavenge_allocator,
                                                1) == 0,
              "local scavenge budget too small") ||
      !expect(local_scavenge_descriptor->state == HZ6_STATE_LOCAL_FREE,
              "local scavenge keeps over-budget cache") ||
      !expect(hz6_allocator_scavenge_local_free(&local_scavenge_allocator,
                                                128) == 1,
              "local scavenge releases cache") ||
      !expect(!hz6_owns(&local_scavenge_allocator, local_scavenged),
              "local scavenge route gone") ||
      !expect(local_scavenge_descriptor->state == HZ6_STATE_DEAD,
              "local scavenge descriptor dead")) {
    return 1;
  }
  hz6_allocator_destroy(&local_scavenge_allocator);

  Hz6Allocator strict_scavenge_allocator;
  hz6_allocator_init_with_profile(&strict_scavenge_allocator,
                                  HZ6_PROFILE_STRICT);
  void* strict_scavenged = hz6_malloc(&strict_scavenge_allocator, 48);
  if (!expect(strict_scavenged != NULL, "strict profile scavenge malloc")) {
    return 1;
  }
  Hz6RouteResult strict_scavenge_route =
      hz6_route_backend_lookup(&strict_scavenge_allocator.route_backend,
                               strict_scavenged);
  Hz6ObjectDescriptor* strict_scavenge_descriptor =
      (Hz6ObjectDescriptor*)strict_scavenge_route.descriptor;
  hz6_free(&strict_scavenge_allocator, strict_scavenged);
  if (!expect(strict_scavenge_descriptor != NULL,
              "strict profile scavenge descriptor") ||
      !expect(hz6_allocator_scavenge_profile(&strict_scavenge_allocator) == 0,
              "strict profile does not scavenge automatically") ||
      !expect(strict_scavenge_descriptor->state == HZ6_STATE_LOCAL_FREE,
              "strict profile keeps local cache")) {
    return 1;
  }
  hz6_allocator_destroy(&strict_scavenge_allocator);

  Hz6Allocator live_adopt_source;
  Hz6Allocator live_adopt_target;
  hz6_allocator_init_with_profile(&live_adopt_source, HZ6_PROFILE_SPEED);
  hz6_allocator_init_with_profile(&live_adopt_target, HZ6_PROFILE_SPEED);
  void* live_adopt = hz6_malloc(&live_adopt_source, 48);
  if (!expect(live_adopt != NULL, "live adopt malloc") ||
      !expect(!hz6_allocator_adopt_orphan(&live_adopt_target,
                                          &live_adopt_source,
                                          live_adopt),
              "cannot adopt active object")) {
    return 1;
  }
  hz6_free(&live_adopt_source, live_adopt);
  hz6_allocator_destroy(&live_adopt_target);
  hz6_allocator_destroy(&live_adopt_source);

  printf("hz6-r1-safety-smoke ok\n");
  return 0;
}
