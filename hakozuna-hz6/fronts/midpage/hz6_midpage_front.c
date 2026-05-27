#include "hz6_midpage_front.h"

#include "../hz6_front_util.h"

int hz6_midpage_class_bytes(uint16_t class_id, size_t* bytes) {
  if (!bytes) {
    return 0;
  }
  switch (class_id) {
    case HZ6_MIDPAGE_8K_CLASS_ID:
      *bytes = HZ6_MIDPAGE_8K_BYTES;
      return 1;
    case HZ6_MIDPAGE_32K_CLASS_ID:
      *bytes = HZ6_MIDPAGE_32K_BYTES;
      return 1;
    default:
      return 0;
  }
}

int hz6_midpage_policy_for_size(size_t size,
                                size_t align,
                                Hz6MidPageRunPolicy* policy) {
  if (!policy || align > 16 || size <= 4096 || size > HZ6_MIDPAGE_BYTES) {
    return 0;
  }

  policy->run_bytes = HZ6_MIDPAGE_RUN_BYTES;
  if (size <= HZ6_MIDPAGE_8K_BYTES) {
    policy->class_id = HZ6_MIDPAGE_8K_CLASS_ID;
    policy->slot_bytes = HZ6_MIDPAGE_8K_BYTES;
  } else {
    policy->class_id = HZ6_MIDPAGE_32K_CLASS_ID;
    policy->slot_bytes = HZ6_MIDPAGE_32K_BYTES;
  }
  policy->slots_per_run = policy->run_bytes / policy->slot_bytes;
  return policy->slots_per_run != 0;
}

static int hz6_midpage_policy_for_class(uint16_t class_id,
                                        Hz6MidPageRunPolicy* policy) {
  size_t bytes = 0;
  if (!policy || !hz6_midpage_class_bytes(class_id, &bytes)) {
    return 0;
  }
  policy->class_id = class_id;
  policy->slot_bytes = bytes;
  policy->run_bytes = HZ6_MIDPAGE_RUN_BYTES;
  policy->slots_per_run = policy->run_bytes / policy->slot_bytes;
  return policy->slots_per_run != 0;
}

size_t hz6_midpage_prefill_run(Hz6Allocator* allocator, uint16_t class_id) {
  Hz6MidPageRunPolicy policy;
  if (!allocator || !hz6_midpage_policy_for_class(class_id, &policy) ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  Hz6FrontCacheBin* bin = &allocator->frontcache_bins[class_id];
  if (bin->count >= bin->capacity) {
    return 0;
  }

  const Hz6OsMemoryOps* source_ops =
      hz6_source_registry_lookup(&allocator->source_registry,
                                 allocator->profile.source_kind);
  Hz6SourceBlock* block = hz6_allocator_create_source_block(
      allocator, policy.run_bytes, source_ops, allocator->profile.source_kind);
  if (!block) {
    return 0;
  }

  size_t filled = 0;
  while (filled < policy.slots_per_run && bin->count < bin->capacity) {
    size_t offset = filled * policy.slot_bytes;
    void* ptr = hz6_front_source_block_slot(
        allocator, HZ6_FRONT_MIDPAGE, class_id, policy.slot_bytes, offset,
        block);
    if (!ptr) {
      break;
    }

    Hz6RouteResult route =
        hz6_route_backend_lookup(&allocator->route_backend, ptr);
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)route.descriptor;
    if (route.kind != HZ6_ROUTE_VALID || !descriptor) {
      break;
    }

    descriptor->state = HZ6_STATE_LOCAL_FREE;
    Hz6FrontCacheEntry entry;
    entry.ptr = ptr;
    entry.descriptor = descriptor;
    entry.class_id = class_id;
    entry.generation = descriptor->generation;
    if (!hz6_frontcache_push(bin, entry)) {
      hz6_route_backend_unregister_exact(&allocator->route_backend, ptr);
      hz6_allocator_release_descriptor_source(descriptor);
      break;
    }

    ++filled;
  }

  if (filled == 0) {
    hz6_allocator_release_source_block(block);
  } else {
    ++allocator->stats.source_alloc;
  }
  return filled;
}

static size_t hz6_midpage_prefill(Hz6Allocator* allocator,
                                  uint16_t class_id,
                                  size_t count) {
  size_t filled = 0;
  while (filled < count) {
    size_t run_filled = hz6_midpage_prefill_run(allocator, class_id);
    if (run_filled == 0) {
      break;
    }
    filled += run_filled;
  }
  return filled;
}

static int hz6_midpage_can_allocate(size_t size,
                                    size_t align,
                                    uint16_t* class_id) {
  Hz6MidPageRunPolicy policy;
  if (!class_id || !hz6_midpage_policy_for_size(size, align, &policy)) {
    return 0;
  }
  *class_id = policy.class_id;
  return 1;
}

static void* hz6_midpage_alloc(Hz6Allocator* allocator,
                               uint16_t class_id,
                               size_t size) {
  (void)size;
  size_t bytes = 0;
  if (!allocator || !hz6_midpage_class_bytes(class_id, &bytes)) {
    return NULL;
  }

  void* reused = hz6_front_reuse_transfer_or_cached(allocator, class_id);
  if (reused) {
    return reused;
  }

  if (hz6_midpage_prefill_run(allocator, class_id) != 0) {
    reused = hz6_front_reuse_transfer_or_cached(allocator, class_id);
    if (reused) {
      return reused;
    }
  }

  return hz6_front_reuse_or_source_kind(
      allocator, HZ6_FRONT_MIDPAGE, class_id, bytes,
      allocator->profile.source_kind);
}

static int hz6_midpage_free_local(Hz6Allocator* allocator,
                                  void* ptr,
                                  Hz6RouteResult route) {
  size_t bytes = 0;
  if (!hz6_midpage_class_bytes(route.class_id, &bytes)) {
    return 0;
  }
  (void)bytes;
  return hz6_front_free_local_to_cache(allocator, ptr, route, route.class_id);
}

static int hz6_midpage_free_remote(Hz6Allocator* allocator,
                                   void* ptr,
                                   Hz6RouteResult route) {
  size_t bytes = 0;
  if (!hz6_midpage_class_bytes(route.class_id, &bytes)) {
    return 0;
  }
  (void)bytes;
  return hz6_front_free_remote_to_transfer(allocator, ptr, route,
                                           route.class_id);
}

const Hz6FrontOps* hz6_midpage_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_MIDPAGE,
      "midpage",
      hz6_midpage_can_allocate,
      hz6_midpage_alloc,
      hz6_midpage_prefill,
      hz6_midpage_free_local,
      hz6_midpage_free_remote,
  };
  return &ops;
}
