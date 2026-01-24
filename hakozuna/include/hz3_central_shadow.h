#pragma once

// S86: CentralShadowNextBox - debug-only shadow map for central bin verification
//
// Purpose:
// - Detect when freelist next pointers get corrupted after push to central
// - Detect duplicate push (same object pushed twice)
//
// Operation:
// - push: record (ptr, next) in shadow map; fail-fast if entry exists (dup)
// - pop: compare actual next with shadow; remove entry after check
// - mismatch: one-shot log and abort
// - map full: one-shot log and disable (silent continue)
//
// Performance:
// - Debug only (HZ3_S86_CENTRAL_SHADOW=1 to enable)
// - Uses global mutex + open-address hash (slow is OK)

#include "hz3_config.h"
#include <stdint.h>

#ifndef HZ3_S86_CENTRAL_SHADOW
#define HZ3_S86_CENTRAL_SHADOW 0
#endif

// S86 option: instrument medium central bins (hz3_central.c).
// Default OFF because S86 can be heavy; enable explicitly when debugging medium.
#ifndef HZ3_S86_CENTRAL_SHADOW_MEDIUM
#define HZ3_S86_CENTRAL_SHADOW_MEDIUM 0
#endif

// S86 option: verify next-pointer integrity on pop (in addition to detecting dup push).
// Can be disabled to reduce overhead while hunting push_dup.
#ifndef HZ3_S86_CENTRAL_SHADOW_VERIFY_POP
#define HZ3_S86_CENTRAL_SHADOW_VERIFY_POP 1
#endif

// S86 option: when a shard table is full, evict an existing entry instead of
// disabling the shadow map. This trades completeness for long-run usability.
#ifndef HZ3_S86_CENTRAL_SHADOW_EVICT_ON_FULL
#define HZ3_S86_CENTRAL_SHADOW_EVICT_ON_FULL 0
#endif

#if HZ3_S86_CENTRAL_SHADOW

// Record (ptr, next, ra) in shadow map.
// ra = return address from the boundary (caller of push_list).
// Returns 0 on success, -1 if entry already exists (duplicate push).
int hz3_central_shadow_record(void* ptr, void* next, void* ra);

// Verify ptr's next matches shadow, then remove entry.
// Returns 0 on success, -1 on mismatch (logs and aborts internally).
int hz3_central_shadow_verify_and_remove(void* ptr, void* actual_next);

// Remove ptr from shadow without verifying next (lightweight mode).
int hz3_central_shadow_remove(void* ptr);

// Check if shadow map is enabled (may be disabled on overflow).
int hz3_central_shadow_is_enabled(void);

#else

// Stubs when disabled
static inline int hz3_central_shadow_record(void* ptr, void* next, void* ra) {
    (void)ptr; (void)next; (void)ra;
    return 0;
}

static inline int hz3_central_shadow_verify_and_remove(void* ptr, void* actual_next) {
    (void)ptr; (void)actual_next;
    return 0;
}

static inline int hz3_central_shadow_remove(void* ptr) {
    (void)ptr;
    return 0;
}

static inline int hz3_central_shadow_is_enabled(void) {
    return 0;
}

#endif  // HZ3_S86_CENTRAL_SHADOW
