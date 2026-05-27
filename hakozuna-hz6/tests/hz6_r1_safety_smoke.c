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

  printf("hz6-r1-safety-smoke ok\n");
  return 0;
}
