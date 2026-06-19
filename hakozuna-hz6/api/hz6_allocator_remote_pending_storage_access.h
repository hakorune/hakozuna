#ifndef HZ6_ALLOCATOR_REMOTE_PENDING_STORAGE_ACCESS_H
#define HZ6_ALLOCATOR_REMOTE_PENDING_STORAGE_ACCESS_H

#include "hz6_allocator.h"

#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
static inline Hz6RemotePendingStorage* hz6_remote_pending_storage_ptr(
    const Hz6Allocator* allocator) {
  return atomic_load_explicit(&allocator->remote_pending_storage,
                              memory_order_acquire);
}

#define HZ6_RP_INBOX(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_inbox)
#define HZ6_RP_NEXT(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_next)
#define HZ6_RP_SLOT_STATE(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_slot_state)
#define HZ6_RP_PUBLISHED_PTR(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_published_ptr)
#define HZ6_RP_PUBLISHED_GENERATION(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_published_generation)
#define HZ6_RP_PUBLISHED_BYTES(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_published_bytes)
#define HZ6_RP_PUBLISHED_FRONT_ID(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_published_front_id)
#define HZ6_RP_PUBLISHED_CLASS_ID(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_published_class_id)
#define HZ6_RP_OWNER_TOKEN(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_owner_token)
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
#define HZ6_RP_EXTERNAL_TICKETS(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_external_tickets)
#define HZ6_RP_EXTERNAL_LOCK(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_external_lock)
#define HZ6_RP_EXTERNAL_FREE_HEAD(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_external_free_head)
#define HZ6_RP_EXTERNAL_HEAD(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_external_head)
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
#define HZ6_RP_EXTERNAL_DUP_INDEX(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_external_dup_index)
#endif
#endif
#define HZ6_RP_CURRENT(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_current)
#define HZ6_RP_QUEUED_CURRENT(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_queued_current)
#define HZ6_RP_CLAIMED_CURRENT(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_claimed_current)
#define HZ6_RP_TOTAL_CURRENT(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_total_current)
#define HZ6_RP_NONEMPTY_MASK(allocator) \
  (hz6_remote_pending_storage_ptr(allocator)->remote_pending_nonempty_mask)
#else
#define HZ6_RP_INBOX(allocator) ((allocator)->remote_pending_inbox)
#define HZ6_RP_NEXT(allocator) ((allocator)->remote_pending_next)
#define HZ6_RP_SLOT_STATE(allocator) ((allocator)->remote_pending_slot_state)
#define HZ6_RP_PUBLISHED_PTR(allocator) \
  ((allocator)->remote_pending_published_ptr)
#define HZ6_RP_PUBLISHED_GENERATION(allocator) \
  ((allocator)->remote_pending_published_generation)
#define HZ6_RP_PUBLISHED_BYTES(allocator) \
  ((allocator)->remote_pending_published_bytes)
#define HZ6_RP_PUBLISHED_FRONT_ID(allocator) \
  ((allocator)->remote_pending_published_front_id)
#define HZ6_RP_PUBLISHED_CLASS_ID(allocator) \
  ((allocator)->remote_pending_published_class_id)
#define HZ6_RP_OWNER_TOKEN(allocator) ((allocator)->remote_pending_owner_token)
#if HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
#define HZ6_RP_EXTERNAL_TICKETS(allocator) \
  ((allocator)->remote_pending_external_tickets)
#define HZ6_RP_EXTERNAL_LOCK(allocator) \
  ((allocator)->remote_pending_external_lock)
#define HZ6_RP_EXTERNAL_FREE_HEAD(allocator) \
  ((allocator)->remote_pending_external_free_head)
#define HZ6_RP_EXTERNAL_HEAD(allocator) \
  ((allocator)->remote_pending_external_head)
#if HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
#define HZ6_RP_EXTERNAL_DUP_INDEX(allocator) \
  ((allocator)->remote_pending_external_dup_index)
#endif
#endif
#define HZ6_RP_CURRENT(allocator) ((allocator)->remote_pending_current)
#define HZ6_RP_QUEUED_CURRENT(allocator) \
  ((allocator)->remote_pending_queued_current)
#define HZ6_RP_CLAIMED_CURRENT(allocator) \
  ((allocator)->remote_pending_claimed_current)
#define HZ6_RP_TOTAL_CURRENT(allocator) ((allocator)->remote_pending_total_current)
#define HZ6_RP_NONEMPTY_MASK(allocator) ((allocator)->remote_pending_nonempty_mask)
#endif

#endif
