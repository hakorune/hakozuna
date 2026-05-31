#ifndef HZ6_ROUTE_BACKEND_H
#define HZ6_ROUTE_BACKEND_H

#include "../include/hz6_config.h"
#include "hz6_route.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6RouteBackendKind {
  HZ6_ROUTE_BACKEND_EXACT_TABLE = 1,
  HZ6_ROUTE_BACKEND_PAGE_TABLE = 2
} Hz6RouteBackendKind;

typedef struct Hz6RouteBackend {
  Hz6RouteBackendKind kind;
  Hz6RouteEntry* exact_entries;
  Hz6RouteTable exact_table;
  size_t page_granularity;
} Hz6RouteBackend;

void hz6_route_backend_init_exact(Hz6RouteBackend* backend,
                                  Hz6RouteEntry* entries,
                                  size_t capacity);

void hz6_route_backend_init_page_table(Hz6RouteBackend* backend,
                                       Hz6RouteEntry* entries,
                                       size_t capacity);

void hz6_route_backend_init_page_table_with_granularity(
    Hz6RouteBackend* backend,
    Hz6RouteEntry* entries,
    size_t capacity,
    size_t page_granularity);

int hz6_route_backend_register_exact(Hz6RouteBackend* backend,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     uint32_t generation,
                                     void* descriptor,
                                     size_t* probe_count);

int hz6_route_backend_register_invalid_range(Hz6RouteBackend* backend,
                                             void* base,
                                             size_t bytes,
                                             uint16_t front_id,
                                             uint16_t class_id,
                                             size_t* probe_count);

void hz6_route_backend_unregister_exact(Hz6RouteBackend* backend,
                                        void* base,
                                        size_t* probe_count);

void hz6_route_backend_unregister_invalid_range(Hz6RouteBackend* backend,
                                                void* base,
                                                size_t* probe_count);

Hz6RouteResult hz6_route_backend_lookup(const Hz6RouteBackend* backend,
                                        const void* ptr);

Hz6RouteResult hz6_route_backend_lookup_probe(const Hz6RouteBackend* backend,
                                              const void* ptr,
                                              size_t* probe_count);

#ifdef __cplusplus
}
#endif

#endif
