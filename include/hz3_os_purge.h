#pragma once

#include "hz3_arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/mman.h>

// ============================================================================
// OS Purge Box (shared helper)
// ============================================================================
//
// A small “box” to centralize arena range checks and OS purge syscalls.
//
// Design:
// - No getenv() usage (compile-time gating stays in callers).
// - No logging here (callers own stats/one-shot prints).
// - Intended for cold paths (epoch/destructor/maintenance).

static inline int hz3_os_in_arena_range(void* addr, size_t len) {
    void* base = atomic_load_explicit(&g_hz3_arena_base, memory_order_acquire);
    if (!base) {
        return 0;
    }
    void* endp = atomic_load_explicit(&g_hz3_arena_end, memory_order_relaxed);

    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + len;
    if (end < start) {
        return 0;  // overflow
    }

    return (start >= (uintptr_t)base) && (end <= (uintptr_t)endp);
}

int hz3_os_madvise_dontneed_checked(void* addr, size_t len);
