// HookBox (IAT mode): patch main module imports to route to hz3.

#include "hz3_hook_link.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* name;
    void* fn;
    int required;
    int hits;
} Hz3IatHook;

typedef struct {
    int enabled;
    int failfast;
    int log_shot;
    int all_modules;
    HMODULE self;
    char ready_event[128];
} Hz3HookConfig;

static Hz3HookConfig g_hz3_cfg = {1, 1, 0, 0, NULL, {0}};

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

static void hz3_iat_signal_ready(void) {
    if (g_hz3_cfg.ready_event[0] == '\0') {
        return;
    }
    HANDLE ev = OpenEventA(EVENT_MODIFY_STATE, FALSE, g_hz3_cfg.ready_event);
    if (!ev) {
        if (g_hz3_cfg.failfast) {
            hz3_hook_abort("iat_ready_event_missing");
        }
        return;
    }
    if (!SetEvent(ev) && g_hz3_cfg.failfast) {
        CloseHandle(ev);
        hz3_hook_abort("iat_ready_event_set_failed");
    }
    CloseHandle(ev);
}

static int hz3_iat_patch_module(HMODULE module, Hz3IatHook* hooks, size_t hook_count) {
    if (!module) {
        return 0;
    }

    uint8_t* base = (uint8_t*)module;
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    IMAGE_DATA_DIRECTORY* dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (dir->VirtualAddress == 0) {
        return 0;
    }

    IMAGE_IMPORT_DESCRIPTOR* desc = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir->VirtualAddress);
    for (; desc->Name; ++desc) {
        IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)(base + desc->FirstThunk);
        IMAGE_THUNK_DATA* orig = NULL;
        if (desc->OriginalFirstThunk) {
            orig = (IMAGE_THUNK_DATA*)(base + desc->OriginalFirstThunk);
        }
        for (; thunk->u1.AddressOfData; ++thunk) {
            IMAGE_THUNK_DATA* lookup = orig ? orig : thunk;
            if (orig) {
                orig++;
            }
            if (IMAGE_SNAP_BY_ORDINAL(lookup->u1.Ordinal)) {
                continue;
            }
            IMAGE_IMPORT_BY_NAME* byname = (IMAGE_IMPORT_BY_NAME*)(base + lookup->u1.AddressOfData);
            const char* name = (const char*)byname->Name;
            for (size_t i = 0; i < hook_count; i++) {
                if (_stricmp(name, hooks[i].name) != 0) {
                    continue;
                }
                DWORD old_protect = 0;
                if (!VirtualProtect(&thunk->u1.Function, sizeof(void*), PAGE_READWRITE, &old_protect)) {
                    continue;
                }
                thunk->u1.Function = (ULONG_PTR)hooks[i].fn;
                VirtualProtect(&thunk->u1.Function, sizeof(void*), old_protect, &old_protect);
                FlushInstructionCache(GetCurrentProcess(), &thunk->u1.Function, sizeof(void*));
                hooks[i].hits++;
                break;
            }
        }
    }
    return 1;
}

static void hz3_iat_accumulate_hits(const Hz3IatHook* hooks, int* malloc_hits, int* free_hits,
                                    int* calloc_hits, int* realloc_hits) {
    if (hooks) {
        *malloc_hits += hooks[0].hits;
        *free_hits += hooks[1].hits;
        *calloc_hits += hooks[2].hits;
        *realloc_hits += hooks[3].hits;
    }
}

static int hz3_iat_patch_other_modules(HMODULE main_mod, int* malloc_hits, int* free_hits,
                                       int* calloc_hits, int* realloc_hits) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    int modules_patched = 0;
    MODULEENTRY32 me;
    ZeroMemory(&me, sizeof(me));
    me.dwSize = sizeof(me);
    if (!Module32First(snap, &me)) {
        CloseHandle(snap);
        return 0;
    }

    do {
        HMODULE mod = me.hModule;
        if (!mod || mod == main_mod || mod == g_hz3_cfg.self) {
            continue;
        }
        Hz3IatHook hooks[] = {
            { "malloc", (void*)hz3_hook_malloc, 0, 0 },
            { "free", (void*)hz3_hook_free, 0, 0 },
            { "calloc", (void*)hz3_hook_calloc, 0, 0 },
            { "realloc", (void*)hz3_hook_realloc, 0, 0 },
            { "posix_memalign", (void*)hz3_hook_posix_memalign, 0, 0 },
            { "aligned_alloc", (void*)hz3_hook_aligned_alloc, 0, 0 },
            { "malloc_usable_size", (void*)hz3_hook_malloc_usable_size, 0, 0 },
            { "_msize", (void*)hz3_hook_malloc_usable_size, 0, 0 }
        };
        if (hz3_iat_patch_module(mod, hooks, sizeof(hooks) / sizeof(hooks[0]))) {
            modules_patched++;
            hz3_iat_accumulate_hits(hooks, malloc_hits, free_hits, calloc_hits, realloc_hits);
        }
    } while (Module32Next(snap, &me));

    CloseHandle(snap);
    return modules_patched;
}

static DWORD WINAPI hz3_hook_worker(LPVOID arg) {
    (void)arg;
    if (!g_hz3_cfg.enabled) {
        if (g_hz3_cfg.log_shot) {
            fprintf(stderr, "[HZ3_HOOK_IAT] enable=0 (skip)\n");
        }
        hz3_iat_signal_ready();
        return 0;
    }

    Hz3IatHook hooks[] = {
        { "malloc", (void*)hz3_hook_malloc, 1, 0 },
        { "free", (void*)hz3_hook_free, 1, 0 },
        { "calloc", (void*)hz3_hook_calloc, 0, 0 },
        { "realloc", (void*)hz3_hook_realloc, 0, 0 },
        { "posix_memalign", (void*)hz3_hook_posix_memalign, 0, 0 },
        { "aligned_alloc", (void*)hz3_hook_aligned_alloc, 0, 0 },
        { "malloc_usable_size", (void*)hz3_hook_malloc_usable_size, 0, 0 },
        { "_msize", (void*)hz3_hook_malloc_usable_size, 0, 0 }
    };

    HMODULE main_mod = GetModuleHandleA(NULL);
    int patched_main = hz3_iat_patch_module(main_mod, hooks, sizeof(hooks) / sizeof(hooks[0]));
    int missing_required = (hooks[0].hits == 0 || hooks[1].hits == 0);

    int other_modules = 0;
    int other_malloc = 0;
    int other_free = 0;
    int other_calloc = 0;
    int other_realloc = 0;
    if (g_hz3_cfg.all_modules) {
        other_modules = hz3_iat_patch_other_modules(main_mod, &other_malloc, &other_free,
                                                    &other_calloc, &other_realloc);
    }

    if (g_hz3_cfg.log_shot) {
        fprintf(stderr,
                "[HZ3_HOOK_IAT] enable=1 patched_main=%d all_modules=%d modules=%d "
                "malloc=%d free=%d calloc=%d realloc=%d\n",
                patched_main,
                g_hz3_cfg.all_modules,
                other_modules,
                hooks[0].hits + other_malloc,
                hooks[1].hits + other_free,
                hooks[2].hits + other_calloc,
                hooks[3].hits + other_realloc);
    }

    if (!patched_main && g_hz3_cfg.failfast) {
        hz3_hook_abort("iat_patch_failed");
    }
    if (missing_required && g_hz3_cfg.failfast) {
        hz3_hook_abort("iat_missing_required");
    }

    hz3_iat_signal_ready();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance;
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        g_hz3_cfg.self = instance;
        g_hz3_cfg.enabled = hz3_win_env_flag("HZ3_HOOK_ENABLE", 1);
        g_hz3_cfg.failfast = hz3_win_env_flag("HZ3_HOOK_FAILFAST", 1);
        g_hz3_cfg.log_shot = hz3_win_env_flag("HZ3_HOOK_LOG_SHOT", 0);
        g_hz3_cfg.all_modules = hz3_win_env_flag("HZ3_HOOK_IAT_ALL_MODULES", 0);
        DWORD n = GetEnvironmentVariableA("HZ3_HOOK_IAT_READY",
                                          g_hz3_cfg.ready_event,
                                          (DWORD)sizeof(g_hz3_cfg.ready_event));
        if (n == 0 || n >= sizeof(g_hz3_cfg.ready_event)) {
            g_hz3_cfg.ready_event[0] = '\0';
        }

        HANDLE thread = CreateThread(NULL, 0, hz3_hook_worker, NULL, 0, NULL);
        if (thread) {
            CloseHandle(thread);
        } else if (g_hz3_cfg.failfast) {
            hz3_hook_abort("iat_thread_create_failed");
        }
    }
    return TRUE;
}
