#ifndef HZ6_ALLOCATOR_OWNER_INBOX_STORAGE_PROVIDER_H
#define HZ6_ALLOCATOR_OWNER_INBOX_STORAGE_PROVIDER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6OwnerInboxStorageBlock {
  void* ptr;
  size_t bytes;
} Hz6OwnerInboxStorageBlock;

Hz6OwnerInboxStorageBlock hz6_owner_inbox_storage_alloc(size_t bytes);
void hz6_owner_inbox_storage_free(Hz6OwnerInboxStorageBlock* block);

#ifdef __cplusplus
}
#endif

#endif
