#include "../src/h8_internal.h"
#include "../src/h8_hz9_segment_entry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(H9_SEGMENT_ENTRY_L1)
#error "segment entry bench requires H9_SEGMENT_ENTRY_L1"
#endif

typedef enum H9SegmentEntryBenchMode {
  H9_SEGMENT_ENTRY_BENCH_ROUTE = 0,
  H9_SEGMENT_ENTRY_BENCH_FUSED = 1,
  H9_SEGMENT_ENTRY_BENCH_FAST = 2,
  H9_SEGMENT_ENTRY_BENCH_PAGE = 3,
  H9_SEGMENT_ENTRY_BENCH_HANDLE = 4,
  H9_SEGMENT_ENTRY_BENCH_TLS = 5,
  H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE = 6,
  H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL = 7,
  H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN = 8,
  H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED = 9
} H9SegmentEntryBenchMode;

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
  const char* mode = env_str("MODE", "route");
  H9SegmentEntryBenchMode bench_mode = H9_SEGMENT_ENTRY_BENCH_ROUTE;
  if (strcmp(mode, "fused") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_FUSED;
  } else if (strcmp(mode, "fast") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_FAST;
  } else if (strcmp(mode, "page") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_PAGE;
  } else if (strcmp(mode, "handle") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_HANDLE;
  } else if (strcmp(mode, "tls") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS;
  } else if (strcmp(mode, "tlsroute") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE;
  } else if (strcmp(mode, "tlslocal") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL;
  } else if (strcmp(mode, "tlsknown") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN;
  } else if (strcmp(mode, "tlschecked") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED;
  }

  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_count == 0u) {
    fprintf(stderr, "segment entry bench invalid class %u\n", class_id);
    return 2;
  }
  slot_size = spec->slot_size;
  run_size = spec->run_size;
  slot_count = spec->slot_count;

  h9_segment_entry_debug_reset();
  uint32_t page_id = UINT32_MAX;
  uintptr_t page_handle = 0u;
  if (bench_mode == H9_SEGMENT_ENTRY_BENCH_PAGE) {
    page_id = h9_segment_entry_debug_prepare_active(class_id);
    if (page_id == UINT32_MAX) {
      fprintf(stderr, "segment entry bench failed to prepare page\n");
      h9_segment_entry_debug_reset();
      return 3;
    }
  } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED) {
    page_handle = h9_segment_entry_debug_prepare_handle(class_id);
    if (page_handle == 0u) {
      fprintf(stderr, "segment entry bench failed to prepare handle\n");
      h9_segment_entry_debug_reset();
      return 3;
    }
  }
  uint64_t ok = 0u;
  double start = now_seconds();
  for (uint64_t i = 0u; i < iters; ++i) {
    void* ptr = NULL;
    bool success = false;
    if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED) {
      success = h9_segment_entry_debug_cycle_tls_checked(class_id, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN) {
      bool owned = false;
      uint32_t slot = 0u;
      success = h9_segment_entry_debug_alloc_tls_slot(class_id, &ptr, &slot);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success &&
                h9_segment_entry_debug_free_tls_slot(class_id, slot, &owned) &&
                owned;
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL) {
      bool owned = false;
      success = h9_segment_entry_debug_alloc_tls_handle(class_id, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success =
          success && h9_segment_entry_debug_free_tls_handle(class_id, ptr,
                                                            &owned) &&
          owned;
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE) {
      bool owned = false;
      success = h9_segment_entry_debug_alloc_tls_handle(class_id, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_segment_entry_debug_free(ptr, &owned) && owned;
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS) {
      success = h9_segment_entry_debug_cycle_tls_handle(class_id, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE) {
      success = h9_segment_entry_debug_cycle_handle(page_handle, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_PAGE) {
      success = h9_segment_entry_debug_cycle_page(page_id, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_FAST) {
      success = h9_segment_entry_debug_cycle_active_fast(class_id, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_FUSED) {
      success = h9_segment_entry_debug_cycle_fused(class_id, &ptr);
    } else {
      bool owned = false;
      success = h9_segment_entry_debug_alloc(class_id, &ptr);
      if (success && touch) {
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        p[0] = (unsigned char)i;
        p[slot_size - 1u] = (unsigned char)(i >> 8);
      }
      success = success && h9_segment_entry_debug_free(ptr, &owned) && owned;
    }
    if (success && touch && bench_mode != H9_SEGMENT_ENTRY_BENCH_ROUTE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED) {
      volatile unsigned char* p = (volatile unsigned char*)ptr;
      p[0] = (unsigned char)i;
      p[slot_size - 1u] = (unsigned char)(i >> 8);
    }
    if (!success) {
      fprintf(stderr, "segment entry bench failed at iter %llu\n",
              (unsigned long long)i);
      h9_segment_entry_debug_reset();
      return 3;
    }
    ++ok;
  }
  double elapsed = now_seconds() - start;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  printf("hz9_segment_entry mode=%s class=%u slot_size=%u run_size=%u "
         "slot_count=%u touch=%u iters=%llu seconds=%.6f cycles_per_s=%.2f "
         "ops_per_s=%.2f pages=%u\n",
         mode, class_id, slot_size, run_size, slot_count, touch ? 1u : 0u,
         (unsigned long long)iters, elapsed, (double)ok / elapsed,
         (double)ok * 2.0 / elapsed, h9_segment_entry_debug_page_count());
  h9_segment_entry_debug_reset();
  return 0;
}
