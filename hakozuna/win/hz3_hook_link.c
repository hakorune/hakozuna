// HookBox (link mode): allocator boundary for Windows
// - Single boundary: all malloc/free-family calls flow here.
// - Fail-fast on unresolved CRT or unsupported calls.

#include "hz3_hook_link.h"
#include "hz3.h"
#include "hz3_large.h"
#include "hz3_types.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef void* (*Hz3MallocFn)(size_t);
typedef void  (*Hz3FreeFn)(void*);
typedef void* (*Hz3CallocFn)(size_t, size_t);
typedef void* (*Hz3ReallocFn)(void*, size_t);
typedef size_t (*Hz3MsizeFn)(void*);

static INIT_ONCE g_hz3_hook_once = INIT_ONCE_STATIC_INIT;
static int g_hz3_hook_enabled = 1;
static int g_hz3_hook_failfast = 1;
static int g_hz3_hook_log_shot = 0;
static __declspec(thread) int g_hz3_hook_depth = 0;

static HMODULE g_hz3_crt = NULL;
static Hz3MallocFn g_crt_malloc = NULL;
static Hz3FreeFn g_crt_free = NULL;
static Hz3CallocFn g_crt_calloc = NULL;
static Hz3ReallocFn g_crt_realloc = NULL;
static Hz3MsizeFn g_crt_msize = NULL;

static int hz3_win_env_flag(const char* name, int defval) {
    char buf[32];
    DWORD n = GetEnvironmentVariableA(name, buf, (DWORD)sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        return defval;
    }
    return (buf[0] != '0') ? 1 : 0;
}

static void hz3_hook_abort(const char* where) {
    fprintf(stderr, "[HZ3_HOOK_FAILFAST] %s\n", where ? where : "unknown");
    abort();
}

static void hz3_hook_resolve_crt(void) {
    g_hz3_crt = GetModuleHandleA("ucrtbase.dll");
    if (!g_hz3_crt) {
        g_hz3_crt = GetModuleHandleA("msvcrt.dll");
    }
    if (!g_hz3_crt) {
        g_hz3_crt = LoadLibraryA("ucrtbase.dll");
    }
    if (!g_hz3_crt) {
        g_hz3_crt = LoadLibraryA("msvcrt.dll");
    }
    if (!g_hz3_crt) {
        return;
    }
    g_crt_malloc = (Hz3MallocFn)GetProcAddress(g_hz3_crt, "malloc");
    g_crt_free = (Hz3FreeFn)GetProcAddress(g_hz3_crt, "free");
    g_crt_calloc = (Hz3CallocFn)GetProcAddress(g_hz3_crt, "calloc");
    g_crt_realloc = (Hz3ReallocFn)GetProcAddress(g_hz3_crt, "realloc");
    g_crt_msize = (Hz3MsizeFn)GetProcAddress(g_hz3_crt, "_msize");
}

static BOOL CALLBACK hz3_hook_init_cb(PINIT_ONCE once, PVOID param, PVOID* context) {
    (void)once;
    (void)param;
    (void)context;
    g_hz3_hook_enabled = hz3_win_env_flag("HZ3_HOOK_ENABLE", 1);
    g_hz3_hook_failfast = hz3_win_env_flag("HZ3_HOOK_FAILFAST", 1);
    g_hz3_hook_log_shot = hz3_win_env_flag("HZ3_HOOK_LOG_SHOT", 0);

    hz3_hook_resolve_crt();

    if ((!g_crt_malloc || !g_crt_free || !g_crt_calloc || !g_crt_realloc) &&
        !g_hz3_hook_enabled && g_hz3_hook_failfast) {
        hz3_hook_abort("crt_resolve_failed");
    }

    if (g_hz3_hook_log_shot) {
        fprintf(stderr, "[HZ3_HOOK] enable=%d failfast=%d crt=%s\n",
                g_hz3_hook_enabled,
                g_hz3_hook_failfast,
                g_hz3_crt ? "ok" : "missing");
    }
    return TRUE;
}

static inline void hz3_hook_init_once(void) {
    InitOnceExecuteOnce(&g_hz3_hook_once, hz3_hook_init_cb, NULL, NULL);
}

static inline int hz3_hook_guard_enter(void) {
    if (g_hz3_hook_depth > 0) {
        return 0;
    }
    g_hz3_hook_depth = 1;
    return 1;
}

static inline void hz3_hook_guard_leave(void) {
    if (g_hz3_hook_depth > 0) {
        g_hz3_hook_depth = 0;
    }
}

static inline int hz3_is_pow2(size_t x) {
    return x && ((x & (x - 1u)) == 0);
}

void* hz3_hook_malloc(size_t size) {
    hz3_hook_init_once();
    if (!hz3_hook_guard_enter()) {
        if (!g_crt_malloc) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_malloc_missing");
            return NULL;
        }
        return g_crt_malloc(size);
    }
    if (!g_hz3_hook_enabled) {
        if (!g_crt_malloc) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_malloc_missing");
            hz3_hook_guard_leave();
            return NULL;
        }
        void* out = g_crt_malloc(size);
        hz3_hook_guard_leave();
        return out;
    }
    void* out = hz3_malloc(size);
    hz3_hook_guard_leave();
    return out;
}

void hz3_hook_free(void* ptr) {
    hz3_hook_init_once();
    if (!hz3_hook_guard_enter()) {
        if (!g_crt_free) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_free_missing");
            return;
        }
        g_crt_free(ptr);
        return;
    }
    if (!g_hz3_hook_enabled) {
        if (!g_crt_free) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_free_missing");
            hz3_hook_guard_leave();
            return;
        }
        g_crt_free(ptr);
        hz3_hook_guard_leave();
        return;
    }
    hz3_free(ptr);
    hz3_hook_guard_leave();
}

void* hz3_hook_calloc(size_t n, size_t size) {
    hz3_hook_init_once();
    if (!hz3_hook_guard_enter()) {
        if (!g_crt_calloc) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_calloc_missing");
            return NULL;
        }
        return g_crt_calloc(n, size);
    }
    if (!g_hz3_hook_enabled) {
        if (!g_crt_calloc) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_calloc_missing");
            hz3_hook_guard_leave();
            return NULL;
        }
        void* out = g_crt_calloc(n, size);
        hz3_hook_guard_leave();
        return out;
    }
    void* out = hz3_calloc(n, size);
    hz3_hook_guard_leave();
    return out;
}

void* hz3_hook_realloc(void* ptr, size_t size) {
    hz3_hook_init_once();
    if (!hz3_hook_guard_enter()) {
        if (!g_crt_realloc) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_realloc_missing");
            return NULL;
        }
        return g_crt_realloc(ptr, size);
    }
    if (!g_hz3_hook_enabled) {
        if (!g_crt_realloc) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_realloc_missing");
            hz3_hook_guard_leave();
            return NULL;
        }
        void* out = g_crt_realloc(ptr, size);
        hz3_hook_guard_leave();
        return out;
    }
    void* out = hz3_realloc(ptr, size);
    hz3_hook_guard_leave();
    return out;
}

int hz3_hook_posix_memalign(void** memptr, size_t alignment, size_t size) {
    hz3_hook_init_once();
    if (!memptr) {
        return EINVAL;
    }
    if (!hz3_is_pow2(alignment) || alignment < sizeof(void*)) {
        return EINVAL;
    }
    if (!hz3_hook_guard_enter()) {
        if (g_hz3_hook_failfast) hz3_hook_abort("posix_memalign_reentry");
        return ENOMEM;
    }

    if (!g_hz3_hook_enabled) {
        if (g_hz3_hook_failfast) hz3_hook_abort("posix_memalign_disabled");
        hz3_hook_guard_leave();
        return ENOMEM;
    }

    void* p = (alignment <= HZ3_SMALL_ALIGN) ? hz3_malloc(size)
                                             : hz3_large_aligned_alloc(alignment, size);
    if (!p) {
        hz3_hook_guard_leave();
        return ENOMEM;
    }
    *memptr = p;
    hz3_hook_guard_leave();
    return 0;
}

void* hz3_hook_aligned_alloc(size_t alignment, size_t size) {
    hz3_hook_init_once();
    if (!hz3_is_pow2(alignment) || alignment == 0) {
        errno = EINVAL;
        return NULL;
    }
    if ((size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }
    if (!hz3_hook_guard_enter()) {
        if (g_hz3_hook_failfast) hz3_hook_abort("aligned_alloc_reentry");
        errno = ENOMEM;
        return NULL;
    }

    if (!g_hz3_hook_enabled) {
        if (g_hz3_hook_failfast) hz3_hook_abort("aligned_alloc_disabled");
        hz3_hook_guard_leave();
        errno = ENOMEM;
        return NULL;
    }

    if (alignment <= HZ3_SMALL_ALIGN) {
        void* out = hz3_malloc(size);
        hz3_hook_guard_leave();
        return out;
    }
    void* p = hz3_large_aligned_alloc(alignment, size);
    if (!p) {
        errno = ENOMEM;
    }
    hz3_hook_guard_leave();
    return p;
}

size_t hz3_hook_malloc_usable_size(void* ptr) {
    hz3_hook_init_once();
    if (!hz3_hook_guard_enter()) {
        if (!g_crt_msize) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_msize_missing");
            return 0;
        }
        return g_crt_msize(ptr);
    }
    if (!g_hz3_hook_enabled) {
        if (!g_crt_msize) {
            if (g_hz3_hook_failfast) hz3_hook_abort("crt_msize_missing");
            hz3_hook_guard_leave();
            return 0;
        }
        size_t out = g_crt_msize(ptr);
        hz3_hook_guard_leave();
        return out;
    }
    size_t out = hz3_usable_size(ptr);
    hz3_hook_guard_leave();
    return out;
}

// Exported C allocator symbols (boundary entry points).
void* malloc(size_t size) { return hz3_hook_malloc(size); }
void  free(void* ptr) { hz3_hook_free(ptr); }
void* calloc(size_t n, size_t size) { return hz3_hook_calloc(n, size); }
void* realloc(void* ptr, size_t size) { return hz3_hook_realloc(ptr, size); }
int   posix_memalign(void** memptr, size_t alignment, size_t size) {
    return hz3_hook_posix_memalign(memptr, alignment, size);
}
void* aligned_alloc(size_t alignment, size_t size) {
    return hz3_hook_aligned_alloc(alignment, size);
}
size_t malloc_usable_size(void* ptr) { return hz3_hook_malloc_usable_size(ptr); }
