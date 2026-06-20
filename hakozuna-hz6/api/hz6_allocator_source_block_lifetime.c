#include "hz6_allocator.h"
#include "hz6_allocator_route_domain.h"

#include <stdlib.h>

static size_t hz6_allocator_source_block_live_descriptor_count(
    const Hz6Allocator* allocator,
    const Hz6SourceBlock* block) {
  size_t live_descriptors = 0;
  if (!allocator || !block) {
    return 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    const Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (descriptor->source_block == block && descriptor->ptr &&
        descriptor->state != HZ6_STATE_DEAD) {
      ++live_descriptors;
    }
  }
  return live_descriptors;
}

static size_t hz6_allocator_source_block_live_route_count(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block) {
  size_t live_routes = 0;
  if (!allocator || !block) {
    return 0;
  }
  hz6_allocator_route_domain_lock(allocator);
  const Hz6RouteTable* table = &allocator->route_backend.exact_table;
  if (!table->entries) {
    hz6_allocator_route_domain_unlock(allocator);
    return 0;
  }
  for (size_t i = 0; i < table->capacity; ++i) {
    const Hz6RouteEntry* entry = &table->entries[i];
    if (!hz6_route_entry_active(entry) ||
        !hz6_route_entry_exact_valid(entry) || !entry->descriptor) {
      continue;
    }
    const Hz6ObjectDescriptor* descriptor =
        (const Hz6ObjectDescriptor*)entry->descriptor;
    if (descriptor->source_block == block) {
      ++live_routes;
    }
  }
  hz6_allocator_route_domain_unlock(allocator);
  return live_routes;
}

static int hz6_allocator_finalize_source_block_release(Hz6Allocator* allocator,
                                                       Hz6SourceBlock* block) {
  hz6_allocator_source_block_range_index_unregister(allocator, block);

  if (hz6_source_block_route_registered(block)) {
    if (hz6_source_block_route_shared(block)) {
#if HZ6_SHARED_ROUTE_DIRECTORY_L1 && HZ6_ELASTIC_ROUTE_OVERFLOW_L1
      hz6_allocator_route_unregister_shared_invalid_range(allocator,
                                                          block->ptr);
#endif
    } else {
      hz6_allocator_route_domain_lock(allocator);
      hz6_route_backend_unregister_invalid_range(&allocator->route_backend,
                                                 block->ptr,
                                                 NULL);
      hz6_allocator_route_domain_unlock(allocator);
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
  hz6_source_block_set_decommitted(block, 0);
  block->source_release = NULL;
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
  block->route_backend = NULL;
#endif
  atomic_store_explicit(&block->ref_count, 0u, memory_order_release);
#if HZ6_SOURCE_RUN_INLINE_META_L1
  block->run_slot_bytes = 0;
  block->run_front_id = HZ6_FRONT_NONE;
  block->run_class_id = 0;
  block->run_slot_count = 0;
  block->run_used_count = 0;
  block->run_next_hint = 0;
  for (size_t i = 0; i < HZ6_SOURCE_RUN_BITMAP_WORDS; ++i) {
    block->run_used_bits[i] = 0;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  free(block->run_descriptor_indices);
  block->run_descriptor_indices = NULL;
#endif
#endif
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

int hz6_allocator_retain_source_block(Hz6SourceBlock* block) {
  if (!block || !hz6_source_block_active(block) || !block->ptr) {
    return 0;
  }
  atomic_fetch_add_explicit(&block->ref_count, 1u, memory_order_acq_rel);
  return 1;
}

int hz6_allocator_release_source_block(Hz6Allocator* allocator,
                                       Hz6SourceBlock* block) {
  if (!allocator || !block || !hz6_source_block_active(block) ||
      !block->ptr) {
    return 0;
  }

  size_t old_ref = atomic_load_explicit(&block->ref_count,
                                        memory_order_acquire);
  while (old_ref != 0) {
    if (atomic_compare_exchange_weak_explicit(&block->ref_count,
                                              &old_ref,
                                              old_ref - 1u,
                                              memory_order_acq_rel,
                                              memory_order_acquire)) {
      break;
    }
  }
  if (old_ref == 0 || old_ref - 1u != 0) {
    return 1;
  }

#if HZ6_SOURCE_BLOCK_RELEASE_LIVE_GUARD_L1
  size_t live_descriptors =
      hz6_allocator_source_block_live_descriptor_count(allocator, block);
  if (live_descriptors != 0) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_release_live_guard;
    if (live_descriptors >
        allocator->stats.source_block_release_live_descriptors_max) {
      allocator->stats.source_block_release_live_descriptors_max =
          live_descriptors;
    }
#endif
    atomic_store_explicit(&block->ref_count, live_descriptors,
                          memory_order_release);
    return 1;
  }
#endif

  return hz6_allocator_finalize_source_block_release(allocator, block);
}

int hz6_allocator_abort_unretained_source_block(Hz6Allocator* allocator,
                                                Hz6SourceBlock* block) {
  if (!allocator || !block || !hz6_source_block_active(block) ||
      !block->ptr) {
    if (allocator) {
      ++allocator->stats.toy2_construct_abort_double_release;
    }
    return 0;
  }
  ++allocator->stats.toy2_construct_abort_attempt;

  size_t ref_count = atomic_load_explicit(&block->ref_count,
                                          memory_order_acquire);
  if (ref_count != 0) {
    ++allocator->stats.toy2_construct_abort_ref_nonzero;
    return 0;
  }

#if HZ6_SOURCE_RUN_INLINE_META_L1
  if (block->run_used_count != 0) {
    ++allocator->stats.toy2_construct_abort_pending_reference;
    return 0;
  }
#endif

  if (hz6_allocator_source_block_live_descriptor_count(allocator, block) !=
      0) {
    ++allocator->stats.toy2_construct_abort_live_descriptor;
    return 0;
  }
  if (hz6_allocator_source_block_live_route_count(allocator, block) != 0) {
    ++allocator->stats.toy2_construct_abort_live_route;
    return 0;
  }

  int released = hz6_allocator_finalize_source_block_release(allocator, block);
  if (!released) {
    ++allocator->stats.toy2_construct_abort_release_fail;
  }
  if (hz6_source_block_active(block) || block->ptr) {
    ++allocator->stats.toy2_unretained_active_block_after_abort;
  } else {
    ++allocator->stats.toy2_construct_abort_success;
  }
  return released;
}
