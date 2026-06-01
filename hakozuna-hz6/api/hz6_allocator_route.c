#include "hz6_allocator.h"

#include <stdatomic.h>
#include <stdint.h>

static _Atomic(Hz6Allocator*) g_hz6_visible_allocators
    [HZ6_ALLOCATOR_VISIBILITY_CAPACITY];

#if HZ6_SHARED_ROUTE_DIRECTORY_L1 || HZ6_OWNER_LOCALITY_INDEX_L1
static size_t hz6_route_directory_index(uintptr_t base) {
  uintptr_t x = base >> 4;
  x ^= x >> 33;
  x *= (uintptr_t)0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= (uintptr_t)0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return (size_t)(x % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY);
}
#endif

#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
typedef struct Hz6SharedRouteDirectoryEntry {
  _Atomic(uintptr_t) base;
  _Atomic(Hz6Allocator*) allocator;
  _Atomic(uintptr_t) descriptor;
  _Atomic(unsigned int) front_id;
  _Atomic(unsigned int) class_id;
  _Atomic(unsigned int) generation;
} Hz6SharedRouteDirectoryEntry;

static Hz6SharedRouteDirectoryEntry g_hz6_shared_route_directory
    [HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY];

static void hz6_shared_route_directory_register(Hz6Allocator* allocator,
                                                void* base,
                                                uint16_t front_id,
                                                uint16_t class_id,
                                                uint32_t generation,
                                                void* descriptor) {
  if (!allocator || !base || !descriptor) {
    return;
  }
  size_t start = hz6_route_directory_index((uintptr_t)base);
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t expected = 0;
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current != 0 && current != (uintptr_t)base) {
      continue;
    }
    if (current == 0 &&
        !atomic_compare_exchange_strong_explicit(&entry->base,
                                                 &expected,
                                                 (uintptr_t)base,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire)) {
      if (expected != (uintptr_t)base) {
        continue;
      }
    }
    atomic_store_explicit(&entry->descriptor,
                          (uintptr_t)descriptor,
                          memory_order_release);
    atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
    atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
    atomic_store_explicit(&entry->generation, generation, memory_order_release);
    atomic_store_explicit(&entry->allocator, allocator, memory_order_release);
    ++allocator->stats.shared_dir_register;
    return;
  }
}

static void hz6_shared_route_directory_unregister(Hz6Allocator* allocator,
                                                  void* base) {
  if (!allocator || !base) {
    return;
  }
  size_t start = hz6_route_directory_index((uintptr_t)base);
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      return;
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
    atomic_store_explicit(&entry->descriptor, 0, memory_order_release);
    atomic_store_explicit(&entry->front_id, 0, memory_order_release);
    atomic_store_explicit(&entry->class_id, 0, memory_order_release);
    atomic_store_explicit(&entry->generation, 0, memory_order_release);
    atomic_store_explicit(&entry->base, 0, memory_order_release);
    ++allocator->stats.shared_dir_unregister;
    return;
  }
}
#endif

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

static void hz6_owner_locality_index_register(Hz6Allocator* allocator,
                                              void* base,
                                              uint32_t generation) {
  if (!allocator || !base) {
    return;
  }

  size_t start = hz6_route_directory_index((uintptr_t)base);
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

static void hz6_owner_locality_index_unregister(Hz6Allocator* allocator,
                                                void* base) {
  if (!allocator || !base) {
    return;
  }

  size_t start = hz6_route_directory_index((uintptr_t)base);
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
#endif

void hz6_allocator_route_visibility_register(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (visible == allocator) {
      return;
    }
  }

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* expected = NULL;
    if (atomic_compare_exchange_strong_explicit(
            &g_hz6_visible_allocators[i],
            &expected,
            allocator,
            memory_order_release,
            memory_order_relaxed)) {
      return;
    }
  }
}

void hz6_allocator_route_visibility_unregister(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (visible != allocator) {
      continue;
    }
    Hz6Allocator* expected = allocator;
    (void)atomic_compare_exchange_strong_explicit(
        &g_hz6_visible_allocators[i],
        &expected,
        NULL,
        memory_order_release,
        memory_order_relaxed);
  }
}

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t lookup_probes = 0;
  Hz6RouteResult route =
      hz6_route_backend_lookup_probe(&allocator->route_backend,
                                     ptr,
                                     &lookup_probes);
  ((Hz6Allocator*)allocator)->stats.route_lookup_probe_total += lookup_probes;
  if (lookup_probes >
      ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max) {
    ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max = lookup_probes;
  }
#else
  Hz6RouteResult route = hz6_route_backend_lookup(&allocator->route_backend, ptr);
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_DIAGNOSTIC_PROBES
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (descriptor->source_release) {
        ++((Hz6Allocator*)allocator)->stats.source_owned_route_hit_local_owner;
      }
    }
#endif
  }
  return route;
}

Hz6RouteResult hz6_allocator_route_lookup_exact(const Hz6Allocator* allocator,
                                                const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t lookup_probes = 0;
  Hz6RouteResult route = hz6_route_backend_lookup_exact_probe(
      &allocator->route_backend, ptr, &lookup_probes);
  ((Hz6Allocator*)allocator)->stats.route_exact_lookup_probe_total +=
      lookup_probes;
  if (lookup_probes >
      ((Hz6Allocator*)allocator)->stats.route_exact_lookup_probe_max) {
    ((Hz6Allocator*)allocator)->stats.route_exact_lookup_probe_max =
        lookup_probes;
  }
#else
  Hz6RouteResult route =
      hz6_route_backend_lookup_exact(&allocator->route_backend, ptr);
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_DIAGNOSTIC_PROBES
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (descriptor->source_release) {
        ++((Hz6Allocator*)allocator)->stats.source_owned_route_hit_local_owner;
      }
    }
#endif
  }
  return route;
}

Hz6RouteResult hz6_allocator_route_lookup_visible(Hz6Allocator* allocator,
                                                  const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }

  Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
  if (route.kind != HZ6_ROUTE_MISS) {
    return route;
  }

  return hz6_allocator_route_lookup_visible_after_local_miss(allocator, ptr);
}

Hz6OwnerLocalityKind hz6_allocator_route_owner_locality_hint(
    const Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr) {
    return HZ6_OWNER_LOCALITY_MAYBE_LOCAL;
  }
#if HZ6_OWNER_LOCALITY_INDEX_L1
  size_t probes = 0;
#if HZ6_DIAGNOSTIC_PROBES
  ++((Hz6Allocator*)allocator)->stats.owner_locality_lookup;
#endif
  size_t start = hz6_route_directory_index((uintptr_t)ptr);
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
#else
  (void)allocator;
  (void)ptr;
#endif
  return HZ6_OWNER_LOCALITY_MAYBE_LOCAL;
}

Hz6RouteResult hz6_allocator_route_lookup_visible_only(Hz6Allocator* allocator,
                                                       const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }

  Hz6RouteResult route = hz6_route_miss();
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  ++allocator->stats.route_visibility_lookup;
#endif
  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (!visible || visible == allocator) {
      continue;
    }
    if (!hz6_owner_is_alive(&visible->owner, visible->owner.token)) {
      continue;
    }

#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    route = hz6_route_backend_lookup(&visible->route_backend, ptr);
    if (route.kind != HZ6_ROUTE_MISS) {
      route.route_allocator = visible;
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.route_visibility_hit;
      if (route.descriptor) {
        const Hz6ObjectDescriptor* descriptor =
            (const Hz6ObjectDescriptor*)route.descriptor;
        if (hz6_owner_equal(descriptor->owner, allocator->owner.token)) {
          ++allocator->stats.route_visibility_hit_local_owner;
          if (descriptor->source_release) {
            ++allocator->stats.source_owned_visibility_hit_local_owner;
          }
        } else {
          ++allocator->stats.route_visibility_hit_foreign_owner;
          if (descriptor->source_release) {
            ++allocator->stats.source_owned_visibility_hit_foreign_owner;
          }
        }
      }
      allocator->stats.route_visibility_probe_total += probes;
      if (probes > allocator->stats.route_visibility_probe_max) {
        allocator->stats.route_visibility_probe_max = probes;
      }
#endif
      return route;
    }
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_visibility_miss;
  allocator->stats.route_visibility_probe_total += probes;
  if (probes > allocator->stats.route_visibility_probe_max) {
    allocator->stats.route_visibility_probe_max = probes;
  }
#endif
  return hz6_route_miss();
}

Hz6RouteResult hz6_allocator_route_lookup_visible_after_local_miss(
    Hz6Allocator* allocator,
    const void* ptr) {
  return hz6_allocator_route_lookup_visible_only(allocator, ptr);
}

int hz6_allocator_route_negative_filter_skip_local(
    Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr) {
    return 0;
  }

  uintptr_t probe = (uintptr_t)ptr;
#if HZ6_DIAGNOSTIC_PROBES
  size_t range_probes = 0;
#endif
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
#if HZ6_DIAGNOSTIC_PROBES
    ++range_probes;
#endif
    const Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!block->active || !block->ptr || block->bytes == 0) {
      continue;
    }
    uintptr_t base = (uintptr_t)block->ptr;
    uintptr_t limit = base + (uintptr_t)block->bytes;
    if (probe >= base && probe < limit) {
#if HZ6_DIAGNOSTIC_PROBES
      allocator->stats.negative_filter_range_probe_total += range_probes;
      if (range_probes >
          allocator->stats.negative_filter_range_probe_max) {
        allocator->stats.negative_filter_range_probe_max = range_probes;
      }
#endif
      return 0;
    }
  }

#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.negative_filter_range_probe_total += range_probes;
  if (range_probes > allocator->stats.negative_filter_range_probe_max) {
    allocator->stats.negative_filter_range_probe_max = range_probes;
  }
#endif
  return 1;
}

void hz6_allocator_route_shared_directory_dryrun(Hz6Allocator* allocator,
                                                 const void* ptr) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !ptr) {
    return;
  }
  ++allocator->stats.shared_dir_lookup;
  uintptr_t key = (uintptr_t)ptr;
  size_t start = hz6_route_directory_index(key);
  size_t probes = 0;
  for (; probes < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probes) {
    size_t index = (start + probes) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      ++allocator->stats.shared_dir_miss;
      break;
    }
    if (current != key) {
      continue;
    }
    Hz6Allocator* route_allocator =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    uintptr_t descriptor_value =
        atomic_load_explicit(&entry->descriptor, memory_order_acquire);
    if (!route_allocator || descriptor_value == 0) {
      ++allocator->stats.shared_dir_stale;
      break;
    }
    ++allocator->stats.shared_dir_hit;
    if (route_allocator == allocator) {
      ++allocator->stats.shared_dir_hit_local_allocator;
    } else {
      ++allocator->stats.shared_dir_hit_foreign_allocator;
      ++allocator->stats.shared_dir_would_skip_local;
    }
    break;
  }
  if (probes == HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY) {
    ++allocator->stats.shared_dir_miss;
  }
  size_t probe_count = probes + 1;
  allocator->stats.shared_dir_probe_total += probe_count;
  if (probe_count > allocator->stats.shared_dir_probe_max) {
    allocator->stats.shared_dir_probe_max = probe_count;
  }
#else
  (void)allocator;
  (void)ptr;
#endif
}

Hz6RouteResult hz6_allocator_route_shared_directory_lookup_exact(
    Hz6Allocator* allocator,
    const void* ptr) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
  ++allocator->stats.shared_dir_first_attempt;
  uintptr_t key = (uintptr_t)ptr;
  size_t start = hz6_route_directory_index(key);
  for (size_t probes = 0; probes < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
       ++probes) {
    size_t index = (start + probes) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      ++allocator->stats.shared_dir_first_fallback;
      return hz6_route_miss();
    }
    if (current != key) {
      continue;
    }
    Hz6Allocator* route_allocator =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    uintptr_t descriptor_value =
        atomic_load_explicit(&entry->descriptor, memory_order_acquire);
    if (!route_allocator || descriptor_value == 0) {
      ++allocator->stats.shared_dir_first_fallback;
      return hz6_route_miss();
    }
    unsigned int front_id =
        atomic_load_explicit(&entry->front_id, memory_order_acquire);
    unsigned int class_id =
        atomic_load_explicit(&entry->class_id, memory_order_acquire);
    unsigned int generation =
        atomic_load_explicit(&entry->generation, memory_order_acquire);
    Hz6RouteResult route = hz6_route_valid((uint16_t)front_id,
                                           (uint16_t)class_id,
                                           (uint32_t)generation,
                                           (void*)descriptor_value);
    route.route_allocator = route_allocator;
    if (route.kind == HZ6_ROUTE_VALID) {
      ++allocator->stats.shared_dir_first_hit;
    }
    return route;
  }
  ++allocator->stats.shared_dir_first_fallback;
  return hz6_route_miss();
#else
  (void)allocator;
  (void)ptr;
  return hz6_route_miss();
#endif
}

int hz6_allocator_route_rehome_exact(Hz6Allocator* allocator,
                                     const Hz6RouteResult* route) {
  if (!allocator || !route || route->kind != HZ6_ROUTE_VALID ||
      !route->descriptor || !route->route_allocator ||
      route->route_allocator == allocator) {
    return 0;
  }

  const Hz6ObjectDescriptor* descriptor =
      (const Hz6ObjectDescriptor*)route->descriptor;
  Hz6Allocator* origin = route->route_allocator;
  void* ptr = descriptor->ptr;
  size_t bytes = descriptor->bytes;
  hz6_allocator_route_unregister_exact(origin, ptr);
  if (!hz6_allocator_route_register_exact(allocator,
                                          ptr,
                                          bytes,
                                          route->front_id,
                                          route->class_id,
                                          route->generation,
                                          (void*)descriptor)) {
    hz6_allocator_route_register_exact(origin,
                                      ptr,
                                      bytes,
                                      route->front_id,
                                      route->class_id,
                                      route->generation,
                                      (void*)descriptor);
    return 0;
  }

  return 1;
}

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr) {
  if (!allocator || !ptr) {
    return;
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_DIAGNOSTIC_PROBES
  hz6_shared_route_directory_unregister(allocator, ptr);
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  hz6_owner_locality_index_unregister(allocator, ptr);
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, &probes);
  allocator->stats.route_unregister_probe_total += probes;
  if (probes > allocator->stats.route_unregister_probe_max) {
    allocator->stats.route_unregister_probe_max = probes;
  }
  allocator->stats.route_active_current =
      allocator->route_backend.exact_table.active_count;
  if (allocator->stats.route_active_current >
      allocator->stats.route_active_max) {
    allocator->stats.route_active_max =
        allocator->stats.route_active_current;
  }
#else
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, NULL);
#endif
}

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor) {
  if (!allocator || !base || bytes == 0) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  int ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                            base,
                                            bytes,
                                            front_id,
                                            class_id,
                                            generation,
                                            descriptor,
                                            &probes);
  allocator->stats.route_register_probe_total += probes;
  if (probes > allocator->stats.route_register_probe_max) {
    allocator->stats.route_register_probe_max = probes;
  }
  allocator->stats.route_active_current =
      allocator->route_backend.exact_table.active_count;
  if (allocator->stats.route_active_current >
      allocator->stats.route_active_max) {
    allocator->stats.route_active_max =
        allocator->stats.route_active_current;
  }
  if (!ok) {
    ++allocator->stats.route_register_fail;
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  if (ok) {
    hz6_shared_route_directory_register(allocator,
                                        base,
                                        front_id,
                                        class_id,
                                        generation,
                                        descriptor);
  }
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (ok) {
    hz6_owner_locality_index_register(allocator, base, generation);
  }
#endif
  return ok;
#else
  int ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                            base,
                                            bytes,
                                            front_id,
                                            class_id,
                                            generation,
                                            descriptor,
                                            NULL);
  return ok;
#endif
}

Hz6RouteBackendKind hz6_allocator_route_backend_kind(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return HZ6_ROUTE_BACKEND_EXACT_TABLE;
  }
  return allocator->route_backend.kind;
}

size_t hz6_allocator_route_page_granularity(const Hz6Allocator* allocator) {
  return allocator ? allocator->route_backend.page_granularity : 0;
}
