#include "hz6_allocator.h"

#if HZ6_THIN_DESCRIPTOR_L1
static int hz6_allocator_descriptor_cold_alloc(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* source_ptr,
    size_t source_bytes,
    Hz6SourceKind source_kind,
    Hz6SourceReleaseFn source_release) {
  if (!allocator || !descriptor || !source_ptr || source_bytes == 0 ||
      source_bytes > (size_t)UINT32_MAX ||
      source_kind == HZ6_SOURCE_NONE || source_kind > UINT8_MAX) {
    return 0;
  }
  for (size_t offset = 0; offset < HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY;
       ++offset) {
    size_t index = allocator->next_descriptor_cold_index + offset;
    if (index >= HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY) {
      index -= HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY;
    }
    Hz6DescriptorColdSource* cold = &allocator->descriptor_cold_sources[index];
    if (cold->active) {
      continue;
    }
    cold->source_ptr = source_ptr;
    cold->source_release = source_release;
    cold->source_bytes = (uint32_t)source_bytes;
    cold->generation = descriptor->generation;
    cold->source_kind = (uint8_t)source_kind;
    cold->active = 1;
    descriptor->cold_index = (uint32_t)index;
    allocator->next_descriptor_cold_index = index + 1;
    if (allocator->next_descriptor_cold_index >=
        HZ6_DESCRIPTOR_COLD_SOURCE_CAPACITY) {
      allocator->next_descriptor_cold_index = 0;
    }
    ++allocator->descriptor_cold_source_current;
    if (allocator->descriptor_cold_source_current >
        allocator->descriptor_cold_source_max) {
      allocator->descriptor_cold_source_max =
          allocator->descriptor_cold_source_current;
    }
    return 1;
  }
  return 0;
}
#endif

int hz6_allocator_prepare_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    size_t bytes,
    void* source_ptr,
    size_t source_bytes,
    Hz6SourceBlock* source_block,
    uint16_t class_id,
    Hz6SourceKind source_kind,
    int (*source_release)(void* ptr, size_t bytes),
    Hz6ObjectState state) {
  size_t actual_source_bytes = source_bytes ? source_bytes : bytes;
  if (!allocator || !descriptor || !ptr || bytes == 0 ||
      bytes > (size_t)UINT32_MAX ||
      actual_source_bytes > (size_t)UINT32_MAX ||
      source_kind == HZ6_SOURCE_NONE || source_kind > UINT8_MAX ||
      state > UINT8_MAX) {
    return 0;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = (uint32_t)bytes;
#if HZ6_THIN_DESCRIPTOR_L1
  descriptor->cold_index = UINT32_MAX;
#else
  descriptor->source_ptr = source_ptr ? source_ptr : ptr;
  descriptor->source_bytes = (uint32_t)actual_source_bytes;
#endif
  descriptor->source_block = source_block;
  descriptor->class_id = class_id;
  descriptor->source_kind = (uint8_t)source_kind;
#if !HZ6_THIN_DESCRIPTOR_L1
  descriptor->source_release = source_release;
#endif
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = (uint8_t)state;
#if HZ6_THIN_DESCRIPTOR_L1
  if (!source_block &&
      !hz6_allocator_descriptor_cold_alloc(
          allocator, descriptor, source_ptr ? source_ptr : ptr,
          actual_source_bytes, source_kind, source_release)) {
    hz6_allocator_reset_descriptor_available(allocator, descriptor);
    return 0;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  if (source_release) {
    ++allocator->stats.source_owned_prepare;
  }
#endif
  return 1;
}
