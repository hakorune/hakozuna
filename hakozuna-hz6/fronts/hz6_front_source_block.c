#include "hz6_front_source_block.h"
#include "hz6_front_util.h"

void* hz6_front_source_block_slot(Hz6Allocator* allocator,
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t user_bytes,
                                  size_t source_offset,
                                  Hz6SourceBlock* source_block) {
  if (!allocator || !source_block || !source_block->active ||
      !source_block->ptr || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      user_bytes == 0 || source_offset > source_block->bytes ||
      user_bytes > source_block->bytes - source_offset) {
    return NULL;
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_find_free_descriptor(allocator);
  if (!descriptor) {
    hz6_allocator_note_frontcache_spill_dryrun(allocator, class_id);
    return NULL;
  }
  if (!source_block->route_registered) {
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
          source_block->source_kind, source_block->source_release,
          HZ6_STATE_ACTIVE)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (source_block->source_release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  if (!hz6_allocator_route_register_exact(
          allocator, user_ptr, user_bytes, front_id, class_id,
          descriptor->generation, descriptor)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (descriptor->source_release) {
      ++allocator->stats.source_owned_release;
    }
#endif
    hz6_allocator_release_descriptor_source(descriptor);
    return NULL;
  }

  return user_ptr;
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

  size_t filled = 0;
  size_t slots_per_block = block_bytes / slot_bytes;
  size_t target = count < slots_per_block ? count : slots_per_block;
  while (filled < target &&
         hz6_allocator_frontcache_count(allocator, class_id) <
             hz6_allocator_frontcache_capacity(allocator, class_id)) {
    size_t offset = filled * slot_bytes;
    void* ptr = hz6_front_source_block_slot(
        allocator, front_id, class_id, slot_bytes, offset, block);
    if (!ptr) {
      break;
    }

    Hz6RouteResult route = hz6_allocator_route_lookup(allocator, ptr);
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)route.descriptor;
    if (route.kind != HZ6_ROUTE_VALID || !descriptor) {
      break;
    }

    if (!hz6_allocator_cache_active_descriptor(allocator, descriptor, ptr)) {
      break;
    }

    ++filled;
  }

  if (filled == 0) {
    hz6_allocator_release_source_block(block);
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
  return filled;
}
