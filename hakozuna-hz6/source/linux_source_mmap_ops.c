#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "linux_source_mmap.h"

size_t hz6_linux_page_size(void);
void* hz6_linux_mmap_reserve(size_t bytes, size_t align);
int hz6_linux_mmap_commit(void* p, size_t bytes);
int hz6_linux_mmap_decommit(void* p, size_t bytes);
int hz6_linux_mmap_release(void* p, size_t bytes);

Hz6OsMemoryOps hz6_linux_mmap_source_ops(void) {
  size_t page_size = hz6_linux_page_size();
  Hz6OsMemoryOps ops;
  ops.reserve = hz6_linux_mmap_reserve;
  ops.commit = hz6_linux_mmap_commit;
  ops.decommit = hz6_linux_mmap_decommit;
  ops.release = hz6_linux_mmap_release;
  ops.page_size = page_size;
  ops.allocation_granularity = page_size;
  return ops;
}
