#include "hz6_frontcache.h"

void hz6_frontcache_bin_init(Hz6FrontCacheBin* bin,
                             Hz6FrontCacheEntry* entries,
                             size_t capacity) {
  if (!bin) {
    return;
  }
  bin->entries = entries;
  bin->capacity = capacity;
  bin->count = 0;
}

int hz6_frontcache_push(Hz6FrontCacheBin* bin, Hz6FrontCacheEntry entry) {
  if (!bin || !bin->entries || !entry.ptr || bin->count >= bin->capacity) {
    return 0;
  }
  bin->entries[bin->count++] = entry;
  return 1;
}

int hz6_frontcache_pop(Hz6FrontCacheBin* bin, Hz6FrontCacheEntry* out) {
  if (!bin || !bin->entries || !out || bin->count == 0) {
    return 0;
  }
  *out = bin->entries[--bin->count];
  return 1;
}

