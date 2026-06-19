#ifndef HZ6_ALLOCATOR_REMOTE_FREE_STATUS_DISPATCH_H
#define HZ6_ALLOCATOR_REMOTE_FREE_STATUS_DISPATCH_H

#include "hz6_allocator.h"

static inline int hz6_remote_free_status_dispatch_transfer_eligible(
    Hz6RouteResult route) {
  return route.front_id == HZ6_FRONT_TOY ||
         route.front_id == HZ6_FRONT_MIDPAGE ||
         route.front_id == HZ6_FRONT_LOCAL2P;
}

static inline Hz6RemoteFreeCommitStatus
hz6_remote_free_status_dispatch_transfer(Hz6Allocator* allocator,
                                         void* ptr,
                                         Hz6RouteResult route,
                                         int* handled) {
#if HZ6_REMOTE_FREE_STATUS_DISPATCH_L1
  if (handled) {
    *handled = 0;
  }
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor ||
      !hz6_remote_free_status_dispatch_transfer_eligible(route)) {
    return HZ6_REMOTE_FREE_COMMIT_STATUS_STALE;
  }
  Hz6ObjectDescriptor* descriptor =
      (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->class_id != route.class_id || descriptor->ptr != ptr) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_free_status_integrity_failure;
#endif
    if (handled) {
      *handled = 1;
    }
    return HZ6_REMOTE_FREE_COMMIT_STATUS_INTEGRITY_FAILURE;
  }
  if (handled) {
    *handled = 1;
  }
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.remote_free_status_dispatch_transfer;
#endif
  return hz6_allocator_remote_free_active_descriptor_status(
      allocator, descriptor, ptr);
#else
  (void)allocator;
  (void)ptr;
  (void)route;
  if (handled) {
    *handled = 0;
  }
  return HZ6_REMOTE_FREE_COMMIT_STATUS_STALE;
#endif
}

static inline void hz6_remote_free_status_note_return(
    Hz6Allocator* allocator,
    Hz6RemoteFreeCommitStatus status) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator) {
    return;
  }
  switch (status) {
    case HZ6_REMOTE_FREE_COMMIT_STATUS_BACKPRESSURE:
      ++allocator->stats.remote_free_returned_backpressure;
      break;
    case HZ6_REMOTE_FREE_COMMIT_STATUS_INTEGRITY_FAILURE:
      ++allocator->stats.remote_free_returned_integrity_failure;
      ++allocator->stats.remote_free_returned_uncommitted;
      break;
    case HZ6_REMOTE_FREE_COMMIT_STATUS_STALE:
      ++allocator->stats.remote_free_returned_stale;
      ++allocator->stats.remote_free_returned_uncommitted;
      break;
    case HZ6_REMOTE_FREE_COMMIT_STATUS_COMMITTED:
    default:
      break;
  }
#else
  (void)allocator;
  (void)status;
#endif
}

static inline int hz6_remote_free_status_try_origin_transfer(
    Hz6Allocator* allocator,
    void* ptr,
    Hz6RouteResult route,
    Hz6RemoteFreeCommitStatus status) {
#if HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_L1
  if (!allocator || !ptr ||
      status != HZ6_REMOTE_FREE_COMMIT_STATUS_BACKPRESSURE ||
      route.kind != HZ6_ROUTE_VALID || !route.descriptor ||
      !route.route_allocator || route.route_allocator == allocator) {
    return 0;
  }
#if HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE > 1
  if (allocator->stats.transfer_reserve_attempt %
          HZ6_REMOTE_FREE_BACKPRESSURE_ORIGIN_TRANSFER_STRIDE !=
      0) {
    return 0;
  }
#endif
  Hz6Allocator* origin = route.route_allocator;
  Hz6ObjectDescriptor* descriptor =
      (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->ptr != ptr || descriptor->state != HZ6_STATE_ACTIVE ||
      descriptor->generation != route.generation ||
      descriptor->class_id != route.class_id ||
      !hz6_owner_is_alive(&origin->owner, origin->owner.token) ||
      !hz6_allocator_descriptor_owner_equal_at(
          origin, descriptor, origin->owner.token,
          HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_free_backpressure_origin_transfer_fail;
#endif
    return 0;
  }
  Hz6RemoteFreeCommitStatus origin_status =
      hz6_allocator_remote_free_active_descriptor_status(origin,
                                                        descriptor,
                                                        ptr);
  if (origin_status != HZ6_REMOTE_FREE_COMMIT_STATUS_COMMITTED) {
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_free_backpressure_origin_transfer_fail;
#endif
    return 0;
  }
#if HZ6_REMOTE_FREE_COMMIT_OBSERVE_L1 && HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.remote_free_backpressure_origin_transfer_success;
#endif
  return 1;
#else
  (void)allocator;
  (void)ptr;
  (void)route;
  (void)status;
  return 0;
#endif
}

#endif
