#include "hz6_allocator.h"

void hz6_allocator_large_span_pool_init(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    allocator->large_span_pool.bins[i].count = 0;
  }
}

size_t hz6_allocator_large_span_pool_capacity(uint16_t class_id) {
  (void)class_id;
  return HZ6_TRANSFER_CACHE_CAPACITY;
}

size_t hz6_allocator_large_span_pool_count(const Hz6Allocator* allocator,
                                           uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->large_span_pool.bins[class_id].count;
}

int hz6_allocator_large_span_pool_push(Hz6Allocator* allocator,
                                       Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      descriptor->state != HZ6_STATE_CENTRAL_FREE) {
    return 0;
  }

  Hz6LargeSpanPoolBin* bin =
      &allocator->large_span_pool.bins[descriptor->class_id];
  if (bin->count >= HZ6_TRANSFER_CACHE_CAPACITY) {
    return 0;
  }

  bin->descriptors[bin->count++] = descriptor;
  return 1;
}

int hz6_allocator_large_span_pool_pop(Hz6Allocator* allocator,
                                      uint16_t class_id,
                                      Hz6ObjectDescriptor** out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  Hz6LargeSpanPoolBin* bin = &allocator->large_span_pool.bins[class_id];
  if (bin->count == 0) {
    return 0;
  }

  *out = bin->descriptors[--bin->count];
  return 1;
}
