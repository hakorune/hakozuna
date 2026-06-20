#ifndef HZ6_LINUX_SOURCE_MMAP_H
#define HZ6_LINUX_SOURCE_MMAP_H

#include "hz6_source.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

Hz6OsMemoryOps hz6_linux_mmap_source_ops(void);

typedef struct Hz6LinuxMmapRetainStats {
  size_t reserve_calls;
  size_t reserve_64k_calls;
  size_t retain_64k_take_hit;
  size_t retain_64k_take_miss;
  size_t retain_generic_take_hit;
  size_t retain_generic_take_miss;
  size_t mmap_fallback;
  size_t release_calls;
  size_t release_64k_calls;
  size_t retain_64k_put_hit;
  size_t retain_64k_put_full;
  size_t retain_generic_put_hit;
  size_t retain_generic_put_full;
  size_t munmap_fallback;
  size_t retained_bytes;
  size_t retained_64k_count;
} Hz6LinuxMmapRetainStats;

Hz6LinuxMmapRetainStats hz6_linux_mmap_retain_stats_snapshot(void);
size_t hz6_linux_mmap_retain_flush(size_t max_bytes);
size_t hz6_linux_page_size(void);
void* hz6_linux_mmap_reserve(size_t bytes, size_t align);
int hz6_linux_mmap_release(void* p, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif
