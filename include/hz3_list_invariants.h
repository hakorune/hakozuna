// hz3_list_invariants.h - Linked list invariants fail-fast (debug)
// Purpose: Detect corrupted lists before they propagate to S42/S44/central.
#pragma once

#include "hz3_config.h"
#include <stdint.h>

// Default: OFF (enable via HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_LIST_FAILFAST=1')
#ifndef HZ3_LIST_FAILFAST
#define HZ3_LIST_FAILFAST 0
#endif

#ifndef HZ3_LIST_FAILFAST_MAX_WALK
#define HZ3_LIST_FAILFAST_MAX_WALK 4096
#endif

#if HZ3_LIST_FAILFAST

// Validate linked list invariants. Aborts on first violation.
// where: call site identifier (e.g., "remote_list:entry")
// owner: destination shard
// sc: size class
// head: list head
// tail: list tail (must be reachable from head in exactly n steps)
// n: expected node count
void hz3_list_failfast(const char* where, uint8_t owner, int sc,
                       void* head, void* tail, uint32_t n);

#else

// No-op when disabled
static inline void hz3_list_failfast(const char* where, uint8_t owner, int sc,
                                     void* head, void* tail, uint32_t n) {
    (void)where; (void)owner; (void)sc; (void)head; (void)tail; (void)n;
}

#endif
