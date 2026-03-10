#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <windows.h>
#endif

#include "hz4_win_api.h"

static void usage(const char* argv0) {
    fprintf(stderr, "usage: %s [size] [--write] [--free] [--thread] [--iters N]\n", argv0);
}

typedef struct smoke_cfg {
    size_t size;
    int do_write;
    int do_free;
    size_t iters;
} smoke_cfg_t;

static int run_once(const smoke_cfg_t* cfg) {
    void* p = hz4_win_malloc(cfg->size);
    printf("hz4_win_malloc(%zu) -> %p\n", cfg->size, p);
    fflush(stdout);

    if (!p) {
        fprintf(stderr, "allocation returned NULL\n");
        return 1;
    }

    if (cfg->do_write) {
        memset(p, 0xA5, cfg->size);
        printf("memset(%p, 0xA5, %zu) ok\n", p, cfg->size);
        fflush(stdout);
    }

    if (cfg->do_free) {
        hz4_win_free(p);
        printf("hz4_win_free(%p) ok\n", p);
        fflush(stdout);
    }

    return 0;
}

#if defined(_WIN32)
static unsigned __stdcall smoke_thread(void* arg) {
    smoke_cfg_t* cfg = (smoke_cfg_t*)arg;
    for (size_t i = 0; i < cfg->iters; ++i) {
        int rc = run_once(cfg);
        if (rc != 0) {
            return (unsigned)rc;
        }
    }
    return 0;
}
#endif

int main(int argc, char** argv) {
    smoke_cfg_t cfg;
    cfg.size = 16;
    cfg.do_write = 0;
    cfg.do_free = 0;
    cfg.iters = 1;
    int do_thread = 0;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (strcmp(arg, "--write") == 0) {
            cfg.do_write = 1;
            continue;
        }
        if (strcmp(arg, "--free") == 0) {
            cfg.do_free = 1;
            continue;
        }
        if (strcmp(arg, "--thread") == 0) {
            do_thread = 1;
            continue;
        }
        if (strcmp(arg, "--iters") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            cfg.iters = (size_t)strtoull(argv[++i], NULL, 10);
            continue;
        }

        char* end = NULL;
        unsigned long long parsed = strtoull(arg, &end, 10);
        if (end && *end == '\0') {
            cfg.size = (size_t)parsed;
            continue;
        }

        usage(argv[0]);
        return 2;
    }

#if defined(_WIN32)
    if (do_thread) {
        uintptr_t h = _beginthreadex(NULL, 0, smoke_thread, &cfg, 0, NULL);
        if (!h) {
            fprintf(stderr, "_beginthreadex failed\n");
            return 3;
        }
        HANDLE handle = (HANDLE)h;
        WaitForSingleObject(handle, INFINITE);
        DWORD code = 0;
        GetExitCodeThread(handle, &code);
        CloseHandle(handle);
        return (int)code;
    }
#endif

    for (size_t i = 0; i < cfg.iters; ++i) {
        int rc = run_once(&cfg);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}
