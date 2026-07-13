#include "h8_bench_support.h"

#include "../include/h8.h"

#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint32_t h8_rng_next(uint32_t* state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x ? x : 0xA341316Cu;
  return *state;
}

uint32_t h8_windows_lcg_next(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

size_t h8_rand_range(uint32_t* state, size_t lo, size_t hi) {
  if (hi <= lo) {
    return lo;
  }
  uint32_t span = (uint32_t)(hi - lo + 1u);
  return lo + (size_t)(h8_rng_next(state) % span);
}

size_t h8_bench_slots_for_class(uint32_t class_id) {
  return h8_class_slot_count(class_id);
}

static uint32_t h8_candidate_class_for_size(const uint32_t* sizes, uint32_t count,
                                            size_t size) {
  for (uint32_t i = 0; i < count; ++i) {
    if (size <= sizes[i]) {
      return i;
    }
  }
  return count - 1u;
}

static uint32_t h8_upper1536_size(uint32_t class_id) {
  static const uint32_t sizes[H8_BENCH_CANDIDATE_UPPER1536_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 1536, 2048, 4096};
  return sizes[class_id];
}

static uint32_t h8_upper1p5_size(uint32_t class_id) {
  static const uint32_t sizes[H8_BENCH_CANDIDATE_UPPER1P5_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 1536, 2048, 3072, 4096};
  return sizes[class_id];
}

static uint32_t h8_upper1536_class_for_size(size_t size) {
  static const uint32_t sizes[H8_BENCH_CANDIDATE_UPPER1536_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 1536, 2048, 4096};
  return h8_candidate_class_for_size(sizes, H8_BENCH_CANDIDATE_UPPER1536_COUNT,
                                     size);
}

static uint32_t h8_upper1p5_class_for_size(size_t size) {
  static const uint32_t sizes[H8_BENCH_CANDIDATE_UPPER1P5_COUNT] = {
      16, 32, 64, 128, 256, 512, 1024, 1536, 2048, 3072, 4096};
  return h8_candidate_class_for_size(sizes, H8_BENCH_CANDIDATE_UPPER1P5_COUNT,
                                     size);
}

static uint32_t h8_bench_medium_round_size(size_t size) {
  if (size <= H8_MAX_SMALL_SIZE) {
    return H8_MAX_SMALL_SIZE;
  }
  uint32_t rounded = 8192u;
  while (rounded < size && rounded < H8_BENCH_MEDIUM_MAX_SIZE) {
    rounded <<= 1u;
  }
  return rounded;
}

static uint32_t h8_bench_medium_class_for_size(size_t size) {
  uint32_t rounded = h8_bench_medium_round_size(size);
  uint32_t class_id = 0;
  while ((8192u << class_id) < rounded &&
         class_id + 1u < H8_BENCH_MEDIUM_CLASS_COUNT) {
    ++class_id;
  }
  return class_id;
}

static uint32_t h8_bench_medium_upper48_size(uint32_t class_id) {
  static const uint32_t sizes[H8_BENCH_MEDIUM_UPPER48_COUNT] = {
      8192u, 16384u, 32768u, 49152u, 65536u};
  return sizes[class_id < H8_BENCH_MEDIUM_UPPER48_COUNT
                   ? class_id
                   : H8_BENCH_MEDIUM_UPPER48_COUNT - 1u];
}

static uint32_t h8_bench_medium_upper48_class_for_size(size_t size) {
  static const uint32_t sizes[H8_BENCH_MEDIUM_UPPER48_COUNT] = {
      8192u, 16384u, 32768u, 49152u, 65536u};
  return h8_candidate_class_for_size(sizes, H8_BENCH_MEDIUM_UPPER48_COUNT,
                                     size);
}

static uint32_t h8_bench_medium_v12_size(uint32_t class_id) {
  static const uint32_t sizes[H8_BENCH_MEDIUM_V12_COUNT] = {
      8192u, 16384u, 24576u, 32768u, 49152u, 65536u};
  return sizes[class_id < H8_BENCH_MEDIUM_V12_COUNT
                   ? class_id
                   : H8_BENCH_MEDIUM_V12_COUNT - 1u];
}

static uint32_t h8_bench_medium_v12_class_for_size(size_t size) {
  static const uint32_t sizes[H8_BENCH_MEDIUM_V12_COUNT] = {
      8192u, 16384u, 24576u, 32768u, 49152u, 65536u};
  return h8_candidate_class_for_size(sizes, H8_BENCH_MEDIUM_V12_COUNT, size);
}

size_t h8_bench_upper1536_slots_for_class(uint32_t class_id) {
  return 65536u / h8_upper1536_size(class_id);
}

size_t h8_bench_upper1p5_slots_for_class(uint32_t class_id) {
  return 65536u / h8_upper1p5_size(class_id);
}

uint32_t h8_bench_note_alloc(H8BenchThread* th, size_t size) {
  if (size > H8_MAX_SMALL_SIZE) {
    if (size <= H8_BENCH_MEDIUM_MAX_SIZE) {
      uint32_t medium_class = h8_bench_medium_class_for_size(size);
      uint32_t upper48_class = h8_bench_medium_upper48_class_for_size(size);
      uint32_t v12_class = h8_bench_medium_v12_class_for_size(size);
      th->medium_candidate_count++;
      th->medium_candidate_by_class[medium_class]++;
      th->medium_candidate_upper48_by_class[upper48_class]++;
      th->medium_candidate_v12_by_class[v12_class]++;
      th->medium_candidate_requested_bytes += (uint64_t)size;
      th->medium_candidate_rounded_bytes +=
          (uint64_t)h8_bench_medium_round_size(size);
      th->medium_candidate_upper48_bytes +=
          (uint64_t)h8_bench_medium_upper48_size(upper48_class);
      th->medium_candidate_v12_bytes +=
          (uint64_t)h8_bench_medium_v12_size(v12_class);
    }
    return H8_CLASS_COUNT;
  }
  uint32_t class_id = h8_class_for_size(size);
  th->alloc_count_by_class[class_id]++;
  th->requested_bytes_by_class[class_id] += (uint64_t)size;
  th->rounded_bytes_by_class[class_id] += (uint64_t)h8_class_size(class_id);
  th->rounded_upper1536_bytes +=
      (uint64_t)h8_upper1536_size(h8_upper1536_class_for_size(size));
  th->rounded_upper1p5_bytes +=
      (uint64_t)h8_upper1p5_size(h8_upper1p5_class_for_size(size));
  return class_id;
}

void h8_bench_note_remote_live(H8BenchThread* th, size_t size) {
  if (size > H8_MAX_SMALL_SIZE) {
    if (size <= H8_BENCH_MEDIUM_MAX_SIZE) {
      uint32_t medium_class = h8_bench_medium_class_for_size(size);
      uint32_t upper48_class = h8_bench_medium_upper48_class_for_size(size);
      uint32_t v12_class = h8_bench_medium_v12_class_for_size(size);
      th->medium_remote_live_count++;
      th->medium_remote_live_by_class[medium_class]++;
      th->medium_remote_live_upper48_by_class[upper48_class]++;
      th->medium_remote_live_v12_by_class[v12_class]++;
      th->medium_remote_live_requested_bytes += (uint64_t)size;
      th->medium_remote_live_rounded_bytes +=
          (uint64_t)h8_bench_medium_round_size(size);
      th->medium_remote_live_upper48_bytes +=
          (uint64_t)h8_bench_medium_upper48_size(upper48_class);
      th->medium_remote_live_v12_bytes +=
          (uint64_t)h8_bench_medium_v12_size(v12_class);
    }
    return;
  }
  uint32_t p2_class = h8_class_for_size(size);
  uint32_t upper1536_class = h8_upper1536_class_for_size(size);
  uint32_t upper1p5_class = h8_upper1p5_class_for_size(size);
  th->remote_live_by_class[p2_class]++;
  th->remote_live_upper1536[upper1536_class]++;
  th->remote_live_upper1p5[upper1p5_class]++;
  th->remote_live_rounded_bytes += (uint64_t)h8_class_size(p2_class);
  th->remote_live_upper1536_bytes +=
      (uint64_t)h8_upper1536_size(upper1536_class);
  th->remote_live_upper1p5_bytes +=
      (uint64_t)h8_upper1p5_size(upper1p5_class);
}

void h8_bench_medium_totals_add(H8BenchMediumTotals* totals,
                                const H8BenchThread* threads, int count) {
  for (int i = 0; i < count; ++i) {
    const H8BenchThread* th = &threads[i];
    totals->candidate_count += th->medium_candidate_count;
    totals->remote_live_count += th->medium_remote_live_count;
    totals->requested_bytes += th->medium_candidate_requested_bytes;
    totals->rounded_bytes += th->medium_candidate_rounded_bytes;
    totals->upper48_rounded_bytes += th->medium_candidate_upper48_bytes;
    totals->v12_rounded_bytes += th->medium_candidate_v12_bytes;
    totals->remote_requested_bytes += th->medium_remote_live_requested_bytes;
    totals->remote_rounded_bytes += th->medium_remote_live_rounded_bytes;
    totals->remote_upper48_rounded_bytes +=
        th->medium_remote_live_upper48_bytes;
    totals->remote_v12_rounded_bytes += th->medium_remote_live_v12_bytes;
    for (uint32_t c = 0; c < H8_BENCH_MEDIUM_CLASS_COUNT; ++c) {
      totals->candidate_by_class[c] += th->medium_candidate_by_class[c];
      totals->remote_live_by_class[c] += th->medium_remote_live_by_class[c];
    }
    for (uint32_t c = 0; c < H8_BENCH_MEDIUM_UPPER48_COUNT; ++c) {
      totals->candidate_upper48_by_class[c] +=
          th->medium_candidate_upper48_by_class[c];
      totals->remote_live_upper48_by_class[c] +=
          th->medium_remote_live_upper48_by_class[c];
    }
    for (uint32_t c = 0; c < H8_BENCH_MEDIUM_V12_COUNT; ++c) {
      totals->candidate_v12_by_class[c] += th->medium_candidate_v12_by_class[c];
      totals->remote_live_v12_by_class[c] +=
          th->medium_remote_live_v12_by_class[c];
    }
  }
}

void h8_bench_print_medium_totals(const H8BenchMediumTotals* totals) {
  printf("medium_route_shadow candidate_count=%zu remote_live_count=%zu requested_bytes=%" PRIu64 " rounded_bytes=%" PRIu64 " remote_requested_bytes=%" PRIu64 " remote_rounded_bytes=%" PRIu64 "\n",
         totals->candidate_count, totals->remote_live_count,
         totals->requested_bytes, totals->rounded_bytes,
         totals->remote_requested_bytes, totals->remote_rounded_bytes);

  static const size_t medium_slots[H8_BENCH_MEDIUM_CLASS_COUNT] = {8, 4, 2, 1};
  static const size_t medium_slots_64k_x2[H8_BENCH_MEDIUM_CLASS_COUNT] = {
      8, 4, 2, 2};
  static const size_t upper48_slots[H8_BENCH_MEDIUM_UPPER48_COUNT] = {
      8, 4, 2, 1, 1};
  static const size_t v12_slots[H8_BENCH_MEDIUM_V12_COUNT] = {
      8, 4, 2, 2, 1, 2};
  static const size_t v12_48k2_slots[H8_BENCH_MEDIUM_V12_COUNT] = {
      8, 4, 2, 2, 2, 2};
  size_t medium_mix_runs = 0;
  size_t medium_mix_runs_64k_x2 = 0;
  size_t upper48_runs = 0;
  size_t v12_runs = 0;
  size_t v12_48k2_runs = 0;
  for (uint32_t c = 0; c < H8_BENCH_MEDIUM_CLASS_COUNT; ++c) {
    medium_mix_runs +=
        (totals->remote_live_by_class[c] + medium_slots[c] - 1u) /
        medium_slots[c];
    medium_mix_runs_64k_x2 +=
        (totals->remote_live_by_class[c] + medium_slots_64k_x2[c] - 1u) /
        medium_slots_64k_x2[c];
  }
  for (uint32_t c = 0; c < H8_BENCH_MEDIUM_UPPER48_COUNT; ++c) {
    upper48_runs +=
        (totals->remote_live_upper48_by_class[c] + upper48_slots[c] - 1u) /
        upper48_slots[c];
  }
  for (uint32_t c = 0; c < H8_BENCH_MEDIUM_V12_COUNT; ++c) {
    v12_runs +=
        (totals->remote_live_v12_by_class[c] + v12_slots[c] - 1u) /
        v12_slots[c];
    v12_48k2_runs +=
        (totals->remote_live_v12_by_class[c] + v12_48k2_slots[c] - 1u) /
        v12_48k2_slots[c];
  }
  printf("medium_class_dist alloc=[%zu,%zu,%zu,%zu] remote_live=[%zu,%zu,%zu,%zu] one_slot_alloc_ratio=%.6f one_slot_remote_ratio=%.6f\n",
         totals->candidate_by_class[0], totals->candidate_by_class[1],
         totals->candidate_by_class[2], totals->candidate_by_class[3],
         totals->remote_live_by_class[0], totals->remote_live_by_class[1],
         totals->remote_live_by_class[2], totals->remote_live_by_class[3],
         totals->candidate_count
             ? (double)totals->candidate_by_class[3] /
                   (double)totals->candidate_count
             : 0.0,
         totals->remote_live_count
             ? (double)totals->remote_live_by_class[3] /
                   (double)totals->remote_live_count
             : 0.0);
  printf("medium_upper48_shadow alloc=[%zu,%zu,%zu,%zu,%zu] remote_live=[%zu,%zu,%zu,%zu,%zu] rounded_bytes=%" PRIu64 " rounded_ratio=%.6f remote_rounded_bytes=%" PRIu64 " remote_ratio=%.6f\n",
         totals->candidate_upper48_by_class[0],
         totals->candidate_upper48_by_class[1],
         totals->candidate_upper48_by_class[2],
         totals->candidate_upper48_by_class[3],
         totals->candidate_upper48_by_class[4],
         totals->remote_live_upper48_by_class[0],
         totals->remote_live_upper48_by_class[1],
         totals->remote_live_upper48_by_class[2],
         totals->remote_live_upper48_by_class[3],
         totals->remote_live_upper48_by_class[4],
         totals->upper48_rounded_bytes,
         totals->requested_bytes
             ? (double)totals->upper48_rounded_bytes /
                   (double)totals->requested_bytes
             : 0.0,
         totals->remote_upper48_rounded_bytes,
         totals->remote_requested_bytes
             ? (double)totals->remote_upper48_rounded_bytes /
                   (double)totals->remote_requested_bytes
             : 0.0);
  printf("medium_v12_sizepolicy_shadow policy=8/16/24/32/48/64 alloc=[%zu,%zu,%zu,%zu,%zu,%zu] remote_live=[%zu,%zu,%zu,%zu,%zu,%zu] rounded_bytes=%" PRIu64 " rounded_ratio=%.6f remote_rounded_bytes=%" PRIu64 " remote_ratio=%.6f remote_runs=%zu run_ratio_vs_one_slot64k=%.6f run_ratio_vs_default64k2=%.6f remote_runs_48k2=%zu run48k2_ratio_vs_default64k2=%.6f\n",
         totals->candidate_v12_by_class[0], totals->candidate_v12_by_class[1],
         totals->candidate_v12_by_class[2], totals->candidate_v12_by_class[3],
         totals->candidate_v12_by_class[4], totals->candidate_v12_by_class[5],
         totals->remote_live_v12_by_class[0],
         totals->remote_live_v12_by_class[1],
         totals->remote_live_v12_by_class[2],
         totals->remote_live_v12_by_class[3],
         totals->remote_live_v12_by_class[4],
         totals->remote_live_v12_by_class[5], totals->v12_rounded_bytes,
         totals->requested_bytes
             ? (double)totals->v12_rounded_bytes /
                   (double)totals->requested_bytes
             : 0.0,
         totals->remote_v12_rounded_bytes,
         totals->remote_requested_bytes
             ? (double)totals->remote_v12_rounded_bytes /
                   (double)totals->remote_requested_bytes
             : 0.0,
         v12_runs,
         medium_mix_runs ? (double)v12_runs / (double)medium_mix_runs : 0.0,
         medium_mix_runs_64k_x2
             ? (double)v12_runs / (double)medium_mix_runs_64k_x2
             : 0.0,
         v12_48k2_runs,
         medium_mix_runs_64k_x2
             ? (double)v12_48k2_runs / (double)medium_mix_runs_64k_x2
             : 0.0);
  printf("medium_run_mix_est one_slot64k_runs=%zu default64k2_runs=%zu upper48_runs=%zu v12_runs=%zu v12_48k2_runs=%zu default64k2_vs_one_slot_ratio=%.6f upper48_vs_one_slot_ratio=%.6f v12_vs_one_slot_ratio=%.6f v12_vs_default64k2_ratio=%.6f v12_48k2_vs_default64k2_ratio=%.6f\n",
         medium_mix_runs, medium_mix_runs_64k_x2, upper48_runs, v12_runs,
         v12_48k2_runs,
         medium_mix_runs ? (double)medium_mix_runs_64k_x2 /
                               (double)medium_mix_runs
                         : 0.0,
         medium_mix_runs ? (double)upper48_runs / (double)medium_mix_runs
                         : 0.0,
         medium_mix_runs ? (double)v12_runs / (double)medium_mix_runs
                         : 0.0,
         medium_mix_runs_64k_x2
             ? (double)v12_runs / (double)medium_mix_runs_64k_x2
             : 0.0,
         medium_mix_runs_64k_x2
             ? (double)v12_48k2_runs / (double)medium_mix_runs_64k_x2
             : 0.0);
}

uint64_t h8_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

H8MemorySample h8_read_memory_sample(void) {
  H8MemorySample sample = {0, 0};
  FILE* f = fopen("/proc/self/status", "r");
  if (!f) {
    return sample;
  }
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    size_t kib = 0;
    if (sscanf(line, "VmRSS: %zu kB", &kib) == 1) {
      sample.rss_bytes = kib * 1024u;
    } else if (sscanf(line, "VmHWM: %zu kB", &kib) == 1) {
      sample.hwm_bytes = kib * 1024u;
    }
  }
  fclose(f);
  return sample;
}

int h8_spsc_push(H8Inbox* inbox, void* ptr) {
  size_t tail = atomic_load_explicit(&inbox->tail, memory_order_relaxed);
  size_t head = atomic_load_explicit(&inbox->head, memory_order_acquire);
  size_t next = tail + 1u;
  if (next - head > inbox->cap) {
    return 0;
  }
  inbox->items[tail % inbox->cap] = ptr;
  atomic_store_explicit(&inbox->tail, next, memory_order_release);
  return 1;
}

void* h8_spsc_pop(H8Inbox* inbox) {
  size_t head = atomic_load_explicit(&inbox->head, memory_order_relaxed);
  size_t tail = atomic_load_explicit(&inbox->tail, memory_order_acquire);
  if (head == tail) {
    return NULL;
  }
  void* ptr = inbox->items[head % inbox->cap];
  atomic_store_explicit(&inbox->head, head + 1u, memory_order_release);
  return ptr;
}

size_t h8_drain_inbox(H8Inbox* inbox) {
  size_t drained = 0;
  for (;;) {
    void* ptr = h8_spsc_pop(inbox);
    if (!ptr) {
      return drained;
    }
    h8_free(ptr);
    ++drained;
  }
}

static int h8_parse_int(const char* s, int* out) {
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if (!s || end == s || *end != '\0' || v < 0 || v > INT32_MAX) {
    return -1;
  }
  *out = (int)v;
  return 0;
}

void h8_usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [--runs N] [--threads N] [--iters N]\n"
          "          [--min-size N] [--max-size N] [--remote-pct N]\n"
          "          [--interleaved 0|1] [--working-set-ring 0|1]\n"
          "          [--working-set-lcg 0|1] [--live-window N]\n",
          argv0);
}

int h8_parse_options(int argc, char** argv, H8BenchOptions* opt) {
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (strcmp(a, "--runs") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->runs) != 0) return -1;
    } else if (strcmp(a, "--threads") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->threads) != 0) return -1;
    } else if (strcmp(a, "--iters") == 0 && i + 1 < argc) {
      int tmp = 0;
      if (h8_parse_int(argv[++i], &tmp) != 0) return -1;
      opt->iters_per_thread = (size_t)tmp;
    } else if (strcmp(a, "--min-size") == 0 && i + 1 < argc) {
      int tmp = 0;
      if (h8_parse_int(argv[++i], &tmp) != 0) return -1;
      opt->min_size = (size_t)tmp;
    } else if (strcmp(a, "--max-size") == 0 && i + 1 < argc) {
      int tmp = 0;
      if (h8_parse_int(argv[++i], &tmp) != 0) return -1;
      opt->max_size = (size_t)tmp;
    } else if (strcmp(a, "--remote-pct") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->remote_pct) != 0) return -1;
    } else if (strcmp(a, "--interleaved") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->interleaved) != 0) return -1;
    } else if (strcmp(a, "--working-set-ring") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->working_set_ring) != 0) return -1;
    } else if (strcmp(a, "--working-set-lcg") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->working_set_lcg) != 0) return -1;
    } else if (strcmp(a, "--live-window") == 0 && i + 1 < argc) {
      int tmp = 0;
      if (h8_parse_int(argv[++i], &tmp) != 0) return -1;
      opt->live_window = (size_t)tmp;
    } else {
      return -1;
    }
  }
  return 0;
}

int h8_cmp_double(const void* a, const void* b) {
  double da = *(const double*)a;
  double db = *(const double*)b;
  return (da > db) - (da < db);
}

int h8_cmp_size_t(const void* a, const void* b) {
  size_t da = *(const size_t*)a;
  size_t db = *(const size_t*)b;
  return (da > db) - (da < db);
}

double h8_percentile(double* values, size_t n, double p) {
  if (n == 0) {
    return 0.0;
  }
  size_t idx = (size_t)((double)(n - 1u) * p + 0.5);
  return values[idx < n ? idx : n - 1u];
}

size_t h8_percentile_size_t(size_t* values, size_t n, double p) {
  if (n == 0) {
    return 0;
  }
  size_t idx = (size_t)((double)(n - 1u) * p + 0.5);
  return values[idx < n ? idx : n - 1u];
}
