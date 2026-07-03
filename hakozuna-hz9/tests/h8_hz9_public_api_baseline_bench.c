#include "../src/h8_internal.h"
#include "../src/h8_hz9_local_slab_route_boundary.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
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

int main(void) {
  const char* mode = getenv("MODE");
  if (!mode || !*mode) {
    mode = "hz8public";
  }
  bool hz9_current = strcmp(mode, "hz9current") == 0 ||
                     strcmp(mode, "hz9currentnonlifo") == 0;
  bool hz9_nosync = strcmp(mode, "hz9nosync") == 0 ||
                    strcmp(mode, "hz9nosyncnonlifo") == 0;
  bool non_lifo = strcmp(mode, "nonlifo") == 0 ||
                  strcmp(mode, "hz8publicnonlifo") == 0 ||
                  strcmp(mode, "hz9currentnonlifo") == 0 ||
                  strcmp(mode, "hz9nosyncnonlifo") == 0;
  uint32_t class_id = (uint32_t)env_u64("CLASS_ID", 5u);
  uint64_t iters = env_u64("ITERS", 30000000ull);
  bool touch = env_u64("TOUCH", 1u) != 0u;
  size_t size = (size_t)env_u64("SIZE", 0u);
  if (size == 0u) {
    const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
    if (!spec || spec->slot_size == 0u) {
      fprintf(stderr, "hz8 public baseline invalid class %u\n", class_id);
      return 2;
    }
    size = spec->slot_size;
  }

  uint64_t ok = 0u;
  uintptr_t sink = 0u;
  if (hz9_current || hz9_nosync) {
    h9_lsp_debug_public_entry_reset();
  }
  double start = now_seconds();
  for (uint64_t i = 0; i < iters; ++i) {
    void* a = (hz9_current || hz9_nosync)
                  ? h9_lsp_debug_public_nosync_malloc(size)
                  : h8_malloc(size);
    if (!a) {
      fprintf(stderr, "public malloc failed at iter %llu mode=%s\n",
              (unsigned long long)i, mode);
      return 3;
    }
    if (touch) {
      volatile unsigned char* p = (volatile unsigned char*)a;
      p[0] = (unsigned char)i;
      p[size - 1u] = (unsigned char)(i >> 8);
    }
    if (non_lifo) {
      void* b = (hz9_current || hz9_nosync)
                    ? h9_lsp_debug_public_nosync_malloc(size)
                    : h8_malloc(size);
      if (!b) {
        fprintf(stderr, "public malloc b failed at iter %llu mode=%s\n",
                (unsigned long long)i, mode);
        return 4;
      }
      if (hz9_current) {
        if (!h9_lsp_debug_public_current_free(a) ||
            !h9_lsp_debug_public_current_free(b)) {
          return 5;
        }
      } else if (hz9_nosync) {
        bool owned_a = false;
        bool owned_b = false;
        if (!h9_lsp_debug_public_nosync_free(a, &owned_a) || !owned_a ||
            !h9_lsp_debug_public_nosync_free(b, &owned_b) || !owned_b) {
          return 6;
        }
      } else {
        h8_free(a);
        h8_free(b);
      }
      sink ^= (uintptr_t)a ^ (uintptr_t)b;
      ok += 2u;
    } else {
      if (hz9_current) {
        if (!h9_lsp_debug_public_current_free(a)) {
          return 7;
        }
      } else if (hz9_nosync) {
        bool owned = false;
        if (!h9_lsp_debug_public_nosync_free(a, &owned) || !owned) {
          return 8;
        }
      } else {
        h8_free(a);
      }
      sink ^= (uintptr_t)a;
      ok++;
    }
  }
  double elapsed = now_seconds() - start;
  if (elapsed <= 0.0) {
    elapsed = 1e-9;
  }
  printf("mode=%s class=%u size=%zu iters=%llu touch=%u "
         "ops_per_s=%.2f elapsed=%.6f sink=%" PRIuPTR "\n",
         mode, class_id, size, (unsigned long long)iters, touch ? 1u : 0u,
         (double)ok / elapsed, elapsed, sink);
  return 0;
}
