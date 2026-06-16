#ifndef HZ6_ALLOCATOR_API_DESCRIPTOR_H
#define HZ6_ALLOCATOR_API_DESCRIPTOR_H

#include <stddef.h>

#include "hz6_allocator_slot_owner_sparse.h"
#include "hz6_allocator_types.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator);

#if HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
int hz6_allocator_descriptor_is_depot(
    const Hz6ObjectDescriptor* descriptor);

Hz6OwnerToken hz6_allocator_descriptor_depot_owner(
    const Hz6ObjectDescriptor* descriptor);

void hz6_allocator_set_descriptor_depot_owner(
    Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner);
#endif

#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || \
    HZ6_OWNER_SOURCE_SIDE_META_L2
Hz6Allocator* hz6_allocator_descriptor_storage_owner(
    Hz6Allocator* observer,
    const Hz6ObjectDescriptor* descriptor,
    size_t* probe_count);
#endif

static inline size_t hz6_allocator_descriptor_index(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor ||
      descriptor < allocator->descriptors ||
      descriptor >= allocator->descriptors + HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    return HZ6_OBJECT_DESCRIPTOR_CAPACITY;
  }
  return (size_t)(descriptor - allocator->descriptors);
}

static inline int hz6_allocator_descriptor_belongs_to(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  return hz6_allocator_descriptor_index(allocator, descriptor) <
         HZ6_OBJECT_DESCRIPTOR_CAPACITY;
}

static inline void hz6_allocator_owner_source_side_meta_dryrun(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
#if HZ6_OWNER_SOURCE_SIDE_META_DRYRUN && HZ6_DIAGNOSTIC_PROBES
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
  if (!mutable_allocator || !descriptor) {
    return;
  }
  ++mutable_allocator->stats.owner_source_side_meta_lookup;
  if (hz6_allocator_descriptor_belongs_to(mutable_allocator, descriptor)) {
    ++mutable_allocator->stats.owner_source_side_meta_local;
    return;
  }

  size_t probes = 0;
  Hz6Allocator* storage = hz6_allocator_descriptor_storage_owner(
      mutable_allocator, descriptor, &probes);
  mutable_allocator->stats.owner_source_side_meta_probe_total += probes;
  if (probes > mutable_allocator->stats.owner_source_side_meta_probe_max) {
    mutable_allocator->stats.owner_source_side_meta_probe_max = probes;
  }
  if (storage) {
    ++mutable_allocator->stats.owner_source_side_meta_foreign;
  } else {
    ++mutable_allocator->stats.owner_source_side_meta_miss;
  }
#else
  (void)allocator;
  (void)descriptor;
#endif
}

static inline Hz6Allocator* hz6_allocator_owner_source_side_meta_storage(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
#if HZ6_OWNER_SOURCE_SIDE_META_L2
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
#if !HZ6_DIAGNOSTIC_PROBES
  (void)mutable_allocator;
#endif
#if HZ6_DIAGNOSTIC_PROBES
  if (mutable_allocator) {
    ++mutable_allocator->stats.owner_source_side_meta_l2_lookup;
  }
#endif
  if (!descriptor || !descriptor->source_block) {
#if HZ6_DIAGNOSTIC_PROBES
    if (mutable_allocator) {
      ++mutable_allocator->stats.owner_source_side_meta_l2_miss_no_block;
    }
#endif
    return NULL;
  }
  if (!hz6_source_block_active(descriptor->source_block) ||
      !descriptor->source_block->owner_source_storage_allocator) {
#if HZ6_DIAGNOSTIC_PROBES
    if (mutable_allocator) {
      ++mutable_allocator->stats.owner_source_side_meta_l2_miss_inactive;
    }
#endif
    return NULL;
  }
#if HZ6_ELASTIC_DEPOT_SLOT_LOCALIZE_L1
  {
    Hz6Allocator* slot_storage =
        hz6_allocator_elastic_slot_owner_sparse_storage(
            mutable_allocator, descriptor);
    if (slot_storage) {
#if HZ6_DIAGNOSTIC_PROBES
      if (mutable_allocator) {
        ++mutable_allocator->stats.owner_source_side_meta_l2_hit;
      }
#endif
      return slot_storage;
    }
  }
#endif
  Hz6Allocator* storage =
      descriptor->source_block->owner_source_storage_allocator;
#if HZ6_OWNER_SOURCE_SIDE_META_DRYRUN && HZ6_DIAGNOSTIC_PROBES
  if (mutable_allocator) {
    Hz6Allocator* scanned =
        hz6_allocator_descriptor_storage_owner(mutable_allocator, descriptor,
                                               NULL);
    if (scanned && scanned != storage) {
      ++mutable_allocator->stats.owner_source_side_meta_l2_storage_mismatch;
    }
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  if (mutable_allocator) {
    ++mutable_allocator->stats.owner_source_side_meta_l2_hit;
  }
#endif
  return storage;
#else
  (void)allocator;
  (void)descriptor;
  return NULL;
#endif
}

static inline uint32_t hz6_descriptor_pack_owner16(Hz6OwnerToken owner) {
  if (owner.slot > UINT16_MAX || owner.generation > UINT16_MAX) {
    return 0;
  }
  return ((uint32_t)owner.generation << 16) | (uint32_t)owner.slot;
}

static inline Hz6OwnerToken hz6_descriptor_unpack_owner16(uint32_t packed) {
  Hz6OwnerToken owner;
  owner.slot = packed & 0xffffu;
  owner.generation = (packed >> 16) & 0xffffu;
  return owner;
}

static inline Hz6OwnerToken hz6_allocator_descriptor_owner(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor) {
  hz6_allocator_owner_source_side_meta_dryrun(allocator, descriptor);
#if HZ6_DESCRIPTOR_DEPOT_OWNER_DIRECT_FASTPATH_L1 && \
    HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
  if (hz6_allocator_descriptor_is_depot(descriptor)) {
    return hz6_allocator_descriptor_depot_owner(descriptor);
  }
#endif
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1 || HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
  const Hz6Allocator* storage = allocator;
#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
  if (!hz6_allocator_descriptor_belongs_to(storage, descriptor)) {
    storage = hz6_allocator_owner_source_side_meta_storage(allocator,
                                                           descriptor);
    if (!storage) {
      storage = hz6_allocator_descriptor_storage_owner(
          (Hz6Allocator*)allocator, descriptor, NULL);
    }
  }
#endif
  size_t index = hz6_allocator_descriptor_index(storage, descriptor);
  if (index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
#if HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
    if (hz6_allocator_descriptor_is_depot(descriptor)) {
      return hz6_allocator_descriptor_depot_owner(descriptor);
    }
#endif
    return (Hz6OwnerToken){0};
  }
  return hz6_descriptor_unpack_owner16(
      storage->descriptor_side_owner16[index]);
#else
  (void)allocator;
  return descriptor ? descriptor->owner : (Hz6OwnerToken){0};
#endif
}

static inline void hz6_allocator_set_descriptor_owner(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner) {
#if HZ6_DESCRIPTOR_DEPOT_OWNER_DIRECT_FASTPATH_L1 && \
    HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
  if (hz6_allocator_descriptor_is_depot(descriptor)) {
    hz6_allocator_set_descriptor_depot_owner(descriptor, owner);
    return;
  }
#endif
#if HZ6_DESCRIPTOR_SIDE_OWNER16_L1 || HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
  Hz6Allocator* storage = allocator;
#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1
  if (!hz6_allocator_descriptor_belongs_to(storage, descriptor)) {
    storage = hz6_allocator_owner_source_side_meta_storage(allocator,
                                                           descriptor);
    if (!storage) {
      storage = hz6_allocator_descriptor_storage_owner(allocator, descriptor,
                                                       NULL);
    }
  }
#endif
  size_t index = hz6_allocator_descriptor_index(storage, descriptor);
  if (index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
#if HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
    if (hz6_allocator_descriptor_is_depot(descriptor)) {
      hz6_allocator_set_descriptor_depot_owner(descriptor, owner);
    }
#endif
    return;
  }
  storage->descriptor_side_owner16[index] =
      hz6_descriptor_pack_owner16(owner);
#else
  (void)allocator;
  if (descriptor) {
    descriptor->owner = owner;
  }
#endif
}

typedef enum Hz6OwnerEqualSite {
  HZ6_OWNER_EQUAL_SITE_UNKNOWN = 0,
  HZ6_OWNER_EQUAL_SITE_FREE,
  HZ6_OWNER_EQUAL_SITE_REMOTE_FREE,
  HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE,
  HZ6_OWNER_EQUAL_SITE_VISIBLE_LOOKUP,
  HZ6_OWNER_EQUAL_SITE_TRANSFER_LOCALITY,
  HZ6_OWNER_EQUAL_SITE_LARGE_CENTRAL,
  HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING,
  HZ6_OWNER_EQUAL_SITE_OWNER_DEAD,
  HZ6_OWNER_EQUAL_SITE_SAME_OWNER_FAST
} Hz6OwnerEqualSite;

static inline void hz6_allocator_note_owner_equal_site(
    const Hz6Allocator* allocator,
    Hz6OwnerEqualSite site) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_OWNER_EQUAL_CALLSITE_DRYRUN_L1
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
  if (!mutable_allocator) {
    return;
  }
  switch (site) {
    case HZ6_OWNER_EQUAL_SITE_FREE:
      ++mutable_allocator->stats.owner_equal_site_free;
      break;
    case HZ6_OWNER_EQUAL_SITE_REMOTE_FREE:
      ++mutable_allocator->stats.owner_equal_site_remote_free;
      break;
    case HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE:
      ++mutable_allocator->stats.owner_equal_site_local_cache;
      break;
    case HZ6_OWNER_EQUAL_SITE_VISIBLE_LOOKUP:
      ++mutable_allocator->stats.owner_equal_site_visible_lookup;
      break;
    case HZ6_OWNER_EQUAL_SITE_TRANSFER_LOCALITY:
      ++mutable_allocator->stats.owner_equal_site_transfer_locality;
      break;
    case HZ6_OWNER_EQUAL_SITE_LARGE_CENTRAL:
      ++mutable_allocator->stats.owner_equal_site_large_central;
      break;
    case HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING:
      ++mutable_allocator->stats.owner_equal_site_remote_pending;
      break;
    case HZ6_OWNER_EQUAL_SITE_OWNER_DEAD:
      ++mutable_allocator->stats.owner_equal_site_owner_dead;
      break;
    case HZ6_OWNER_EQUAL_SITE_SAME_OWNER_FAST:
      ++mutable_allocator->stats.owner_equal_site_same_owner_fast;
      break;
    case HZ6_OWNER_EQUAL_SITE_UNKNOWN:
    default:
      ++mutable_allocator->stats.owner_equal_site_unknown;
      break;
  }
#else
  (void)allocator;
  (void)site;
#endif
}

static inline void hz6_allocator_note_free_local_cache_owner_predicate(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerEqualSite site) {
#if HZ6_DIAGNOSTIC_PROBES && HZ6_FREE_LOCAL_CACHE_OWNER_PREDICATE_DRYRUN_L0
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
  if (!mutable_allocator ||
      (site != HZ6_OWNER_EQUAL_SITE_FREE &&
       site != HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE)) {
    return;
  }
  ++mutable_allocator->stats.flc_owner_predicate_probe;
  if (site == HZ6_OWNER_EQUAL_SITE_FREE) {
    ++mutable_allocator->stats.flc_owner_predicate_site_free;
  } else {
    ++mutable_allocator->stats.flc_owner_predicate_site_local_cache;
  }
#if HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
  if (hz6_allocator_descriptor_is_depot(descriptor)) {
    ++mutable_allocator->stats.flc_owner_predicate_depot_descriptor;
  }
#endif
  if (hz6_allocator_descriptor_belongs_to(mutable_allocator, descriptor)) {
    ++mutable_allocator->stats.flc_owner_predicate_local_descriptor;
  } else {
    ++mutable_allocator->stats.flc_owner_predicate_foreign_descriptor;
  }
  if (!descriptor || !descriptor->source_block) {
    ++mutable_allocator->stats.flc_owner_predicate_no_source_block;
    return;
  }
  ++mutable_allocator->stats.flc_owner_predicate_source_block;
  if (hz6_source_block_active(descriptor->source_block)) {
    ++mutable_allocator->stats.flc_owner_predicate_source_block_active;
  }
  if (hz6_source_block_route_shared(descriptor->source_block)) {
    ++mutable_allocator->stats.flc_owner_predicate_source_block_shared;
  }
  if (hz6_source_block_run_active(descriptor->source_block)) {
    ++mutable_allocator->stats.flc_owner_predicate_source_run_active;
  }
  if (descriptor->source_block->source_release) {
    ++mutable_allocator->stats.flc_owner_predicate_source_release;
  }
#else
  (void)allocator;
  (void)descriptor;
  (void)site;
#endif
}

static inline int hz6_allocator_depot_descriptor_owner_equal_fastpath(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner,
    Hz6OwnerEqualSite site,
    int* answered) {
#if HZ6_DEPOT_DESCRIPTOR_OWNER_EQUAL_FASTPATH_L1 && \
    HZ6_ELASTIC_DESCRIPTOR_OVERFLOW_L1
  Hz6Allocator* mutable_allocator = (Hz6Allocator*)allocator;
#if !HZ6_DIAGNOSTIC_PROBES
  (void)mutable_allocator;
#endif
  if (answered) {
    *answered = 0;
  }
  if (site != HZ6_OWNER_EQUAL_SITE_FREE &&
      site != HZ6_OWNER_EQUAL_SITE_LOCAL_CACHE) {
#if HZ6_DIAGNOSTIC_PROBES
    if (mutable_allocator) {
      ++mutable_allocator->stats.depot_owner_equal_fastpath_other_site;
    }
#endif
    return 0;
  }
#if HZ6_DIAGNOSTIC_PROBES
  if (mutable_allocator) {
    ++mutable_allocator->stats.depot_owner_equal_fastpath_probe;
  }
#endif
  if (!hz6_allocator_descriptor_is_depot(descriptor)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (mutable_allocator) {
      ++mutable_allocator->stats.depot_owner_equal_fastpath_fallback;
    }
#endif
    return 0;
  }
  if (answered) {
    *answered = 1;
  }
  int equal =
      hz6_owner_equal(hz6_allocator_descriptor_depot_owner(descriptor), owner);
#if HZ6_DIAGNOSTIC_PROBES
  if (mutable_allocator) {
    if (equal) {
      ++mutable_allocator->stats.depot_owner_equal_fastpath_hit;
    } else {
      ++mutable_allocator->stats.depot_owner_equal_fastpath_miss;
    }
  }
#endif
  return equal;
#else
  (void)allocator;
  (void)descriptor;
  (void)owner;
  (void)site;
  if (answered) {
    *answered = 0;
  }
  return 0;
#endif
}

static inline int hz6_allocator_descriptor_owner_equal_at(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner,
    Hz6OwnerEqualSite site) {
  hz6_allocator_note_owner_equal_site(allocator, site);
  hz6_allocator_note_free_local_cache_owner_predicate(
      allocator, descriptor, site);
  int depot_owner_equal_answered = 0;
  int depot_owner_equal = hz6_allocator_depot_descriptor_owner_equal_fastpath(
      allocator, descriptor, owner, site, &depot_owner_equal_answered);
  if (depot_owner_equal_answered) {
    return depot_owner_equal;
  }
#if HZ6_ELASTIC_SLOT_OWNER_LOGICAL_FASTPATH_L1
  if (hz6_allocator_elastic_slot_owner_logical_owner_match(
          (Hz6Allocator*)allocator, descriptor, owner)) {
    return 1;
  }
#endif
  int equal =
      hz6_owner_equal(hz6_allocator_descriptor_owner(allocator, descriptor),
                      owner);
#if HZ6_DIAGNOSTIC_PROBES && HZ6_ELASTIC_SLOT_OWNER_CONSUMER_DRYRUN_L1
  hz6_allocator_elastic_slot_owner_consumer_dryrun(allocator, descriptor,
                                                   owner, equal);
#endif
  return equal;
}

static inline int hz6_allocator_descriptor_owner_equal(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    Hz6OwnerToken owner) {
  return hz6_allocator_descriptor_owner_equal_at(
      allocator, descriptor, owner, HZ6_OWNER_EQUAL_SITE_UNKNOWN);
}

void hz6_allocator_reset_descriptor_available(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

int hz6_allocator_activate_descriptor(Hz6Allocator* allocator,
                                      Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner);

int hz6_allocator_recommit_source_block_if_needed(Hz6Allocator* allocator,
                                                  Hz6SourceBlock* block);

int hz6_allocator_prepare_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    size_t bytes,
    void* source_ptr,
    size_t source_bytes,
    Hz6SourceBlock* source_block,
    uint16_t class_id,
    Hz6SourceKind source_kind,
    int (*source_release)(void* ptr, size_t bytes),
    Hz6ObjectState state);

int hz6_allocator_cache_active_descriptor(Hz6Allocator* allocator,
                                          Hz6ObjectDescriptor* descriptor,
                                          void* ptr);

/* Trusted-owner variants are for paths that already proved local ownership.
 * They still validate pointer/generation/state but skip owner equality. */
int hz6_allocator_cache_active_descriptor_trusted_owner(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr);

int hz6_allocator_activate_local_descriptor_trusted_owner(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr,
    uint32_t generation);

int hz6_allocator_remote_free_active_descriptor(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    void* ptr);

int hz6_allocator_release_descriptor_source(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

int hz6_allocator_descriptor_source_meta(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    void** source_ptr,
    size_t* source_bytes,
    Hz6SourceReleaseFn* source_release);

int hz6_allocator_descriptor_has_source_release(
    const Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor);

int hz6_allocator_detach_descriptor_keep_source_slot(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

int hz6_allocator_reserve_descriptor_keep_source_slot(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor);

void hz6_allocator_note_descriptor_frontcache_reuse_dryrun(
    Hz6Allocator* allocator,
    uint16_t requested_class_id);

void hz6_allocator_note_descgov_descriptor_fail(
    Hz6Allocator* allocator,
    uint16_t requested_class_id);

int hz6_allocator_descgov_descriptor_available(
    const Hz6Allocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
