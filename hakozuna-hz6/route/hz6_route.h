#ifndef HZ6_ROUTE_H
#define HZ6_ROUTE_H

#include "../include/hz6_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6RouteEntry {
  uintptr_t base;
  void* descriptor;
  uint32_t bytes;
  uint32_t generation;
  uint16_t front_id;
  uint16_t class_id;
  unsigned exact_valid : 1;
  unsigned active : 1;
  unsigned tombstone : 1;
} Hz6RouteEntry;

typedef struct Hz6RouteTable {
  Hz6RouteEntry* entries;
  size_t capacity;
  size_t active_count;
  size_t tombstone_count;
  size_t register_used_tombstone;
  size_t register_full_probe_with_tombstone;
} Hz6RouteTable;

size_t hz6_route_hash_index(uintptr_t base, size_t capacity);
size_t hz6_route_probe_step(uintptr_t base, size_t capacity);

#if HZ6_ROUTE_LINEAR_WRAP_L1
static inline size_t hz6_route_linear_next_index(size_t index,
                                                 size_t capacity) {
  ++index;
  if (index == capacity) {
    return 0;
  }
  return index;
}
#endif

#if HZ6_ROUTE_DOUBLE_HASH_L1
#define HZ6_ROUTE_PROBE_INDEX(start, step, capacity, probe_index) \
  (((start) + (((probe_index) % (capacity)) * (step))) % (capacity))
#elif HZ6_ROUTE_LINEAR_WRAP_L1
static inline size_t hz6_route_linear_probe_index(size_t start,
                                                  size_t capacity,
                                                  size_t probe_index) {
  size_t index = start + probe_index;
  if (index >= capacity) {
    index -= capacity;
  }
  return index;
}

#define HZ6_ROUTE_PROBE_INDEX(start, step, capacity, probe_index) \
  ((void)(step), hz6_route_linear_probe_index((start), (capacity), \
                                              (probe_index)))
#else
#define HZ6_ROUTE_PROBE_INDEX(start, step, capacity, probe_index) \
  ((void)(step), (((start) + (probe_index)) % (capacity)))
#endif

void hz6_route_table_init(Hz6RouteTable* table,
                          Hz6RouteEntry* entries,
                          size_t capacity);

int hz6_route_table_compact_tombstones(Hz6RouteTable* table,
                                       size_t* moved_count);

int hz6_route_register_exact(Hz6RouteTable* table,
                             void* base,
                             size_t bytes,
                             uint16_t front_id,
                             uint16_t class_id,
                             uint32_t generation,
                             void* descriptor,
                             size_t* probe_count);

int hz6_route_register_invalid_range(Hz6RouteTable* table,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t* probe_count);

void hz6_route_unregister_exact(Hz6RouteTable* table,
                               void* base,
                               size_t* probe_count);

void hz6_route_unregister_invalid_range(Hz6RouteTable* table,
                                       void* base,
                                       size_t* probe_count);

Hz6RouteResult hz6_route_lookup(const Hz6RouteTable* table, const void* ptr);

Hz6RouteResult hz6_route_lookup_exact_probe(const Hz6RouteTable* table,
                                            const void* ptr,
                                            size_t* probe_count);

Hz6RouteResult hz6_route_lookup_exact(const Hz6RouteTable* table,
                                      const void* ptr);

Hz6RouteResult hz6_route_lookup_probe(const Hz6RouteTable* table,
                                      const void* ptr,
                                      size_t* probe_count);

#ifdef __cplusplus
}
#endif

#endif
