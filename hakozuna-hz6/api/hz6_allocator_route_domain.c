#include "hz6_allocator_route_domain.h"

#include <stdatomic.h>

static void hz6_allocator_route_domain_spin_pause(void) {
#if HZ6_ROUTE_DOMAIN_SPIN_PAUSE_L1 && \
    (defined(__x86_64__) || defined(__i386__))
  __asm__ __volatile__("pause");
#endif
}

static void hz6_allocator_route_domain_note_wait(
    const Hz6Allocator* allocator,
    int read_lock,
    size_t spins) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || spins == 0) {
    return;
  }
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
  if (read_lock) {
    ++mutable_allocator->stats.route_lock_read_contended;
  } else {
    ++mutable_allocator->stats.route_lock_write_contended;
  }
  if (spins > mutable_allocator->stats.route_lock_max_wait) {
    mutable_allocator->stats.route_lock_max_wait = spins;
  }
#else
  (void)allocator;
  (void)read_lock;
  (void)spins;
#endif
}

void hz6_allocator_route_domain_init(Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
  atomic_flag_clear_explicit(&allocator->route_domain_lock,
                             memory_order_release);
#if HZ6_ROUTE_DOMAIN_RWLOCK_L1
  atomic_flag_clear_explicit(&allocator->route_domain_writer,
                             memory_order_release);
  atomic_store_explicit(&allocator->route_domain_readers,
                        0u,
                        memory_order_release);
#endif
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
#if HZ6_ROUTE_DOMAIN_RWLOCK_L1
  atomic_flag* writer = &((Hz6Allocator*)allocator)->route_domain_writer;
  size_t spins = 0;
  while (atomic_flag_test_and_set_explicit(writer, memory_order_acquire)) {
    ++spins;
    hz6_allocator_route_domain_spin_pause();
  }
  while (atomic_load_explicit(
             &((Hz6Allocator*)allocator)->route_domain_readers,
             memory_order_acquire) != 0u) {
    ++spins;
    hz6_allocator_route_domain_spin_pause();
  }
  hz6_allocator_route_domain_note_wait(allocator, 0, spins);
#else
  atomic_flag* lock = &((Hz6Allocator*)allocator)->route_domain_lock;
  size_t spins = 0;
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
    ++spins;
    hz6_allocator_route_domain_spin_pause();
  }
  hz6_allocator_route_domain_note_wait(allocator, 0, spins);
#endif
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_unlock(const Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
#if HZ6_ROUTE_DOMAIN_RWLOCK_L1
  atomic_flag_clear_explicit(&((Hz6Allocator*)allocator)->route_domain_writer,
                             memory_order_release);
#else
  atomic_flag_clear_explicit(&((Hz6Allocator*)allocator)->route_domain_lock,
                             memory_order_release);
#endif
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_read_lock(const Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
#if HZ6_ROUTE_DOMAIN_RWLOCK_L1
  for (;;) {
    size_t spins = 0;
    while (atomic_flag_test_and_set_explicit(
        &((Hz6Allocator*)allocator)->route_domain_writer,
        memory_order_acquire)) {
      ++spins;
      hz6_allocator_route_domain_spin_pause();
    }
    atomic_fetch_add_explicit(
        &((Hz6Allocator*)allocator)->route_domain_readers, 1u,
        memory_order_acquire);
    atomic_flag_clear_explicit(
        &((Hz6Allocator*)allocator)->route_domain_writer,
        memory_order_release);
    hz6_allocator_route_domain_note_wait(allocator, 1, spins);
    return;
  }
#else
  hz6_allocator_route_domain_lock(allocator);
#endif
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_read_unlock(const Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1
  if (!allocator) {
    return;
  }
#if HZ6_ROUTE_DOMAIN_RWLOCK_L1
  atomic_fetch_sub_explicit(
      &((Hz6Allocator*)allocator)->route_domain_readers, 1u,
      memory_order_release);
#else
  hz6_allocator_route_domain_unlock(allocator);
#endif
#else
  (void)allocator;
#endif
}

void hz6_allocator_route_domain_note_compact_debt(Hz6Allocator* allocator) {
#if HZ6_ROUTE_DOMAIN_SYNC_L1 && HZ6_ROUTE_COMPACT_DEFER_REMOTE_L1
  if (!allocator) {
    return;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_compact_deferred;
#endif
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
