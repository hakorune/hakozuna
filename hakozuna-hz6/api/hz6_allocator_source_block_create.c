#include "hz6_allocator.h"

#include <stdlib.h>

#if HZ6_ELASTIC_SOURCE_BLOCK_OVERFLOW_L1
static Hz6SourceBlock
    g_hz6_source_block_depot[HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY];
static size_t g_hz6_source_block_depot_next;
static atomic_flag g_hz6_source_block_depot_lock = ATOMIC_FLAG_INIT;

static void hz6_allocator_lock_source_block_depot(void) {
  while (atomic_flag_test_and_set_explicit(&g_hz6_source_block_depot_lock,
                                           memory_order_acquire)) {
  }
}

static void hz6_allocator_unlock_source_block_depot(void) {
  atomic_flag_clear_explicit(&g_hz6_source_block_depot_lock,
                             memory_order_release);
}

static Hz6SourceBlock* hz6_allocator_claim_source_block_depot_slot(void) {
  hz6_allocator_lock_source_block_depot();
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
    hz6_source_block_set_active(block, 1);
    g_hz6_source_block_depot_next = index + 1;
    if (g_hz6_source_block_depot_next >=
        HZ6_ELASTIC_SOURCE_BLOCK_DEPOT_CAPACITY) {
      g_hz6_source_block_depot_next = 0;
    }
    hz6_allocator_unlock_source_block_depot();
    return block;
  }
  hz6_allocator_unlock_source_block_depot();
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
  block->run_front_id = HZ6_FRONT_NONE;
  block->run_class_id = 0;
  block->run_slot_count = 0;
  block->run_used_count = 0;
  block->run_next_hint = 0;
  hz6_source_block_set_run_active(block, 0);
  for (size_t i = 0; i < HZ6_SOURCE_RUN_BITMAP_WORDS; ++i) {
    block->run_used_bits[i] = 0;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  free(block->run_descriptor_indices);
  block->run_descriptor_indices = NULL;
#else
  for (size_t i = 0; i < HZ6_SOURCE_RUN_MAX_SLOTS; ++i) {
    block->run_descriptor_indices[i] = UINT32_MAX;
  }
#endif
#endif
}

#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
static int hz6_source_run_descriptor_map_init(Hz6SourceBlock* block) {
  if (!block || block->run_slot_count == 0) {
    return 0;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  block->run_descriptor_indices =
      (uint32_t*)malloc(sizeof(uint32_t) * block->run_slot_count);
  if (!block->run_descriptor_indices) {
    return 0;
  }
  for (size_t i = 0; i < block->run_slot_count; ++i) {
    block->run_descriptor_indices[i] = UINT32_MAX;
  }
#else
  for (size_t i = 0; i < HZ6_SOURCE_RUN_MAX_SLOTS; ++i) {
    block->run_descriptor_indices[i] = UINT32_MAX;
  }
#endif
  return 1;
}
#endif

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
                                  uint16_t front_id,
                                  uint16_t class_id,
                                  size_t slot_bytes) {
  if (!block || !hz6_source_block_active(block) || !block->ptr ||
      front_id == HZ6_FRONT_NONE || slot_bytes == 0 ||
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
  block->run_front_id = front_id;
  block->run_class_id = class_id;
  block->run_slot_count = (uint16_t)slots;
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  if (!hz6_source_run_descriptor_map_init(block)) {
    hz6_source_run_reset(block);
    return 0;
  }
#endif
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

#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
static int hz6_allocator_source_run_slot_index(const Hz6SourceBlock* block,
                                               const void* ptr,
                                               size_t* slot_index) {
  if (!block || !ptr || !slot_index || !hz6_source_block_run_active(block) ||
      !block->ptr || block->run_slot_bytes == 0) {
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
  size_t index = offset / block->run_slot_bytes;
  if (index >= block->run_slot_count ||
      index >= HZ6_SOURCE_RUN_MAX_SLOTS) {
    return 0;
  }
  *slot_index = index;
  return 1;
}
#endif

void hz6_allocator_source_run_set_descriptor(
    Hz6Allocator* allocator,
    Hz6SourceBlock* block,
    const void* ptr,
    const Hz6ObjectDescriptor* descriptor) {
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  size_t slot_index = 0;
  size_t descriptor_index =
      hz6_allocator_descriptor_index(allocator, descriptor);
  if (!hz6_allocator_source_run_slot_index(block, ptr, &slot_index) ||
      descriptor_index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY ||
      descriptor_index > UINT32_MAX) {
    return;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  if (!block->run_descriptor_indices) {
    return;
  }
#endif
  block->run_descriptor_indices[slot_index] = (uint32_t)descriptor_index;
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.source_block_route_descriptor_map_set;
  }
#endif
#else
  (void)allocator;
  (void)block;
  (void)ptr;
  (void)descriptor;
#endif
}

void hz6_allocator_source_run_clear_descriptor(Hz6Allocator* allocator,
                                               Hz6SourceBlock* block,
                                               const void* ptr) {
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  size_t slot_index = 0;
  if (!hz6_allocator_source_run_slot_index(block, ptr, &slot_index)) {
    return;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  if (!block->run_descriptor_indices) {
    return;
  }
#endif
  block->run_descriptor_indices[slot_index] = UINT32_MAX;
#if HZ6_DIAGNOSTIC_PROBES
  if (allocator) {
    ++allocator->stats.source_block_route_descriptor_map_clear;
  }
#endif
#else
  (void)allocator;
  (void)block;
  (void)ptr;
#endif
}

Hz6ObjectDescriptor* hz6_allocator_source_run_descriptor_at(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block,
    const void* ptr) {
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
  size_t slot_index = 0;
  if (!allocator ||
      !hz6_allocator_source_run_slot_index(block, ptr, &slot_index)) {
    return NULL;
  }
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_DYNAMIC_L1
  if (!block->run_descriptor_indices) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_descriptor_map_miss;
#endif
    return NULL;
  }
#endif
  uint32_t descriptor_index = block->run_descriptor_indices[slot_index];
  if (descriptor_index == UINT32_MAX ||
      descriptor_index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_descriptor_map_miss;
#endif
    return NULL;
  }
  Hz6ObjectDescriptor* descriptor =
      &allocator->descriptors[descriptor_index];
  if (descriptor->ptr != ptr || descriptor->source_block != block ||
      descriptor->state == HZ6_STATE_DEAD ||
      descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_descriptor_map_stale;
#endif
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_descriptor_map_hit;
#endif
  return descriptor;
#else
  (void)allocator;
  (void)block;
  (void)ptr;
  return NULL;
#endif
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
  uint16_t run_slot_count = (uint16_t)slot_count;

  if (!hz6_source_block_run_active(block)) {
    hz6_source_run_reset(block);
    block->run_slot_bytes = slot_bytes;
    block->run_front_id = HZ6_FRONT_NONE;
    block->run_class_id = class_id;
    block->run_slot_count = run_slot_count;
#if HZ6_SOURCE_BLOCK_ROUTE_SLOT_DESCRIPTOR_MAP_L1
    if (!hz6_source_run_descriptor_map_init(block)) {
      hz6_source_run_reset(block);
      ++allocator->stats.elastic_depot_run_meta_class_mismatch;
      return 0;
    }
#endif
    hz6_source_block_set_run_active(block, 1);
    ++allocator->stats.elastic_depot_run_meta_init;
  } else if (block->run_slot_count == 0 ||
             block->run_class_id != class_id ||
             block->run_slot_bytes != slot_bytes ||
             block->run_slot_count != run_slot_count) {
    ++allocator->stats.elastic_depot_run_meta_class_mismatch;
    return 0;
  }

  if (hz6_source_run_bit_get(block, slot_index)) {
    ++allocator->stats.elastic_depot_run_meta_used_count_mismatch;
    return 1;
  }
  hz6_source_run_bit_set(block, slot_index);
  if (block->run_used_count < run_slot_count) {
    ++block->run_used_count;
  } else {
    ++allocator->stats.elastic_depot_run_meta_used_count_mismatch;
  }
  block->run_next_hint = (uint16_t)((slot_index + 1u) % run_slot_count);
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

#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
static uintptr_t hz6_source_block_route_range_page(uintptr_t address) {
  return address & ~(uintptr_t)(HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY -
                                1u);
}

static size_t hz6_source_block_route_range_hash(uintptr_t page) {
  return (size_t)((page >> 12) %
                  HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY);
}

static size_t hz6_allocator_source_block_index(const Hz6Allocator* allocator,
                                               const Hz6SourceBlock* block) {
  if (!allocator || !block || block < allocator->source_blocks ||
      block >= allocator->source_blocks + HZ6_SOURCE_BLOCK_CAPACITY) {
    return HZ6_SOURCE_BLOCK_CAPACITY;
  }
  return (size_t)(block - allocator->source_blocks);
}
#endif

void hz6_allocator_source_block_range_index_register(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block) {
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  if (!allocator || !block || !block->ptr || block->bytes == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY == 0) {
    return;
  }
  size_t block_index = hz6_allocator_source_block_index(allocator, block);
  if (block_index >= HZ6_SOURCE_BLOCK_CAPACITY) {
    return;
  }
  uintptr_t begin = hz6_source_block_route_range_page((uintptr_t)block->ptr);
  uintptr_t end = (uintptr_t)block->ptr + block->bytes - 1u;
  uintptr_t last = hz6_source_block_route_range_page(end);
  for (uintptr_t page = begin;; page +=
           HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY) {
    size_t start = hz6_source_block_route_range_hash(page);
    size_t first_tombstone = HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
    int inserted = 0;
    for (size_t probe = 0;
         probe < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
         ++probe) {
      size_t index = start + probe;
      if (index >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
        index -= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
      }
      Hz6SourceBlockRangeIndexEntry* entry =
          &allocator->source_block_range_index[index];
      if (entry->active == 2u) {
        if (first_tombstone >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
          first_tombstone = index;
        }
        continue;
      }
      if (entry->active == 1u && entry->page != page) {
        continue;
      }
      if (entry->active == 0u &&
          first_tombstone < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
        entry = &allocator->source_block_range_index[first_tombstone];
      }
      entry->page = page;
      entry->block_index = (uint32_t)block_index;
      entry->active = 1;
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_register;
#endif
      inserted = 1;
      break;
    }
    if (!inserted) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_register_fail;
#endif
      return;
    }
    if (page == last) {
      break;
    }
  }
#else
  (void)allocator;
  (void)block;
#endif
}

void hz6_allocator_source_block_range_index_unregister(
    Hz6Allocator* allocator,
    const Hz6SourceBlock* block) {
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  if (!allocator || !block || !block->ptr || block->bytes == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY == 0) {
    return;
  }
  size_t block_index = hz6_allocator_source_block_index(allocator, block);
  if (block_index >= HZ6_SOURCE_BLOCK_CAPACITY) {
    return;
  }
  uintptr_t begin = hz6_source_block_route_range_page((uintptr_t)block->ptr);
  uintptr_t end = (uintptr_t)block->ptr + block->bytes - 1u;
  uintptr_t last = hz6_source_block_route_range_page(end);
  for (uintptr_t page = begin;; page +=
           HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY) {
    size_t start = hz6_source_block_route_range_hash(page);
    for (size_t probe = 0;
         probe < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
         ++probe) {
      size_t index = start + probe;
      if (index >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
        index -= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
      }
      Hz6SourceBlockRangeIndexEntry* entry =
          &allocator->source_block_range_index[index];
      if (entry->active == 0u) {
        break;
      }
      if (entry->active != 1u) {
        continue;
      }
      if (entry->page != page) {
        continue;
      }
      if (entry->block_index == block_index) {
        entry->active = 2u;
        entry->page = 0;
        entry->block_index = 0;
#if HZ6_DIAGNOSTIC_PROBES
        ++allocator->stats.source_block_route_range_index_unregister;
#endif
      }
      break;
    }
    if (page == last) {
      break;
    }
  }
#else
  (void)allocator;
  (void)block;
#endif
}

Hz6SourceBlock* hz6_allocator_source_block_range_index_lookup(
    Hz6Allocator* allocator,
    const void* ptr) {
#if HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_L1
  if (!allocator || !ptr ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY == 0 ||
      HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_GRANULARITY == 0) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_range_index_lookup;
#endif
  uintptr_t page = hz6_source_block_route_range_page((uintptr_t)ptr);
  size_t start = hz6_source_block_route_range_hash(page);
  for (size_t probe = 0;
       probe < HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
       ++probe) {
    size_t index = start + probe;
    if (index >= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY) {
      index -= HZ6_SOURCE_BLOCK_ROUTE_RANGE_INDEX_CAPACITY;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_range_index_probe_total;
    if (probe + 1u >
        allocator->stats.source_block_route_range_index_probe_max) {
      allocator->stats.source_block_route_range_index_probe_max = probe + 1u;
    }
#endif
    Hz6SourceBlockRangeIndexEntry* entry =
        &allocator->source_block_range_index[index];
    if (entry->active == 0u) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_miss;
#endif
      return NULL;
    }
    if (entry->active != 1u) {
      continue;
    }
    if (entry->page != page) {
      continue;
    }
    if (entry->block_index >= HZ6_SOURCE_BLOCK_CAPACITY) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_stale;
#endif
      return NULL;
    }
    Hz6SourceBlock* block = &allocator->source_blocks[entry->block_index];
    const unsigned char* user = (const unsigned char*)ptr;
    const unsigned char* base = (const unsigned char*)block->ptr;
    if (!hz6_source_block_active(block) || !block->ptr ||
        user < base || user >= base + block->bytes) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.source_block_route_range_index_stale;
#endif
      return NULL;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.source_block_route_range_index_hit;
#endif
    return block;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.source_block_route_range_index_miss;
#endif
  return NULL;
#else
  (void)allocator;
  (void)ptr;
  return NULL;
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
  hz6_source_run_reset(block);
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  block->owner_source_storage_allocator = allocator;
#endif
  hz6_source_block_set_active(block, 1);
  hz6_source_block_set_route_registered(block, 0);
  hz6_source_block_set_route_shared(block, 0);
  hz6_allocator_source_block_range_index_register(allocator, block);
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
