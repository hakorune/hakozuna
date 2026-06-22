#ifndef H8_BENCH_SUPPORT_H
#define H8_BENCH_SUPPORT_H

#include "../src/h8_class_map.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#define H8_BENCH_CANDIDATE_UPPER1536_COUNT 10u
#define H8_BENCH_CANDIDATE_UPPER1P5_COUNT 11u

typedef struct H8BenchOptions {
  int runs;
  int threads;
  size_t iters_per_thread;
  size_t min_size;
  size_t max_size;
  int remote_pct;
  int interleaved;
} H8BenchOptions;

typedef struct H8Inbox {
  void** items;
  size_t count;
  size_t cap;
  _Atomic size_t head;
  _Atomic size_t tail;
} H8Inbox;

typedef struct H8BenchThread {
  int index;
  const H8BenchOptions* opt;
  H8Inbox* inboxes;
  _Atomic int* done;
  pthread_barrier_t* barrier;
  uint32_t rng;
  uint64_t alloc_ns;
  uint64_t remote_ns;
  size_t remote_live_by_class[9];
  uint64_t requested_bytes_by_class[9];
  uint64_t rounded_bytes_by_class[9];
  size_t alloc_count_by_class[9];
  uint64_t rounded_upper1536_bytes;
  uint64_t rounded_upper1p5_bytes;
  size_t remote_live_upper1536[H8_BENCH_CANDIDATE_UPPER1536_COUNT];
  size_t remote_live_upper1p5[H8_BENCH_CANDIDATE_UPPER1P5_COUNT];
  int error;
} H8BenchThread;

typedef struct H8MemorySample {
  size_t rss_bytes;
  size_t hwm_bytes;
} H8MemorySample;

uint32_t h8_rng_next(uint32_t* state);
size_t h8_rand_range(uint32_t* state, size_t lo, size_t hi);
uint32_t h8_bench_note_alloc(H8BenchThread* th, size_t size);
void h8_bench_note_remote_live(H8BenchThread* th, size_t size);
size_t h8_bench_slots_for_class(uint32_t class_id);
size_t h8_bench_upper1536_slots_for_class(uint32_t class_id);
size_t h8_bench_upper1p5_slots_for_class(uint32_t class_id);
uint64_t h8_now_ns(void);
H8MemorySample h8_read_memory_sample(void);
int h8_spsc_push(H8Inbox* inbox, void* ptr);
void* h8_spsc_pop(H8Inbox* inbox);
size_t h8_drain_inbox(H8Inbox* inbox);
int h8_parse_options(int argc, char** argv, H8BenchOptions* opt);
void h8_usage(const char* argv0);
int h8_cmp_double(const void* a, const void* b);
int h8_cmp_size_t(const void* a, const void* b);
double h8_percentile(double* values, size_t n, double p);
size_t h8_percentile_size_t(size_t* values, size_t n, double p);

#endif
