#include "../src/h8_internal.h"
#include "../src/h8_hz9_last_token_integrated_entry.h"
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

#if defined(__GNUC__) || defined(__clang__)
#define H9_BENCH_NOINLINE __attribute__((noinline))
#else
#define H9_BENCH_NOINLINE
#endif

typedef enum H9LspBenchMode {
  H9_LSP_BENCH_SPLIT = 0,
  H9_LSP_BENCH_USABLE = 1,
  H9_LSP_BENCH_REALLOC = 2,
  H9_LSP_BENCH_SPLIT_DIRECT = 3,
  H9_LSP_BENCH_KNOWN_SLOT = 4,
  H9_LSP_BENCH_ALLOC_SLOT_ONLY = 5,
  H9_LSP_BENCH_INLINE_BODY = 6,
  H9_LSP_BENCH_PTR_TOKEN = 7,
  H9_LSP_BENCH_PTR_ENTRY = 8,
  H9_LSP_BENCH_PTR_ENTRY_USABLE = 9,
  H9_LSP_BENCH_PTR_ENTRY_REALLOC = 10,
  H9_LSP_BENCH_INLINE_PUBLIC = 11,
  H9_LSP_BENCH_LAST_PUBLIC = 12,
  H9_LSP_BENCH_LAST_LEDGER = 13,
  H9_LSP_BENCH_HOT_COLD = 14,
  H9_LSP_BENCH_LAST_ONLY = 15,
  H9_LSP_BENCH_LAST_ENTRY = 16,
  H9_LSP_BENCH_INTEGRATED = 17,
  H9_LSP_BENCH_FAST_LEAF = 18
} H9LspBenchMode;

typedef struct H9LspLoopResult {
  uint64_t ok;
  uintptr_t sink;
  double elapsed;
  uint64_t fast_hits;
  uint64_t fallback_hits;
  uint64_t state_mismatch;
  int rc;
} H9LspLoopResult;

typedef struct H9LspInlinePublic {
  H9LspInlinePage page;
  H9LspPtrLedger ledger;
  uint32_t class_id;
} H9LspInlinePublic;

typedef struct H9LspLastPublic {
  H9LspInlinePage page;
  H9LspPtrToken last;
  uint32_t class_id;
} H9LspLastPublic;

typedef struct H9LspLastOnlyPublic {
  H9LspInlinePage page;
  H9LspPtrLastOnly cache;
  uint32_t class_id;
} H9LspLastOnlyPublic;

typedef struct H9LspLastLedgerPublic {
  H9LspInlinePage page;
  H9LspPtrTokenCache cache;
  uint32_t class_id;
} H9LspLastLedgerPublic;

typedef struct H9LspHotColdPublic {
  H9LspInlinePage page;
  H9LspPtrTokenHotCold cache;
  H9LspPtrLedger ledger;
  uint32_t class_id;
} H9LspHotColdPublic;

static inline bool h9_lsp_inline_public_alloc(H9LspInlinePublic* entry,
                                              void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  h9_lsp_ptr_ledger_insert(&entry->ledger, ptr, &entry->page, slot,
                           entry->class_id);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_inline_public_free(H9LspInlinePublic* entry,
                                             void* ptr) {
  return h9_lsp_ptr_ledger_free_hit(&entry->ledger, ptr);
}

static inline bool h9_lsp_last_public_alloc(H9LspLastPublic* entry,
                                            void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  entry->last = (H9LspPtrToken){
      .ptr = ptr,
      .page = &entry->page,
      .slot = slot,
      .generation = entry->page.generation,
      .live = 1u,
  };
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_last_public_free(H9LspLastPublic* entry,
                                           void* ptr) {
  H9LspPtrToken* token = &entry->last;
  if (!token->live || token->ptr != ptr || token->page != &entry->page ||
      token->generation != entry->page.generation ||
      token->slot >= entry->page.slot_count) {
    return false;
  }
  bool ok = h9_lsp_inline_free_slot(&entry->page, token->slot);
  h9_lsp_ptr_ledger_clear_entry(token);
  return ok;
}

static inline bool h9_lsp_last_only_alloc(H9LspLastOnlyPublic* entry,
                                          void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  h9_lsp_ptr_last_insert(&entry->cache, ptr, &entry->page, slot,
                         entry->class_id);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_last_only_free(H9LspLastOnlyPublic* entry,
                                         void* ptr) {
  return h9_lsp_ptr_last_free_hit(&entry->cache, ptr);
}

static inline bool h9_lsp_last_ledger_alloc(H9LspLastLedgerPublic* entry,
                                            void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  h9_lsp_ptr_cache_insert(&entry->cache, ptr, &entry->page, slot,
                          entry->class_id);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_last_ledger_free(H9LspLastLedgerPublic* entry,
                                           void* ptr) {
  return h9_lsp_ptr_cache_free_hit(&entry->cache, ptr);
}

static inline bool h9_lsp_hotcold_alloc(H9LspHotColdPublic* entry,
                                        void** ptr_out) {
  uint32_t slot = UINT32_MAX;
  void* ptr = NULL;
  if (!h9_lsp_inline_alloc_slot(&entry->page, &slot, &ptr)) {
    return false;
  }
  h9_lsp_ptr_hotcold_insert(&entry->cache, ptr, &entry->page, slot,
                            entry->class_id);
  if (ptr_out) {
    *ptr_out = ptr;
  }
  return true;
}

static inline bool h9_lsp_hotcold_free(H9LspHotColdPublic* entry, void* ptr) {
  return h9_lsp_ptr_hotcold_free_hit(&entry->cache, ptr);
}

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

static inline bool h9_lsp_bench_fastleaf_free(H9LspIntegratedEntry* entry,
                                              void* ptr) {
  if (entry->last_live && entry->last_ptr == ptr &&
      entry->last_generation == entry->page.generation &&
      entry->last_slot < entry->page.slot_count) {
    uint32_t slot = entry->last_slot;
    uint64_t bit = UINT64_C(1) << slot;
    if ((entry->page.alloc_bits & bit) != 0u &&
        (entry->page.free_bits & bit) == 0u) {
      entry->page.alloc_bits &= ~bit;
      entry->page.free_bits |= bit;
      entry->last_live = 0u;
      entry->fast_hits++;
      return true;
    }
    entry->last_live = 0u;
    entry->state_mismatch++;
  }
  return false;
}

static H9_BENCH_NOINLINE H9LspLoopResult h9_lsp_run_integrated_worker(
    uint32_t class_id, uint32_t slot_size, uint64_t iters, bool touch) {
  H9LspLoopResult result = {0};
  size_t payload_bytes = (size_t)slot_size * 64u;
  void* payload = malloc(payload_bytes);
  if (!payload) {
    result.rc = 2;
    return result;
  }
  H9LspIntegratedEntry entry;
  h9_lsp_integrated_init(&entry, (uintptr_t)payload, slot_size, 64u, class_id);
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = NULL;
    bool success = h9_lsp_integrated_alloc(&entry, &ptr);
    if (success && touch) {
      volatile unsigned char* p = (volatile unsigned char*)ptr;
      p[0] = (unsigned char)i;
      p[slot_size - 1u] = (unsigned char)(i >> 8);
    }
    success = success && h9_lsp_integrated_free(&entry, ptr);
    if (!success) {
      result.rc = 3;
      free(payload);
      return result;
    }
    result.sink ^= (uintptr_t)ptr;
    ++result.ok;
  }
  result.elapsed = now_seconds() - start;
  result.fast_hits = entry.fast_hits;
  result.fallback_hits = entry.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  free(payload);
  return result;
}

static H9_BENCH_NOINLINE H9LspLoopResult h9_lsp_run_fastleaf_worker(
    uint32_t class_id, uint32_t slot_size, uint64_t iters, bool touch) {
  H9LspLoopResult result = {0};
  size_t payload_bytes = (size_t)slot_size * 64u;
  void* payload = malloc(payload_bytes);
  if (!payload) {
    result.rc = 2;
    return result;
  }
  H9LspIntegratedEntry entry;
  h9_lsp_integrated_init(&entry, (uintptr_t)payload, slot_size, 64u, class_id);
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = NULL;
    bool success = h9_lsp_integrated_alloc(&entry, &ptr);
    if (success && touch) {
      volatile unsigned char* p = (volatile unsigned char*)ptr;
      p[0] = (unsigned char)i;
      p[slot_size - 1u] = (unsigned char)(i >> 8);
    }
    success = success && h9_lsp_bench_fastleaf_free(&entry, ptr);
    if (!success) {
      result.rc = 3;
      free(payload);
      return result;
    }
    result.sink ^= (uintptr_t)ptr;
    ++result.ok;
  }
  result.elapsed = now_seconds() - start;
  result.fast_hits = entry.fast_hits;
  result.fallback_hits = entry.fallback_hits;
  result.state_mismatch = entry.state_mismatch;
  free(payload);
  return result;
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
  } else if (strcmp(mode, "ptrentry") == 0) {
    bench_mode = H9_LSP_BENCH_PTR_ENTRY;
  } else if (strcmp(mode, "ptrusable") == 0) {
    bench_mode = H9_LSP_BENCH_PTR_ENTRY_USABLE;
  } else if (strcmp(mode, "ptrrealloc") == 0) {
    bench_mode = H9_LSP_BENCH_PTR_ENTRY_REALLOC;
  } else if (strcmp(mode, "inlinepublic") == 0) {
    bench_mode = H9_LSP_BENCH_INLINE_PUBLIC;
  } else if (strcmp(mode, "lastpublic") == 0) {
    bench_mode = H9_LSP_BENCH_LAST_PUBLIC;
  } else if (strcmp(mode, "lastledger") == 0) {
    bench_mode = H9_LSP_BENCH_LAST_LEDGER;
  } else if (strcmp(mode, "hotcold") == 0) {
    bench_mode = H9_LSP_BENCH_HOT_COLD;
  } else if (strcmp(mode, "lastonly") == 0) {
    bench_mode = H9_LSP_BENCH_LAST_ONLY;
  } else if (strcmp(mode, "lastentry") == 0) {
    bench_mode = H9_LSP_BENCH_LAST_ENTRY;
  } else if (strcmp(mode, "integrated") == 0) {
    bench_mode = H9_LSP_BENCH_INTEGRATED;
  } else if (strcmp(mode, "fastleaf") == 0) {
    bench_mode = H9_LSP_BENCH_FAST_LEAF;
  }
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_size == 0u) {
    fprintf(stderr, "lsp bench invalid class %u\n", class_id);
    return 1;
  }
  uint32_t slot_size = spec->slot_size;

  if (bench_mode == H9_LSP_BENCH_INLINE_PUBLIC) {
    size_t payload_bytes = (size_t)slot_size * 64u;
    void* payload = malloc(payload_bytes);
    if (!payload) {
      fprintf(stderr, "lsp inline public payload alloc failed\n");
      return 2;
    }
    H9LspInlinePublic public_entry = {
        .class_id = class_id,
    };
    h9_lsp_inline_page_init(&public_entry.page, (uintptr_t)payload, slot_size,
                            64u);
    public_entry.page.class_id = class_id;
    uint64_t ok = 0u;
    uintptr_t sink = 0u;
    double start = now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      void* ptr = NULL;
      bool success = h9_lsp_inline_public_alloc(&public_entry, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_lsp_inline_public_free(&public_entry, ptr);
      if (!success) {
        fprintf(stderr, "lsp inline public transition failed at iter %llu\n",
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

  if (bench_mode == H9_LSP_BENCH_LAST_PUBLIC) {
    size_t payload_bytes = (size_t)slot_size * 64u;
    void* payload = malloc(payload_bytes);
    if (!payload) {
      fprintf(stderr, "lsp last public payload alloc failed\n");
      return 2;
    }
    H9LspLastPublic public_entry = {
        .class_id = class_id,
    };
    h9_lsp_inline_page_init(&public_entry.page, (uintptr_t)payload, slot_size,
                            64u);
    public_entry.page.class_id = class_id;
    uint64_t ok = 0u;
    uintptr_t sink = 0u;
    double start = now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      void* ptr = NULL;
      bool success = h9_lsp_last_public_alloc(&public_entry, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_lsp_last_public_free(&public_entry, ptr);
      if (!success) {
        fprintf(stderr, "lsp last public transition failed at iter %llu\n",
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

  if (bench_mode == H9_LSP_BENCH_INTEGRATED ||
      bench_mode == H9_LSP_BENCH_FAST_LEAF) {
    H9LspLoopResult result =
        bench_mode == H9_LSP_BENCH_FAST_LEAF
            ? h9_lsp_run_fastleaf_worker(class_id, slot_size, iters, touch)
            : h9_lsp_run_integrated_worker(class_id, slot_size, iters, touch);
    if (result.rc != 0) {
      fprintf(stderr, "lsp %s transition failed rc=%d\n", mode, result.rc);
      return result.rc;
    }
    double elapsed = result.elapsed;
    if (elapsed <= 0.0) {
      elapsed = 1e-9;
    }
    printf("mode=%s class=%u iters=%llu touch=%u ops_per_s=%.2f "
           "elapsed=%.6f route_valid=0 route_invalid=0 route_miss=0 "
           "malloc_hit=0 ptr_fast=%llu ptr_fallback=%llu "
           "state_mismatch=%llu segment_create=0 sink=%" PRIuPTR "\n",
           mode, class_id, (unsigned long long)iters, touch ? 1u : 0u,
           (double)result.ok / elapsed, elapsed,
           (unsigned long long)result.fast_hits,
           (unsigned long long)result.fallback_hits,
           (unsigned long long)result.state_mismatch, result.sink);
    return 0;
  }

  if (bench_mode == H9_LSP_BENCH_LAST_ONLY) {
    size_t payload_bytes = (size_t)slot_size * 64u;
    void* payload = malloc(payload_bytes);
    if (!payload) {
      fprintf(stderr, "lsp lastonly payload alloc failed\n");
      return 2;
    }
    H9LspLastOnlyPublic public_entry = {
        .class_id = class_id,
    };
    h9_lsp_inline_page_init(&public_entry.page, (uintptr_t)payload, slot_size,
                            64u);
    public_entry.page.class_id = class_id;
    uint64_t ok = 0u;
    uintptr_t sink = 0u;
    double start = now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      void* ptr = NULL;
      bool success = h9_lsp_last_only_alloc(&public_entry, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_lsp_last_only_free(&public_entry, ptr);
      if (!success) {
        fprintf(stderr, "lsp lastonly transition failed at iter %llu\n",
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

  if (bench_mode == H9_LSP_BENCH_LAST_LEDGER) {
    size_t payload_bytes = (size_t)slot_size * 64u;
    void* payload = malloc(payload_bytes);
    if (!payload) {
      fprintf(stderr, "lsp last ledger payload alloc failed\n");
      return 2;
    }
    H9LspLastLedgerPublic public_entry = {
        .class_id = class_id,
    };
    h9_lsp_inline_page_init(&public_entry.page, (uintptr_t)payload, slot_size,
                            64u);
    public_entry.page.class_id = class_id;
    uint64_t ok = 0u;
    uintptr_t sink = 0u;
    double start = now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      void* ptr = NULL;
      bool success = h9_lsp_last_ledger_alloc(&public_entry, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_lsp_last_ledger_free(&public_entry, ptr);
      if (!success) {
        fprintf(stderr, "lsp last ledger transition failed at iter %llu\n",
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

  if (bench_mode == H9_LSP_BENCH_HOT_COLD) {
    size_t payload_bytes = (size_t)slot_size * 64u;
    void* payload = malloc(payload_bytes);
    if (!payload) {
      fprintf(stderr, "lsp hotcold payload alloc failed\n");
      return 2;
    }
    H9LspHotColdPublic public_entry = {
        .class_id = class_id,
    };
    public_entry.cache.cold = &public_entry.ledger;
    h9_lsp_inline_page_init(&public_entry.page, (uintptr_t)payload, slot_size,
                            64u);
    public_entry.page.class_id = class_id;
    uint64_t ok = 0u;
    uintptr_t sink = 0u;
    double start = now_seconds();
    for (uint64_t i = 0; i < iters; ++i) {
      void* ptr = NULL;
      bool success = h9_lsp_hotcold_alloc(&public_entry, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_lsp_hotcold_free(&public_entry, ptr);
      if (!success) {
        fprintf(stderr, "lsp hotcold transition failed at iter %llu\n",
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
    bool alloc_ok = bench_mode == H9_LSP_BENCH_PTR_ENTRY ||
                            bench_mode == H9_LSP_BENCH_PTR_ENTRY_USABLE ||
                            bench_mode == H9_LSP_BENCH_PTR_ENTRY_REALLOC
                        ? (ptr = h9_lsp_debug_ptrtoken_alloc(class_id)) != NULL
                    : bench_mode == H9_LSP_BENCH_LAST_ENTRY
                        ? (ptr = h9_lsp_debug_lasttoken_alloc(class_id)) !=
                              NULL
                    : bench_mode == H9_LSP_BENCH_KNOWN_SLOT ||
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
    } else if (bench_mode == H9_LSP_BENCH_PTR_ENTRY_USABLE) {
      size_t usable = 0u;
      success = h9_lsp_debug_ptrtoken_usable_size(ptr, &usable, &owned) &&
                owned && usable != 0u &&
                h9_lsp_debug_ptrtoken_free(ptr, &owned) && owned;
      sink += usable;
    } else if (bench_mode == H9_LSP_BENCH_REALLOC) {
      void* same = h9_lsp_debug_realloc_in_place(ptr, 1u, &owned);
      success = same == ptr && owned && h9_lsp_debug_free(ptr, &owned) &&
                owned;
      sink ^= (uintptr_t)same;
    } else if (bench_mode == H9_LSP_BENCH_PTR_ENTRY_REALLOC) {
      void* same = h9_lsp_debug_ptrtoken_realloc_in_place(ptr, 1u, &owned);
      success = same == ptr && owned &&
                h9_lsp_debug_ptrtoken_free(ptr, &owned) && owned;
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
    } else if (bench_mode == H9_LSP_BENCH_PTR_ENTRY) {
      success = h9_lsp_debug_ptrtoken_free(ptr, &owned) && owned;
      sink ^= (uintptr_t)ptr;
    } else if (bench_mode == H9_LSP_BENCH_LAST_ENTRY) {
      success = h9_lsp_debug_lasttoken_free(ptr, &owned) && owned;
      sink ^= (uintptr_t)ptr;
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
         "malloc_hit=%zu ptr_fast=%zu ptr_fallback=%zu segment_create=%zu "
         "sink=%" PRIuPTR "\n",
         mode, class_id, (unsigned long long)iters, touch ? 1u : 0u, ops,
         elapsed, stats.route_valid, stats.route_invalid, stats.route_miss,
         stats.malloc_tls_page_hit, stats.ptrtoken_free_fast,
         stats.ptrtoken_free_fallback, stats.segment_create, sink);
  h9_lsp_debug_reset();
  return 0;
}

#undef H9_BENCH_NOINLINE
