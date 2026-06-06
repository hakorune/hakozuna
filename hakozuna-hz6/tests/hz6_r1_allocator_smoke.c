#include "../api/hz6_allocator.h"
#include "../fronts/large/hz6_large128_front.h"
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

static size_t smoke_large128_central_count(const Hz6Allocator* allocator) {
  return hz6_allocator_large_span_pool_count(allocator,
                                             HZ6_LARGE128_CLASS_ID);
}

static size_t smoke_large128_central_bytes(const Hz6Allocator* allocator) {
  return hz6_allocator_large_span_pool_bytes(allocator,
                                             HZ6_LARGE128_CLASS_ID);
}

int main(void) {
  int foreign = 0;

  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, HZ6_PROFILE_SPEED);
  if (!expect(hz6_allocator_profile_transfer_first(&allocator), "allocator profile") ||
      !expect(hz6_allocator_route_backend_kind(&allocator) ==
                  HZ6_ROUTE_BACKEND_PAGE_TABLE,
              "speed profile page route backend")) {
    return 1;
  }

  Hz6Allocator strict_route_allocator;
  hz6_allocator_init_with_profile(&strict_route_allocator, HZ6_PROFILE_STRICT);
  Hz6Allocator rss_route_allocator;
  hz6_allocator_init_with_profile(&rss_route_allocator, HZ6_PROFILE_RSS);
  if (!expect(hz6_allocator_route_backend_kind(&strict_route_allocator) ==
                  HZ6_ROUTE_BACKEND_EXACT_TABLE,
              "strict profile exact route backend") ||
      !expect(hz6_allocator_route_backend_kind(&rss_route_allocator) ==
                  HZ6_ROUTE_BACKEND_EXACT_TABLE,
              "rss profile exact route backend")) {
    return 1;
  }
  hz6_allocator_destroy(&strict_route_allocator);
  hz6_allocator_destroy(&rss_route_allocator);

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
      !expect(stats.source_alloc == 1, "stats source block alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&allocator);

  Hz6Allocator midpage_allocator;
  hz6_allocator_init_with_profile(&midpage_allocator, HZ6_PROFILE_REMOTE);
  if (!expect(hz6_allocator_route_backend_kind(&midpage_allocator) ==
                  HZ6_ROUTE_BACKEND_PAGE_TABLE,
              "remote profile page route backend") ||
      !expect(hz6_allocator_route_page_granularity(&midpage_allocator) ==
                  HZ6_ROUTE_PAGE_GRANULARITY,
              "remote profile page route granularity")) {
    return 1;
  }
  void* midpage_object = hz6_malloc(&midpage_allocator, 16384);
  if (!expect(midpage_object != NULL, "midpage malloc")) {
    return 1;
  }
  Hz6RouteResult midpage_route =
      hz6_allocator_route_lookup(&midpage_allocator, midpage_object);
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
      hz6_allocator_route_lookup(&large_allocator, large_object);
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
      !expect(large_descriptor->source_ptr == large_object,
              "large128 source pointer") ||
      !expect(large_descriptor->source_bytes == HZ6_LARGE128_BYTES,
              "large128 source bytes") ||
      !expect(hz6_free_remote(&large_allocator, large_object),
              "large128 remote free")) {
    return 1;
  }
  if (!expect(large_descriptor->state == HZ6_STATE_CENTRAL_FREE,
              "large128 central free state") ||
      !expect(smoke_large128_central_count(&large_allocator) == 1,
              "large128 central pool count") ||
      !expect(smoke_large128_central_bytes(&large_allocator) ==
                  HZ6_LARGE128_BYTES,
              "large128 central pool bytes") ||
      !expect(hz6_allocator_large_span_pool_global_bytes(&large_allocator) ==
                  HZ6_LARGE128_BYTES,
              "large128 central global bytes")) {
    return 1;
  }
  void* large_reused = hz6_malloc(&large_allocator, 70000);
  if (!expect(large_reused == large_object, "large128 transfer reuse")) {
    return 1;
  }
  if (!expect(smoke_large128_central_count(&large_allocator) == 0,
              "large128 central pool empty") ||
      !expect(smoke_large128_central_bytes(&large_allocator) == 0,
              "large128 central pool bytes empty") ||
      !expect(hz6_allocator_large_span_pool_global_bytes(&large_allocator) ==
                  0,
              "large128 central global bytes empty")) {
    return 1;
  }
  hz6_free(&large_allocator, large_reused);
  Hz6StatsSnapshot large_stats = hz6_stats_snapshot(&large_allocator);
  if (!expect(large_stats.transfer_push == 0, "large128 transfer push") ||
      !expect(large_stats.transfer_pop == 0, "large128 transfer pop") ||
      !expect(large_stats.source_alloc == 1,
              "large128 source alloc")) {
    return 1;
  }
  if (!expect(hz6_malloc(&large_allocator, HZ6_LARGE128_BYTES + 1) == NULL,
              "unsupported over-large allocation")) {
    return 1;
  }
  hz6_allocator_destroy(&large_allocator);

  printf("hz6-r1-allocator-smoke ok\n");
  return 0;
}
