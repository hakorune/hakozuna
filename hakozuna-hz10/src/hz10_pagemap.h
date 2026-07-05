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
 * L0 scope: multi-slot registrations must still fit in one quantum
 * (slot_size * slot_count <= HZ10_PAGE_QUANTUM); a single-slot
 * (slot_count == 1) registration may span more quanta than that -- see
 * hz10_pagemap_register_with_owner_and_flags() and src/hz10_large_alloc.h.
 * No allocator malloc/free behavior lives here.
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
  void* owner; /* opaque, whatever hz10_pagemap_register_with_owner() was
                * given; NULL if registered via plain hz10_pagemap_register()
                * or if the route missed/was invalid. */
  uint32_t flags; /* opaque, whatever hz10_pagemap_register_with_owner_and_flags()
                   * was given; 0 if registered via any other register call, or
                   * if the route missed/was invalid. Box 1 never interprets
                   * this -- same treatment as owner, just a second slot for a
                   * caller that wants to distinguish "kinds" of registration
                   * (e.g. a small-class page vs. a large direct allocation,
                   * see src/hz10_large_alloc.h) without a second lookup. */
} H10RouteResult;

/*
 * Lean route result for the public-entry local/free fast path. It is
 * intentionally narrower than H10RouteResult: no reason/page_base/slot_index.
 * HZ10EntryTrim-L0 first wires tests and benches to this name while the
 * implementation delegates to the slow route; E1 replaces the body with the
 * header-inline reader. Contract: returns 1 only for small/slotted VALID
 * registrations (flags == 0) with HZ10_GENERATION_ANY semantics. Large or
 * otherwise flagged routes return 0 and must use the slow .c route.
 */
typedef struct H10RouteLocalResult {
  void* owner;
  uint32_t slot_size;
  uint32_t generation;
  uint32_t flags;
} H10RouteLocalResult;

/*
 * Registers `base` (must be HZ10_PAGE_QUANTUM-aligned) as a page holding
 * slot_count slots of slot_size bytes each. If slot_count > 1, slot_size *
 * slot_count must fit in one HZ10_PAGE_QUANTUM (every slot has to be
 * addressable through this one registration's single leaf entry). A
 * single-slot registration (slot_count == 1) has no such limit and may
 * span any number of quanta -- see src/hz10_large_alloc.h, the first
 * caller that needs a span bigger than one quantum. Returns the new
 * generation (>= 1) on success, or 0 on failure (misaligned base, zero
 * size/count, overflow, or OOM committing the backing leaf). Registering
 * an already-registered base bumps and returns a new generation for the
 * same slot.
 */
uint32_t hz10_pagemap_register(void* base, uint32_t slot_size,
                               uint32_t slot_count);

/*
 * Same as hz10_pagemap_register(), but also stores an opaque owner tag
 * (e.g. a higher-level module's own page handle) that hz10_pagemap_route()
 * later returns verbatim in H10RouteResult.owner. Box 1 never dereferences
 * or interprets owner -- it is pure storage for whoever registered it, so
 * a caller that resolved an unknown pointer through route() can recover
 * "which of my own structures does this belong to" without a second
 * lookup. hz10_pagemap_register() is a thin wrapper for owner == NULL,
 * kept so Box 1-4 callers that don't care about ownership need no change.
 */
uint32_t hz10_pagemap_register_with_owner(void* base, uint32_t slot_size,
                                          uint32_t slot_count, void* owner);

/*
 * Same as hz10_pagemap_register_with_owner(), but also stores an opaque
 * flags word (e.g. a "kind of registration" tag) that hz10_pagemap_route()
 * later returns verbatim in H10RouteResult.flags. Box 1 never interprets
 * flags, same rule as owner. hz10_pagemap_register_with_owner() is a thin
 * wrapper for flags == 0, kept so existing callers need no change.
 */
uint32_t hz10_pagemap_register_with_owner_and_flags(void* base,
                                                    uint32_t slot_size,
                                                    uint32_t slot_count,
                                                    void* owner,
                                                    uint32_t flags);

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

static inline int hz10_pagemap_route_local_fast(
    const void* ptr, H10RouteLocalResult* out) {
  H10RouteResult route = hz10_pagemap_route(ptr, HZ10_GENERATION_ANY);
  if (route.kind != H10_ROUTE_VALID || route.flags != 0u) {
    return 0;
  }
  if (out) {
    out->owner = route.owner;
    out->slot_size = route.slot_size;
    out->generation = route.generation;
    out->flags = route.flags;
  }
  return 1;
}

/* Test/bench only: unregisters everything and unmaps all leaves. */
void hz10_pagemap_reset_for_tests(void);

#endif
