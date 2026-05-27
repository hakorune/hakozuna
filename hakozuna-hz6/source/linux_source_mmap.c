#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "linux_source_mmap.h"

#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t hz6_linux_page_size(void) {
  long value = sysconf(_SC_PAGESIZE);
  return value > 0 ? (size_t)value : (size_t)4096;
}

static size_t hz6_round_up(size_t value, size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  size_t mask = alignment - (size_t)1;
  return (value + mask) & ~mask;
}

static void* hz6_linux_mmap_reserve(size_t bytes, size_t align) {
  size_t page_size = hz6_linux_page_size();
  size_t effective_align = align > page_size ? align : page_size;
  size_t rounded = hz6_round_up(bytes, effective_align);
  if (rounded == 0) {
    errno = EINVAL;
    return NULL;
  }

  void* p = mmap(NULL, rounded, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  return p == MAP_FAILED ? NULL : p;
}

static int hz6_linux_mmap_commit(void* p, size_t bytes) {
  (void)p;
  return bytes != 0;
}

static int hz6_linux_mmap_decommit(void* p, size_t bytes) {
  if (!p || bytes == 0) {
    return 0;
  }
  return madvise(p, bytes, MADV_DONTNEED) == 0;
}

static int hz6_linux_mmap_release(void* p, size_t bytes) {
  if (!p || bytes == 0) {
    return 0;
  }
  return munmap(p, bytes) == 0;
}

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
