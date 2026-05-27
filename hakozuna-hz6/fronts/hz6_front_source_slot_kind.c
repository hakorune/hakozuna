#include "hz6_front_source.h"

#include "hz6_front_util.h"
#include "../source/hz6_source.h"

void* hz6_front_source_slot_kind(Hz6Allocator* allocator,
                                 uint16_t front_id,
                                 uint16_t class_id,
                                 size_t user_bytes,
                                 size_t source_bytes,
                                 size_t source_offset,
                                 Hz6SourceKind source_kind) {
  if (!allocator) {
    return NULL;
  }
  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  return hz6_front_source_slot_ops(allocator, front_id, class_id, user_bytes,
                                   source_bytes, source_offset, source_ops,
                                   source_kind);
}
