#ifndef HZ6_SOURCE_H
#define HZ6_SOURCE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6OsMemoryOps {
  void* (*reserve)(size_t bytes, size_t align);
  int (*commit)(void* p, size_t bytes);
  int (*decommit)(void* p, size_t bytes);
  int (*release)(void* p, size_t bytes);
  size_t page_size;
  size_t allocation_granularity;
} Hz6OsMemoryOps;

typedef enum Hz6SourceKind {
  HZ6_SOURCE_NONE = 0,
  HZ6_SOURCE_SYSTEM = 1,
  HZ6_SOURCE_LINUX_MMAP = 2,
  HZ6_SOURCE_WIN_VIRTUALALLOC = 3,
  HZ6_SOURCE_OS_PAGED = 4
} Hz6SourceKind;

int hz6_source_ops_valid(const Hz6OsMemoryOps* ops);

Hz6OsMemoryOps hz6_system_source_ops(void);

void* hz6_source_system_alloc(size_t bytes);

void hz6_source_system_free(void* ptr);

int hz6_source_system_release(void* ptr, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif
