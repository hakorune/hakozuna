#include "../src/h8_hz9_segment_entry_internal.h"

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
  H9_SEGMENT_ENTRY_BENCH_HANDLE_CHECKED_TOUCH = 5,
  H9_SEGMENT_ENTRY_BENCH_HANDLE_BODY = 6,
  H9_SEGMENT_ENTRY_BENCH_HANDLE_GUARD_BODY = 7,
  H9_SEGMENT_ENTRY_BENCH_TOKEN_BODY = 8,
  H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_BODY = 9,
  H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE = 10,
  H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE = 11,
  H9_SEGMENT_ENTRY_BENCH_TLS = 12,
  H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE = 13,
  H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL = 14,
  H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN = 15,
  H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED = 16,
  H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED_TOUCH = 17,
  H9_SEGMENT_ENTRY_BENCH_TLS_BODY = 18,
  H9_SEGMENT_ENTRY_BENCH_TLS_BODY_CHECKED = 19,
  H9_SEGMENT_ENTRY_BENCH_TLS_GUARD_BODY = 20,
  H9_SEGMENT_ENTRY_BENCH_TLS_EPOCH_BODY = 21,
  H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_BODY = 22,
  H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_EVERY = 23,
  H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE64_BODY = 24,
  H9_SEGMENT_ENTRY_BENCH_TLS_CACHE = 25,
  H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE = 26,
  H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY = 27,
  H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER = 28,
  H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER_BODY = 29
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
  uint32_t route_period = (uint32_t)env_u64("ROUTE_PERIOD", 16u);
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
  } else if (strcmp(mode, "handlecheckedtouch") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_HANDLE_CHECKED_TOUCH;
  } else if (strcmp(mode, "handlebody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_HANDLE_BODY;
  } else if (strcmp(mode, "handleguardbody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_HANDLE_GUARD_BODY;
  } else if (strcmp(mode, "tokenbody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TOKEN_BODY;
  } else if (strcmp(mode, "tokencachebody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_BODY;
  } else if (strcmp(mode, "tokencacheretire") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE;
  } else if (strcmp(mode, "tokencachestate") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE;
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
  } else if (strcmp(mode, "tlscheckedtouch") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED_TOUCH;
  } else if (strcmp(mode, "tlsbody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_BODY;
  } else if (strcmp(mode, "tlsbodychecked") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_BODY_CHECKED;
  } else if (strcmp(mode, "tlsguardbody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_GUARD_BODY;
  } else if (strcmp(mode, "tlsepochbody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_EPOCH_BODY;
  } else if (strcmp(mode, "tlsroutebody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_BODY;
  } else if (strcmp(mode, "tlsrouteevery") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_EVERY;
  } else if (strcmp(mode, "tlsroute64body") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE64_BODY;
  } else if (strcmp(mode, "tlscache") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_CACHE;
  } else if (strcmp(mode, "tlstokencache") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE;
  } else if (strcmp(mode, "tlstokencachebody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY;
  } else if (strcmp(mode, "tlsledger") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER;
  } else if (strcmp(mode, "tlsledgerbody") == 0) {
    bench_mode = H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER_BODY;
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
  uint32_t page_generation = 0u;
  H9SegmentEntryToken page_token = {0};
  if (bench_mode == H9_SEGMENT_ENTRY_BENCH_PAGE) {
    page_id = h9_segment_entry_debug_prepare_active(class_id);
    if (page_id == UINT32_MAX) {
      fprintf(stderr, "segment entry bench failed to prepare page\n");
      h9_segment_entry_debug_reset();
      return 3;
    }
  } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE_CHECKED_TOUCH ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE_GUARD_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED_TOUCH ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_BODY_CHECKED ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_GUARD_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_EPOCH_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_EVERY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE64_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CACHE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER ||
             bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER_BODY) {
    page_handle = h9_segment_entry_debug_prepare_handle(class_id);
    if (page_handle == 0u) {
      fprintf(stderr, "segment entry bench failed to prepare handle\n");
      h9_segment_entry_debug_reset();
      return 3;
    }
    page_generation = h9_segment_entry_debug_handle_generation(page_handle);
    if ((bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_BODY ||
         bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_BODY ||
         bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE ||
         bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE ||
         bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY) &&
        !h9_segment_entry_debug_acquire_token(class_id, &page_token)) {
      fprintf(stderr, "segment entry bench failed to acquire token\n");
      h9_segment_entry_debug_reset();
      return 3;
    }
  }
  uint32_t token_cache_slot = UINT32_MAX;
  void* token_cache_ptr = NULL;
  H9SegmentEntryTokenCache token_cache_state;
  h9_segment_entry_token_cache_reset(&token_cache_state);
  token_cache_state.token = page_token;
  if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY) {
    h9_segment_entry_token_cache_reset(
        &h9_segment_entry_token_cache_state[class_id]);
    h9_segment_entry_token_cache_state[class_id].token = page_token;
  }
  uint64_t ok = 0u;
  double start = now_seconds();
  for (uint64_t i = 0u; i < iters; ++i) {
    void* ptr = NULL;
    bool success = false;
    if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER_BODY) {
      success =
          h9_segment_entry_debug_cycle_tls_ledger_body(class_id, i, touch,
                                                       &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY) {
      success = h9_segment_entry_cycle_token_cache_state_inline(
          &h9_segment_entry_token_cache_state[class_id], i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE) {
      success =
          h9_segment_entry_debug_cycle_tls_token_cache(class_id, i, touch,
                                                       &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_EPOCH_BODY) {
      success =
          h9_segment_entry_debug_cycle_tls_epoch_body(class_id, i, touch,
                                                      &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_BODY) {
      success =
          h9_segment_entry_debug_cycle_tls_route_body(class_id, i, touch,
                                                      &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_EVERY) {
      success = h9_segment_entry_debug_cycle_tls_route_every(
          class_id, i, touch, route_period, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE64_BODY) {
      success =
          h9_segment_entry_debug_cycle_tls_route64_body(class_id, i, touch,
                                                       &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER) {
      success =
          h9_segment_entry_debug_cycle_tls_ledger(class_id, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CACHE) {
      success =
          h9_segment_entry_debug_cycle_tls_cache(class_id, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED_TOUCH) {
      success =
          h9_segment_entry_debug_cycle_tls_checked_touch(class_id, i, touch,
                                                         &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_BODY) {
      success = h9_segment_entry_cycle_page_checked_touch_inline(
          (H9SegmentEntryPage*)h9_segment_entry_handle[class_id], i, touch,
          &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_BODY_CHECKED) {
      H9SegmentEntryPage* page =
          (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
      success = page && page->class_id == class_id && page->free_bits != 0u &&
                h9_segment_entry_cycle_page_checked_touch_inline(page, i,
                                                                 touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_GUARD_BODY) {
      H9SegmentEntryPage* page =
          (H9SegmentEntryPage*)h9_segment_entry_handle[class_id];
      success =
          page &&
          page->generation == h9_segment_entry_handle_generation[class_id] &&
          h9_segment_entry_cycle_page_checked_touch_inline(page, i, touch,
                                                           &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_BODY) {
      success =
          h9_segment_entry_cycle_token_hot_inline(&page_token, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_BODY) {
      success = h9_segment_entry_cycle_token_cache_inline(
          &page_token, &token_cache_slot, &token_cache_ptr, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE) {
      success = h9_segment_entry_cycle_token_cache_inline(
          &page_token, &token_cache_slot, &token_cache_ptr, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE) {
      success = h9_segment_entry_cycle_token_cache_state_inline(
          &token_cache_state, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED) {
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
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE_CHECKED_TOUCH) {
      success = h9_segment_entry_debug_cycle_handle_checked_touch(
          page_handle, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE_BODY) {
      success = h9_segment_entry_cycle_page_checked_touch_inline(
          (H9SegmentEntryPage*)page_handle, i, touch, &ptr);
    } else if (bench_mode == H9_SEGMENT_ENTRY_BENCH_HANDLE_GUARD_BODY) {
      H9SegmentEntryPage* page = (H9SegmentEntryPage*)page_handle;
      success = page && page->generation == page_generation &&
                h9_segment_entry_cycle_page_checked_touch_inline(page, i,
                                                                 touch, &ptr);
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
        bench_mode != H9_SEGMENT_ENTRY_BENCH_HANDLE_CHECKED_TOUCH &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_HANDLE_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_HANDLE_GUARD_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TOKEN_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_LOCAL &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_KNOWN &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_CHECKED_TOUCH &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_BODY_CHECKED &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_GUARD_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_EPOCH_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE_EVERY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_ROUTE64_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_CACHE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER &&
        bench_mode != H9_SEGMENT_ENTRY_BENCH_TLS_LEDGER_BODY) {
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
  if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_RETIRE &&
      !h9_segment_entry_retire_token_cache_inline(&page_token,
                                                  &token_cache_slot,
                                                  &token_cache_ptr)) {
    fprintf(stderr, "segment entry bench failed to retire token cache\n");
    h9_segment_entry_debug_reset();
    return 3;
  }
  if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TOKEN_CACHE_STATE &&
      !h9_segment_entry_retire_token_cache_state_inline(&token_cache_state)) {
    fprintf(stderr, "segment entry bench failed to retire token cache state\n");
    h9_segment_entry_debug_reset();
    return 3;
  }
  if (bench_mode == H9_SEGMENT_ENTRY_BENCH_TLS_TOKEN_CACHE_BODY &&
      !h9_segment_entry_retire_token_cache_state_inline(
          &h9_segment_entry_token_cache_state[class_id])) {
    fprintf(stderr, "segment entry bench failed to retire tls token cache\n");
    h9_segment_entry_debug_reset();
    return 3;
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
