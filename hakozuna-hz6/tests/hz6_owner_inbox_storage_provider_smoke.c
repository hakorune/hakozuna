#include "hz6_allocator_owner_inbox_storage_provider.h"

#include <stdint.h>
#include <stdio.h>

int main(void) {
  Hz6OwnerInboxStorageBlock block = hz6_owner_inbox_storage_alloc(12345u);
  if (!block.ptr || block.bytes < 12345u) {
    fprintf(stderr, "owner inbox storage alloc failed\n");
    return 1;
  }
  unsigned char* bytes = (unsigned char*)block.ptr;
  for (size_t i = 0; i < 12345u; ++i) {
    if (bytes[i] != 0) {
      fprintf(stderr, "owner inbox storage was not zero-filled\n");
      hz6_owner_inbox_storage_free(&block);
      return 2;
    }
  }
  bytes[0] = 0x6u;
  bytes[12344u] = 0x6u;
  hz6_owner_inbox_storage_free(&block);
  if (block.ptr || block.bytes != 0) {
    fprintf(stderr, "owner inbox storage free did not clear block\n");
    return 3;
  }
  return 0;
}
