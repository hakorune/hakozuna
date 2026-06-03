#include "hz6_allocator.h"

#if HZ6_THIN_DESCRIPTOR_L1
static Hz6DescriptorColdSource* hz6_allocator_descriptor_cold_record(
    const Hz6ObjectDescriptor* descriptor) {
  if (!descriptor || !descriptor->allocator ||
      descriptor->cold_index == UINT32_MAX ||
      descriptor->cold_index >= HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY) {
    return NULL;
  }
  Hz6DescriptorColdSource* cold =
      &descriptor->allocator->descriptor_cold_sources[descriptor->cold_index];
  if (!cold->active || cold->generation != descriptor->generation) {
    return NULL;
  }
  return cold;
}

static void hz6_allocator_descriptor_cold_release(
    Hz6ObjectDescriptor* descriptor) {
  Hz6DescriptorColdSource* cold =
      hz6_allocator_descriptor_cold_record(descriptor);
  if (!cold) {
    if (descriptor) {
      descriptor->cold_index = UINT32_MAX;
    }
    return;
  }
  cold->active = 0;
  cold->source_ptr = NULL;
  cold->source_release = NULL;
  cold->source_bytes = 0;
  cold->generation = 0;
  cold->source_kind = HZ6_SOURCE_NONE;
  if (descriptor->allocator &&
      descriptor->allocator->descriptor_cold_source_current != 0) {
    --descriptor->allocator->descriptor_cold_source_current;
  }
  descriptor->cold_index = UINT32_MAX;
}
#endif

int hz6_allocator_descriptor_source_meta(
    const Hz6ObjectDescriptor* descriptor,
    void** source_ptr,
    size_t* source_bytes,
    Hz6SourceReleaseFn* source_release) {
  if (!descriptor || !descriptor->ptr) {
    return 0;
  }
  if (descriptor->source_block) {
    if (source_ptr) {
      *source_ptr = descriptor->source_block->ptr;
    }
    if (source_bytes) {
      *source_bytes = descriptor->source_block->bytes;
    }
    if (source_release) {
      *source_release = descriptor->source_block->source_release;
    }
    return 1;
  }
#if HZ6_THIN_DESCRIPTOR_L1
  Hz6DescriptorColdSource* cold =
      hz6_allocator_descriptor_cold_record(descriptor);
  if (!cold) {
    return 0;
  }
  if (source_ptr) {
    *source_ptr = cold->source_ptr;
  }
  if (source_bytes) {
    *source_bytes = cold->source_bytes;
  }
  if (source_release) {
    *source_release = cold->source_release;
  }
  return 1;
#else
  if (source_ptr) {
    *source_ptr = descriptor->source_ptr ? descriptor->source_ptr
                                         : descriptor->ptr;
  }
  if (source_bytes) {
    *source_bytes = descriptor->source_bytes ? descriptor->source_bytes
                                             : descriptor->bytes;
  }
  if (source_release) {
    *source_release = descriptor->source_release;
  }
  return 1;
#endif
}

int hz6_allocator_descriptor_has_source_release(
    const Hz6ObjectDescriptor* descriptor) {
  Hz6SourceReleaseFn release = NULL;
  return hz6_allocator_descriptor_source_meta(descriptor, NULL, NULL,
                                              &release) &&
         release != NULL;
}

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

#if HZ6_THIN_DESCRIPTOR_L1
  hz6_allocator_descriptor_cold_release(descriptor);
#endif
  descriptor->ptr = NULL;
  descriptor->bytes = 0;
#if !HZ6_THIN_DESCRIPTOR_L1
  descriptor->source_ptr = NULL;
  descriptor->source_bytes = 0;
#endif
  descriptor->source_block = NULL;
  descriptor->class_id = 0;
  descriptor->source_kind = HZ6_SOURCE_NONE;
#if !HZ6_THIN_DESCRIPTOR_L1
  descriptor->source_release = NULL;
#endif
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
    void* source_ptr = NULL;
    size_t source_bytes = 0;
    Hz6SourceReleaseFn source_release = NULL;
    if (!hz6_allocator_descriptor_source_meta(
            descriptor, &source_ptr, &source_bytes, &source_release)) {
      return 0;
    }
    if (source_release) {
      released = source_release(source_ptr, source_bytes);
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
#if !HZ6_THIN_DESCRIPTOR_L1
  descriptor->source_ptr = NULL;
  descriptor->source_bytes = 0;
#endif
  descriptor->source_block = NULL;
  descriptor->class_id = 0;
  descriptor->source_kind = HZ6_SOURCE_NONE;
#if !HZ6_THIN_DESCRIPTOR_L1
  descriptor->source_release = NULL;
#endif
  descriptor->owner = (Hz6OwnerToken){0};
  descriptor->generation = 0;
  descriptor->state = HZ6_STATE_DESCRIPTOR_RESERVED;
  descriptor->allocator = allocator;
  return 1;
}
