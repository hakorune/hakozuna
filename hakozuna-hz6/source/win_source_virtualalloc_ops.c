#include "win_source_virtualalloc.h"

#if defined(_WIN32)
#include <windows.h>

size_t hz6_win_allocation_granularity(void);
size_t hz6_win_page_size(void);
void* hz6_win_virtualalloc_reserve(size_t bytes, size_t align);
int hz6_win_virtualalloc_commit(void* p, size_t bytes);
int hz6_win_virtualalloc_decommit(void* p, size_t bytes);
int hz6_win_virtualalloc_release(void* p, size_t bytes);

Hz6OsMemoryOps hz6_win_virtualalloc_source_ops(void) {
  Hz6OsMemoryOps ops;
  ops.reserve = hz6_win_virtualalloc_reserve;
  ops.commit = hz6_win_virtualalloc_commit;
  ops.decommit = hz6_win_virtualalloc_decommit;
  ops.release = hz6_win_virtualalloc_release;
  ops.page_size = hz6_win_page_size();
  ops.allocation_granularity = hz6_win_allocation_granularity();
  return ops;
}
#endif
