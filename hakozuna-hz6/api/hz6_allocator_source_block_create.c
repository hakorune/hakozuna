
#include "hz6_allocator.h"
#include "hz6_allocator_source_run.h"

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_allocator_record_source_block_failure_state(
    Hz6Allocator* allocator) {
  size_t active = 0;
  size_t registered = 0;
  size_t ref_nonzero = 0;
  size_t ref_zero = 0;

  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    const Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!hz6_source_block_active(block)) {
      continue;
    }
    ++active;
    if (hz6_source_block_route_registered(block)) {
      ++registered;
    }
    if (hz6_source_block_ref_count(block) != 0) {
      ++ref_nonzero;
    } else {
      ++ref_zero;
    }
  }

  if (active > allocator->stats.source_block_fail_active_max) {
    allocator->stats.source_block_fail_active_max = active;
  }
  if (registered > allocator->stats.source_block_fail_registered_max) {
    allocator->stats.source_block_fail_registered_max = registered;
  }
  if (ref_nonzero > allocator->stats.source_block_fail_ref_nonzero_max) {
    allocator->stats.source_block_fail_ref_nonzero_max = ref_nonzero;
  }
  if (ref_zero > allocator->stats.source_block_fail_ref_zero_max) {
    allocator->stats.source_block_fail_ref_zero_max = ref_zero;
  }
}
#endif

void hz6_allocator_note_source_run_reuse_dryrun(Hz6Allocator* allocator,
                                                Hz6SourceKind source_kind,
                                                size_t block_bytes,
                                                size_t slot_bytes) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || source_kind == HZ6_SOURCE_NONE || block_bytes == 0 ||
      slot_bytes == 0 || block_bytes < slot_bytes) {
    return;
  }

  size_t slots_per_block = block_bytes / slot_bytes;
  if (slots_per_block == 0) {
    return;
  }

  ++allocator->stats.source_run_reuse_dryrun_calls;
  size_t candidate_blocks = 0;
  size_t free_slots_total = 0;
  size_t largest_free_slots = 0;
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    const Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!hz6_source_block_active(block) || !block->ptr ||
        hz6_source_block_source_kind(block) != source_kind ||
        block->bytes != block_bytes ||
        hz6_source_block_ref_count(block) >= slots_per_block) {
      continue;
    }
    size_t free_slots = slots_per_block - hz6_source_block_ref_count(block);
    ++candidate_blocks;
    free_slots_total += free_slots;
    if (free_slots > largest_free_slots) {
      largest_free_slots = free_slots;
    }
  }

  if (candidate_blocks != 0) {
    ++allocator->stats.source_run_reuse_dryrun_candidate_calls;
    allocator->stats.source_run_reuse_dryrun_candidate_blocks_total +=
        candidate_blocks;
    allocator->stats.source_run_reuse_dryrun_free_slots_total +=
        free_slots_total;
  }
  if (largest_free_slots >
      allocator->stats.source_run_reuse_dryrun_largest_free_slots_max) {
    allocator->stats.source_run_reuse_dryrun_largest_free_slots_max =
        largest_free_slots;
  }
#else
  (void)allocator;
  (void)source_kind;
  (void)block_bytes;
  (void)slot_bytes;
#endif
}

Hz6SourceBlock* hz6_allocator_create_source_block(
    Hz6Allocator* allocator,
    size_t bytes,
    const Hz6OsMemoryOps* source_ops,
    Hz6SourceKind source_kind) {
  if (!allocator || bytes == 0 || !hz6_source_ops_valid(source_ops) ||
      source_kind == HZ6_SOURCE_NONE) {
    return NULL;
  }

  Hz6SourceBlock* block = NULL;
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
#endif
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    if (!hz6_source_block_active(&allocator->source_blocks[i])) {
      block = &allocator->source_blocks[i];
      break;
    }
  }
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.source_block_probe_total += probes;
  if (probes > allocator->stats.source_block_probe_max) {
    allocator->stats.source_block_probe_max = probes;
  }
#endif
  if (!block) {
#if HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
    block = hz6_allocator_claim_source_block_depot_slot();
    if (block) {
      ++allocator->stats.elastic_source_block_overflow_alloc;
    }
#endif
  }
  if (!block) {
    ++allocator->stats.source_block_exhausted;
#if HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
    ++allocator->stats.elastic_source_block_overflow_exhausted;
#endif
#if HZ6_DIAGNOSTIC_PROBES
    hz6_allocator_record_source_block_failure_state(allocator);
#endif
    return NULL;
  }

  void* ptr = source_ops->reserve(bytes, source_ops->allocation_granularity);
  if (!ptr) {
#if HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
    if (hz6_allocator_source_block_is_elastic_depot(block)) {
      hz6_source_block_set_active(block, 0);
      hz6_source_block_set_route_registered(block, 0);
      hz6_source_block_set_route_shared(block, 0);
      hz6_source_block_set_source_kind(block, HZ6_SOURCE_NONE);
      atomic_store_explicit(&block->ref_count, 0u, memory_order_release);
    }
#endif
    return NULL;
  }

  block->ptr = ptr;
  block->bytes = bytes;
  hz6_source_block_set_source_kind(block, source_kind);
  block->source_release = source_ops->release;
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
  block->route_backend = NULL;
#endif
  atomic_store_explicit(&block->ref_count, 0u, memory_order_release);
#if HZ6_SOURCE_RUN_INLINE_META_L1
  hz6_source_run_reset(block);
#else
  hz6_source_block_set_run_active(block, 0);
#endif
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  block->owner_source_storage_allocator = allocator;
#endif
  hz6_source_block_set_active(block, 1);
  hz6_source_block_set_route_registered(block, 0);
  hz6_source_block_set_route_shared(block, 0);
#if !HZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1
  hz6_allocator_source_block_range_index_register(allocator, block);
#endif
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->diagnostic_source_block_active_current;
  if (allocator->diagnostic_source_block_active_current >
      allocator->stats.source_block_active_max) {
    allocator->stats.source_block_active_max =
        allocator->diagnostic_source_block_active_current;
  }
#endif
  return block;
}
