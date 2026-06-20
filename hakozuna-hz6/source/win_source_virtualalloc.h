#ifndef HZ6_WIN_SOURCE_VIRTUALALLOC_H
#define HZ6_WIN_SOURCE_VIRTUALALLOC_H

#include "hz6_source.h"

#ifdef __cplusplus
extern "C" {
#endif

Hz6OsMemoryOps hz6_win_virtualalloc_source_ops(void);
size_t hz6_win_page_size(void);
void* hz6_win_virtualalloc_reserve(size_t bytes, size_t align);
int hz6_win_virtualalloc_release(void* p, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif
