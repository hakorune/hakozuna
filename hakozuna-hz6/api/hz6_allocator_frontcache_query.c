#include "hz6_allocator.h"

size_t hz6_allocator_frontcache_count(const Hz6Allocator* allocator,
                                      uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->frontcache_bins[class_id].count;
}

size_t hz6_allocator_frontcache_capacity(const Hz6Allocator* allocator,
                                         uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->frontcache_bins[class_id].capacity;
}

void hz6_allocator_note_frontcache_spill_dryrun(
    Hz6Allocator* allocator,
    uint16_t requested_class_id) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }

  ++allocator->stats.frontcache_spill_dryrun_calls;
  if (allocator->frontcache_bins[requested_class_id].count == 0) {
    ++allocator->stats.frontcache_spill_dryrun_requested_empty;
  }

  size_t reclaimable = 0;
  size_t largest_donor = 0;
  size_t donor_bins = 0;
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    if (i == requested_class_id) {
      continue;
    }
    size_t count = allocator->frontcache_bins[i].count;
    if (count <= 1) {
      continue;
    }
    size_t donor = count - 1;
    reclaimable += donor;
    ++donor_bins;
    if (donor > largest_donor) {
      largest_donor = donor;
    }
  }

  if (reclaimable != 0) {
    ++allocator->stats.frontcache_spill_dryrun_candidate_calls;
    allocator->stats.frontcache_spill_dryrun_reclaimable_total += reclaimable;
  }
  if (largest_donor >
      allocator->stats.frontcache_spill_dryrun_largest_donor_max) {
    allocator->stats.frontcache_spill_dryrun_largest_donor_max = largest_donor;
  }
  if (donor_bins > allocator->stats.frontcache_spill_dryrun_donor_bins_max) {
    allocator->stats.frontcache_spill_dryrun_donor_bins_max = donor_bins;
  }
#else
  (void)allocator;
  (void)requested_class_id;
#endif
}
