#pragma once

#if !defined(_WIN32)
#error "mman shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>

#include "hz3_win_mman_stats.h"

typedef long long off_t;

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2

#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FIXED     0x10

#define MADV_DONTNEED   4
#define MADV_FREE       8
#define MADV_HUGEPAGE   14
#define MADV_NOHUGEPAGE 15

#define MAP_FAILED ((void*)-1)

static inline size_t hz3_win_page_size(void) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (size_t)info.dwPageSize;
}

static inline void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off) {
    (void)flags;
    (void)fd;
    (void)off;
    DWORD protect = PAGE_READWRITE;
    DWORD alloc = MEM_RESERVE;
    if (prot == PROT_NONE) {
        protect = PAGE_NOACCESS;
    } else {
        if (prot & PROT_WRITE) {
            protect = PAGE_READWRITE;
        } else {
            protect = PAGE_READONLY;
        }
        if (flags & MAP_FIXED) {
            alloc = MEM_COMMIT;
        } else {
            alloc |= MEM_COMMIT;
        }
    }
    void* p = VirtualAlloc(addr, len, alloc, protect);
#if HZ3_WIN_MMAN_STATS
    hz3_win_mman_stats_mmap(len, prot);
#endif
    return p ? p : MAP_FAILED;
}

static inline int munmap(void* addr, size_t len) {
    if (VirtualFree(addr, 0, MEM_RELEASE)) {
#if HZ3_WIN_MMAN_STATS
        hz3_win_mman_stats_munmap(len);
#endif
        return 0;
    }
    if (!len) {
        return -1;
    }
    if (VirtualFree(addr, len, MEM_DECOMMIT)) {
#if HZ3_WIN_MMAN_STATS
        hz3_win_mman_stats_munmap(len);
#endif
        return 0;
    }
    return -1;
}

static inline int madvise(void* addr, size_t len, int advice) {
    if (!addr || !len) {
        return 0;
    }
    switch (advice) {
        case MADV_DONTNEED:
        case MADV_FREE:
            if (VirtualFree(addr, len, MEM_DECOMMIT)) {
#if HZ3_WIN_MMAN_STATS
                hz3_win_mman_stats_madvise(len);
#endif
                return 0;
            }
            return -1;
        case MADV_HUGEPAGE:
        case MADV_NOHUGEPAGE:
        default:
            return 0;
    }
}
