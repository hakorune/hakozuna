#include "h8_bench_support.h"

#include "../include/h8.h"

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

size_t h8_bench_upper1536_slots_for_class(uint32_t class_id) {
  return 65536u / h8_upper1536_size(class_id);
}

size_t h8_bench_upper1p5_slots_for_class(uint32_t class_id) {
  return 65536u / h8_upper1p5_size(class_id);
}

uint32_t h8_bench_note_alloc(H8BenchThread* th, size_t size) {
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
  th->remote_live_by_class[h8_class_for_size(size)]++;
  th->remote_live_upper1536[h8_upper1536_class_for_size(size)]++;
  th->remote_live_upper1p5[h8_upper1p5_class_for_size(size)]++;
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
          "          [--interleaved 0|1]\n",
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
