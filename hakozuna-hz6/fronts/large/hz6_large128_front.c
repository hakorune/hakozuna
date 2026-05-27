#include "hz6_large128_front.h"

#include "../../source/hz6_source.h"

static int hz6_large128_can_allocate(size_t size,
                                     size_t align,
                                     uint16_t* class_id) {
  if (!class_id || align > 16 || size <= 4096 || size > HZ6_LARGE128_BYTES) {
    return 0;
  }
  *class_id = HZ6_LARGE128_CLASS_ID;
  return 1;
}

static void* hz6_large128_alloc(Hz6Allocator* allocator,
                                uint16_t class_id,
                                size_t size) {
  (void)size;
  if (!allocator || class_id != HZ6_LARGE128_CLASS_ID) {
    return NULL;
  }

  Hz6FrontCacheEntry entry;
  if (hz6_frontcache_pop(&allocator->frontcache_bins[class_id], &entry)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)entry.descriptor;
    if (!hz6_allocator_activate_descriptor(
            descriptor, HZ6_STATE_LOCAL_FREE, entry.ptr, entry.generation)) {
      return NULL;
    }
    return entry.ptr;
  }

  if (allocator->profile.transfer_first) {
    Hz6TransferObject transfer;
    while (hz6_transfer_pop(&allocator->transfer_cache, class_id, &transfer)) {
      Hz6ObjectDescriptor* descriptor =
          (Hz6ObjectDescriptor*)transfer.descriptor;
      if (!hz6_allocator_activate_descriptor(
              descriptor, HZ6_STATE_TRANSFER_FREE, transfer.ptr,
              transfer.generation)) {
        continue;
      }
      ++allocator->stats.transfer_pop;
      return transfer.ptr;
    }
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    return NULL;
  }

  void* ptr = hz6_source_system_alloc(HZ6_LARGE128_BYTES);
  if (!ptr) {
    return NULL;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = HZ6_LARGE128_BYTES;
  descriptor->class_id = class_id;
  descriptor->generation = 1;
  descriptor->state = HZ6_STATE_ACTIVE;
  if (!hz6_route_register_exact(&allocator->route_table, ptr,
                                HZ6_LARGE128_BYTES, HZ6_FRONT_LARGE,
                                class_id, descriptor->generation,
                                descriptor)) {
    hz6_source_system_free(ptr);
    descriptor->ptr = NULL;
    descriptor->state = HZ6_STATE_DEAD;
    return NULL;
  }

  ++allocator->stats.source_alloc;
  return ptr;
}

static int hz6_large128_free_local(Hz6Allocator* allocator,
                                   void* ptr,
                                   Hz6RouteResult route) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != HZ6_LARGE128_CLASS_ID) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  descriptor->state = HZ6_STATE_LOCAL_FREE;
  Hz6FrontCacheEntry entry;
  entry.ptr = ptr;
  entry.descriptor = descriptor;
  entry.class_id = descriptor->class_id;
  entry.generation = descriptor->generation;
  if (hz6_frontcache_push(&allocator->frontcache_bins[entry.class_id],
                          entry)) {
    return 1;
  }

  descriptor->state = HZ6_STATE_DEAD;
  hz6_route_unregister_exact(&allocator->route_table, ptr);
  hz6_source_system_free(ptr);
  descriptor->ptr = NULL;
  return 1;
}

static int hz6_large128_free_remote(Hz6Allocator* allocator,
                                    void* ptr,
                                    Hz6RouteResult route) {
  if (!allocator || !ptr || route.kind != HZ6_ROUTE_VALID ||
      !route.descriptor || route.class_id != HZ6_LARGE128_CLASS_ID) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = (Hz6ObjectDescriptor*)route.descriptor;
  if (descriptor->state != HZ6_STATE_ACTIVE || descriptor->ptr != ptr) {
    return 0;
  }

  Hz6TransferObject object;
  object.ptr = ptr;
  object.descriptor = descriptor;
  object.class_id = descriptor->class_id;
  object.generation = descriptor->generation;
  descriptor->state = HZ6_STATE_TRANSFER_FREE;
  if (!hz6_transfer_push(&allocator->transfer_cache, object)) {
    descriptor->state = HZ6_STATE_ACTIVE;
    return 0;
  }

  ++allocator->stats.transfer_push;
  return 1;
}

const Hz6FrontOps* hz6_large128_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_LARGE,
      "large128",
      hz6_large128_can_allocate,
      hz6_large128_alloc,
      hz6_large128_free_local,
      hz6_large128_free_remote,
  };
  return &ops;
}
