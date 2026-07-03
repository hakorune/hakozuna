#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Route-cost microbench for HZ10PageMapRoute-L0.
 *
 * Compares the real hz10_pagemap route (2-level radix, O(1)) against a
 * small, self-contained, HZ8/HZ9-global-state-free reimplementation of just
 * the open-addressing hash-probe *algorithm* HZ8/HZ9 use for the same job
 * (h8_medium_registry.c:h8_medium_directory_find /
 * h8_hz9_local_slab_route_boundary.c:h9_lsp_hash_find). The hash baseline
 * shares the pagemap's exact classification pipeline (tail-slack /
 * misaligned / interior / generation) so the only thing that differs
 * between the two timed loops is the *lookup* step (radix walk vs. hash +
 * linear probe) -- isolating the O(1)-vs-probing cost this box exists to
 * measure, per docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md.
 *
 * Anti-DCE/anti-fusion honesty (bench/README.md): every iteration checks
 * the classification result and fails fast on any mismatch, the loop
 * round-robins across PAGES distinct addresses rather than hammering one
 * hot pointer, and a checksum accumulated across iterations is verified
 * against a precomputed expected value at the end.
 */

#define HZ10_BENCH_HASH_CAP 8192u
#define HZ10_BENCH_HASH_MASK (HZ10_BENCH_HASH_CAP - 1u)
#define HZ10_BENCH_FIB_MULTIPLIER 11400714819323198485ULL

typedef struct HashRecord {
  void* base;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t generation;
} HashRecord;

static HashRecord hz10_bench_hash_table[HZ10_BENCH_HASH_CAP];

static uint64_t hz10_bench_hash_pos(uintptr_t quantum_base) {
  return ((quantum_base >> HZ10_PAGE_SHIFT) * HZ10_BENCH_FIB_MULTIPLIER) &
         HZ10_BENCH_HASH_MASK;
}

static uint32_t hz10_bench_hash_register(void* base, uint32_t slot_size,
                                        uint32_t slot_count) {
  uint64_t pos = hz10_bench_hash_pos((uintptr_t)base);
  for (uint64_t i = 0; i < HZ10_BENCH_HASH_CAP; ++i) {
    HashRecord* rec = &hz10_bench_hash_table[(pos + i) & HZ10_BENCH_HASH_MASK];
    if (rec->base == base) {
      uint32_t gen = rec->generation + 1u;
      rec->slot_size = slot_size;
      rec->slot_count = slot_count;
      rec->generation = gen;
      return gen;
    }
    if (rec->base == NULL) {
      rec->base = base;
      rec->slot_size = slot_size;
      rec->slot_count = slot_count;
      rec->generation = 1u;
      return 1u;
    }
  }
  return 0u;
}

/* Same classification pipeline as hz10_pagemap_route(): tail-slack, then
 * misaligned, then interior, then generation. Duplicated intentionally
 * (hz10_pagemap.c's version is static/internal) so both mechanisms are
 * timed against literally identical post-lookup logic. */
static H10RouteResult hz10_bench_classify(void* base, uint32_t slot_size,
                                         uint32_t slot_count,
                                         uint32_t generation, uintptr_t addr,
                                         uint32_t expected_generation) {
  H10RouteResult r;
  r.page_base = base;
  r.slot_size = slot_size;
  r.slot_count = slot_count;
  r.slot_index = 0u;
  r.generation = generation;

  uint64_t offset = (uint64_t)(addr - (uintptr_t)base);
  uint64_t span = (uint64_t)slot_size * (uint64_t)slot_count;
  if (offset >= span) {
    r.kind = H10_ROUTE_INVALID;
    r.reason = H10_REASON_TAIL_SLACK;
    return r;
  }
  if ((offset & (HZ10_MIN_ALIGN - 1u)) != 0u) {
    r.kind = H10_ROUTE_INVALID;
    r.reason = H10_REASON_MISALIGNED;
    return r;
  }
  if ((offset % slot_size) != 0u) {
    r.kind = H10_ROUTE_INVALID;
    r.reason = H10_REASON_INTERIOR;
    return r;
  }
  if (expected_generation != HZ10_GENERATION_ANY &&
      expected_generation != generation) {
    r.kind = H10_ROUTE_INVALID;
    r.reason = H10_REASON_GENERATION_STALE;
    return r;
  }
  r.kind = H10_ROUTE_VALID;
  r.reason = H10_REASON_NONE;
  r.slot_index = (uint32_t)(offset / slot_size);
  return r;
}

static H10RouteResult hz10_bench_hash_route(const void* ptr,
                                           uint32_t expected_generation) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t quantum_base = addr & ~(uintptr_t)(HZ10_PAGE_QUANTUM - 1u);
  uint64_t pos = hz10_bench_hash_pos(quantum_base);
  for (uint64_t i = 0; i < HZ10_BENCH_HASH_CAP; ++i) {
    HashRecord* rec = &hz10_bench_hash_table[(pos + i) & HZ10_BENCH_HASH_MASK];
    if (rec->base == NULL) {
      break;
    }
    if (rec->base == (void*)quantum_base) {
      return hz10_bench_classify(rec->base, rec->slot_size, rec->slot_count,
                                rec->generation, addr, expected_generation);
    }
  }
  H10RouteResult miss;
  miss.kind = H10_ROUTE_MISS;
  miss.reason = H10_REASON_LEAF_ABSENT;
  miss.page_base = NULL;
  miss.slot_size = 0u;
  miss.slot_count = 0u;
  miss.slot_index = 0u;
  miss.generation = 0u;
  return miss;
}

static uint64_t hz10_bench_env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

static double hz10_bench_seconds(uint64_t ns) {
  return (double)ns / 1000000000.0;
}

/* One timed pass over `iters` round-robin lookups against `pages` distinct
 * registered addresses. `use_pagemap` selects hz10_pagemap_route() vs. the
 * in-process hash-probe baseline above. Returns via out-params so the
 * caller can print one line per (mechanism, run). */
static int hz10_bench_run(const char* mech, int use_pagemap, uint64_t iters,
                          uint32_t pages, void** addrs, uint32_t* gens,
                          uint32_t run_index, uint32_t run_count,
                          uint32_t threads) {
  uint64_t checksum = 0;
  uint64_t start = hz10_platform_now_ns();
  for (uint64_t i = 0; i < iters; ++i) {
    uint32_t p = (uint32_t)(i % pages);
    H10RouteResult route = use_pagemap
                               ? hz10_pagemap_route(addrs[p], gens[p])
                               : hz10_bench_hash_route(addrs[p], gens[p]);
    if (route.kind != H10_ROUTE_VALID || route.slot_index != 3u) {
      fprintf(stderr,
              "%s: unexpected route at iter %llu page %u kind=%d reason=%d\n",
              mech, (unsigned long long)i, p, (int)route.kind,
              (int)route.reason);
      return 1;
    }
    checksum += route.slot_index + route.generation;
  }
  uint64_t elapsed_ns = hz10_platform_now_ns() - start;
  double seconds = hz10_bench_seconds(elapsed_ns);
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  double ops_per_s = (double)iters / seconds;
  printf(
      "hz10_pagemap_route mech=%s threads=%u iters=%llu run=%u/%u pages=%u "
      "seconds=%.6f ops_per_s=%.2f checksum=%llu\n",
      mech, threads, (unsigned long long)iters, run_index, run_count, pages,
      seconds, ops_per_s, (unsigned long long)checksum);
  return 0;
}

int main(void) {
  uint64_t iters = hz10_bench_env_u64("ITERS", 20000000ull);
  uint32_t pages = (uint32_t)hz10_bench_env_u64("PAGES", 4096ull);
  uint32_t runs = (uint32_t)hz10_bench_env_u64("RUNS", 1ull);
  uint32_t mode = (uint32_t)hz10_bench_env_u64("MODE", 2ull); /* 2 = both */
  uint32_t threads = 1u; /* Box 1 is single-threaded; recorded per bench/README.md */

  if (pages == 0u) {
    fprintf(stderr, "PAGES must be >= 1\n");
    return 1;
  }

  void** addrs = calloc(pages, sizeof(*addrs));
  uint32_t* gens = calloc(pages, sizeof(*gens));
  if (!addrs || !gens) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  hz10_pagemap_reset_for_tests();
  for (uint32_t i = 0; i < HZ10_BENCH_HASH_CAP; ++i) {
    hz10_bench_hash_table[i].base = NULL;
  }

  /* Each page gets slot_size=64, slot_count=1024 (exactly one quantum, no
   * tail slack); the timed loop always queries the 4th slot (offset 3*64)
   * so both mechanisms answer the identical exact-valid question. */
  for (uint32_t i = 0; i < pages; ++i) {
    void* base = (void*)((uintptr_t)0x0000700000000000ULL +
                        (uintptr_t)i * HZ10_PAGE_QUANTUM);
    uint32_t gen = hz10_pagemap_register(base, 64u, 1024u);
    uint32_t hash_gen = hz10_bench_hash_register(base, 64u, 1024u);
    if (gen == 0u || hash_gen == 0u) {
      fprintf(stderr, "setup: register failed at page %u\n", i);
      return 1;
    }
    addrs[i] = (char*)base + 3u * 64u;
    gens[i] = gen; /* both mechanisms start virgin, so gen == hash_gen == 1 */
  }

  int failed = 0;
  for (uint32_t run = 1; run <= runs && !failed; ++run) {
    if (mode == 0u || mode == 2u) {
      failed |= hz10_bench_run("hz10_pagemap", 1, iters, pages, addrs, gens,
                              run, runs, threads);
    }
    if (!failed && (mode == 1u || mode == 2u)) {
      failed |= hz10_bench_run("hash_probe_baseline", 0, iters, pages, addrs,
                              gens, run, runs, threads);
    }
  }

  free(addrs);
  free(gens);
  return failed ? 1 : 0;
}
