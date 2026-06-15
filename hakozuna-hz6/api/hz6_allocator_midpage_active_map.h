#ifndef HZ6_ALLOCATOR_MIDPAGE_ACTIVE_MAP_H
#define HZ6_ALLOCATOR_MIDPAGE_ACTIVE_MAP_H

#include "hz6_allocator.h"

#include "../fronts/midpage/hz6_midpage_front.h"

#include <stdint.h>
#include <stdlib.h>

static inline int hz6_midpage_active_map_eligible(uint16_t front_id,
                                                  uint16_t class_id) {
  return front_id == HZ6_FRONT_MIDPAGE &&
         (class_id == HZ6_MIDPAGE_8K_CLASS_ID ||
          class_id == HZ6_MIDPAGE_32K_CLASS_ID);
}

static inline int hz6_midpage_active_map_aligned(const void* ptr) {
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2
  (void)ptr;
  return 1;
#else
  return (((uintptr_t)ptr) & (uintptr_t)(HZ6_MIDPAGE_8K_BYTES - 1u)) == 0u;
#endif
}

static inline size_t hz6_midpage_active_map_wrap(size_t index) {
#if HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1
  return index & (HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY - 1u);
#else
  return index % HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY;
#endif
}

static inline size_t hz6_midpage_active_map_index(const void* ptr) {
#if HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1
  uintptr_t key = (uintptr_t)ptr >> 12u;
#else
  uintptr_t key = (uintptr_t)ptr >> 13u;
#endif
  key ^= key >> 16u;
  key *= (uintptr_t)0x9e3779b1U;
  key ^= key >> 13u;
  return hz6_midpage_active_map_wrap((size_t)key);
}

static inline size_t hz6_midpage_active_map_class_index(const void* ptr,
                                                        uint16_t class_id) {
#if HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1
#if HZ6_MIDPAGE_ACTIVE_MAP_SHIFT12_INDEX_L1
  uintptr_t key = (uintptr_t)ptr >> 12u;
#else
  uintptr_t key = (uintptr_t)ptr >> 13u;
#endif
  key ^= (uintptr_t)class_id * (uintptr_t)0x85ebca6bU;
  key ^= key >> 16u;
  key *= (uintptr_t)0x9e3779b1U;
  key ^= key >> 13u;
  return hz6_midpage_active_map_wrap((size_t)key);
#else
  (void)class_id;
  return hz6_midpage_active_map_index(ptr);
#endif
}

#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
static inline Hz6MidPageActiveMapEntry* hz6_midpage_active_map_entries(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return NULL;
  }
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2
  return allocator->midpage_active_map;
#else
  return allocator->midpage_active_map;
#endif
}

static inline Hz6MidPageActiveMapEntry* hz6_midpage_active_map_ensure(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return NULL;
  }
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2
  if (!allocator->midpage_active_map) {
    allocator->midpage_active_map =
        (Hz6MidPageActiveMapEntry*)calloc(
            HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY,
            sizeof(Hz6MidPageActiveMapEntry));
  }
#endif
  return hz6_midpage_active_map_entries(allocator);
}

static inline void hz6_midpage_active_map_destroy(Hz6Allocator* allocator) {
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2
  if (!allocator || !allocator->midpage_active_map) {
    return;
  }
  free(allocator->midpage_active_map);
  allocator->midpage_active_map = NULL;
  allocator->midpage_active_map_current = 0;
#if HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1
  allocator->midpage_active_map_min_addr = 0;
  allocator->midpage_active_map_max_addr = 0;
#endif
#else
  (void)allocator;
#endif
}

static inline void hz6_midpage_active_map_clear(
    Hz6Allocator* allocator,
    Hz6MidPageActiveMapEntry* entry) {
  if (!allocator || !entry || !entry->ptr) {
    return;
  }
  entry->ptr = NULL;
  entry->descriptor = NULL;
  entry->generation = 0;
  entry->class_id = 0;
  if (allocator->midpage_active_map_current > 0) {
    --allocator->midpage_active_map_current;
  }
}
#endif

static inline void hz6_midpage_active_map_register(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    void* ptr,
    Hz6ObjectDescriptor* descriptor) {
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  if (!allocator || !ptr || !descriptor ||
      !hz6_midpage_active_map_eligible(front_id, class_id) ||
      !hz6_midpage_active_map_aligned(ptr)) {
    return;
  }
  Hz6MidPageActiveMapEntry* entries =
      hz6_midpage_active_map_ensure(allocator);
  if (!entries) {
    return;
  }
  size_t base_index = hz6_midpage_active_map_class_index(ptr, class_id);
#if HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1
  Hz6MidPageActiveMapEntry* base_entry = &entries[base_index];
  if (!base_entry->ptr || base_entry->ptr == ptr) {
    if (!base_entry->ptr) {
      ++allocator->midpage_active_map_current;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_register;
    ++allocator->stats.midpage_active_map_register_base_slot;
    allocator->stats.midpage_active_map_register_probe_total += 1u;
    if (allocator->stats.midpage_active_map_register_probe_max < 1u) {
      allocator->stats.midpage_active_map_register_probe_max = 1u;
    }
#endif
    base_entry->ptr = ptr;
    base_entry->descriptor = descriptor;
    base_entry->generation = descriptor->generation;
    base_entry->class_id = class_id;
    return;
  }
#endif
  Hz6MidPageActiveMapEntry* entry = NULL;
  Hz6MidPageActiveMapEntry* first_empty = NULL;
#if HZ6_DIAGNOSTIC_PROBES || HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1
  Hz6MidPageActiveMapEntry* dryrun_same_class_victim = NULL;
#endif
#if HZ6_DIAGNOSTIC_PROBES
  Hz6MidPageActiveMapEntry* dryrun_stale_victim = NULL;
#endif
  int saw_collision = 0;
  int register_same_ptr = 0;
#if HZ6_DIAGNOSTIC_PROBES
  size_t register_selected_probe = 1u;
  size_t register_first_empty_probe = 0u;
#endif
#if HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1
  saw_collision = 1;
  for (size_t probe = 1; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
       ++probe) {
#else
  for (size_t probe = 0; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
       ++probe) {
#endif
    size_t index = hz6_midpage_active_map_wrap(base_index + probe);
    Hz6MidPageActiveMapEntry* candidate = &entries[index];
    if (candidate->ptr == ptr) {
      entry = candidate;
      register_same_ptr = 1;
#if HZ6_DIAGNOSTIC_PROBES
      register_selected_probe = probe + 1u;
#endif
      break;
    }
    if (!candidate->ptr) {
      if (!first_empty) {
        first_empty = candidate;
#if HZ6_DIAGNOSTIC_PROBES
        register_first_empty_probe = probe + 1u;
#endif
      }
      continue;
    }
#if HZ6_DIAGNOSTIC_PROBES || HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1
    if (!dryrun_same_class_victim && candidate->class_id == class_id) {
      dryrun_same_class_victim = candidate;
    }
#endif
#if HZ6_DIAGNOSTIC_PROBES
    if (!dryrun_stale_victim &&
        (!candidate->descriptor ||
         candidate->descriptor->ptr != candidate->ptr ||
         candidate->descriptor->generation != candidate->generation ||
         candidate->descriptor->class_id != candidate->class_id ||
         candidate->descriptor->state != HZ6_STATE_ACTIVE)) {
      dryrun_stale_victim = candidate;
    }
#endif
    saw_collision = 1;
  }
  if (!entry) {
#if HZ6_MIDPAGE_ACTIVE_MAP_NO_OVERWRITE_FULL_L1
    if (!first_empty) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.midpage_active_map_register;
      allocator->stats.midpage_active_map_register_probe_total +=
          HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
      if (HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT >
          allocator->stats.midpage_active_map_register_probe_max) {
        allocator->stats.midpage_active_map_register_probe_max =
            HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
      }
      if (saw_collision) {
        ++allocator->stats.midpage_active_map_register_collision;
      }
#else
      (void)saw_collision;
#endif
      return;
    }
#endif
    entry = first_empty ? first_empty :
#if HZ6_MIDPAGE_ACTIVE_MAP_SAME_CLASS_VICTIM_L1
          (dryrun_same_class_victim ? dryrun_same_class_victim :
                                      &entries[base_index]);
#else
          &entries[base_index];
#endif
#if HZ6_DIAGNOSTIC_PROBES
    if (first_empty && register_first_empty_probe != 0u) {
      register_selected_probe = register_first_empty_probe;
    } else {
      register_selected_probe = 1u;
    }
#endif
  }
  if (!entry->ptr) {
    ++allocator->midpage_active_map_current;
  }
#if HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1
  uintptr_t ptr_addr = (uintptr_t)ptr;
  if (allocator->midpage_active_map_min_addr == 0 ||
      ptr_addr < allocator->midpage_active_map_min_addr) {
    allocator->midpage_active_map_min_addr = ptr_addr;
  }
  if (ptr_addr > allocator->midpage_active_map_max_addr) {
    allocator->midpage_active_map_max_addr = ptr_addr;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  int register_empty_slot = entry && !entry->ptr;
  uint16_t overwritten_class_id = 0;
  int register_overwrite = entry && entry->ptr && entry->ptr != ptr;
  if (register_overwrite) {
    overwritten_class_id = entry->class_id;
  }
  if (register_overwrite && dryrun_same_class_victim &&
      dryrun_same_class_victim != entry) {
    ++allocator->stats.midpage_active_map_register_overwrite_same_class_alt;
  }
  if (register_overwrite && dryrun_stale_victim &&
      dryrun_stale_victim != entry) {
    ++allocator->stats.midpage_active_map_register_overwrite_stale_alt;
  }
  ++allocator->stats.midpage_active_map_register;
  if (register_selected_probe == 1u) {
    ++allocator->stats.midpage_active_map_register_base_slot;
  }
  allocator->stats.midpage_active_map_register_probe_total +=
      register_selected_probe;
  if (register_selected_probe >
      allocator->stats.midpage_active_map_register_probe_max) {
    allocator->stats.midpage_active_map_register_probe_max =
        register_selected_probe;
  }
  if (class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
    ++allocator->stats.midpage_8k_active_map_register;
  } else if (class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    ++allocator->stats.midpage_32k_active_map_register;
  }
  if (saw_collision) {
    ++allocator->stats.midpage_active_map_register_collision;
  }
  if (register_empty_slot) {
    ++allocator->stats.midpage_active_map_register_empty_slot;
  }
  if (register_same_ptr) {
    ++allocator->stats.midpage_active_map_register_same_ptr;
  }
  if (register_overwrite) {
    ++allocator->stats.midpage_active_map_register_overwrite;
    if (overwritten_class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
      ++allocator->stats.midpage_8k_active_map_register_overwrite;
    } else if (overwritten_class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
      ++allocator->stats.midpage_32k_active_map_register_overwrite;
    }
  }
#else
  (void)saw_collision;
  (void)register_same_ptr;
#endif
  entry->ptr = ptr;
  entry->descriptor = descriptor;
  entry->generation = descriptor->generation;
  entry->class_id = class_id;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)ptr;
  (void)descriptor;
#endif
}

static inline void hz6_midpage_active_map_register_route(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    void* ptr) {
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  if (!allocator || !ptr ||
      !hz6_midpage_active_map_eligible(front_id, class_id) ||
      !hz6_midpage_active_map_aligned(ptr)) {
    return;
  }
  Hz6RouteResult route = hz6_allocator_route_lookup_exact(allocator, ptr);
  if (route.kind != HZ6_ROUTE_VALID || route.route_allocator != allocator ||
      route.front_id != front_id || route.class_id != class_id ||
      !route.descriptor) {
    return;
  }
  hz6_midpage_active_map_register(
      allocator, front_id, class_id, ptr,
      (Hz6ObjectDescriptor*)route.descriptor);
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)ptr;
#endif
}

#if HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1
static inline int hz6_midpage_active_map_cache_trusted_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr) {
  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry = {0};
  entry.ptr = ptr;
  entry.descriptor = descriptor;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
  entry.source_block = descriptor->source_block;
#endif
  entry.generation = descriptor->generation;
  hz6_frontcache_entry_set_bytes(&entry, descriptor->bytes);
  hz6_frontcache_entry_set_class_id(&entry, descriptor->class_id);

  if (hz6_allocator_frontcache_push(allocator, descriptor->class_id, entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_DEAD;
  hz6_allocator_route_unregister_exact_reason(
      allocator, ptr, HZ6_ROUTE_UNREGISTER_REASON_FRONTCACHE_OVERFLOW);
#if HZ6_DIAGNOSTIC_PROBES
  if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
    ++allocator->stats.source_owned_release;
  }
#endif
  hz6_allocator_release_descriptor_source(allocator, descriptor);
  return 1;
}
#endif

static inline int hz6_midpage_active_map_try_free(Hz6Allocator* allocator,
                                                  void* ptr) {
#if HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
  if (!allocator || !ptr || allocator->midpage_active_map_current == 0) {
    return 0;
  }
#if HZ6_MIDPAGE_ACTIVE_MAP_ADDR_ENVELOPE_L1
  uintptr_t ptr_addr = (uintptr_t)ptr;
  if (allocator->midpage_active_map_min_addr != 0 &&
      (ptr_addr < allocator->midpage_active_map_min_addr ||
       ptr_addr > allocator->midpage_active_map_max_addr)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_addr_envelope_skip;
#endif
    return 0;
  }
#endif
  Hz6MidPageActiveMapEntry* entries =
      hz6_midpage_active_map_entries(allocator);
  if (!entries) {
    return 0;
  }
  if (!hz6_midpage_active_map_aligned(ptr)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_alignment_skip;
#endif
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.midpage_active_map_free_attempt;
#endif
  Hz6MidPageActiveMapEntry* entry = NULL;
#if HZ6_DIAGNOSTIC_PROBES
  size_t free_hit_probe = 0u;
#endif
#if HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1
  uint16_t probe_classes[2] = {HZ6_MIDPAGE_32K_CLASS_ID,
                               HZ6_MIDPAGE_8K_CLASS_ID};
  for (size_t class_probe = 0; class_probe < 2 && !entry; ++class_probe) {
    size_t base_index =
        hz6_midpage_active_map_class_index(ptr, probe_classes[class_probe]);
    for (size_t probe = 0; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
         ++probe) {
      size_t index = hz6_midpage_active_map_wrap(base_index + probe);
      Hz6MidPageActiveMapEntry* candidate = &entries[index];
      if (candidate->ptr == ptr) {
        entry = candidate;
#if HZ6_DIAGNOSTIC_PROBES
        free_hit_probe =
            class_probe * HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT + probe + 1u;
#endif
        break;
      }
    }
  }
#else
  size_t base_index = hz6_midpage_active_map_index(ptr);
#if HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1
  Hz6MidPageActiveMapEntry* base_entry = &entries[base_index];
  if (base_entry->ptr == ptr) {
    entry = base_entry;
#if HZ6_DIAGNOSTIC_PROBES
    free_hit_probe = 1u;
#endif
  } else {
    for (size_t probe = 1; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
         ++probe) {
      size_t index = hz6_midpage_active_map_wrap(base_index + probe);
      Hz6MidPageActiveMapEntry* candidate = &entries[index];
      if (candidate->ptr == ptr) {
        entry = candidate;
#if HZ6_DIAGNOSTIC_PROBES
        free_hit_probe = probe + 1u;
#endif
        break;
      }
    }
  }
#else
  for (size_t probe = 0; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
       ++probe) {
    size_t index = hz6_midpage_active_map_wrap(base_index + probe);
    Hz6MidPageActiveMapEntry* candidate = &entries[index];
    if (candidate->ptr == ptr) {
      entry = candidate;
#if HZ6_DIAGNOSTIC_PROBES
      free_hit_probe = probe + 1u;
#endif
      break;
    }
  }
#endif
#endif
  if (!entry) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_free_miss;
    int probe_window_empty = 0;
#if HZ6_MIDPAGE_ACTIVE_MAP_CLASS_INDEX_L1
    for (size_t class_probe = 0; class_probe < 2 && !probe_window_empty;
         ++class_probe) {
      size_t base_index =
          hz6_midpage_active_map_class_index(ptr, probe_classes[class_probe]);
      for (size_t probe = 0; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
           ++probe) {
        size_t index = hz6_midpage_active_map_wrap(base_index + probe);
        if (!entries[index].ptr) {
          probe_window_empty = 1;
          break;
        }
      }
    }
#else
    for (size_t probe = 0; probe < HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT;
         ++probe) {
      size_t index = hz6_midpage_active_map_wrap(base_index + probe);
      if (!entries[index].ptr) {
        probe_window_empty = 1;
        break;
      }
    }
#endif
    if (probe_window_empty) {
      ++allocator->stats.midpage_active_map_free_miss_probe_empty;
    } else {
      ++allocator->stats.midpage_active_map_free_miss_probe_occupied;
    }
    for (size_t index = 0; index < HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY;
         ++index) {
      Hz6MidPageActiveMapEntry* candidate = &entries[index];
      if (candidate->ptr == ptr) {
        ++allocator->stats.midpage_active_map_free_miss_found_elsewhere;
        if (candidate->class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
          ++allocator->stats.midpage_8k_active_map_free_miss_found_elsewhere;
        } else if (candidate->class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
          ++allocator->stats.midpage_32k_active_map_free_miss_found_elsewhere;
        }
        break;
      }
    }
#endif
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = entry->descriptor;
  if (!descriptor || descriptor->ptr != ptr ||
      descriptor->generation != entry->generation ||
      descriptor->class_id != entry->class_id ||
      descriptor->state != HZ6_STATE_ACTIVE ||
      !hz6_midpage_active_map_eligible(HZ6_FRONT_MIDPAGE, entry->class_id)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_free_stale;
#endif
    hz6_midpage_active_map_clear(allocator, entry);
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.midpage_active_map_free_cache_attempt;
#endif
#if HZ6_MIDPAGE_ACTIVE_MAP_TRUSTED_CACHE_PUSH_L1
  if (!hz6_midpage_active_map_cache_trusted_descriptor(allocator, descriptor,
                                                        ptr)) {
#else
  if (!hz6_allocator_cache_active_descriptor_trusted_owner(allocator,
                                                            descriptor, ptr)) {
#endif
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.midpage_active_map_free_cache_fail;
#endif
    hz6_midpage_active_map_clear(allocator, entry);
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.midpage_active_map_free_cache_success;
#endif

  hz6_midpage_active_map_clear(allocator, entry);
  ++allocator->stats.route_valid;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.midpage_active_map_free_hit;
  if (free_hit_probe == 1u) {
    ++allocator->stats.midpage_active_map_free_hit_base_slot;
  }
  allocator->stats.midpage_active_map_free_hit_probe_total += free_hit_probe;
  if (free_hit_probe > allocator->stats.midpage_active_map_free_hit_probe_max) {
    allocator->stats.midpage_active_map_free_hit_probe_max = free_hit_probe;
  }
  if (descriptor->class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
    ++allocator->stats.midpage_8k_active_map_free_hit;
  } else if (descriptor->class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    ++allocator->stats.midpage_32k_active_map_free_hit;
  }
  ++allocator->stats.midpage_active_map_route_bypass;
#endif
  return 1;
#else
  (void)allocator;
  (void)ptr;
  return 0;
#endif
}

#if !HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2
static inline void hz6_midpage_active_map_destroy(Hz6Allocator* allocator) {
  (void)allocator;
}
#endif

#endif
