#include "h8_bench_support.h"

#include "../include/h8.h"
#include "../src/h8_internal.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct H8RemoteMicroThread {
  int index;
  int threads;
  size_t iters;
  size_t min_size;
  size_t max_size;
  void** all_ptrs;
  size_t count;
  pthread_barrier_t* barrier;
  uint64_t alloc_ns;
  uint64_t remote_ns;
  uint64_t collect_ns;
  int error;
} H8RemoteMicroThread;

static void* h8_remote_micro_thread(void* arg) {
  H8RemoteMicroThread* th = (H8RemoteMicroThread*)arg;
  uint32_t rng = 0xC0DEC0DEu ^ (uint32_t)(th->index * 2654435761u);
  void** own = &th->all_ptrs[(size_t)th->index * th->iters];

  uint64_t alloc_start = h8_now_ns();
  for (size_t i = 0; i < th->iters; ++i) {
    size_t size = h8_rand_range(&rng, th->min_size, th->max_size);
    void* ptr = h8_malloc(size);
    if (!ptr) {
      th->error = 1;
      break;
    }
    ((volatile unsigned char*)ptr)[0] = (unsigned char)size;
    if (size > 1) {
      ((volatile unsigned char*)ptr)[size - 1] = (unsigned char)(size >> 8);
    }
    own[i] = ptr;
  }
  th->alloc_ns = h8_now_ns() - alloc_start;

  H8OwnerRecord* owner = h8_owner_current();
  pthread_barrier_wait(th->barrier);
  if (th->error) {
    return NULL;
  }

  int prev = (th->index + th->threads - 1) % th->threads;
  void** remote = &th->all_ptrs[(size_t)prev * th->iters];
  uint64_t start = h8_now_ns();
  for (size_t i = 0; i < th->iters; ++i) {
    h8_free(remote[i]);
  }
  th->remote_ns = h8_now_ns() - start;

  pthread_barrier_wait(th->barrier);
  uint64_t collect_start = h8_now_ns();
  h8_collect_owner_pending(owner);
  th->collect_ns = h8_now_ns() - collect_start;
  return NULL;
}

static int h8_remote_micro_run(const H8BenchOptions* opt, double* publish_ops,
                               double* alloc_ms, double* publish_ms,
                               double* collect_ms) {
  size_t total = (size_t)opt->threads * opt->iters_per_thread;
  void** ptrs = calloc(total, sizeof(*ptrs));
  H8RemoteMicroThread* th = calloc((size_t)opt->threads, sizeof(*th));
  pthread_t* tids = calloc((size_t)opt->threads, sizeof(*tids));
  if (!ptrs || !th || !tids) {
    free(ptrs);
    free(th);
    free(tids);
    return -1;
  }

  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, (unsigned)opt->threads);
  for (int t = 0; t < opt->threads; ++t) {
    th[t].index = t;
    th[t].threads = opt->threads;
    th[t].iters = opt->iters_per_thread;
    th[t].min_size = opt->min_size;
    th[t].max_size = opt->max_size;
    th[t].all_ptrs = ptrs;
    th[t].barrier = &barrier;
    pthread_create(&tids[t], NULL, h8_remote_micro_thread, &th[t]);
  }

  uint64_t max_alloc_ns = 0;
  uint64_t max_remote_ns = 0;
  uint64_t max_collect_ns = 0;
  int error = 0;
  for (int t = 0; t < opt->threads; ++t) {
    pthread_join(tids[t], NULL);
    if (th[t].error) {
      error = th[t].error;
    }
    if (th[t].alloc_ns > max_alloc_ns) {
      max_alloc_ns = th[t].alloc_ns;
    }
    if (th[t].remote_ns > max_remote_ns) {
      max_remote_ns = th[t].remote_ns;
    }
    if (th[t].collect_ns > max_collect_ns) {
      max_collect_ns = th[t].collect_ns;
    }
  }
  pthread_barrier_destroy(&barrier);
  if (error) {
    free(ptrs);
    free(th);
    free(tids);
    return -1;
  }

  *alloc_ms = (double)max_alloc_ns / 1e6;
  *collect_ms = (double)max_collect_ns / 1e6;
  *publish_ms = (double)max_remote_ns / 1e6;
  *publish_ops = max_remote_ns ? (double)total / ((double)max_remote_ns / 1e9)
                               : 0.0;

  free(ptrs);
  free(th);
  free(tids);
  return 0;
}

int main(int argc, char** argv) {
  H8BenchOptions opt = {
      .runs = 5,
      .threads = 16,
      .iters_per_thread = 100000,
      .min_size = 16,
      .max_size = 2048,
      .remote_pct = 100,
      .interleaved = 0,
  };
  if (h8_parse_options(argc, argv, &opt) != 0 || opt.runs <= 0 ||
      opt.threads <= 0 || opt.iters_per_thread == 0 || opt.min_size == 0 ||
      opt.max_size < opt.min_size) {
    h8_usage(argv[0]);
    return 1;
  }

  double* publish_ops = calloc((size_t)opt.runs, sizeof(*publish_ops));
  double* alloc_ms = calloc((size_t)opt.runs, sizeof(*alloc_ms));
  double* publish_ms = calloc((size_t)opt.runs, sizeof(*publish_ms));
  double* collect_ms = calloc((size_t)opt.runs, sizeof(*collect_ms));
  if (!publish_ops || !alloc_ms || !publish_ms || !collect_ms) {
    free(publish_ops);
    free(alloc_ms);
    free(publish_ms);
    free(collect_ms);
    return 1;
  }

  for (int run = 0; run < opt.runs; ++run) {
    if (h8_remote_micro_run(&opt, &publish_ops[run], &alloc_ms[run],
                            &publish_ms[run], &collect_ms[run]) != 0) {
      fprintf(stderr, "remote micro run %d failed\n", run + 1);
      return 1;
    }
    printf("run=%d publish_ops/s=%.3f alloc_ms=%.3f publish_ms=%.3f collect_ms=%.3f\n",
           run + 1, publish_ops[run], alloc_ms[run], publish_ms[run],
           collect_ms[run]);
  }

  qsort(publish_ops, (size_t)opt.runs, sizeof(*publish_ops), h8_cmp_double);
  qsort(alloc_ms, (size_t)opt.runs, sizeof(*alloc_ms), h8_cmp_double);
  qsort(publish_ms, (size_t)opt.runs, sizeof(*publish_ms), h8_cmp_double);
  qsort(collect_ms, (size_t)opt.runs, sizeof(*collect_ms), h8_cmp_double);

  H8Stats stats = h8_stats();
  H8DebugStats debug = h8_debug_stats();
  printf("remote_micro_summary runs=%d threads=%d iters=%zu size=%zu..%zu class_map_id=%s\n",
         opt.runs, opt.threads, opt.iters_per_thread, opt.min_size,
         opt.max_size, H8_CLASS_MAP_ID);
  printf("remote_micro_publish throughput_median=%.3f p25=%.3f p75=%.3f publish_ms_median=%.3f alloc_ms_median=%.3f collect_ms_median=%.3f\n",
         h8_percentile(publish_ops, (size_t)opt.runs, 0.50),
         h8_percentile(publish_ops, (size_t)opt.runs, 0.25),
         h8_percentile(publish_ops, (size_t)opt.runs, 0.75),
         h8_percentile(publish_ms, (size_t)opt.runs, 0.50),
         h8_percentile(alloc_ms, (size_t)opt.runs, 0.50),
         h8_percentile(collect_ms, (size_t)opt.runs, 0.50));
  printf("remote_micro_counters local=%zu remote_collect=%zu owner_exit=%zu publish_ok=%zu lease_ok=%zu pending_claim_ok=%zu notify_first=%zu validate_fail=%zu duplicate_claim=%zu quiescent_repair=%zu\n",
         stats.local_alloc_count, stats.remote_collect_count,
         stats.owner_exit_count, debug.remote_stage_publish_ok,
         debug.remote_stage_regular_lease_ok,
         debug.remote_stage_pending_claim_ok, debug.remote_stage_notify_first,
         debug.remote_stage_validate_fail,
         debug.remote_publish_pending_claim_duplicate_count,
         debug.quiescent_pending_repair);

  free(publish_ops);
  free(alloc_ms);
  free(publish_ms);
  free(collect_ms);
  return 0;
}
