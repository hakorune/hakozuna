#include "hz6_allocator_owner_inbox_storage_provider.h"
#include "hz6_source_util.h"

#include <string.h>

#if defined(_WIN32)
#include "win_source_virtualalloc.h"
#else
#include "linux_source_mmap.h"
#endif

static size_t hz6_owner_inbox_storage_page_size(void) {
#if defined(_WIN32)
  return hz6_win_page_size();
#else
  return hz6_linux_page_size();
#endif
}

Hz6OwnerInboxStorageBlock hz6_owner_inbox_storage_alloc(size_t bytes) {
  Hz6OwnerInboxStorageBlock block = {0};
  size_t page_size = hz6_owner_inbox_storage_page_size();
  size_t rounded = hz6_source_round_up(bytes, page_size);
  if (rounded == 0) {
    return block;
  }
#if defined(_WIN32)
  block.ptr = hz6_win_virtualalloc_reserve(rounded, page_size);
#else
  block.ptr = hz6_linux_mmap_reserve(rounded, page_size);
#endif
  if (!block.ptr) {
    return (Hz6OwnerInboxStorageBlock){0};
  }
  block.bytes = rounded;
  memset(block.ptr, 0, block.bytes);
  return block;
}

void hz6_owner_inbox_storage_free(Hz6OwnerInboxStorageBlock* block) {
  if (!block || !block->ptr || block->bytes == 0) {
    return;
  }
#if defined(_WIN32)
  (void)hz6_win_virtualalloc_release(block->ptr, block->bytes);
#else
  (void)hz6_linux_mmap_release(block->ptr, block->bytes);
#endif
  block->ptr = NULL;
  block->bytes = 0;
}
