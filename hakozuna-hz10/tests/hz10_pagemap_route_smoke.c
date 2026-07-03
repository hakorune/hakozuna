#include "../src/hz10_pagemap.h"

#include <stdint.h>
#include <stdio.h>

/* Distinct, well-separated synthetic quantum-aligned addresses. route()
 * never dereferences its argument, so these never need to be mmap'd. */
#define HZ10_SMOKE_BASE1 ((void*)(uintptr_t)(0x0000700000000000ULL))
#define HZ10_SMOKE_BASE2 \
  ((void*)(uintptr_t)(0x0000700000000000ULL + HZ10_PAGE_QUANTUM))

static int expect(H10RouteKind kind, H10RouteReason reason,
                  H10RouteResult got, const char* label) {
  if (got.kind != kind || got.reason != reason) {
    fprintf(stderr, "%s: kind=%d(want %d) reason=%d(want %d)\n", label,
            (int)got.kind, (int)kind, (int)got.reason, (int)reason);
    return 1;
  }
  return 0;
}

static int expect_slot(uint32_t slot_index, H10RouteResult got,
                       const char* label) {
  if (got.slot_index != slot_index) {
    fprintf(stderr, "%s: slot_index=%u(want %u)\n", label, got.slot_index,
            slot_index);
    return 1;
  }
  return 0;
}

/* Case 1: exact pointer valid. */
static int check_exact_valid(uint32_t gen) {
  int failed = 0;
  H10RouteResult base_route = hz10_pagemap_route(HZ10_SMOKE_BASE1, gen);
  failed |= expect(H10_ROUTE_VALID, H10_REASON_NONE, base_route, "exact/base");
  failed |= expect_slot(0u, base_route, "exact/base");

  void* third_slot = (char*)HZ10_SMOKE_BASE1 + 3u * 64u;
  H10RouteResult third_route = hz10_pagemap_route(third_slot, gen);
  failed |= expect(H10_ROUTE_VALID, H10_REASON_NONE, third_route, "exact/+3");
  failed |= expect_slot(3u, third_route, "exact/+3");
  return failed;
}

/* Case 2: interior pointer invalid (16-aligned, not a multiple of
 * slot_size=64). */
static int check_interior_invalid(uint32_t gen) {
  void* interior = (char*)HZ10_SMOKE_BASE1 + 16u;
  H10RouteResult route = hz10_pagemap_route(interior, gen);
  return expect(H10_ROUTE_INVALID, H10_REASON_INTERIOR, route, "interior");
}

/* Case 3: misaligned pointer invalid (not a multiple of HZ10_MIN_ALIGN=16).
 */
static int check_misaligned_invalid(uint32_t gen) {
  void* misaligned = (char*)HZ10_SMOKE_BASE1 + 8u;
  H10RouteResult route = hz10_pagemap_route(misaligned, gen);
  return expect(H10_ROUTE_INVALID, H10_REASON_MISALIGNED, route, "misaligned");
}

/* Case 4: generation mismatch invalid. Release + re-register the same base
 * so the pre-release generation becomes stale, while the new generation and
 * HZ10_GENERATION_ANY still resolve to VALID. */
static int check_generation_mismatch_invalid(uint32_t stale_gen) {
  int failed = 0;
  if (!hz10_pagemap_release(HZ10_SMOKE_BASE1)) {
    fprintf(stderr, "generation: release failed\n");
    return 1;
  }
  uint32_t fresh_gen = hz10_pagemap_register(HZ10_SMOKE_BASE1, 64u, 1024u);
  if (fresh_gen == 0u || fresh_gen == stale_gen) {
    fprintf(stderr, "generation: re-register did not bump generation\n");
    return 1;
  }

  H10RouteResult stale_route = hz10_pagemap_route(HZ10_SMOKE_BASE1, stale_gen);
  failed |= expect(H10_ROUTE_INVALID, H10_REASON_GENERATION_STALE, stale_route,
                   "generation/stale");

  H10RouteResult fresh_route = hz10_pagemap_route(HZ10_SMOKE_BASE1, fresh_gen);
  failed |= expect(H10_ROUTE_VALID, H10_REASON_NONE, fresh_route,
                   "generation/fresh");

  H10RouteResult any_route =
      hz10_pagemap_route(HZ10_SMOKE_BASE1, HZ10_GENERATION_ANY);
  failed |= expect(H10_ROUTE_VALID, H10_REASON_NONE, any_route,
                   "generation/any");
  return failed;
}

/* Case 5: unknown pointer miss. A genuine stack address, never registered.
 */
static int check_unknown_pointer_miss(void) {
  int sentinel = 0;
  void* miss_ptr = &sentinel;
  H10RouteResult route = hz10_pagemap_route(miss_ptr, HZ10_GENERATION_ANY);
  return expect(H10_ROUTE_MISS, route.reason, route, "unknown");
}

/* Bonus: tail slack invalid, beyond the registered span but still inside
 * the quantum. Falls out of the pipeline for free; not one of the 5
 * required cases but worth one extra assertion. */
static int check_tail_slack_invalid(void) {
  uint32_t gen = hz10_pagemap_register(HZ10_SMOKE_BASE2, 48u, 1000u);
  if (gen == 0u) {
    fprintf(stderr, "tail_slack: register failed\n");
    return 1;
  }
  void* tail = (char*)HZ10_SMOKE_BASE2 + 48u * 1000u;
  H10RouteResult route = hz10_pagemap_route(tail, gen);
  return expect(H10_ROUTE_INVALID, H10_REASON_TAIL_SLACK, route, "tail_slack");
}

/* Bonus: a single-slot (slot_count == 1) registration may span more than
 * one HZ10_PAGE_QUANTUM -- the relaxation src/hz10_large_alloc.h needs.
 * A multi-slot (slot_count > 1) registration must still be rejected if its
 * span exceeds one quantum (the relaxation must not silently widen to
 * cases where it would break addressing of the later slots). Also checks
 * that flags round-trips through route() alongside owner, unaffected by
 * either fact above. */
static int check_multi_quantum_single_slot(void) {
  void* base3 = (char*)HZ10_SMOKE_BASE1 + 4u * (uintptr_t)HZ10_PAGE_QUANTUM;
  uint32_t big_size = (uint32_t)HZ10_PAGE_QUANTUM * 3u;
  int sentinel_owner;
  uint32_t gen = hz10_pagemap_register_with_owner_and_flags(
      base3, big_size, 1u, &sentinel_owner, 7u);
  if (gen == 0u) {
    fprintf(stderr,
            "multi_quantum: single-slot registration spanning 3 quanta "
            "unexpectedly failed\n");
    return 1;
  }

  H10RouteResult exact = hz10_pagemap_route(base3, gen);
  int failed = expect(H10_ROUTE_VALID, H10_REASON_NONE, exact, "multi_q/exact");
  if (exact.owner != &sentinel_owner || exact.flags != 7u) {
    fprintf(stderr, "multi_quantum: owner/flags did not round-trip\n");
    failed = 1;
  }

  /* An address in the SECOND quantum of the span was never itself
   * registered (only base3's own quantum was) -- it must MISS, not
   * crash and not spuriously validate. This is the documented reason a
   * large allocation only ever hands out its own base pointer. */
  void* second_quantum = (char*)base3 + HZ10_PAGE_QUANTUM;
  H10RouteResult mid = hz10_pagemap_route(second_quantum, gen);
  if (mid.kind != H10_ROUTE_MISS) {
    fprintf(stderr,
            "multi_quantum: address in a later quantum of the span "
            "should MISS (kind=%d)\n",
            (int)mid.kind);
    failed = 1;
  }

  /* One byte past the whole span lands in yet another unregistered
   * quantum too -- MISS, not TAIL_SLACK. TAIL_SLACK only fires for an
   * offset that is still within the registered quantum itself (see
   * check_tail_slack_invalid above, where slot_size*slot_count is
   * smaller than one quantum); this registration's span is an exact
   * multiple of HZ10_PAGE_QUANTUM by construction (see
   * src/hz10_large_alloc.h's rounding), so there is no such byte here. */
  void* tail = (char*)base3 + big_size;
  H10RouteResult past_end = hz10_pagemap_route(tail, gen);
  if (past_end.kind != H10_ROUTE_MISS) {
    fprintf(stderr,
            "multi_quantum: one byte past the whole span should MISS "
            "(kind=%d reason=%d)\n",
            (int)past_end.kind, (int)past_end.reason);
    failed = 1;
  }

  /* A multi-slot registration must still be rejected if its span exceeds
   * one quantum -- the relaxation is single-slot only. */
  void* base4 = (char*)HZ10_SMOKE_BASE1 + 8u * (uintptr_t)HZ10_PAGE_QUANTUM;
  uint32_t rejected_gen = hz10_pagemap_register(base4, 4096u, 20u);
  if (rejected_gen != 0u) {
    fprintf(stderr,
            "multi_quantum: multi-slot registration spanning more than "
            "one quantum should still be rejected\n");
    failed = 1;
  }
  return failed;
}

int main(void) {
  hz10_pagemap_reset_for_tests();

  uint32_t gen = hz10_pagemap_register(HZ10_SMOKE_BASE1, 64u, 1024u);
  if (gen == 0u) {
    fprintf(stderr, "setup: register(base1) failed\n");
    return 1;
  }

  if (check_exact_valid(gen)) {
    return 2;
  }
  if (check_interior_invalid(gen)) {
    return 3;
  }
  if (check_misaligned_invalid(gen)) {
    return 4;
  }
  if (check_generation_mismatch_invalid(gen)) {
    return 5;
  }
  if (check_unknown_pointer_miss()) {
    return 6;
  }
  if (check_tail_slack_invalid()) {
    return 7;
  }
  if (check_multi_quantum_single_slot()) {
    return 8;
  }

  puts("hz10_pagemap_route_smoke ok");
  return 0;
}
