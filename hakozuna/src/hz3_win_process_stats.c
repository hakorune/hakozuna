#include "hz3_win_process_stats.h"

#if defined(_WIN32) && HZ3_S257_PROCESS_MEMORY_OBS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef BOOL(WINAPI* Hz3GetProcessMemoryInfoFn)(
    HANDLE,
    PPROCESS_MEMORY_COUNTERS,
    DWORD);

static _Atomic int g_hz3_s257_process_atexit_registered = 0;

static Hz3GetProcessMemoryInfoFn hz3_s257_resolve_get_process_memory_info(void) {
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (kernel32) {
        Hz3GetProcessMemoryInfoFn fn =
            (Hz3GetProcessMemoryInfoFn)GetProcAddress(kernel32, "K32GetProcessMemoryInfo");
        if (fn) {
            return fn;
        }
    }

    HMODULE psapi = LoadLibraryA("psapi.dll");
    if (psapi) {
        return (Hz3GetProcessMemoryInfoFn)GetProcAddress(psapi, "GetProcessMemoryInfo");
    }
    return NULL;
}

static void hz3_s257_process_memory_dump(void) {
    Hz3GetProcessMemoryInfoFn get_info = hz3_s257_resolve_get_process_memory_info();
    if (!get_info) {
        fprintf(stderr, "[HZ3_S257_PROCESS] available=0 error=get_process_memory_info_unavailable\n");
        return;
    }

    PROCESS_MEMORY_COUNTERS_EX counters;
    memset(&counters, 0, sizeof(counters));
    counters.cb = sizeof(counters);
    if (!get_info(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&counters, sizeof(counters))) {
        fprintf(stderr,
                "[HZ3_S257_PROCESS] available=0 error=get_process_memory_info_failed "
                "winerr=%lu\n",
                (unsigned long)GetLastError());
        return;
    }

    fprintf(stderr,
            "[HZ3_S257_PROCESS] available=1 page_fault_count=%lu "
            "peak_working_set_bytes=%llu working_set_bytes=%llu "
            "quota_peak_paged_pool_bytes=%llu quota_paged_pool_bytes=%llu "
            "quota_peak_nonpaged_pool_bytes=%llu quota_nonpaged_pool_bytes=%llu "
            "pagefile_bytes=%llu peak_pagefile_bytes=%llu private_usage_bytes=%llu\n",
            (unsigned long)counters.PageFaultCount,
            (unsigned long long)counters.PeakWorkingSetSize,
            (unsigned long long)counters.WorkingSetSize,
            (unsigned long long)counters.QuotaPeakPagedPoolUsage,
            (unsigned long long)counters.QuotaPagedPoolUsage,
            (unsigned long long)counters.QuotaPeakNonPagedPoolUsage,
            (unsigned long long)counters.QuotaNonPagedPoolUsage,
            (unsigned long long)counters.PagefileUsage,
            (unsigned long long)counters.PeakPagefileUsage,
            (unsigned long long)counters.PrivateUsage);
}

void hz3_win_process_stats_register(void) {
    if (atomic_exchange_explicit(&g_hz3_s257_process_atexit_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_s257_process_memory_dump);
    }
}

#endif
