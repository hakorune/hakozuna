#include "hz6_allocator.h"

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
  if (!allocator || !descriptor || !ptr || bytes == 0 ||
      source_kind == HZ6_SOURCE_NONE) {
    return 0;
  }

  descriptor->ptr = ptr;
  descriptor->bytes = bytes;
  descriptor->source_ptr = source_ptr ? source_ptr : ptr;
  descriptor->source_bytes = source_bytes ? source_bytes : bytes;
  descriptor->source_block = source_block;
  descriptor->class_id = class_id;
  descriptor->source_kind = source_kind;
  descriptor->source_release = source_release;
  descriptor->owner = allocator->owner.token;
  descriptor->generation = 1;
  descriptor->state = state;
#if HZ6_DIAGNOSTIC_PROBES
  if (source_release) {
    ++allocator->stats.source_owned_prepare;
  }
#endif
  return 1;
}
