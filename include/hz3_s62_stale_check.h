// hz3_s62_stale_check.h - Stale free_bits fail-fast detection (S62-1 debug)
// Purpose: Abort immediately when accessing an object on a page marked as free.
#pragma once

#include "hz3_config.h"
#include "hz3_config_sub4k.h"

#if HZ3_S62_STALE_FAILFAST

// Check if ptr belongs to a page marked as free in segment's free_bits.
// Aborts immediately if free_bits[page_idx] == 1 (stale access detected).
// where: call site identifier (e.g., "small_alloc_local")
// sc_or_bin_hint: size class or bin hint for diagnostics
void hz3_s62_stale_check_ptr(void* ptr, const char* where, int sc_or_bin_hint);

#else

// No-op when disabled
static inline void hz3_s62_stale_check_ptr(void* ptr, const char* where, int sc_or_bin_hint) {
    (void)ptr;
    (void)where;
    (void)sc_or_bin_hint;
}

#endif
