#include "../src/hz10_pagemap.h"

#include <stdint.h>
#include <stdio.h>

#define BASE0 ((void*)(uintptr_t)(0x0000710000000000ULL))
#define BASE_STRIDE ((uintptr_t)HZ10_PAGE_QUANTUM)

typedef struct RouteCase {
  const char* label;
  void* base;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t flags;
  void* owner;
  uint32_t generation;
} RouteCase;

static int compare_route(const RouteCase* rc, const void* ptr,
                         uint32_t expected_generation, const char* label) {
  H10RouteResult slow = hz10_pagemap_route(ptr, expected_generation);
  if (expected_generation != HZ10_GENERATION_ANY) {
    return 0;
  }
  H10RouteLocalResult fast = {0};
  int fast_ok = hz10_pagemap_route_local_fast(ptr, &fast);
  int want_fast = slow.kind == H10_ROUTE_VALID && slow.flags == 0u &&
                  expected_generation == HZ10_GENERATION_ANY;

  if (fast_ok != want_fast) {
    fprintf(stderr,
            "%s/%s: fast_ok=%d want=%d slow_kind=%d reason=%d flags=%u\n",
            rc->label, label, fast_ok, want_fast, (int)slow.kind,
            (int)slow.reason, slow.flags);
    return 1;
  }
  if (fast_ok) {
    if (fast.owner != slow.owner || fast.slot_size != slow.slot_size ||
        fast.generation != slow.generation || fast.flags != slow.flags) {
      fprintf(stderr,
              "%s/%s: fast payload mismatch owner=%p/%p slot=%u/%u "
              "gen=%u/%u flags=%u/%u\n",
              rc->label, label, fast.owner, slow.owner, fast.slot_size,
              slow.slot_size, fast.generation, slow.generation, fast.flags,
              slow.flags);
      return 1;
    }
  }
  return 0;
}

static int check_case(const RouteCase* rc) {
  int failed = 0;
  uintptr_t base = (uintptr_t)rc->base;
  uint64_t span = (uint64_t)rc->slot_size * (uint64_t)rc->slot_count;

  failed |= compare_route(rc, rc->base, HZ10_GENERATION_ANY, "base-any");
  failed |= compare_route(rc, rc->base, rc->generation, "base-gen");

  if (rc->slot_count > 1u) {
    void* second = (void*)(base + rc->slot_size);
    failed |= compare_route(rc, second, HZ10_GENERATION_ANY, "slot1");
  }
  if (rc->slot_size >= 32u) {
    void* interior_aligned = (void*)(base + 16u);
    failed |= compare_route(rc, interior_aligned, HZ10_GENERATION_ANY,
                            "interior-aligned");
  }
  failed |= compare_route(rc, (void*)(base + 1u), HZ10_GENERATION_ANY,
                          "misaligned");
  failed |= compare_route(rc, (void*)(base + (uintptr_t)span),
                          HZ10_GENERATION_ANY, "span-boundary");
  if (span < (uint64_t)HZ10_PAGE_QUANTUM) {
    failed |= compare_route(rc, (void*)(base + (uintptr_t)span + 16u),
                            HZ10_GENERATION_ANY, "tail-slack");
  }
  failed |= compare_route(rc, (void*)(base - 16u), HZ10_GENERATION_ANY,
                          "before-base");
  if (rc->slot_count > 1u) {
    for (uint32_t offset = 0u; offset < (uint32_t)span; ++offset) {
      failed |= compare_route(rc, (void*)(base + offset), HZ10_GENERATION_ANY,
                              "exhaustive-span");
    }
  }
  return failed;
}

static int register_case(RouteCase* rc, uintptr_t index, uint32_t slot_size,
                         uint32_t slot_count, uint32_t flags) {
  rc->base = (void*)((uintptr_t)BASE0 + index * BASE_STRIDE);
  rc->slot_size = slot_size;
  rc->slot_count = slot_count;
  rc->flags = flags;
  rc->owner = (void*)((uintptr_t)0x1000u + index);
  rc->generation = hz10_pagemap_register_with_owner_and_flags(
      rc->base, slot_size, slot_count, rc->owner, flags);
  if (rc->generation == 0u) {
    fprintf(stderr, "%s: register failed size=%u count=%u flags=%u\n",
            rc->label, slot_size, slot_count, flags);
    return 1;
  }
  return 0;
}

int main(void) {
  hz10_pagemap_reset_for_tests();

  int failed = 0;
  RouteCase cases[] = {
      {"class16", NULL, 0, 0, 0, NULL, 0},
      {"class48", NULL, 0, 0, 0, NULL, 0},
      {"class64", NULL, 0, 0, 0, NULL, 0},
      {"class24576", NULL, 0, 0, 0, NULL, 0},
      {"class65536", NULL, 0, 0, 0, NULL, 0},
      {"odd80", NULL, 0, 0, 0, NULL, 0},
      {"odd1000", NULL, 0, 0, 0, NULL, 0},
      {"large-flagged", NULL, 0, 0, 0, NULL, 0},
  };
  failed |= register_case(&cases[0], 0u, 16u, 4096u, 0u);
  failed |= register_case(&cases[1], 1u, 48u, 1000u, 0u);
  failed |= register_case(&cases[2], 2u, 64u, 1024u, 0u);
  failed |= register_case(&cases[3], 3u, 24576u, 2u, 0u);
  failed |= register_case(&cases[4], 4u, 65536u, 1u, 0u);
  failed |= register_case(&cases[5], 5u, 80u, 13u, 0u);
  failed |= register_case(&cases[6], 6u, 1000u, 17u, 0u);
  failed |= register_case(&cases[7], 7u, HZ10_PAGE_QUANTUM * 3u, 1u, 1u);
  if (failed) {
    return 1;
  }

  for (uint32_t i = 0u; i < (uint32_t)(sizeof(cases) / sizeof(cases[0]));
       ++i) {
    failed |= check_case(&cases[i]);
  }

  RouteCase stale = {"stale-reregister", NULL, 0, 0, 0, NULL, 0};
  failed |= register_case(&stale, 8u, 64u, 16u, 0u);
  uint32_t stale_generation = stale.generation;
  if (!hz10_pagemap_release(stale.base)) {
    fprintf(stderr, "stale-reregister: release failed\n");
    failed = 1;
  }
  stale.generation = hz10_pagemap_register_with_owner_and_flags(
      stale.base, 64u, 16u, stale.owner, 0u);
  if (stale.generation == 0u || stale.generation == stale_generation) {
    fprintf(stderr, "stale-reregister: generation did not bump\n");
    failed = 1;
  }
  failed |= compare_route(&stale, stale.base, stale_generation, "stale-gen");
  failed |= compare_route(&stale, stale.base, HZ10_GENERATION_ANY,
                          "fresh-any");

  RouteCase miss = {"miss", NULL, 0, 0, 0, NULL, 0};
  failed |= compare_route(&miss, NULL, HZ10_GENERATION_ANY, "null");
  failed |= compare_route(&miss,
                          (void*)((uintptr_t)BASE0 + 1000u * BASE_STRIDE),
                          HZ10_GENERATION_ANY, "root-or-leaf-absent");
  failed |= compare_route(&cases[7],
                          (void*)((uintptr_t)cases[7].base +
                                  HZ10_PAGE_QUANTUM),
                          HZ10_GENERATION_ANY, "large-second-quantum");

  if (failed) {
    return 2;
  }
  puts("hz10_pagemap_route_diff_smoke ok");
  return 0;
}
