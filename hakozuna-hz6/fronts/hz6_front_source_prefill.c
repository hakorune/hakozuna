#include "hz6_front_source_prefill.h"

size_t hz6_front_prefill_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind,
                                     size_t count) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0 ||
      count == 0) {
    return 0;
  }

  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return 0;
  }

  size_t filled = 0;
  while (filled < count &&
         hz6_allocator_frontcache_count(allocator, class_id) <
             hz6_allocator_frontcache_capacity(allocator, class_id)) {
    if (!hz6_front_prefill_one(allocator, front_id, class_id, bytes,
                               source_ops, source_kind)) {
      break;
    }
    ++filled;
  }
  return filled;
}
