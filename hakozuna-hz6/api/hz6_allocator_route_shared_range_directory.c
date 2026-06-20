#include "hz6_allocator_route_shared_directory.h"

#include "hz6_allocator_route_hash.h"

#include <stdatomic.h>
#include <stdint.h>

#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
typedef struct Hz6SharedRouteRangeEntry {
  _Atomic(uintptr_t) base;
  _Atomic(uintptr_t) bytes;
  _Atomic(unsigned int) front_id;
  _Atomic(unsigned int) class_id;
} Hz6SharedRouteRangeEntry;

static Hz6SharedRouteRangeEntry g_hz6_shared_route_ranges
    [HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY];

size_t hz6_shared_route_range_directory_bytes(void) {
  return sizeof(g_hz6_shared_route_ranges);
}

static size_t hz6_shared_route_range_index(uintptr_t base) {
  return hz6_route_directory_index(base, HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY);
}

static int hz6_shared_route_range_register(void* base,
                                           size_t bytes,
                                           uint16_t front_id,
                                           uint16_t class_id) {
  if (!base || bytes == 0) {
    return 0;
  }
  uintptr_t key = (uintptr_t)base;
  size_t start = hz6_shared_route_range_index(key);
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
  size_t start = hz6_shared_route_range_index(key);
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

Hz6RouteResult hz6_shared_route_range_lookup_raw(const void* ptr) {
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
#else
size_t hz6_shared_route_range_directory_bytes(void) {
  return 0;
}

Hz6RouteResult hz6_shared_route_range_lookup_raw(const void* ptr) {
  (void)ptr;
  return hz6_route_miss();
}

int hz6_allocator_route_register_shared_invalid_range(Hz6Allocator* allocator,
                                                      void* base,
                                                      size_t bytes,
                                                      uint16_t front_id,
                                                      uint16_t class_id) {
  (void)allocator;
  (void)base;
  (void)bytes;
  (void)front_id;
  (void)class_id;
  return 0;
}

void hz6_allocator_route_unregister_shared_invalid_range(
    Hz6Allocator* allocator,
    void* base) {
  (void)allocator;
  (void)base;
}
#endif
