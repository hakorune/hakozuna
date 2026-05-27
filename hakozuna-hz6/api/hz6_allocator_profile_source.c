#include "hz6_allocator.h"

const Hz6OsMemoryOps* hz6_allocator_source_ops(
    const Hz6Allocator* allocator,
    Hz6SourceKind source_kind) {
  return allocator ? hz6_source_registry_lookup(&allocator->source_registry,
                                                source_kind)
                   : NULL;
}

void hz6_allocator_note_source_alloc(Hz6Allocator* allocator) {
  if (allocator) {
    ++allocator->stats.source_alloc;
  }
}
