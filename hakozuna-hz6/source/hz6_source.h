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

int hz6_source_ops_valid(const Hz6OsMemoryOps* ops);

void* hz6_source_system_alloc(size_t bytes);

void hz6_source_system_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
