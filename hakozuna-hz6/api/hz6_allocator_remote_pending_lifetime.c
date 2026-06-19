#include "hz6_allocator.h"
#include "hz6_allocator_api_route_transfer.h"

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
#define HZ6_REMOTE_PENDING_LIFETIME_INDEX_NONE UINT32_MAX

static void hz6_remote_pending_lifetime_reset_inline(Hz6Allocator* allocator) {
  for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
       ++class_id) {
    Hz6RemotePendingClassInbox* inbox =
        &allocator->remote_pending_inbox[class_id];
    for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
         ++front_index) {
      inbox->head_index[front_index] = HZ6_REMOTE_PENDING_LIFETIME_INDEX_NONE;
      inbox->key_count[front_index] = 0;
    }
    inbox->total_count = 0;
  }
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    if (allocator->remote_pending_slot_state[i] !=
        HZ6_REMOTE_PENDING_SLOT_NONE) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.remote_pending_destroy_inline_closeout;
#endif
    }
    allocator->remote_pending_next[i] = HZ6_REMOTE_PENDING_LIFETIME_INDEX_NONE;
    allocator->remote_pending_slot_state[i] = HZ6_REMOTE_PENDING_SLOT_NONE;
    allocator->remote_pending_published_ptr[i] = NULL;
    allocator->remote_pending_published_generation[i] = 0;
    allocator->remote_pending_published_bytes[i] = 0;
    allocator->remote_pending_published_front_id[i] = 0;
    allocator->remote_pending_published_class_id[i] = 0;
    allocator->remote_pending_owner_token[i] = (Hz6OwnerToken){0};
  }
  atomic_store_explicit(&allocator->remote_pending_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&allocator->remote_pending_queued_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&allocator->remote_pending_claimed_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&allocator->remote_pending_total_current, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&allocator->remote_pending_nonempty_mask, 0u,
                        memory_order_relaxed);
}

#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
static int hz6_remote_pending_lifetime_release_external(
    Hz6Allocator* allocator,
    Hz6RemotePendingExternalTicket* ticket) {
  if (!allocator || !ticket || !ticket->descriptor || !ticket->ptr ||
      ticket->state == HZ6_REMOTE_PENDING_SLOT_NONE) {
    return 0;
  }
  if (!hz6_owner_equal(ticket->owner_token, allocator->owner.token)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_pending_ticket_to_dead_owner;
#endif
    return 0;
  }
  Hz6ObjectDescriptor* descriptor = ticket->descriptor;
  int valid = descriptor->ptr == ticket->ptr &&
              descriptor->generation == ticket->generation &&
              descriptor->bytes == ticket->bytes &&
              descriptor->class_id == ticket->class_id &&
              descriptor->state == HZ6_STATE_REMOTE_PENDING;
  Hz6Allocator* storage = NULL;
#if HZ6_DESCRIPTOR_STORAGE_OWNER16_L1 || HZ6_DIAGNOSTIC_PROBES || \
    HZ6_OWNER_SOURCE_SIDE_META_DRYRUN || HZ6_OWNER_SOURCE_SIDE_META_L2
  storage = hz6_allocator_descriptor_storage_owner(allocator, descriptor, NULL);
#endif
  if (!storage ||
      !hz6_owner_equal(storage->owner.token,
                       ticket->descriptor_storage_owner_token)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_pending_ticket_to_dead_storage_owner;
#endif
    return 0;
  }
  if (!hz6_owner_is_alive(&storage->owner, storage->owner.token)) {
#if HZ6_DIAGNOSTIC_PROBES
    ++allocator->stats.remote_pending_ticket_to_dead_storage_owner;
#endif
    return 0;
  }
  if (valid) {
    Hz6RouteResult route = hz6_allocator_route_lookup_exact(allocator,
                                                            ticket->ptr);
    if (route.kind != HZ6_ROUTE_VALID || route.descriptor != descriptor ||
        route.generation != ticket->generation ||
        route.front_id != ticket->front_id ||
        route.class_id != ticket->class_id ||
        route.route_allocator != allocator) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.remote_pending_route_removed_while_pending;
#endif
      valid = 0;
    }
  }
  if (!valid) {
    return 0;
  }
  hz6_allocator_route_unregister_exact(allocator, ticket->ptr);
  return hz6_allocator_release_descriptor_source(storage, descriptor);
}

static void hz6_remote_pending_lifetime_close_external(
    Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    Hz6RemotePendingExternalTicket* ticket =
        &allocator->remote_pending_external_tickets[i];
    if (ticket->state == HZ6_REMOTE_PENDING_SLOT_NONE) {
      continue;
    }
    if (hz6_remote_pending_lifetime_release_external(allocator, ticket)) {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.remote_pending_destroy_external_closeout;
#endif
    } else {
#if HZ6_DIAGNOSTIC_PROBES
      ++allocator->stats.remote_pending_destroy_external_release_fail;
#endif
    }
  }
  for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
       ++front_index) {
    for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
         ++class_id) {
      allocator->remote_pending_external_head[front_index][class_id] =
          HZ6_REMOTE_PENDING_LIFETIME_INDEX_NONE;
    }
  }
  allocator->remote_pending_external_free_head =
      HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY == 0
          ? HZ6_REMOTE_PENDING_LIFETIME_INDEX_NONE
          : 0u;
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    Hz6RemotePendingExternalTicket* ticket =
        &allocator->remote_pending_external_tickets[i];
    *ticket = (Hz6RemotePendingExternalTicket){0};
    ticket->next =
        i + 1u < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY
            ? (uint32_t)(i + 1u)
            : HZ6_REMOTE_PENDING_LIFETIME_INDEX_NONE;
  }
#if HZ6_DIAGNOSTIC_PROBES
  allocator->stats.remote_pending_external_ticket_current = 0;
#endif
}
#endif

void hz6_allocator_remote_pending_closeout_for_destroy(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  hz6_remote_pending_lifetime_close_external(allocator);
#endif
  hz6_remote_pending_lifetime_reset_inline(allocator);
}

void hz6_allocator_remote_pending_note_after_destroy(
    Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }
#if HZ6_DIAGNOSTIC_PROBES
  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    if (allocator->descriptors[i].state == HZ6_STATE_REMOTE_PENDING ||
        allocator->remote_pending_slot_state[i] !=
            HZ6_REMOTE_PENDING_SLOT_NONE) {
      ++allocator->stats.remote_pending_after_allocator_destroy;
      return;
    }
  }
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    if (allocator->remote_pending_external_tickets[i].state !=
        HZ6_REMOTE_PENDING_SLOT_NONE) {
      ++allocator->stats.remote_pending_after_allocator_destroy;
      return;
    }
  }
#endif
#endif
}
#else
void hz6_allocator_remote_pending_closeout_for_destroy(
    Hz6Allocator* allocator) {
  (void)allocator;
}

void hz6_allocator_remote_pending_note_after_destroy(
    Hz6Allocator* allocator) {
  (void)allocator;
}
#endif
