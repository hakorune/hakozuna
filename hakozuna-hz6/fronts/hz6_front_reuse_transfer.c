#include "hz6_front_util.h"

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SOURCE_BLOCK_LOCALIZE_DRYRUN_L1
static int hz6_front_has_free_local_source_block_slot(
    const Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }
  for (size_t i = 0; i < HZ6_SOURCE_BLOCK_CAPACITY; ++i) {
    if (!hz6_source_block_active(&allocator->source_blocks[i])) {
      return 1;
    }
  }
  return 0;
}

static void hz6_front_note_source_block_localize_dryrun(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor || !descriptor->source_block ||
      !hz6_allocator_source_block_is_elastic_depot(
          descriptor->source_block)) {
    return;
  }

  ++allocator->stats.elastic_source_block_localize_probe;
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  if (descriptor->source_block->owner_source_storage_allocator ==
      allocator) {
    ++allocator->stats.elastic_source_block_localize_storage_match;
    return;
  }
#endif
  ++allocator->stats.elastic_source_block_localize_storage_mismatch;
  if (descriptor->source_block->ref_count > 1) {
    ++allocator->stats.elastic_source_block_localize_block_shared;
    return;
  }
  if (!hz6_front_has_free_local_source_block_slot(allocator)) {
    ++allocator->stats.elastic_source_block_localize_no_local_slot;
    return;
  }
  ++allocator->stats.elastic_source_block_localize_would_move;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1
static void hz6_front_note_source_run_locality_dryrun(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor || !descriptor->source_block ||
      !hz6_allocator_source_block_is_elastic_depot(
          descriptor->source_block)) {
    return;
  }

  ++allocator->stats.elastic_source_run_locality_probe;
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  if (descriptor->source_block->owner_source_storage_allocator ==
      allocator) {
    return;
  }
#endif
  ++allocator->stats.elastic_source_run_locality_storage_mismatch;
  if (!hz6_source_block_run_active(descriptor->source_block)) {
    ++allocator->stats.elastic_source_run_locality_run_miss;
  } else if (descriptor->source_block->run_class_id != descriptor->class_id ||
             descriptor->source_block->run_slot_bytes != descriptor->bytes) {
    ++allocator->stats.elastic_source_run_locality_class_mismatch;
    return;
  } else if (!hz6_allocator_source_run_contains_slot(
                 descriptor->source_block, descriptor->ptr,
                 descriptor->class_id, descriptor->bytes)) {
    ++allocator->stats.elastic_source_run_locality_run_miss;
    return;
  } else {
    ++allocator->stats.elastic_source_run_locality_slot_match;
    ++allocator->stats.elastic_source_run_locality_would_rehome_slot;
    return;
  }

  const unsigned char* user = (const unsigned char*)descriptor->ptr;
  const unsigned char* base =
      (const unsigned char*)descriptor->source_block->ptr;
  if (!user || !base || user < base || descriptor->bytes == 0) {
    return;
  }
  size_t offset = (size_t)(user - base);
  if (offset >= descriptor->source_block->bytes ||
      (offset % descriptor->bytes) != 0) {
    return;
  }
  ++allocator->stats.elastic_source_run_locality_slot_match;
  ++allocator->stats.elastic_source_run_locality_would_rehome_slot;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1
typedef struct Hz6ElasticSlotOwnerSparseEntry {
  const Hz6SourceBlock* block;
  const void* block_ptr;
  uint32_t owner16;
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

static int hz6_front_source_run_slot_index(
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

static void hz6_front_note_slot_owner_sparse_meta(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor || !descriptor->source_block ||
      HZ6_ELASTIC_SLOT_OWNER_SPARSE_CAPACITY == 0) {
    return;
  }

  uint16_t slot_index = 0;
  if (!hz6_front_source_run_slot_index(descriptor, &slot_index)) {
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
      entry->used = 1;
      ++allocator->stats.elastic_slot_owner_sparse_miss;
      ++allocator->stats.elastic_slot_owner_sparse_insert;
      return;
    }
    if (entry->block == descriptor->source_block &&
        entry->block_ptr == descriptor->source_block->ptr &&
        entry->slot_index == slot_index) {
      ++allocator->stats.elastic_slot_owner_sparse_hit;
      if (entry->owner16 == owner16) {
        ++allocator->stats.elastic_slot_owner_sparse_owner_match;
      } else {
        ++allocator->stats.elastic_slot_owner_sparse_owner_mismatch;
        entry->owner16 = owner16;
        ++allocator->stats.elastic_slot_owner_sparse_update;
      }
      return;
    }
    ++allocator->stats.elastic_slot_owner_sparse_collision;
  }
  ++allocator->stats.elastic_slot_owner_sparse_full;
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SLOT_OWNER_LOCALITY_DRYRUN_L1
static void hz6_front_note_slot_owner_locality_dryrun(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor || !descriptor->source_block ||
      !hz6_allocator_source_block_is_elastic_depot(
          descriptor->source_block)) {
    return;
  }

  ++allocator->stats.elastic_slot_owner_locality_probe;
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  if (descriptor->source_block->owner_source_storage_allocator ==
      allocator) {
    ++allocator->stats.elastic_slot_owner_locality_storage_match;
    return;
  }
#endif
  ++allocator->stats.elastic_slot_owner_locality_storage_mismatch;
  if (!hz6_source_block_run_active(descriptor->source_block)) {
    ++allocator->stats.elastic_slot_owner_locality_run_miss;
    return;
  }
  if (descriptor->source_block->run_class_id != descriptor->class_id ||
      descriptor->source_block->run_slot_bytes != descriptor->bytes) {
    ++allocator->stats.elastic_slot_owner_locality_class_mismatch;
    return;
  }
  if (!hz6_allocator_source_run_contains_slot(descriptor->source_block,
                                             descriptor->ptr,
                                             descriptor->class_id,
                                             descriptor->bytes)) {
    ++allocator->stats.elastic_slot_owner_locality_run_miss;
    return;
  }

  ++allocator->stats.elastic_slot_owner_locality_slot_match;
  if (hz6_allocator_descriptor_owner_equal(allocator, descriptor,
                                           allocator->owner.token)) {
    ++allocator->stats.elastic_slot_owner_locality_owner_match;
    ++allocator->stats.elastic_slot_owner_locality_would_set_owner;
    ++allocator->stats.elastic_slot_owner_locality_would_hit_owner;
  } else {
    ++allocator->stats.elastic_slot_owner_locality_owner_mismatch;
  }
}
#endif

void* hz6_front_reuse_transfer(Hz6Allocator* allocator,
                               uint16_t front_id,
                               uint16_t class_id,
                               Hz6AllocPath* path) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      !hz6_allocator_profile_transfer_first(allocator)) {
    return NULL;
  }

  Hz6TransferObject transfer;
  while (hz6_allocator_transfer_pop(allocator, class_id, &transfer)) {
    Hz6ObjectDescriptor* descriptor =
        (Hz6ObjectDescriptor*)transfer.descriptor;
    if (!hz6_allocator_activate_descriptor(allocator, descriptor, HZ6_STATE_TRANSFER_FREE, transfer.ptr,
            transfer.generation, hz6_allocator_owner_token(allocator))) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.transfer_reuse_invalid;
#endif
      continue;
    }
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.transfer_reuse_hit;
#if HZ6_ELASTIC_SOURCE_BLOCK_LOCALIZE_DRYRUN_L1
    hz6_front_note_source_block_localize_dryrun(allocator, descriptor);
#endif
#if HZ6_ELASTIC_SOURCE_RUN_LOCALITY_DRYRUN_L1
    hz6_front_note_source_run_locality_dryrun(allocator, descriptor);
#endif
#if HZ6_ELASTIC_SLOT_OWNER_LOCALITY_DRYRUN_L1
    hz6_front_note_slot_owner_locality_dryrun(allocator, descriptor);
#endif
#if HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1
    hz6_front_note_slot_owner_sparse_meta(allocator, descriptor);
#endif
#endif
    if (path) {
      *path = HZ6_ALLOC_PATH_TRANSFER_REUSE;
    } else {
      hz6_allocator_note_front_alloc_path(allocator, front_id,
                                          HZ6_ALLOC_PATH_TRANSFER_REUSE);
    }
    hz6_allocator_note_transfer_pop(allocator);
    return transfer.ptr;
  }

  return NULL;
}
