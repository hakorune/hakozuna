#include "hz6_front_source.h"

#include "hz6_front_source_prefill.h"
#include "hz6_front_util.h"
#include "../source/hz6_source.h"

void* hz6_front_reuse_or_source(Hz6Allocator* allocator,
                                uint16_t front_id,
                                uint16_t class_id,
                                size_t bytes) {
  return hz6_front_reuse_or_source_kind(allocator, front_id, class_id, bytes,
                                        HZ6_SOURCE_SYSTEM);
}

void* hz6_front_reuse_or_source_kind(Hz6Allocator* allocator,
                                     uint16_t front_id,
                                     uint16_t class_id,
                                     size_t bytes,
                                     Hz6SourceKind source_kind) {
  if (!allocator) {
    return NULL;
  }
  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  return hz6_front_reuse_or_source_ops(allocator, front_id, class_id, bytes,
                                       source_ops, source_kind);
}

void* hz6_front_reuse_or_prefill_source_kind(Hz6Allocator* allocator,
                                             uint16_t front_id,
                                             uint16_t class_id,
                                             size_t bytes,
                                             Hz6SourceKind source_kind,
                                             size_t count) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || bytes == 0) {
    return NULL;
  }

  void* reused = hz6_allocator_profile_transfer_first(allocator)
                     ? hz6_front_reuse_transfer_or_cached(allocator, class_id)
                     : hz6_front_reuse_cached_or_transfer(allocator,
                                                          class_id);
  if (reused) {
    return reused;
  }

  size_t refill_count = count ? count : 1;
  if (hz6_front_prefill_source_kind(allocator, front_id, class_id, bytes,
                                    source_kind, refill_count) == 0) {
    return hz6_front_reuse_or_source_kind(allocator, front_id, class_id,
                                          bytes, source_kind);
  }

  return hz6_allocator_profile_transfer_first(allocator)
             ? hz6_front_reuse_transfer_or_cached(allocator, class_id)
             : hz6_front_reuse_cached_or_transfer(allocator, class_id);
}
