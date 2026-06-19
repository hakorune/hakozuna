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

#endif
