#include "hz6_front_util.h"
#include "../api/hz6_allocator_slot_owner_sparse.h"

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
  if (hz6_source_block_ref_count(descriptor->source_block) > 1) {
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
  if (hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_TRANSFER_LOCALITY)) {
    ++allocator->stats.elastic_slot_owner_locality_owner_match;
    ++allocator->stats.elastic_slot_owner_locality_would_set_owner;
    ++allocator->stats.elastic_slot_owner_locality_would_hit_owner;
  } else {
    ++allocator->stats.elastic_slot_owner_locality_owner_mismatch;
  }
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_DEPOT_DRAIN_DRYRUN_L1
static void hz6_front_note_elastic_depot_drain_dryrun(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor || !descriptor->source_block ||
      !hz6_allocator_source_block_is_elastic_depot(
          descriptor->source_block)) {
    return;
  }

  Hz6SourceBlock* block = descriptor->source_block;
  ++allocator->stats.elastic_depot_drain_probe;

  int storage_match = 0;
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  storage_match = block->owner_source_storage_allocator == allocator;
#endif
  if (storage_match) {
    ++allocator->stats.elastic_depot_drain_storage_match;
  } else {
    ++allocator->stats.elastic_depot_drain_storage_mismatch;
  }

  if (hz6_source_block_ref_count(block) <= 1) {
    ++allocator->stats.elastic_depot_drain_ref_exclusive;
  } else {
    ++allocator->stats.elastic_depot_drain_ref_shared;
    ++allocator->stats.elastic_depot_drain_would_block_whole_localize;
  }

  int run_match = 0;
  if (!hz6_source_block_run_active(block)) {
    ++allocator->stats.elastic_depot_drain_run_miss;
  } else if (block->run_class_id != descriptor->class_id ||
             block->run_slot_bytes != descriptor->bytes) {
    ++allocator->stats.elastic_depot_drain_class_mismatch;
  } else if (!hz6_allocator_source_run_contains_slot(
                 block, descriptor->ptr, descriptor->class_id,
                 descriptor->bytes)) {
    ++allocator->stats.elastic_depot_drain_slot_mismatch;
  } else {
    run_match = 1;
    ++allocator->stats.elastic_depot_drain_run_match;
  }

  if (hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_TRANSFER_LOCALITY)) {
    ++allocator->stats.elastic_depot_drain_owner_match;
  } else {
    ++allocator->stats.elastic_depot_drain_owner_mismatch;
  }

  if (!storage_match && run_match) {
    ++allocator->stats.elastic_depot_drain_would_slot_localize;
    if (hz6_source_block_ref_count(block) > 1) {
      ++allocator->stats.elastic_depot_drain_would_keep_shared;
    }
  }
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && \
    (HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_DRYRUN_L1 || \
     HZ6_ELASTIC_DEPOT_ROUTE_REPLACE_DRYRUN_L1)
static int hz6_front_has_local_descriptor_slot(const Hz6Allocator* allocator) {
  if (!allocator) {
    return 0;
  }
#if HZ6_DESCRIPTOR_AVAIL_COUNT_L1
  return allocator->descriptor_available_count != 0;
#else
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    const Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
        && descriptor->state == HZ6_STATE_DEAD
#endif
    ) {
      return 1;
    }
  }
  return 0;
#endif
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && \
    HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_DRYRUN_L1
static void hz6_front_note_elastic_depot_descriptor_rehome_dryrun(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor) {
    return;
  }
  ++allocator->stats.elastic_depot_descriptor_rehome_probe;
  if (!hz6_allocator_descriptor_is_depot(descriptor)) {
    return;
  }
  ++allocator->stats.elastic_depot_descriptor_rehome_depot_descriptor;
  if (hz6_allocator_descriptor_belongs_to(allocator, descriptor)) {
    ++allocator->stats.elastic_depot_descriptor_rehome_already_local;
    return;
  }
  if (!descriptor->source_block ||
      !hz6_allocator_source_block_is_elastic_depot(descriptor->source_block) ||
      !hz6_allocator_source_run_contains_slot(descriptor->source_block,
                                             descriptor->ptr,
                                             descriptor->class_id,
                                             descriptor->bytes)) {
    ++allocator->stats.elastic_depot_descriptor_rehome_run_mismatch;
    return;
  }
  ++allocator->stats.elastic_depot_descriptor_rehome_run_match;
  if (hz6_front_has_local_descriptor_slot(allocator)) {
    ++allocator->stats
          .elastic_depot_descriptor_rehome_local_descriptor_available;
    ++allocator->stats.elastic_depot_descriptor_rehome_would_rehome;
  } else {
    ++allocator->stats.elastic_depot_descriptor_rehome_no_local_descriptor;
  }
}
#endif

#if HZ6_ELASTIC_DEPOT_ROUTE_REPLACE_DRYRUN_L1 || \
    HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_L1
static Hz6Allocator* hz6_front_depot_route_origin_allocator(
    const Hz6ObjectDescriptor* descriptor) {
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  if (!descriptor || !descriptor->source_block) {
    return NULL;
  }
  return descriptor->source_block->owner_source_storage_allocator;
#else
  (void)descriptor;
  return NULL;
#endif
}
#endif

#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_DEPOT_ROUTE_REPLACE_DRYRUN_L1
static int hz6_front_depot_route_result_matches_descriptor(
    Hz6Allocator* allocator,
    Hz6RouteResult route,
    const Hz6ObjectDescriptor* descriptor,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || !descriptor || route.kind != HZ6_ROUTE_VALID) {
    return 0;
  }
  int ok = 1;
  if (route.descriptor == descriptor) {
    ++allocator->stats.elastic_depot_route_replace_descriptor_match;
  } else {
    ++allocator->stats.elastic_depot_route_replace_descriptor_mismatch;
    ok = 0;
  }
  if (route.generation == descriptor->generation) {
    ++allocator->stats.elastic_depot_route_replace_generation_match;
  } else {
    ++allocator->stats.elastic_depot_route_replace_generation_mismatch;
    ok = 0;
  }
  if (route.front_id == front_id && route.class_id == class_id &&
      descriptor->class_id == class_id) {
    ++allocator->stats.elastic_depot_route_replace_front_class_match;
  } else {
    ++allocator->stats.elastic_depot_route_replace_front_class_mismatch;
    ok = 0;
  }
  return ok;
}

static void hz6_front_note_elastic_depot_route_replace_dryrun(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || !descriptor) {
    return;
  }
  ++allocator->stats.elastic_depot_route_replace_probe;
  if (!hz6_allocator_descriptor_is_depot(descriptor)) {
    ++allocator->stats.elastic_depot_route_replace_would_rollback;
    return;
  }
  ++allocator->stats.elastic_depot_route_replace_depot_descriptor;

  Hz6SourceBlock* block = descriptor->source_block;
  if (!block || !hz6_allocator_source_block_is_elastic_depot(block) ||
      !hz6_allocator_source_run_contains_slot(block, descriptor->ptr,
                                             descriptor->class_id,
                                             descriptor->bytes)) {
    ++allocator->stats.elastic_depot_route_replace_run_mismatch;
    ++allocator->stats.elastic_depot_route_replace_would_rollback;
    return;
  }
  ++allocator->stats.elastic_depot_route_replace_run_match;

  int old_route_ok = 0;
  Hz6Allocator* origin = hz6_front_depot_route_origin_allocator(descriptor);
  if (!origin) {
    ++allocator->stats.elastic_depot_route_replace_origin_missing;
  } else {
    Hz6RouteResult old_route =
        hz6_allocator_route_lookup_exact(origin, descriptor->ptr);
    if (old_route.kind == HZ6_ROUTE_VALID) {
      ++allocator->stats.elastic_depot_route_replace_old_route_found;
      old_route_ok = hz6_front_depot_route_result_matches_descriptor(
          allocator, old_route, descriptor, front_id, class_id);
    } else if (old_route.kind == HZ6_ROUTE_INVALID) {
      ++allocator->stats.elastic_depot_route_replace_old_route_invalid;
    } else {
      ++allocator->stats.elastic_depot_route_replace_old_route_missing;
    }
  }

  int current_route_ok = 0;
  Hz6RouteResult current_route =
      hz6_allocator_route_lookup_exact(allocator, descriptor->ptr);
  if (current_route.kind == HZ6_ROUTE_MISS) {
    ++allocator->stats.elastic_depot_route_replace_current_route_empty;
    current_route_ok = 1;
  } else if (current_route.kind == HZ6_ROUTE_VALID &&
             current_route.descriptor == descriptor &&
             current_route.generation == descriptor->generation &&
             current_route.front_id == front_id &&
             current_route.class_id == class_id) {
    ++allocator->stats.elastic_depot_route_replace_current_route_same;
    current_route_ok = 1;
  } else {
    ++allocator->stats.elastic_depot_route_replace_current_route_conflict;
  }

  if ((old_route_ok || current_route.kind == HZ6_ROUTE_VALID) &&
      current_route_ok && hz6_front_has_local_descriptor_slot(allocator)) {
    ++allocator->stats.elastic_depot_route_replace_would_commit;
  } else {
    ++allocator->stats.elastic_depot_route_replace_would_rollback;
  }
}
#endif

#if HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_L1
static Hz6ObjectDescriptor* hz6_front_try_elastic_depot_descriptor_rehome(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    uint16_t front_id,
    uint16_t class_id) {
  if (!allocator || !descriptor) {
    return descriptor;
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.elastic_depot_descriptor_rehome_l1_attempt;
#endif
  if (!hz6_allocator_descriptor_is_depot(descriptor) ||
      hz6_allocator_descriptor_belongs_to(allocator, descriptor) ||
      !descriptor->source_block ||
      !hz6_allocator_source_block_is_elastic_depot(descriptor->source_block) ||
      !hz6_allocator_source_run_contains_slot(descriptor->source_block,
                                             descriptor->ptr,
                                             descriptor->class_id,
                                             descriptor->bytes)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.elastic_depot_descriptor_rehome_l1_ineligible;
#endif
    return descriptor;
  }

#if HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET_L1
  if (allocator->depot_descriptor_rehome_budget_used >=
      HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.elastic_depot_descriptor_rehome_l1_budget_denied;
#endif
    return descriptor;
  }
#endif

  Hz6ObjectDescriptor* local = hz6_allocator_find_free_descriptor(allocator);
  if (!local || !hz6_allocator_descriptor_belongs_to(allocator, local)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats
          .elastic_depot_descriptor_rehome_l1_no_local_descriptor;
#endif
    return descriptor;
  }

  void* ptr = descriptor->ptr;
  uint32_t bytes = descriptor->bytes;
  uint32_t generation = descriptor->generation;
  Hz6SourceBlock* block = descriptor->source_block;
  if (!hz6_allocator_prepare_descriptor(
          allocator, local, ptr, bytes, block->ptr, block->bytes, block,
          descriptor->class_id, hz6_source_block_source_kind(block),
          block->source_release, HZ6_STATE_ACTIVE)) {
    hz6_allocator_reset_descriptor_available(allocator, local);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.elastic_depot_descriptor_rehome_l1_prepare_fail;
#endif
    return descriptor;
  }
  local->generation = generation;

  if (!hz6_allocator_route_replace_exact_descriptor(
          allocator, ptr, bytes, front_id, class_id, generation, descriptor,
          generation, local)) {
    hz6_allocator_reset_descriptor_available(allocator, local);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats
          .elastic_depot_descriptor_rehome_l1_route_replace_fail;
#endif
    return descriptor;
  }

  Hz6Allocator* origin = hz6_front_depot_route_origin_allocator(descriptor);
  if (!hz6_allocator_detach_descriptor_keep_source_slot(
          origin ? origin : allocator, descriptor)) {
    (void)hz6_allocator_route_replace_exact_descriptor(
        allocator, ptr, bytes, front_id, class_id, generation, local,
        generation, descriptor);
    hz6_allocator_reset_descriptor_available(allocator, local);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.elastic_depot_descriptor_rehome_l1_detach_fail;
    ++allocator->stats.elastic_depot_descriptor_rehome_l1_rollback;
#endif
    return descriptor;
  }

#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.elastic_depot_descriptor_rehome_l1_success;
#endif
#if HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_BUDGET_L1
  ++allocator->depot_descriptor_rehome_budget_used;
#endif
  return local;
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
#if HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_L1
    descriptor = hz6_front_try_elastic_depot_descriptor_rehome(
        allocator, descriptor, front_id, class_id);
#endif
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
#if HZ6_ELASTIC_DEPOT_DRAIN_DRYRUN_L1
    hz6_front_note_elastic_depot_drain_dryrun(allocator, descriptor);
#endif
#if HZ6_ELASTIC_DEPOT_DESCRIPTOR_REHOME_DRYRUN_L1
    hz6_front_note_elastic_depot_descriptor_rehome_dryrun(allocator,
                                                          descriptor);
#endif
#if HZ6_ELASTIC_DEPOT_ROUTE_REPLACE_DRYRUN_L1
    hz6_front_note_elastic_depot_route_replace_dryrun(
        allocator, descriptor, front_id, class_id);
#endif
#endif
#if HZ6_ELASTIC_DEPOT_SLOT_LOCALIZE_L1 || \
    HZ6_ELASTIC_DEPOT_SLOT_TRANSFER_SCOPED_L1
    hz6_allocator_elastic_depot_slot_localize(allocator, descriptor);
#endif
#if HZ6_ELASTIC_SLOT_OWNER_SPARSE_META_L1 && \
    (HZ6_DIAGNOSTIC_PROBES || HZ6_ELASTIC_SLOT_OWNER_LOGICAL_FASTPATH_L1)
    hz6_allocator_elastic_slot_owner_sparse_note(allocator, descriptor);
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
