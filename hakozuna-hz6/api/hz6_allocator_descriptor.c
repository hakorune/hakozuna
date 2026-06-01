#include "hz6_allocator.h"

#if HZ6_DIAGNOSTIC_PROBES
static void hz6_allocator_record_descriptor_failure_state(
    Hz6Allocator* allocator) {
  size_t active = 0;
  size_t local_free = 0;
  size_t transfer_free = 0;
  size_t remote_pending = 0;
  size_t central_free = 0;
  size_t released = 0;
  size_t orphan = 0;
  size_t dead_with_ptr = 0;
  size_t frontcache_total = 0;
  size_t frontcache_largest_bin = 0;
  size_t frontcache_nonempty_bins = 0;

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    const Hz6ObjectDescriptor* descriptor = &allocator->descriptors[i];
    if (!descriptor->ptr) {
      continue;
    }
    switch (descriptor->state) {
      case HZ6_STATE_ACTIVE:
        ++active;
        break;
      case HZ6_STATE_LOCAL_FREE:
        ++local_free;
        break;
      case HZ6_STATE_TRANSFER_FREE:
        ++transfer_free;
        break;
      case HZ6_STATE_REMOTE_PENDING:
        ++remote_pending;
        break;
      case HZ6_STATE_CENTRAL_FREE:
        ++central_free;
        break;
      case HZ6_STATE_RELEASED:
        ++released;
        break;
      case HZ6_STATE_ORPHAN:
        ++orphan;
        break;
      case HZ6_STATE_DESCRIPTOR_RESERVED:
        break;
      case HZ6_STATE_DEAD:
      default:
        ++dead_with_ptr;
        break;
    }
  }
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    size_t count = allocator->frontcache_bins[i].count;
    frontcache_total += count;
    if (count > frontcache_largest_bin) {
      frontcache_largest_bin = count;
    }
    if (count != 0) {
      ++frontcache_nonempty_bins;
    }
  }

  if (active > allocator->stats.descriptor_fail_active_max) {
    allocator->stats.descriptor_fail_active_max = active;
  }
  if (local_free > allocator->stats.descriptor_fail_local_free_max) {
    allocator->stats.descriptor_fail_local_free_max = local_free;
  }
  if (transfer_free > allocator->stats.descriptor_fail_transfer_free_max) {
    allocator->stats.descriptor_fail_transfer_free_max = transfer_free;
  }
  if (remote_pending > allocator->stats.descriptor_fail_remote_pending_max) {
    allocator->stats.descriptor_fail_remote_pending_max = remote_pending;
  }
  if (central_free > allocator->stats.descriptor_fail_central_free_max) {
    allocator->stats.descriptor_fail_central_free_max = central_free;
  }
  if (released > allocator->stats.descriptor_fail_released_max) {
    allocator->stats.descriptor_fail_released_max = released;
  }
  if (orphan > allocator->stats.descriptor_fail_orphan_max) {
    allocator->stats.descriptor_fail_orphan_max = orphan;
  }
  if (dead_with_ptr > allocator->stats.descriptor_fail_dead_with_ptr_max) {
    allocator->stats.descriptor_fail_dead_with_ptr_max = dead_with_ptr;
  }
  if (frontcache_total >
      allocator->stats.descriptor_fail_frontcache_total_max) {
    allocator->stats.descriptor_fail_frontcache_total_max = frontcache_total;
  }
  if (frontcache_largest_bin >
      allocator->stats.descriptor_fail_frontcache_largest_bin_max) {
    allocator->stats.descriptor_fail_frontcache_largest_bin_max =
        frontcache_largest_bin;
  }
  if (frontcache_nonempty_bins >
      allocator->stats.descriptor_fail_frontcache_nonempty_bins_max) {
    allocator->stats.descriptor_fail_frontcache_nonempty_bins_max =
        frontcache_nonempty_bins;
  }
}
#endif

Hz6ObjectDescriptor* hz6_allocator_find_free_descriptor(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return NULL;
  }
#if HZ6_DIAGNOSTIC_PROBES
  size_t probes = 0;
#endif
  size_t start = allocator->next_descriptor_index;
  if (start >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    start = 0;
  }
  for (size_t offset = 0; offset < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++offset) {
#if HZ6_DIAGNOSTIC_PROBES
    ++probes;
#endif
    size_t i = start + offset;
    if (i >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
      i -= HZ6_OBJECT_DESCRIPTOR_CAPACITY;
    }
    if (!allocator->descriptors[i].ptr
#if HZ6_DESCRIPTOR_MATERIALIZE_RESERVE_L1
        && allocator->descriptors[i].state == HZ6_STATE_DEAD
#endif
    ) {
      allocator->next_descriptor_index = i + 1;
      if (allocator->next_descriptor_index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
        allocator->next_descriptor_index = 0;
      }
#if HZ6_DIAGNOSTIC_PROBES
      allocator->stats.descriptor_probe_total += probes;
      if (probes > allocator->stats.descriptor_probe_max) {
        allocator->stats.descriptor_probe_max = probes;
      }
#endif
      return &allocator->descriptors[i];
    }
  }
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.descriptor_probe_total += probes;
  if (probes > allocator->stats.descriptor_probe_max) {
    allocator->stats.descriptor_probe_max = probes;
  }
#endif
  ++allocator->stats.descriptor_exhausted;
#if HZ6_DIAGNOSTIC_PROBES
  hz6_allocator_record_descriptor_failure_state(allocator);
#endif
  return NULL;
}

void hz6_allocator_note_descriptor_frontcache_reuse_dryrun(
    Hz6Allocator* allocator,
    uint16_t requested_class_id) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || requested_class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }

  size_t requested_count =
      allocator->frontcache_bins[requested_class_id].count;
  size_t donor_total = 0;
  size_t largest_donor = 0;
  size_t donor_bins = 0;

  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    if (i == requested_class_id) {
      continue;
    }
    size_t count = allocator->frontcache_bins[i].count;
    donor_total += count;
    if (count != 0) {
      ++donor_bins;
    }
    if (count > largest_donor) {
      largest_donor = count;
    }
  }

  ++allocator->stats.descriptor_frontcache_reuse_dryrun_calls;
  if (requested_count != 0) {
    ++allocator->stats.descriptor_frontcache_reuse_requested_nonempty;
  }
  allocator->stats.descriptor_frontcache_reuse_requested_total +=
      requested_count;
  allocator->stats.descriptor_frontcache_reuse_donor_total += donor_total;
  if (largest_donor >
      allocator->stats.descriptor_frontcache_reuse_largest_donor_max) {
    allocator->stats.descriptor_frontcache_reuse_largest_donor_max =
        largest_donor;
  }
  if (donor_bins > allocator->stats.descriptor_frontcache_reuse_donor_bins_max) {
    allocator->stats.descriptor_frontcache_reuse_donor_bins_max = donor_bins;
  }
#else
  (void)allocator;
  (void)requested_class_id;
#endif
}

int hz6_allocator_activate_descriptor(Hz6ObjectDescriptor* descriptor,
                                      Hz6ObjectState expected,
                                      void* ptr,
                                      uint32_t generation,
                                      Hz6OwnerToken owner) {
  if (!descriptor || descriptor->state != expected) {
    return 0;
  }
  if (descriptor->ptr != ptr || descriptor->generation != generation) {
    return 0;
  }
  descriptor->state = HZ6_STATE_ACTIVE;
  descriptor->owner = owner;
  return 1;
}
