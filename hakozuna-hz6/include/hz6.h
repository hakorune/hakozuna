#ifndef HZ6_H
#define HZ6_H

#include "hz6_contract.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6Allocator Hz6Allocator;

typedef struct Hz6StatsSnapshot {
  size_t route_valid;
  size_t route_invalid;
  size_t route_miss;
  size_t transfer_push;
  size_t transfer_pop;
  size_t source_alloc;
  size_t large_span_central_push;
  size_t large_span_central_pop;
  size_t large_span_source_alloc;
} Hz6StatsSnapshot;

void hz6_allocator_init(Hz6Allocator* allocator);

void hz6_allocator_destroy(Hz6Allocator* allocator);

void* hz6_malloc(Hz6Allocator* allocator, size_t size);

void hz6_free(Hz6Allocator* allocator, void* ptr);

int hz6_free_remote(Hz6Allocator* allocator, void* ptr);

int hz6_owns(Hz6Allocator* allocator, const void* ptr);

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
