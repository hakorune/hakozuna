#include "hz6_allocator.h"

#include <stdatomic.h>
#include <stdint.h>

static _Atomic(Hz6Allocator*) g_hz6_visible_allocators
    [HZ6_ALLOCATOR_VISIBILITY_CAPACITY];
static _Atomic(size_t) g_hz6_visible_allocator_count;

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

#if HZ6_SHARED_ROUTE_DIRECTORY_L1
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

#define HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE ((uintptr_t)UINTPTR_MAX)

#if HZ6_ELASTIC_ROUTE_OVERFLOW_L1
typedef struct Hz6SharedRouteRangeEntry {
  _Atomic(uintptr_t) base;
  _Atomic(uintptr_t) bytes;
  _Atomic(unsigned int) front_id;
  _Atomic(unsigned int) class_id;
} Hz6SharedRouteRangeEntry;

static Hz6SharedRouteRangeEntry g_hz6_shared_route_ranges
    [HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY];
#endif

size_t hz6_allocator_shared_route_directory_bytes(void) {
  size_t bytes = sizeof(g_hz6_shared_route_directory);
#if HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  bytes += sizeof(g_hz6_shared_route_ranges);
#endif
  return bytes;
}

static int hz6_shared_route_directory_register(Hz6Allocator* allocator,
                                               void* base,
                                               uint16_t front_id,
                                               uint16_t class_id,
                                               uint32_t generation,
                                               void* descriptor) {
  if (!allocator || !base || !descriptor) {
    return 0;
  }
  size_t start = hz6_route_directory_index((uintptr_t)base);
  size_t tombstone_index = (size_t)-1;
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t expected = 0;
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      if (tombstone_index == (size_t)-1) {
        tombstone_index = index;
      }
      continue;
    }
    if (current != 0 && current != (uintptr_t)base) {
      continue;
    }
    if (current == 0) {
      if (tombstone_index != (size_t)-1) {
        entry = &g_hz6_shared_route_directory[tombstone_index];
        expected = HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE;
        if (!atomic_compare_exchange_strong_explicit(
                &entry->base,
                &expected,
                (uintptr_t)base,
                memory_order_acq_rel,
                memory_order_acquire)) {
          continue;
        }
      } else if (!atomic_compare_exchange_strong_explicit(
                     &entry->base,
                     &expected,
                     (uintptr_t)base,
                     memory_order_acq_rel,
                     memory_order_acquire)) {
        if (expected != (uintptr_t)base) {
          continue;
        }
      }
    }
    atomic_store_explicit(&entry->descriptor,
                          (uintptr_t)descriptor,
                          memory_order_release);
    atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
    atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
    atomic_store_explicit(&entry->generation, generation, memory_order_release);
    atomic_store_explicit(&entry->allocator, allocator, memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_register;
#endif
    return 1;
  }
  if (tombstone_index != (size_t)-1) {
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[tombstone_index];
    uintptr_t expected = HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE;
    if (atomic_compare_exchange_strong_explicit(&entry->base,
                                                &expected,
                                                (uintptr_t)base,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
      atomic_store_explicit(&entry->descriptor,
                            (uintptr_t)descriptor,
                            memory_order_release);
      atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
      atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
      atomic_store_explicit(&entry->generation,
                            generation,
                            memory_order_release);
      atomic_store_explicit(&entry->allocator,
                            allocator,
                            memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.shared_dir_register;
#endif
      return 1;
    }
  }
  return 0;
}

static Hz6RouteResult hz6_shared_route_directory_lookup_raw(const void* ptr) {
  if (!ptr) {
    return hz6_route_miss();
  }
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
      return hz6_route_miss();
    }
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      continue;
    }
    if (current != key) {
      continue;
    }
    Hz6Allocator* route_allocator =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    uintptr_t descriptor_value =
        atomic_load_explicit(&entry->descriptor, memory_order_acquire);
    if (!route_allocator || descriptor_value == 0) {
      return hz6_route_miss();
    }
    unsigned int front_id =
        atomic_load_explicit(&entry->front_id, memory_order_acquire);
    unsigned int class_id =
        atomic_load_explicit(&entry->class_id, memory_order_acquire);
    unsigned int generation =
        atomic_load_explicit(&entry->generation, memory_order_acquire);
    const Hz6ObjectDescriptor* descriptor =
        (const Hz6ObjectDescriptor*)descriptor_value;
    if (!descriptor || descriptor->ptr != ptr ||
        descriptor->generation != (uint32_t)generation ||
        descriptor->state == HZ6_STATE_DEAD ||
        descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED) {
      return hz6_route_miss();
    }
    if (descriptor->source_block &&
        (!hz6_source_block_active(descriptor->source_block) ||
         !descriptor->source_block->ptr)) {
      return hz6_route_miss();
    }
    Hz6RouteResult route = hz6_route_valid((uint16_t)front_id,
                                           (uint16_t)class_id,
                                           (uint32_t)generation,
                                           (void*)descriptor_value);
    route.route_allocator = route_allocator;
    return route;
  }
  return hz6_route_miss();
}

#if HZ6_ELASTIC_ROUTE_OVERFLOW_L1
static int hz6_shared_route_range_register(void* base,
                                           size_t bytes,
                                           uint16_t front_id,
                                           uint16_t class_id) {
  if (!base || bytes == 0) {
    return 0;
  }
  uintptr_t key = (uintptr_t)base;
  size_t start = hz6_route_directory_index(key);
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
       ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteRangeEntry* entry = &g_hz6_shared_route_ranges[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == key) {
      return 0;
    }
    if (current != 0) {
      continue;
    }
    uintptr_t expected = 0;
    if (!atomic_compare_exchange_strong_explicit(&entry->base,
                                                 &expected,
                                                 key,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire)) {
      continue;
    }
    atomic_store_explicit(&entry->bytes, (uintptr_t)bytes,
                          memory_order_release);
    atomic_store_explicit(&entry->front_id, (unsigned int)front_id,
                          memory_order_release);
    atomic_store_explicit(&entry->class_id, (unsigned int)class_id,
                          memory_order_release);
    return 1;
  }
  return 0;
}

static void hz6_shared_route_range_unregister(void* base) {
  if (!base) {
    return;
  }
  uintptr_t key = (uintptr_t)base;
  size_t start = hz6_route_directory_index(key);
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
       ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteRangeEntry* entry = &g_hz6_shared_route_ranges[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      return;
    }
    if (current != key) {
      continue;
    }
    atomic_store_explicit(&entry->base, (uintptr_t)0, memory_order_release);
    atomic_store_explicit(&entry->bytes, (uintptr_t)0, memory_order_release);
    atomic_store_explicit(&entry->front_id, 0u, memory_order_release);
    atomic_store_explicit(&entry->class_id, 0u, memory_order_release);
    return;
  }
}

static Hz6RouteResult hz6_shared_route_range_lookup_raw(const void* ptr) {
  if (!ptr) {
    return hz6_route_miss();
  }
  uintptr_t addr = (uintptr_t)ptr;
  for (size_t i = 0; i < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++i) {
    Hz6SharedRouteRangeEntry* entry = &g_hz6_shared_route_ranges[i];
    uintptr_t base = atomic_load_explicit(&entry->base, memory_order_acquire);
    if (base == 0) {
      continue;
    }
    uintptr_t bytes =
        atomic_load_explicit(&entry->bytes, memory_order_acquire);
    if (bytes == 0 || addr < base || addr >= base + bytes) {
      continue;
    }
    unsigned int front_id =
        atomic_load_explicit(&entry->front_id, memory_order_acquire);
    unsigned int class_id =
        atomic_load_explicit(&entry->class_id, memory_order_acquire);
    return hz6_route_invalid((uint16_t)front_id, (uint16_t)class_id);
  }
  return hz6_route_miss();
}

int hz6_allocator_route_register_shared_invalid_range(Hz6Allocator* allocator,
                                                      void* base,
                                                      size_t bytes,
                                                      uint16_t front_id,
                                                      uint16_t class_id) {
  (void)allocator;
  return hz6_shared_route_range_register(base, bytes, front_id, class_id);
}

void hz6_allocator_route_unregister_shared_invalid_range(
    Hz6Allocator* allocator,
    void* base) {
  (void)allocator;
  hz6_shared_route_range_unregister(base);
}
#endif

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
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
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
    atomic_store_explicit(&entry->descriptor, 0, memory_order_release);
    atomic_store_explicit(&entry->front_id, 0, memory_order_release);
    atomic_store_explicit(&entry->class_id, 0, memory_order_release);
    atomic_store_explicit(&entry->generation, 0, memory_order_release);
    atomic_store_explicit(&entry->base, HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE,
                          memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_unregister;
#endif
    return;
  }
}
#else
size_t hz6_allocator_shared_route_directory_bytes(void) {
  return 0;
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

size_t hz6_allocator_owner_locality_index_bytes(void) {
  return sizeof(g_hz6_owner_locality_index);
}

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
#else
size_t hz6_allocator_owner_locality_index_bytes(void) {
  return 0;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_allocator_note_route_probe_hist(size_t* hist,
                                                size_t probes);
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
      atomic_fetch_add_explicit(&g_hz6_visible_allocator_count, 1u,
                                memory_order_acq_rel);
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
    if (atomic_compare_exchange_strong_explicit(&g_hz6_visible_allocators[i],
                                                &expected,
                                                NULL,
                                                memory_order_release,
                                                memory_order_relaxed)) {
      atomic_fetch_sub_explicit(&g_hz6_visible_allocator_count, 1u,
                                memory_order_acq_rel);
      return;
    }
  }
}

Hz6RouteResult hz6_allocator_route_lookup(const Hz6Allocator* allocator,
                                          const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t lookup_probes = 0;
  size_t page_exact_hash_probes = 0;
  size_t page_exact_range_probes = 0;
  size_t page_invalid_probes = 0;
  if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    ++((Hz6Allocator*)allocator)->stats.route_lookup_page_backend;
  } else {
    ++((Hz6Allocator*)allocator)->stats.route_lookup_exact_backend;
  }
  Hz6RouteResult route =
      hz6_route_backend_lookup_probe_ex(&allocator->route_backend,
                                        ptr,
                                        &lookup_probes,
                                        &page_exact_hash_probes,
                                        &page_exact_range_probes,
                                        &page_invalid_probes);
  ((Hz6Allocator*)allocator)->stats.route_lookup_probe_total += lookup_probes;
  hz6_allocator_note_route_probe_hist(
      ((Hz6Allocator*)allocator)->stats.route_lookup_probe_hist,
      lookup_probes);
  if (lookup_probes >
      ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max) {
    ((Hz6Allocator*)allocator)->stats.route_lookup_probe_max = lookup_probes;
  }
  if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
    ((Hz6Allocator*)allocator)->stats.route_lookup_page_probe_total +=
        lookup_probes;
    if (lookup_probes >
        ((Hz6Allocator*)allocator)->stats.route_lookup_page_probe_max) {
      ((Hz6Allocator*)allocator)->stats.route_lookup_page_probe_max =
          lookup_probes;
    }
    size_t page_exact_probes =
        page_exact_hash_probes + page_exact_range_probes;
    ((Hz6Allocator*)allocator)->stats.route_lookup_page_exact_probe_total +=
        page_exact_probes;
    if (page_exact_probes >
        ((Hz6Allocator*)allocator)->stats.route_lookup_page_exact_probe_max) {
      ((Hz6Allocator*)allocator)->stats.route_lookup_page_exact_probe_max =
          page_exact_probes;
    }
    ((Hz6Allocator*)allocator)->stats
        .route_lookup_page_exact_hash_probe_total += page_exact_hash_probes;
    if (page_exact_hash_probes >
        ((Hz6Allocator*)allocator)
            ->stats.route_lookup_page_exact_hash_probe_max) {
      ((Hz6Allocator*)allocator)
          ->stats.route_lookup_page_exact_hash_probe_max =
          page_exact_hash_probes;
    }
    ((Hz6Allocator*)allocator)->stats
        .route_lookup_page_exact_range_probe_total += page_exact_range_probes;
    if (page_exact_range_probes >
        ((Hz6Allocator*)allocator)
            ->stats.route_lookup_page_exact_range_probe_max) {
      ((Hz6Allocator*)allocator)
          ->stats.route_lookup_page_exact_range_probe_max =
          page_exact_range_probes;
    }
    ((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid_probe_total +=
        page_invalid_probes;
    if (page_invalid_probes >
        ((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid_probe_max) {
      ((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid_probe_max =
          page_invalid_probes;
    }
  }
#else
  Hz6RouteResult route = hz6_route_backend_lookup(&allocator->route_backend, ptr);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  if (route.kind != HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_lookup;
#endif
    Hz6RouteResult overflow_route =
        hz6_shared_route_directory_lookup_raw(ptr);
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_lookup;
#endif
    if (overflow_route.kind == HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_hit;
      ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_hit;
#endif
      return overflow_route;
    }
    if (route.kind == HZ6_ROUTE_MISS) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_range_lookup;
#endif
      Hz6RouteResult overflow_range =
          hz6_shared_route_range_lookup_raw(ptr);
      if (overflow_range.kind == HZ6_ROUTE_INVALID) {
#if HZ6_DIAGNOSTIC_PROBES
        ++((Hz6Allocator*)allocator)->stats.route_lookup_overflow_range_hit;
#endif
        return overflow_range;
      }
    }
  }
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
      if (route.kind == HZ6_ROUTE_VALID) {
        ++((Hz6Allocator*)allocator)->stats.route_lookup_page_valid;
      } else if (route.kind == HZ6_ROUTE_INVALID) {
        ++((Hz6Allocator*)allocator)->stats.route_lookup_page_invalid;
      }
    }
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
        ++((Hz6Allocator*)allocator)->stats.source_owned_route_hit_local_owner;
      }
    }
#endif
  } else {
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE) {
      ++((Hz6Allocator*)allocator)->stats.route_lookup_page_miss;
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
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  if (route.kind == HZ6_ROUTE_MISS) {
    Hz6RouteResult overflow_route =
        hz6_shared_route_directory_lookup_raw(ptr);
#if HZ6_DIAGNOSTIC_PROBES
    ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_lookup;
#endif
    if (overflow_route.kind == HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
      ++((Hz6Allocator*)allocator)->stats.elastic_route_overflow_hit;
#endif
      return overflow_route;
    }
  }
#endif
  if (route.kind != HZ6_ROUTE_MISS) {
    route.route_allocator = (Hz6Allocator*)allocator;
#if HZ6_DIAGNOSTIC_PROBES
    if (allocator->route_backend.kind == HZ6_ROUTE_BACKEND_PAGE_TABLE &&
        route.kind == HZ6_ROUTE_MISS) {
      ++((Hz6Allocator*)allocator)->stats.route_lookup_page_miss;
    }
    if (route.descriptor) {
      const Hz6ObjectDescriptor* descriptor =
          (const Hz6ObjectDescriptor*)route.descriptor;
      if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
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

  if (atomic_load_explicit(&g_hz6_visible_allocator_count,
                           memory_order_acquire) <= 1u) {
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
        if (hz6_allocator_descriptor_owner_equal_at(
                visible, descriptor, allocator->owner.token,
                HZ6_OWNER_EQUAL_SITE_VISIBLE_LOOKUP)) {
          ++allocator->stats.route_visibility_hit_local_owner;
          if (hz6_allocator_descriptor_has_source_release(visible,
                                                          descriptor)) {
            ++allocator->stats.source_owned_visibility_hit_local_owner;
          }
        } else {
          ++allocator->stats.route_visibility_hit_foreign_owner;
          if (hz6_allocator_descriptor_has_source_release(visible,
                                                          descriptor)) {
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

#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_DIAGNOSTIC_PROBES || \
    HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || HZ6_OWNER_SOURCE_SIDE_META_L2
Hz6Allocator* hz6_allocator_descriptor_storage_owner(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count) {
  if (probe_count) {
    *probe_count = 0;
  }
  if (!observer || !descriptor) {
    return NULL;
  }

#if HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
  if (hz6_allocator_descriptor_is_depot(descriptor)) {
    return hz6_allocator_owner_source_side_meta_storage(observer, descriptor);
  }
#endif

  for (size_t i = 0; i < HZ6_ALLOCATOR_VISIBILITY_CAPACITY; ++i) {
    Hz6Allocator* visible =
        atomic_load_explicit(&g_hz6_visible_allocators[i],
                             memory_order_acquire);
    if (!visible) {
      continue;
    }
    if (probe_count) {
      ++*probe_count;
    }
    if (hz6_allocator_descriptor_belongs_to(visible, descriptor)) {
      return visible;
    }
  }
  return NULL;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES
Hz6Allocator* hz6_allocator_descriptor_storage_owner_diagnostic(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count) {
  return hz6_allocator_descriptor_storage_owner(observer, descriptor,
                                                probe_count);
}
#endif

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
    if (!hz6_source_block_active(block) || !block->ptr ||
        block->bytes == 0) {
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
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.shared_dir_first_attempt;
#endif
  Hz6RouteResult route = hz6_shared_route_directory_lookup_raw(ptr);
  if (route.kind == HZ6_ROUTE_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_first_hit;
#endif
    return route;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.shared_dir_first_fallback;
#endif
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
  hz6_allocator_route_unregister_exact_reason(
      origin, ptr, HZ6_ROUTE_UNREGISTER_REASON_REHOME);
  if (!hz6_allocator_route_register_exact_reason(
          allocator, ptr, bytes, route->front_id, route->class_id,
          route->generation, (void*)descriptor,
          HZ6_ROUTE_REGISTER_REASON_REHOME)) {
    hz6_allocator_route_register_exact_reason(
        origin, ptr, bytes, route->front_id, route->class_id,
        route->generation, (void*)descriptor,
        HZ6_ROUTE_REGISTER_REASON_REHOME);
    return 0;
  }

  return 1;
}

#if HZ6_DIAGNOSTIC_PROBES
static size_t hz6_allocator_route_probe_bucket(size_t probes) {
  if (probes <= 1) {
    return 0;
  }
  if (probes <= 4) {
    return 1;
  }
  if (probes <= 8) {
    return 2;
  }
  if (probes <= 16) {
    return 3;
  }
  if (probes <= 64) {
    return 4;
  }
  return 5;
}

static void hz6_allocator_note_route_probe_hist(size_t* hist,
                                                size_t probes) {
  if (!hist) {
    return;
  }
  ++hist[hz6_allocator_route_probe_bucket(probes)];
}

static void hz6_allocator_note_route_tombstones(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
  Hz6RouteTable* table = &allocator->route_backend.exact_table;
  allocator->stats.route_tombstone_current = table->tombstone_count;
  if (table->tombstone_count > allocator->stats.route_tombstone_max) {
    allocator->stats.route_tombstone_max = table->tombstone_count;
  }
  allocator->stats.route_register_used_tombstone =
      table->register_used_tombstone;
  allocator->stats.route_register_full_probe_with_tombstone =
      table->register_full_probe_with_tombstone;
}
#endif

#if HZ6_ROUTE_TOMBSTONE_COMPACT_L1 && \
    !HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
static size_t hz6_allocator_route_tombstone_compact_threshold(
    const Hz6RouteTable* table) {
  if (!table || table->capacity == 0) {
    return (size_t)-1;
  }
  size_t threshold = HZ6_ROUTE_TOMBSTONE_COMPACT_MIN;
#if !HZ6_ROUTE_TOMBSTONE_COMPACT_AGGRESSIVE_L1
  size_t half = table->capacity / 2;
  if (threshold < half) {
    threshold = half;
  }
#endif
  if (threshold >= table->capacity) {
    threshold = table->capacity - 1;
  }
  return threshold == 0 ? 1 : threshold;
}
#endif

#if HZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1 || \
    HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
static int hz6_allocator_route_conditional_tombstone_ready(
    Hz6Allocator* allocator,
    int note_stats) {
  if (!allocator) {
    return 0;
  }
  Hz6RouteTable* table = &allocator->route_backend.exact_table;
  if (!table || table->capacity == 0) {
    return 0;
  }
  size_t tombstones = table->tombstone_count;
  size_t abs_min = HZ6_ROUTE_TOMBSTONE_CONDITIONAL_ABS_MIN;
  if (abs_min == 0) {
    abs_min = 1;
  }
  if (abs_min >= table->capacity) {
    abs_min = table->capacity - 1;
  }
  if (tombstones < abs_min) {
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  if (note_stats) {
    ++allocator->stats.route_tombstone_cond_probe;
  }
#else
  (void)note_stats;
#endif
  size_t active = table->active_count;
  int ratio25 = active != 0 && tombstones * 4u >= active;
  int occupancy75 =
      (active + tombstones) * 4u >= table->capacity * 3u;
#if HZ6_DIAGNOSTIC_PROBES
  if (note_stats) {
    if (ratio25) {
      ++allocator->stats.route_tombstone_cond_ratio25;
    }
    if (occupancy75) {
      ++allocator->stats.route_tombstone_cond_occupancy75;
    }
  }
#endif
  if (!ratio25 && !occupancy75) {
    return 0;
  }

  size_t cooldown = HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COOLDOWN;
  size_t highwater = allocator->stats.route_tombstone_cond_highwater;
  if (cooldown != 0 && highwater != 0 &&
      tombstones < highwater + cooldown) {
#if HZ6_DIAGNOSTIC_PROBES
    if (note_stats) {
      ++allocator->stats.route_tombstone_cond_cooldown_blocked;
    }
#endif
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  if (note_stats) {
    ++allocator->stats.route_tombstone_cond_would_compact;
  }
#endif
  allocator->stats.route_tombstone_cond_highwater = tombstones;
  return 1;
}
#endif

static void hz6_allocator_route_note_conditional_tombstone_dryrun(
    Hz6Allocator* allocator) {
#if HZ6_ROUTE_TOMBSTONE_CONDITIONAL_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES
  (void)hz6_allocator_route_conditional_tombstone_ready(allocator, 1);
#else
  (void)allocator;
#endif
}

static void hz6_allocator_route_maybe_compact_tombstones(
    Hz6Allocator* allocator) {
#if HZ6_ROUTE_TOMBSTONE_COMPACT_L1
  if (!allocator) {
    return;
  }
#if HZ6_ROUTE_TOMBSTONE_CONDITIONAL_COMPACT_L1
  if (!hz6_allocator_route_conditional_tombstone_ready(allocator, 1)) {
    return;
  }
#else
  Hz6RouteTable* table = &allocator->route_backend.exact_table;
  size_t threshold =
      hz6_allocator_route_tombstone_compact_threshold(table);
  if (table->tombstone_count < threshold) {
    return;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.route_tombstone_compact_attempt;
#endif
  size_t moved = 0;
  if (hz6_route_backend_compact_tombstones(&allocator->route_backend,
                                           &moved)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.route_tombstone_compact_success;
    allocator->stats.route_tombstone_compact_moved += moved;
#endif
  } else {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.route_tombstone_compact_fail_alloc;
#endif
  }
#else
  (void)allocator;
#endif
}

static void hz6_allocator_note_route_register_reason(
    Hz6Allocator* allocator,
    Hz6RouteRegisterReason reason) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator) {
    return;
  }
  switch (reason) {
    case HZ6_ROUTE_REGISTER_REASON_SOURCE_RUN_SLOT:
      ++allocator->stats.route_register_reason_source_run_slot;
      break;
    case HZ6_ROUTE_REGISTER_REASON_DIRECT_SOURCE:
      ++allocator->stats.route_register_reason_direct_source;
      break;
    case HZ6_ROUTE_REGISTER_REASON_MATERIALIZE:
      ++allocator->stats.route_register_reason_materialize;
      break;
    case HZ6_ROUTE_REGISTER_REASON_REHOME:
      ++allocator->stats.route_register_reason_rehome;
      break;
    case HZ6_ROUTE_REGISTER_REASON_UNKNOWN:
    default:
      ++allocator->stats.route_register_reason_unknown;
      break;
  }
#else
  (void)allocator;
  (void)reason;
#endif
}

static void hz6_allocator_note_route_unregister_reason(
    Hz6Allocator* allocator,
    Hz6RouteUnregisterReason reason) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator) {
    return;
  }
  switch (reason) {
    case HZ6_ROUTE_UNREGISTER_REASON_FRONTCACHE_OVERFLOW:
      ++allocator->stats.route_unregister_reason_frontcache_overflow;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_CAP_RELEASE:
      ++allocator->stats.route_unregister_reason_cap_release;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_DESCRIPTORLESS_DETACH:
      ++allocator->stats.route_unregister_reason_descriptorless_detach;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_SOURCE_SLOT_RELEASE:
      ++allocator->stats.route_unregister_reason_source_slot_release;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_REHOME:
      ++allocator->stats.route_unregister_reason_rehome;
      break;
    case HZ6_ROUTE_UNREGISTER_REASON_UNKNOWN:
    default:
      ++allocator->stats.route_unregister_reason_unknown;
      break;
  }
#else
  (void)allocator;
  (void)reason;
#endif
}

void hz6_allocator_route_unregister_exact_reason(
    Hz6Allocator* allocator,
    void* ptr,
    Hz6RouteUnregisterReason reason) {
  if (!allocator || !ptr) {
    return;
  }
  hz6_allocator_note_route_unregister_reason(allocator, reason);
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  hz6_shared_route_directory_unregister(allocator, ptr);
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  hz6_owner_locality_index_unregister(allocator, ptr);
#endif
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, &probes);
  hz6_allocator_route_note_conditional_tombstone_dryrun(allocator);
  hz6_allocator_route_maybe_compact_tombstones(allocator);
  allocator->stats.route_unregister_probe_total += probes;
  hz6_allocator_note_route_probe_hist(
      allocator->stats.route_unregister_probe_hist, probes);
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
  hz6_allocator_note_route_tombstones(allocator);
#else
  hz6_route_backend_unregister_exact(&allocator->route_backend, ptr, NULL);
  hz6_allocator_route_note_conditional_tombstone_dryrun(allocator);
  hz6_allocator_route_maybe_compact_tombstones(allocator);
#endif
}

void hz6_allocator_route_unregister_exact(Hz6Allocator* allocator,
                                          void* ptr) {
  hz6_allocator_route_unregister_exact_reason(
      allocator, ptr, HZ6_ROUTE_UNREGISTER_REASON_UNKNOWN);
}

int hz6_allocator_route_register_exact_reason(
    Hz6Allocator* allocator,
    void* base,
    size_t bytes,
    uint16_t front_id,
    uint16_t class_id,
    uint32_t generation,
    void* descriptor,
    Hz6RouteRegisterReason reason) {
  if (!allocator || !base || bytes == 0) {
    return 0;
  }
  int ok = 0;
  hz6_allocator_note_route_register_reason(allocator, reason);
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                        base,
                                        bytes,
                                        front_id,
                                        class_id,
                                        generation,
                                        descriptor,
                                        &probes);
  allocator->stats.route_register_probe_total += probes;
  hz6_allocator_note_route_probe_hist(
      allocator->stats.route_register_probe_hist, probes);
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
  hz6_allocator_note_route_tombstones(allocator);
#else
  ok = hz6_route_backend_register_exact(&allocator->route_backend,
                                        base,
                                        bytes,
                                        front_id,
                                        class_id,
                                        generation,
                                        descriptor,
                                        NULL);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  if (ok) {
    (void)hz6_shared_route_directory_register(allocator,
                                              base,
                                              front_id,
                                              class_id,
                                              generation,
                                              descriptor);
  }
#if HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  if (!ok) {
    ok = hz6_shared_route_directory_register(allocator,
                                             base,
                                             front_id,
                                             class_id,
                                             generation,
                                             descriptor);
#if HZ6_DIAGNOSTIC_PROBES
    if (ok) {
      ++allocator->stats.elastic_route_overflow_register;
    } else {
      ++allocator->stats.elastic_route_overflow_register_fail;
    }
#endif
  }
#endif
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (ok) {
    hz6_owner_locality_index_register(allocator, base, generation);
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  if (!ok) {
    ++allocator->stats.route_register_fail;
  }
#endif
  return ok;
}

int hz6_allocator_route_register_exact(Hz6Allocator* allocator,
                                       void* base,
                                       size_t bytes,
                                       uint16_t front_id,
                                       uint16_t class_id,
                                       uint32_t generation,
                                       void* descriptor) {
  return hz6_allocator_route_register_exact_reason(
      allocator, base, bytes, front_id, class_id, generation, descriptor,
      HZ6_ROUTE_REGISTER_REASON_UNKNOWN);
}

int hz6_allocator_route_replace_exact_descriptor(
    Hz6Allocator* allocator,
    void* base,
    size_t bytes,
    uint16_t front_id,
    uint16_t class_id,
    uint32_t old_generation,
    void* old_descriptor,
    uint32_t new_generation,
    void* new_descriptor) {
  if (!allocator || !base || bytes == 0 || !old_descriptor ||
      !new_descriptor) {
    return 0;
  }
  int ok = 0;
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
  ok = hz6_route_backend_replace_exact_descriptor(
      &allocator->route_backend, base, bytes, front_id, class_id,
      old_generation, old_descriptor, new_generation, new_descriptor,
      &probes);
  allocator->stats.route_register_probe_total += probes;
  hz6_allocator_note_route_probe_hist(
      allocator->stats.route_register_probe_hist, probes);
  if (probes > allocator->stats.route_register_probe_max) {
    allocator->stats.route_register_probe_max = probes;
  }
#else
  ok = hz6_route_backend_replace_exact_descriptor(
      &allocator->route_backend, base, bytes, front_id, class_id,
      old_generation, old_descriptor, new_generation, new_descriptor, NULL);
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_L1
  if (!ok) {
    Hz6RouteResult shared_route = hz6_shared_route_directory_lookup_raw(base);
    if (shared_route.kind == HZ6_ROUTE_VALID &&
        shared_route.descriptor == old_descriptor &&
        shared_route.generation == old_generation &&
        shared_route.front_id == front_id &&
        shared_route.class_id == class_id) {
      ok = hz6_shared_route_directory_register(allocator,
                                               base,
                                               front_id,
                                               class_id,
                                               new_generation,
                                               new_descriptor);
    }
  } else {
    (void)hz6_shared_route_directory_register(allocator,
                                              base,
                                              front_id,
                                              class_id,
                                              new_generation,
                                              new_descriptor);
  }
#endif
#if HZ6_OWNER_LOCALITY_INDEX_L1
  if (ok) {
    hz6_owner_locality_index_register(allocator, base, new_generation);
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  if (!ok) {
    ++allocator->stats.route_register_fail;
  }
#endif
  return ok;
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
