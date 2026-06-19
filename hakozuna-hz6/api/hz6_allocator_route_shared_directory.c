#include "hz6_allocator_route_shared_directory.h"

#include "hz6_allocator_route_hash.h"

#include <stdatomic.h>
#include <stdint.h>

#if HZ6_SHARED_ROUTE_DIRECTORY_L1
typedef struct Hz6SharedRouteDirectoryEntry {
  _Atomic(unsigned int) sequence;
  _Atomic(uintptr_t) base;
  _Atomic(Hz6Allocator*) allocator;
  _Atomic(uintptr_t) descriptor;
  _Atomic(unsigned int) front_id;
  _Atomic(unsigned int) class_id;
  _Atomic(unsigned int) generation;
  _Atomic(unsigned int) owner_slot;
  _Atomic(unsigned int) owner_generation;
} Hz6SharedRouteDirectoryEntry;

static Hz6SharedRouteDirectoryEntry g_hz6_shared_route_directory
    [HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY];
static atomic_flag g_hz6_shared_route_directory_writer_locks
    [HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS];

#define HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE ((uintptr_t)UINTPTR_MAX)

static size_t hz6_shared_route_directory_index(uintptr_t base) {
  return hz6_route_directory_index(base, HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY);
}

static size_t hz6_shared_route_directory_lock_index(size_t start) {
  return start % HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS;
}

static void hz6_shared_route_directory_writer_lock(size_t lock_index) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  while (atomic_flag_test_and_set_explicit(
      &g_hz6_shared_route_directory_writer_locks[lock_index],
      memory_order_acquire)) {
  }
#else
  (void)lock_index;
#endif
}

static void hz6_shared_route_directory_writer_unlock(size_t lock_index) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  atomic_flag_clear_explicit(&g_hz6_shared_route_directory_writer_locks
                                 [lock_index],
                             memory_order_release);
#else
  (void)lock_index;
#endif
}

#include "hz6_allocator_route_shared_directory_entry.inc"

static Hz6SharedRouteLookup hz6_shared_route_directory_lookup_retry(void) {
  return hz6_shared_route_directory_lookup_result(
      HZ6_SHARED_ROUTE_LOOKUP_RETRY, hz6_route_miss());
}

static Hz6SharedRouteLookup hz6_shared_route_directory_lookup_stale(void) {
  return hz6_shared_route_directory_lookup_result(
      HZ6_SHARED_ROUTE_LOOKUP_STALE, hz6_route_miss());
}

static Hz6SharedRouteLookup hz6_shared_route_directory_lookup_valid(
    Hz6RouteResult route) {
  return hz6_shared_route_directory_lookup_result(
      HZ6_SHARED_ROUTE_LOOKUP_VALID, route);
}

size_t hz6_allocator_shared_route_directory_bytes(void) {
  size_t bytes = sizeof(g_hz6_shared_route_directory);
#if HZ6_ELASTIC_ROUTE_OVERFLOW_L1
  bytes += hz6_shared_route_range_directory_bytes();
#endif
  return bytes;
}

int hz6_shared_route_directory_register(Hz6Allocator* allocator,
                                        void* base,
                                        uint16_t front_id,
                                        uint16_t class_id,
                                        uint32_t generation,
                                        void* descriptor) {
  if (!allocator || !base || !descriptor) {
    return 0;
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  size_t start = hz6_shared_route_directory_index((uintptr_t)base);
  size_t lock_index = hz6_shared_route_directory_lock_index(start);
  hz6_shared_route_directory_writer_lock(lock_index);
  size_t tombstone_index = (size_t)-1;
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      if (tombstone_index == (size_t)-1) {
        tombstone_index = index;
      }
      continue;
    }
    if (current == 0 || current == (uintptr_t)base) {
      int reused_tombstone = current == 0 && tombstone_index != (size_t)-1;
      if (current == 0 && tombstone_index != (size_t)-1) {
        entry = &g_hz6_shared_route_directory[tombstone_index];
      }
      hz6_shared_route_directory_entry_publish(entry, allocator, base,
                                               front_id, class_id, generation,
                                               descriptor);
      if (reused_tombstone) {
        hz6_shared_route_directory_diag_tombstone_reused();
      }
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.shared_dir_register;
#endif
      hz6_shared_route_directory_writer_unlock(lock_index);
      return 1;
    }
  }
  if (tombstone_index != (size_t)-1) {
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[tombstone_index];
    hz6_shared_route_directory_entry_publish(entry, allocator, base,
                                             front_id, class_id, generation,
                                             descriptor);
    hz6_shared_route_directory_diag_tombstone_reused();
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_register;
#endif
    hz6_shared_route_directory_writer_unlock(lock_index);
    return 1;
  }
  hz6_shared_route_directory_writer_unlock(lock_index);
  return 0;
#else
  size_t start = hz6_shared_route_directory_index((uintptr_t)base);
  size_t tombstone_index = (size_t)-1;
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t expected = 0;
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      if (tombstone_index == (size_t)-1) {
        tombstone_index = index;
      }
      continue;
    }
    if (current != 0 && current != (uintptr_t)base) {
      continue;
    }
    if (current == 0) {
      if (tombstone_index != (size_t)-1) {
        entry = &g_hz6_shared_route_directory[tombstone_index];
        expected = HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE;
        if (!atomic_compare_exchange_strong_explicit(
                &entry->base,
                &expected,
                (uintptr_t)base,
                memory_order_acq_rel,
                memory_order_acquire)) {
          continue;
        }
        hz6_shared_route_directory_diag_tombstone_reused();
      } else if (!atomic_compare_exchange_strong_explicit(
                     &entry->base,
                     &expected,
                     (uintptr_t)base,
                     memory_order_acq_rel,
                     memory_order_acquire)) {
        if (expected != (uintptr_t)base) {
          continue;
        }
      }
    }
    atomic_store_explicit(&entry->descriptor,
                          (uintptr_t)descriptor,
                          memory_order_release);
    atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
    atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
    atomic_store_explicit(&entry->generation, generation, memory_order_release);
    atomic_store_explicit(&entry->owner_slot, allocator->owner.token.slot,
                          memory_order_release);
    atomic_store_explicit(&entry->owner_generation,
                          allocator->owner.token.generation,
                          memory_order_release);
    atomic_store_explicit(&entry->allocator, allocator, memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_register;
#endif
    return 1;
  }
  if (tombstone_index != (size_t)-1) {
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[tombstone_index];
    uintptr_t expected = HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE;
    if (atomic_compare_exchange_strong_explicit(&entry->base,
                                                &expected,
                                                (uintptr_t)base,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
      hz6_shared_route_directory_diag_tombstone_reused();
      atomic_store_explicit(&entry->descriptor,
                            (uintptr_t)descriptor,
                            memory_order_release);
      atomic_store_explicit(&entry->front_id, front_id, memory_order_release);
      atomic_store_explicit(&entry->class_id, class_id, memory_order_release);
      atomic_store_explicit(&entry->generation,
                            generation,
                            memory_order_release);
      atomic_store_explicit(&entry->owner_slot,
                            allocator->owner.token.slot,
                            memory_order_release);
      atomic_store_explicit(&entry->owner_generation,
                            allocator->owner.token.generation,
                            memory_order_release);
      atomic_store_explicit(&entry->allocator,
                            allocator,
                            memory_order_release);
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.shared_dir_register;
#endif
      return 1;
    }
  }
  return 0;
#endif
}

Hz6SharedRouteLookup hz6_shared_route_directory_lookup_snapshot(
    const void* ptr) {
  if (!ptr) {
    return hz6_shared_route_directory_lookup_miss();
  }
  uintptr_t key = (uintptr_t)ptr;
  size_t start = hz6_shared_route_directory_index(key);
  int saw_retry = 0;
  for (size_t probes = 0; probes < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
       ++probes) {
    size_t index = (start + probes) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    unsigned int sequence_before =
        atomic_load_explicit(&entry->sequence, memory_order_acquire);
    if ((sequence_before & 1u) != 0u) {
      saw_retry = 1;
      continue;
    }
#endif
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    if (current == 0) {
      unsigned int sequence_after =
          atomic_load_explicit(&entry->sequence, memory_order_acquire);
      if (sequence_before == sequence_after &&
          (sequence_after & 1u) == 0u) {
        return hz6_shared_route_directory_lookup_miss();
      }
      saw_retry = 1;
      continue;
    }
#else
    if (current == 0) {
      return hz6_shared_route_directory_lookup_miss();
    }
#endif
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      continue;
    }
    if (current != key) {
      continue;
    }
    Hz6Allocator* route_allocator =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    uintptr_t descriptor_value =
        atomic_load_explicit(&entry->descriptor, memory_order_acquire);
    if (!route_allocator || descriptor_value == 0) {
      return hz6_shared_route_directory_lookup_stale();
    }
    unsigned int front_id =
        atomic_load_explicit(&entry->front_id, memory_order_acquire);
    unsigned int class_id =
        atomic_load_explicit(&entry->class_id, memory_order_acquire);
    unsigned int generation =
        atomic_load_explicit(&entry->generation, memory_order_acquire);
    unsigned int owner_slot =
        atomic_load_explicit(&entry->owner_slot, memory_order_acquire);
    unsigned int owner_generation =
        atomic_load_explicit(&entry->owner_generation, memory_order_acquire);
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    unsigned int sequence_after =
        atomic_load_explicit(&entry->sequence, memory_order_acquire);
    if (sequence_before != sequence_after || (sequence_after & 1u) != 0u) {
      saw_retry = 1;
      continue;
    }
#endif
    const Hz6ObjectDescriptor* descriptor =
        (const Hz6ObjectDescriptor*)descriptor_value;
    if (route_allocator->owner.token.slot != (uint32_t)owner_slot ||
        route_allocator->owner.token.generation !=
            (uint32_t)owner_generation) {
      return hz6_shared_route_directory_lookup_stale();
    }
    if (!descriptor || descriptor->ptr != ptr ||
        descriptor->generation != (uint32_t)generation ||
        descriptor->state == HZ6_STATE_DEAD ||
        descriptor->state == HZ6_STATE_DESCRIPTOR_RESERVED) {
      return hz6_shared_route_directory_lookup_stale();
    }
    if (descriptor->source_block &&
        (!hz6_source_block_active(descriptor->source_block) ||
         !descriptor->source_block->ptr)) {
      return hz6_shared_route_directory_lookup_stale();
    }
    Hz6RouteResult route = hz6_route_valid((uint16_t)front_id,
                                           (uint16_t)class_id,
                                           (uint32_t)generation,
                                           (void*)descriptor_value);
    route.route_allocator = route_allocator;
    return hz6_shared_route_directory_lookup_valid(route);
  }
  return saw_retry ? hz6_shared_route_directory_lookup_retry()
                   : hz6_shared_route_directory_lookup_miss();
}

Hz6RouteResult hz6_shared_route_directory_lookup_raw(const void* ptr) {
  Hz6SharedRouteLookup lookup =
      hz6_shared_route_directory_lookup_snapshot(ptr);
  return lookup.status == HZ6_SHARED_ROUTE_LOOKUP_VALID ? lookup.route
                                                        : hz6_route_miss();
}

void hz6_shared_route_directory_unregister(Hz6Allocator* allocator,
                                           void* base) {
  if (!allocator || !base) {
    return;
  }
  size_t start = hz6_shared_route_directory_index((uintptr_t)base);
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  size_t lock_index = hz6_shared_route_directory_lock_index(start);
  hz6_shared_route_directory_writer_lock(lock_index);
#endif
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
      hz6_shared_route_directory_writer_unlock(lock_index);
#endif
      return;
    }
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      continue;
    }
    if (current != (uintptr_t)base) {
      continue;
    }
    Hz6Allocator* owner =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    if (owner != allocator) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
      hz6_shared_route_directory_writer_unlock(lock_index);
#endif
      return;
    }
    unsigned int owner_slot =
        atomic_load_explicit(&entry->owner_slot, memory_order_acquire);
    unsigned int owner_generation =
        atomic_load_explicit(&entry->owner_generation, memory_order_acquire);
    if (allocator->owner.token.slot != (uint32_t)owner_slot ||
        allocator->owner.token.generation != (uint32_t)owner_generation) {
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
      hz6_shared_route_directory_writer_unlock(lock_index);
#endif
      return;
    }
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    hz6_shared_route_directory_entry_begin_update(entry);
#endif
    atomic_store_explicit(&entry->allocator, NULL, memory_order_release);
    atomic_store_explicit(&entry->descriptor, 0, memory_order_release);
    atomic_store_explicit(&entry->front_id, 0, memory_order_release);
    atomic_store_explicit(&entry->class_id, 0, memory_order_release);
    atomic_store_explicit(&entry->generation, 0, memory_order_release);
    atomic_store_explicit(&entry->owner_slot, 0, memory_order_release);
    atomic_store_explicit(&entry->owner_generation, 0, memory_order_release);
    atomic_store_explicit(&entry->base, HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE,
                          memory_order_release);
    hz6_shared_route_directory_diag_tombstone_added();
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    hz6_shared_route_directory_entry_end_update(entry);
#endif
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_unregister;
#endif
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    hz6_shared_route_directory_writer_unlock(lock_index);
#endif
    return;
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  hz6_shared_route_directory_writer_unlock(lock_index);
#endif
}

int hz6_shared_route_directory_transfer_owner(Hz6Allocator* old_allocator,
                                              Hz6Allocator* new_allocator,
                                              void* base,
                                              uint32_t generation,
                                              void* descriptor) {
  if (!old_allocator || !new_allocator || !base || !descriptor) {
    return 0;
  }
  size_t start = hz6_shared_route_directory_index((uintptr_t)base);
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  size_t lock_index = hz6_shared_route_directory_lock_index(start);
  hz6_shared_route_directory_writer_lock(lock_index);
#endif
  for (size_t probe = 0; probe < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probe) {
    size_t index = (start + probe) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      break;
    }
    if (current == HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE ||
        current != (uintptr_t)base) {
      continue;
    }
    Hz6Allocator* owner =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    uintptr_t descriptor_value =
        atomic_load_explicit(&entry->descriptor, memory_order_acquire);
    unsigned int current_generation =
        atomic_load_explicit(&entry->generation, memory_order_acquire);
    unsigned int owner_slot =
        atomic_load_explicit(&entry->owner_slot, memory_order_acquire);
    unsigned int owner_generation =
        atomic_load_explicit(&entry->owner_generation, memory_order_acquire);
    if (owner != old_allocator || descriptor_value != (uintptr_t)descriptor ||
        current_generation != generation ||
        old_allocator->owner.token.slot != (uint32_t)owner_slot ||
        old_allocator->owner.token.generation !=
            (uint32_t)owner_generation) {
      break;
    }
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    hz6_shared_route_directory_entry_begin_update(entry);
#endif
    atomic_store_explicit(&entry->allocator, new_allocator,
                          memory_order_release);
    atomic_store_explicit(&entry->owner_slot,
                          new_allocator->owner.token.slot,
                          memory_order_release);
    atomic_store_explicit(&entry->owner_generation,
                          new_allocator->owner.token.generation,
                          memory_order_release);
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
    hz6_shared_route_directory_entry_end_update(entry);
    hz6_shared_route_directory_writer_unlock(lock_index);
#endif
    return 1;
  }
#if HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  hz6_shared_route_directory_writer_unlock(lock_index);
#endif
  return 0;
}

void hz6_shared_route_directory_maintain_tombstones(void) {
#if HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE_MAINTENANCE_L1 && \
    HZ6_SHARED_ROUTE_DIRECTORY_SEQ_SNAPSHOT_L1
  size_t current = hz6_shared_route_directory_diag_tombstone_current();
  if (current < HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE_MAINTENANCE_MIN) {
    return;
  }
  hz6_shared_route_directory_diag_maintenance_attempt();
  size_t cleared = 0;
  for (size_t shard = 0; shard < HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS;
       ++shard) {
    hz6_shared_route_directory_writer_lock(shard);
  }
  for (size_t index = 0; index < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
       ++index) {
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t entry_base =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (entry_base != HZ6_SHARED_ROUTE_DIRECTORY_TOMBSTONE) {
      continue;
    }
    hz6_shared_route_directory_entry_begin_update(entry);
    atomic_store_explicit(&entry->base, 0, memory_order_release);
    hz6_shared_route_directory_entry_end_update(entry);
    ++cleared;
  }
  for (size_t shard = HZ6_SHARED_ROUTE_DIRECTORY_LOCK_SHARDS; shard != 0;
       --shard) {
    hz6_shared_route_directory_writer_unlock(shard - 1u);
  }
  if (cleared != 0) {
    hz6_shared_route_directory_diag_maintenance_success();
    hz6_shared_route_directory_diag_tombstone_cleared(cleared);
  }
#endif
}

void hz6_allocator_route_shared_directory_dryrun(Hz6Allocator* allocator,
                                                 const void* ptr) {
#if HZ6_DIAGNOSTIC_PROBES
  if (!allocator || !ptr) {
    return;
  }
  ++allocator->stats.shared_dir_lookup;
  uintptr_t key = (uintptr_t)ptr;
  size_t start = hz6_shared_route_directory_index(key);
  size_t probes = 0;
  for (; probes < HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY; ++probes) {
    size_t index = (start + probes) % HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY;
    Hz6SharedRouteDirectoryEntry* entry =
        &g_hz6_shared_route_directory[index];
    uintptr_t current =
        atomic_load_explicit(&entry->base, memory_order_acquire);
    if (current == 0) {
      ++allocator->stats.shared_dir_miss;
      break;
    }
    if (current != key) {
      continue;
    }
    Hz6Allocator* route_allocator =
        atomic_load_explicit(&entry->allocator, memory_order_acquire);
    uintptr_t descriptor_value =
        atomic_load_explicit(&entry->descriptor, memory_order_acquire);
    if (!route_allocator || descriptor_value == 0) {
      ++allocator->stats.shared_dir_stale;
      break;
    }
    ++allocator->stats.shared_dir_hit;
    if (route_allocator == allocator) {
      ++allocator->stats.shared_dir_hit_local_allocator;
    } else {
      ++allocator->stats.shared_dir_hit_foreign_allocator;
      ++allocator->stats.shared_dir_would_skip_local;
    }
    break;
  }
  if (probes == HZ6_SHARED_ROUTE_DIRECTORY_CAPACITY) {
    ++allocator->stats.shared_dir_miss;
  }
  size_t probe_count = probes + 1;
  allocator->stats.shared_dir_probe_total += probe_count;
  if (probe_count > allocator->stats.shared_dir_probe_max) {
    allocator->stats.shared_dir_probe_max = probe_count;
  }
#else
  (void)allocator;
  (void)ptr;
#endif
}

Hz6RouteResult hz6_allocator_route_shared_directory_lookup_exact(
    Hz6Allocator* allocator,
    const void* ptr) {
  if (!allocator || !ptr) {
    return hz6_route_miss();
  }
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.shared_dir_first_attempt;
#endif
  Hz6SharedRouteLookup lookup =
      hz6_shared_route_directory_lookup_snapshot(ptr);
  if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_VALID) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.shared_dir_first_hit;
#endif
    return lookup.route;
  }
#if HZ6_DIAGNOSTIC_PROBES
  if (lookup.status == HZ6_SHARED_ROUTE_LOOKUP_STALE) {
    ++allocator->stats.shared_dir_stale;
  }
#endif
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.shared_dir_first_fallback;
#endif
  return hz6_route_miss();
}
#else
size_t hz6_allocator_shared_route_directory_bytes(void) {
  return 0;
}

int hz6_shared_route_directory_register(Hz6Allocator* allocator,
                                        void* base,
                                        uint16_t front_id,
                                        uint16_t class_id,
                                        uint32_t generation,
                                        void* descriptor) {
  (void)allocator;
  (void)base;
  (void)front_id;
  (void)class_id;
  (void)generation;
  (void)descriptor;
  return 0;
}

void hz6_shared_route_directory_unregister(Hz6Allocator* allocator,
                                           void* base) {
  (void)allocator;
  (void)base;
}

int hz6_shared_route_directory_transfer_owner(Hz6Allocator* old_allocator,
                                              Hz6Allocator* new_allocator,
                                              void* base,
                                              uint32_t generation,
                                              void* descriptor) {
  (void)old_allocator;
  (void)new_allocator;
  (void)base;
  (void)generation;
  (void)descriptor;
  return 0;
}

Hz6SharedRouteLookup hz6_shared_route_directory_lookup_snapshot(
    const void* ptr) {
  (void)ptr;
  Hz6SharedRouteLookup lookup;
  lookup.status = HZ6_SHARED_ROUTE_LOOKUP_MISS;
  lookup.route = hz6_route_miss();
  return lookup;
}

Hz6RouteResult hz6_shared_route_directory_lookup_raw(const void* ptr) {
  (void)ptr;
  return hz6_route_miss();
}

void hz6_shared_route_directory_maintain_tombstones(void) {
}
#endif
