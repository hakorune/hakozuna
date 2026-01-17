#pragma once

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <time.h>

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

static inline int hz3_clock_gettime_monotonic(struct timespec* ts) {
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&freq) || !QueryPerformanceCounter(&counter)) {
        return -1;
    }
    uint64_t ns = (uint64_t)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
    ts->tv_sec = (time_t)(ns / 1000000000ULL);
    ts->tv_nsec = (long)(ns % 1000000000ULL);
    return 0;
}

// Provide clock_gettime on Windows for code that expects it.
static inline int clock_gettime(int clock_id, struct timespec* ts) {
    if (clock_id != CLOCK_MONOTONIC || !ts) {
        return -1;
    }
    return hz3_clock_gettime_monotonic(ts);
}
#endif
