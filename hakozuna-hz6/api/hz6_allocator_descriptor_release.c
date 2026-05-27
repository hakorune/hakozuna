#include "hz6_allocator.h"

int hz6_allocator_release_descriptor_source(
    Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->ptr) {
    return 0;
  }

  int released = 0;
  if (descriptor->source_block) {
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
  return released;
}
