#pragma once
#ifndef HZ3_DTOR_STATS_H
#define HZ3_DTOR_STATS_H

// ============================================================================
// Common Stats Infrastructure for Dtor/Cold-Path Modules
// ============================================================================
//
// Provides shared patterns for:
// - Atomic stats declaration
// - atexit registration (once pattern)
// - Common dump function signature
//
// Used by: S61 (DtorHardPurge), S62 (Observe), etc.

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Stats Declaration Macros
// ============================================================================

// Declare a single atomic stat counter
#define HZ3_DTOR_STAT(name) static _Atomic(uint32_t) name

// Load stat value (relaxed ordering, cold path only)
#define HZ3_DTOR_STAT_LOAD(name) \
    atomic_load_explicit(&(name), memory_order_relaxed)

// Increment stat by 1 (relaxed ordering)
#define HZ3_DTOR_STAT_INC(name) \
    atomic_fetch_add_explicit(&(name), 1, memory_order_relaxed)

// Add value to stat (relaxed ordering)
#define HZ3_DTOR_STAT_ADD(name, val) \
    atomic_fetch_add_explicit(&(name), (val), memory_order_relaxed)

// ============================================================================
// atexit Registration Helper (once pattern)
// ============================================================================

// Declare atexit registered flag
#define HZ3_DTOR_ATEXIT_FLAG(prefix) \
    static int prefix##_atexit_registered = 0

// Register atexit handler once (cold path, use in main function)
// Uses __builtin_expect for branch prediction (unlikely on first call)
#define HZ3_DTOR_ATEXIT_REGISTER_ONCE(prefix, dump_func) \
    do { \
        if (__builtin_expect(!prefix##_atexit_registered, 0)) { \
            prefix##_atexit_registered = 1; \
            atexit(dump_func); \
        } \
    } while (0)

// ============================================================================
// Combined Declaration/Registration Helpers
// ============================================================================

// Declare stats group header (for documentation/grouping)
#define HZ3_DTOR_STATS_BEGIN(module_name) \
    /* Stats for module_name */

// Example usage pattern:
//
// HZ3_DTOR_STATS_BEGIN(S61);
// HZ3_DTOR_STAT(g_s61_dtor_calls);
// HZ3_DTOR_STAT(g_s61_madvise_ok);
// HZ3_DTOR_ATEXIT_FLAG(g_s61);
//
// static void hz3_s61_atexit_dump(void) {
//     uint32_t dtor_calls = HZ3_DTOR_STAT_LOAD(g_s61_dtor_calls);
//     ...
// }
//
// void hz3_s61_main(void) {
//     HZ3_DTOR_STAT_INC(g_s61_dtor_calls);
//     HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s61, hz3_s61_atexit_dump);
//     ...
// }

#endif // HZ3_DTOR_STATS_H
