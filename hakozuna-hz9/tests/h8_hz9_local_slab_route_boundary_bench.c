#include "../src/h8_internal.h"
#include "../src/h8_hz9_local_slab_inline_body.h"
#include "../src/h8_hz9_local_slab_pointer_token.h"
#include "../src/h8_hz9_local_slab_route_boundary.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)
#error "local slab route boundary bench requires L0 flag"
#endif

typedef enum H9LspBenchMode {
  H9_LSP_BENCH_SPLIT = 0,
  H9_LSP_BENCH_USABLE = 1,
  H9_LSP_BENCH_REALLOC = 2,
  H9_LSP_BENCH_SPLIT_DIRECT = 3,
  H9_LSP_BENCH_KNOWN_SLOT = 4,
  H9_LSP_BENCH_ALLOC_SLOT_ONLY = 5,
  H9_LSP_BENCH_INLINE_BODY = 6,
  H9_LSP_BENCH_PTR_TOKEN = 7
} H9LspBenchMode;

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !*value) {
    return fallback;
  }
  char* end = NULL;
  unsigned long long parsed = strtoull(value, &end, 10);
  return end && *end == '\0' ? (uint64_t)parsed : fallback;
}

static const char* env_str(const char* name, const char* fallback) {
  const char* value = getenv(name);
  return value && *value ? value : fallback;
}

int main(void) {
  uint32_t class_id = (uint32_t)env_u64("CLASS_ID", 5u);
  uint64_t iters = env_u64("ITERS", 10000000u);
  bool touch = env_u64("TOUCH", 1u) != 0u;
  const char* mode = env_str("MODE", "split");
  H9LspBenchMode bench_mode = H9_LSP_BENCH_SPLIT;
  if (strcmp(mode, "usable") == 0) {
    bench_mode = H9_LSP_BENCH_USABLE;
  } else if (strcmp(mode, "realloc") == 0) {
    bench_mode = H9_LSP_BENCH_REALLOC;
  } else if (strcmp(mode, "splitdirect") == 0) {
    bench_mode = H9_LSP_BENCH_SPLIT_DIRECT;
  } else if (strcmp(mode, "knownslot") == 0) {
    bench_mode = H9_LSP_BENCH_KNOWN_SLOT;
  } else if (strcmp(mode, "allocslotonly") == 0) {
    bench_mode = H9_LSP_BENCH_ALLOC_SLOT_ONLY;
  } else if (strcmp(mode, "inlinebody") == 0) {
    bench_mode = H9_LSP_BENCH_INLINE_BODY;
  } else if (strcmp(mode, "ptrtoken") == 0) {
    bench_mode = H9_LSP_BENCH_PTR_TOKEN;
  }
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_size == 0u) {
    fprintf(stderr, "lsp bench invalid class %u\n", class_id);
    return 1;
  }
  uint32_t slot_size = spec->slot_size;

  if (bench_mode == H9_LSP_BENCH_INLINE_BODY ||
      bench_mode == H9_LSP_BENCH_PTR_TOKEN) {
    size_t payload_bytes = (size_t)slot_size * 64u;
    void* payload = malloc(payload_bytes);
    if (!payload) {
      fprintf(stderr, "lsp inline payload alloc failed\n");
      return 2;
    }
    H9LspInlinePage page;
    h9_lsp_inline_page_init(&page, (uintptr_t)payload, slot_size, 64u);
    page.class_id = class_id;
    H9LspPtrLedger ledger = {0};
    uint64_t ok = 0u;
    uintptr_t sink = 0u;
    double start = now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      uint32_t slot = UINT32_MAX;
      void* ptr = NULL;
      bool success = h9_lsp_inline_alloc_slot(&page, &slot, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      if (success && bench_mode == H9_LSP_BENCH_PTR_TOKEN) {
        h9_lsp_ptr_ledger_insert(&ledger, ptr, &page, slot, class_id);
        success = h9_lsp_ptr_ledger_free_hit(&ledger, ptr);
      } else {
        success = success && h9_lsp_inline_free_slot(&page, slot);
      }
      if (!success) {
        fprintf(stderr, "lsp inline transition failed at iter %llu\n",
                (unsigned long long)i);
        free(payload);
        return 3;
      }
      sink ^= (uintptr_t)ptr;
      ++ok;
    }
    double elapsed = now_seconds() - start;
    if (elapsed <= 0.0) {
      elapsed = 1e-9;
    }
    printf("mode=%s class=%u iters=%llu touch=%u ops_per_s=%.2f "
           "elapsed=%.6f route_valid=0 route_invalid=0 route_miss=0 "
           "malloc_hit=0 segment_create=0 sink=%" PRIuPTR "\n",
           mode, class_id, (unsigned long long)iters, touch ? 1u : 0u,
           (double)ok / elapsed, elapsed, sink);
    free(payload);
    return 0;
  }

  h9_lsp_debug_reset();
  uint64_t ok = 0u;
  uintptr_t sink = 0u;
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    bool owned = false;
    uint32_t slot = UINT32_MAX;
    void* ptr = NULL;
    bool alloc_ok = bench_mode == H9_LSP_BENCH_KNOWN_SLOT ||
                            bench_mode == H9_LSP_BENCH_ALLOC_SLOT_ONLY
                        ? h9_lsp_debug_alloc_slot(class_id, &ptr, &slot)
                        : (ptr = h9_lsp_debug_alloc(class_id)) != NULL;
    if (!alloc_ok || !ptr) {
      fprintf(stderr, "lsp bench alloc failed at iter %llu\n",
              (unsigned long long)i);
      h9_lsp_debug_reset();
      return 2;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)ptr;
      p[0] = (unsigned char)i;
      p[slot_size - 1u] = (unsigned char)(i >> 8);
    }

    bool success = false;
    if (bench_mode == H9_LSP_BENCH_USABLE) {
      size_t usable = 0u;
      success = h9_lsp_debug_usable_size(ptr, &usable, &owned) && owned &&
                usable != 0u && h9_lsp_debug_free(ptr, &owned) && owned;
      sink += usable;
    } else if (bench_mode == H9_LSP_BENCH_REALLOC) {
      void* same = h9_lsp_debug_realloc_in_place(ptr, 1u, &owned);
      success = same == ptr && owned && h9_lsp_debug_free(ptr, &owned) &&
                owned;
      sink ^= (uintptr_t)same;
    } else if (bench_mode == H9_LSP_BENCH_SPLIT_DIRECT) {
      success = h9_lsp_debug_free_direct_owned(ptr);
      sink ^= (uintptr_t)ptr;
    } else if (bench_mode == H9_LSP_BENCH_KNOWN_SLOT) {
      success = h9_lsp_debug_free_known_slot(class_id, slot);
      sink ^= (uintptr_t)ptr;
    } else if (bench_mode == H9_LSP_BENCH_ALLOC_SLOT_ONLY) {
      success = h9_lsp_debug_free_known_slot(class_id, slot);
      sink += slot;
    } else {
      success = h9_lsp_debug_free(ptr, &owned) && owned;
      sink ^= (uintptr_t)ptr;
    }
    if (!success) {
      fprintf(stderr, "lsp bench transition failed at iter %llu\n",
              (unsigned long long)i);
      h9_lsp_debug_reset();
      return 4;
    }
    ++ok;
  }
  double elapsed = now_seconds() - start;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  H9LspStats stats = h9_lsp_debug_stats();
  double ops = (double)ok / elapsed;
  printf("mode=%s class=%u iters=%llu touch=%u ops_per_s=%.2f "
         "elapsed=%.6f route_valid=%zu route_invalid=%zu route_miss=%zu "
         "malloc_hit=%zu segment_create=%zu sink=%" PRIuPTR "\n",
         mode, class_id, (unsigned long long)iters, touch ? 1u : 0u, ops,
         elapsed, stats.route_valid, stats.route_invalid, stats.route_miss,
         stats.malloc_tls_page_hit, stats.segment_create, sink);
  h9_lsp_debug_reset();
  return 0;
}
