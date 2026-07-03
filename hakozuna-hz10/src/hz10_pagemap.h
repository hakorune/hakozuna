#ifndef HZ10_PAGEMAP_H
#define HZ10_PAGEMAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * HZ10PageMapRoute-L0: a 2-level radix address->page-metadata route.
 *
 * One quantum (HZ10_PAGE_QUANTUM bytes) is the registration granule. The
 * route function never dereferences the pointer it classifies; it only
 * does address arithmetic against metadata this module owns, so callers
 * may register/query synthetic addresses that were never mmap'd.
 *
 * L0 scope: single-quantum registration only (slot_size * slot_count must
 * fit in one quantum). No allocator malloc/free behavior lives here.
 */

#define HZ10_PAGE_SHIFT 16u
#define HZ10_PAGE_QUANTUM (1u << HZ10_PAGE_SHIFT)
#define HZ10_MIN_ALIGN 16u
#define HZ10_GENERATION_ANY 0u

#define HZ10_ROOT_BITS 11u
#define HZ10_LEAF_BITS 20u

typedef enum H10RouteKind {
  H10_ROUTE_MISS = 0,
  H10_ROUTE_VALID = 1,
  H10_ROUTE_INVALID = 2
} H10RouteKind;

typedef enum H10RouteReason {
  H10_REASON_NONE = 0,
  H10_REASON_ROOT_ABSENT = 1,
  H10_REASON_LEAF_ABSENT = 2,
  H10_REASON_TAIL_SLACK = 3,
  H10_REASON_MISALIGNED = 4,
  H10_REASON_INTERIOR = 5,
  H10_REASON_GENERATION_STALE = 6
} H10RouteReason;

typedef struct H10RouteResult {
  H10RouteKind kind;
  H10RouteReason reason;
  void* page_base;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t slot_index;
  uint32_t generation;
} H10RouteResult;

/*
 * Registers `base` (must be HZ10_PAGE_QUANTUM-aligned) as a page holding
 * slot_count slots of slot_size bytes each (slot_size * slot_count must be
 * <= HZ10_PAGE_QUANTUM). Returns the new generation (>= 1) on success, or 0
 * on failure (misaligned base, zero size/count, overflow, or OOM committing
 * the backing leaf). Registering an already-registered base bumps and
 * returns a new generation for the same slot.
 */
uint32_t hz10_pagemap_register(void* base, uint32_t slot_size,
                               uint32_t slot_count);

/*
 * Marks `base` as no longer holding a live page. The generation counter is
 * preserved (not reset) so a later register() at the same address can bump
 * it further; a stale route() against the pre-release generation must keep
 * failing even before any re-registration happens.
 * Returns 1 if a live registration was cleared, 0 if base was not
 * registered (or was already released).
 */
int hz10_pagemap_release(void* base);

/*
 * Classifies `ptr` against whatever is currently registered.
 * expected_generation == HZ10_GENERATION_ANY skips the generation check.
 */
H10RouteResult hz10_pagemap_route(const void* ptr, uint32_t expected_generation);

/* Test/bench only: unregisters everything and unmaps all leaves. */
void hz10_pagemap_reset_for_tests(void);

#endif
