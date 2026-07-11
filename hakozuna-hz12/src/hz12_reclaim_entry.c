#include "hz12_reclaim_entry.h"

#include <stdatomic.h>

static atomic_uint_fast32_t
    h12_reclaim_generations[HZ12_OWNER_REGISTRY_CAP];

void h12_reclaim_entry_reset(void) {
  uint32_t slot;
  for (slot = 0u; slot < HZ12_OWNER_REGISTRY_CAP; ++slot) {
    atomic_store_explicit(&h12_reclaim_generations[slot], 0u,
                          memory_order_relaxed);
  }
}

int h12_reclaim_entry_begin(H12OwnerToken owner) {
  uint_fast32_t expected = 0u;
  if (owner.slot >= HZ12_OWNER_REGISTRY_CAP || owner.generation == 0u) return 0;
  return atomic_compare_exchange_strong_explicit(
      &h12_reclaim_generations[owner.slot], &expected, owner.generation,
      memory_order_acq_rel, memory_order_acquire);
}

int h12_reclaim_entry_end(H12OwnerToken owner) {
  uint_fast32_t expected = owner.generation;
  if (owner.slot >= HZ12_OWNER_REGISTRY_CAP || owner.generation == 0u) return 0;
  return atomic_compare_exchange_strong_explicit(
      &h12_reclaim_generations[owner.slot], &expected, 0u,
      memory_order_acq_rel, memory_order_acquire);
}
