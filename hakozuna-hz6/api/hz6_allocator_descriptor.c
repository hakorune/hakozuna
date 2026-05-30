#include "hz6_allocator.h"

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
#endif
  size_t start = allocator->next_descriptor_index;
  if (start >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    start = 0;
  }
  for (size_t offset = 0; offset < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++offset) {
#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    size_t i = start + offset;
    if (i >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
      i -= HZ6_OBJECT_DESCRIPTOR_CAPACITY;
    }
    if (!allocator->descriptors[i].ptr) {
      allocator->next_descriptor_index = i + 1;
      if (allocator->next_descriptor_index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
        allocator->next_descriptor_index = 0;
      }
#if HZ6_DIAGNOSTIC_PROBES
      allocator->stats.descriptor_probe_total += probes;
      if (probes > allocator->stats.descriptor_probe_max) {
        allocator->stats.descriptor_probe_max = probes;
      }
#endif
      return &allocator->descriptors[i];
    }
  }
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.descriptor_probe_total += probes;
  if (probes > allocator->stats.descriptor_probe_max) {
    allocator->stats.descriptor_probe_max = probes;
  }
#endif
  ++allocator->stats.descriptor_exhausted;
  return NULL;
}

int hz6_allocator_activate_descriptor(Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner) {
  if (!descriptor || descriptor->state != expected) {
    return 0;
  }
  if (descriptor->ptr != ptr || descriptor->generation != generation) {
    return 0;
  }
  descriptor->state = HZ6_STATE_ACTIVE;
  descriptor->owner = owner;
  return 1;
}
