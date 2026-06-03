#include "win_source_virtualalloc.h"
#include "hz6_source_util.h"

#if defined(_WIN32)
#include <windows.h>

size_t hz6_win_allocation_granularity(void) {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (size_t)info.dwAllocationGranularity;
}

size_t hz6_win_page_size(void) {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (size_t)info.dwPageSize;
}

void* hz6_win_virtualalloc_reserve(size_t bytes, size_t align) {
  size_t granularity = hz6_win_allocation_granularity();
  size_t effective_align = align > granularity ? align : granularity;
  size_t rounded = hz6_source_round_up(bytes, effective_align);
  if (rounded == 0) {
    return NULL;
  }
  return VirtualAlloc(NULL, rounded, MEM_RESERVE | MEM_COMMIT,
                      PAGE_READWRITE);
}

int hz6_win_virtualalloc_commit(void* p, size_t bytes) {
  return p != NULL && bytes != 0;
}

int hz6_win_virtualalloc_decommit(void* p, size_t bytes) {
  if (!p || bytes == 0) {
    return 0;
  }
  return VirtualFree(p, bytes, MEM_DECOMMIT) != 0;
}

int hz6_win_virtualalloc_release(void* p, size_t bytes) {
  (void)bytes;
  if (!p) {
    return 0;
  }
  return VirtualFree(p, 0, MEM_RELEASE) != 0;
}
#endif
