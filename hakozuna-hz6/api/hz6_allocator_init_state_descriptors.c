#include "hz6_allocator_init_internal.h"

void hz6_allocator_init_state_descriptors(Hz6Allocator* allocator) {
  allocator->next_descriptor_index = 0;
  allocator->descriptor_available_count = HZ6_OBJECT_DESCRIPTOR_CAPACITY;
#if HZ6_THIN_DESCRIPTOR_L1
  allocator->next_descriptor_cold_index = 0;
  allocator->descriptor_cold_source_current = 0;
  allocator->descriptor_cold_source_max = 0;
  for (size_t i = 0; i < HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY; ++i) {
    allocator->descriptor_cold_sources[i] = (Hz6DescriptorColdSource){0};
  }
#endif
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
#if !HZ6_DESCRIPTOR_NO_BACKPTR_L1
    allocator->descriptors[i].allocator = allocator;
#endif
    allocator->descriptors[i].ptr = NULL;
    allocator->descriptors[i].bytes = 0;
#if !HZ6_THIN_DESCRIPTOR_L1
    allocator->descriptors[i].source_ptr = NULL;
    allocator->descriptors[i].source_bytes = 0;
#endif
    allocator->descriptors[i].source_block = NULL;
    allocator->descriptors[i].class_id = 0;
    allocator->descriptors[i].source_kind = HZ6_SOURCE_NONE;
#if !HZ6_THIN_DESCRIPTOR_L1
    allocator->descriptors[i].source_release = NULL;
#endif
    allocator->descriptors[i].owner = (Hz6OwnerToken){0};
    allocator->descriptors[i].generation = 0;
#if HZ6_THIN_DESCRIPTOR_L1
    allocator->descriptors[i].cold_index = UINT32_MAX;
#endif
    allocator->descriptors[i].state = HZ6_STATE_DEAD;
  }
}
