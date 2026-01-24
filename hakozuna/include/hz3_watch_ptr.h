#pragma once

#include "hz3_config.h"

#if HZ3_WATCH_PTR_BOX
void hz3_watch_ptr_init_once(void);
void hz3_watch_ptr_on_alloc(const char* where, void* ptr, int sc, int owner);
void hz3_watch_ptr_on_free(const char* where, void* ptr, int sc, int owner);
#else
static inline void hz3_watch_ptr_init_once(void) {}
static inline void hz3_watch_ptr_on_alloc(const char* where, void* ptr, int sc, int owner) {
    (void)where;
    (void)ptr;
    (void)sc;
    (void)owner;
}
static inline void hz3_watch_ptr_on_free(const char* where, void* ptr, int sc, int owner) {
    (void)where;
    (void)ptr;
    (void)sc;
    (void)owner;
}
#endif
