#include "hz6_allocator.h"
#include "hz6_allocator_midpage_active_map.h"
#include "hz6_allocator_same_owner_fast_inline.h"
#include "hz6_allocator_toy_small_diag.h"

#include "../fronts/hz6_front.h"
#include "../fronts/hz6_front_util.h"
#include "../fronts/midpage/hz6_midpage_front.h"
#include "../fronts/toy/hz6_toy_front.h"

#if HZ6_TOY_PRECLASSIFIED_MALLOC_L1
static uint16_t hz6_allocator_toy_class_for_small_size(size_t size) {
  if (size <= 16u) {
    return 0;
  }
  if (size <= 64u) {
    return 1;
  }
  if (size <= 256u) {
    return 2;
  }
  if (size <= 1024u) {
    return 3;
  }
  return 4;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1 || HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static int hz6_allocator_direct_local_alloc_front_eligible(
    uint16_t front_id,
    uint16_t class_id) {
  if (class_id > HZ6_LOCAL_CACHE_DIRECT_MAX_CLASS) {
    return 0;
  }
  return front_id == HZ6_FRONT_TOY || front_id == HZ6_FRONT_MIDPAGE ||
         front_id == HZ6_FRONT_LOCAL2P;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
static int hz6_allocator_direct_local_reuse_pop(Hz6Allocator* allocator,
                                                uint16_t class_id,
                                                Hz6FrontCacheEntry* out) {
#if HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1 && !HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  Hz6FrontCacheBin* bin = &allocator->frontcache_bins[class_id];
  if (!bin->entries || bin->count == 0) {
    return 0;
  }
  *out = bin->entries[--bin->count];
  return 1;
#else
  return hz6_allocator_frontcache_pop(allocator, class_id, out);
#endif
}

static void* hz6_allocator_direct_local_reuse(Hz6Allocator* allocator,
                                              uint16_t class_id,
                                              Hz6ObjectDescriptor**
                                                  out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  while (hz6_allocator_direct_local_reuse_pop(allocator, class_id, &entry)) {
    if (!entry.descriptor) {
      (void)hz6_allocator_frontcache_push(allocator, class_id, entry);
      return NULL;
    }

    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (descriptor->class_id == class_id &&
#if HZ6_LOCAL_CACHE_TRUSTED_OWNER_L1
        hz6_allocator_activate_local_descriptor_trusted_owner(
            allocator, descriptor, entry.ptr, entry.generation)) {
#else
        hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation,
            hz6_allocator_owner_token(allocator))) {
#endif
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.frontcache_reuse_hit;
#endif
      if (out_descriptor) {
        *out_descriptor = descriptor;
      }
      return entry.ptr;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.frontcache_reuse_invalid;
#endif
  }

  return NULL;
}
#endif

#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
static void* hz6_allocator_direct_local_alloc(Hz6Allocator* allocator,
                                              uint16_t front_id,
                                              uint16_t class_id,
                                              Hz6ObjectDescriptor**
                                                  out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  if (!hz6_allocator_direct_local_alloc_front_eligible(front_id, class_id)) {
    return NULL;
  }
  if (hz6_allocator_profile_transfer_first(allocator)) {
    /* Preserve transfer-first semantics before bypassing front dispatch. */
    void* transfer_ptr = hz6_front_reuse_transfer_with_descriptor(
        allocator, front_id, class_id, NULL, out_descriptor);
    if (transfer_ptr) {
      return transfer_ptr;
    }
  }
#if HZ6_LOCAL_CACHE_DIRECT_REUSE_L1
  return hz6_allocator_direct_local_reuse(allocator, class_id,
                                          out_descriptor);
#else
  (void)out_descriptor;
  return hz6_front_reuse_cached_or_transfer(allocator, front_id, class_id,
                                            NULL);
#endif
}
#endif

void* hz6_malloc(Hz6Allocator* allocator, size_t size) {
  if (!allocator) {
    return NULL;
  }
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    return NULL;
  }

  uint16_t class_id = 0;
  const Hz6FrontOps* front = NULL;
#if HZ6_TOY_PRECLASSIFIED_MALLOC_L1
  if (size <= 4096u) {
    class_id = hz6_allocator_toy_class_for_small_size(size);
    front = hz6_toy_front_ops();
  } else
#endif
  {
    front = hz6_front_for_allocation(size, 16, &class_id);
  }
  if (!front || !front->alloc) {
    ++allocator->stats.alloc_fail;
    return NULL;
  }

#if HZ6_SAME_OWNER_FAST_L1 || HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
  Hz6ObjectDescriptor* direct_descriptor = NULL;
  hz6_toy_small_hotpath_diag_malloc_fast_attempt(allocator, front->front_id,
                                                 class_id);
  void* direct_ptr = NULL;
#if HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
  direct_ptr = hz6_allocator_direct_local_alloc(
      allocator, front->front_id, class_id, &direct_descriptor);
#endif
#if HZ6_SAME_OWNER_FAST_L1
  if (!direct_ptr) {
    direct_ptr = hz6_allocator_same_owner_fast_alloc_inline(
        allocator, front->front_id, class_id);
  }
#endif
  if (direct_ptr) {
    if (front->front_id == HZ6_FRONT_MIDPAGE) {
      hz6_midpage_active_map_register(
          allocator, front->front_id, class_id, direct_ptr, direct_descriptor);
    }
    hz6_toy_small_active_map_register(
        allocator, front->front_id, class_id, direct_ptr, direct_descriptor);
    hz6_toy_small_hotpath_diag_malloc_fast_hit(allocator, front->front_id,
                                               class_id);
    return direct_ptr;
  }
#if HZ6_MIDPAGE_PREFILL_DIRECT_REUSE_L1 && HZ6_LOCAL_CACHE_DIRECT_ALLOC_L1
  if (front->front_id == HZ6_FRONT_MIDPAGE &&
      hz6_midpage_prefill_run(allocator, class_id) != 0) {
    direct_descriptor = NULL;
    direct_ptr = hz6_allocator_direct_local_alloc(
        allocator, front->front_id, class_id, &direct_descriptor);
    if (direct_ptr) {
      hz6_midpage_active_map_register(
          allocator, front->front_id, class_id, direct_ptr,
          direct_descriptor);
      hz6_toy_small_hotpath_diag_malloc_fast_hit(allocator, front->front_id,
                                                 class_id);
      return direct_ptr;
    }
  }
#endif
#endif

  hz6_toy_small_hotpath_diag_malloc_front_dispatch(allocator, front->front_id,
                                                   class_id);
  void* ptr = front->alloc(allocator, class_id, size);
  if (!ptr) {
    ++allocator->stats.alloc_fail;
  } else if (front->front_id == HZ6_FRONT_MIDPAGE) {
    hz6_midpage_active_map_register_route(
        allocator, front->front_id, class_id, ptr);
  }
  return ptr;
}
