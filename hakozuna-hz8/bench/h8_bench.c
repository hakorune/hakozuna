#include "../include/h8.h"
#include "h8_bench_support.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/resource.h>

int main(int argc, char** argv) {
  H8BenchOptions opt = {
      .runs = 10,
      .threads = 16,
      .iters_per_thread = 100000,
      .min_size = 16,
      .max_size = 2048,
      .remote_pct = 0,
      .interleaved = 0,
      .live_window = 0,
  };
  if (h8_parse_options(argc, argv, &opt) != 0 ||
      opt.runs <= 0 || opt.threads <= 0 || opt.iters_per_thread == 0 ||
      opt.min_size == 0 || opt.max_size < opt.min_size ||
      opt.remote_pct < 0 || opt.remote_pct > 100 ||
      opt.interleaved < 0 || opt.interleaved > 1) {
    h8_usage(argv[0]);
    return 1;
  }

  double* throughput = calloc((size_t)opt.runs, sizeof(*throughput));
  size_t* rss = calloc((size_t)opt.runs, sizeof(*rss));
  size_t* peak_rss = calloc((size_t)opt.runs, sizeof(*peak_rss));
  double* alloc_phase_ms = calloc((size_t)opt.runs, sizeof(*alloc_phase_ms));
  double* remote_phase_ms = calloc((size_t)opt.runs, sizeof(*remote_phase_ms));
  double* work_throughput = calloc((size_t)opt.runs, sizeof(*work_throughput));
  size_t* minor_faults = calloc((size_t)opt.runs, sizeof(*minor_faults));
  size_t* span_lower_bound = calloc((size_t)opt.runs, sizeof(*span_lower_bound));
  size_t* remote_live_objects = calloc((size_t)opt.runs, sizeof(*remote_live_objects));
  size_t* upper1536_span_lower_bound =
      calloc((size_t)opt.runs, sizeof(*upper1536_span_lower_bound));
  size_t* upper1p5_span_lower_bound =
      calloc((size_t)opt.runs, sizeof(*upper1p5_span_lower_bound));
  size_t* remote_live_rounded = calloc((size_t)opt.runs, sizeof(*remote_live_rounded));
  size_t* remote_live_upper1536 =
      calloc((size_t)opt.runs, sizeof(*remote_live_upper1536));
  size_t* remote_live_upper1p5 =
      calloc((size_t)opt.runs, sizeof(*remote_live_upper1p5));
#if defined(H8_BENCH_ATTRIBUTION)
  uint64_t frag_requested_total = 0;
  uint64_t frag_rounded_total = 0;
  uint64_t frag_upper1536_total = 0;
  uint64_t frag_upper1p5_total = 0;
  H8BenchMediumTotals medium_totals = {0};
  uint64_t frag_rounded_by_class[H8_CLASS_COUNT] = {0};
  size_t frag_allocs_by_class[H8_CLASS_COUNT] = {0};
#endif
  size_t interleaved_remote_enqueue_total = 0;
  size_t interleaved_local_free_total = 0;
  size_t interleaved_drain_calls_total = 0;
  size_t interleaved_drain_objects_total = 0;
  size_t interleaved_drain_empty_total = 0;
  size_t interleaved_push_yields_total = 0;
  size_t interleaved_finish_yields_total = 0;
  if (!throughput || !rss || !peak_rss || !alloc_phase_ms || !remote_phase_ms ||
      !work_throughput || !minor_faults || !span_lower_bound || !remote_live_objects ||
      !upper1536_span_lower_bound || !upper1p5_span_lower_bound ||
      !remote_live_rounded || !remote_live_upper1536 || !remote_live_upper1p5) {
    fprintf(stderr, "bench allocation failed\n");
    free(throughput);
    free(rss);
    free(peak_rss);
    free(alloc_phase_ms);
    free(remote_phase_ms);
    free(work_throughput);
    free(minor_faults);
    free(span_lower_bound);
    free(remote_live_objects);
    free(upper1536_span_lower_bound);
    free(upper1p5_span_lower_bound);
    free(remote_live_rounded);
    free(remote_live_upper1536);
    free(remote_live_upper1p5);
    return 1;
  }

  for (int run = 0; run < opt.runs; ++run) {
    H8Inbox* inboxes = calloc((size_t)opt.threads, sizeof(*inboxes));
    H8BenchThread* th = calloc((size_t)opt.threads, sizeof(*th));
    pthread_t* tids = calloc((size_t)opt.threads, sizeof(*tids));
    _Atomic int* done = calloc((size_t)opt.threads, sizeof(*done));
    if (!inboxes || !th || !tids || !done) {
      fprintf(stderr, "bench run allocation failed\n");
      free(inboxes);
      free(th);
      free(tids);
      free(done);
      free(throughput);
      free(rss);
      free(peak_rss);
      free(alloc_phase_ms);
      free(remote_phase_ms);
      free(minor_faults);
      free(span_lower_bound);
      free(remote_live_objects);
      free(upper1536_span_lower_bound);
      free(upper1p5_span_lower_bound);
      return 1;
    }

    for (int i = 0; i < opt.threads; ++i) {
      if (opt.interleaved) {
        size_t window = opt.live_window ? opt.live_window : opt.iters_per_thread;
        inboxes[i].cap = window + 1u;
      } else {
        inboxes[i].cap = opt.iters_per_thread;
      }
      inboxes[i].items = calloc(inboxes[i].cap, sizeof(void*));
      if (!inboxes[i].items) {
        fprintf(stderr, "inbox allocation failed\n");
        return 1;
      }
    }

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned)opt.threads);
    struct rusage usage_before;
    struct rusage usage_after;
    getrusage(RUSAGE_SELF, &usage_before);
    uint64_t start = h8_now_ns();
    for (int i = 0; i < opt.threads; ++i) {
      th[i].index = i;
      th[i].opt = &opt;
      th[i].inboxes = inboxes;
      th[i].done = done;
      th[i].barrier = &barrier;
      th[i].rng = 0x9E3779B9u ^ (uint32_t)(run * 131 + i * 17);
      th[i].error = 0;
      pthread_create(&tids[i], NULL, h8_bench_thread_main, &th[i]);
    }

    int error = 0;
    uint64_t alloc_ns_max = 0;
    uint64_t remote_ns_max = 0;
    size_t run_remote_enqueue = 0;
    size_t run_local_free = 0;
    size_t run_drain_calls = 0;
    size_t run_drain_objects = 0;
    size_t run_drain_empty = 0;
    size_t run_push_yields = 0;
    size_t run_finish_yields = 0;
    for (int i = 0; i < opt.threads; ++i) {
      pthread_join(tids[i], NULL);
      if (th[i].error != 0) {
        error = th[i].error;
      }
      if (th[i].alloc_ns > alloc_ns_max) {
        alloc_ns_max = th[i].alloc_ns;
      }
      if (th[i].remote_ns > remote_ns_max) {
        remote_ns_max = th[i].remote_ns;
      }
      run_remote_enqueue += th[i].interleaved_remote_enqueue;
      run_local_free += th[i].interleaved_local_free;
      run_drain_calls += th[i].interleaved_drain_calls;
      run_drain_objects += th[i].interleaved_drain_objects;
      run_drain_empty += th[i].interleaved_drain_empty;
      run_push_yields += th[i].interleaved_push_yields;
      run_finish_yields += th[i].interleaved_finish_yields;
      interleaved_remote_enqueue_total += th[i].interleaved_remote_enqueue;
      interleaved_local_free_total += th[i].interleaved_local_free;
      interleaved_drain_calls_total += th[i].interleaved_drain_calls;
      interleaved_drain_objects_total += th[i].interleaved_drain_objects;
      interleaved_drain_empty_total += th[i].interleaved_drain_empty;
      interleaved_push_yields_total += th[i].interleaved_push_yields;
      interleaved_finish_yields_total += th[i].interleaved_finish_yields;
    }
    uint64_t end = h8_now_ns();
    getrusage(RUSAGE_SELF, &usage_after);
    pthread_barrier_destroy(&barrier);

    if (error != 0) {
      fprintf(stderr, "bench run %d failed: %d\n", run, error);
      return 1;
    }

    double seconds = (double)(end - start) / 1e9;
    double ops = (double)opt.threads * (double)opt.iters_per_thread;
    size_t lower = 0;
    size_t upper1536_lower = 0;
    size_t upper1p5_lower = 0;
    size_t live_objects = 0;
#if defined(H8_BENCH_ATTRIBUTION)
    uint64_t run_requested = 0;
    uint64_t run_rounded = 0;
    uint64_t run_live_rounded = 0;
    uint64_t run_live_upper1536 = 0;
    uint64_t run_live_upper1p5 = 0;
    if (!opt.interleaved) {
      for (int i = 0; i < opt.threads; ++i) {
        for (uint32_t c = 0; c < H8_CLASS_COUNT; ++c) {
          size_t live = th[i].remote_live_by_class[c];
          size_t slots = h8_bench_slots_for_class(c);
          live_objects += live;
          lower += (live + slots - 1u) / slots;
        }
        for (uint32_t c = 0; c < H8_BENCH_CANDIDATE_UPPER1536_COUNT; ++c) {
          size_t live = th[i].remote_live_upper1536[c];
          size_t slots = h8_bench_upper1536_slots_for_class(c);
          upper1536_lower += (live + slots - 1u) / slots;
        }
        for (uint32_t c = 0; c < H8_BENCH_CANDIDATE_UPPER1P5_COUNT; ++c) {
          size_t live = th[i].remote_live_upper1p5[c];
          size_t slots = h8_bench_upper1p5_slots_for_class(c);
          upper1p5_lower += (live + slots - 1u) / slots;
        }
        run_live_rounded += th[i].remote_live_rounded_bytes;
        run_live_upper1536 += th[i].remote_live_upper1536_bytes;
        run_live_upper1p5 += th[i].remote_live_upper1p5_bytes;
      }
    }
    for (int i = 0; i < opt.threads; ++i) {
      for (uint32_t c = 0; c < H8_CLASS_COUNT; ++c) {
        run_requested += th[i].requested_bytes_by_class[c];
        run_rounded += th[i].rounded_bytes_by_class[c];
        frag_rounded_by_class[c] += th[i].rounded_bytes_by_class[c];
        frag_allocs_by_class[c] += th[i].alloc_count_by_class[c];
      }
      frag_upper1536_total += th[i].rounded_upper1536_bytes;
      frag_upper1p5_total += th[i].rounded_upper1p5_bytes;
    }
    h8_bench_medium_totals_add(&medium_totals, th, opt.threads);
    frag_requested_total += run_requested;
    frag_rounded_total += run_rounded;
    remote_live_rounded[run] = (size_t)run_live_rounded;
    remote_live_upper1536[run] = (size_t)run_live_upper1536;
    remote_live_upper1p5[run] = (size_t)run_live_upper1p5;
#endif
    throughput[run] = ops / seconds;
    H8MemorySample mem = h8_read_memory_sample();
    rss[run] = mem.rss_bytes;
    peak_rss[run] = mem.hwm_bytes;
    span_lower_bound[run] = lower;
    upper1536_span_lower_bound[run] = upper1536_lower;
    upper1p5_span_lower_bound[run] = upper1p5_lower;
    remote_live_objects[run] = live_objects;
    long minflt_delta = usage_after.ru_minflt - usage_before.ru_minflt;
    minor_faults[run] = minflt_delta > 0 ? (size_t)minflt_delta : 0;
    alloc_phase_ms[run] = (double)alloc_ns_max / 1e6;
    remote_phase_ms[run] = (double)remote_ns_max / 1e6;
    work_throughput[run] =
        alloc_ns_max ? ops / ((double)alloc_ns_max / 1e9) : 0.0;
    printf("run=%d ops/s=%.3f post_rss=%zu peak_rss=%zu minor_faults=%zu\n",
           run + 1, throughput[run], rss[run], peak_rss[run],
           minor_faults[run]);
    if (!opt.interleaved) {
      printf("run_phase=%d alloc_ms=%.3f remote_ms=%.3f\n", run + 1,
             alloc_phase_ms[run], remote_phase_ms[run]);
    } else {
      printf("run_interleaved=%d work_ms=%.3f work_ops/s=%.3f tail_ms=%.3f remote_enqueue=%zu local_free=%zu drain_calls=%zu drain_objects=%zu drain_empty=%zu push_yields=%zu finish_yields=%zu\n",
             run + 1, alloc_phase_ms[run], work_throughput[run],
             remote_phase_ms[run],
             run_remote_enqueue, run_local_free, run_drain_calls,
             run_drain_objects, run_drain_empty, run_push_yields,
             run_finish_yields);
    }

    for (int i = 0; i < opt.threads; ++i) {
      free(inboxes[i].items);
    }
    free(inboxes);
    free(th);
    free(tids);
    free(done);
  }

  qsort(throughput, (size_t)opt.runs, sizeof(*throughput), h8_cmp_double);
  qsort(rss, (size_t)opt.runs, sizeof(*rss), h8_cmp_size_t);
  qsort(peak_rss, (size_t)opt.runs, sizeof(*peak_rss), h8_cmp_size_t);
  qsort(minor_faults, (size_t)opt.runs, sizeof(*minor_faults), h8_cmp_size_t);
  qsort(span_lower_bound, (size_t)opt.runs, sizeof(*span_lower_bound), h8_cmp_size_t);
  qsort(upper1536_span_lower_bound, (size_t)opt.runs,
        sizeof(*upper1536_span_lower_bound), h8_cmp_size_t);
  qsort(upper1p5_span_lower_bound, (size_t)opt.runs,
        sizeof(*upper1p5_span_lower_bound), h8_cmp_size_t);
  qsort(remote_live_objects, (size_t)opt.runs, sizeof(*remote_live_objects),
        h8_cmp_size_t);
  qsort(remote_live_rounded, (size_t)opt.runs, sizeof(*remote_live_rounded),
        h8_cmp_size_t);
  qsort(remote_live_upper1536, (size_t)opt.runs, sizeof(*remote_live_upper1536),
        h8_cmp_size_t);
  qsort(remote_live_upper1p5, (size_t)opt.runs, sizeof(*remote_live_upper1p5),
        h8_cmp_size_t);
  qsort(alloc_phase_ms, (size_t)opt.runs, sizeof(*alloc_phase_ms), h8_cmp_double);
  qsort(remote_phase_ms, (size_t)opt.runs, sizeof(*remote_phase_ms), h8_cmp_double);
  qsort(work_throughput, (size_t)opt.runs, sizeof(*work_throughput),
        h8_cmp_double);

  printf("summary runs=%d threads=%d iters=%zu size=%zu..%zu remote_pct=%d interleaved=%d live_window=%zu bench_attribution=%d class_map_id=%s\n",
         opt.runs, opt.threads, opt.iters_per_thread, opt.min_size, opt.max_size,
         opt.remote_pct, opt.interleaved, opt.live_window,
#if defined(H8_BENCH_ATTRIBUTION)
         1,
#else
         0,
#endif
         H8_CLASS_MAP_ID);
#if defined(H8_MEDIUM_UPPER48_CLASS)
  printf("medium_geometry_id=q64-upper48\n");
#elif defined(H8_MEDIUM_64K_ONE_SLOT)
  printf("medium_geometry_id=q64-run64k\n");
#else
  printf("medium_geometry_id=q64-run64k2\n");
#endif
#if defined(H8_MEDIUM_CHUNK_CARVE)
  printf("medium_arena_id=chunk16m\n");
#else
  printf("medium_arena_id=per-run-mmap\n");
#endif
  printf("throughput median=%.3f p25=%.3f p75=%.3f min=%.3f max=%.3f\n",
         h8_percentile(throughput, (size_t)opt.runs, 0.50),
         h8_percentile(throughput, (size_t)opt.runs, 0.25),
         h8_percentile(throughput, (size_t)opt.runs, 0.75),
         throughput[0], throughput[(size_t)opt.runs - 1u]);
  printf("post_rss median=%zu min=%zu max=%zu\n",
         h8_percentile_size_t(rss, (size_t)opt.runs, 0.50),
         rss[0], rss[(size_t)opt.runs - 1u]);
  printf("peak_rss median=%zu min=%zu max=%zu source=VmHWM_process\n",
         h8_percentile_size_t(peak_rss, (size_t)opt.runs, 0.50),
         peak_rss[0], peak_rss[(size_t)opt.runs - 1u]);
  printf("page_faults minor_median=%zu minor_min=%zu minor_max=%zu\n",
         h8_percentile_size_t(minor_faults, (size_t)opt.runs, 0.50),
         minor_faults[0], minor_faults[(size_t)opt.runs - 1u]);
  printf("steady_work throughput_median=%.3f p25=%.3f p75=%.3f\n",
         h8_percentile(work_throughput, (size_t)opt.runs, 0.50),
         h8_percentile(work_throughput, (size_t)opt.runs, 0.25),
         h8_percentile(work_throughput, (size_t)opt.runs, 0.75));
  if (!opt.interleaved) {
    printf("phase_ms alloc_median=%.3f remote_median=%.3f\n",
           h8_percentile(alloc_phase_ms, (size_t)opt.runs, 0.50),
           h8_percentile(remote_phase_ms, (size_t)opt.runs, 0.50));
    printf("live_span_lower_bound remote_live_median=%zu spans_median=%zu\n",
           h8_percentile_size_t(remote_live_objects, (size_t)opt.runs, 0.50),
           h8_percentile_size_t(span_lower_bound, (size_t)opt.runs, 0.50));
    printf("candidate_span_lower_bound upper1536_median=%zu upper1p5_median=%zu\n",
           h8_percentile_size_t(upper1536_span_lower_bound, (size_t)opt.runs,
                                0.50),
           h8_percentile_size_t(upper1p5_span_lower_bound, (size_t)opt.runs,
                                0.50));
    printf("size_policy_live_bytes p2_median=%zu upper1536_median=%zu upper1p5_median=%zu\n",
           h8_percentile_size_t(remote_live_rounded, (size_t)opt.runs, 0.50),
           h8_percentile_size_t(remote_live_upper1536, (size_t)opt.runs, 0.50),
           h8_percentile_size_t(remote_live_upper1p5, (size_t)opt.runs, 0.50));
  } else {
    printf("interleaved_phase_ms work_median=%.3f tail_median=%.3f\n",
           h8_percentile(alloc_phase_ms, (size_t)opt.runs, 0.50),
           h8_percentile(remote_phase_ms, (size_t)opt.runs, 0.50));
    printf("interleaved_work remote_enqueue=%zu local_free=%zu drain_calls=%zu drain_objects=%zu drain_empty=%zu push_yields=%zu finish_yields=%zu\n",
           interleaved_remote_enqueue_total,
           interleaved_local_free_total,
           interleaved_drain_calls_total,
           interleaved_drain_objects_total,
           interleaved_drain_empty_total,
           interleaved_push_yields_total,
           interleaved_finish_yields_total);
  }
  H8BenchReportInput report = {
      .opt = &opt,
      .minor_faults = minor_faults,
      .span_lower_bound = span_lower_bound,
#if defined(H8_BENCH_ATTRIBUTION)
      .frag_requested_total = frag_requested_total,
      .frag_rounded_total = frag_rounded_total,
      .frag_upper1536_total = frag_upper1536_total,
      .frag_upper1p5_total = frag_upper1p5_total,
      .frag_rounded_by_class = frag_rounded_by_class,
      .frag_allocs_by_class = frag_allocs_by_class,
      .medium_totals = &medium_totals,
#endif
  };
  h8_bench_print_final_report(&report);

  free(throughput);
  free(rss);
  free(peak_rss);
  free(alloc_phase_ms);
  free(remote_phase_ms);
  free(work_throughput);
  free(minor_faults);
  free(span_lower_bound);
  free(remote_live_objects);
  free(upper1536_span_lower_bound);
  free(upper1p5_span_lower_bound);
  free(remote_live_rounded);
  free(remote_live_upper1536);
  free(remote_live_upper1p5);
  return 0;
}
