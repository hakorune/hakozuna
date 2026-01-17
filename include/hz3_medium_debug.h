#pragma once

#include "hz3_config.h"

// S72-M: Medium boundary debug (PTAG-based, no deref)
#if HZ3_S72_MEDIUM_DEBUG
void hz3_medium_boundary_check_ptr(const char* where, void* ptr, int sc, int owner);
#else
static inline void hz3_medium_boundary_check_ptr(const char* where, void* ptr, int sc, int owner) {
    (void)where;
    (void)ptr;
    (void)sc;
    (void)owner;
}
#endif
