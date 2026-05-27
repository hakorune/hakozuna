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

static size_t smoke_profile_refill_batch(const Hz6Allocator* allocator) {
  return hz6_allocator_profile_source_refill_batch(allocator, 0, 0);
}

static size_t smoke_frontcache_capped_batch(const Hz6Allocator* allocator) {
  size_t batch = smoke_profile_refill_batch(allocator);
  return batch < HZ6_FRONT_CACHE_BIN_CAPACITY ? batch
                                              : HZ6_FRONT_CACHE_BIN_CAPACITY;
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
      !expect(stats.source_alloc == 1, "stats source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&allocator);

  Hz6Allocator local2p_allocator;
  hz6_allocator_init_with_profile(&local2p_allocator, HZ6_PROFILE_REMOTE);
  if (!expect(hz6_allocator_transfer_backend_kind(&local2p_allocator) ==
                  HZ6_TRANSFER_BACKEND_SHARDED_CACHE,
              "remote profile sharded transfer")) {
    return 1;
  }
  if (!expect(hz6_allocator_transfer_capacity(&local2p_allocator) ==
                  HZ6_TRANSFER_CACHE_CAPACITY,
              "remote profile transfer capacity")) {
    return 1;
  }
  void* local2p_object = hz6_malloc(&local2p_allocator, HZ6_LOCAL2P_BYTES);
  if (!expect(local2p_object != NULL, "local2p malloc")) {
    return 1;
  }
  Hz6RouteResult local2p_route =
      hz6_allocator_route_lookup(&local2p_allocator, local2p_object);
  Hz6ObjectDescriptor* local2p_descriptor =
      (Hz6ObjectDescriptor*)local2p_route.descriptor;
  if (!expect(local2p_route.kind == HZ6_ROUTE_VALID,
              "local2p route valid") ||
      !expect(local2p_route.front_id == HZ6_FRONT_LOCAL2P,
              "local2p route front") ||
      !expect(local2p_route.class_id == HZ6_LOCAL2P_CLASS_ID,
              "local2p route class") ||
      !expect(local2p_descriptor != NULL, "local2p descriptor") ||
      !expect(hz6_owner_equal(
                  local2p_descriptor->owner,
                  hz6_allocator_owner_token(&local2p_allocator)),
              "local2p owner assigned") ||
      !expect(hz6_free_remote(&local2p_allocator, local2p_object),
              "local2p remote free")) {
    return 1;
  }
  if (!expect(local2p_descriptor->state == HZ6_STATE_TRANSFER_FREE,
              "local2p transfer state") ||
      !expect(local2p_descriptor->owner.slot == 0 &&
                  local2p_descriptor->owner.generation == 0,
              "local2p transfer owner clear") ||
      !expect(hz6_allocator_transfer_count(&local2p_allocator) == 1,
              "local2p transfer count") ||
      !expect(hz6_allocator_transfer_count_class(
                  &local2p_allocator, HZ6_LOCAL2P_CLASS_ID) == 1,
              "local2p transfer class count") ||
      !expect(hz6_allocator_transfer_shard_capacity_at(
                  &local2p_allocator, 0) != 0,
              "local2p transfer shard capacity")) {
    return 1;
  }
  void* local2p_reused = hz6_malloc(&local2p_allocator, HZ6_LOCAL2P_BYTES);
  if (!expect(local2p_reused == local2p_object, "local2p transfer reuse")) {
    return 1;
  }
  if (!expect(hz6_owner_equal(
                  local2p_descriptor->owner,
                  hz6_allocator_owner_token(&local2p_allocator)),
              "local2p owner restored")) {
    return 1;
  }
  hz6_free(&local2p_allocator, local2p_reused);
  Hz6StatsSnapshot local2p_stats = hz6_stats_snapshot(&local2p_allocator);
  size_t local2p_expected_source =
      smoke_frontcache_capped_batch(&local2p_allocator);
  if (!expect(local2p_stats.transfer_push == 1, "local2p transfer push") ||
      !expect(local2p_stats.transfer_pop == 1, "local2p transfer pop") ||
      !expect(local2p_stats.source_alloc == local2p_expected_source,
              "local2p source batch alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&local2p_allocator);

  Hz6Allocator rss_capacity_allocator;
  hz6_allocator_init_with_profile(&rss_capacity_allocator, HZ6_PROFILE_RSS);
  if (!expect(hz6_allocator_transfer_capacity(&rss_capacity_allocator) ==
                  hz6_allocator_profile_transfer_capacity(&rss_capacity_allocator),
              "rss profile transfer capacity") ||
      !expect(hz6_allocator_transfer_backend_kind(&rss_capacity_allocator) ==
                  HZ6_TRANSFER_BACKEND_SINGLE_CACHE,
              "rss profile single transfer")) {
    return 1;
  }
  hz6_allocator_destroy(&rss_capacity_allocator);

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
  void* large_reused = hz6_malloc(&large_allocator, 70000);
  if (!expect(large_reused == large_object, "large128 transfer reuse")) {
    return 1;
  }
  hz6_free(&large_allocator, large_reused);
  Hz6StatsSnapshot large_stats = hz6_stats_snapshot(&large_allocator);
  size_t large_expected_source =
      smoke_frontcache_capped_batch(&large_allocator);
  if (!expect(large_stats.transfer_push == 1, "large128 transfer push") ||
      !expect(large_stats.transfer_pop == 1, "large128 transfer pop") ||
      !expect(large_stats.source_alloc == large_expected_source,
              "large128 source batch alloc")) {
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

  Hz6Allocator consumer_shard_allocator;
  hz6_allocator_init_with_profile(&consumer_shard_allocator,
                                  HZ6_PROFILE_REMOTE);
  if (!expect(hz6_allocator_debug_set_owner_slot(&consumer_shard_allocator, 1),
              "consumer shard owner slot setup")) {
    return 1;
  }
  void* consumer_shard_a = hz6_malloc(&consumer_shard_allocator, 128);
  void* consumer_shard_b = hz6_malloc(&consumer_shard_allocator, 128);
  if (!expect(consumer_shard_a != NULL, "consumer shard malloc a") ||
      !expect(consumer_shard_b != NULL, "consumer shard malloc b") ||
      !expect(hz6_free_remote(&consumer_shard_allocator, consumer_shard_a),
              "consumer shard remote free a") ||
      !expect(hz6_free_remote(&consumer_shard_allocator, consumer_shard_b),
              "consumer shard remote free b") ||
      !expect(hz6_allocator_transfer_shard_count_at(
                  &consumer_shard_allocator, 0) == 0,
              "consumer shard fallback shard empty") ||
      !expect(hz6_allocator_transfer_shard_count_at(
                  &consumer_shard_allocator, 1) == 2,
              "consumer shard producer shard count")) {
    return 1;
  }
  void* consumer_shard_reused = hz6_malloc(&consumer_shard_allocator, 128);
  if (!expect(consumer_shard_reused == consumer_shard_b,
              "consumer shard producer home pop")) {
    return 1;
  }
  hz6_free(&consumer_shard_allocator, consumer_shard_reused);
  hz6_allocator_destroy(&consumer_shard_allocator);

  Hz6Allocator strict_remote_allocator;
  hz6_allocator_init_with_profile(&strict_remote_allocator,
                                  HZ6_PROFILE_STRICT);
  void* strict_remote = hz6_malloc(&strict_remote_allocator, 128);
  if (!expect(strict_remote != NULL, "strict remote malloc") ||
      !expect(hz6_free_remote(&strict_remote_allocator, strict_remote),
              "strict remote free")) {
    return 1;
  }
  Hz6RouteResult strict_remote_route =
      hz6_allocator_route_lookup(&strict_remote_allocator, strict_remote);
  Hz6ObjectDescriptor* strict_remote_descriptor =
      (Hz6ObjectDescriptor*)strict_remote_route.descriptor;
  if (!expect(strict_remote_descriptor != NULL,
              "strict remote descriptor") ||
      !expect(strict_remote_descriptor->state == HZ6_STATE_REMOTE_PENDING,
              "strict remote pending") ||
      !expect(!hz6_free_remote(&strict_remote_allocator, strict_remote),
              "strict remote double-free rejected") ||
      !expect(hz6_allocator_drain_remote_pending(&strict_remote_allocator) ==
                  1,
              "strict remote drain")) {
    return 1;
  }
  void* strict_remote_reused = hz6_malloc(&strict_remote_allocator, 128);
  if (!expect(strict_remote_reused == strict_remote,
              "strict remote drained reuse")) {
    return 1;
  }
  hz6_free(&strict_remote_allocator, strict_remote_reused);
  hz6_allocator_destroy(&strict_remote_allocator);

  printf("hz6-r1-allocator-smoke ok\n");
  return 0;
}
