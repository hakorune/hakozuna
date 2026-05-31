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
  size_t frontcache_reuse_hit;
  size_t frontcache_reuse_invalid;
  size_t transfer_reuse_hit;
  size_t transfer_reuse_invalid;
  size_t source_refill_starvation;
  size_t source_refill_saturation;
  size_t source_refill_boost;
  size_t source_refill_clamp;
  size_t alloc_fail;
  size_t descriptor_exhausted;
  size_t route_register_fail;
  size_t source_block_exhausted;
  size_t route_active_current;
  size_t route_active_max;
  size_t descriptor_probe_total;
  size_t descriptor_probe_max;
  size_t route_register_probe_total;
  size_t route_register_probe_max;
  size_t route_unregister_probe_total;
  size_t route_unregister_probe_max;
  size_t source_block_probe_total;
  size_t source_block_probe_max;
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
