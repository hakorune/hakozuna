#pragma once

#ifndef HZ3_WIN_MMAN_STATS
#define HZ3_WIN_MMAN_STATS 0
#endif

static inline void hz3_win_mman_stats_mmap(size_t len, int prot) {
    (void)len;
    (void)prot;
}

static inline void hz3_win_mman_stats_munmap(size_t len) {
    (void)len;
}

static inline void hz3_win_mman_stats_madvise(size_t len) {
    (void)len;
}
