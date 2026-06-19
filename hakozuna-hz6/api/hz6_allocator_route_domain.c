#include "hz6_allocator_route_domain.h"

#include <stdatomic.h>

void hz6_allocator_route_domain_init(Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
  atomic_flag_clear_explicit(&allocator->route_domain_lock,
                             memory_order_release);
  atomic_store_explicit(&allocator->route_compact_requested,
                        0u,
                        memory_order_release);
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_lock(const Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
  atomic_flag* lock = &((Hz6Allocator*)allocator)->route_domain_lock;
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
  }
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_unlock(const Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
  atomic_flag_clear_explicit(&((Hz6Allocator*)allocator)->route_domain_lock,
                             memory_order_release);
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_note_compact_debt(Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1 && HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1
  if (!allocator) {
    return;
  }
  atomic_store_explicit(&allocator->route_compact_requested,
                        1u,
                        memory_order_release);
#else
  (void)allocator;
#endif
}

int hz6_allocator_route_domain_consume_compact_debt(
    Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1 && HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1
  if (!allocator) {
    return 0;
  }
  return atomic_exchange_explicit(&allocator->route_compact_requested,
                                  0u,
                                  memory_order_acq_rel) != 0u;
#else
  (void)allocator;
  return 0;
#endif
}
