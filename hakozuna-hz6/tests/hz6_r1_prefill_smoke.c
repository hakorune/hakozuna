#include "../api/hz6_allocator.h"
#include "../fronts/large/hz6_large128_front.h"
#include "../fronts/hz6_front.h"
#include "../fronts/hz6_front_source.h"
#include "../fronts/hz6_front_source_prefill.h"
#include "../fronts/local2p/hz6_local2p_front.h"
#include "../fronts/midpage/hz6_midpage_front.h"
#include "../include/hz6_contract.h"

#include <stdio.h>

static int expect(int condition, const char* label) {
  if (!condition) {
    fprintf(stderr, "hz6-r1-prefill-smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

static size_t smoke_profile_refill_batch(const Hz6Allocator* allocator) {
  return hz6_allocator_profile_source_refill_batch(allocator, 0, 0);
}

static size_t smoke_source_alloc_count(const Hz6Allocator* allocator) {
  return hz6_stats_snapshot(allocator).source_alloc;
}

int main(void) {
  Hz6Allocator rss_prefill_allocator;
  hz6_allocator_init_with_profile(&rss_prefill_allocator, HZ6_PROFILE_RSS);
  size_t rss_refill_batch = smoke_profile_refill_batch(
      &rss_prefill_allocator);
  size_t prefilled = hz6_front_prefill_source_kind(
      &rss_prefill_allocator, HZ6_FRONT_MIDPAGE, HZ6_MIDPAGE_CLASS_ID,
      HZ6_MIDPAGE_BYTES, HZ6_SOURCE_OS_PAGED, rss_refill_batch);
  if (!expect(prefilled == rss_refill_batch,
              "rss profile source prefill count") ||
      !expect(smoke_source_alloc_count(&rss_prefill_allocator) == prefilled,
              "rss profile prefill source alloc")) {
    return 1;
  }
  for (size_t i = 0; i < prefilled; ++i) {
    void* prefetched = hz6_malloc(&rss_prefill_allocator, 16384);
    if (!expect(prefetched != NULL, "rss profile prefilled malloc")) {
      return 1;
    }
  }
  if (!expect(smoke_source_alloc_count(&rss_prefill_allocator) == prefilled,
              "rss profile prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&rss_prefill_allocator);

  Hz6Allocator large_prefill_allocator;
  hz6_allocator_init_with_profile(&large_prefill_allocator, HZ6_PROFILE_RSS);
  if (!expect(hz6_front_for_id(HZ6_FRONT_LARGE) != NULL,
              "large128 registry front")) {
    return 1;
  }
  size_t large_refill_batch = smoke_profile_refill_batch(
      &large_prefill_allocator);
  size_t large_prefilled = hz6_front_prefill_by_id(
      &large_prefill_allocator, HZ6_FRONT_LARGE, large_refill_batch);
  if (!expect(hz6_front_prefill_by_id(
                  &large_prefill_allocator, HZ6_FRONT_MIDPAGE,
                  large_refill_batch) == 0,
              "midpage registry prefill hook absent")) {
    return 1;
  }
  if (!expect(large_prefilled == large_refill_batch,
              "large128 profile prefill count") ||
      !expect(smoke_source_alloc_count(&large_prefill_allocator) ==
                  large_prefilled,
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
  if (!expect(smoke_source_alloc_count(&large_prefill_allocator) ==
                  large_prefilled,
              "large128 profile prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&large_prefill_allocator);

  Hz6Allocator local2p_prefill_allocator;
  hz6_allocator_init_with_profile(&local2p_prefill_allocator,
                                  HZ6_PROFILE_RSS);
  if (!expect(hz6_front_for_id(HZ6_FRONT_LOCAL2P) != NULL,
              "local2p registry front")) {
    return 1;
  }
  size_t local2p_refill_batch = smoke_profile_refill_batch(
      &local2p_prefill_allocator);
  size_t local2p_prefilled = hz6_front_prefill_by_id(
      &local2p_prefill_allocator, HZ6_FRONT_LOCAL2P, local2p_refill_batch);
  if (!expect(local2p_prefilled == local2p_refill_batch,
              "local2p profile prefill count") ||
      !expect(smoke_source_alloc_count(&local2p_prefill_allocator) ==
                  local2p_prefilled,
              "local2p profile prefill source alloc")) {
    return 1;
  }
  for (size_t i = 0; i < local2p_prefilled; ++i) {
    void* prefetched_local2p =
        hz6_malloc(&local2p_prefill_allocator, HZ6_LOCAL2P_BYTES);
    if (!expect(prefetched_local2p != NULL,
                "local2p profile prefilled malloc")) {
      return 1;
    }
  }
  if (!expect(smoke_source_alloc_count(&local2p_prefill_allocator) ==
                  local2p_prefilled,
              "local2p profile prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&local2p_prefill_allocator);

  Hz6Allocator size_prefill_allocator;
  hz6_allocator_init_with_profile(&size_prefill_allocator, HZ6_PROFILE_RSS);
  size_t size_refill_batch = smoke_profile_refill_batch(
      &size_prefill_allocator);
  size_t size_large_prefilled = hz6_allocator_prefill_size(
      &size_prefill_allocator, 70000, size_refill_batch);
  size_t size_local2p_prefilled = hz6_allocator_prefill_size(
      &size_prefill_allocator, HZ6_LOCAL2P_BYTES, size_refill_batch);
  size_t size_midpage_prefilled = hz6_allocator_prefill_size(
      &size_prefill_allocator, 16384, size_refill_batch);
  if (!expect(size_large_prefilled == size_refill_batch,
              "size prefill large128 count") ||
      !expect(size_local2p_prefilled == size_refill_batch,
              "size prefill local2p count") ||
      !expect(size_midpage_prefilled == size_refill_batch,
              "size prefill midpage count") ||
      !expect(smoke_source_alloc_count(&size_prefill_allocator) ==
                  size_large_prefilled + size_local2p_prefilled +
                      (size_midpage_prefilled / 2),
              "size prefill source alloc")) {
    return 1;
  }
  for (size_t i = 0; i < size_large_prefilled; ++i) {
    void* prefetched_large = hz6_malloc(&size_prefill_allocator, 70000);
    if (!expect(prefetched_large != NULL,
                "size prefill large128 malloc")) {
      return 1;
    }
  }
  for (size_t i = 0; i < size_local2p_prefilled; ++i) {
    void* prefetched_local2p =
        hz6_malloc(&size_prefill_allocator, HZ6_LOCAL2P_BYTES);
    if (!expect(prefetched_local2p != NULL,
                "size prefill local2p malloc")) {
      return 1;
    }
  }
  for (size_t i = 0; i < size_midpage_prefilled; ++i) {
    void* prefetched_midpage = hz6_malloc(&size_prefill_allocator, 16384);
    if (!expect(prefetched_midpage != NULL,
                "size prefill midpage malloc")) {
      return 1;
    }
  }
  if (!expect(smoke_source_alloc_count(&size_prefill_allocator) ==
                  size_large_prefilled + size_local2p_prefilled +
                      (size_midpage_prefilled / 2),
              "size prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&size_prefill_allocator);

  Hz6Allocator front_prefill_allocator;
  hz6_allocator_init_with_profile(&front_prefill_allocator, HZ6_PROFILE_RSS);
  size_t front_refill_batch = smoke_profile_refill_batch(
      &front_prefill_allocator);
  size_t front_large_prefilled = hz6_allocator_prefill_front(
      &front_prefill_allocator, HZ6_FRONT_LARGE, front_refill_batch);
  size_t front_midpage8_prefilled = hz6_allocator_prefill_front_class(
      &front_prefill_allocator, HZ6_FRONT_MIDPAGE,
      HZ6_MIDPAGE_8K_CLASS_ID, 1);
  size_t front_midpage_bad_prefilled = hz6_allocator_prefill_front_class(
      &front_prefill_allocator, HZ6_FRONT_MIDPAGE, HZ6_LARGE128_CLASS_ID, 1);
  if (!expect(front_large_prefilled == front_refill_batch,
              "allocator front prefill large count") ||
      !expect(front_midpage8_prefilled == 8,
              "allocator front class prefill midpage8 count") ||
      !expect(front_midpage_bad_prefilled == 0,
              "allocator front class prefill rejects wrong class") ||
      !expect(smoke_source_alloc_count(&front_prefill_allocator) ==
                  front_large_prefilled + 1,
              "allocator front class prefill source alloc")) {
    return 1;
  }
  for (size_t i = 0; i < front_large_prefilled; ++i) {
    void* prefetched_large = hz6_malloc(&front_prefill_allocator, 70000);
    if (!expect(prefetched_large != NULL,
                "allocator front prefill large malloc")) {
      return 1;
    }
  }
  for (size_t i = 0; i < front_midpage8_prefilled; ++i) {
    void* prefetched_midpage = hz6_malloc(&front_prefill_allocator, 6000);
    if (!expect(prefetched_midpage != NULL,
                "allocator front class prefill midpage malloc")) {
      return 1;
    }
  }
  if (!expect(smoke_source_alloc_count(&front_prefill_allocator) ==
                  front_large_prefilled + 1,
              "allocator front class prefill avoids refill")) {
    return 1;
  }
  hz6_allocator_destroy(&front_prefill_allocator);

  printf("hz6-r1-prefill-smoke ok\n");
  return 0;
}
