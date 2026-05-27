#ifndef HZ6_ROUTE_BACKEND_H
#define HZ6_ROUTE_BACKEND_H

#include "hz6_route.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz6RouteBackendKind {
  HZ6_ROUTE_BACKEND_EXACT_TABLE = 1
} Hz6RouteBackendKind;

typedef struct Hz6RouteBackend {
  Hz6RouteBackendKind kind;
  Hz6RouteEntry* exact_entries;
  Hz6RouteTable exact_table;
} Hz6RouteBackend;

void hz6_route_backend_init_exact(Hz6RouteBackend* backend,
                                  Hz6RouteEntry* entries,
                                  size_t capacity);

int hz6_route_backend_register_exact(Hz6RouteBackend* backend,
                                     void* base,
                                     size_t bytes,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     uint32_t generation,
                                     void* descriptor);

void hz6_route_backend_unregister_exact(Hz6RouteBackend* backend,
                                        void* base);

Hz6RouteResult hz6_route_backend_lookup(const Hz6RouteBackend* backend,
                                        const void* ptr);

#ifdef __cplusplus
}
#endif

#endif
