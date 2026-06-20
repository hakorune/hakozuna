#include "hz6_allocator.h"

#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
static uint32_t hz6_origin_phase_load_u32(const _Atomic uint32_t* value) {
  return atomic_load_explicit(value, memory_order_relaxed);
}

static void hz6_origin_phase_inc_u32(_Atomic uint32_t* value) {
  atomic_fetch_add_explicit(value, 1u, memory_order_relaxed);
}

static int hz6_origin_phase_dec_u32(_Atomic uint32_t* value) {
  uint32_t current = hz6_origin_phase_load_u32(value);
  while (current != 0) {
    if (atomic_compare_exchange_weak_explicit(
            value, &current, current - 1u, memory_order_relaxed,
            memory_order_relaxed)) {
      return 1;
    }
  }
  return 0;
}

static size_t hz6_origin_phase_producer_bucket(Hz6OwnerToken token) {
  return (size_t)((token.slot ^ token.generation) & 15u);
}

static void hz6_origin_phase_note_age(Hz6Allocator* allocator,
                                      uint32_t age,
                                      size_t* bucket0) {
  if (!allocator || !bucket0) {
    return;
  }
  if (age == 0) {
    ++bucket0[0];
  } else if (age == 1) {
    ++bucket0[1];
  } else if (age <= 3) {
    ++bucket0[2];
  } else if (age <= 7) {
    ++bucket0[3];
  } else if (age <= 15) {
    ++bucket0[4];
  } else if (age <= 63) {
    ++bucket0[5];
  } else {
    ++bucket0[6];
  }
}

static uint32_t hz6_origin_phase_requested_age(
    const Hz6Allocator* allocator,
    uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  uint32_t global_epoch =
      hz6_origin_phase_load_u32(&allocator->transfer_phase_demand_epoch);
  uint32_t class_epoch = hz6_origin_phase_load_u32(
      &allocator->transfer_phase_class_demand_epoch[class_id]);
  return global_epoch - class_epoch;
}

static void hz6_origin_phase_note_requested_age(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t* bucket0) {
  hz6_origin_phase_note_age(
      allocator, hz6_origin_phase_requested_age(allocator, class_id),
      bucket0);
}

static void hz6_origin_phase_note_kind_occupancy(
    Hz6Allocator* observer,
    const Hz6Allocator* owner,
    int for_empty) {
  if (!observer || !owner) {
    return;
  }
  size_t destination = hz6_origin_phase_load_u32(
      &owner->transfer_phase_destination_occupancy);
  size_t origin_fallback = hz6_origin_phase_load_u32(
      &owner->transfer_phase_origin_fallback_occupancy);
  if (for_empty) {
    observer->stats.origin_phase_audit_empty_destination_occupancy_total +=
        destination;
    observer->stats
        .origin_phase_audit_empty_origin_fallback_occupancy_total +=
        origin_fallback;
    if (destination >
        observer->stats.origin_phase_audit_empty_destination_occupancy_max) {
      observer->stats.origin_phase_audit_empty_destination_occupancy_max =
          destination;
    }
    if (origin_fallback > observer->stats
                              .origin_phase_audit_empty_origin_fallback_occupancy_max) {
      observer->stats
          .origin_phase_audit_empty_origin_fallback_occupancy_max =
          origin_fallback;
    }
  } else {
    observer->stats.origin_phase_audit_full_destination_occupancy_total +=
        destination;
    observer->stats
        .origin_phase_audit_full_origin_fallback_occupancy_total +=
        origin_fallback;
    if (destination >
        observer->stats.origin_phase_audit_full_destination_occupancy_max) {
      observer->stats.origin_phase_audit_full_destination_occupancy_max =
          destination;
    }
    if (origin_fallback > observer->stats
                              .origin_phase_audit_full_origin_fallback_occupancy_max) {
      observer->stats.origin_phase_audit_full_origin_fallback_occupancy_max =
          origin_fallback;
    }
  }
}

static void hz6_origin_phase_note_resident_split(
    Hz6Allocator* observer,
    const Hz6Allocator* owner,
    uint16_t requested_class_id,
    int for_empty) {
  if (!observer || !owner ||
      requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }
  uint32_t global_epoch =
      hz6_origin_phase_load_u32(&owner->transfer_phase_demand_epoch);
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    uint32_t destination = hz6_origin_phase_load_u32(
        &owner->transfer_phase_destination_class_occupancy[i]);
    uint32_t origin_fallback = hz6_origin_phase_load_u32(
        &owner->transfer_phase_origin_fallback_class_occupancy[i]);
    uint32_t total = destination + origin_fallback;
    if (total == 0) {
      continue;
    }
    if (i == requested_class_id) {
      if (for_empty) {
        observer->stats
            .origin_phase_audit_empty_same_class_destination_total +=
            destination;
        observer->stats
            .origin_phase_audit_empty_same_class_origin_fallback_total +=
            origin_fallback;
      } else {
        observer->stats
            .origin_phase_audit_full_same_class_destination_total +=
            destination;
        observer->stats
            .origin_phase_audit_full_same_class_origin_fallback_total +=
            origin_fallback;
      }
      continue;
    }
    uint32_t resident_epoch = hz6_origin_phase_load_u32(
        &owner->transfer_phase_class_demand_epoch[i]);
    uint32_t resident_age = global_epoch - resident_epoch;
    if (for_empty) {
      observer->stats
          .origin_phase_audit_empty_cross_class_destination_total +=
          destination;
      observer->stats
          .origin_phase_audit_empty_cross_class_origin_fallback_total +=
          origin_fallback;
      if (resident_age == 0) {
        observer->stats.origin_phase_audit_empty_cross_resident_fresh_total +=
            total;
      } else if (resident_age <= 15) {
        observer->stats.origin_phase_audit_empty_cross_resident_recent_total +=
            total;
      } else {
        observer->stats.origin_phase_audit_empty_cross_resident_stale_total +=
            total;
      }
    } else {
      observer->stats.origin_phase_audit_full_cross_class_destination_total +=
          destination;
      observer->stats
          .origin_phase_audit_full_cross_class_origin_fallback_total +=
          origin_fallback;
      if (resident_age == 0) {
        observer->stats.origin_phase_audit_full_cross_resident_fresh_total +=
            total;
      } else if (resident_age <= 15) {
        observer->stats.origin_phase_audit_full_cross_resident_recent_total +=
            total;
      } else {
        observer->stats.origin_phase_audit_full_cross_resident_stale_total +=
            total;
      }
    }
  }
}

static void hz6_origin_phase_note_capacity_bounds(
    Hz6Allocator* observer,
    const Hz6Allocator* owner) {
  if (!observer || !owner) {
    return;
  }
  size_t capacity = hz6_allocator_transfer_capacity(owner);
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    size_t count = hz6_origin_phase_load_u32(
        &owner->transfer_phase_class_occupancy[i]);
    if (count > capacity) {
      ++observer->stats.origin_phase_audit_class_count_over_capacity;
    }
  }
}

static void hz6_origin_phase_note_producer_snapshot(
    Hz6Allocator* observer,
    const Hz6Allocator* owner) {
  if (!observer || !owner) {
    return;
  }
  size_t largest = 0;
  size_t distinct = 0;
  for (size_t i = 0; i < 16; ++i) {
    size_t count = hz6_origin_phase_load_u32(
        &owner->transfer_phase_producer_bucket_occupancy[i]);
    if (count != 0) {
      ++distinct;
    }
    if (count > largest) {
      largest = count;
    }
  }
  if (largest >
      observer->stats.origin_phase_audit_full_largest_producer_bucket_max) {
    observer->stats.origin_phase_audit_full_largest_producer_bucket_max =
        largest;
  }
  if (distinct >
      observer->stats.origin_phase_audit_full_distinct_producer_bucket_max) {
    observer->stats.origin_phase_audit_full_distinct_producer_bucket_max =
        distinct;
  }
}
#endif

void hz6_allocator_origin_transfer_phase_audit_init(
    Hz6Allocator* allocator) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator) {
    return;
  }
  atomic_store_explicit(&allocator->transfer_phase_demand_epoch, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&allocator->transfer_phase_destination_occupancy, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&allocator->transfer_phase_origin_fallback_occupancy,
                        0, memory_order_relaxed);
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    atomic_store_explicit(&allocator->transfer_phase_class_demand_epoch[i],
                          0, memory_order_relaxed);
    atomic_store_explicit(&allocator->transfer_phase_class_occupancy[i], 0,
                          memory_order_relaxed);
    atomic_store_explicit(
        &allocator->transfer_phase_destination_class_occupancy[i], 0,
        memory_order_relaxed);
    atomic_store_explicit(
        &allocator->transfer_phase_origin_fallback_class_occupancy[i], 0,
        memory_order_relaxed);
  }
  for (size_t i = 0; i < 16; ++i) {
    atomic_store_explicit(&allocator->transfer_phase_producer_bucket_occupancy[i],
                          0, memory_order_relaxed);
  }
#else
  (void)allocator;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_note_demand(
    Hz6Allocator* allocator,
    uint16_t class_id) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (allocator) {
      ++allocator->stats.origin_phase_audit_invalid_class;
    }
    return;
  }
  uint32_t epoch = atomic_fetch_add_explicit(
                       &allocator->transfer_phase_demand_epoch, 1u,
                       memory_order_relaxed) +
                   1u;
  atomic_store_explicit(&allocator->transfer_phase_class_demand_epoch[class_id],
                        epoch, memory_order_relaxed);
#else
  (void)allocator;
  (void)class_id;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_stamp(
    Hz6Allocator* allocator,
    Hz6TransferObject* object,
    Hz6TransferPublishKind publish_kind,
    Hz6OwnerToken producer_token) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !object ||
      object->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (allocator) {
      ++allocator->stats.origin_phase_audit_invalid_class;
    }
    return;
  }
  if (object->audit_tag.stamped &&
      (object->audit_tag.publish_kind == HZ6_TRANSFER_PUBLISH_DESTINATION ||
       object->audit_tag.publish_kind ==
           HZ6_TRANSFER_PUBLISH_ORIGIN_FALLBACK)) {
    return;
  }
  object->audit_tag.publish_epoch =
      hz6_origin_phase_load_u32(&allocator->transfer_phase_demand_epoch);
  object->audit_tag.class_demand_epoch_at_publish =
      hz6_origin_phase_load_u32(
          &allocator->transfer_phase_class_demand_epoch[object->class_id]);
  object->audit_tag.producer_token = producer_token;
  object->audit_tag.publish_kind = (uint8_t)publish_kind;
  object->audit_tag.stamped = 1;
  object->audit_tag.accounted = 0;
#else
  (void)allocator;
  (void)object;
  (void)publish_kind;
  (void)producer_token;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_note_commit(
    Hz6Allocator* allocator,
    const Hz6TransferObject* object) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !object ||
      object->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (allocator) {
      ++allocator->stats.origin_phase_audit_invalid_class;
    }
    return;
  }
  if (!object->audit_tag.stamped) {
    ++allocator->stats.origin_phase_audit_commit_without_stamp;
    return;
  }
  ((Hz6TransferObject*)object)->audit_tag.accounted = 1;
  hz6_origin_phase_inc_u32(
      &allocator->transfer_phase_class_occupancy[object->class_id]);
  size_t bucket =
      hz6_origin_phase_producer_bucket(object->audit_tag.producer_token);
  hz6_origin_phase_inc_u32(
      &allocator->transfer_phase_producer_bucket_occupancy[bucket]);
  if (object->audit_tag.publish_kind == HZ6_TRANSFER_PUBLISH_DESTINATION) {
    ++allocator->stats.origin_phase_audit_destination_publish;
    hz6_origin_phase_inc_u32(
        &allocator->transfer_phase_destination_occupancy);
    hz6_origin_phase_inc_u32(
        &allocator->transfer_phase_destination_class_occupancy
             [object->class_id]);
  } else if (object->audit_tag.publish_kind ==
             HZ6_TRANSFER_PUBLISH_ORIGIN_FALLBACK) {
    ++allocator->stats.origin_phase_audit_origin_fallback_publish;
    hz6_origin_phase_inc_u32(
        &allocator->transfer_phase_origin_fallback_occupancy);
    hz6_origin_phase_inc_u32(
        &allocator->transfer_phase_origin_fallback_class_occupancy
             [object->class_id]);
  } else {
    ++allocator->stats.origin_phase_audit_unknown_publish_kind;
  }
#else
  (void)allocator;
  (void)object;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_note_pop(
    Hz6Allocator* allocator,
    const Hz6TransferObject* object) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !object ||
      object->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (allocator) {
      ++allocator->stats.origin_phase_audit_invalid_class;
    }
    return;
  }
  if (!object->audit_tag.stamped) {
    ++allocator->stats.origin_phase_audit_pop_without_stamp;
    return;
  }
  if (!object->audit_tag.accounted) {
    ++allocator->stats.origin_phase_audit_class_count_underflow;
    return;
  }
  Hz6ObjectDescriptor* descriptor =
      (Hz6ObjectDescriptor*)object->descriptor;
  if (!descriptor || descriptor->generation != object->generation) {
    ++allocator->stats.origin_phase_audit_generation_mismatch;
  }
  if (!hz6_origin_phase_dec_u32(
          &allocator->transfer_phase_class_occupancy[object->class_id])) {
    /* Aggregate occupancy is a diagnostic snapshot under concurrent transfer
     * mutation.  The object-level accounted bit above is the zero-gate. */
  }
  size_t bucket =
      hz6_origin_phase_producer_bucket(object->audit_tag.producer_token);
  if (!hz6_origin_phase_dec_u32(
          &allocator->transfer_phase_producer_bucket_occupancy[bucket])) {
    ++allocator->stats.origin_phase_audit_producer_count_underflow;
  }
  if (object->audit_tag.publish_kind == HZ6_TRANSFER_PUBLISH_DESTINATION) {
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_destination_occupancy);
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_destination_class_occupancy
             [object->class_id]);
  } else if (object->audit_tag.publish_kind ==
             HZ6_TRANSFER_PUBLISH_ORIGIN_FALLBACK) {
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_origin_fallback_occupancy);
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_origin_fallback_class_occupancy
             [object->class_id]);
  } else {
    ++allocator->stats.origin_phase_audit_unknown_publish_kind;
  }
  uint32_t global_epoch =
      hz6_origin_phase_load_u32(&allocator->transfer_phase_demand_epoch);
  uint32_t class_epoch = hz6_origin_phase_load_u32(
      &allocator->transfer_phase_class_demand_epoch[object->class_id]);
  hz6_origin_phase_note_age(
      allocator, global_epoch - object->audit_tag.publish_epoch,
      &allocator->stats.origin_phase_audit_pop_global_age_0);
  hz6_origin_phase_note_age(
      allocator,
      class_epoch - object->audit_tag.class_demand_epoch_at_publish,
      &allocator->stats.origin_phase_audit_pop_class_age_0);
#else
  (void)allocator;
  (void)object;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_note_commit_cancel(
    Hz6Allocator* allocator,
    const Hz6TransferObject* object) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !object ||
      object->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !object->audit_tag.stamped) {
    return;
  }
  hz6_origin_phase_dec_u32(
      &allocator->transfer_phase_class_occupancy[object->class_id]);
  size_t bucket =
      hz6_origin_phase_producer_bucket(object->audit_tag.producer_token);
  hz6_origin_phase_dec_u32(
      &allocator->transfer_phase_producer_bucket_occupancy[bucket]);
  if (object->audit_tag.publish_kind == HZ6_TRANSFER_PUBLISH_DESTINATION) {
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_destination_occupancy);
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_destination_class_occupancy
             [object->class_id]);
  } else if (object->audit_tag.publish_kind ==
             HZ6_TRANSFER_PUBLISH_ORIGIN_FALLBACK) {
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_origin_fallback_occupancy);
    hz6_origin_phase_dec_u32(
        &allocator->transfer_phase_origin_fallback_class_occupancy
             [object->class_id]);
  }
#else
  (void)allocator;
  (void)object;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_note_origin_full(
    Hz6Allocator* observer,
    const Hz6Allocator* origin,
    uint16_t requested_class_id) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!observer || !origin ||
      requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (observer) {
      ++observer->stats.origin_phase_audit_invalid_class;
    }
    return;
  }
  hz6_origin_phase_note_requested_age(
      observer, requested_class_id,
      &observer->stats.origin_phase_audit_full_requested_age_0);
  hz6_origin_phase_note_kind_occupancy(observer, origin, 0);
  hz6_origin_phase_note_resident_split(observer, origin, requested_class_id,
                                       0);
  hz6_origin_phase_note_producer_snapshot(observer, origin);
  hz6_origin_phase_note_capacity_bounds(observer, origin);
#else
  (void)observer;
  (void)origin;
  (void)requested_class_id;
#endif
}

void hz6_allocator_origin_transfer_phase_audit_note_pop_empty(
    Hz6Allocator* allocator,
    uint16_t requested_class_id) {
#if HZ6_ORIGIN_TRANSFER_PHASE_AGE_AUDIT_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (allocator) {
      ++allocator->stats.origin_phase_audit_invalid_class;
    }
    return;
  }
  hz6_origin_phase_note_requested_age(
      allocator, requested_class_id,
      &allocator->stats.origin_phase_audit_empty_requested_age_0);
  hz6_origin_phase_note_kind_occupancy(allocator, allocator, 1);
  hz6_origin_phase_note_resident_split(allocator, allocator,
                                       requested_class_id, 1);
  hz6_origin_phase_note_capacity_bounds(allocator, allocator);
#else
  (void)allocator;
  (void)requested_class_id;
#endif
}
