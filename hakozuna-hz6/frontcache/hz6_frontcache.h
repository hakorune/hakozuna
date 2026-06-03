#ifndef HZ6_FRONTCACHE_H
#define HZ6_FRONTCACHE_H

#include "../include/hz6_config.h"
#include "../include/hz6_contract.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6FrontCacheEntry {
  void* ptr;
  void* descriptor;
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
  void* reserved_descriptor;
#endif
  void* source_block;
  size_t bytes;
  uint16_t class_id;
  uint32_t generation;
  uint8_t descgov_detached;
} Hz6FrontCacheEntry;

typedef struct Hz6FrontCacheBin {
  Hz6FrontCacheEntry* entries;
  size_t capacity;
  size_t count;
} Hz6FrontCacheBin;

void hz6_frontcache_bin_init(Hz6FrontCacheBin* bin,
                             Hz6FrontCacheEntry* entries,
                             size_t capacity);

int hz6_frontcache_push(Hz6FrontCacheBin* bin, Hz6FrontCacheEntry entry);

int hz6_frontcache_pop(Hz6FrontCacheBin* bin, Hz6FrontCacheEntry* out);

int hz6_frontcache_remove(Hz6FrontCacheBin* bin,
                          void* ptr,
                          void* descriptor,
                          uint32_t generation,
                          Hz6FrontCacheEntry* out);

#ifdef __cplusplus
}
#endif

#endif
