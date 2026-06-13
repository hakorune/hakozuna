#include "hz6_allocator_route_owner_locality.h"

#include "hz6_allocator_route_hash.h"

#include <stdatomic.h>
#include <stdint.h>

#if HZ6_OWNER_LOCALITY_INDEX_L1
typedef struct Hz6OwnerLocalityIndexEntry {
  _Atomic(uintptr_t) base;
  _Atomic(Hz6Allocator*) allocator;
  _Atomic(unsigned int) generation;
  _Atomic(unsigned int) active;
  _Atomic(unsigned int) tombstone;
} Hz6OwnerLocalityIndexEntry;

static Hz6OwnerLocalityIndexEntry g_hz6_owner_locality_index
    [HZ6_OWNER_LOCALITY_INDEX_CAPACITY];

size_t hz6_allocator_owner_locality_index_bytes(void) {
  return sizeof(g_hz6_owner_locality_index);
}

void hz6_owner_locality_index_register(Hz6Allocator* allocator,
                                       void* base,
                                       uint32_t generation) {
  if (!allocator || !base) {
    return;
  }

  size_t start = hz6_route_directory_index(
      (uintptr_t)base, HZ6_OWNER_LOCALITY_INDEX_CAPACITY);
  size_t tombstone_index = (size_t)-1;
  for (size_t probe = 0; probe < HZ6_OWNER_LOCALITY_INDEX_CAPACITY;
       ++probe) {
    size_t index = (start + probe) % HZ6_OWNER_LOCALITY_INDEX_CAPACITY;
    Hz6OwnerLocalityIndexEntry* entry = &g_hz6_owner_locality_index[index];
    uintptr_t expected = 0;
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == (uintptr_t)base) {
      atomic_store_explicit(&entry->allocator,
                            allocator,
                            memory_order_release);
      atomic_store_explicit(&entry->generation, generation,
                            memory_order_release);
      atomic_store_explicit(&entry->active, 1u, memory_order_release);
      atomic_store_explicit(&entry->tombstone, 0u, memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.owner_locality_register;
#endif
      return;
    }
    if (current == 0) {
      if (atomic_load_explicit(&entry->tombstone, memory_order_acquire)) {
        if (tombstone_index == (size_t)-1) {
          tombstone_index = index;
        }
        continue;
      }
      if (!atomic_compare_exchange_strong_explicit(&entry->base,
                                                   &expected,
                                                   (uintptr_t)base,
                                                   memory_order_acq_rel,
                                                   memory_order_acquire)) {
        if (expected != (uintptr_t)base) {
          if (tombstone_index == (size_t)-1) {
            tombstone_index = index;
          }
          continue;
        }
      }
      if (tombstone_index == (size_t)-1) {
        tombstone_index = index;
      }
      break;
    }
    if (atomic_load_explicit(&entry->tombstone, memory_order_acquire) &&
        tombstone_index == (size_t)-1) {
      tombstone_index = index;
    }
  }

  if (tombstone_index == (size_t)-1) {
    return;
  }

  {
    Hz6OwnerLocalityIndexEntry* entry =
        &g_hz6_owner_locality_index[tombstone_index];
    atomic_store_explicit(&entry->base, (uintptr_t)base, memory_order_release);
    atomic_store_explicit(&entry->allocator, allocator, memory_order_release);
    atomic_store_explicit(&entry->generation, generation, memory_order_release);
    atomic_store_explicit(&entry->active, 1u, memory_order_release);
    atomic_store_explicit(&entry->tombstone, 0u, memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.owner_locality_register;
#endif
  }
}

void hz6_owner_locality_index_unregister(Hz6Allocator* allocator,
                                         void* base) {
  if (!allocator || !base) {
    return;
  }

  size_t start = hz6_route_directory_index(
      (uintptr_t)base, HZ6_OWNER_LOCALITY_INDEX_CAPACITY);
  for (size_t probe = 0; probe < HZ6_OWNER_LOCALITY_INDEX_CAPACITY;
       ++probe) {
    size_t index = (start + probe) % HZ6_OWNER_LOCALITY_INDEX_CAPACITY;
    Hz6OwnerLocalityIndexEntry* entry = &g_hz6_owner_locality_index[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      if (!atomic_load_explicit(&entry->tombstone, memory_order_acquire)) {
        return;
      }
      continue;
    }
    if (current != (uintptr_t)base) {
      continue;
    }
    Hz6Allocator* owner =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    if (owner != allocator) {
      return;
    }
    atomic_store_explicit(&entry->allocator, NULL, memory_order_release);
    atomic_store_explicit(&entry->generation, 0u, memory_order_release);
    atomic_store_explicit(&entry->active, 0u, memory_order_release);
    atomic_store_explicit(&entry->tombstone, 1u, memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.owner_locality_unregister;
#endif
    return;
  }
}

Hz6OwnerLocalityKind hz6_allocator_route_owner_locality_hint(
    const Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr) {
    return HZ6_OWNER_LOCALITY_MAYBE_LOCAL;
  }
  size_t probes = 0;
#if HZ6_DIAGNOSTIC_PROBES
  ++((Hz6Allocator*)allocator)->stats.owner_locality_lookup;
#endif
  size_t start = hz6_route_directory_index(
      (uintptr_t)ptr, HZ6_OWNER_LOCALITY_INDEX_CAPACITY);
  for (; probes < HZ6_OWNER_LOCALITY_INDEX_CAPACITY; ++probes) {
    size_t index = (start + probes) % HZ6_OWNER_LOCALITY_INDEX_CAPACITY;
    Hz6OwnerLocalityIndexEntry* entry = &g_hz6_owner_locality_index[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    unsigned int active =
        atomic_load_explicit(&entry->active, memory_order_acquire);
    if (!active) {
      if (!atomic_load_explicit(&entry->tombstone, memory_order_acquire)) {
        break;
      }
      continue;
    }
    if (current != (uintptr_t)ptr) {
      continue;
    }
    Hz6Allocator* owner =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    if (owner == allocator) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.owner_locality_hit_local_allocator;
      ((Hz6Allocator*)allocator)->stats.owner_locality_probe_total +=
          probes + 1u;
      if (probes + 1u >
          ((Hz6Allocator*)allocator)->stats.owner_locality_probe_max) {
        ((Hz6Allocator*)allocator)->stats.owner_locality_probe_max =
            probes + 1u;
      }
#endif
      return HZ6_OWNER_LOCALITY_MAYBE_LOCAL;
    }
    if (owner) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.owner_locality_hit_foreign_allocator;
      ((Hz6Allocator*)allocator)->stats.owner_locality_probe_total +=
          probes + 1u;
      if (probes + 1u >
          ((Hz6Allocator*)allocator)->stats.owner_locality_probe_max) {
        ((Hz6Allocator*)allocator)->stats.owner_locality_probe_max =
            probes + 1u;
      }
#endif
      return HZ6_OWNER_LOCALITY_DEFINITELY_FOREIGN;
    }
    break;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++((Hz6Allocator*)allocator)->stats.owner_locality_miss;
  ((Hz6Allocator*)allocator)->stats.owner_locality_probe_total += probes + 1u;
  if (probes + 1u >
      ((Hz6Allocator*)allocator)->stats.owner_locality_probe_max) {
    ((Hz6Allocator*)allocator)->stats.owner_locality_probe_max = probes + 1u;
  }
#endif
  return HZ6_OWNER_LOCALITY_MAYBE_LOCAL;
}
#else
size_t hz6_allocator_owner_locality_index_bytes(void) {
  return 0;
}

void hz6_owner_locality_index_register(Hz6Allocator* allocator,
                                       void* base,
                                       uint32_t generation) {
  (void)allocator;
  (void)base;
  (void)generation;
}

void hz6_owner_locality_index_unregister(Hz6Allocator* allocator,
                                         void* base) {
  (void)allocator;
  (void)base;
}

Hz6OwnerLocalityKind hz6_allocator_route_owner_locality_hint(
    const Hz6Allocator* allocator,
    const void* ptr) {
  (void)allocator;
  (void)ptr;
  return HZ6_OWNER_LOCALITY_MAYBE_LOCAL;
}
#endif
