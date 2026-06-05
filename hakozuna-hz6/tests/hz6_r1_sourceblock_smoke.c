#include "../api/hz6_allocator.h"
#include "../fronts/hz6_front_source.h"
#include "../fronts/hz6_front_source_block.h"
#include "../fronts/midpage/hz6_midpage_front.h"
#include "../include/hz6_contract.h"
#include "../route/hz6_route.h"

#include <stdio.h>

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-sourceblock-smoke failed: %s\n", label);
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

static size_t smoke_source_alloc_count(const Hz6Allocator* allocator) {
  return hz6_stats_snapshot(allocator).source_alloc;
}

int main(void) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, HZ6_PROFILE_SPEED);

  unsigned char source_block[128];
  Hz6ObjectDescriptor source_descriptor;
  source_descriptor.ptr = source_block + 16;
  source_descriptor.bytes = 32;
  source_descriptor.source_ptr = source_block;
  source_descriptor.source_bytes = sizeof(source_block);
  source_descriptor.class_id = 1;
  source_descriptor.source_kind = HZ6_SOURCE_SYSTEM;
  source_descriptor.source_release = smoke_release_source;
#if !(HZ6_DESCRIPTOR_SIDE_OWNER16_L1 || HZ6_DESCRIPTOR_STORAGE_OWNER16_L1)
  source_descriptor.owner = hz6_allocator_owner_token(&allocator);
#endif
  source_descriptor.generation = 1;
  source_descriptor.state = HZ6_STATE_LOCAL_FREE;
  g_expected_release_source_ptr = source_block;
  g_expected_release_source_bytes = sizeof(source_block);
  if (!expect(hz6_allocator_release_descriptor_source(&allocator,
                                                      &source_descriptor),
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
  hz6_allocator_destroy(&allocator);

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
      hz6_allocator_route_lookup(&midpage8_allocator, midpage8_object);
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
  if (!expect(smoke_source_alloc_count(&midpage8_allocator) == 1,
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
      hz6_allocator_route_lookup(&midpage_slot_allocator, midpage_slot);
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
      !expect(hz6_allocator_route_lookup(
                  &midpage_slot_allocator,
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
      !expect(hz6_source_block_ref_count(block) == 2,
              "source block refcount")) {
    return 1;
  }
  void* duplicate_block_slot = hz6_front_source_block_slot(
      &midpage_block_allocator, HZ6_FRONT_MIDPAGE,
      HZ6_MIDPAGE_8K_CLASS_ID, HZ6_MIDPAGE_8K_BYTES, 0, block);
  if (!expect(duplicate_block_slot == NULL,
              "source block duplicate slot rejected") ||
      !expect(hz6_source_block_ref_count(block) == 2,
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
      !expect(hz6_source_block_ref_count(block) == 3,
              "source block duplicate failure descriptor reused")) {
    return 1;
  }
  Hz6RouteResult block_slot0_route =
      hz6_allocator_route_lookup(&midpage_block_allocator, block_slot0);
  Hz6RouteResult block_slot1_route =
      hz6_allocator_route_lookup(&midpage_block_allocator, block_slot1);
  Hz6RouteResult block_slot2_route =
      hz6_allocator_route_lookup(&midpage_block_allocator, block_slot2);
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
      !expect(hz6_allocator_route_lookup(
                  &midpage_block_allocator,
                  g_source_block_storage + (3 * HZ6_MIDPAGE_8K_BYTES)).kind ==
                  HZ6_ROUTE_INVALID,
              "block unregistered slot invalid")) {
    return 1;
  }
  hz6_allocator_route_unregister_exact(&midpage_block_allocator, block_slot2);
  if (!expect(hz6_allocator_release_descriptor_source(
                  &midpage_block_allocator, block_slot2_descriptor),
              "source block extra slot release") ||
      !expect(hz6_source_block_ref_count(block) == 2,
              "source block extra slot release decremented") ||
      !expect(g_source_block_release_count == 0,
              "source block extra slot release keeps source alive")) {
    return 1;
  }
  hz6_allocator_route_unregister_exact(&midpage_block_allocator, block_slot0);
  if (!expect(hz6_allocator_release_descriptor_source(
                  &midpage_block_allocator, block_slot0_descriptor),
              "source block first release") ||
      !expect(hz6_source_block_active(block),
              "source block retained after first release") ||
      !expect(hz6_source_block_ref_count(block) == 1,
              "source block decremented") ||
      !expect(g_source_block_release_count == 0,
              "source block not released early")) {
    return 1;
  }
  hz6_allocator_route_unregister_exact(&midpage_block_allocator, block_slot1);
  if (!expect(hz6_allocator_release_descriptor_source(
                  &midpage_block_allocator, block_slot1_descriptor),
              "source block final release") ||
      !expect(!hz6_source_block_active(block),
              "source block inactive after final release") ||
      !expect(g_source_block_release_count == 1,
              "source block released once") ||
      !expect(hz6_allocator_route_lookup(
                  &midpage_block_allocator,
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
      !expect(smoke_source_alloc_count(&midpage_run_allocator) == 1,
              "midpage run source allocation count")) {
    return 1;
  }
  void* run_slots[8];
  Hz6SourceBlock* run_block = NULL;
  for (size_t i = 0; i < run_prefilled; ++i) {
    run_slots[i] = hz6_malloc(&midpage_run_allocator, 6000);
    Hz6RouteResult run_route =
        hz6_allocator_route_lookup(&midpage_run_allocator, run_slots[i]);
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
  if (!expect(smoke_source_alloc_count(&midpage_run_allocator) == 1,
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
      !expect(smoke_source_alloc_count(&midpage_32k_run_allocator) == 1,
              "midpage 32k run source allocation count")) {
    return 1;
  }
  void* run32_slots[2];
  Hz6SourceBlock* run32_block = NULL;
  for (size_t i = 0; i < run32_prefilled; ++i) {
    run32_slots[i] = hz6_malloc(&midpage_32k_run_allocator, 20000);
    Hz6RouteResult run32_route =
        hz6_allocator_route_lookup(&midpage_32k_run_allocator, run32_slots[i]);
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
  if (!expect(smoke_source_alloc_count(&midpage_32k_run_allocator) == 1,
              "midpage 32k run prefill avoids refill")) {
    return 1;
  }
  for (size_t i = 0; i < run32_prefilled; ++i) {
    hz6_free(&midpage_32k_run_allocator, run32_slots[i]);
  }
  hz6_allocator_destroy(&midpage_32k_run_allocator);

  printf("hz6-r1-sourceblock-smoke ok\n");
  return 0;
}
