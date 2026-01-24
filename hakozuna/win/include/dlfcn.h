#pragma once

#if !defined(_WIN32)
#error "dlfcn shim is Windows-only"
#endif

#include <stddef.h>

typedef struct {
    const char* dli_fname;
    void* dli_fbase;
} Dl_info;

#define RTLD_DEFAULT ((void*)0)
#define RTLD_NEXT ((void*)-1)

static inline void* dlsym(void* handle, const char* name) {
    (void)handle;
    (void)name;
    return NULL;
}

static inline int dladdr(const void* addr, Dl_info* info) {
    (void)addr;
    if (info) {
        info->dli_fname = NULL;
        info->dli_fbase = NULL;
    }
    return 0;
}
