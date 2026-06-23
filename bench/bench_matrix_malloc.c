#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

typedef struct Options {
  int runs;
  int threads;
  size_t iters;
  size_t min_size;
  size_t max_size;
  int remote_pct;
  int interleaved;
  size_t live_window;
} Options;

typedef struct Inbox {
  void** items;
  size_t count;
  size_t cap;
  _Atomic size_t head;
  _Atomic size_t tail;
} Inbox;

typedef struct ThreadState {
  int index;
  const Options* opt;
  Inbox* inboxes;
  _Atomic int* done;
  pthread_barrier_t* barrier;
  uint32_t rng;
  uint64_t work_ns;
  uint64_t tail_ns;
  size_t remote_enqueue;
  size_t local_free;
  size_t drain_calls;
  size_t drain_objects;
  size_t drain_empty;
  size_t push_yields;
  size_t finish_yields;
  size_t remote_live_objects;
  uint64_t remote_live_rounded;
  int error;
} ThreadState;

typedef struct MemorySample {
  size_t rss;
  size_t hwm;
} MemorySample;

static uint32_t rng_next(uint32_t* state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x ? x : 0xA341316Cu;
  return *state;
}

static size_t rand_range(uint32_t* state, size_t lo, size_t hi) {
  if (hi <= lo) return lo;
  return lo + (size_t)(rng_next(state) % (uint32_t)(hi - lo + 1u));
}

static size_t p2_round_size(size_t size) {
  size_t out = 16u;
  while (out < size && out < 4096u) out <<= 1u;
  return out;
}

static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static MemorySample read_memory(void) {
  MemorySample sample = {0, 0};
  FILE* f = fopen("/proc/self/status", "r");
  if (!f) return sample;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    size_t kib = 0;
    if (sscanf(line, "VmRSS: %zu kB", &kib) == 1) {
      sample.rss = kib * 1024u;
    } else if (sscanf(line, "VmHWM: %zu kB", &kib) == 1) {
      sample.hwm = kib * 1024u;
    }
  }
  fclose(f);
  return sample;
}

static int inbox_push(Inbox* inbox, void* ptr) {
  size_t tail = atomic_load_explicit(&inbox->tail, memory_order_relaxed);
  size_t head = atomic_load_explicit(&inbox->head, memory_order_acquire);
  size_t next = tail + 1u;
  if (next - head > inbox->cap) return 0;
  inbox->items[tail % inbox->cap] = ptr;
  atomic_store_explicit(&inbox->tail, next, memory_order_release);
  return 1;
}

static void* inbox_pop(Inbox* inbox) {
  size_t head = atomic_load_explicit(&inbox->head, memory_order_relaxed);
  size_t tail = atomic_load_explicit(&inbox->tail, memory_order_acquire);
  if (head == tail) return NULL;
  void* ptr = inbox->items[head % inbox->cap];
  atomic_store_explicit(&inbox->head, head + 1u, memory_order_release);
  return ptr;
}

static size_t drain_inbox(Inbox* inbox) {
  size_t drained = 0;
  for (;;) {
    void* ptr = inbox_pop(inbox);
    if (!ptr) return drained;
    free(ptr);
    ++drained;
  }
}

static void touch_payload(void* ptr, size_t size) {
  ((volatile unsigned char*)ptr)[0] = (unsigned char)size;
  if (size > 1) {
    ((volatile unsigned char*)ptr)[size - 1] = (unsigned char)(size >> 8);
  }
}

static void* thread_interleaved(void* arg) {
  ThreadState* th = (ThreadState*)arg;
  const Options* opt = th->opt;
  int next = (th->index + 1) % opt->threads;
  int prev = (th->index + opt->threads - 1) % opt->threads;
  Inbox* next_inbox = &th->inboxes[next];
  Inbox* my_inbox = &th->inboxes[th->index];

  uint64_t start = now_ns();
  for (size_t i = 0; i < opt->iters; ++i) {
    ++th->drain_calls;
    size_t drained = drain_inbox(my_inbox);
    th->drain_objects += drained;
    if (drained == 0) ++th->drain_empty;

    size_t size = rand_range(&th->rng, opt->min_size, opt->max_size);
    void* ptr = malloc(size);
    if (!ptr) {
      th->error = 1;
      break;
    }
    touch_payload(ptr, size);
    if (opt->remote_pct > 0 &&
        (int)(rng_next(&th->rng) % 100u) < opt->remote_pct) {
      while (!inbox_push(next_inbox, ptr)) {
        ++th->push_yields;
        ++th->drain_calls;
        size_t pushed_drain = drain_inbox(my_inbox);
        th->drain_objects += pushed_drain;
        if (pushed_drain == 0) ++th->drain_empty;
        sched_yield();
      }
      ++th->remote_enqueue;
    } else {
      ++th->local_free;
      free(ptr);
    }
  }
  th->work_ns = now_ns() - start;
  atomic_store_explicit(&th->done[th->index], 1, memory_order_release);

  uint64_t tail_start = now_ns();
  for (;;) {
    ++th->drain_calls;
    size_t drained = drain_inbox(my_inbox);
    th->drain_objects += drained;
    if (drained == 0) ++th->drain_empty;
    if (atomic_load_explicit(&th->done[prev], memory_order_acquire) &&
        drained == 0) {
      ++th->drain_calls;
      size_t final_drain = drain_inbox(my_inbox);
      th->drain_objects += final_drain;
      if (final_drain == 0) {
        ++th->drain_empty;
        break;
      }
    }
    if (drained == 0) {
      ++th->finish_yields;
      sched_yield();
    }
  }
  th->tail_ns = now_ns() - tail_start;
  pthread_barrier_wait(th->barrier);
  return NULL;
}

static void* thread_phase(void* arg) {
  ThreadState* th = (ThreadState*)arg;
  const Options* opt = th->opt;
  int next = (th->index + 1) % opt->threads;
  Inbox* next_inbox = &th->inboxes[next];

  uint64_t start = now_ns();
  for (size_t i = 0; i < opt->iters; ++i) {
    size_t size = rand_range(&th->rng, opt->min_size, opt->max_size);
    void* ptr = malloc(size);
    if (!ptr) {
      th->error = 1;
      break;
    }
    touch_payload(ptr, size);
    if (opt->remote_pct > 0 &&
        (int)(rng_next(&th->rng) % 100u) < opt->remote_pct) {
      if (next_inbox->count >= next_inbox->cap) {
        th->error = 2;
        break;
      }
      next_inbox->items[next_inbox->count++] = ptr;
      ++th->remote_live_objects;
      th->remote_live_rounded += (uint64_t)p2_round_size(size);
    } else {
      free(ptr);
    }
  }
  th->work_ns = now_ns() - start;
  pthread_barrier_wait(th->barrier);

  if (!th->error) {
    uint64_t remote_start = now_ns();
    Inbox* my_inbox = &th->inboxes[th->index];
    for (size_t i = 0; i < my_inbox->count; ++i) {
      free(my_inbox->items[i]);
    }
    th->tail_ns = now_ns() - remote_start;
  }
  pthread_barrier_wait(th->barrier);
  return NULL;
}

static int cmp_double(const void* a, const void* b) {
  double da = *(const double*)a;
  double db = *(const double*)b;
  return (da > db) - (da < db);
}

static int cmp_size(const void* a, const void* b) {
  size_t da = *(const size_t*)a;
  size_t db = *(const size_t*)b;
  return (da > db) - (da < db);
}

static double percentile_double(double* values, size_t n, double p) {
  if (n == 0) return 0.0;
  size_t idx = (size_t)((double)(n - 1u) * p + 0.5);
  return values[idx < n ? idx : n - 1u];
}

static size_t percentile_size(size_t* values, size_t n, double p) {
  if (n == 0) return 0;
  size_t idx = (size_t)((double)(n - 1u) * p + 0.5);
  return values[idx < n ? idx : n - 1u];
}

static int parse_size(const char* s, size_t* out) {
  char* end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (!s || end == s || *end != '\0') return -1;
  *out = (size_t)v;
  return 0;
}

static int parse_int(const char* s, int* out) {
  size_t value = 0;
  if (parse_size(s, &value) != 0 || value > 2147483647u) return -1;
  *out = (int)value;
  return 0;
}

static void usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [--runs N] [--threads N] [--iters N]\n"
          "          [--min-size N] [--max-size N] [--remote-pct N]\n"
          "          [--interleaved 0|1] [--live-window N]\n",
          argv0);
}

static int parse_options(int argc, char** argv, Options* opt) {
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (strcmp(a, "--runs") == 0 && i + 1 < argc) {
      if (parse_int(argv[++i], &opt->runs) != 0) return -1;
    } else if (strcmp(a, "--threads") == 0 && i + 1 < argc) {
      if (parse_int(argv[++i], &opt->threads) != 0) return -1;
    } else if (strcmp(a, "--iters") == 0 && i + 1 < argc) {
      if (parse_size(argv[++i], &opt->iters) != 0) return -1;
    } else if (strcmp(a, "--min-size") == 0 && i + 1 < argc) {
      if (parse_size(argv[++i], &opt->min_size) != 0) return -1;
    } else if (strcmp(a, "--max-size") == 0 && i + 1 < argc) {
      if (parse_size(argv[++i], &opt->max_size) != 0) return -1;
    } else if (strcmp(a, "--remote-pct") == 0 && i + 1 < argc) {
      if (parse_int(argv[++i], &opt->remote_pct) != 0) return -1;
    } else if (strcmp(a, "--interleaved") == 0 && i + 1 < argc) {
      if (parse_int(argv[++i], &opt->interleaved) != 0) return -1;
    } else if (strcmp(a, "--live-window") == 0 && i + 1 < argc) {
      if (parse_size(argv[++i], &opt->live_window) != 0) return -1;
    } else {
      return -1;
    }
  }
  return 0;
}

static int run_once(const Options* opt, int run, double* throughput, size_t* rss,
                    size_t* peak, size_t* faults, double* work_ms,
                    double* tail_ms, double* steady, size_t* remote_live,
                    uint64_t* rounded_live) {
  Inbox* inboxes = calloc((size_t)opt->threads, sizeof(*inboxes));
  ThreadState* th = calloc((size_t)opt->threads, sizeof(*th));
  pthread_t* tids = calloc((size_t)opt->threads, sizeof(*tids));
  _Atomic int* done = calloc((size_t)opt->threads, sizeof(*done));
  if (!inboxes || !th || !tids || !done) return 1;

  for (int i = 0; i < opt->threads; ++i) {
    inboxes[i].cap = opt->interleaved
                         ? ((opt->live_window ? opt->live_window : opt->iters) + 1u)
                         : opt->iters;
    inboxes[i].items = calloc(inboxes[i].cap, sizeof(void*));
    if (!inboxes[i].items) return 1;
  }

  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, (unsigned)opt->threads);
  struct rusage before;
  struct rusage after;
  getrusage(RUSAGE_SELF, &before);
  uint64_t start = now_ns();
  for (int i = 0; i < opt->threads; ++i) {
    th[i].index = i;
    th[i].opt = opt;
    th[i].inboxes = inboxes;
    th[i].done = done;
    th[i].barrier = &barrier;
    th[i].rng = 0x9E3779B9u ^ (uint32_t)(run * 131 + i * 17);
    void* (*entry)(void*) = opt->interleaved ? thread_interleaved : thread_phase;
    pthread_create(&tids[i], NULL, entry, &th[i]);
  }

  int error = 0;
  uint64_t work_ns_max = 0;
  uint64_t tail_ns_max = 0;
  size_t enq = 0, loc = 0, calls = 0, objs = 0, empty = 0, py = 0, fy = 0;
  *remote_live = 0;
  *rounded_live = 0;
  for (int i = 0; i < opt->threads; ++i) {
    pthread_join(tids[i], NULL);
    if (th[i].error) error = th[i].error;
    if (th[i].work_ns > work_ns_max) work_ns_max = th[i].work_ns;
    if (th[i].tail_ns > tail_ns_max) tail_ns_max = th[i].tail_ns;
    enq += th[i].remote_enqueue;
    loc += th[i].local_free;
    calls += th[i].drain_calls;
    objs += th[i].drain_objects;
    empty += th[i].drain_empty;
    py += th[i].push_yields;
    fy += th[i].finish_yields;
    *remote_live += th[i].remote_live_objects;
    *rounded_live += th[i].remote_live_rounded;
  }
  uint64_t end = now_ns();
  getrusage(RUSAGE_SELF, &after);
  pthread_barrier_destroy(&barrier);
  if (error) return error;

  double ops = (double)opt->threads * (double)opt->iters;
  double sec = (double)(end - start) / 1e9;
  *throughput = sec > 0.0 ? ops / sec : 0.0;
  *work_ms = (double)work_ns_max / 1e6;
  *tail_ms = (double)tail_ns_max / 1e6;
  *steady = work_ns_max ? ops / ((double)work_ns_max / 1e9) : 0.0;
  MemorySample mem = read_memory();
  *rss = mem.rss;
  *peak = mem.hwm;
  long minflt = after.ru_minflt - before.ru_minflt;
  *faults = minflt > 0 ? (size_t)minflt : 0;

  printf("run=%d ops/s=%.3f post_rss=%zu peak_rss=%zu minor_faults=%zu\n",
         run + 1, *throughput, *rss, *peak, *faults);
  if (opt->interleaved) {
    printf("run_interleaved=%d work_ms=%.3f work_ops/s=%.3f tail_ms=%.3f remote_enqueue=%zu local_free=%zu drain_calls=%zu drain_objects=%zu drain_empty=%zu push_yields=%zu finish_yields=%zu\n",
           run + 1, *work_ms, *steady, *tail_ms, enq, loc, calls, objs, empty,
           py, fy);
  } else {
    printf("run_phase=%d alloc_ms=%.3f remote_ms=%.3f remote_live=%zu rounded_live_bytes=%llu\n",
           run + 1, *work_ms, *tail_ms, *remote_live,
           (unsigned long long)*rounded_live);
  }

  for (int i = 0; i < opt->threads; ++i) free(inboxes[i].items);
  free(inboxes);
  free(th);
  free(tids);
  free(done);
  return 0;
}

int main(int argc, char** argv) {
  Options opt = {10, 16, 100000, 16, 2048, 0, 0, 0};
  if (parse_options(argc, argv, &opt) != 0 || opt.runs <= 0 ||
      opt.threads <= 0 || opt.iters == 0 || opt.min_size == 0 ||
      opt.max_size < opt.min_size || opt.remote_pct < 0 ||
      opt.remote_pct > 100 || opt.interleaved < 0 || opt.interleaved > 1) {
    usage(argv[0]);
    return 1;
  }

  size_t n = (size_t)opt.runs;
  double* throughput = calloc(n, sizeof(*throughput));
  double* work_ms = calloc(n, sizeof(*work_ms));
  double* tail_ms = calloc(n, sizeof(*tail_ms));
  double* steady = calloc(n, sizeof(*steady));
  size_t* rss = calloc(n, sizeof(*rss));
  size_t* peak = calloc(n, sizeof(*peak));
  size_t* faults = calloc(n, sizeof(*faults));
  size_t* remote_live = calloc(n, sizeof(*remote_live));
  uint64_t* rounded_live = calloc(n, sizeof(*rounded_live));
  if (!throughput || !work_ms || !tail_ms || !steady || !rss || !peak ||
      !faults || !remote_live || !rounded_live) {
    fprintf(stderr, "bench allocation failed\n");
    return 1;
  }

  for (int run = 0; run < opt.runs; ++run) {
    int rc = run_once(&opt, run, &throughput[run], &rss[run], &peak[run],
                      &faults[run], &work_ms[run], &tail_ms[run],
                      &steady[run], &remote_live[run], &rounded_live[run]);
    if (rc != 0) {
      fprintf(stderr, "bench run %d failed: %d\n", run + 1, rc);
      return 1;
    }
  }

  qsort(throughput, n, sizeof(*throughput), cmp_double);
  qsort(work_ms, n, sizeof(*work_ms), cmp_double);
  qsort(tail_ms, n, sizeof(*tail_ms), cmp_double);
  qsort(steady, n, sizeof(*steady), cmp_double);
  qsort(rss, n, sizeof(*rss), cmp_size);
  qsort(peak, n, sizeof(*peak), cmp_size);
  qsort(faults, n, sizeof(*faults), cmp_size);
  qsort(remote_live, n, sizeof(*remote_live), cmp_size);

  printf("summary runs=%d threads=%d iters=%zu size=%zu..%zu remote_pct=%d interleaved=%d live_window=%zu api=malloc_free\n",
         opt.runs, opt.threads, opt.iters, opt.min_size, opt.max_size,
         opt.remote_pct, opt.interleaved, opt.live_window);
  printf("throughput median=%.3f p25=%.3f p75=%.3f min=%.3f max=%.3f\n",
         percentile_double(throughput, n, 0.50),
         percentile_double(throughput, n, 0.25),
         percentile_double(throughput, n, 0.75), throughput[0],
         throughput[n - 1u]);
  printf("post_rss median=%zu min=%zu max=%zu\n",
         percentile_size(rss, n, 0.50), rss[0], rss[n - 1u]);
  printf("peak_rss median=%zu min=%zu max=%zu source=VmHWM_process\n",
         percentile_size(peak, n, 0.50), peak[0], peak[n - 1u]);
  printf("page_faults minor_median=%zu minor_min=%zu minor_max=%zu\n",
         percentile_size(faults, n, 0.50), faults[0], faults[n - 1u]);
  printf("steady_work throughput_median=%.3f p25=%.3f p75=%.3f\n",
         percentile_double(steady, n, 0.50),
         percentile_double(steady, n, 0.25),
         percentile_double(steady, n, 0.75));
  if (opt.interleaved) {
    printf("interleaved_phase_ms work_median=%.3f tail_median=%.3f\n",
           percentile_double(work_ms, n, 0.50),
           percentile_double(tail_ms, n, 0.50));
  } else {
    printf("phase_ms alloc_median=%.3f remote_median=%.3f\n",
           percentile_double(work_ms, n, 0.50),
           percentile_double(tail_ms, n, 0.50));
    printf("phase_live remote_live_median=%zu rounded_live_bytes_note=per-run-logs\n",
           percentile_size(remote_live, n, 0.50));
  }

  free(throughput);
  free(work_ms);
  free(tail_ms);
  free(steady);
  free(rss);
  free(peak);
  free(faults);
  free(remote_live);
  free(rounded_live);
  return 0;
}
