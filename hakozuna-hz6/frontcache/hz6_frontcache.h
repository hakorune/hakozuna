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
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  void* source_block;
#endif
#if HZ6_FRONTCACHE_PACKED_META_L1
  uint32_t generation;
  uint32_t meta;
#else
  size_t bytes;
  uint32_t generation;
  uint16_t class_id;
#if HZ6_DESCRIPTOR_COLD_GOV_L1
  uint8_t descgov_detached;
#endif
#endif
} Hz6FrontCacheEntry;

#if HZ6_FRONTCACHE_PACKED_META_L1
#define HZ6_FRONTCACHE_META_BYTES_SHIFT 0u
#define HZ6_FRONTCACHE_META_CLASS_SHIFT 16u
#define HZ6_FRONTCACHE_META_DETACHED 0x01000000u
#define HZ6_FRONTCACHE_META_BYTES_OVERFLOW 0x02000000u
#define HZ6_FRONTCACHE_META_BYTES_MASK 0xffffu
#define HZ6_FRONTCACHE_META_CLASS_MASK 0xffu

static inline uint16_t hz6_frontcache_entry_class_id(
    const Hz6FrontCacheEntry* entry) {
  return entry ? (uint16_t)((entry->meta >> HZ6_FRONTCACHE_META_CLASS_SHIFT) &
                            HZ6_FRONTCACHE_META_CLASS_MASK)
               : 0;
}

static inline size_t hz6_frontcache_entry_bytes(
    const Hz6FrontCacheEntry* entry) {
  if (!entry) {
    return 0;
  }
  return (size_t)((entry->meta >> HZ6_FRONTCACHE_META_BYTES_SHIFT) &
                  HZ6_FRONTCACHE_META_BYTES_MASK) +
         1u;
}

static inline int hz6_frontcache_entry_descgov_detached(
    const Hz6FrontCacheEntry* entry) {
  return entry ? ((entry->meta & HZ6_FRONTCACHE_META_DETACHED) != 0) : 0;
}

static inline int hz6_frontcache_entry_bytes_overflow(
    const Hz6FrontCacheEntry* entry) {
  return entry ? ((entry->meta & HZ6_FRONTCACHE_META_BYTES_OVERFLOW) != 0) : 1;
}

static inline void hz6_frontcache_entry_set_class_id(
    Hz6FrontCacheEntry* entry,
    uint16_t class_id) {
  if (!entry) {
    return;
  }
  entry->meta &= ~(HZ6_FRONTCACHE_META_CLASS_MASK
                   << HZ6_FRONTCACHE_META_CLASS_SHIFT);
  entry->meta |= (((uint32_t)class_id & HZ6_FRONTCACHE_META_CLASS_MASK)
                  << HZ6_FRONTCACHE_META_CLASS_SHIFT);
}

static inline void hz6_frontcache_entry_set_bytes(Hz6FrontCacheEntry* entry,
                                                  size_t bytes) {
  if (!entry) {
    return;
  }
  entry->meta &= ~(HZ6_FRONTCACHE_META_BYTES_MASK
                   << HZ6_FRONTCACHE_META_BYTES_SHIFT);
  entry->meta &= ~HZ6_FRONTCACHE_META_BYTES_OVERFLOW;
  if (bytes == 0 || bytes > ((size_t)UINT16_MAX + 1u)) {
    entry->meta |= HZ6_FRONTCACHE_META_BYTES_OVERFLOW;
    return;
  }
  entry->meta |= ((uint32_t)(bytes == 0 ? 0u : (bytes - 1u)) &
                  HZ6_FRONTCACHE_META_BYTES_MASK)
                 << HZ6_FRONTCACHE_META_BYTES_SHIFT;
}

static inline void hz6_frontcache_entry_set_descgov_detached(
    Hz6FrontCacheEntry* entry,
    int detached) {
  if (!entry) {
    return;
  }
  if (detached) {
    entry->meta |= HZ6_FRONTCACHE_META_DETACHED;
  } else {
    entry->meta &= ~HZ6_FRONTCACHE_META_DETACHED;
  }
}
#else
static inline uint16_t hz6_frontcache_entry_class_id(
    const Hz6FrontCacheEntry* entry) {
  return entry ? entry->class_id : 0;
}

static inline size_t hz6_frontcache_entry_bytes(
    const Hz6FrontCacheEntry* entry) {
  return entry ? entry->bytes : 0;
}

static inline int hz6_frontcache_entry_descgov_detached(
    const Hz6FrontCacheEntry* entry) {
#if HZ6_DESCRIPTOR_COLD_GOV_L1
  return entry ? entry->descgov_detached != 0 : 0;
#else
  (void)entry;
  return 0;
#endif
}

static inline int hz6_frontcache_entry_bytes_overflow(
    const Hz6FrontCacheEntry* entry) {
  (void)entry;
  return 0;
}

static inline void hz6_frontcache_entry_set_class_id(
    Hz6FrontCacheEntry* entry,
    uint16_t class_id) {
  if (entry) {
    entry->class_id = class_id;
  }
}

static inline void hz6_frontcache_entry_set_bytes(Hz6FrontCacheEntry* entry,
                                                  size_t bytes) {
  if (entry) {
    entry->bytes = bytes;
  }
}

static inline void hz6_frontcache_entry_set_descgov_detached(
    Hz6FrontCacheEntry* entry,
    int detached) {
#if HZ6_DESCRIPTOR_COLD_GOV_L1
  if (entry) {
    entry->descgov_detached = detached ? 1u : 0u;
  }
#else
  (void)entry;
  (void)detached;
#endif
}
#endif

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
