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
    if (hz6_source_block_route_shared(block)) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
      hz6_allocator_route_unregister_shared_invalid_range(allocator,
                                                          block->ptr);
#endif
    } else {
      hz6_route_backend_unregister_invalid_range(&allocator->route_backend,
                                                 block->ptr,
                                                 NULL);
    }
  }
  void* release_ptr = block->ptr;
  size_t release_bytes = block->bytes;
  Hz6SourceReleaseFn release_fn = block->source_release;
  int elastic_depot_block = hz6_allocator_source_block_is_elastic_depot(block);

  /* Hide the backing from stale cached descriptors before OS release. */
  block->ptr = NULL;
  hz6_source_block_set_run_active(block, 0);

  int released = release_fn
                     ? release_fn(release_ptr, release_bytes)
                     : hz6_source_system_release(release_ptr, release_bytes);
  if (elastic_depot_block) {
    ++allocator->stats.elastic_source_block_overflow_release;
  }
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
  hz6_source_block_set_route_shared(block, 0);
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator->diagnostic_source_block_active_current != 0) {
    --allocator->diagnostic_source_block_active_current;
  }
#endif
  return released;
}
