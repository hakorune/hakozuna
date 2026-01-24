#pragma once

#if !defined(_WIN32)
#error "syscall shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#define SYS_gettid 0

static inline long syscall(long number, ...) {
    (void)number;
    return (long)GetCurrentThreadId();
}
