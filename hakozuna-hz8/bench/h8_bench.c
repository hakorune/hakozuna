#include "../include/h8.h"
#include "h8_bench_support.h"

#include <inttypes.h>
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
  size_t medium_candidate_count = 0;
  size_t medium_remote_live_count = 0;
  uint64_t medium_requested_total = 0;
  uint64_t medium_rounded_total = 0;
  uint64_t medium_remote_requested_total = 0;
  uint64_t medium_remote_rounded_total = 0;
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
      medium_candidate_count += th[i].medium_candidate_count;
      medium_remote_live_count += th[i].medium_remote_live_count;
      medium_requested_total += th[i].medium_candidate_requested_bytes;
      medium_rounded_total += th[i].medium_candidate_rounded_bytes;
      medium_remote_requested_total += th[i].medium_remote_live_requested_bytes;
      medium_remote_rounded_total += th[i].medium_remote_live_rounded_bytes;
    }
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
#if defined(H8_BENCH_ATTRIBUTION)
  double frag_ratio = frag_requested_total
                          ? (double)frag_rounded_total /
                                (double)frag_requested_total
                          : 0.0;
  printf("fragmentation requested_bytes=%" PRIu64 " rounded_bytes=%" PRIu64 " rounding_ratio=%.6f\n",
         frag_requested_total, frag_rounded_total, frag_ratio);
  printf("fragmentation_candidates upper1536_rounded_bytes=%" PRIu64 " upper1536_ratio=%.6f upper1p5_rounded_bytes=%" PRIu64 " upper1p5_ratio=%.6f\n",
         frag_upper1536_total,
         frag_requested_total ? (double)frag_upper1536_total /
                                    (double)frag_requested_total
                              : 0.0,
         frag_upper1p5_total,
         frag_requested_total ? (double)frag_upper1p5_total /
                                    (double)frag_requested_total
                              : 0.0);
  printf("fragmentation_by_class class_count=%u allocs=[", H8_CLASS_COUNT);
  for (uint32_t c = 0; c < H8_CLASS_COUNT; ++c) {
    printf("%s%zu", c == 0 ? "" : ",", frag_allocs_by_class[c]);
  }
  printf("] rounded_bytes=[");
  for (uint32_t c = 0; c < H8_CLASS_COUNT; ++c) {
    printf("%s%" PRIu64, c == 0 ? "" : ",", frag_rounded_by_class[c]);
  }
  printf("]\n");
  printf("medium_route_shadow candidate_count=%zu remote_live_count=%zu requested_bytes=%" PRIu64 " rounded_bytes=%" PRIu64 " remote_requested_bytes=%" PRIu64 " remote_rounded_bytes=%" PRIu64 "\n",
         medium_candidate_count, medium_remote_live_count, medium_requested_total,
         medium_rounded_total, medium_remote_requested_total,
         medium_remote_rounded_total);
#else
  printf("fragmentation attribution=disabled\n");
#endif

  H8Stats stats = h8_stats();
  H8DebugStats debug = h8_debug_stats();
  printf("stats_snapshot owner_exit=%zu pending_enqueue=%zu pending_dequeue=%zu orphan_handoff=%zu handoff_ok=%zu local=%zu remote=%zu\n",
         stats.owner_exit_count,
         stats.pending_enqueue_count,
         stats.pending_dequeue_count,
         stats.orphan_handoff_count,
         stats.handoff_success_count,
         stats.local_alloc_count,
         stats.remote_collect_count);
  printf("counters_dbg publish_enter=%zu publish_exit=%zu lifecycle_enter=%zu lifecycle_exit=%zu span_publish_enter=%zu span_publish_exit=%zu remote_regular=%zu remote_orphan=%zu pending_notify=%zu pending_calls=%zu pending_carry_hit=%zu pending_requeue=%zu pending_word_set=%zu pending_word_shadow_hit=%zu pending_word_false_pos=%zu pending_word_false_neg=%zu pending_word_rearm=%zu pending_words=%zu pending_words_nonzero=%zu pending_bits=%zu orphan_quiesce=%zu orphan_ready=%zu dry_scan=%zu dry_candidate=%zu dry_block_state=%zu dry_block_quiesce=%zu dry_empty=%zu dry_target_closed=%zu dry_would_adopt=%zu handoff_fail=%zu invalid=%zu miss=%zu owner_transition=%zu adopt_scan=%zu adopt_candidate=%zu adopt_block_state=%zu adopt_block_quiesce=%zu adopt_empty=%zu adopt_target_closed=%zu adopt_ok=%zu\n",
         debug.owner_publish_enter_count,
         debug.owner_publish_exit_count,
         debug.owner_lifecycle_enter_count,
         debug.owner_lifecycle_exit_count,
         debug.span_publish_enter_count,
         debug.span_publish_exit_count,
         debug.remote_regular_admission_count,
         debug.remote_orphan_admission_count,
         debug.pending_notify_count,
         debug.pending_collect_call_count,
         debug.pending_collect_carry_hit_count,
         debug.pending_collect_requeue_count,
         debug.pending_word_summary_set,
         debug.pending_word_summary_shadow_hit,
         debug.pending_word_summary_false_positive,
         debug.pending_word_summary_false_negative,
         debug.pending_word_summary_rearm,
         debug.pending_collect_word_count,
         debug.pending_collect_word_nonzero_count,
         debug.pending_collect_bit_count,
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
  printf("remote_stage enter=%zu span_miss=%zu owner_missing=%zu regular_lease_ok=%zu regular_lease_fail=%zu regular_lease_elided=%zu orphan_lease_ok=%zu orphan_lease_fail=%zu pending_claim_ok=%zu validate_fail=%zu notify_first=%zu publish_ok=%zu pending_publish_elided=%zu\n",
         debug.remote_stage_enter,
         debug.remote_stage_span_miss,
         debug.remote_stage_owner_missing,
         debug.remote_stage_regular_lease_ok,
         debug.remote_stage_regular_lease_fail,
         debug.remote_stage_regular_lease_elided,
         debug.remote_stage_orphan_lease_ok,
         debug.remote_stage_orphan_lease_fail,
         debug.remote_stage_pending_claim_ok,
         debug.remote_stage_validate_fail,
         debug.remote_stage_notify_first,
         debug.remote_stage_publish_ok,
         debug.remote_stage_pending_publish_elided);
  printf("remote_lookup enter=%zu arena_miss=%zu span_miss=%zu retired=%zu slot_oob=%zu ok=%zu owner_word=%zu\n",
         debug.remote_lookup_enter,
         debug.remote_lookup_arena_miss,
         debug.remote_lookup_span_miss,
         debug.remote_lookup_retired,
         debug.remote_lookup_slot_oob,
         debug.remote_lookup_ok,
         debug.remote_owner_word_load);
  double slots_per_nonzero_word = 0.0;
  double singleton_ratio = 0.0;
  double multi_ratio = 0.0;
  if (debug.pending_word_drain_count != 0) {
    slots_per_nonzero_word =
        (double)debug.pending_slots_drained / (double)debug.pending_word_drain_count;
    singleton_ratio =
        (double)debug.pending_word_popcount_1 / (double)debug.pending_word_drain_count;
    multi_ratio = 1.0 - singleton_ratio;
  }
  printf("pending_word_density drain=%zu pop1=%zu pop2=%zu pop3_4=%zu pop5_8=%zu pop9_16=%zu pop17p=%zu slots=%zu rearmed=%zu new_publish=%zu slots_per_nonzero_word=%.3f singleton_ratio=%.3f multi_ratio=%.3f\n",
         debug.pending_word_drain_count,
         debug.pending_word_popcount_1,
         debug.pending_word_popcount_2,
         debug.pending_word_popcount_3_4,
         debug.pending_word_popcount_5_8,
         debug.pending_word_popcount_9_16,
         debug.pending_word_popcount_17_plus,
         debug.pending_slots_drained,
         debug.pending_words_rearmed,
         debug.pending_word_new_publish_during_drain,
         slots_per_nonzero_word,
         singleton_ratio,
         multi_ratio);
  printf("pending_count_shadow mask_notify_without_count=%zu count_notify_without_mask=%zu mask_requeue_without_count=%zu count_requeue_without_mask=%zu\n",
         debug.pending_mask_notify_without_count,
         debug.pending_count_notify_without_mask,
         debug.pending_mask_requeue_without_count,
         debug.pending_count_requeue_without_mask);
  printf("pending_finish_shadow count_mask0_bitmap0=%zu count_mask0_bitmap1=%zu mask1_bitmap0=%zu mask1_bitmap1=%zu\n",
         debug.pending_finish_count_mask_zero_bitmap_zero,
         debug.pending_finish_count_mask_zero_bitmap_nonzero,
         debug.pending_finish_mask_nonzero_bitmap_zero,
         debug.pending_finish_mask_nonzero_bitmap_nonzero);
  printf("qstate_dirty set=%zu self_set=%zu requeue=%zu\n",
         debug.qstate_dirty_set,
         debug.qstate_dirty_self_set,
         debug.qstate_dirty_requeue);
  printf("quiescent_pending bitmap_nonzero=%zu repair=%zu\n",
         debug.quiescent_pending_bitmap_nonzero,
         debug.quiescent_pending_repair);
  printf("queue_contention qstate_attempt=%zu qstate_success=%zu qstate_skip=%zu pending_push_attempt=%zu pending_push_retry=%zu pending_push_success=%zu owner_enter_retry=%zu owner_exit_retry=%zu duplicate_claim=%zu\n",
         debug.qstate_notify_attempt_count,
         debug.qstate_notify_success_count,
         debug.qstate_notify_skip_nonidle_count,
         debug.pending_head_push_attempt_count,
         debug.pending_head_push_retry_count,
         debug.pending_head_push_success_count,
         debug.owner_lifecycle_enter_cas_retry_count,
         debug.owner_lifecycle_exit_cas_retry_count,
         debug.remote_publish_pending_claim_duplicate_count);
  printf("local_zero_gates alloc_pending=%zu free_pending=%zu live_set_already=%zu live_clear_free=%zu\n",
         debug.local_alloc_pending_nonzero,
         debug.local_free_pending_nonzero,
         debug.owner_live_set_already_live,
         debug.owner_live_clear_already_free);
  printf("local_path active_hit=%zu active_miss=%zu freelist=%zu bump=%zu slow_collect=%zu span_commit=%zu find_scan=%zu scan_span=%zu free_hit=%zu reject_owner=%zu reject_state=%zu reject_live=%zu\n",
         debug.local_active_hit,
         debug.local_active_miss,
         debug.local_freelist_pop,
         debug.local_bump_alloc,
         debug.local_slow_collect,
         debug.local_span_commit,
         debug.local_find_scan,
         debug.local_find_scan_span,
         debug.local_free_hit,
         debug.local_free_reject_owner,
         debug.local_free_reject_state,
         debug.local_free_reject_live);
  printf("medium_stats malloc=%zu create=%zu active_reuse=%zu owner_reuse=%zu global_reuse=%zu madvise=%zu owner_scan=%zu owner_steps=%zu global_scan=%zu global_steps=%zu free_lookup=%zu route_lookup=%zu invalid_owned=%zu empty=%zu retain=%zu budget_reject=%zu reactivate=%zu exit_drain=%zu madvise_fail=%zu resident_bytes=%zu resident_peak=%zu madvise_ms=%.3f global_lock_ms=%.3f run_lock_ms=%.3f alloc_slot_ms=%.3f free_slot_ms=%.3f alloc_slot=%zu free_slot=%zu lock_elide_alloc=%zu lock_elide_free=%zu lock_elide_mismatch=%zu active_owner_mismatch=%zu owner_list_mismatch=%zu global_skip_foreign=%zu local_free_owner=%zu remote_free_owner=%zu free_steps=%zu route_steps=%zu route_authority_mismatch=%zu remote_pub=%zu remote_lease_ms=%.3f remote_run_lock_ms=%.3f remote_claim=%zu remote_claim_ms=%.3f remote_lockless_claim=%zu remote_lockless_accept=%zu remote_lockless_rb_invalid=%zu remote_lockless_rb_accept=%zu writer_overlap=%zu writer_foreign=%zu writer_token_change=%zu collect_wrong_owner=%zu detached_while_attached=%zu remote_shadow_attempt=%zu remote_shadow_accept=%zu remote_shadow_reject=%zu remote_shadow_match=%zu remote_shadow_mismatch=%zu remote_notify=%zu remote_qpush=%zu remote_qpush_ms=%.3f remote_collect_call=%zu remote_collect_run=%zu remote_collect_slot=%zu remote_collect_ms=%.3f\n",
         debug.medium_malloc_count, debug.medium_run_create_count, debug.medium_run_reuse_active_count,
         debug.medium_run_reuse_owner_list_count, debug.medium_run_reuse_global_count, debug.medium_run_madvise_count,
         debug.medium_owner_scan_count, debug.medium_owner_scan_step_count, debug.medium_global_scan_count, debug.medium_global_scan_step_count,
         debug.medium_free_lookup_count, debug.medium_route_lookup_count, debug.medium_invalid_owned_count, debug.medium_empty_transition_count,
         debug.medium_empty_retain_count, debug.medium_empty_budget_reject_count, debug.medium_empty_reactivate_count, debug.medium_owner_exit_drain_count,
         debug.medium_madvise_fail_count, debug.medium_resident_empty_bytes, debug.medium_resident_empty_peak, (double)debug.medium_madvise_ns / 1e6,
         (double)debug.medium_global_lock_wait_ns / 1e6, (double)debug.medium_run_lock_wait_ns / 1e6, (double)debug.medium_alloc_slot_ns / 1e6, (double)debug.medium_free_slot_ns / 1e6,
         debug.medium_alloc_slot_count, debug.medium_free_slot_count, debug.medium_lock_elide_alloc_candidate, debug.medium_lock_elide_free_candidate, debug.medium_lock_elide_owner_mismatch, debug.medium_active_alloc_owner_mismatch, debug.medium_owner_list_owner_mismatch, debug.medium_global_skip_foreign_attached, debug.medium_local_free_owner_match, debug.medium_remote_free_owner_mismatch, debug.medium_free_lookup_step_count, debug.medium_route_lookup_step_count, debug.medium_route_authority_mismatch, debug.medium_remote_publish_count, (double)debug.medium_remote_owner_lease_ns / 1e6, (double)debug.medium_remote_run_lock_ns / 1e6, debug.medium_remote_pending_claim_count, (double)debug.medium_remote_pending_claim_ns / 1e6, debug.medium_remote_lockless_claim_count, debug.medium_remote_lockless_claim_collector_accept, debug.medium_remote_lockless_claim_rollback_invalid, debug.medium_remote_lockless_claim_rollback_accepted, debug.medium_attached_writer_overlap, debug.medium_attached_foreign_mask_writer, debug.medium_owner_token_changed_during_mutation, debug.medium_collect_wrong_owner, debug.medium_detached_direct_free_while_attached, debug.medium_remote_lockless_shadow_attempt, debug.medium_remote_lockless_shadow_would_accept, debug.medium_remote_lockless_shadow_would_reject, debug.medium_remote_lockless_shadow_match, debug.medium_remote_lockless_shadow_mismatch, debug.medium_remote_notify_count, debug.medium_remote_queue_push_count, (double)debug.medium_remote_queue_push_ns / 1e6, debug.medium_remote_collect_call_count, debug.medium_remote_collect_run_count, debug.medium_remote_collect_slot_count, (double)debug.medium_remote_collect_ns / 1e6);
  printf("medium_remote_class pub=[%zu,%zu,%zu,%zu] qpush=[%zu,%zu,%zu,%zu] collect_run=[%zu,%zu,%zu,%zu] collect_slot=[%zu,%zu,%zu,%zu]\n",
         debug.medium_remote_publish_class_8k,
         debug.medium_remote_publish_class_16k,
         debug.medium_remote_publish_class_32k,
         debug.medium_remote_publish_class_64k,
         debug.medium_remote_qpush_class_8k,
         debug.medium_remote_qpush_class_16k,
         debug.medium_remote_qpush_class_32k,
         debug.medium_remote_qpush_class_64k,
         debug.medium_remote_collect_run_class_8k,
         debug.medium_remote_collect_run_class_16k,
         debug.medium_remote_collect_run_class_32k,
         debug.medium_remote_collect_run_class_64k,
         debug.medium_remote_collect_slot_class_8k,
         debug.medium_remote_collect_slot_class_16k,
         debug.medium_remote_collect_slot_class_32k,
         debug.medium_remote_collect_slot_class_64k);
  double medium_slots_per_run_8k =
      debug.medium_remote_collect_run_class_8k
          ? (double)debug.medium_remote_collect_slot_class_8k /
                (double)debug.medium_remote_collect_run_class_8k
          : 0.0;
  double medium_slots_per_run_16k =
      debug.medium_remote_collect_run_class_16k
          ? (double)debug.medium_remote_collect_slot_class_16k /
                (double)debug.medium_remote_collect_run_class_16k
          : 0.0;
  double medium_slots_per_run_32k =
      debug.medium_remote_collect_run_class_32k
          ? (double)debug.medium_remote_collect_slot_class_32k /
                (double)debug.medium_remote_collect_run_class_32k
          : 0.0;
  double medium_slots_per_run_64k =
      debug.medium_remote_collect_run_class_64k
          ? (double)debug.medium_remote_collect_slot_class_64k /
                (double)debug.medium_remote_collect_run_class_64k
          : 0.0;
  printf("medium_remote_class_density slots_per_run=[%.3f,%.3f,%.3f,%.3f]\n",
         medium_slots_per_run_8k,
         medium_slots_per_run_16k,
         medium_slots_per_run_32k,
         medium_slots_per_run_64k);
  size_t lower_median =
      h8_percentile_size_t(span_lower_bound, (size_t)opt.runs, 0.50);
  double actual_per_run =
      opt.runs > 0 ? (double)debug.local_span_commit / (double)opt.runs : 0.0;
  double span_excess_ratio =
      lower_median && debug.local_span_commit
          ? actual_per_run / (double)lower_median
          : 0.0;
  printf("span_commit_lower_bound actual_total=%zu actual_per_run=%.1f lower_bound_median=%zu excess_ratio=%.3f\n",
         debug.local_span_commit, actual_per_run, lower_median, span_excess_ratio);
  printf("local_scan_detail hint_null=%zu hint_full=%zu hint_state_blocked=%zu hint_trusted=%zu hint_class_mismatch=%zu hint_owner_mismatch=%zu hint_generation_mismatch=%zu hint_state_mismatch=%zu scan_usable=%zu scan_full=%zu scan_state_blocked=%zu skip_no_pending=%zu\n",
         debug.local_active_hint_null,
         debug.local_active_hint_full,
         debug.local_active_hint_state_blocked,
         debug.local_active_hint_trusted,
         debug.local_active_hint_class_mismatch,
         debug.local_active_hint_owner_mismatch,
         debug.local_active_hint_generation_mismatch,
         debug.local_active_hint_state_mismatch,
         debug.local_find_scan_span_usable,
         debug.local_find_scan_span_full,
         debug.local_find_scan_span_state_blocked,
         debug.local_find_skip_scan_no_pending);
  printf("local_hot live_alloc=%zu live_free=%zu word0=%zu word1=%zu word2_7=%zu word8_31=%zu word32_63=%zu free_head_alloc=%zu free_head_free=%zu pending_alloc=%zu pending_free=%zu used_alloc=%zu used_free=%zu\n",
         debug.local_live_touch_alloc,
         debug.local_live_touch_free,
         debug.local_live_word_0,
         debug.local_live_word_1,
         debug.local_live_word_2_7,
         debug.local_live_word_8_31,
         debug.local_live_word_32_63,
         debug.local_free_head_touch_alloc,
         debug.local_free_head_touch_free,
         debug.local_pending_check_alloc,
         debug.local_pending_check_free,
         debug.local_used_touch_alloc,
         debug.local_used_touch_free);
  printf("used_count_detail load_alloc=%zu store_alloc=%zu load_free=%zu store_free=%zu full_check=%zu underflow=%zu mirror_mismatch=%zu mirror_underflow=%zu\n",
         debug.local_used_count_load_alloc,
         debug.local_used_count_store_alloc,
         debug.local_used_count_load_free,
         debug.local_used_count_store_free,
         debug.local_used_count_full_check,
         debug.local_used_count_underflow,
         debug.local_used_mirror_mismatch,
         debug.local_used_mirror_underflow);
  printf("used_count_cold active_hint=%zu owner_scan_locked=%zu adoption_locked=%zu owner_exit=%zu verify_quiescent=%zu derived_mismatch=%zu derived_scan=%zu\n",
         debug.local_used_cold_active_hint,
         debug.local_used_cold_owner_scan_locked,
         debug.local_used_cold_adoption_locked,
         debug.local_used_cold_owner_exit,
         debug.local_used_cold_verify_quiescent,
         debug.local_used_derived_mismatch,
         debug.local_used_derived_quiescent_scan);
  printf("span_commit_timing total_ms=%.3f meta_ms=%.3f mprotect_ms=%.3f\n",
         (double)debug.span_commit_total_ns / 1e6,
         (double)debug.span_commit_meta_ns / 1e6,
         (double)debug.span_commit_mprotect_ns / 1e6);
  printf("lifecycle_timing owner_exit_total_ms=%.3f owner_exit_collect_ms=%.3f owner_exit_span_walk_ms=%.3f span_retire_count=%zu span_retire_total_ms=%.3f span_retire_lock_wait_ms=%.3f span_retire_madvise_ms=%.3f span_retire_meta_free_ms=%.3f\n",
         (double)debug.owner_exit_total_ns / 1e6,
         (double)debug.owner_exit_collect_ns / 1e6,
         (double)debug.owner_exit_span_walk_ns / 1e6,
         debug.span_retire_count,
         (double)debug.span_retire_total_ns / 1e6,
         (double)debug.span_retire_lock_wait_ns / 1e6,
         (double)debug.span_retire_madvise_ns / 1e6,
         (double)debug.span_retire_meta_free_ns / 1e6);
  double purge_avg =
      debug.span_purge_run_count
          ? (double)debug.span_purge_run_spans_total /
                (double)debug.span_purge_run_count
          : 0.0;
  printf("span_purge runs=%zu spans=%zu avg_spans_per_run=%.3f max_run=%zu singleton_runs=%zu madvise_calls=%zu madvise_bytes=%zu madvise_ms=%.3f\n",
         debug.span_purge_run_count,
         debug.span_purge_run_spans_total,
         purge_avg,
         debug.span_purge_run_max,
         debug.span_purge_singleton_runs,
         debug.span_purge_madvise_calls,
         debug.span_purge_madvise_bytes,
         (double)debug.span_purge_madvise_ns / 1e6);
  printf("slot_shadow valid_mismatch=%zu invalid_mismatch=%zu pending_nonallocated=%zu free_unreachable=%zu free_duplicate=%zu free_cycle=%zu bad_next=%zu never_used_below_bump=%zu nonvirgin_above_bump=%zu used_mismatch=%zu reserved_quiescent=%zu\n",
         debug.slot_shadow_valid_mismatch,
         debug.slot_shadow_invalid_mismatch,
         debug.slot_shadow_pending_nonallocated,
         debug.slot_shadow_free_unreachable,
         debug.slot_shadow_free_duplicate,
         debug.slot_shadow_free_cycle,
         debug.slot_shadow_bad_next,
         debug.slot_shadow_never_used_below_bump,
         debug.slot_shadow_nonvirgin_above_bump,
         debug.slot_shadow_used_mismatch,
         debug.slot_shadow_reserved_quiescent);

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
