#pragma once

#if !defined(_WIN32)
#error "execinfo shim is Windows-only"
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int backtrace(void **buffer, int size) {
    (void)buffer;
    (void)size;
    return 0;
}

static inline char **backtrace_symbols(void *const *buffer, int size) {
    (void)buffer;
    (void)size;
    return (char **)0;
}

static inline void backtrace_symbols_fd(void *const *buffer, int size, int fd) {
    (void)buffer;
    (void)size;
    (void)fd;
}

#ifdef __cplusplus
}
#endif
