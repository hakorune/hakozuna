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
  if (!bin || !bin->entries || !entry.ptr ||
      hz6_frontcache_entry_bytes_overflow(&entry) ||
      bin->count >= bin->capacity) {
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

int hz6_frontcache_remove(Hz6FrontCacheBin* bin,
                          void* ptr,
                          void* descriptor,
                          uint32_t generation,
                          Hz6FrontCacheEntry* out) {
  if (!bin || !bin->entries || !ptr || !descriptor) {
    return 0;
  }

  for (size_t i = 0; i < bin->count; ++i) {
    Hz6FrontCacheEntry entry = bin->entries[i];
    if (entry.ptr != ptr || entry.descriptor != descriptor ||
        entry.generation != generation) {
      continue;
    }

    if (out) {
      *out = entry;
    }
    bin->entries[i] = bin->entries[--bin->count];
    return 1;
  }

  return 0;
}
