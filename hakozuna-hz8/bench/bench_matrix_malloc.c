#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#if defined(H8_MATRIX_USE_HZ8_API)
#include "../include/h8.h"
#endif

typedef struct PtrVec {
  void** items;
  size_t len;
  size_t cap;
  pthread_mutex_t lock;
} PtrVec;

typedef struct BenchOptions {
  int runs;
  int threads;
  size_t iters;
  size_t min_size;
  size_t max_size;
  int remote_pct;
  int interleaved;
  size_t live_window;
} BenchOptions;

typedef struct ThreadArg {
  const BenchOptions* opt;
  PtrVec* inboxes;
  pthread_barrier_t* barrier;
  int tid;
  uint64_t seed;
  double alloc_ms;
  double remote_ms;
} ThreadArg;

static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static uint32_t rng_next(uint64_t* state) {
  uint64_t x = *state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  *state = x;
  return (uint32_t)((x * UINT64_C(2685821657736338717)) >> 32);
}

static size_t rss_bytes(void) {
  FILE* fp = fopen("/proc/self/status", "r");
  if (!fp) {
    return 0;
  }
  char line[256];
  size_t rss_kb = 0;
  while (fgets(line, sizeof(line), fp)) {
    if (sscanf(line, "VmRSS: %zu kB", &rss_kb) == 1) {
      break;
    }
  }
  fclose(fp);
  return rss_kb * 1024u;
}

static size_t peak_rss_bytes(void) {
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) != 0) {
    return 0;
  }
  return (size_t)ru.ru_maxrss * 1024u;
}

static long minor_faults(void) {
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) != 0) {
    return 0;
  }
  return ru.ru_minflt;
}

static void vec_push(PtrVec* vec, void* p) {
  pthread_mutex_lock(&vec->lock);
  if (vec->len >= vec->cap) {
    fprintf(stderr, "matrix vector capacity exceeded\n");
    abort();
  }
  vec->items[vec->len++] = p;
  pthread_mutex_unlock(&vec->lock);
}

static void* bench_payload_malloc(size_t size) {
#if defined(H8_MATRIX_USE_HZ8_API)
  return h8_malloc(size);
#else
  return malloc(size);
#endif
}

static void bench_payload_free(void* ptr) {
#if defined(H8_MATRIX_USE_HZ8_API)
  h8_free(ptr);
#else
  free(ptr);
#endif
}

static size_t vec_drain(PtrVec* vec) {
  pthread_mutex_lock(&vec->lock);
  size_t n = vec->len;
  for (size_t i = 0; i < n; ++i) {
    bench_payload_free(vec->items[i]);
    vec->items[i] = NULL;
  }
  vec->len = 0;
  pthread_mutex_unlock(&vec->lock);
  return n;
}

static void vec_init(PtrVec* vec, size_t cap) {
  vec->items = (void**)calloc(cap ? cap : 1u, sizeof(*vec->items));
  if (!vec->items) {
    fprintf(stderr, "calloc failed\n");
    abort();
  }
  vec->len = 0;
  vec->cap = cap ? cap : 1u;
  pthread_mutex_init(&vec->lock, NULL);
}

static void vec_destroy(PtrVec* vec) {
  vec_drain(vec);
  free(vec->items);
  vec->items = NULL;
  vec->cap = 0;
  pthread_mutex_destroy(&vec->lock);
}

static size_t choose_size(const BenchOptions* opt, uint64_t* seed) {
  size_t span = opt->max_size - opt->min_size + 1u;
  return opt->min_size + (size_t)(rng_next(seed) % span);
}

static void touch_payload(void* p, size_t size) {
  unsigned char* c = (unsigned char*)p;
  c[0] = 0xA5u;
  c[size - 1u] = 0x5Au;
}

static void owner_collect_tick(size_t size) {
  for (unsigned i = 0; i < 64u; ++i) {
    void* p = bench_payload_malloc(size);
    if (!p) {
      fprintf(stderr, "malloc failed during owner collect tick\n");
      abort();
    }
    touch_payload(p, size);
    bench_payload_free(p);
  }
}

static void* worker(void* argp) {
  ThreadArg* arg = (ThreadArg*)argp;
  const BenchOptions* opt = arg->opt;
  uint64_t seed = arg->seed;
  int phase_mode = !opt->interleaved && opt->remote_pct > 0;
  PtrVec local = {0};
  vec_init(&local, opt->live_window ? opt->live_window + 1u : opt->iters);
  uint64_t alloc_start = now_ns();

  if (opt->interleaved) {
    for (size_t i = 0; i < opt->iters; ++i) {
      if ((i & 31u) == 0u) {
        vec_drain(&arg->inboxes[arg->tid]);
      }
      size_t size = choose_size(opt, &seed);
      void* p = bench_payload_malloc(size);
      if (!p) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        abort();
      }
      touch_payload(p, size);
      int remote = (int)(rng_next(&seed) % 100u) < opt->remote_pct;
      if (remote && opt->threads > 1) {
        int dst = (arg->tid + 1 + (int)(rng_next(&seed) % (uint32_t)(opt->threads - 1))) %
                  opt->threads;
        vec_push(&arg->inboxes[dst], p);
      } else if (opt->live_window > 0) {
        vec_push(&local, p);
        if (local.len > opt->live_window) {
          void* old = local.items[--local.len];
          bench_payload_free(old);
        }
      } else {
        bench_payload_free(p);
      }
    }
    vec_drain(&local);
    uint64_t remote_start = now_ns();
    pthread_barrier_wait(arg->barrier);
    vec_drain(&arg->inboxes[arg->tid]);
    pthread_barrier_wait(arg->barrier);
    owner_collect_tick(opt->min_size);
    uint64_t end = now_ns();
    arg->alloc_ms = (double)(remote_start - alloc_start) / 1000000.0;
    arg->remote_ms = (double)(end - remote_start) / 1000000.0;
  } else {
    for (size_t i = 0; i < opt->iters; ++i) {
      size_t size = choose_size(opt, &seed);
      void* p = bench_payload_malloc(size);
      if (!p) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        abort();
      }
      touch_payload(p, size);
      int remote = (int)(rng_next(&seed) % 100u) < opt->remote_pct;
      if (remote && opt->threads > 1) {
        int dst = (arg->tid + 1 + (int)(rng_next(&seed) % (uint32_t)(opt->threads - 1))) %
                  opt->threads;
        vec_push(&arg->inboxes[dst], p);
      } else if (!phase_mode) {
        bench_payload_free(p);
      } else {
        vec_push(&local, p);
      }
    }
    uint64_t alloc_end = now_ns();
    pthread_barrier_wait(arg->barrier);
    vec_drain(&local);
    vec_drain(&arg->inboxes[arg->tid]);
    pthread_barrier_wait(arg->barrier);
    owner_collect_tick(opt->min_size);
    uint64_t end = now_ns();
    arg->alloc_ms = (double)(alloc_end - alloc_start) / 1000000.0;
    arg->remote_ms = (double)(end - alloc_end) / 1000000.0;
  }

  vec_destroy(&local);
  return NULL;
}

static int parse_int_arg(int argc, char** argv, const char* name, int def) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (strcmp(argv[i], name) == 0) {
      return atoi(argv[i + 1]);
    }
  }
  return def;
}

static size_t parse_size_arg(int argc, char** argv, const char* name,
                             size_t def) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (strcmp(argv[i], name) == 0) {
      return (size_t)strtoull(argv[i + 1], NULL, 10);
    }
  }
  return def;
}

static int cmp_double(const void* a, const void* b) {
  double da = *(const double*)a;
  double db = *(const double*)b;
  return (da > db) - (da < db);
}

static double pick(double* v, int n, double p) {
  int idx = (int)((double)(n - 1) * p + 0.5);
  if (idx < 0) {
    idx = 0;
  }
  if (idx >= n) {
    idx = n - 1;
  }
  return v[idx];
}

int main(int argc, char** argv) {
  BenchOptions opt = {
      .runs = parse_int_arg(argc, argv, "--runs", 1),
      .threads = parse_int_arg(argc, argv, "--threads", 16),
      .iters = parse_size_arg(argc, argv, "--iters", 50000),
      .min_size = parse_size_arg(argc, argv, "--min-size", 16),
      .max_size = parse_size_arg(argc, argv, "--max-size", 2048),
      .remote_pct = parse_int_arg(argc, argv, "--remote-pct", 0),
      .interleaved = parse_int_arg(argc, argv, "--interleaved", 0),
      .live_window = parse_size_arg(argc, argv, "--live-window", 0),
  };
  if (opt.runs <= 0 || opt.threads <= 0 || opt.iters == 0 ||
      opt.min_size == 0 || opt.max_size < opt.min_size ||
      opt.remote_pct < 0 || opt.remote_pct > 100) {
    fprintf(stderr, "invalid options\n");
    return 2;
  }

  double* throughput = (double*)calloc((size_t)opt.runs, sizeof(*throughput));
  double* work_ms = (double*)calloc((size_t)opt.runs, sizeof(*work_ms));
  double* tail_ms = (double*)calloc((size_t)opt.runs, sizeof(*tail_ms));
  double* post_rss = (double*)calloc((size_t)opt.runs, sizeof(*post_rss));
  double* peak_rss = (double*)calloc((size_t)opt.runs, sizeof(*peak_rss));
  double* faults = (double*)calloc((size_t)opt.runs, sizeof(*faults));
  if (!throughput || !work_ms || !tail_ms || !post_rss || !peak_rss ||
      !faults) {
    return 3;
  }

  for (int run = 0; run < opt.runs; ++run) {
    PtrVec* inboxes = (PtrVec*)calloc((size_t)opt.threads, sizeof(*inboxes));
    pthread_t* threads =
        (pthread_t*)calloc((size_t)opt.threads, sizeof(*threads));
    ThreadArg* args = (ThreadArg*)calloc((size_t)opt.threads, sizeof(*args));
    if (!inboxes || !threads || !args) {
      return 4;
    }
    for (int t = 0; t < opt.threads; ++t) {
      vec_init(&inboxes[t], opt.iters * (size_t)opt.threads);
    }
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned)opt.threads);
    long faults_before = minor_faults();
    uint64_t start = now_ns();
    for (int t = 0; t < opt.threads; ++t) {
      args[t].opt = &opt;
      args[t].inboxes = inboxes;
      args[t].barrier = &barrier;
      args[t].tid = t;
      args[t].seed = UINT64_C(0x9e3779b97f4a7c15) ^
                     (uint64_t)(run + 1) * UINT64_C(0xbf58476d1ce4e5b9) ^
                     (uint64_t)(t + 1) * UINT64_C(0x94d049bb133111eb);
      pthread_create(&threads[t], NULL, worker, &args[t]);
    }
    double alloc_sum = 0.0;
    double remote_sum = 0.0;
    for (int t = 0; t < opt.threads; ++t) {
      pthread_join(threads[t], NULL);
      alloc_sum += args[t].alloc_ms;
      remote_sum += args[t].remote_ms;
    }
    uint64_t end = now_ns();
    long faults_after = minor_faults();
    double ms = (double)(end - start) / 1000000.0;
    double ops = (double)opt.threads * (double)opt.iters;
    throughput[run] = ops / (ms / 1000.0);
    work_ms[run] = alloc_sum / (double)opt.threads;
    tail_ms[run] = remote_sum / (double)opt.threads;
    post_rss[run] = (double)rss_bytes();
    peak_rss[run] = (double)peak_rss_bytes();
    faults[run] = (double)(faults_after - faults_before);
    printf("run=%d ops/s=%.3f post_rss=%.0f peak_rss=%.0f minor_faults=%.0f work_ms=%.3f tail_ms=%.3f\n",
           run + 1, throughput[run], post_rss[run], peak_rss[run],
           faults[run], work_ms[run], tail_ms[run]);
    pthread_barrier_destroy(&barrier);
    for (int t = 0; t < opt.threads; ++t) {
      vec_destroy(&inboxes[t]);
    }
    free(inboxes);
    free(threads);
    free(args);
  }

  qsort(throughput, (size_t)opt.runs, sizeof(*throughput), cmp_double);
  qsort(work_ms, (size_t)opt.runs, sizeof(*work_ms), cmp_double);
  qsort(tail_ms, (size_t)opt.runs, sizeof(*tail_ms), cmp_double);
  qsort(post_rss, (size_t)opt.runs, sizeof(*post_rss), cmp_double);
  qsort(peak_rss, (size_t)opt.runs, sizeof(*peak_rss), cmp_double);
  qsort(faults, (size_t)opt.runs, sizeof(*faults), cmp_double);
  printf("summary runs=%d threads=%d iters=%zu size=%zu..%zu remote_pct=%d interleaved=%d live_window=%zu\n",
         opt.runs, opt.threads, opt.iters, opt.min_size, opt.max_size,
         opt.remote_pct, opt.interleaved, opt.live_window);
  printf("throughput median=%.3f p25=%.3f min=%.3f max=%.3f\n",
         pick(throughput, opt.runs, 0.5), pick(throughput, opt.runs, 0.25),
         pick(throughput, opt.runs, 0.0), pick(throughput, opt.runs, 1.0));
  printf("post_rss median=%.0f\n", pick(post_rss, opt.runs, 0.5));
  printf("peak_rss median=%.0f\n", pick(peak_rss, opt.runs, 0.5));
  printf("page_faults minor_median=%.0f minor_max=%.0f\n",
         pick(faults, opt.runs, 0.5), pick(faults, opt.runs, 1.0));
  printf("phase_ms work_median=%.3f tail_median=%.3f\n",
         pick(work_ms, opt.runs, 0.5), pick(tail_ms, opt.runs, 0.5));
  return 0;
}
