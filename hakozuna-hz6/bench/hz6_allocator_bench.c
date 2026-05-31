#include "hz6_allocator.h"
#include "hz6_allocator_api_init.h"
#include "hz6_allocator_api_owner.h"
#include "hz6_profiles.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <time.h>
#endif

typedef enum Hz6BenchMode {
  HZ6_BENCH_LOCAL = 0,
  HZ6_BENCH_REMOTE = 1,
  HZ6_BENCH_REUSE = 2,
} Hz6BenchMode;

static double now_sec(void) {
#if defined(_WIN32)
  LARGE_INTEGER freq;
  LARGE_INTEGER counter;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / (double)freq.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

static const char* mode_name(Hz6BenchMode mode) {
  switch (mode) {
    case HZ6_BENCH_LOCAL:
      return "local";
    case HZ6_BENCH_REMOTE:
      return "remote";
    case HZ6_BENCH_REUSE:
      return "reuse";
    default:
      return "unknown";
  }
}

static const char* profile_name(Hz6ProfileId profile) {
  switch (profile) {
    case HZ6_PROFILE_STRICT:
      return "strict";
    case HZ6_PROFILE_SPEED:
      return "speed";
    case HZ6_PROFILE_RSS:
      return "rss";
    case HZ6_PROFILE_REMOTE:
      return "remote";
    default:
      return "unknown";
  }
}

#if HZ6_DIAGNOSTIC_PROBES
static const char* front_name(size_t index) {
  switch (index) {
    case HZ6_FRONT_ATTR_LOCAL2P:
      return "local2p";
    case HZ6_FRONT_ATTR_MIDPAGE:
      return "midpage";
    case HZ6_FRONT_ATTR_LARGE:
      return "large";
    case HZ6_FRONT_ATTR_TOY:
      return "toy";
    default:
      return "unknown";
  }
}

static void print_front_prefill_stats(const Hz6Allocator* allocator) {
  size_t front;
  for (front = 0; front < HZ6_FRONT_ATTR_COUNT; ++front) {
    printf("[HZ6_PREFILL] front=%s attempt=%zu filled=%zu fallback=%zu\n",
           front_name(front),
           allocator->stats.front_source_prefill_attempt[front],
           allocator->stats.front_source_prefill_filled[front],
           allocator->stats.front_source_prefill_fallback[front]);
  }
}
#endif

static int parse_mode(const char* value, Hz6BenchMode* mode) {
  if (!value || !mode) {
    return 0;
  }
  if (strcmp(value, "local") == 0) {
    *mode = HZ6_BENCH_LOCAL;
    return 1;
  }
  if (strcmp(value, "remote") == 0) {
    *mode = HZ6_BENCH_REMOTE;
    return 1;
  }
  if (strcmp(value, "reuse") == 0) {
    *mode = HZ6_BENCH_REUSE;
    return 1;
  }
  return 0;
}

static int parse_profile(const char* value, Hz6ProfileId* profile) {
  if (!value || !profile) {
    return 0;
  }
  if (strcmp(value, "strict") == 0) {
    *profile = HZ6_PROFILE_STRICT;
    return 1;
  }
  if (strcmp(value, "speed") == 0) {
    *profile = HZ6_PROFILE_SPEED;
    return 1;
  }
  if (strcmp(value, "rss") == 0) {
    *profile = HZ6_PROFILE_RSS;
    return 1;
  }
  if (strcmp(value, "remote") == 0) {
    *profile = HZ6_PROFILE_REMOTE;
    return 1;
  }
  return 0;
}

static void touch_payload(void* ptr, size_t size) {
  if (!ptr || size == 0) {
    return;
  }
  volatile unsigned char* bytes = (volatile unsigned char*)ptr;
  bytes[0] ^= 0x5au;
  bytes[size / 2u] ^= 0xa5u;
  bytes[size - 1u] ^= 0x3cu;
}

static void print_stats(const Hz6Allocator* allocator) {
  Hz6StatsSnapshot stats = hz6_stats_snapshot(allocator);
  printf("[HZ6_STATS] route_valid=%zu route_invalid=%zu route_miss=%zu "
         "transfer_push=%zu transfer_pop=%zu source_alloc=%zu "
#if HZ6_DIAGNOSTIC_PROBES
         "frontcache_reuse_hit=%zu frontcache_reuse_invalid=%zu "
         "transfer_reuse_hit=%zu transfer_reuse_invalid=%zu "
         "source_refill_starvation=%zu source_refill_saturation=%zu "
         "source_refill_boost=%zu source_refill_clamp=%zu "
         "source_admission_open=%zu source_admission_boosted=%zu "
         "source_admission_clamped=%zu "
         "source_prefill_attempt=%zu source_prefill_filled=%zu "
         "source_prefill_fallback=%zu front_source_ops_alloc=%zu "
         "front_source_slot_alloc=%zu front_source_prefill_alloc=%zu "
         "toy_source_prefill_call=%zu "
         "local2p_source_alloc=%zu "
         "midpage_source_alloc=%zu large_source_alloc=%zu toy_source_alloc=%zu "
#endif
         "alloc_fail=%zu descriptor_exhausted=%zu route_register_fail=%zu "
         "source_block_exhausted=%zu descriptor_probe_total=%zu "
         "descriptor_probe_max=%zu route_lookup_probe_total=%zu "
         "route_lookup_probe_max=%zu route_register_probe_total=%zu "
         "route_register_probe_max=%zu route_unregister_probe_total=%zu "
         "route_unregister_probe_max=%zu source_block_probe_total=%zu "
         "source_block_probe_max=%zu "
         "large_span_central_push=%zu large_span_central_pop=%zu "
         "large_span_source_alloc=%zu\n",
         stats.route_valid, stats.route_invalid, stats.route_miss,
         stats.transfer_push, stats.transfer_pop, stats.source_alloc,
#if HZ6_DIAGNOSTIC_PROBES
         stats.frontcache_reuse_hit, stats.frontcache_reuse_invalid,
         stats.transfer_reuse_hit, stats.transfer_reuse_invalid,
         stats.source_refill_starvation, stats.source_refill_saturation,
         stats.source_refill_boost, stats.source_refill_clamp,
         stats.source_admission_open, stats.source_admission_boosted,
         stats.source_admission_clamped,
         stats.source_prefill_attempt, stats.source_prefill_filled,
         stats.source_prefill_fallback, stats.front_source_ops_alloc,
         stats.front_source_slot_alloc, stats.front_source_prefill_alloc,
         stats.toy_source_prefill_call,
         stats.local2p_source_alloc,
         stats.midpage_source_alloc, stats.large_source_alloc,
         stats.toy_source_alloc,
#endif
         stats.alloc_fail, stats.descriptor_exhausted,
         stats.route_register_fail, stats.source_block_exhausted,
         stats.descriptor_probe_total, stats.descriptor_probe_max,
         stats.route_lookup_probe_total, stats.route_lookup_probe_max,
         stats.route_register_probe_total, stats.route_register_probe_max,
         stats.route_unregister_probe_total, stats.route_unregister_probe_max,
         stats.source_block_probe_total, stats.source_block_probe_max,
         stats.large_span_central_push, stats.large_span_central_pop,
         stats.large_span_source_alloc);
#if HZ6_DIAGNOSTIC_PROBES
  print_front_prefill_stats(allocator);
#endif
}

static int run_local(Hz6ProfileId profile, uint64_t iters, size_t size) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, profile);
  if (!hz6_allocator_debug_set_owner_slot(&allocator, 0)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&allocator);
    return 4;
  }

  uint64_t ops = 0;
  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = hz6_malloc(&allocator, size);
    if (!ptr) {
      fprintf(stderr, "local malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 5;
    }
    touch_payload(ptr, size);
    hz6_free(&allocator, ptr);
    ops += 2u;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=0\n",
         profile_name(profile), mode_name(HZ6_BENCH_LOCAL),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed);
  print_stats(&allocator);
  hz6_allocator_destroy(&allocator);
  return 0;
}

static int run_remote(Hz6ProfileId profile, uint64_t iters, size_t size) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, profile);
  if (!hz6_allocator_debug_set_owner_slot(&allocator, 0)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&allocator);
    return 4;
  }

  uint64_t ops = 0;
  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    void* ptr = hz6_malloc(&allocator, size);
    if (!ptr) {
      fprintf(stderr, "remote malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 5;
    }
    touch_payload(ptr, size);
    if (!hz6_allocator_debug_set_owner_slot(&allocator, 1)) {
      fprintf(stderr, "remote owner slot switch failed\n");
      hz6_allocator_destroy(&allocator);
      return 6;
    }
    if (!hz6_free_remote(&allocator, ptr)) {
      fprintf(stderr, "remote free failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 7;
    }
    if (!hz6_allocator_debug_set_owner_slot(&allocator, 0)) {
      fprintf(stderr, "remote owner restore failed\n");
      hz6_allocator_destroy(&allocator);
      return 8;
    }
    ops += 2u;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=0\n",
         profile_name(profile), mode_name(HZ6_BENCH_REMOTE),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed);
  print_stats(&allocator);
  hz6_allocator_destroy(&allocator);
  return 0;
}

static int run_reuse(Hz6ProfileId profile, uint64_t iters, size_t size) {
  Hz6Allocator allocator;
  hz6_allocator_init_with_profile(&allocator, profile);
  if (!hz6_allocator_debug_set_owner_slot(&allocator, 0)) {
    fprintf(stderr, "owner slot setup failed\n");
    hz6_allocator_destroy(&allocator);
    return 4;
  }

  uint64_t ops = 0;
  uint64_t reuse_hits = 0;
  double start = now_sec();
  for (uint64_t i = 0; i < iters; ++i) {
    void* first = hz6_malloc(&allocator, size);
    if (!first) {
      fprintf(stderr, "reuse malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 5;
    }
    touch_payload(first, size);
    if (!hz6_allocator_debug_set_owner_slot(&allocator, 1)) {
      fprintf(stderr, "reuse owner slot switch failed\n");
      hz6_allocator_destroy(&allocator);
      return 6;
    }
    if (!hz6_free_remote(&allocator, first)) {
      fprintf(stderr, "reuse remote free failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 7;
    }
    if (!hz6_allocator_debug_set_owner_slot(&allocator, 0)) {
      fprintf(stderr, "reuse owner restore failed\n");
      hz6_allocator_destroy(&allocator);
      return 8;
    }
    void* second = hz6_malloc(&allocator, size);
    if (!second) {
      fprintf(stderr, "reuse second malloc failed iter=%llu size=%zu\n",
              (unsigned long long)i, size);
      hz6_allocator_destroy(&allocator);
      return 9;
    }
    if (second == first) {
      ++reuse_hits;
    }
    touch_payload(second, size);
    hz6_free(&allocator, second);
    ops += 4u;
  }
  double elapsed = now_sec() - start;
  printf("allocator=hz6 profile=%s mode=%s iters=%llu size=%zu ops=%llu "
         "time=%.6f ops/s=%.3f reuse_hits=%llu\n",
         profile_name(profile), mode_name(HZ6_BENCH_REUSE),
         (unsigned long long)iters, size, (unsigned long long)ops, elapsed,
         (double)ops / elapsed, (unsigned long long)reuse_hits);
  print_stats(&allocator);
  hz6_allocator_destroy(&allocator);
  return 0;
}

static void usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [mode] [profile] [iters] [size]\n"
          "  mode: local | remote | reuse\n"
          "  profile: strict | speed | rss | remote\n"
          "  iters: iteration count\n"
          "  size: allocation size in bytes\n",
          argv0);
}

int main(int argc, char** argv) {
  Hz6BenchMode mode = HZ6_BENCH_LOCAL;
  Hz6ProfileId profile = HZ6_PROFILE_STRICT;
  uint64_t iters = 1000000u;
  size_t size = 65536u;

  if (argc > 1 && !parse_mode(argv[1], &mode)) {
    usage(argv[0]);
    return 2;
  }
  if (argc > 2 && !parse_profile(argv[2], &profile)) {
    usage(argv[0]);
    return 2;
  }
  if (argc > 3) {
    iters = strtoull(argv[3], NULL, 10);
  }
  if (argc > 4) {
    size = (size_t)strtoull(argv[4], NULL, 10);
  }
  if (iters == 0 || size == 0) {
    usage(argv[0]);
    return 2;
  }

  switch (mode) {
    case HZ6_BENCH_LOCAL:
      return run_local(profile, iters, size);
    case HZ6_BENCH_REMOTE:
      return run_remote(profile, iters, size);
    case HZ6_BENCH_REUSE:
      return run_reuse(profile, iters, size);
    default:
      usage(argv[0]);
      return 2;
  }
}
