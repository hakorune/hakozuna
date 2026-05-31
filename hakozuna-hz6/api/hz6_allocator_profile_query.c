#include "hz6_allocator.h"

Hz6ProfileId hz6_allocator_profile_id(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.id : HZ6_PROFILE_STRICT;
}

int hz6_allocator_profile_transfer_first(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.transfer_first != 0 : 0;
}

int hz6_allocator_profile_strict_owner_remote(const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.strict_owner_remote != 0 : 0;
}

Hz6SourceKind hz6_allocator_profile_source_kind(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.source_kind : HZ6_SOURCE_NONE;
}

size_t hz6_allocator_profile_source_refill_batch(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  return allocator ? hz6_profile_source_refill_batch(&allocator->profile,
                                                     front_id, class_id)
                   : 0;
}

static size_t hz6_control_pressure_scale(size_t base, size_t pressure) {
  if (base == 0) {
    return 0;
  }
  if (pressure >= 3) {
    size_t scaled = base / 4;
    return scaled ? scaled : 1;
  }
  if (pressure >= 1) {
    size_t scaled = base / 2;
    return scaled ? scaled : 1;
  }
  return base;
}

size_t hz6_allocator_control_source_refill_batch(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator) {
    return 0;
  }

  size_t base = hz6_allocator_profile_source_refill_batch(allocator,
                                                          front_id,
                                                          class_id);
  size_t pressure = 0;
  size_t transfer_capacity = hz6_allocator_transfer_capacity(allocator);
  size_t transfer_count = hz6_allocator_transfer_count(allocator);
  size_t front_count = hz6_allocator_frontcache_count(allocator, class_id);
  size_t front_capacity = hz6_allocator_frontcache_capacity(allocator,
                                                           class_id);
  size_t source_alloc = allocator->stats.source_alloc;
  size_t transfer_push = allocator->stats.transfer_push;
  size_t transfer_pop = allocator->stats.transfer_pop;
  int starvation = 0;
  int saturation = 0;

  if (allocator->stats.alloc_fail > 0) {
    starvation = 1;
  }
  if (allocator->stats.route_miss > source_alloc + transfer_pop) {
    starvation = 1;
  }
  if (transfer_capacity > 0 && transfer_count * 2 >= transfer_capacity) {
    saturation = 1;
  }
  if (front_capacity > 0 && front_count * 2 >= front_capacity) {
    saturation = 1;
  }
  if (transfer_push > transfer_pop + 128) {
    saturation = 1;
  }

#if HZ6_DIAGNOSTIC_PROBES
  Hz6Allocator* diag_allocator = (Hz6Allocator*)allocator;
  if (starvation) {
    ++diag_allocator->stats.source_refill_starvation;
  }
  if (saturation) {
    ++diag_allocator->stats.source_refill_saturation;
  }
  if (starvation && !saturation) {
    ++diag_allocator->stats.source_admission_boosted;
  } else if (saturation) {
    ++diag_allocator->stats.source_admission_clamped;
  } else {
    ++diag_allocator->stats.source_admission_open;
  }
#endif

  if (starvation && !saturation) {
    size_t boosted = base < 16 ? 16 : base * 2;
    if (boosted < base) {
      boosted = base;
    }
#if HZ6_DIAGNOSTIC_PROBES
    if (boosted > base) {
      ++diag_allocator->stats.source_refill_boost;
    }
#endif
    return boosted;
  }

  if (saturation) {
    pressure += 2;
  }

#if HZ6_DIAGNOSTIC_PROBES
  if (pressure > 0) {
    ++diag_allocator->stats.source_refill_clamp;
  }
#endif

  return hz6_control_pressure_scale(base, pressure);
}

size_t hz6_allocator_control_source_prefill_count(
    const Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t requested) {
  if (!allocator || requested == 0) {
    return 0;
  }

  size_t refill_batch = hz6_allocator_control_source_refill_batch(
      allocator, front_id, class_id);
  if (refill_batch == 0) {
    return 0;
  }
  return requested < refill_batch ? requested : refill_batch;
}

size_t hz6_allocator_profile_transfer_capacity(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->profile.transfer_capacity : 0;
}
