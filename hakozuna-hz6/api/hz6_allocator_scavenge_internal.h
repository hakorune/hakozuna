#ifndef HZ6_ALLOCATOR_SCAVENGE_INTERNAL_H
#define HZ6_ALLOCATOR_SCAVENGE_INTERNAL_H

#include "hz6_allocator.h"

static inline size_t hz6_allocator_scavenge_descriptor_cost(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return 0;
  }
  if (descriptor->source_block) {
    return descriptor->bytes;
  }
  size_t source_bytes = 0;
  if (hz6_allocator_descriptor_source_meta(allocator, descriptor, NULL,
                                           &source_bytes, NULL) &&
      source_bytes != 0) {
    return source_bytes;
  }
  return descriptor->bytes;
}

#endif
