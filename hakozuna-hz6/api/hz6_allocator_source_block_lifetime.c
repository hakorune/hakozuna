#include "hz6_allocator.h"

int hz6_allocator_retain_source_block(Hz6SourceBlock* block) {
  if (!block || !hz6_source_block_active(block) || !block->ptr) {
    return 0;
  }
  ++block->ref_count;
  return 1;
}

int hz6_allocator_release_source_block(Hz6Allocator* allocator,
                                       Hz6SourceBlock* block) {
  if (!allocator || !block || !hz6_source_block_active(block) ||
      !block->ptr) {
    return 0;
  }

  if (block->ref_count != 0) {
    --block->ref_count;
  }
  if (block->ref_count != 0) {
    return 1;
  }

  if (hz6_source_block_route_registered(block)) {
    hz6_route_backend_unregister_invalid_range(&allocator->route_backend,
                                               block->ptr,
                                               NULL);
  }
  int released = block->source_release
                     ? block->source_release(block->ptr, block->bytes)
                     : hz6_source_system_release(block->ptr, block->bytes);
  block->ptr = NULL;
  block->bytes = 0;
  hz6_source_block_set_source_kind(block, HZ6_SOURCE_NONE);
  block->source_release = NULL;
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
  block->route_backend = NULL;
#endif
  block->ref_count = 0;
  block->run_slot_bytes = 0;
  block->run_class_id = 0;
  block->run_slot_count = 0;
  block->run_used_count = 0;
  block->run_next_hint = 0;
  for (size_t i = 0; i < HZ6_SOURCE_RUN_BITMAP_WORDS; ++i) {
    block->run_used_bits[i] = 0;
  }
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  block->owner_source_storage_allocator = NULL;
#endif
  hz6_source_block_set_active(block, 0);
  hz6_source_block_set_route_registered(block, 0);
  hz6_source_block_set_run_active(block, 0);
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator->diagnostic_source_block_active_current != 0) {
    --allocator->diagnostic_source_block_active_current;
  }
#endif
  return released;
}
