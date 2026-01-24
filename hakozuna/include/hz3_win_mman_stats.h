#pragma once

#include <stddef.h>

#ifndef HZ3_WIN_MMAN_STATS
#define HZ3_WIN_MMAN_STATS 0
#endif

#if HZ3_WIN_MMAN_STATS
void hz3_win_mman_stats_mmap(size_t len, int prot);
void hz3_win_mman_stats_munmap(size_t len);
void hz3_win_mman_stats_madvise(size_t len);
#endif
