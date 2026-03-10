#pragma once

#if !defined(_WIN32)
#error "memcached mman shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <sys/types.h>

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20

#define MADV_DONTNEED   4
#define MADV_FREE       8
#define MADV_HUGEPAGE   14
#define MADV_NOHUGEPAGE 15

#define MS_ASYNC      1
#define MS_INVALIDATE 2
#define MS_SYNC       4

#define MCL_CURRENT 1
#define MCL_FUTURE  2

#define MAP_FAILED ((void*)-1)

static inline void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off) {
    (void)flags;
    (void)fd;
    (void)off;

    DWORD protect = PAGE_READWRITE;
    DWORD alloc = MEM_RESERVE | MEM_COMMIT;
    if (prot == PROT_NONE) {
        protect = PAGE_NOACCESS;
    } else if ((prot & PROT_WRITE) == 0) {
        protect = PAGE_READONLY;
    }

    if ((flags & MAP_FIXED) != 0) {
        alloc = MEM_COMMIT;
    }

    void* p = VirtualAlloc(addr, len, alloc, protect);
    return p ? p : MAP_FAILED;
}

static inline int munmap(void* addr, size_t len) {
    if (VirtualFree(addr, 0, MEM_RELEASE)) {
        return 0;
    }
    if (len != 0 && VirtualFree(addr, len, MEM_DECOMMIT)) {
        return 0;
    }
    return -1;
}

static inline int madvise(void* addr, size_t len, int advice) {
    (void)advice;
    if (!addr || len == 0) {
        return 0;
    }
    return VirtualFree(addr, len, MEM_DECOMMIT) ? 0 : -1;
}

static inline int msync(void* addr, size_t len, int flags) {
    (void)flags;
    if (!addr || len == 0) {
        return 0;
    }
    return FlushViewOfFile(addr, len) ? 0 : -1;
}

static inline int mlockall(int flags) {
    (void)flags;
    return 0;
}
