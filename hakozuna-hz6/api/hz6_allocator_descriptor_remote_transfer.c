#include "hz6_allocator.h"

#if HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_L1
static void hz6_allocator_remote_free_requeue_transfer(
    Hz6Allocator* allocator,
    Hz6TransferObject object) {
  if (!hz6_allocator_transfer_push(allocator, object)) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_free_backpressure_requeue_fail;
#endif
    return;
  }
  hz6_allocator_note_transfer_push(allocator);
}

static int hz6_allocator_remote_free_drain_transfer_one(
    Hz6Allocator* allocator,
    uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  size_t front_count = hz6_allocator_frontcache_count(allocator, class_id);
  if (front_count >= hz6_allocator_frontcache_capacity(allocator, class_id)) {
    return 0;
  }
  if (front_count >
      HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_MAX_FRONTCACHE_COUNT) {
    return 0;
  }

#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.remote_free_backpressure_drain_attempt;
#endif

  Hz6TransferObject transfer = {0};
  if (!hz6_allocator_transfer_pop(allocator, class_id, &transfer)) {
    return 0;
  }
  hz6_allocator_note_transfer_pop(allocator);

  Hz6ObjectDescriptor* drained =
      (Hz6ObjectDescriptor*)transfer.descriptor;
  int valid = drained && transfer.ptr && drained->ptr == transfer.ptr &&
              drained->generation == transfer.generation &&
              drained->class_id == class_id &&
              drained->state == HZ6_STATE_TRANSFER_FREE;
  if (valid) {
    Hz6RouteResult route =
        hz6_allocator_route_lookup_exact(allocator, transfer.ptr);
    valid = route.kind == HZ6_ROUTE_VALID && route.descriptor == drained &&
            route.generation == transfer.generation &&
            route.class_id == class_id;
  }
  if (!valid) {
    hz6_allocator_remote_free_requeue_transfer(allocator, transfer);
    return 0;
  }

  drained->state = HZ6_STATE_LOCAL_FREE;
  hz6_allocator_set_descriptor_owner(allocator, drained,
                                     allocator->owner.token);

  Hz6FrontCacheEntry entry = {0};
  entry.ptr = transfer.ptr;
  entry.descriptor = drained;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  entry.source_block = drained->source_block;
#endif
  entry.generation = transfer.generation;
  hz6_frontcache_entry_set_bytes(&entry, drained->bytes);
  hz6_frontcache_entry_set_class_id(&entry, drained->class_id);

  if (!hz6_allocator_frontcache_push(allocator, class_id, entry)) {
    drained->state = HZ6_STATE_TRANSFER_FREE;
    hz6_allocator_set_descriptor_owner(allocator, drained,
                                       (Hz6OwnerToken){0});
    hz6_allocator_remote_free_requeue_transfer(allocator, transfer);
    return 0;
  }

#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.remote_free_backpressure_drain_success;
#endif
  return 1;
}

static int hz6_allocator_remote_free_should_try_backpressure_drain(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }
#if HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_STRIDE <= 1
  return 1;
#else
  return allocator->stats.transfer_reserve_attempt %
             HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_STRIDE ==
         0;
#endif
}
#endif

#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
static void hz6_allocator_note_transfer_reserve_full(
    Hz6Allocator* allocator,
    uint16_t class_id) {
  if (!allocator) {
    return;
  }
  ++allocator->stats.transfer_reserve_full;
  ++allocator->stats.remote_free_backpressure;

  size_t transfer_count = hz6_allocator_transfer_count(allocator);
  size_t class_count = hz6_allocator_transfer_count_class(allocator,
                                                          class_id);
  allocator->stats.transfer_reserve_full_transfer_count_total +=
      transfer_count;
  allocator->stats.transfer_reserve_full_class_count_total += class_count;
  if (class_count >
      allocator->stats.transfer_reserve_full_class_count_max) {
    allocator->stats.transfer_reserve_full_class_count_max = class_count;
  }
}
#endif

int hz6_allocator_remote_free_active_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr) {
  if (!allocator || !descriptor || !ptr ||
      descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  ++allocator->stats.remote_free_attempt;
  if (hz6_allocator_profile_strict_owner_remote(allocator)) {
    if (!hz6_allocator_descriptor_owner_equal_at(
            allocator, descriptor, allocator->owner.token,
            HZ6_OWNER_EQUAL_SITE_REMOTE_FREE)) {
      ++allocator->stats.remote_free_strict_owner_block;
      return 0;
    }
    descriptor->state = HZ6_STATE_REMOTE_PENDING;
    return 1;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;

#if HZ6_REMOTE_FREE_COMMIT_L1
  Hz6TransferReservation reservation = {0};
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_attempt;
#endif
  int reserve_ok = hz6_allocator_transfer_reserve(allocator, object.class_id,
                                                  &reservation);
#if HZ6_REMOTE_FREE_BACKPRESSURE_DRAIN_L1
  if (!reserve_ok &&
      hz6_allocator_remote_free_should_try_backpressure_drain(allocator) &&
      hz6_allocator_remote_free_drain_transfer_one(allocator,
                                                  object.class_id)) {
    reserve_ok = hz6_allocator_transfer_reserve(allocator, object.class_id,
                                                &reservation);
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    if (reserve_ok) {
      ++allocator->stats.remote_free_backpressure_retry_success;
    }
#endif
  }
#endif
  if (!reserve_ok) {
    ++allocator->stats.remote_free_transfer_fail;
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    hz6_allocator_note_transfer_reserve_full(allocator, object.class_id);
#endif
    return 0;
  }

  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     (Hz6OwnerToken){0});
  hz6_allocator_transfer_commit(&reservation, object);
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_success;
#endif
  hz6_allocator_note_transfer_push(allocator);
  return 1;
#else
  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                     (Hz6OwnerToken){0});
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_attempt;
#endif
  if (!hz6_allocator_transfer_push(allocator, object)) {
    ++allocator->stats.remote_free_transfer_fail;
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    hz6_allocator_note_transfer_reserve_full(allocator, object.class_id);
    ++allocator->stats.transfer_reserve_full_after_state_mutation;
#endif
    descriptor->state = HZ6_STATE_ACTIVE;
    hz6_allocator_set_descriptor_owner(allocator, descriptor,
                                       allocator->owner.token);
    return 0;
  }

#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.transfer_reserve_success;
#endif
  hz6_allocator_note_transfer_push(allocator);
  return 1;
#endif
}
