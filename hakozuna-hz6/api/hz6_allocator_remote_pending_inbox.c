#include "hz6_allocator.h"

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
#define HZ6_REMOTE_PENDING_INDEX_NONE UINT32_MAX

typedef struct Hz6RemotePendingInboxEntry {
  void* ptr;
  Hz6ObjectDescriptor* descriptor;
  uint32_t generation;
  uint16_t class_id;
  uint16_t reserved;
  Hz6OwnerToken owner_token;
} Hz6RemotePendingInboxEntry;

static void hz6_remote_pending_lock(Hz6RemotePendingClassInbox* inbox) {
  while (atomic_flag_test_and_set_explicit(&inbox->lock,
                                           memory_order_acquire)) {
  }
}

static void hz6_remote_pending_unlock(Hz6RemotePendingClassInbox* inbox) {
  atomic_flag_clear_explicit(&inbox->lock, memory_order_release);
}

static int hz6_remote_pending_descriptor_index(
    Hz6Allocator* allocator,
    Hz6ObjectDescriptor* descriptor,
    uint32_t* out_index) {
  if (!allocator || !descriptor || !out_index ||
      descriptor < allocator->descriptors ||
      descriptor >= allocator->descriptors + HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    return 0;
  }
  *out_index = (uint32_t)(descriptor - allocator->descriptors);
  return 1;
}

static void hz6_remote_pending_note_current(Hz6Allocator* allocator,
                                            size_t current) {
  allocator->stats.remote_pending_current = current;
  if (current > allocator->stats.remote_pending_high_water) {
    allocator->stats.remote_pending_high_water = current;
  }
}

static void hz6_remote_pending_requeue_locked(
    Hz6Allocator* allocator,
    uint16_t class_id,
    uint32_t index) {
  Hz6RemotePendingClassInbox* inbox =
      &allocator->remote_pending_inbox[class_id];
  allocator->remote_pending_enqueued[index] = 1;
  allocator->remote_pending_next[index] = inbox->head_index;
  inbox->head_index = index;
  ++inbox->count;
  size_t current = atomic_fetch_add_explicit(
                       &allocator->remote_pending_current, 1u,
                       memory_order_relaxed) +
                   1u;
  hz6_remote_pending_note_current(allocator, current);
}

static int hz6_remote_pending_pop_class(
    Hz6Allocator* allocator,
    uint16_t class_id,
    Hz6RemotePendingInboxEntry* out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  Hz6RemotePendingClassInbox* inbox =
      &allocator->remote_pending_inbox[class_id];
  hz6_remote_pending_lock(inbox);
  uint32_t index = inbox->head_index;
  if (index == HZ6_REMOTE_PENDING_INDEX_NONE ||
      index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
    hz6_remote_pending_unlock(inbox);
    return 0;
  }
  inbox->head_index = allocator->remote_pending_next[index];
  allocator->remote_pending_next[index] = HZ6_REMOTE_PENDING_INDEX_NONE;
  allocator->remote_pending_enqueued[index] = 0;
  --inbox->count;
  size_t current = atomic_fetch_sub_explicit(
                       &allocator->remote_pending_current, 1u,
                       memory_order_relaxed) -
                   1u;
  hz6_remote_pending_note_current(allocator, current);
  hz6_remote_pending_unlock(inbox);

  Hz6ObjectDescriptor* descriptor = &allocator->descriptors[index];
  out->ptr = descriptor->ptr;
  out->descriptor = descriptor;
  out->generation = descriptor->generation;
  out->class_id = descriptor->class_id;
  out->reserved = 0;
  out->owner_token = allocator->owner.token;
  return 1;
}

static void hz6_remote_pending_requeue_entry(
    Hz6Allocator* allocator,
    const Hz6RemotePendingInboxEntry* entry) {
  uint32_t index = 0;
  if (!entry || !hz6_remote_pending_descriptor_index(
                    allocator, entry->descriptor, &index) ||
      entry->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return;
  }
  Hz6RemotePendingClassInbox* inbox =
      &allocator->remote_pending_inbox[entry->class_id];
  hz6_remote_pending_lock(inbox);
  if (!allocator->remote_pending_enqueued[index]) {
    hz6_remote_pending_requeue_locked(allocator, entry->class_id, index);
  }
  hz6_remote_pending_unlock(inbox);
}
#endif

void hz6_allocator_remote_pending_inbox_init(Hz6Allocator* allocator) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator) {
    return;
  }
  for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
       ++class_id) {
    atomic_flag_clear_explicit(
        &allocator->remote_pending_inbox[class_id].lock,
        memory_order_relaxed);
    allocator->remote_pending_inbox[class_id].head_index =
        HZ6_REMOTE_PENDING_INDEX_NONE;
    allocator->remote_pending_inbox[class_id].count = 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    allocator->remote_pending_next[i] = HZ6_REMOTE_PENDING_INDEX_NONE;
    allocator->remote_pending_enqueued[i] = 0;
  }
  atomic_store_explicit(&allocator->remote_pending_current, 0u,
                        memory_order_relaxed);
#else
  (void)allocator;
#endif
}

int hz6_allocator_remote_pending_enqueue(Hz6Allocator* allocator,
                                         Hz6ObjectDescriptor* descriptor,
                                         void* ptr,
                                         uint32_t generation,
                                         uint16_t class_id) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator || !descriptor || !ptr ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  ++allocator->stats.remote_pending_enqueue_attempt;
  if (!hz6_owner_is_alive(&allocator->owner, allocator->owner.token)) {
    ++allocator->stats.remote_pending_owner_dead;
    return 0;
  }
  uint32_t index = 0;
  if (!hz6_remote_pending_descriptor_index(allocator, descriptor, &index) ||
      descriptor->ptr != ptr || descriptor->generation != generation ||
      descriptor->class_id != class_id ||
      descriptor->state != HZ6_STATE_ACTIVE) {
    ++allocator->stats.remote_pending_publish_fail;
    return 0;
  }
  if (!hz6_allocator_descriptor_owner_equal_at(
          allocator, descriptor, allocator->owner.token,
          HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
    ++allocator->stats.remote_pending_owner_mismatch;
    return 0;
  }

  Hz6RemotePendingClassInbox* inbox =
      &allocator->remote_pending_inbox[class_id];
  hz6_remote_pending_lock(inbox);
  if (allocator->remote_pending_enqueued[index]) {
    ++allocator->stats.remote_pending_duplicate_claim;
    hz6_remote_pending_unlock(inbox);
    return 0;
  }
  if (inbox->count >= HZ6_REMOTE_PENDING_INBOX_CLASS_CAPACITY) {
    ++allocator->stats.remote_pending_enqueue_full;
    hz6_remote_pending_unlock(inbox);
    return 0;
  }
  descriptor->state = HZ6_STATE_REMOTE_PENDING;
  hz6_remote_pending_requeue_locked(allocator, class_id, index);
  ++allocator->stats.remote_pending_enqueue_success;
  hz6_remote_pending_unlock(inbox);
  return 1;
#else
  (void)allocator;
  (void)descriptor;
  (void)ptr;
  (void)generation;
  (void)class_id;
  return 0;
#endif
}

size_t hz6_allocator_remote_pending_maintenance_class(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t budget) {
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT || budget == 0) {
    return 0;
  }
  ++allocator->stats.remote_pending_maintenance_check;
  if (atomic_load_explicit(&allocator->remote_pending_current,
                           memory_order_relaxed) == 0) {
    return 0;
  }
  ++allocator->stats.remote_pending_maintenance_armed;
  ++allocator->stats.remote_pending_batch_call;
  size_t drained = 0;
  while (drained < budget) {
    if (hz6_allocator_frontcache_count(allocator, class_id) >=
        hz6_allocator_frontcache_capacity(allocator, class_id)) {
      ++allocator->stats.remote_pending_frontcache_full;
      break;
    }
    Hz6RemotePendingInboxEntry entry = {0};
    if (!hz6_remote_pending_pop_class(allocator, class_id, &entry)) {
      break;
    }
    Hz6ObjectDescriptor* descriptor = entry.descriptor;
    int valid = descriptor && descriptor->ptr == entry.ptr &&
                descriptor->generation == entry.generation &&
                descriptor->class_id == class_id;
    if (valid && descriptor->state != HZ6_STATE_REMOTE_PENDING) {
      ++allocator->stats.remote_pending_state_mismatch;
      valid = 0;
    }
    if (valid && !hz6_allocator_descriptor_owner_equal_at(
                     allocator, descriptor, entry.owner_token,
                     HZ6_OWNER_EQUAL_SITE_REMOTE_PENDING)) {
      ++allocator->stats.remote_pending_owner_mismatch;
      valid = 0;
    }
    if (valid) {
      Hz6RouteResult route =
          hz6_allocator_route_lookup_exact(allocator, entry.ptr);
      if (route.kind != HZ6_ROUTE_VALID || route.descriptor != descriptor ||
          route.generation != entry.generation || route.class_id != class_id ||
          route.route_allocator != allocator) {
        ++allocator->stats.remote_pending_route_mismatch;
        valid = 0;
      }
    }
    if (!valid) {
      if (descriptor && descriptor->generation != entry.generation) {
        ++allocator->stats.remote_pending_generation_mismatch;
      }
      continue;
    }

    Hz6FrontCacheEntry front_entry = {0};
    front_entry.ptr = entry.ptr;
    front_entry.descriptor = descriptor;
#if HZ6_DESCRIPTORLESS_FRONTCACHE_L1
    front_entry.source_block = descriptor->source_block;
#endif
    front_entry.generation = entry.generation;
    hz6_frontcache_entry_set_bytes(&front_entry, descriptor->bytes);
    hz6_frontcache_entry_set_class_id(&front_entry, class_id);
    if (!hz6_allocator_frontcache_push(allocator, class_id, front_entry)) {
      hz6_remote_pending_requeue_entry(allocator, &entry);
      ++allocator->stats.remote_pending_frontcache_full;
      break;
    }
    descriptor->state = HZ6_STATE_LOCAL_FREE;
    ++allocator->stats.remote_pending_frontcache_push;
    ++drained;
  }
  allocator->stats.remote_pending_batch_items += drained;
  return drained;
#else
  (void)allocator;
  (void)class_id;
  (void)budget;
  return 0;
#endif
}
