#pragma once

#if !defined(_WIN32)
#error "sched shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static inline int sched_yield(void) {
    return SwitchToThread() ? 0 : 0;
}
