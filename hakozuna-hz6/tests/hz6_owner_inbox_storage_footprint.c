#include "hz6_allocator.h"

#include <stdio.h>

int main(void) {
  size_t class_inbox_bytes =
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
      sizeof(Hz6RemotePendingClassInbox) * HZ6_FRONT_CACHE_CLASS_COUNT;
#else
      0u;
#endif
  size_t inline_slot_bytes =
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1
#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_next) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_slot_state) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_published_ptr) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_published_generation) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_published_bytes) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_published_front_id) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_published_class_id) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_owner_token);
#else
      sizeof(((Hz6Allocator*)0)->remote_pending_next) +
      sizeof(((Hz6Allocator*)0)->remote_pending_slot_state) +
      sizeof(((Hz6Allocator*)0)->remote_pending_published_ptr) +
      sizeof(((Hz6Allocator*)0)->remote_pending_published_generation) +
      sizeof(((Hz6Allocator*)0)->remote_pending_published_bytes) +
      sizeof(((Hz6Allocator*)0)->remote_pending_published_front_id) +
      sizeof(((Hz6Allocator*)0)->remote_pending_published_class_id) +
      sizeof(((Hz6Allocator*)0)->remote_pending_owner_token);
#endif
#else
      0u;
#endif
  size_t external_ticket_bytes =
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1
#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_external_tickets) +
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_external_head);
#else
      sizeof(((Hz6Allocator*)0)->remote_pending_external_tickets) +
      sizeof(((Hz6Allocator*)0)->remote_pending_external_head);
#endif
#else
      0u;
#endif
  size_t external_dup_index_bytes =
#if HZ6_REMOTE_PENDING_INBOX_CORE_L1 && HZ6_REMOTE_PENDING_EXTERNAL_TICKET_L1 && \
    HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_L1
#if HZ6_REMOTE_PENDING_LAZY_STORAGE_L1
      sizeof(((Hz6RemotePendingStorage*)0)->remote_pending_external_dup_index);
#else
      sizeof(((Hz6Allocator*)0)->remote_pending_external_dup_index);
#endif
#else
      0u;
#endif
  size_t owner_inbox_bytes = class_inbox_bytes + inline_slot_bytes +
                             external_ticket_bytes +
                             external_dup_index_bytes;
  printf("sizeof_Hz6Allocator=%zu\n", sizeof(Hz6Allocator));
  printf("owner_inbox_bytes=%zu\n", owner_inbox_bytes);
  printf("owner_inbox_class_inbox_bytes=%zu\n", class_inbox_bytes);
  printf("owner_inbox_inline_slot_bytes=%zu\n", inline_slot_bytes);
  printf("owner_inbox_external_ticket_bytes=%zu\n", external_ticket_bytes);
  printf("owner_inbox_external_dup_index_bytes=%zu\n",
         external_dup_index_bytes);
  printf("object_descriptor_capacity=%u\n",
         (unsigned)HZ6_OBJECT_DESCRIPTOR_CAPACITY);
  printf("external_ticket_capacity=%u\n",
         (unsigned)HZ6_REMOTE_PENDING_EXTERNAL_TICKET_CAPACITY);
  printf("external_dup_index_capacity=%u\n",
         (unsigned)HZ6_REMOTE_PENDING_EXTERNAL_DUP_INDEX_CAPACITY);
  return 0;
}
