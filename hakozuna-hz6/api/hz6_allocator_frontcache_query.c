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

void hz6_allocator_note_frontcache_borrow_dryrun(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t requested_class_id,
    size_t requested_bytes) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      requested_bytes == 0) {
    return;
  }

  ++allocator->stats.frontcache_borrow_dryrun_calls;
  size_t candidate_total = 0;
  size_t largest_candidate_bin = 0;
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    if (i <= requested_class_id) {
      continue;
    }
    const Hz6FrontCacheBin* bin = &allocator->frontcache_bins[i];
    size_t bin_candidates = 0;
    for (size_t j = 0; j < bin->count; ++j) {
      Hz6FrontCacheEntry entry = bin->entries[j];
      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)entry.descriptor;
      if (!descriptor || descriptor->state != HZ6_STATE_LOCAL_FREE ||
          descriptor->ptr != entry.ptr || descriptor->bytes < requested_bytes ||
          descriptor->generation != entry.generation) {
        continue;
      }
      Hz6RouteResult route =
          hz6_allocator_route_lookup_exact(allocator, entry.ptr);
      if (route.kind != HZ6_ROUTE_VALID || route.front_id != front_id) {
        continue;
      }
      ++bin_candidates;
    }
    candidate_total += bin_candidates;
    if (bin_candidates > largest_candidate_bin) {
      largest_candidate_bin = bin_candidates;
    }
  }

  if (candidate_total != 0) {
    ++allocator->stats.frontcache_borrow_dryrun_candidate_calls;
    allocator->stats.frontcache_borrow_dryrun_candidate_total +=
        candidate_total;
  }
  if (largest_candidate_bin >
      allocator->stats.frontcache_borrow_dryrun_largest_candidate_max) {
    allocator->stats.frontcache_borrow_dryrun_largest_candidate_max =
        largest_candidate_bin;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)requested_class_id;
  (void)requested_bytes;
#endif
}

void hz6_allocator_note_frontcache_cap_dryrun(Hz6Allocator* allocator,
                                              uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }

  size_t capacity = allocator->frontcache_bins[class_id].capacity;
  if (capacity == 0) {
    return;
  }

  size_t soft_cap = capacity / 8;
  if (soft_cap < 4) {
    soft_cap = capacity < 4 ? capacity : 4;
  }
  if (soft_cap == 0) {
    soft_cap = 1;
  }

  size_t count = allocator->frontcache_bins[class_id].count;
  ++allocator->stats.frontcache_cap_dryrun_push;
  if (soft_cap > allocator->stats.frontcache_cap_dryrun_soft_cap_max) {
    allocator->stats.frontcache_cap_dryrun_soft_cap_max = soft_cap;
  }
  if (count > allocator->stats.frontcache_cap_dryrun_bin_count_max) {
    allocator->stats.frontcache_cap_dryrun_bin_count_max = count;
  }
  if (count >= soft_cap) {
    ++allocator->stats.frontcache_cap_dryrun_over_cap;
    ++allocator->stats.frontcache_cap_dryrun_would_release;
  }
#else
  (void)allocator;
  (void)class_id;
#endif
}

int hz6_allocator_frontcache_should_cap_release(Hz6Allocator* allocator,
                                                uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
#if !HZ6_FRONTCACHE_CAP_ON_FREE
  return 0;
#else
  size_t capacity = allocator->frontcache_bins[class_id].capacity;
  if (capacity == 0) {
    return 0;
  }
  size_t soft_cap = capacity / 8;
  if (soft_cap < 4) {
    soft_cap = capacity < 4 ? capacity : 4;
  }
  if (soft_cap == 0) {
    soft_cap = 1;
  }
  return allocator->frontcache_bins[class_id].count >= soft_cap;
#endif
}
