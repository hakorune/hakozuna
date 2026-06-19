#include "hz6_allocator_route_shared_directory.h"

#include <stdatomic.h>

#if HZ6_SHARED_ROUTE_DIRECTORY_L1
static _Atomic(size_t) g_hz6_shared_route_directory_tombstone_current;
static _Atomic(size_t) g_hz6_shared_route_directory_tombstone_max;
static _Atomic(size_t) g_hz6_shared_route_directory_register_used_tombstone;
static _Atomic(size_t) g_hz6_shared_route_directory_maintenance_attempt;
static _Atomic(size_t) g_hz6_shared_route_directory_maintenance_success;
static _Atomic(size_t) g_hz6_shared_route_directory_maintenance_cleared;

void hz6_shared_route_directory_diag_tombstone_added(void) {
  size_t current = atomic_fetch_add_explicit(
                       &g_hz6_shared_route_directory_tombstone_current,
                       1u,
                       memory_order_relaxed) +
                   1u;
  size_t observed = atomic_load_explicit(
      &g_hz6_shared_route_directory_tombstone_max, memory_order_relaxed);
  while (current > observed &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz6_shared_route_directory_tombstone_max,
             &observed,
             current,
             memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

void hz6_shared_route_directory_diag_tombstone_reused(void) {
  atomic_fetch_add_explicit(
      &g_hz6_shared_route_directory_register_used_tombstone,
      1u,
      memory_order_relaxed);
  size_t current = atomic_load_explicit(
      &g_hz6_shared_route_directory_tombstone_current, memory_order_relaxed);
  while (current != 0 &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz6_shared_route_directory_tombstone_current,
             &current,
             current - 1u,
             memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

void hz6_shared_route_directory_diag_tombstone_cleared(size_t cleared) {
  if (cleared == 0) {
    return;
  }
  atomic_fetch_add_explicit(
      &g_hz6_shared_route_directory_maintenance_cleared,
      cleared,
      memory_order_relaxed);
  size_t current = atomic_load_explicit(
      &g_hz6_shared_route_directory_tombstone_current, memory_order_relaxed);
  while (current != 0) {
    size_t next = current > cleared ? current - cleared : 0;
    if (atomic_compare_exchange_weak_explicit(
            &g_hz6_shared_route_directory_tombstone_current,
            &current,
            next,
            memory_order_relaxed,
            memory_order_relaxed)) {
      return;
    }
  }
}

size_t hz6_shared_route_directory_diag_tombstone_current(void) {
  return atomic_load_explicit(
      &g_hz6_shared_route_directory_tombstone_current, memory_order_relaxed);
}

void hz6_shared_route_directory_diag_maintenance_attempt(void) {
  atomic_fetch_add_explicit(
      &g_hz6_shared_route_directory_maintenance_attempt,
      1u,
      memory_order_relaxed);
}

void hz6_shared_route_directory_diag_maintenance_success(void) {
  atomic_fetch_add_explicit(
      &g_hz6_shared_route_directory_maintenance_success,
      1u,
      memory_order_relaxed);
}

void hz6_shared_route_directory_note_stats(Hz6StatsSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }
  snapshot->shared_dir_tombstone_current =
      hz6_shared_route_directory_diag_tombstone_current();
  snapshot->shared_dir_tombstone_max =
      atomic_load_explicit(&g_hz6_shared_route_directory_tombstone_max,
                           memory_order_relaxed);
  snapshot->shared_dir_register_used_tombstone =
      atomic_load_explicit(&g_hz6_shared_route_directory_register_used_tombstone,
                           memory_order_relaxed);
  snapshot->shared_dir_maintenance_attempt =
      atomic_load_explicit(&g_hz6_shared_route_directory_maintenance_attempt,
                           memory_order_relaxed);
  snapshot->shared_dir_maintenance_success =
      atomic_load_explicit(&g_hz6_shared_route_directory_maintenance_success,
                           memory_order_relaxed);
  snapshot->shared_dir_maintenance_cleared =
      atomic_load_explicit(&g_hz6_shared_route_directory_maintenance_cleared,
                           memory_order_relaxed);
}
#else
void hz6_shared_route_directory_diag_tombstone_added(void) {
}

void hz6_shared_route_directory_diag_tombstone_reused(void) {
}

void hz6_shared_route_directory_diag_tombstone_cleared(size_t cleared) {
  (void)cleared;
}

size_t hz6_shared_route_directory_diag_tombstone_current(void) {
  return 0;
}

void hz6_shared_route_directory_diag_maintenance_attempt(void) {
}

void hz6_shared_route_directory_diag_maintenance_success(void) {
}

void hz6_shared_route_directory_note_stats(Hz6StatsSnapshot* snapshot) {
  (void)snapshot;
}
#endif
