#ifndef HZ6_H
#define HZ6_H

#include "hz6_contract.h"

#include <stddef.h>

#include "hz6_stats_snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6Allocator Hz6Allocator;

void hz6_allocator_init(Hz6Allocator* allocator);

void hz6_allocator_destroy(Hz6Allocator* allocator);

void* hz6_malloc(Hz6Allocator* allocator, size_t size);

void* hz6_allocator_preload_toy_malloc_direct_class(Hz6Allocator* allocator,
                                                    size_t size);

void* hz6_allocator_preload_midpage_malloc_skip_transfer(Hz6Allocator* allocator,
                                                         size_t size);

void* hz6_allocator_preload_midpage_malloc_class_skip_transfer(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t size);

void hz6_free(Hz6Allocator* allocator, void* ptr);

int hz6_free_remote(Hz6Allocator* allocator, void* ptr);

int hz6_owns(Hz6Allocator* allocator, const void* ptr);

Hz6StatsSnapshot hz6_stats_snapshot(const Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
