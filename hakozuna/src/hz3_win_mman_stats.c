#include "hz3_win_mman_stats.h"

#if defined(_WIN32) && HZ3_WIN_MMAN_STATS

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static _Atomic uint64_t g_hz3_win_mmap_calls = 0;
static _Atomic uint64_t g_hz3_win_mmap_bytes = 0;
static _Atomic uint64_t g_hz3_win_mmap_reserve_calls = 0;
static _Atomic uint64_t g_hz3_win_mmap_reserve_bytes = 0;
static _Atomic uint64_t g_hz3_win_mmap_commit_calls = 0;
static _Atomic uint64_t g_hz3_win_mmap_commit_bytes = 0;
static _Atomic uint64_t g_hz3_win_munmap_calls = 0;
static _Atomic uint64_t g_hz3_win_munmap_bytes = 0;
static _Atomic uint64_t g_hz3_win_madvise_calls = 0;
static _Atomic uint64_t g_hz3_win_madvise_bytes = 0;
static _Atomic int g_hz3_win_mman_atexit_registered = 0;

static void hz3_win_mman_dump(void) {
    uint64_t mmap_calls = atomic_load_explicit(&g_hz3_win_mmap_calls, memory_order_relaxed);
    uint64_t mmap_bytes = atomic_load_explicit(&g_hz3_win_mmap_bytes, memory_order_relaxed);
    uint64_t res_calls = atomic_load_explicit(&g_hz3_win_mmap_reserve_calls, memory_order_relaxed);
    uint64_t res_bytes = atomic_load_explicit(&g_hz3_win_mmap_reserve_bytes, memory_order_relaxed);
    uint64_t com_calls = atomic_load_explicit(&g_hz3_win_mmap_commit_calls, memory_order_relaxed);
    uint64_t com_bytes = atomic_load_explicit(&g_hz3_win_mmap_commit_bytes, memory_order_relaxed);
    uint64_t mun_calls = atomic_load_explicit(&g_hz3_win_munmap_calls, memory_order_relaxed);
    uint64_t mun_bytes = atomic_load_explicit(&g_hz3_win_munmap_bytes, memory_order_relaxed);
    uint64_t mad_calls = atomic_load_explicit(&g_hz3_win_madvise_calls, memory_order_relaxed);
    uint64_t mad_bytes = atomic_load_explicit(&g_hz3_win_madvise_bytes, memory_order_relaxed);

    if (mmap_calls == 0 && mun_calls == 0 && mad_calls == 0) {
        return;
    }

    fprintf(stderr,
            "[HZ3_WIN_MMAN] mmap_calls=%llu mmap_bytes=%llu reserve_calls=%llu reserve_bytes=%llu "
            "commit_calls=%llu commit_bytes=%llu munmap_calls=%llu munmap_bytes=%llu "
            "madvise_calls=%llu madvise_bytes=%llu\n",
            (unsigned long long)mmap_calls,
            (unsigned long long)mmap_bytes,
            (unsigned long long)res_calls,
            (unsigned long long)res_bytes,
            (unsigned long long)com_calls,
            (unsigned long long)com_bytes,
            (unsigned long long)mun_calls,
            (unsigned long long)mun_bytes,
            (unsigned long long)mad_calls,
            (unsigned long long)mad_bytes);
}

static void hz3_win_mman_register_atexit(void) {
    if (atomic_exchange_explicit(&g_hz3_win_mman_atexit_registered, 1, memory_order_relaxed) == 0) {
        atexit(hz3_win_mman_dump);
    }
}

void hz3_win_mman_stats_mmap(size_t len, int prot) {
    hz3_win_mman_register_atexit();
    atomic_fetch_add_explicit(&g_hz3_win_mmap_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz3_win_mmap_bytes, (uint64_t)len, memory_order_relaxed);
    if (prot == 0) {
        atomic_fetch_add_explicit(&g_hz3_win_mmap_reserve_calls, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_hz3_win_mmap_reserve_bytes, (uint64_t)len, memory_order_relaxed);
    } else {
        atomic_fetch_add_explicit(&g_hz3_win_mmap_commit_calls, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_hz3_win_mmap_commit_bytes, (uint64_t)len, memory_order_relaxed);
    }
}

void hz3_win_mman_stats_munmap(size_t len) {
    hz3_win_mman_register_atexit();
    atomic_fetch_add_explicit(&g_hz3_win_munmap_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz3_win_munmap_bytes, (uint64_t)len, memory_order_relaxed);
}

void hz3_win_mman_stats_madvise(size_t len) {
    hz3_win_mman_register_atexit();
    atomic_fetch_add_explicit(&g_hz3_win_madvise_calls, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_hz3_win_madvise_bytes, (uint64_t)len, memory_order_relaxed);
}

#endif
