#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <time.h>

static inline int gettimeofday(struct timeval* tv, void* tz) {
    (void)tz;
    if (tv == NULL) {
        return -1;
    }

    FILETIME ft;
    ULARGE_INTEGER ticks;
    GetSystemTimeAsFileTime(&ft);
    ticks.LowPart = ft.dwLowDateTime;
    ticks.HighPart = ft.dwHighDateTime;

    const unsigned long long windows_to_unix_epoch_100ns = 116444736000000000ULL;
    unsigned long long unix_time_100ns = ticks.QuadPart - windows_to_unix_epoch_100ns;
    tv->tv_sec = (long)(unix_time_100ns / 10000000ULL);
    tv->tv_usec = (long)((unix_time_100ns % 10000000ULL) / 10ULL);
    return 0;
}

#ifndef timersub
#define timersub(a, b, result)                                                   \
    do {                                                                         \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                            \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                         \
        if ((result)->tv_usec < 0) {                                             \
            --(result)->tv_sec;                                                  \
            (result)->tv_usec += 1000000;                                        \
        }                                                                        \
    } while (0)
#endif
