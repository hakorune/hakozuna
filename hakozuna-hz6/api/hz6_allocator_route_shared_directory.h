#ifndef HZ6_ALLOCATOR_ROUTE_SHARED_DIRECTORY_H
#define HZ6_ALLOCATOR_ROUTE_SHARED_DIRECTORY_H

#include "hz6_allocator.h"

int hz6_shared_route_directory_register(Hz6Allocator* allocator,
                                        void* base,
                                        uint16_t front_id,
                                        uint16_t class_id,
                                        uint32_t generation,
                                        void* descriptor);

void hz6_shared_route_directory_unregister(Hz6Allocator* allocator,
                                           void* base);

int hz6_shared_route_directory_transfer_owner(Hz6Allocator* old_allocator,
                                              Hz6Allocator* new_allocator,
                                              void* base,
                                              uint32_t generation,
                                              void* descriptor);

typedef enum Hz6SharedRouteLookupStatus {
  HZ6_SHARED_ROUTE_LOOKUP_MISS = 0,
  HZ6_SHARED_ROUTE_LOOKUP_VALID = 1,
  HZ6_SHARED_ROUTE_LOOKUP_RETRY = 2,
  HZ6_SHARED_ROUTE_LOOKUP_STALE = 3
} Hz6SharedRouteLookupStatus;

typedef struct Hz6SharedRouteLookup {
  Hz6SharedRouteLookupStatus status;
  Hz6RouteResult route;
} Hz6SharedRouteLookup;

Hz6SharedRouteLookup hz6_shared_route_directory_lookup_snapshot(
    const void* ptr);

Hz6RouteResult hz6_shared_route_directory_lookup_raw(const void* ptr);

void hz6_shared_route_directory_diag_tombstone_added(void);

void hz6_shared_route_directory_diag_tombstone_reused(void);

void hz6_shared_route_directory_diag_tombstone_cleared(size_t cleared);

size_t hz6_shared_route_directory_diag_tombstone_current(void);

void hz6_shared_route_directory_diag_maintenance_attempt(void);

void hz6_shared_route_directory_diag_maintenance_success(void);

void hz6_shared_route_directory_note_stats(Hz6StatsSnapshot* snapshot);

void hz6_shared_route_directory_maintain_tombstones(void);

size_t hz6_shared_route_range_directory_bytes(void);

Hz6RouteResult hz6_shared_route_range_lookup_raw(const void* ptr);

#endif
