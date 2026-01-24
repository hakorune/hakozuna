#pragma once

#if !defined(_WIN32)
#error "unistd shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>

typedef intptr_t ssize_t;

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 30
#endif

static inline long sysconf(int name) {
    if (name != _SC_PAGESIZE) {
        return -1;
    }
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (long)info.dwPageSize;
}

static inline int usleep(unsigned int usec) {
    Sleep((DWORD)((usec + 999) / 1000));
    return 0;
}
