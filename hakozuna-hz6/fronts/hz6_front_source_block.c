#include "hz6_front_source_block.h"
#include "hz6_front_util.h"
#include "midpage/hz6_midpage_front.h"

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_front_note_midpage_source_run_slot_route_register(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || front_id != HZ6_FRONT_MIDPAGE) {
    return;
  }
  if (class_id == HZ6_MIDPAGE_8K_CLASS_ID) {
    ++allocator->stats.midpage_8k_source_run_slot_route_register;
  } else if (class_id == HZ6_MIDPAGE_32K_CLASS_ID) {
    ++allocator->stats.midpage_32k_source_run_slot_route_register;
  }
}
#endif

#if HZ6_SOURCE_RUN_REUSE_L1
static void* hz6_front_source_block_reserved_slot(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t slot_bytes,
    Hz6SourceBlock* block,
    size_t* slot_index,
    Hz6ObjectDescriptor** out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  if (!slot_index) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
#if HZ6_SOURCE_RUN_RECLAIM_SAME_CLASS_L1
    if (hz6_allocator_reclaim_frontcache_descriptor_for_source_run_same_class(
            allocator, class_id)) {
      descriptor = hz6_allocator_find_free_descriptor(allocator);
    }
#elif HZ6_SOURCE_RUN_RECLAIM_DESCRIPTOR_L1
    if (hz6_allocator_reclaim_frontcache_descriptor_for_source_run(
            allocator, class_id)) {
      descriptor = hz6_allocator_find_free_descriptor(allocator);
    }
#endif
  }
  if (!descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_descriptor_fail;
#endif
    hz6_allocator_note_descriptor_frontcache_reuse_dryrun(allocator,
                                                          class_id);
    hz6_allocator_note_descgov_descriptor_fail(allocator, class_id);
    return NULL;
  }

  if (!hz6_allocator_source_run_reserve_slot(block, slot_index)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_miss_no_slot;
#endif
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_reserved;
#endif

  if (!hz6_source_block_route_registered(block)) {
    if (!hz6_allocator_source_block_register_invalid_range(
            allocator, block, front_id, class_id)) {
      hz6_allocator_source_run_rollback_slot(block, *slot_index);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_run_reuse_route_fail;
      ++allocator->stats.source_run_reuse_slot_fail;
#endif
      return NULL;
    }
  }
  if (!hz6_allocator_retain_source_block(block)) {
    hz6_allocator_source_run_rollback_slot(block, *slot_index);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_slot_fail;
#endif
    return NULL;
  }

  void* user_ptr =
      (void*)((unsigned char*)block->ptr + ((*slot_index) * slot_bytes));
  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, user_ptr, slot_bytes, block->ptr,
          block->bytes, block, class_id, hz6_source_block_source_kind(block),
          block->source_release, HZ6_STATE_ACTIVE)) {
    hz6_allocator_source_run_rollback_slot(block, *slot_index);
    hz6_allocator_release_source_block(allocator, block);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_prepare_fail;
    ++allocator->stats.source_run_reuse_slot_fail;
#endif
    return NULL;
  }

  if (!hz6_allocator_route_register_exact_reason(
          allocator, user_ptr, slot_bytes, front_id, class_id,
          descriptor->generation, descriptor,
          HZ6_ROUTE_REGISTER_REASON_SOURCE_RUN_SLOT)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_route_fail;
    ++allocator->stats.source_run_reuse_slot_fail;
#endif
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  hz6_front_note_midpage_source_run_slot_route_register(allocator, front_id,
                                                       class_id);
#endif

  hz6_allocator_source_run_set_descriptor(allocator, block, user_ptr,
                                          descriptor);
  hz6_allocator_source_run_commit_slot(block, *slot_index);
  if (out_descriptor) {
    *out_descriptor = descriptor;
  }
  return user_ptr;
}
#endif

static size_t hz6_front_prefill_from_source_run(Hz6Allocator* allocator,
                                                uint16_t front_id,
                                                uint16_t class_id,
                                                size_t slot_bytes,
                                                size_t block_bytes,
                                                Hz6SourceKind source_kind,
                                                size_t count) {
#if HZ6_SOURCE_RUN_REUSE_L1
  size_t filled = 0;
  while (filled < count &&
         hz6_allocator_frontcache_count(allocator, class_id) <
             hz6_allocator_frontcache_capacity(allocator, class_id)) {
    Hz6SourceBlock* block = hz6_allocator_source_run_find_reusable(
        allocator, source_kind, block_bytes, class_id, slot_bytes);
    if (!block) {
      break;
    }

    size_t slot_index = 0;
    Hz6ObjectDescriptor* descriptor = NULL;
    void* ptr = hz6_front_source_block_reserved_slot(
        allocator, front_id, class_id, slot_bytes, block, &slot_index,
        &descriptor);
    if (!ptr) {
      break;
    }

#if !HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1
    Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
    descriptor = (Hz6ObjectDescriptor*)route.descriptor;
    if (route.kind != HZ6_ROUTE_VALID || !descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_run_reuse_rollback;
#endif
      break;
    }
#else
    if (!descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_run_reuse_rollback;
#endif
      break;
    }
#endif

    if (!hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr)) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_run_reuse_rollback;
#endif
      break;
    }

#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_hit;
#endif
    ++filled;
  }
  return filled;
#else
  (void)allocator;
  (void)front_id;
  (void)class_id;
  (void)slot_bytes;
  (void)block_bytes;
  (void)source_kind;
  (void)count;
  return 0;
#endif
}

static void* hz6_front_source_block_slot_with_descriptor(
    Hz6Allocator* allocator,
    uint16_t front_id,
    uint16_t class_id,
    size_t user_bytes,
    size_t source_offset,
    Hz6SourceBlock* source_block,
    Hz6ObjectDescriptor** out_descriptor) {
  if (out_descriptor) {
    *out_descriptor = NULL;
  }
  if (!allocator || !source_block || !hz6_source_block_active(source_block) ||
      !source_block->ptr || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      user_bytes == 0 || source_offset > source_block->bytes ||
      user_bytes > source_block->bytes - source_offset) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    hz6_allocator_note_frontcache_spill_dryrun(allocator, class_id);
    hz6_allocator_note_frontcache_borrow_dryrun(allocator, front_id,
                                                class_id, user_bytes);
    if (hz6_allocator_spill_frontcache_for_descriptor(allocator, class_id)) {
      descriptor = hz6_allocator_find_free_descriptor(allocator);
#if HZ6_DIAGNOSTIC_PROBES
      if (descriptor) {
        ++allocator->stats.frontcache_spill_retry_success;
      }
#endif
    }
  }
  if (!descriptor) {
    hz6_allocator_note_descriptor_frontcache_reuse_dryrun(allocator,
                                                          class_id);
    hz6_allocator_note_descgov_descriptor_fail(allocator, class_id);
    return NULL;
  }
  if (!hz6_source_block_route_registered(source_block)) {
    if (!hz6_allocator_source_block_register_invalid_range(
            allocator, source_block, front_id, class_id)) {
      return NULL;
    }
  }
  if (!hz6_allocator_retain_source_block(source_block)) {
    return NULL;
  }

  void* user_ptr = (void*)((unsigned char*)source_block->ptr + source_offset);
  if (!hz6_allocator_prepare_descriptor(
          allocator, descriptor, user_ptr, user_bytes, source_block->ptr,
          source_block->bytes, source_block, class_id,
          hz6_source_block_source_kind(source_block),
          source_block->source_release,
          HZ6_STATE_ACTIVE)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (source_block->source_release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    return NULL;
  }

  if (!hz6_allocator_route_register_exact_reason(
          allocator, user_ptr, user_bytes, front_id, class_id,
          descriptor->generation, descriptor,
          HZ6_ROUTE_REGISTER_REASON_SOURCE_RUN_SLOT)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (hz6_allocator_descriptor_has_source_release(allocator, descriptor)) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(allocator, descriptor);
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  hz6_front_note_midpage_source_run_slot_route_register(allocator, front_id,
                                                       class_id);
#endif

  hz6_allocator_source_run_set_descriptor(allocator, source_block, user_ptr,
                                          descriptor);
  hz6_allocator_elastic_depot_source_run_mark_slot(
      allocator, source_block, user_ptr, class_id, user_bytes);
  if (out_descriptor) {
    *out_descriptor = descriptor;
  }
  return user_ptr;
}

void* hz6_front_source_block_slot(Hz6Allocator* allocator,
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t user_bytes,
                                  size_t source_offset,
                                  Hz6SourceBlock* source_block) {
  return hz6_front_source_block_slot_with_descriptor(
      allocator, front_id, class_id, user_bytes, source_offset, source_block,
      NULL);
}

size_t hz6_front_prefill_source_block_kind(Hz6Allocator* allocator,
                                           uint16_t front_id,
                                           uint16_t class_id,
                                           size_t slot_bytes,
                                           size_t block_bytes,
                                           Hz6SourceKind source_kind,
                                           size_t count) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      slot_bytes == 0 || count == 0) {
    return 0;
  }

  const Hz6OsMemoryOps* source_ops =
      hz6_allocator_source_ops(allocator, source_kind);
  if (!hz6_source_ops_valid(source_ops) || source_kind == HZ6_SOURCE_NONE) {
    return 0;
  }

  if (block_bytes < slot_bytes) {
    block_bytes = slot_bytes;
  }
  hz6_allocator_note_source_run_reuse_dryrun(allocator, source_kind,
                                             block_bytes, slot_bytes);

  size_t front_index = 0;
  int has_front_index =
      hz6_front_attr_index_from_id(front_id, &front_index) &&
      front_index < HZ6_FRONT_ATTR_COUNT;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_prefill_attempt;
  if (has_front_index) {
    ++allocator->stats.front_source_prefill_attempt[front_index];
  }
#else
  (void)has_front_index;
#endif
  hz6_allocator_remote_pending_note_prefill_attempt(allocator, front_id,
                                                   class_id);

  size_t filled = hz6_front_prefill_from_source_run(
      allocator, front_id, class_id, slot_bytes, block_bytes, source_kind,
      count);
  if (filled != 0) {
#if HZ6_DIAGNOSTIC_PROBES
    allocator->stats.front_source_prefill_alloc += filled;
    allocator->stats.source_prefill_filled += filled;
    if (has_front_index) {
      allocator->stats.front_source_prefill_filled[front_index] += filled;
    }
#endif
    hz6_allocator_remote_pending_note_prefill_commit(allocator, front_id,
                                                    class_id);
    return filled;
  }

  Hz6SourceBlock* block = hz6_allocator_create_source_block(
      allocator, block_bytes, source_ops, source_kind);
  if (!block) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }
#if HZ6_SOURCE_RUN_REUSE_L1
  if (!hz6_allocator_source_run_init(block, front_id, class_id, slot_bytes)) {
    hz6_allocator_release_source_block(allocator, block);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_LATE_REGISTER_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  int should_register_range =
      class_id <= HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS &&
      (HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 || front_id != HZ6_FRONT_TOY);
#if HZ6_SMALL_RUN_ROUTE_TOY_RANGE_ONLY_L1
  should_register_range =
      front_id == HZ6_FRONT_TOY &&
      slot_bytes >= HZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES &&
      slot_bytes <= HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES;
#endif
  if (should_register_range) {
    hz6_allocator_source_block_range_index_register(allocator, block);
  }
#endif
#endif

  filled = 0;
  size_t slots_per_block = block_bytes / slot_bytes;
  size_t target = count < slots_per_block ? count : slots_per_block;
  while (filled < target &&
         hz6_allocator_frontcache_count(allocator, class_id) <
             hz6_allocator_frontcache_capacity(allocator, class_id)) {
#if HZ6_SOURCE_RUN_REUSE_L1
    size_t slot_index = 0;
    void* ptr = hz6_front_source_block_reserved_slot(
        allocator, front_id, class_id, slot_bytes, block, &slot_index,
        NULL);
#else
    size_t offset = filled * slot_bytes;
    Hz6ObjectDescriptor* slot_descriptor = NULL;
    void* ptr = hz6_front_source_block_slot_with_descriptor(
        allocator, front_id, class_id, slot_bytes, offset, block,
        &slot_descriptor);
#endif
    if (!ptr) {
      break;
    }

#if HZ6_FRONT_PREFILL_DESCRIPTOR_OUT_L1 && !HZ6_SOURCE_RUN_REUSE_L1
    Hz6ObjectDescriptor* descriptor = slot_descriptor;
    if (!descriptor) {
      break;
    }
#else
    Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)route.descriptor;
    if (route.kind != HZ6_ROUTE_VALID || !descriptor) {
      break;
    }
#endif

    if (!hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr)) {
      break;
    }

    ++filled;
  }

  if (filled == 0) {
#if HZ6_TOY2_UNRETAINED_SOURCE_BLOCK_ABORT_L1
    if (front_id == HZ6_FRONT_TOY && class_id == 2) {
      if (!hz6_allocator_abort_unretained_source_block(allocator, block)) {
        hz6_allocator_release_source_block(allocator, block);
      }
    } else {
      hz6_allocator_release_source_block(allocator, block);
    }
#else
    hz6_allocator_release_source_block(allocator, block);
#endif
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_prefill_fallback;
    if (has_front_index) {
      ++allocator->stats.front_source_prefill_fallback[front_index];
    }
#endif
    return 0;
  }

#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.front_source_prefill_alloc += filled;
  allocator->stats.source_prefill_filled += filled;
  if (has_front_index) {
    allocator->stats.front_source_prefill_filled[front_index] += filled;
  }
#endif
  hz6_allocator_note_source_alloc_for_front(allocator, front_id);
  hz6_allocator_remote_pending_note_prefill_commit(allocator, front_id,
                                                  class_id);
  hz6_allocator_remote_pending_note_source_block_commit(allocator, front_id,
                                                       class_id);
  return filled;
}
