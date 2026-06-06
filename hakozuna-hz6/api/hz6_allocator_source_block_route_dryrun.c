#include "hz6_allocator.h"

#if (HZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES) || \
    HZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1 ||                         \
    (HZ6_SMALL_RUN_ROUTE_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES) ||    \
    HZ6_SMALL_RUN_ROUTE_BEHAVIOR_L1
static int hz6_source_block_route_bit_get(const Hz6SourceBlock* block,
                                          size_t slot_index) {
  size_t word = slot_index / 64u;
  size_t bit = slot_index % 64u;
  if (!block || word >= HZ6_SOURCE_RUN_BITMAP_WORDS) {
    return 0;
  }
  return (block->run_used_bits[word] & (UINT64_C(1) << bit)) != 0;
}
#endif

#if HZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES
static int hz6_source_block_route_descriptor_matches(
    const Hz6ObjectDescriptor* descriptor,
    const Hz6SourceBlock* block,
    const void* ptr) {
  return descriptor && descriptor->ptr == ptr &&
         descriptor->source_block == block &&
         descriptor->state != HZ6_STATE_DEAD &&
         descriptor->state != HZ6_STATE_DESCRIPTOR_RESERVED;
}
#endif

void hz6_allocator_small_run_route_dryrun(Hz6Allocator* allocator,
                                          const void* ptr) {
#if HZ6_SMALL_RUN_ROUTE_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES && \
    HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 &&                  \
    HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  if (!allocator || !ptr) {
    return;
  }
  ++allocator->stats.smallrun_route_attempt;

  Hz6SourceBlock* block =
      hz6_allocator_source_block_range_index_lookup(allocator, ptr);
  if (!block) {
    ++allocator->stats.smallrun_exact_fallback_needed;
    return;
  }
  ++allocator->stats.smallrun_range_hit;

  if (!hz6_source_block_run_active(block) || block->run_front_id != HZ6_FRONT_TOY ||
      block->run_slot_bytes == 0 ||
      block->run_slot_bytes < HZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES ||
      block->run_slot_bytes > HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES ||
      block->run_slot_count == 0) {
    ++allocator->stats.smallrun_exact_fallback_needed;
    return;
  }

  const unsigned char* user = (const unsigned char*)ptr;
  const unsigned char* base = (const unsigned char*)block->ptr;
  size_t offset = (size_t)(user - base);
  if ((offset % block->run_slot_bytes) != 0) {
    ++allocator->stats.smallrun_would_invalid;
    return;
  }
  size_t slot_index = offset / block->run_slot_bytes;
  if (slot_index >= block->run_slot_count ||
      !hz6_source_block_route_bit_get(block, slot_index)) {
    ++allocator->stats.smallrun_would_invalid;
    return;
  }
  ++allocator->stats.smallrun_active_slot_hit;

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_source_run_descriptor_at(allocator, block, ptr);
  if (!descriptor) {
    ++allocator->stats.smallrun_exact_fallback_needed;
    return;
  }
  if (descriptor->ptr != ptr || descriptor->source_block != block ||
      descriptor->class_id != block->run_class_id ||
      descriptor->bytes != block->run_slot_bytes ||
      descriptor->state == HZ6_STATE_DEAD ||
      descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED) {
    ++allocator->stats.smallrun_false_positive;
    ++allocator->stats.smallrun_exact_fallback_needed;
    return;
  }
  ++allocator->stats.smallrun_descriptor_match;

  if (descriptor->generation == 0) {
    ++allocator->stats.smallrun_exact_fallback_needed;
    return;
  }
  ++allocator->stats.smallrun_generation_match;
  ++allocator->stats.smallrun_would_valid;
#else
  (void)allocator;
  (void)ptr;
#endif
}

Hz6RouteResult hz6_allocator_small_run_route_lookup(Hz6Allocator* allocator,
                                                    const void* ptr) {
#if HZ6_SMALL_RUN_ROUTE_BEHAVIOR_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.smallrun_behavior_attempt;
#endif

  Hz6SourceBlock* block =
      hz6_allocator_source_block_range_index_lookup(allocator, ptr);
  if (!block) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.smallrun_behavior_fallback;
#endif
    return hz6_route_miss();
  }
  if (!hz6_source_block_run_active(block) || block->run_front_id != HZ6_FRONT_TOY ||
      block->run_slot_bytes == 0 ||
      block->run_slot_bytes < HZ6_SMALL_RUN_ROUTE_MIN_SLOT_BYTES ||
      block->run_slot_bytes > HZ6_SMALL_RUN_ROUTE_MAX_SLOT_BYTES ||
      block->run_slot_count == 0) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.smallrun_behavior_fallback;
#endif
    return hz6_route_miss();
  }

  const unsigned char* user = (const unsigned char*)ptr;
  const unsigned char* base = (const unsigned char*)block->ptr;
  size_t offset = (size_t)(user - base);
  if ((offset % block->run_slot_bytes) != 0) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.smallrun_behavior_invalid_slot;
#endif
    return hz6_route_invalid(block->run_front_id, block->run_class_id);
  }
  size_t slot_index = offset / block->run_slot_bytes;
  if (slot_index >= block->run_slot_count ||
      !hz6_source_block_route_bit_get(block, slot_index)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.smallrun_behavior_invalid_slot;
#endif
    return hz6_route_invalid(block->run_front_id, block->run_class_id);
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_source_run_descriptor_at(allocator, block, ptr);
  if (!descriptor) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.smallrun_behavior_fallback;
#endif
    return hz6_route_miss();
  }
  if (descriptor->ptr != ptr || descriptor->source_block != block ||
      descriptor->class_id != block->run_class_id ||
      descriptor->bytes != block->run_slot_bytes ||
      descriptor->state == HZ6_STATE_DEAD ||
      descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED ||
      descriptor->generation == 0) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.smallrun_behavior_invalid_descriptor;
#endif
    return hz6_route_miss();
  }

  Hz6RouteResult route = hz6_route_valid(block->run_front_id,
                                         block->run_class_id,
                                         descriptor->generation,
                                         descriptor);
  route.route_allocator = allocator;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.smallrun_behavior_valid;
#endif
  return route;
#else
  (void)allocator;
  (void)ptr;
  return hz6_route_miss();
#endif
}

Hz6RouteResult hz6_allocator_source_block_route_lookup(Hz6Allocator* allocator,
                                                       const void* ptr) {
#if HZ6_SOURCE_BLOCK_ROUTE_BEHAVIOR_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1 && \
    HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_behavior_attempt;
#endif
  Hz6SourceBlock* block =
      hz6_allocator_source_block_range_index_lookup(allocator, ptr);
  if (!block) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_fallback;
#endif
    return hz6_route_miss();
  }
  if (!hz6_source_block_run_active(block) || block->run_slot_bytes == 0 ||
      block->run_slot_count == 0 || block->run_front_id == HZ6_FRONT_NONE) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_invalid_front;
#endif
    return hz6_route_miss();
  }
  if (block->run_class_id > HZ6_SOURCE_BLOCK_ROUTE_MAX_CLASS) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_fallback;
#endif
    return hz6_route_miss();
  }
  size_t front_index = 0;
  if (!hz6_front_attr_index_from_id(block->run_front_id, &front_index)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_invalid_front;
#endif
    return hz6_route_miss();
  }
  if (!HZ6_SOURCE_BLOCK_ROUTE_TOY_FRONT_L1 &&
      block->run_front_id == HZ6_FRONT_TOY) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_fallback;
#endif
    return hz6_route_miss();
  }

  const unsigned char* user = (const unsigned char*)ptr;
  const unsigned char* base = (const unsigned char*)block->ptr;
  size_t offset = (size_t)(user - base);
  if ((offset % block->run_slot_bytes) != 0) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_fallback;
#endif
    return hz6_route_miss();
  }
  size_t slot_index = offset / block->run_slot_bytes;
  if (slot_index >= block->run_slot_count ||
      !hz6_source_block_route_bit_get(block, slot_index)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_fallback;
#endif
    return hz6_route_miss();
  }

  Hz6ObjectDescriptor* descriptor =
      hz6_allocator_source_run_descriptor_at(allocator, block, ptr);
  if (!descriptor || descriptor->class_id != block->run_class_id ||
      descriptor->bytes != block->run_slot_bytes ||
      descriptor->ptr != ptr || descriptor->source_block != block ||
      descriptor->state == HZ6_STATE_DEAD ||
      descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_behavior_invalid_descriptor;
#endif
    return hz6_route_miss();
  }

  Hz6RouteResult route = hz6_route_valid(block->run_front_id,
                                         block->run_class_id,
                                         descriptor->generation,
                                         descriptor);
  route.route_allocator = allocator;
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_behavior_valid;
#endif
  return route;
#else
  (void)allocator;
  (void)ptr;
  return hz6_route_miss();
#endif
}

void hz6_allocator_source_block_route_dryrun(Hz6Allocator* allocator,
                                             const void* ptr) {
#if HZ6_SOURCE_BLOCK_ROUTE_DRYRUN_L1 && HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !ptr) {
    return;
  }

  ++allocator->stats.source_block_route_dryrun_attempt;
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  {
    Hz6SourceBlock* block =
        hz6_allocator_source_block_range_index_lookup(allocator, ptr);
    if (block) {
      ++allocator->stats.source_block_route_block_hit;
      if (!hz6_source_block_run_active(block) ||
          block->run_slot_bytes == 0 ||
          block->run_slot_count == 0) {
        ++allocator->stats.source_block_route_invalid_alignment;
        return;
      }

      const unsigned char* user = (const unsigned char*)ptr;
      const unsigned char* base = (const unsigned char*)block->ptr;
      size_t offset = (size_t)(user - base);
      if ((offset % block->run_slot_bytes) != 0) {
        ++allocator->stats.source_block_route_invalid_alignment;
        return;
      }
      size_t slot_index = offset / block->run_slot_bytes;
      if (slot_index >= block->run_slot_count) {
        ++allocator->stats.source_block_route_invalid_alignment;
        return;
      }
      if (!hz6_source_block_route_bit_get(block, slot_index)) {
        ++allocator->stats.source_block_route_invalid_unused;
        return;
      }
      ++allocator->stats.source_block_route_slot_hit;
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
      {
        Hz6ObjectDescriptor* descriptor =
            hz6_allocator_source_run_descriptor_at(allocator, block, ptr);
        if (descriptor) {
          if (descriptor->class_id != block->run_class_id ||
              descriptor->bytes != block->run_slot_bytes) {
            ++allocator->stats.source_block_route_class_mismatch;
            return;
          }
          ++allocator->stats.source_block_route_descriptor_hit;
          return;
        }
      }
#endif
    }
  }
#endif
  size_t probes = 0;
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    Hz6SourceBlock* block = &allocator->source_blocks[i];
    ++probes;
    if (!hz6_source_block_active(block) || !block->ptr || block->bytes == 0) {
      continue;
    }

    const unsigned char* user = (const unsigned char*)ptr;
    const unsigned char* base = (const unsigned char*)block->ptr;
    const unsigned char* end = base + block->bytes;
    if (user < base || user >= end) {
      continue;
    }

    ++allocator->stats.source_block_route_block_hit;
    allocator->stats.source_block_route_probe_total += probes;
    if (probes > allocator->stats.source_block_route_probe_max) {
      allocator->stats.source_block_route_probe_max = probes;
    }

    if (!hz6_source_block_run_active(block) ||
        block->run_slot_bytes == 0 ||
        block->run_slot_count == 0) {
      ++allocator->stats.source_block_route_invalid_alignment;
      return;
    }

    size_t offset = (size_t)(user - base);
    if ((offset % block->run_slot_bytes) != 0) {
      ++allocator->stats.source_block_route_invalid_alignment;
      return;
    }

    size_t slot_index = offset / block->run_slot_bytes;
    if (slot_index >= block->run_slot_count) {
      ++allocator->stats.source_block_route_invalid_alignment;
      return;
    }

    if (!hz6_source_block_route_bit_get(block, slot_index)) {
      ++allocator->stats.source_block_route_invalid_unused;
      return;
    }

    ++allocator->stats.source_block_route_slot_hit;

#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
    {
      Hz6ObjectDescriptor* descriptor =
          hz6_allocator_source_run_descriptor_at(allocator, block, ptr);
      if (descriptor) {
        if (descriptor->class_id != block->run_class_id ||
            descriptor->bytes != block->run_slot_bytes) {
          ++allocator->stats.source_block_route_class_mismatch;
          return;
        }
        ++allocator->stats.source_block_route_descriptor_hit;
        return;
      }
    }
#endif

    for (size_t d = 0; d < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++d) {
      const Hz6ObjectDescriptor* descriptor = &allocator->descriptors[d];
      if (!hz6_source_block_route_descriptor_matches(descriptor, block, ptr)) {
        continue;
      }
      if (descriptor->class_id != block->run_class_id ||
          descriptor->bytes != block->run_slot_bytes) {
        ++allocator->stats.source_block_route_class_mismatch;
        return;
      }
      ++allocator->stats.source_block_route_descriptor_hit;
      return;
    }

    ++allocator->stats.source_block_route_descriptor_miss;
    return;
  }

  allocator->stats.source_block_route_probe_total += probes;
  if (probes > allocator->stats.source_block_route_probe_max) {
    allocator->stats.source_block_route_probe_max = probes;
  }
  ++allocator->stats.source_block_route_miss_no_block;
#else
  (void)allocator;
  (void)ptr;
#endif
}
