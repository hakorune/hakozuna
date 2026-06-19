#include "hz6_allocator.h"
#include "hz6_allocator_api_route_transfer.h"

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && HZ6_DIAGNOSTIC_PROBES
#define HZ6_REMOTE_PENDING_ACCOUNTING_INDEX_NONE UINT32_MAX

static int hz6_remote_pending_accounting_front_index(uint16_t front_id,
                                                     uint16_t* out) {
  if (!out) {
    return 0;
  }
  switch (front_id) {
    case HZ6_FRONT_LOCAL2P:
      *out = 0;
      return 1;
    case HZ6_FRONT_MIDPAGE:
      *out = 1;
      return 1;
    case HZ6_FRONT_LARGE:
      *out = 2;
      return 1;
    case HZ6_FRONT_TOY:
      *out = 3;
      return 1;
    default:
      return 0;
  }
}

static void hz6_remote_pending_check_inline(
    const Hz6Allocator* allocator,
    Hz6StatsSnapshot* snapshot,
    size_t* out_expected_pending,
    size_t* out_known_pending_state) {
  uint8_t seen[HZ6_OBJECT_DESCRIPTOR_CAPACITY] = {0};
  size_t queued_from_lists = 0;
  size_t queued_slots = 0;
  size_t claimed_slots = 0;
  int mismatch = 0;

  for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
       ++class_id) {
    const Hz6RemotePendingClassInbox* inbox =
        &allocator->remote_pending_inbox[class_id];
    size_t class_queued = 0;
    size_t class_claimed = 0;
    for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
         ++front_index) {
      size_t key_count = 0;
      uint32_t index = inbox->head_index[front_index];
      size_t guard = 0;
      while (index != HZ6_REMOTE_PENDING_ACCOUNTING_INDEX_NONE) {
        if (index >= HZ6_OBJECT_DESCRIPTOR_CAPACITY ||
            guard++ > HZ6_OBJECT_DESCRIPTOR_CAPACITY) {
          mismatch = 1;
          break;
        }
        if (seen[index] != 0) {
          mismatch = 1;
          break;
        }
        seen[index] = 1;
        if (allocator->remote_pending_slot_state[index] !=
                HZ6_REMOTE_PENDING_SLOT_QUEUED ||
            allocator->remote_pending_published_class_id[index] != class_id) {
          mismatch = 1;
        }
        uint16_t entry_front = 0;
        if (!hz6_remote_pending_accounting_front_index(
                allocator->remote_pending_published_front_id[index],
                &entry_front) ||
            entry_front != front_index) {
          mismatch = 1;
        }
        ++key_count;
        ++class_queued;
        ++queued_from_lists;
        index = allocator->remote_pending_next[index];
      }
      if (key_count != inbox->key_count[front_index]) {
        mismatch = 1;
      }
    }
    for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
      if (allocator->remote_pending_published_class_id[i] == class_id &&
          allocator->remote_pending_slot_state[i] ==
              HZ6_REMOTE_PENDING_SLOT_CLAIMED) {
        ++class_claimed;
      }
    }
    if (class_queued + class_claimed != inbox->total_count) {
      mismatch = 1;
    }
  }

  for (size_t i = 0; i < HZ6_OBJECT_DESCRIPTOR_CAPACITY; ++i) {
    switch (allocator->remote_pending_slot_state[i]) {
      case HZ6_REMOTE_PENDING_SLOT_QUEUED:
        ++queued_slots;
        if (seen[i] == 0) {
          mismatch = 1;
        }
        if (allocator->descriptors[i].state != HZ6_STATE_REMOTE_PENDING) {
          ++snapshot->remote_pending_total_state_count_mismatch;
        }
        break;
      case HZ6_REMOTE_PENDING_SLOT_CLAIMED:
        ++claimed_slots;
        if (allocator->descriptors[i].state != HZ6_STATE_REMOTE_PENDING) {
          ++snapshot->remote_pending_total_state_count_mismatch;
        }
        break;
      case HZ6_REMOTE_PENDING_SLOT_NONE:
        break;
      default:
        mismatch = 1;
        break;
    }
  }

  size_t queued_current = atomic_load_explicit(
      &allocator->remote_pending_current, memory_order_relaxed);
  size_t claimed_current = atomic_load_explicit(
      &allocator->remote_pending_claimed_current, memory_order_relaxed);
  size_t total_current = atomic_load_explicit(
      &allocator->remote_pending_total_current, memory_order_relaxed);
  if (queued_from_lists != queued_slots || queued_slots != queued_current ||
      claimed_slots != claimed_current ||
      queued_slots + claimed_slots != total_current) {
    mismatch = 1;
  }
  if (claimed_slots != 0) {
    ++snapshot->remote_pending_claimed_current_at_quiescence;
  }
  if (mismatch) {
    ++snapshot->remote_pending_inline_accounting_mismatch;
  }
  *out_expected_pending += queued_slots + claimed_slots;
  *out_known_pending_state += queued_slots + claimed_slots;
}

#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
static void hz6_remote_pending_check_external(
    const Hz6Allocator* allocator,
    Hz6StatsSnapshot* snapshot,
    size_t* out_expected_pending,
    size_t* out_known_pending_state) {
  uint8_t membership[HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY] = {0};
  size_t free_count = 0;
  size_t queued_from_lists = 0;
  size_t queued_state = 0;
  size_t claimed_state = 0;
  int mismatch = 0;

  uint32_t free_index = allocator->remote_pending_external_free_head;
  size_t free_guard = 0;
  while (free_index != HZ6_REMOTE_PENDING_ACCOUNTING_INDEX_NONE) {
    if (free_index >= HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY ||
        free_guard++ > HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
      ++snapshot->remote_pending_external_free_list_corruption;
      mismatch = 1;
      break;
    }
    if (membership[free_index] != 0) {
      ++snapshot->remote_pending_external_list_cycle;
      mismatch = 1;
      break;
    }
    membership[free_index] = 1;
    if (allocator->remote_pending_external_tickets[free_index].state !=
        HZ6_REMOTE_PENDING_SLOT_NONE) {
      ++snapshot->remote_pending_external_free_list_corruption;
      mismatch = 1;
    }
    ++free_count;
    free_index = allocator->remote_pending_external_tickets[free_index].next;
  }

  for (size_t front_index = 0; front_index < HZ6_REMOTE_PENDING_FRONT_COUNT;
       ++front_index) {
    for (size_t class_id = 0; class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
         ++class_id) {
      uint32_t index =
          allocator->remote_pending_external_head[front_index][class_id];
      size_t guard = 0;
      while (index != HZ6_REMOTE_PENDING_ACCOUNTING_INDEX_NONE) {
        if (index >= HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY ||
            guard++ > HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
          ++snapshot->remote_pending_external_list_cycle;
          mismatch = 1;
          break;
        }
        if (membership[index] != 0) {
          ++snapshot->remote_pending_external_ticket_multiple_list_membership;
          mismatch = 1;
          break;
        }
        membership[index] = 2;
        const Hz6RemotePendingExternalTicket* ticket =
            &allocator->remote_pending_external_tickets[index];
        if (ticket->state != HZ6_REMOTE_PENDING_SLOT_QUEUED ||
            ticket->class_id != class_id) {
          mismatch = 1;
        }
        uint16_t ticket_front = 0;
        if (!hz6_remote_pending_accounting_front_index(ticket->front_id,
                                                       &ticket_front) ||
            ticket_front != front_index) {
          mismatch = 1;
        }
        ++queued_from_lists;
        index = ticket->next;
      }
    }
  }

  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY; ++i) {
    const Hz6RemotePendingExternalTicket* ticket =
        &allocator->remote_pending_external_tickets[i];
    switch (ticket->state) {
      case HZ6_REMOTE_PENDING_SLOT_NONE:
        if (membership[i] != 1) {
          ++snapshot->remote_pending_external_free_list_corruption;
          mismatch = 1;
        }
        break;
      case HZ6_REMOTE_PENDING_SLOT_QUEUED:
        ++queued_state;
        if (membership[i] != 2) {
          mismatch = 1;
        }
        break;
      case HZ6_REMOTE_PENDING_SLOT_CLAIMED:
        ++claimed_state;
        break;
      default:
        mismatch = 1;
        break;
    }
    if (ticket->state != HZ6_REMOTE_PENDING_SLOT_NONE &&
        (!ticket->descriptor ||
         ticket->descriptor->state != HZ6_STATE_REMOTE_PENDING)) {
      ++snapshot->remote_pending_total_state_count_mismatch;
    }
  }

  if (free_count + queued_state + claimed_state !=
      HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
    ++snapshot->remote_pending_external_free_list_corruption;
    mismatch = 1;
  }
  if (queued_from_lists != queued_state ||
      queued_state + claimed_state !=
          snapshot->remote_pending_external_ticket_current) {
    mismatch = 1;
  }
  if (claimed_state != 0) {
    ++snapshot->remote_pending_external_claimed_at_quiescence;
  }
  if (mismatch) {
    ++snapshot->remote_pending_external_accounting_mismatch;
  }
  *out_expected_pending += queued_state + claimed_state;
  *out_known_pending_state += queued_state + claimed_state;
}
#endif

void hz6_allocator_remote_pending_note_accounting_snapshot(
    const Hz6Allocator* allocator,
    Hz6StatsSnapshot* snapshot) {
  if (!allocator || !snapshot) {
    return;
  }
  size_t expected_pending = 0;
  size_t known_pending_state = 0;
  hz6_remote_pending_check_inline(allocator, snapshot, &expected_pending,
                                  &known_pending_state);
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
  hz6_remote_pending_check_external(allocator, snapshot, &expected_pending,
                                    &known_pending_state);
#endif
  if (expected_pending != known_pending_state) {
    ++snapshot->remote_pending_total_state_count_mismatch;
  }
}
#else
void hz6_allocator_remote_pending_note_accounting_snapshot(
    const Hz6Allocator* allocator,
    Hz6StatsSnapshot* snapshot) {
  (void)allocator;
  (void)snapshot;
}
#endif
