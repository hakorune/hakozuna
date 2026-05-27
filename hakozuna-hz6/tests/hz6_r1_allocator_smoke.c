#include "../api/hz6_allocator.h"
#include "../fronts/large/hz6_large128_front.h"
#include "../fronts/hz6_front_util.h"
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

static void* g_expected_release_source_ptr;
static size_t g_expected_release_source_bytes;
static unsigned char g_source_block_storage[HZ6_MIDPAGE_RUN_BYTES];
static size_t g_source_block_release_count;

static int smoke_release_source(void* ptr, size_t bytes) {
  ++g_source_block_release_count;
  return ptr == g_expected_release_source_ptr &&
         bytes == g_expected_release_source_bytes;
}

static void* smoke_reserve_source(size_t bytes, size_t align) {
  (void)align;
  return bytes <= sizeof(g_source_block_storage) ? g_source_block_storage
                                                : NULL;
}

static int smoke_memory_ok(void* ptr, size_t bytes) {
  return ptr != NULL && bytes != 0;
}

int main(void) {
  int foreign = 0;

  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, HZ6_PROFILE_SPEED);
  if (!expect(allocator.profile.transfer_first == 1, "allocator profile") ||
      !expect(allocator.route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE,
              "speed profile page route backend")) {
    return 1;
  }

  Hz6Allocator strict_route_allocator;
  hz6_allocator_init_with_profile(&strict_route_allocator, HZ6_PROFILE_STRICT);
  Hz6Allocator rss_route_allocator;
  hz6_allocator_init_with_profile(&rss_route_allocator, HZ6_PROFILE_RSS);
  if (!expect(strict_route_allocator.route_backend.kind ==
                  HZ6_ROUTE_BACKEND_EXACT_TABLE,
              "strict profile exact route backend") ||
      !expect(rss_route_allocator.route_backend.kind ==
                  HZ6_ROUTE_BACKEND_EXACT_TABLE,
              "rss profile exact route backend")) {
    return 1;
  }
  hz6_allocator_destroy(&strict_route_allocator);
  hz6_allocator_destroy(&rss_route_allocator);

  unsigned char source_block[128];
  Hz6ObjectDescriptor source_descriptor;
  source_descriptor.ptr = source_block + 16;
  source_descriptor.bytes = 32;
  source_descriptor.source_ptr = source_block;
  source_descriptor.source_bytes = sizeof(source_block);
  source_descriptor.class_id = 1;
  source_descriptor.source_kind = HZ6_SOURCE_SYSTEM;
  source_descriptor.source_release = smoke_release_source;
  source_descriptor.owner = allocator.owner.token;
  source_descriptor.generation = 1;
  source_descriptor.state = HZ6_STATE_LOCAL_FREE;
  g_expected_release_source_ptr = source_block;
  g_expected_release_source_bytes = sizeof(source_block);
  if (!expect(hz6_allocator_release_descriptor_source(&source_descriptor),
              "descriptor release uses source pointer") ||
      !expect(g_source_block_release_count == 1,
              "descriptor direct release count") ||
      !expect(source_descriptor.ptr == NULL,
              "descriptor release clears user pointer") ||
      !expect(source_descriptor.source_ptr == NULL,
              "descriptor release clears source pointer")) {
    return 1;
  }
  g_source_block_release_count = 0;

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

  Hz6Allocator rss_prefill_allocator;
  hz6_allocator_init_with_profile(&rss_prefill_allocator, HZ6_PROFILE_RSS);
  size_t prefilled = hz6_front_prefill_source_kind(
      &rss_prefill_allocator, HZ6_FRONT_MIDPAGE, HZ6_MIDPAGE_CLASS_ID,
      HZ6_MIDPAGE_BYTES, HZ6_SOURCE_OS_PAGED,
      rss_prefill_allocator.profile.source_batch);
  if (!expect(prefilled == rss_prefill_allocator.profile.source_batch,
              "rss profile source prefill count") ||
      !expect(rss_prefill_allocator.stats.source_alloc == prefilled,
              "rss profile prefill source alloc")) {
    return 1;
  }
  for (size_t i = 0; i < prefilled; ++i) {
    void* prefetched = hz6_malloc(&rss_prefill_allocator, 16384);
    if (!expect(prefetched != NULL, "rss profile prefilled malloc")) {
      return 1;
    }
  }
  if (!expect(rss_prefill_allocator.stats.source_alloc == prefilled,
              "rss profile prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&rss_prefill_allocator);

  Hz6Allocator large_prefill_allocator;
  hz6_allocator_init_with_profile(&large_prefill_allocator, HZ6_PROFILE_RSS);
  size_t large_prefilled = hz6_large128_prefill(
      &large_prefill_allocator, large_prefill_allocator.profile.source_batch);
  if (!expect(large_prefilled ==
                  large_prefill_allocator.profile.source_batch,
              "large128 profile prefill count") ||
      !expect(large_prefill_allocator.stats.source_alloc == large_prefilled,
              "large128 profile prefill source alloc")) {
    return 1;
  }
  for (size_t i = 0; i < large_prefilled; ++i) {
    void* prefetched_large = hz6_malloc(&large_prefill_allocator, 70000);
    if (!expect(prefetched_large != NULL,
                "large128 profile prefilled malloc")) {
      return 1;
    }
  }
  if (!expect(large_prefill_allocator.stats.source_alloc == large_prefilled,
              "large128 profile prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&large_prefill_allocator);

  Hz6MidPageRunPolicy midpage8_policy;
  Hz6MidPageRunPolicy midpage32_policy;
  if (!expect(hz6_midpage_policy_for_size(6000, 16, &midpage8_policy),
              "midpage 8k policy") ||
      !expect(midpage8_policy.class_id == HZ6_MIDPAGE_8K_CLASS_ID,
              "midpage 8k class") ||
      !expect(midpage8_policy.slot_bytes == HZ6_MIDPAGE_8K_BYTES,
              "midpage 8k slot bytes") ||
      !expect(midpage8_policy.slots_per_run == 8,
              "midpage 8k slots per run") ||
      !expect(hz6_midpage_policy_for_size(16384, 16, &midpage32_policy),
              "midpage 32k policy") ||
      !expect(midpage32_policy.class_id == HZ6_MIDPAGE_32K_CLASS_ID,
              "midpage 32k class") ||
      !expect(midpage32_policy.slot_bytes == HZ6_MIDPAGE_32K_BYTES,
              "midpage 32k slot bytes") ||
      !expect(midpage32_policy.slots_per_run == 2,
              "midpage 32k slots per run")) {
    return 1;
  }

  Hz6Allocator midpage8_allocator;
  hz6_allocator_init_with_profile(&midpage8_allocator, HZ6_PROFILE_REMOTE);
  void* midpage8_object = hz6_malloc(&midpage8_allocator, 6000);
  if (!expect(midpage8_object != NULL, "midpage 8k malloc")) {
    return 1;
  }
  Hz6RouteResult midpage8_route =
      hz6_route_backend_lookup(&midpage8_allocator.route_backend,
                               midpage8_object);
  Hz6ObjectDescriptor* midpage8_descriptor =
      (Hz6ObjectDescriptor*)midpage8_route.descriptor;
  if (!expect(midpage8_route.kind == HZ6_ROUTE_VALID,
              "midpage 8k route valid") ||
      !expect(midpage8_route.front_id == HZ6_FRONT_MIDPAGE,
              "midpage 8k route front") ||
      !expect(midpage8_route.class_id == HZ6_MIDPAGE_8K_CLASS_ID,
              "midpage 8k route class") ||
      !expect(midpage8_descriptor != NULL, "midpage 8k descriptor") ||
      !expect(midpage8_descriptor->source_block != NULL,
              "midpage 8k auto run source block") ||
      !expect(midpage8_descriptor->bytes == HZ6_MIDPAGE_8K_BYTES,
              "midpage 8k descriptor bytes")) {
    return 1;
  }
  hz6_free(&midpage8_allocator, midpage8_object);
  void* midpage8_reused = hz6_malloc(&midpage8_allocator, 6000);
  if (!expect(midpage8_reused == midpage8_object,
              "midpage 8k local reuse")) {
    return 1;
  }
  hz6_free(&midpage8_allocator, midpage8_reused);
  if (!expect(midpage8_allocator.stats.source_alloc == 1,
              "midpage 8k auto prefill source alloc")) {
    return 1;
  }
  hz6_allocator_destroy(&midpage8_allocator);

  Hz6Allocator midpage_slot_allocator;
  hz6_allocator_init_with_profile(&midpage_slot_allocator,
                                  HZ6_PROFILE_REMOTE);
  void* midpage_slot = hz6_front_source_slot_kind(
      &midpage_slot_allocator, HZ6_FRONT_MIDPAGE, HZ6_MIDPAGE_8K_CLASS_ID,
      HZ6_MIDPAGE_8K_BYTES, HZ6_MIDPAGE_RUN_BYTES, HZ6_MIDPAGE_8K_BYTES,
      HZ6_SOURCE_OS_PAGED);
  if (!expect(midpage_slot != NULL, "midpage source slot malloc")) {
    return 1;
  }
  Hz6RouteResult midpage_slot_route =
      hz6_route_backend_lookup(&midpage_slot_allocator.route_backend,
                               midpage_slot);
  Hz6ObjectDescriptor* midpage_slot_descriptor =
      (Hz6ObjectDescriptor*)midpage_slot_route.descriptor;
  if (!expect(midpage_slot_route.kind == HZ6_ROUTE_VALID,
              "midpage source slot route valid") ||
      !expect(midpage_slot_descriptor != NULL,
              "midpage source slot descriptor") ||
      !expect(midpage_slot_descriptor->ptr == midpage_slot,
              "midpage source slot user pointer") ||
      !expect(midpage_slot_descriptor->source_ptr != midpage_slot,
              "midpage source slot source pointer split") ||
      !expect(midpage_slot_descriptor->bytes == HZ6_MIDPAGE_8K_BYTES,
              "midpage source slot user bytes") ||
      !expect(midpage_slot_descriptor->source_bytes == HZ6_MIDPAGE_RUN_BYTES,
              "midpage source slot source bytes") ||
      !expect(hz6_route_backend_lookup(
                  &midpage_slot_allocator.route_backend,
                  (unsigned char*)midpage_slot + 16).kind ==
                  HZ6_ROUTE_INVALID,
              "midpage source slot interior invalid")) {
    return 1;
  }
  hz6_free(&midpage_slot_allocator, midpage_slot);
  void* midpage_slot_reused = hz6_malloc(&midpage_slot_allocator, 6000);
  if (!expect(midpage_slot_reused == midpage_slot,
              "midpage source slot local reuse")) {
    return 1;
  }
  hz6_free(&midpage_slot_allocator, midpage_slot_reused);
  hz6_allocator_destroy(&midpage_slot_allocator);

  Hz6Allocator midpage_block_allocator;
  hz6_allocator_init_with_profile(&midpage_block_allocator,
                                  HZ6_PROFILE_REMOTE);
  Hz6OsMemoryOps smoke_source_ops;
  smoke_source_ops.reserve = smoke_reserve_source;
  smoke_source_ops.commit = smoke_memory_ok;
  smoke_source_ops.decommit = smoke_memory_ok;
  smoke_source_ops.release = smoke_release_source;
  smoke_source_ops.page_size = 16;
  smoke_source_ops.allocation_granularity = 16;
  g_expected_release_source_ptr = g_source_block_storage;
  g_expected_release_source_bytes = sizeof(g_source_block_storage);
  g_source_block_release_count = 0;
  Hz6SourceBlock* block = hz6_allocator_create_source_block(
      &midpage_block_allocator, sizeof(g_source_block_storage),
      &smoke_source_ops, HZ6_SOURCE_SYSTEM);
  void* block_slot0 = hz6_front_source_block_slot(
      &midpage_block_allocator, HZ6_FRONT_MIDPAGE,
      HZ6_MIDPAGE_8K_CLASS_ID, HZ6_MIDPAGE_8K_BYTES, 0, block);
  void* block_slot1 = hz6_front_source_block_slot(
      &midpage_block_allocator, HZ6_FRONT_MIDPAGE,
      HZ6_MIDPAGE_8K_CLASS_ID, HZ6_MIDPAGE_8K_BYTES,
      HZ6_MIDPAGE_8K_BYTES, block);
  if (!expect(block != NULL, "source block create") ||
      !expect(block_slot0 == g_source_block_storage,
              "source block slot zero") ||
      !expect(block_slot1 == g_source_block_storage + HZ6_MIDPAGE_8K_BYTES,
              "source block slot one") ||
      !expect(block->ref_count == 2, "source block refcount")) {
    return 1;
  }
  void* duplicate_block_slot = hz6_front_source_block_slot(
      &midpage_block_allocator, HZ6_FRONT_MIDPAGE,
      HZ6_MIDPAGE_8K_CLASS_ID, HZ6_MIDPAGE_8K_BYTES, 0, block);
  if (!expect(duplicate_block_slot == NULL,
              "source block duplicate slot rejected") ||
      !expect(block->ref_count == 2,
              "source block duplicate slot refcount restored") ||
      !expect(g_source_block_release_count == 0,
              "source block duplicate slot keeps source alive")) {
    return 1;
  }
  void* block_slot2 = hz6_front_source_block_slot(
      &midpage_block_allocator, HZ6_FRONT_MIDPAGE,
      HZ6_MIDPAGE_8K_CLASS_ID, HZ6_MIDPAGE_8K_BYTES,
      2 * HZ6_MIDPAGE_8K_BYTES, block);
  if (!expect(block_slot2 ==
                  g_source_block_storage + (2 * HZ6_MIDPAGE_8K_BYTES),
              "source block slot after duplicate failure") ||
      !expect(block->ref_count == 3,
              "source block duplicate failure descriptor reused")) {
    return 1;
  }
  Hz6RouteResult block_slot0_route =
      hz6_route_backend_lookup(&midpage_block_allocator.route_backend,
                               block_slot0);
  Hz6RouteResult block_slot1_route =
      hz6_route_backend_lookup(&midpage_block_allocator.route_backend,
                               block_slot1);
  Hz6RouteResult block_slot2_route =
      hz6_route_backend_lookup(&midpage_block_allocator.route_backend,
                               block_slot2);
  Hz6ObjectDescriptor* block_slot0_descriptor =
      (Hz6ObjectDescriptor*)block_slot0_route.descriptor;
  Hz6ObjectDescriptor* block_slot1_descriptor =
      (Hz6ObjectDescriptor*)block_slot1_route.descriptor;
  Hz6ObjectDescriptor* block_slot2_descriptor =
      (Hz6ObjectDescriptor*)block_slot2_route.descriptor;
  if (!expect(block_slot0_descriptor != NULL, "block slot0 descriptor") ||
      !expect(block_slot1_descriptor != NULL, "block slot1 descriptor") ||
      !expect(block_slot2_descriptor != NULL, "block slot2 descriptor") ||
      !expect(block_slot0_descriptor->source_block == block,
              "block slot0 source block") ||
      !expect(block_slot1_descriptor->source_block == block,
              "block slot1 source block") ||
      !expect(block_slot2_descriptor->source_block == block,
              "block slot2 source block") ||
      !expect(hz6_route_backend_lookup(
                  &midpage_block_allocator.route_backend,
                  g_source_block_storage + (3 * HZ6_MIDPAGE_8K_BYTES)).kind ==
                  HZ6_ROUTE_INVALID,
              "block unregistered slot invalid")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&midpage_block_allocator.route_backend,
                                     block_slot2);
  if (!expect(hz6_allocator_release_descriptor_source(
                  block_slot2_descriptor),
              "source block extra slot release") ||
      !expect(block->ref_count == 2,
              "source block extra slot release decremented") ||
      !expect(g_source_block_release_count == 0,
              "source block extra slot release keeps source alive")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&midpage_block_allocator.route_backend,
                                     block_slot0);
  if (!expect(hz6_allocator_release_descriptor_source(
                  block_slot0_descriptor),
              "source block first release") ||
      !expect(block->active, "source block retained after first release") ||
      !expect(block->ref_count == 1, "source block decremented") ||
      !expect(g_source_block_release_count == 0,
              "source block not released early")) {
    return 1;
  }
  hz6_route_backend_unregister_exact(&midpage_block_allocator.route_backend,
                                     block_slot1);
  if (!expect(hz6_allocator_release_descriptor_source(
                  block_slot1_descriptor),
              "source block final release") ||
      !expect(!block->active, "source block inactive after final release") ||
      !expect(g_source_block_release_count == 1,
              "source block released once") ||
      !expect(hz6_route_backend_lookup(
                  &midpage_block_allocator.route_backend,
                  g_source_block_storage + (2 * HZ6_MIDPAGE_8K_BYTES)).kind ==
                  HZ6_ROUTE_MISS,
              "block envelope unregistered after final release")) {
    return 1;
  }
  hz6_allocator_destroy(&midpage_block_allocator);

  Hz6Allocator midpage_run_allocator;
  hz6_allocator_init_with_profile(&midpage_run_allocator,
                                  HZ6_PROFILE_REMOTE);
  size_t run_prefilled =
      hz6_midpage_prefill_run(&midpage_run_allocator,
                              HZ6_MIDPAGE_8K_CLASS_ID);
  if (!expect(run_prefilled == 8, "midpage run prefill count") ||
      !expect(midpage_run_allocator.stats.source_alloc == 1,
              "midpage run source allocation count")) {
    return 1;
  }
  void* run_slots[8];
  Hz6SourceBlock* run_block = NULL;
  for (size_t i = 0; i < run_prefilled; ++i) {
    run_slots[i] = hz6_malloc(&midpage_run_allocator, 6000);
    Hz6RouteResult run_route =
        hz6_route_backend_lookup(&midpage_run_allocator.route_backend,
                                 run_slots[i]);
    Hz6ObjectDescriptor* run_descriptor =
        (Hz6ObjectDescriptor*)run_route.descriptor;
    if (!expect(run_slots[i] != NULL, "midpage run malloc") ||
        !expect(run_route.kind == HZ6_ROUTE_VALID,
                "midpage run route valid") ||
        !expect(run_route.class_id == HZ6_MIDPAGE_8K_CLASS_ID,
                "midpage run route class") ||
        !expect(run_descriptor != NULL, "midpage run descriptor") ||
        !expect(run_descriptor->source_block != NULL,
                "midpage run source block")) {
      return 1;
    }
    if (i == 0) {
      run_block = run_descriptor->source_block;
    } else if (!expect(run_descriptor->source_block == run_block,
                       "midpage run shared source block")) {
      return 1;
    }
  }
  if (!expect(midpage_run_allocator.stats.source_alloc == 1,
              "midpage run prefill avoids refill")) {
    return 1;
  }
  for (size_t i = 0; i < run_prefilled; ++i) {
    hz6_free(&midpage_run_allocator, run_slots[i]);
  }
  hz6_allocator_destroy(&midpage_run_allocator);

  Hz6Allocator midpage_32k_run_allocator;
  hz6_allocator_init_with_profile(&midpage_32k_run_allocator,
                                  HZ6_PROFILE_REMOTE);
  size_t run32_prefilled =
      hz6_midpage_prefill_run(&midpage_32k_run_allocator,
                              HZ6_MIDPAGE_32K_CLASS_ID);
  if (!expect(run32_prefilled == 2, "midpage 32k run prefill count") ||
      !expect(midpage_32k_run_allocator.stats.source_alloc == 1,
              "midpage 32k run source allocation count")) {
    return 1;
  }
  void* run32_slots[2];
  Hz6SourceBlock* run32_block = NULL;
  for (size_t i = 0; i < run32_prefilled; ++i) {
    run32_slots[i] = hz6_malloc(&midpage_32k_run_allocator, 20000);
    Hz6RouteResult run32_route =
        hz6_route_backend_lookup(&midpage_32k_run_allocator.route_backend,
                                 run32_slots[i]);
    Hz6ObjectDescriptor* run32_descriptor =
        (Hz6ObjectDescriptor*)run32_route.descriptor;
    if (!expect(run32_slots[i] != NULL, "midpage 32k run malloc") ||
        !expect(run32_route.kind == HZ6_ROUTE_VALID,
                "midpage 32k run route valid") ||
        !expect(run32_route.class_id == HZ6_MIDPAGE_32K_CLASS_ID,
                "midpage 32k run route class") ||
        !expect(run32_descriptor != NULL, "midpage 32k run descriptor") ||
        !expect(run32_descriptor->source_block != NULL,
                "midpage 32k run source block")) {
      return 1;
    }
    if (i == 0) {
      run32_block = run32_descriptor->source_block;
    } else if (!expect(run32_descriptor->source_block == run32_block,
                       "midpage 32k run shared source block")) {
      return 1;
    }
  }
  if (!expect(midpage_32k_run_allocator.stats.source_alloc == 1,
              "midpage 32k run prefill avoids refill")) {
    return 1;
  }
  for (size_t i = 0; i < run32_prefilled; ++i) {
    hz6_free(&midpage_32k_run_allocator, run32_slots[i]);
  }
  hz6_allocator_destroy(&midpage_32k_run_allocator);

  Hz6Allocator midpage_allocator;
  hz6_allocator_init_with_profile(&midpage_allocator, HZ6_PROFILE_REMOTE);
  if (!expect(midpage_allocator.route_backend.kind ==
                  HZ6_ROUTE_BACKEND_PAGE_TABLE,
              "remote profile page route backend") ||
      !expect(midpage_allocator.route_backend.page_granularity ==
                  HZ6_ROUTE_PAGE_GRANULARITY,
              "remote profile page route granularity")) {
    return 1;
  }
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
      hz6_route_backend_lookup(&strict_remote_allocator.route_backend,
                               strict_remote);
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
