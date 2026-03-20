// hz4_foreign_probe.c - Windows foreign-pointer smoke for HZ4 ownership checks.
//
// Build this file twice:
// - once with HZ4_FOREIGN_HELPER_DLL=1 to produce the helper DLL
// - once without that define to produce the host exe

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HZ4_FOREIGN_HELPER_DLL)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void* g_foreign_reserve_base = NULL;
static void* g_foreign_commit_base = NULL;
static size_t g_foreign_reserve_size = 0;
static size_t g_foreign_commit_size = 0;

static size_t round_up_size(size_t value, size_t align) {
    return (value + (align - 1u)) & ~(align - 1u);
}

__declspec(dllexport) void* hz4_foreign_reserve_base(void) {
    return g_foreign_reserve_base;
}

__declspec(dllexport) void* hz4_foreign_commit_base(void) {
    return g_foreign_commit_base;
}

__declspec(dllexport) size_t hz4_foreign_reserve_size(void) {
    return g_foreign_reserve_size;
}

__declspec(dllexport) size_t hz4_foreign_commit_size(void) {
    return g_foreign_commit_size;
}

__declspec(dllexport) void* hz4_foreign_alloc(size_t reserve_size,
                                              size_t commit_offset,
                                              size_t commit_size) {
    SYSTEM_INFO si;
    BYTE* reserve_base;
    BYTE* commit_base;

    GetSystemInfo(&si);
    reserve_size = round_up_size(reserve_size ? reserve_size : si.dwAllocationGranularity,
                                 si.dwAllocationGranularity);
    commit_offset = round_up_size(commit_offset ? commit_offset : si.dwPageSize,
                                  si.dwPageSize);
    commit_size = round_up_size(commit_size ? commit_size : si.dwPageSize,
                                si.dwPageSize);

    if (commit_offset + commit_size > reserve_size) {
        return NULL;
    }

    reserve_base = (BYTE*)VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_NOACCESS);
    if (!reserve_base) {
        return NULL;
    }

    commit_base = (BYTE*)VirtualAlloc(reserve_base + commit_offset,
                                      commit_size, MEM_COMMIT, PAGE_READWRITE);
    if (!commit_base) {
        VirtualFree(reserve_base, 0, MEM_RELEASE);
        return NULL;
    }

    memset(commit_base, 0xC5, commit_size);

    g_foreign_reserve_base = reserve_base;
    g_foreign_commit_base = commit_base;
    g_foreign_reserve_size = reserve_size;
    g_foreign_commit_size = commit_size;

    // Return a pointer inside the committed page so HZ4 will align down to the
    // 64 KiB reservation base on Windows.
    return commit_base + 128u;
}

__declspec(dllexport) void hz4_foreign_release(void) {
    if (g_foreign_reserve_base) {
        VirtualFree(g_foreign_reserve_base, 0, MEM_RELEASE);
    }
    g_foreign_reserve_base = NULL;
    g_foreign_commit_base = NULL;
    g_foreign_reserve_size = 0;
    g_foreign_commit_size = 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance;
    (void)reserved;
    if (reason == DLL_PROCESS_DETACH) {
        hz4_foreign_release();
    }
    return TRUE;
}

#else

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "hz4_win_api.h"

typedef void* (*hz4_foreign_alloc_fn)(size_t reserve_size,
                                     size_t commit_offset,
                                     size_t commit_size);
typedef void (*hz4_foreign_release_fn)(void);
typedef void* (*hz4_foreign_base_fn)(void);
typedef size_t (*hz4_foreign_size_fn)(void);

static void die(const char* what) {
    fprintf(stderr, "%s\n", what);
    exit(1);
}

static void dump_region(const char* label, const void* ptr) {
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T got = VirtualQuery(ptr, &mbi, sizeof(mbi));
    if (!got) {
        fprintf(stderr, "%s: VirtualQuery failed for %p (err=%lu)\n",
                label, ptr, (unsigned long)GetLastError());
        return;
    }

    const char* state = "unknown";
    if (mbi.State == MEM_RESERVE) {
        state = "MEM_RESERVE";
    } else if (mbi.State == MEM_COMMIT) {
        state = "MEM_COMMIT";
    } else if (mbi.State == MEM_FREE) {
        state = "MEM_FREE";
    }

    printf("[%s] ptr=%p base=%p state=%s protect=0x%lx region=%zu\n",
           label, ptr, mbi.BaseAddress, state, (unsigned long)mbi.Protect,
           (size_t)mbi.RegionSize);
}

static int check_control_pointer(void) {
    void* p = hz4_win_malloc(256);
    if (!p) {
        die("control allocation failed");
    }

    size_t usable = hz4_win_usable_size(p);
    if (usable == 0) {
        fprintf(stderr, "control usable_size unexpectedly returned 0\n");
        hz4_win_free(p);
        return 1;
    }

    printf("[control] ptr=%p usable_size=%zu\n", p, usable);
    hz4_win_free(p);
    return 0;
}

static int check_foreign_pointer(hz4_foreign_alloc_fn alloc_fn,
                                 hz4_foreign_release_fn release_fn,
                                 hz4_foreign_base_fn base_fn,
                                 hz4_foreign_size_fn reserve_size_fn,
                                 hz4_foreign_size_fn commit_size_fn,
                                 const char* mode) {
    void* ptr;
    void* reserve_base;
    void* commit_base;
    size_t reserve_size;
    size_t commit_size;
    size_t usable = 0;
    DWORD exception_code = 0;
    int do_usable = 0;
    int do_free = 0;
    int do_realloc = 0;
    void* next = NULL;

    if (strcmp(mode, "usable") == 0) {
        do_usable = 1;
    } else if (strcmp(mode, "free") == 0) {
        do_free = 1;
    } else if (strcmp(mode, "realloc") == 0) {
        do_realloc = 1;
    } else {
        fprintf(stderr, "unsupported mode: %s\n", mode);
        return 2;
    }

    ptr = alloc_fn(65536u, 4096u, 4096u);
    if (!ptr) {
        die("foreign allocation failed");
    }

    reserve_base = base_fn();
    commit_base = (void*)((uintptr_t)ptr & ~((uintptr_t)4096u - 1u));
    reserve_size = reserve_size_fn();
    commit_size = commit_size_fn();

    dump_region("foreign.reserve", reserve_base);
    dump_region("foreign.commit", commit_base);
    dump_region("foreign.ptr", ptr);
    printf("[foreign] reserve_size=%zu commit_size=%zu ptr_offset=%zu\n",
           reserve_size, commit_size, (size_t)((BYTE*)ptr - (BYTE*)commit_base));

    if (do_usable) {
        __try {
            usable = hz4_win_usable_size(ptr);
            printf("[foreign] hz4_win_usable_size(%p) -> %zu\n", ptr, usable);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            exception_code = EXCEPTION_ACCESS_VIOLATION;
            fprintf(stderr, "[foreign] exception=0x%08lx while probing usable_size\n",
                    (unsigned long)exception_code);
            release_fn();
            return 100 + (int)(exception_code == EXCEPTION_ACCESS_VIOLATION);
        }
        if (usable == 0) {
            printf("[foreign] usable_size returned 0 (safe reject)\n");
        }
    }

    if (do_free) {
        __try {
            hz4_win_free(ptr);
            printf("[foreign] hz4_win_free(%p) returned\n", ptr);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            exception_code = EXCEPTION_ACCESS_VIOLATION;
            fprintf(stderr, "[foreign] exception=0x%08lx while probing free\n",
                    (unsigned long)exception_code);
            release_fn();
            return 100 + (int)(exception_code == EXCEPTION_ACCESS_VIOLATION);
        }
        ptr = NULL;
    }

    if (do_realloc) {
        __try {
            next = hz4_win_realloc(ptr, 512u);
            printf("[foreign] hz4_win_realloc(%p, 512) -> %p\n", ptr, next);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            exception_code = EXCEPTION_ACCESS_VIOLATION;
            fprintf(stderr, "[foreign] exception=0x%08lx while probing realloc\n",
                    (unsigned long)exception_code);
            release_fn();
            return 100 + (int)(exception_code == EXCEPTION_ACCESS_VIOLATION);
        }
        if (next) {
            hz4_win_free(next);
        }
    }

    release_fn();
    return 0;
}

static HMODULE load_helper_dll(const char* exe_path, char* dll_path, size_t dll_path_cap) {
    char* slash;
    size_t len;

    if (!exe_path || !*exe_path) {
        return NULL;
    }

    len = strlen(exe_path);
    if (len + 1 >= dll_path_cap) {
        return NULL;
    }
    memcpy(dll_path, exe_path, len + 1u);
    slash = strrchr(dll_path, '\\');
    if (!slash) {
        slash = strrchr(dll_path, '/');
    }
    if (slash) {
        slash[1] = '\0';
    } else {
        dll_path[0] = '\0';
    }
    if (strlen(dll_path) + strlen("hz4_foreign_probe_helper.dll") + 1 >= dll_path_cap) {
        return NULL;
    }
    strcat_s(dll_path, dll_path_cap, "hz4_foreign_probe_helper.dll");
    return LoadLibraryA(dll_path);
}

int main(int argc, char** argv) {
    int runs = 1;
    const char* mode = "usable";
    int argi;

    for (argi = 1; argi < argc; ++argi) {
        if (strcmp(argv[argi], "--runs") == 0) {
            if (argi + 1 >= argc) {
                die("missing value after --runs");
            }
            runs = atoi(argv[++argi]);
            continue;
        }
        if (strcmp(argv[argi], "--mode") == 0) {
            if (argi + 1 >= argc) {
                die("missing value after --mode");
            }
            mode = argv[++argi];
            continue;
        }
        if (strcmp(argv[argi], "--help") == 0) {
            printf("usage: %s [--runs N] [--mode usable|free|realloc]\n", argv[0]);
            return 0;
        }
        die("unknown argument");
    }

    if (runs < 1) {
        runs = 1;
    }

    char exe_path[MAX_PATH];
    char dll_path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        die("GetModuleFileNameA failed");
    }
    HMODULE helper = load_helper_dll(exe_path, dll_path, sizeof(dll_path));
    if (!helper) {
        die("LoadLibraryA(helper) failed");
    }

    hz4_foreign_alloc_fn alloc_fn =
        (hz4_foreign_alloc_fn)GetProcAddress(helper, "hz4_foreign_alloc");
    hz4_foreign_release_fn release_fn =
        (hz4_foreign_release_fn)GetProcAddress(helper, "hz4_foreign_release");
    hz4_foreign_base_fn base_fn =
        (hz4_foreign_base_fn)GetProcAddress(helper, "hz4_foreign_reserve_base");
    hz4_foreign_size_fn reserve_size_fn =
        (hz4_foreign_size_fn)GetProcAddress(helper, "hz4_foreign_reserve_size");
    hz4_foreign_size_fn commit_size_fn =
        (hz4_foreign_size_fn)GetProcAddress(helper, "hz4_foreign_commit_size");
    if (!alloc_fn || !release_fn || !base_fn || !reserve_size_fn || !commit_size_fn) {
        die("GetProcAddress failed");
    }

    printf("helper=%s\n", dll_path);
    printf("mode=%s runs=%d\n", mode, runs);

    for (int i = 0; i < runs; ++i) {
        int rc = check_control_pointer();
        if (rc != 0) {
            FreeLibrary(helper);
            return rc;
        }

        rc = check_foreign_pointer(alloc_fn, release_fn, base_fn,
                                    reserve_size_fn, commit_size_fn, mode);
        if (rc != 0) {
            FreeLibrary(helper);
            return rc;
        } else {
            continue;
        }
    }

    FreeLibrary(helper);
    return 0;
}

#endif
