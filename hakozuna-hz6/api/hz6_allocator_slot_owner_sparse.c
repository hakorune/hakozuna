#include "hz6_allocator_slot_owner_sparse.h"

#include "hz6_allocator_api_descriptor.h"

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1
typedef struct Hz6ElasticSlotOwnerSparseEntry {
  const Hz6SourceBlock* block;
  const void* block_ptr;
  uint32_t owner16;
  uint32_t generation;
  uint16_t slot_index;
  uint16_t used;
} Hz6ElasticSlotOwnerSparseEntry;

static Hz6ElasticSlotOwnerSparseEntry
    g_hz6_elastic_slot_owner_sparse[HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY];

static size_t hz6_elastic_slot_owner_sparse_hash(
    const Hz6SourceBlock* block,
    uint16_t slot_index) {
  uintptr_t x = (uintptr_t)block;
  x ^= ((uintptr_t)slot_index + UINT64_C(0x9e3779b97f4a7c15));
  x ^= x >> 33;
  x *= (uintptr_t)0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  return (size_t)(x % HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY);
}

static int hz6_elastic_slot_owner_sparse_slot_index(
    const Hz6ObjectDescriptor* descriptor,
    uint16_t* slot_index) {
  if (!descriptor || !descriptor->source_block || !descriptor->ptr ||
      !descriptor->source_block->ptr || descriptor->bytes == 0 ||
      !hz6_source_block_run_active(descriptor->source_block) ||
      descriptor->source_block->run_class_id != descriptor->class_id ||
      descriptor->source_block->run_slot_bytes != descriptor->bytes) {
    return 0;
  }
  const unsigned char* user = (const unsigned char*)descriptor->ptr;
  const unsigned char* base =
      (const unsigned char*)descriptor->source_block->ptr;
  if (user < base) {
    return 0;
  }
  size_t offset = (size_t)(user - base);
  if (offset >= descriptor->source_block->bytes ||
      (offset % descriptor->bytes) != 0) {
    return 0;
  }
  size_t index = offset / descriptor->bytes;
  if (index >= descriptor->source_block->run_slot_count ||
      index > UINT16_MAX) {
    return 0;
  }
  *slot_index = (uint16_t)index;
  return 1;
}
#endif

void hz6_allocator_elastic_slot_owner_sparse_note(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1
  if (!allocator || !descriptor || !descriptor->source_block ||
      HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY == 0) {
    return;
  }

  uint16_t slot_index = 0;
  if (!hz6_elastic_slot_owner_sparse_slot_index(descriptor, &slot_index)) {
    return;
  }

  ++allocator->stats.elastic_slot_owner_sparse_lookup;
  const uint32_t owner16 =
      hz6_descriptor_pack_owner16(allocator->owner.token);
  size_t start =
      hz6_elastic_slot_owner_sparse_hash(descriptor->source_block,
                                         slot_index);
  for (size_t probe = 0; probe < HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY;
       ++probe) {
    size_t index = start + probe;
    if (index >= HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY) {
      index -= HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY;
    }
    Hz6ElasticSlotOwnerSparseEntry* entry =
        &g_hz6_elastic_slot_owner_sparse[index];
    if (!entry->used) {
      entry->block = descriptor->source_block;
      entry->block_ptr = descriptor->source_block->ptr;
      entry->slot_index = slot_index;
      entry->owner16 = owner16;
      entry->generation = descriptor->generation;
      entry->used = 1;
      ++allocator->stats.elastic_slot_owner_sparse_miss;
      ++allocator->stats.elastic_slot_owner_sparse_insert;
      return;
    }
    if (entry->block == descriptor->source_block &&
        entry->block_ptr == descriptor->source_block->ptr &&
        entry->slot_index == slot_index) {
      ++allocator->stats.elastic_slot_owner_sparse_hit;
      if (entry->owner16 == owner16 &&
          entry->generation == descriptor->generation) {
        ++allocator->stats.elastic_slot_owner_sparse_owner_match;
      } else {
        ++allocator->stats.elastic_slot_owner_sparse_owner_mismatch;
        entry->owner16 = owner16;
        entry->generation = descriptor->generation;
        ++allocator->stats.elastic_slot_owner_sparse_update;
      }
      return;
    }
    ++allocator->stats.elastic_slot_owner_sparse_collision;
  }
  ++allocator->stats.elastic_slot_owner_sparse_full;
#else
  (void)allocator;
  (void)descriptor;
#endif
}

void hz6_allocator_elastic_slot_owner_consumer_dryrun(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken expected_owner,
    int existing_equal) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1 && \
    HZ6_ELASTIC_SLOT_OWNER_CONSUMER_DRYRUN_L1
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
  if (!mutable_allocator || !descriptor || !descriptor->source_block ||
      HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY == 0) {
    return;
  }

  uint16_t slot_index = 0;
  if (!hz6_elastic_slot_owner_sparse_slot_index(descriptor, &slot_index)) {
    return;
  }

  ++mutable_allocator->stats.elastic_slot_owner_consumer_probe;
  const uint32_t expected16 = hz6_descriptor_pack_owner16(expected_owner);
  size_t start =
      hz6_elastic_slot_owner_sparse_hash(descriptor->source_block,
                                         slot_index);
  for (size_t probe = 0; probe < HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY;
       ++probe) {
    size_t index = start + probe;
    if (index >= HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY) {
      index -= HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY;
    }
    const Hz6ElasticSlotOwnerSparseEntry* entry =
        &g_hz6_elastic_slot_owner_sparse[index];
    if (!entry->used) {
      ++mutable_allocator->stats.elastic_slot_owner_consumer_miss;
      ++mutable_allocator->stats.elastic_slot_owner_consumer_fallback;
      return;
    }
    if (entry->block != descriptor->source_block ||
        entry->block_ptr != descriptor->source_block->ptr ||
        entry->slot_index != slot_index) {
      continue;
    }
    ++mutable_allocator->stats.elastic_slot_owner_consumer_hit;
    if (entry->generation != descriptor->generation) {
      ++mutable_allocator->stats.elastic_slot_owner_consumer_stale_generation;
      ++mutable_allocator->stats.elastic_slot_owner_consumer_fallback;
      return;
    }
    if (entry->owner16 == expected16) {
      ++mutable_allocator->stats.elastic_slot_owner_consumer_owner_match;
      if (existing_equal) {
        ++mutable_allocator->stats.elastic_slot_owner_consumer_would_skip_l2;
      } else {
        ++mutable_allocator->stats.elastic_slot_owner_consumer_false_positive;
      }
    } else {
      ++mutable_allocator->stats.elastic_slot_owner_consumer_owner_mismatch;
      ++mutable_allocator->stats.elastic_slot_owner_consumer_fallback;
    }
    return;
  }
  ++mutable_allocator->stats.elastic_slot_owner_consumer_miss;
  ++mutable_allocator->stats.elastic_slot_owner_consumer_fallback;
#else
  (void)allocator;
  (void)descriptor;
  (void)expected_owner;
  (void)existing_equal;
#endif
}
