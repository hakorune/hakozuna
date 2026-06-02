#include "hz6_allocator.h"

void hz6_allocator_reset_descriptor_available(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return;
  }
#if HZ6_DESCRIPTOR_AVAIL_COUNT_L1
  int was_available =
      !descriptor->ptr && descriptor->state == HZ6_STATE_DEAD;
#endif
  struct Hz6Allocator* allocator = descriptor->allocator;

  descriptor->ptr = NULL;
  descriptor->bytes = 0;
  descriptor->source_ptr = NULL;
  descriptor->source_bytes = 0;
  descriptor->source_block = NULL;
  descriptor->class_id = 0;
  descriptor->source_kind = HZ6_SOURCE_NONE;
  descriptor->source_release = NULL;
  descriptor->owner = (Hz6OwnerToken){0};
  descriptor->generation = 0;
  descriptor->state = HZ6_STATE_DEAD;
  descriptor->allocator = allocator;

#if HZ6_DESCRIPTOR_AVAIL_COUNT_L1
  if (!was_available && allocator &&
      allocator->descriptor_available_count < HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    ++allocator->descriptor_available_count;
  }
#endif
}

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->ptr) {
    return 0;
  }

  int released = 0;
  if (descriptor->source_block) {
    hz6_allocator_source_run_release_slot(descriptor->source_block,
                                          descriptor->ptr);
    released = hz6_allocator_release_source_block(descriptor->source_block);
  } else {
    void* source_ptr = descriptor->source_ptr ? descriptor->source_ptr
                                              : descriptor->ptr;
    if (descriptor->source_release) {
      released =
          descriptor->source_release(source_ptr, descriptor->source_bytes);
    } else {
      released = hz6_source_system_release(source_ptr, descriptor->bytes);
    }
  }

  hz6_allocator_reset_descriptor_available(descriptor);
  return released;
}

int hz6_allocator_detach_descriptor_keep_source_slot(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->ptr || !descriptor->source_block) {
    return 0;
  }

  hz6_allocator_reset_descriptor_available(descriptor);
  return 1;
}

int hz6_allocator_reserve_descriptor_keep_source_slot(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->ptr || !descriptor->source_block) {
    return 0;
  }

  struct Hz6Allocator* allocator = descriptor->allocator;
  descriptor->ptr = NULL;
  descriptor->bytes = 0;
  descriptor->source_ptr = NULL;
  descriptor->source_bytes = 0;
  descriptor->source_block = NULL;
  descriptor->class_id = 0;
  descriptor->source_kind = HZ6_SOURCE_NONE;
  descriptor->source_release = NULL;
  descriptor->owner = (Hz6OwnerToken){0};
  descriptor->generation = 0;
  descriptor->state = HZ6_STATE_DESCRIPTOR_RESERVED;
  descriptor->allocator = allocator;
  return 1;
}
