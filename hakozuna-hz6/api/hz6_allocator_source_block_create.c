#include "hz6_allocator.h"

#if HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
static Hz6SourceBlock
    g_hz6_source_block_depot[HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY];
static size_t g_hz6_source_block_depot_next;

static Hz6SourceBlock* hz6_allocator_find_source_block_depot_slot(void) {
  for (size_t offset = 0; offset < HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY;
       ++offset) {
    size_t index = g_hz6_source_block_depot_next + offset;
    if (index >= HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY) {
      index -= HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY;
    }
    Hz6SourceBlock* block = &g_hz6_source_block_depot[index];
    if (hz6_source_block_active(block) || block->ptr) {
      continue;
    }
    g_hz6_source_block_depot_next = index + 1;
    if (g_hz6_source_block_depot_next >=
        HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY) {
      g_hz6_source_block_depot_next = 0;
    }
    return block;
  }
  return NULL;
}

int hz6_allocator_source_block_is_elastic_depot(
    const Hz6SourceBlock* block) {
  const uintptr_t address = (uintptr_t)block;
  const uintptr_t begin = (uintptr_t)&g_hz6_source_block_depot[0];
  const uintptr_t end =
      (uintptr_t)&g_hz6_source_block_depot
          [HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY];
  return address >= begin && address < end;
}
#else
int hz6_allocator_source_block_is_elastic_depot(
    const Hz6SourceBlock* block) {
  (void)block;
  return 0;
}
#endif

static void hz6_source_run_reset(Hz6SourceBlock* block) {
  if (!block) {
    return;
  }
  block->run_slot_bytes = 0;
  block->run_class_id = 0;
  block->run_slot_count = 0;
  block->run_used_count = 0;
  block->run_next_hint = 0;
  hz6_source_block_set_run_active(block, 0);
  for (size_t i = 0; i < HZ6_SOURCE_RUN_BITMAP_WORDS; ++i) {
    block->run_used_bits[i] = 0;
  }
}

static int hz6_source_run_bit_get(const Hz6SourceBlock* block,
                                  size_t slot_index) {
  size_t word = slot_index / 64u;
  size_t bit = slot_index % 64u;
  return (block->run_used_bits[word] & (UINT64_C(1) << bit)) != 0;
}

static void hz6_source_run_bit_set(Hz6SourceBlock* block,
                                   size_t slot_index) {
  size_t word = slot_index / 64u;
  size_t bit = slot_index % 64u;
  block->run_used_bits[word] |= (UINT64_C(1) << bit);
}

static void hz6_source_run_bit_clear(Hz6SourceBlock* block,
                                     size_t slot_index) {
  size_t word = slot_index / 64u;
  size_t bit = slot_index % 64u;
  block->run_used_bits[word] &= ~(UINT64_C(1) << bit);
}

#if HZ6_SOURCE_RUN_REUSE_L1
static uint16_t hz6_source_run_count_used_bits(const Hz6SourceBlock* block) {
  if (!block || !hz6_source_block_run_active(block)) {
    return 0;
  }
  size_t count = 0;
  for (size_t word = 0; word < HZ6_SOURCE_RUN_BITMAP_WORDS; ++word) {
    uint64_t bits = block->run_used_bits[word];
    while (bits != 0) {
      bits &= bits - 1u;
      ++count;
    }
  }
  if (count > UINT16_MAX) {
    count = UINT16_MAX;
  }
  return (uint16_t)count;
}
#endif

int hz6_allocator_source_run_init(Hz6SourceBlock* block,
                                  uint16_t class_id,
                                  size_t slot_bytes) {
  if (!block || !hz6_source_block_active(block) || !block->ptr ||
      slot_bytes == 0 ||
      block->bytes < slot_bytes) {
    return 0;
  }
  size_t slots = block->bytes / slot_bytes;
  if (slots == 0 || slots > HZ6_SOURCE_RUN_MAX_SLOTS ||
      slots > UINT16_MAX) {
    return 0;
  }

  hz6_source_run_reset(block);
  block->run_slot_bytes = slot_bytes;
  block->run_class_id = class_id;
  block->run_slot_count = (uint16_t)slots;
  hz6_source_block_set_run_active(block, 1);
  return 1;
}

Hz6SourceBlock* hz6_allocator_source_run_find_reusable(
    Hz6Allocator* allocator,
    Hz6SourceKind source_kind,
    size_t block_bytes,
    uint16_t class_id,
    size_t slot_bytes) {
#if HZ6_SOURCE_RUN_REUSE_L1
  if (!allocator || source_kind == HZ6_SOURCE_NONE || block_bytes == 0 ||
      slot_bytes == 0) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_attempt;
#endif
#if HZ6_SOURCE_RUN_SLOT_LOOKUP_L1
  Hz6SourceBlock* best_block = NULL;
  uint16_t best_free_slots = 0;
#endif
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    Hz6SourceBlock* block = &allocator->source_blocks[i];
    if (!hz6_source_block_active(block) || !block->ptr ||
        !hz6_source_block_run_active(block) ||
        hz6_source_block_source_kind(block) != source_kind ||
        block->bytes != block_bytes ||
        block->run_class_id != class_id ||
        block->run_slot_bytes != slot_bytes) {
      continue;
    }
    uint16_t used_count = hz6_source_run_count_used_bits(block);
    if (used_count != block->run_used_count) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_run_reuse_used_count_mismatch;
#endif
      block->run_used_count = used_count;
      if (block->run_used_count > block->run_slot_count) {
        block->run_used_count = block->run_slot_count;
      }
    }
    if (block->run_used_count >= block->run_slot_count) {
      continue;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_run_reuse_candidate;
#endif
#if HZ6_SOURCE_RUN_SLOT_LOOKUP_L1
    {
      uint16_t free_slots =
          (uint16_t)(block->run_slot_count - block->run_used_count);
      if (!best_block || free_slots > best_free_slots) {
        best_block = block;
        best_free_slots = free_slots;
      }
    }
#else
    return block;
#endif
  }
#if HZ6_SOURCE_RUN_SLOT_LOOKUP_L1
  if (best_block) {
    return best_block;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_run_reuse_miss_no_block;
#endif
#else
  (void)allocator;
  (void)source_kind;
  (void)block_bytes;
  (void)class_id;
  (void)slot_bytes;
#endif
  return NULL;
}

int hz6_allocator_source_run_reserve_slot(Hz6SourceBlock* block,
                                          size_t* slot_index) {
  if (!block || !slot_index || !hz6_source_block_run_active(block) ||
      block->run_used_count >= block->run_slot_count) {
    return 0;
  }

  size_t start = block->run_next_hint;
  if (start >= block->run_slot_count) {
    start = 0;
  }
  for (size_t pass = 0; pass < 2; ++pass) {
    size_t begin = pass == 0 ? start : 0;
    size_t end = pass == 0 ? block->run_slot_count : start;
    for (size_t i = begin; i < end; ++i) {
      if (hz6_source_run_bit_get(block, i)) {
        continue;
      }
      hz6_source_run_bit_set(block, i);
      ++block->run_used_count;
      block->run_next_hint = (uint16_t)((i + 1u) % block->run_slot_count);
      *slot_index = i;
      return 1;
    }
  }
  return 0;
}

void hz6_allocator_source_run_commit_slot(Hz6SourceBlock* block,
                                          size_t slot_index) {
  (void)block;
  (void)slot_index;
}

void hz6_allocator_source_run_rollback_slot(Hz6SourceBlock* block,
                                            size_t slot_index) {
  if (!block || !hz6_source_block_run_active(block) ||
      slot_index >= block->run_slot_count ||
      !hz6_source_run_bit_get(block, slot_index)) {
    return;
  }
  hz6_source_run_bit_clear(block, slot_index);
  if (block->run_used_count != 0) {
    --block->run_used_count;
  }
  block->run_next_hint = (uint16_t)slot_index;
}

void hz6_allocator_source_run_release_slot(Hz6SourceBlock* block,
                                           void* ptr) {
  if (!block || !ptr || !hz6_source_block_run_active(block) || !block->ptr ||
      block->run_slot_bytes == 0) {
    return;
  }
  unsigned char* user = (unsigned char*)ptr;
  unsigned char* base = (unsigned char*)block->ptr;
  if (user < base) {
    return;
  }
  size_t offset = (size_t)(user - base);
  if (offset >= block->bytes || (offset % block->run_slot_bytes) != 0) {
    return;
  }
  size_t slot_index = offset / block->run_slot_bytes;
  if (slot_index >= block->run_slot_count ||
      !hz6_source_run_bit_get(block, slot_index)) {
    return;
  }
  hz6_source_run_bit_clear(block, slot_index);
  if (block->run_used_count != 0) {
    --block->run_used_count;
  }
  block->run_next_hint = (uint16_t)slot_index;
}

int hz6_allocator_source_run_contains_slot(const Hz6SourceBlock* block,
                                           const void* ptr,
                                           uint16_t class_id,
                                           size_t slot_bytes) {
  if (!block || !ptr || !hz6_source_block_run_active(block) || !block->ptr ||
      slot_bytes == 0 || block->run_slot_bytes != slot_bytes ||
      block->run_class_id != class_id) {
    return 0;
  }
  const unsigned char* user = (const unsigned char*)ptr;
  const unsigned char* base = (const unsigned char*)block->ptr;
  if (user < base) {
    return 0;
  }
  size_t offset = (size_t)(user - base);
  if (offset >= block->bytes || (offset % block->run_slot_bytes) != 0) {
    return 0;
  }
  size_t slot_index = offset / block->run_slot_bytes;
  if (slot_index >= block->run_slot_count) {
    return 0;
  }
  return hz6_source_run_bit_get(block, slot_index);
}

int hz6_allocator_elastic_depot_source_run_mark_slot(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    const void* ptr,
    uint16_t class_id,
    size_t slot_bytes) {
#if HZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1
  if (!allocator || !block || !ptr ||
      !hz6_allocator_source_block_is_elastic_depot(block) ||
      !hz6_source_block_active(block) || !block->ptr || slot_bytes == 0) {
    return 0;
  }

  const unsigned char* user = (const unsigned char*)ptr;
  const unsigned char* base = (const unsigned char*)block->ptr;
  if (user < base) {
    ++allocator->stats.elastic_depot_run_meta_slot_misaligned;
    return 0;
  }
  size_t offset = (size_t)(user - base);
  if (offset >= block->bytes || (offset % slot_bytes) != 0) {
    ++allocator->stats.elastic_depot_run_meta_slot_misaligned;
    return 0;
  }
  size_t slot_count = block->bytes / slot_bytes;
  if (slot_count == 0 || slot_count > HZ6_SOURCE_RUN_MAX_SLOTS ||
      slot_count > UINT16_MAX) {
    ++allocator->stats.elastic_depot_run_meta_too_many_slots;
    return 0;
  }
  size_t slot_index = offset / slot_bytes;
  if (slot_index >= slot_count) {
    ++allocator->stats.elastic_depot_run_meta_slot_misaligned;
    return 0;
  }

  if (!hz6_source_block_run_active(block)) {
    hz6_source_run_reset(block);
    block->run_slot_bytes = slot_bytes;
    block->run_class_id = class_id;
    block->run_slot_count = (uint16_t)slot_count;
    hz6_source_block_set_run_active(block, 1);
    ++allocator->stats.elastic_depot_run_meta_init;
  } else if (block->run_class_id != class_id ||
             block->run_slot_bytes != slot_bytes ||
             block->run_slot_count != (uint16_t)slot_count) {
    ++allocator->stats.elastic_depot_run_meta_class_mismatch;
    return 0;
  }

  if (hz6_source_run_bit_get(block, slot_index)) {
    ++allocator->stats.elastic_depot_run_meta_used_count_mismatch;
    return 1;
  }
  hz6_source_run_bit_set(block, slot_index);
  if (block->run_used_count < block->run_slot_count) {
    ++block->run_used_count;
  } else {
    ++allocator->stats.elastic_depot_run_meta_used_count_mismatch;
  }
  block->run_next_hint = (uint16_t)((slot_index + 1u) % block->run_slot_count);
  ++allocator->stats.elastic_depot_run_meta_mark;
  return 1;
#else
  (void)allocator;
  (void)block;
  (void)ptr;
  (void)class_id;
  (void)slot_bytes;
  return 0;
#endif
}

int hz6_allocator_elastic_depot_source_run_will_clear_slot(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block,
    const void* ptr,
    uint16_t class_id,
    size_t slot_bytes) {
#if HZ6_ELASTIC_DEPOT_SOURCE_RUN_META_L1
  if (!allocator || !block ||
      !hz6_allocator_source_block_is_elastic_depot(block)) {
    return 0;
  }
  if (hz6_allocator_source_run_contains_slot(block, ptr, class_id,
                                             slot_bytes)) {
    ++allocator->stats.elastic_depot_run_meta_clear;
    return 1;
  }
  return 0;
#else
  (void)allocator;
  (void)block;
  (void)ptr;
  (void)class_id;
  (void)slot_bytes;
  return 0;
#endif
}

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
    if (block->ref_count != 0) {
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
        block->bytes != block_bytes || block->ref_count >= slots_per_block) {
      continue;
    }
    size_t free_slots = slots_per_block - block->ref_count;
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
    block = hz6_allocator_find_source_block_depot_slot();
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
    return NULL;
  }

  block->ptr = ptr;
  block->bytes = bytes;
  hz6_source_block_set_source_kind(block, source_kind);
  block->source_release = source_ops->release;
#if !HZ6_SOURCE_BLOCK_NO_ROUTE_BACKPTR_L1
  block->route_backend = NULL;
#endif
  block->ref_count = 0;
  hz6_source_run_reset(block);
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  block->owner_source_storage_allocator = allocator;
#endif
  hz6_source_block_set_active(block, 1);
  hz6_source_block_set_route_registered(block, 0);
  hz6_source_block_set_route_shared(block, 0);
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
