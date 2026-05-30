#ifndef HZ6_ROUTE_H
#define HZ6_ROUTE_H

#include "../include/hz6_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6RouteEntry {
  uintptr_t base;
  size_t bytes;
  uint16_t front_id;
  uint16_t class_id;
  uint32_t generation;
  void* descriptor;
  int exact_valid;
  int active;
  int tombstone;
} Hz6RouteEntry;

typedef struct Hz6RouteTable {
  Hz6RouteEntry* entries;
  size_t capacity;
  size_t active_count;
} Hz6RouteTable;

size_t hz6_route_hash_index(uintptr_t base, size_t capacity);

void hz6_route_table_init(Hz6RouteTable* table,
                          Hz6RouteEntry* entries,
                          size_t capacity);

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

#ifdef __cplusplus
}
#endif

#endif
