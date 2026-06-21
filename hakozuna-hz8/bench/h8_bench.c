#include "../include/h8.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct H8BenchOptions {
  int runs;
  int threads;
  size_t iters_per_thread;
  size_t min_size;
  size_t max_size;
  int remote_pct;
} H8BenchOptions;

typedef struct H8Inbox {
  void** items;
  size_t count;
  size_t cap;
} H8Inbox;

typedef struct H8BenchThread {
  int index;
  const H8BenchOptions* opt;
  H8Inbox* inboxes;
  pthread_barrier_t* barrier;
  uint32_t rng;
  int error;
} H8BenchThread;

static uint32_t h8_rng_next(uint32_t* state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x ? x : 0xA341316Cu;
  return *state;
}

static size_t h8_rand_range(uint32_t* state, size_t lo, size_t hi) {
  if (hi <= lo) {
    return lo;
  }
  uint32_t span = (uint32_t)(hi - lo + 1u);
  return lo + (size_t)(h8_rng_next(state) % span);
}

static uint64_t h8_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static size_t h8_read_rss_bytes(void) {
  FILE* f = fopen("/proc/self/status", "r");
  if (!f) {
    return 0;
  }
  char line[256];
  size_t rss_kib = 0;
  while (fgets(line, sizeof(line), f)) {
    if (sscanf(line, "VmRSS: %zu kB", &rss_kib) == 1) {
      break;
    }
  }
  fclose(f);
  return rss_kib * 1024u;
}

static void* h8_bench_thread_main(void* arg) {
  H8BenchThread* th = (H8BenchThread*)arg;
  const H8BenchOptions* opt = th->opt;
  int next = (th->index + 1) % opt->threads;
  H8Inbox* next_inbox = &th->inboxes[next];
  H8Inbox* my_inbox = &th->inboxes[th->index];

  for (size_t i = 0; i < opt->iters_per_thread; ++i) {
    size_t size = h8_rand_range(&th->rng, opt->min_size, opt->max_size);
    void* ptr = h8_malloc(size);
    if (!ptr) {
      th->error = 1;
      break;
    }
    ((volatile unsigned char*)ptr)[0] = (unsigned char)size;
    if (size > 1) {
      ((volatile unsigned char*)ptr)[size - 1] = (unsigned char)(size >> 8);
    }
    if (opt->remote_pct > 0 &&
        (int)(h8_rng_next(&th->rng) % 100u) < opt->remote_pct) {
      if (next_inbox->count >= next_inbox->cap) {
        th->error = 2;
        break;
      }
      next_inbox->items[next_inbox->count++] = ptr;
    } else {
      h8_free(ptr);
    }
  }

  pthread_barrier_wait(th->barrier);

  if (!th->error) {
    for (size_t i = 0; i < my_inbox->count; ++i) {
      h8_free(my_inbox->items[i]);
    }
  }

  pthread_barrier_wait(th->barrier);
  return NULL;
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

static void h8_usage(const char* argv0) {
  fprintf(stderr,
          "usage: %s [--runs N] [--threads N] [--iters N]\n"
          "          [--min-size N] [--max-size N] [--remote-pct N]\n",
          argv0);
}

static int h8_parse_options(int argc, char** argv, H8BenchOptions* opt) {
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
    } else {
      return -1;
    }
  }
  return 0;
}

static int h8_cmp_double(const void* a, const void* b) {
  double da = *(const double*)a;
  double db = *(const double*)b;
  return (da > db) - (da < db);
}

static int h8_cmp_size_t(const void* a, const void* b) {
  size_t da = *(const size_t*)a;
  size_t db = *(const size_t*)b;
  return (da > db) - (da < db);
}

static double h8_percentile(double* values, size_t n, double p) {
  if (n == 0) {
    return 0.0;
  }
  size_t idx = (size_t)((double)(n - 1u) * p + 0.5);
  if (idx >= n) {
    idx = n - 1u;
  }
  return values[idx];
}

static size_t h8_percentile_size_t(size_t* values, size_t n, double p) {
  if (n == 0) {
    return 0;
  }
  size_t idx = (size_t)((double)(n - 1u) * p + 0.5);
  if (idx >= n) {
    idx = n - 1u;
  }
  return values[idx];
}

int main(int argc, char** argv) {
  H8BenchOptions opt = {
      .runs = 10,
      .threads = 16,
      .iters_per_thread = 100000,
      .min_size = 16,
      .max_size = 2048,
      .remote_pct = 0,
  };
  if (h8_parse_options(argc, argv, &opt) != 0 ||
      opt.runs <= 0 || opt.threads <= 0 || opt.iters_per_thread == 0 ||
      opt.min_size == 0 || opt.max_size < opt.min_size ||
      opt.remote_pct < 0 || opt.remote_pct > 100) {
    h8_usage(argv[0]);
    return 1;
  }

  double* throughput = calloc((size_t)opt.runs, sizeof(*throughput));
  size_t* rss = calloc((size_t)opt.runs, sizeof(*rss));
  if (!throughput || !rss) {
    fprintf(stderr, "bench allocation failed\n");
    free(throughput);
    free(rss);
    return 1;
  }

  for (int run = 0; run < opt.runs; ++run) {
    H8Inbox* inboxes = calloc((size_t)opt.threads, sizeof(*inboxes));
    H8BenchThread* th = calloc((size_t)opt.threads, sizeof(*th));
    pthread_t* tids = calloc((size_t)opt.threads, sizeof(*tids));
    if (!inboxes || !th || !tids) {
      fprintf(stderr, "bench run allocation failed\n");
      free(inboxes);
      free(th);
      free(tids);
      free(throughput);
      free(rss);
      return 1;
    }

    for (int i = 0; i < opt.threads; ++i) {
      inboxes[i].cap = opt.iters_per_thread;
      inboxes[i].items = calloc(opt.iters_per_thread, sizeof(void*));
      if (!inboxes[i].items) {
        fprintf(stderr, "inbox allocation failed\n");
        return 1;
      }
    }

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned)opt.threads);
    uint64_t start = h8_now_ns();
    for (int i = 0; i < opt.threads; ++i) {
      th[i].index = i;
      th[i].opt = &opt;
      th[i].inboxes = inboxes;
      th[i].barrier = &barrier;
      th[i].rng = 0x9E3779B9u ^ (uint32_t)(run * 131 + i * 17);
      th[i].error = 0;
      pthread_create(&tids[i], NULL, h8_bench_thread_main, &th[i]);
    }

    int error = 0;
    for (int i = 0; i < opt.threads; ++i) {
      pthread_join(tids[i], NULL);
      if (th[i].error != 0) {
        error = th[i].error;
      }
    }
    uint64_t end = h8_now_ns();
    pthread_barrier_destroy(&barrier);

    if (error != 0) {
      fprintf(stderr, "bench run %d failed: %d\n", run, error);
      return 1;
    }

    double seconds = (double)(end - start) / 1e9;
    double ops = (double)opt.threads * (double)opt.iters_per_thread;
    throughput[run] = ops / seconds;
    rss[run] = h8_read_rss_bytes();
    printf("run=%d ops/s=%.3f rss=%zu\n", run + 1, throughput[run], rss[run]);

    for (int i = 0; i < opt.threads; ++i) {
      free(inboxes[i].items);
    }
    free(inboxes);
    free(th);
    free(tids);
  }

  qsort(throughput, (size_t)opt.runs, sizeof(*throughput), h8_cmp_double);
  qsort(rss, (size_t)opt.runs, sizeof(*rss), h8_cmp_size_t);

  printf("summary runs=%d threads=%d iters=%zu size=%zu..%zu remote_pct=%d\n",
         opt.runs, opt.threads, opt.iters_per_thread, opt.min_size, opt.max_size,
         opt.remote_pct);
  printf("throughput median=%.3f p25=%.3f p75=%.3f min=%.3f max=%.3f\n",
         h8_percentile(throughput, (size_t)opt.runs, 0.50),
         h8_percentile(throughput, (size_t)opt.runs, 0.25),
         h8_percentile(throughput, (size_t)opt.runs, 0.75),
         throughput[0], throughput[(size_t)opt.runs - 1u]);
  printf("rss median=%zu min=%zu max=%zu\n",
         h8_percentile_size_t(rss, (size_t)opt.runs, 0.50),
         rss[0], rss[(size_t)opt.runs - 1u]);

  H8Stats stats = h8_stats();
  H8DebugStats debug = h8_debug_stats();
  printf("counters_prod owner_exit=%zu pending_enqueue=%zu pending_dequeue=%zu orphan_handoff=%zu handoff_ok=%zu local=%zu remote=%zu\n",
         stats.owner_exit_count,
         stats.pending_enqueue_count,
         stats.pending_dequeue_count,
         stats.orphan_handoff_count,
         stats.handoff_success_count,
         stats.local_alloc_count,
         stats.remote_collect_count);
  printf("counters_dbg publish_enter=%zu publish_exit=%zu lifecycle_enter=%zu lifecycle_exit=%zu span_publish_enter=%zu span_publish_exit=%zu orphan_quiesce=%zu orphan_ready=%zu dry_scan=%zu dry_candidate=%zu dry_block_state=%zu dry_block_quiesce=%zu dry_empty=%zu dry_target_closed=%zu dry_would_adopt=%zu handoff_fail=%zu invalid=%zu miss=%zu owner_transition=%zu adopt_scan=%zu adopt_candidate=%zu adopt_block_state=%zu adopt_block_quiesce=%zu adopt_empty=%zu adopt_target_closed=%zu adopt_ok=%zu\n",
         debug.owner_publish_enter_count,
         debug.owner_publish_exit_count,
         debug.owner_lifecycle_enter_count,
         debug.owner_lifecycle_exit_count,
         debug.span_publish_enter_count,
         debug.span_publish_exit_count,
         debug.orphan_quiesce_count,
         debug.orphan_ready_count,
         debug.adoption_dry_run_scan_count,
         debug.adoption_dry_run_candidate_count,
         debug.adoption_dry_run_block_state_count,
         debug.adoption_dry_run_block_quiesce_count,
         debug.adoption_dry_run_empty_count,
         debug.adoption_dry_run_target_closed_count,
         debug.adoption_dry_run_would_adopt_count,
         debug.handoff_fail_count,
         debug.invalid_count,
         debug.miss_count,
         debug.owner_transition_count,
         debug.adoption_scan_count,
         debug.adoption_candidate_count,
         debug.adoption_block_state_count,
         debug.adoption_block_quiesce_count,
         debug.adoption_empty_count,
         debug.adoption_target_closed_count,
         debug.adoption_success_count);

  free(throughput);
  free(rss);
  return 0;
}
