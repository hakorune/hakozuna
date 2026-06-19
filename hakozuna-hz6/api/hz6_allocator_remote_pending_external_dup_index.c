#include "hz6_allocator_remote_pending_external_dup_index.h"

#include <stdint.h>

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && \
    HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1 && \
    HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1

#define HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE UINT32_MAX
#define HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_TOMBSTONE (UINT32_MAX - 1u)

static void hz6_remote_pending_external_dup_index_note_stale(
    Hz6Allocator* allocator) {
#if HZ6_DIAGNOSTIC_PROBES
  ++allocator->stats.remote_pending_external_ticket_duplicate_index_stale;
#else
  (void)allocator;
#endif
}

static size_t hz6_remote_pending_external_dup_hash(
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation) {
  uintptr_t x = (uintptr_t)descriptor;
  x >>= 4u;
  x ^= (uintptr_t)generation * UINT64_C(11400714819323198485);
  x ^= x >> 33u;
  return (size_t)(x % HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY);
}

void hz6_remote_pending_external_dup_index_init(Hz6Allocator* allocator) {
  for (size_t i = 0; i < HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY; ++i) {
    allocator->remote_pending_external_dup_index[i] =
        HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE;
  }
}

int hz6_remote_pending_external_dup_index_find(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation,
    uint32_t* out_ticket_index,
    size_t* out_probes) {
  size_t start = hz6_remote_pending_external_dup_hash(descriptor, generation);
  size_t probes = 0;
  for (size_t offset = 0; offset < HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY;
       ++offset) {
    size_t slot = (start + offset) %
                  HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY;
    uint32_t ticket_index = allocator->remote_pending_external_dup_index[slot];
    ++probes;
    if (ticket_index == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE) {
      if (out_probes) {
        *out_probes = probes;
      }
      return 0;
    }
    if (ticket_index == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_TOMBSTONE) {
      continue;
    }
    if (ticket_index >= HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
      hz6_remote_pending_external_dup_index_note_stale(allocator);
      allocator->remote_pending_external_dup_index[slot] =
          HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_TOMBSTONE;
      continue;
    }
    Hz6RemotePendingExternalTicket* ticket =
        &allocator->remote_pending_external_tickets[ticket_index];
    if (ticket->state == HZ6_REMOTE_PENDING_SLOT_NONE) {
      hz6_remote_pending_external_dup_index_note_stale(allocator);
      allocator->remote_pending_external_dup_index[slot] =
          HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_TOMBSTONE;
      continue;
    }
    if (ticket->descriptor == descriptor && ticket->generation == generation) {
      if (out_ticket_index) {
        *out_ticket_index = ticket_index;
      }
      if (out_probes) {
        *out_probes = probes;
      }
      return 1;
    }
  }
  if (out_probes) {
    *out_probes = probes;
  }
  return 0;
}

int hz6_remote_pending_external_dup_index_insert(Hz6Allocator* allocator,
                                                 uint32_t ticket_index) {
  if (ticket_index >= HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY) {
    return 0;
  }
  Hz6RemotePendingExternalTicket* ticket =
      &allocator->remote_pending_external_tickets[ticket_index];
  size_t start =
      hz6_remote_pending_external_dup_hash(ticket->descriptor,
                                           ticket->generation);
  uint32_t first_tombstone = HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE;
  for (size_t offset = 0; offset < HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY;
       ++offset) {
    size_t slot = (start + offset) %
                  HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY;
    uint32_t value = allocator->remote_pending_external_dup_index[slot];
    if (value == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_TOMBSTONE &&
        first_tombstone == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE) {
      first_tombstone = (uint32_t)slot;
      continue;
    }
    if (value == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE) {
      allocator->remote_pending_external_dup_index
          [first_tombstone == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE
               ? slot
               : first_tombstone] = ticket_index;
      return 1;
    }
  }
  if (first_tombstone != HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE) {
    allocator->remote_pending_external_dup_index[first_tombstone] =
        ticket_index;
    return 1;
  }
  return 0;
}

int hz6_remote_pending_external_dup_index_remove(
    Hz6Allocator* allocator,
    uint32_t ticket_index,
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation) {
  size_t start = hz6_remote_pending_external_dup_hash(descriptor, generation);
  for (size_t offset = 0; offset < HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY;
       ++offset) {
    size_t slot = (start + offset) %
                  HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY;
    uint32_t value = allocator->remote_pending_external_dup_index[slot];
    if (value == HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_NONE) {
      return 0;
    }
    if (value == ticket_index) {
      allocator->remote_pending_external_dup_index[slot] =
          HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_TOMBSTONE;
      return 1;
    }
  }
  return 0;
}

#else

void hz6_remote_pending_external_dup_index_init(Hz6Allocator* allocator) {
  (void)allocator;
}

int hz6_remote_pending_external_dup_index_find(
    Hz6Allocator* allocator,
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation,
    uint32_t* out_ticket_index,
    size_t* out_probes) {
  (void)allocator;
  (void)descriptor;
  (void)generation;
  (void)out_ticket_index;
  (void)out_probes;
  return 0;
}

int hz6_remote_pending_external_dup_index_insert(Hz6Allocator* allocator,
                                                 uint32_t ticket_index) {
  (void)allocator;
  (void)ticket_index;
  return 0;
}

int hz6_remote_pending_external_dup_index_remove(
    Hz6Allocator* allocator,
    uint32_t ticket_index,
    const Hz6ObjectDescriptor* descriptor,
    uint32_t generation) {
  (void)allocator;
  (void)ticket_index;
  (void)descriptor;
  (void)generation;
  return 0;
}

#endif
