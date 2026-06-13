#ifndef HZ6_ALLOCATOR_TOY_SMALL_DIAG_H
#define HZ6_ALLOCATOR_TOY_SMALL_DIAG_H

#include "hz6_allocator.h"

#include "../fronts/hz6_front.h"

#include <stdint.h>

static inline int hz6_toy_small_hotpath_diag_is_toy_small(uint16_t front_id,
                                                          uint16_t class_id) {
  return front_id == HZ6_FRONT_TOY &&
         class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
}

static inline void hz6_toy_small_hotpath_diag_malloc_fast_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_fast_attempt;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_fast_hit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_fast_hit;
    ++allocator->stats.toy_small_activate_descriptor;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_front_dispatch(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_front_dispatch;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_route_lookup(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_route_lookup;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_owner_equal(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_owner_equal;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_fast_hit(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_fast_hit;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_cache_push(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_cache_push;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_frontcache_pop(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_frontcache_pop;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_malloc_activate_success(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_malloc_activate_success;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_cache_attempt(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_cache_attempt;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline void hz6_toy_small_hotpath_diag_free_cache_success(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_TOY_SMALL_HOTPATH_DIAG_L1
  if (allocator && hz6_toy_small_hotpath_diag_is_toy_small(front_id,
                                                           class_id)) {
    ++allocator->stats.toy_small_free_cache_success;
  }
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
#endif
}

static inline size_t hz6_toy_small_active_map_index(const void* ptr) {
  uintptr_t key = (uintptr_t)ptr >> 4u;
  key ^= key >> 17u;
  key *= (uintptr_t)0xed5ad4bbU;
  key ^= key >> 11u;
  return (size_t)(key % HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY);
}

#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
static inline void hz6_toy_small_active_map_clear(
    Hz6Allocator* allocator,
    Hz6ToySmallActiveMapEntry* entry) {
  if (!allocator || !entry || !entry->ptr) {
    return;
  }
  entry->ptr = NULL;
  entry->descriptor = NULL;
  entry->generation = 0;
  entry->class_id = 0;
  entry->front_id = 0;
  if (allocator->toy_small_active_map_current > 0) {
    --allocator->toy_small_active_map_current;
  }
}
#endif

static inline void hz6_toy_small_active_map_register(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    void* ptr,
    Hz6ObjectDescriptor* descriptor) {
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  if (!allocator || !ptr || !descriptor ||
      !hz6_toy_small_hotpath_diag_is_toy_small(front_id, class_id)) {
    return;
  }
  size_t base_index = hz6_toy_small_active_map_index(ptr);
#if HZ6_TOY_ACTIVE_MAP_REGISTER_FAST_SLOT_L1
  Hz6ToySmallActiveMapEntry* base_entry =
      &allocator->toy_small_active_map[base_index];
  if (!base_entry->ptr || base_entry->ptr == ptr) {
    if (!base_entry->ptr) {
      ++allocator->toy_small_active_map_current;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.toy_small_active_map_register;
#endif
    base_entry->ptr = ptr;
    base_entry->descriptor = descriptor;
    base_entry->generation = descriptor->generation;
    base_entry->class_id = class_id;
    base_entry->front_id = front_id;
    return;
  }
#endif
  Hz6ToySmallActiveMapEntry* entry = NULL;
  Hz6ToySmallActiveMapEntry* first_empty = NULL;
  int saw_collision = 0;
#if HZ6_TOY_ACTIVE_MAP_REGISTER_FAST_SLOT_L1
  saw_collision = 1;
  for (size_t probe = 1; probe < HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT;
       ++probe) {
#else
  for (size_t probe = 0; probe < HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT;
       ++probe) {
#endif
    size_t index =
        (base_index + probe) % HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY;
    Hz6ToySmallActiveMapEntry* candidate =
        &allocator->toy_small_active_map[index];
    if (candidate->ptr == ptr) {
      entry = candidate;
      break;
    }
    if (!candidate->ptr) {
      if (!first_empty) {
        first_empty = candidate;
      }
      continue;
    }
    saw_collision = 1;
  }
  if (!entry) {
    entry = first_empty ? first_empty
                        : &allocator->toy_small_active_map[base_index];
  }
  if (!entry->ptr) {
    ++allocator->toy_small_active_map_current;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.toy_small_active_map_register;
  if (saw_collision) {
    ++allocator->stats.toy_small_active_map_register_collision;
  }
#else
  (void)saw_collision;
#endif
  entry->ptr = ptr;
  entry->descriptor = descriptor;
  entry->generation = descriptor->generation;
  entry->class_id = class_id;
  entry->front_id = front_id;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)ptr;
  (void)descriptor;
#endif
}

static inline int hz6_toy_small_active_map_try_free(Hz6Allocator* allocator,
                                                   void* ptr) {
#if HZ6_TOY_SMALL_ACTIVE_FREE_MAP_L1
  if (!allocator || !ptr) {
    return 0;
  }
  if (allocator->toy_small_active_map_current == 0) {
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.toy_small_active_map_free_attempt;
#endif
  size_t base_index = hz6_toy_small_active_map_index(ptr);
  Hz6ToySmallActiveMapEntry* entry = NULL;
#if HZ6_TOY_ACTIVE_MAP_FREE_FAST_SLOT_L1
  Hz6ToySmallActiveMapEntry* base_entry =
      &allocator->toy_small_active_map[base_index];
  if (base_entry->ptr == ptr) {
    entry = base_entry;
  } else {
    for (size_t probe = 1; probe < HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT;
         ++probe) {
      size_t index =
          (base_index + probe) % HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY;
      Hz6ToySmallActiveMapEntry* candidate =
          &allocator->toy_small_active_map[index];
      if (candidate->ptr == ptr) {
        entry = candidate;
        break;
      }
    }
  }
#else
  for (size_t probe = 0; probe < HZ6_TOY_SMALL_ACTIVE_FREE_MAP_PROBE_LIMIT;
       ++probe) {
    size_t index =
        (base_index + probe) % HZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY;
    Hz6ToySmallActiveMapEntry* candidate =
        &allocator->toy_small_active_map[index];
    if (candidate->ptr == ptr) {
      entry = candidate;
      break;
    }
  }
#endif
  if (!entry) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.toy_small_active_map_free_miss;
#endif
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = entry->descriptor;
  int owner_ok = 1;
#if !HZ6_TOY_SMALL_ACTIVE_MAP_TRUSTED_OWNER_L1
  owner_ok = descriptor &&
             hz6_allocator_descriptor_owner_equal_at(
                 allocator, descriptor, allocator->owner.token,
                 HZ6_OWNER_EQUAL_SITE_FREE);
#endif
  if (!descriptor || descriptor->ptr != ptr ||
      descriptor->generation != entry->generation ||
      descriptor->class_id != entry->class_id ||
      descriptor->state != HZ6_STATE_ACTIVE ||
      !hz6_toy_small_hotpath_diag_is_toy_small(entry->front_id,
                                               entry->class_id) ||
      !owner_ok) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.toy_small_active_map_free_stale;
#endif
    hz6_toy_small_active_map_clear(allocator, entry);
    return 0;
  }

  if (!hz6_allocator_cache_active_descriptor_trusted_owner(allocator,
                                                            descriptor, ptr)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.toy_small_active_map_free_cache_fail;
#endif
    hz6_toy_small_active_map_clear(allocator, entry);
    return 0;
  }

  hz6_toy_small_active_map_clear(allocator, entry);
  ++allocator->stats.route_valid;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.toy_small_active_map_free_hit;
  ++allocator->stats.toy_small_active_map_route_bypass;
#if !HZ6_TOY_SMALL_ACTIVE_MAP_TRUSTED_OWNER_L1
  hz6_toy_small_hotpath_diag_free_owner_equal(
      allocator, HZ6_FRONT_TOY, descriptor->class_id);
#endif
  hz6_toy_small_hotpath_diag_free_fast_hit(
      allocator, HZ6_FRONT_TOY, descriptor->class_id);
  hz6_toy_small_hotpath_diag_free_cache_push(
      allocator, HZ6_FRONT_TOY, descriptor->class_id);
#endif
  return 1;
#else
  (void)allocator;
  (void)ptr;
  return 0;
#endif
}

#endif
