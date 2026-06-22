#include "../include/h8.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/resource.h>

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

static int h8_spsc_push(H8Inbox* inbox, void* ptr) {
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

static void* h8_spsc_pop(H8Inbox* inbox) {
  size_t head = atomic_load_explicit(&inbox->head, memory_order_relaxed);
  size_t tail = atomic_load_explicit(&inbox->tail, memory_order_acquire);
  if (head == tail) {
    return NULL;
  }
  void* ptr = inbox->items[head % inbox->cap];
  atomic_store_explicit(&inbox->head, head + 1u, memory_order_release);
  return ptr;
}

static size_t h8_drain_inbox(H8Inbox* inbox) {
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

static void* h8_bench_thread_interleaved(void* arg) {
  H8BenchThread* th = (H8BenchThread*)arg;
  const H8BenchOptions* opt = th->opt;
  int next = (th->index + 1) % opt->threads;
  int prev = (th->index + opt->threads - 1) % opt->threads;
  H8Inbox* next_inbox = &th->inboxes[next];
  H8Inbox* my_inbox = &th->inboxes[th->index];

  for (size_t i = 0; i < opt->iters_per_thread; ++i) {
    h8_drain_inbox(my_inbox);

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
      while (!h8_spsc_push(next_inbox, ptr)) {
        h8_drain_inbox(my_inbox);
        sched_yield();
      }
    } else {
      h8_free(ptr);
    }
  }

  atomic_store_explicit(&th->done[th->index], 1, memory_order_release);

  for (;;) {
    size_t drained = h8_drain_inbox(my_inbox);
    if (atomic_load_explicit(&th->done[prev], memory_order_acquire) &&
        drained == 0) {
      if (h8_drain_inbox(my_inbox) == 0) {
        break;
      }
    }
    if (drained == 0) {
      sched_yield();
    }
  }

  pthread_barrier_wait(th->barrier);
  return NULL;
}

static void* h8_bench_thread_main(void* arg) {
  H8BenchThread* th = (H8BenchThread*)arg;
  const H8BenchOptions* opt = th->opt;
  if (opt->interleaved) {
    return h8_bench_thread_interleaved(arg);
  }

  int next = (th->index + 1) % opt->threads;
  H8Inbox* next_inbox = &th->inboxes[next];
  H8Inbox* my_inbox = &th->inboxes[th->index];

  uint64_t alloc_start = h8_now_ns();
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
  th->alloc_ns = h8_now_ns() - alloc_start;

  pthread_barrier_wait(th->barrier);

  if (!th->error) {
    uint64_t remote_start = h8_now_ns();
    for (size_t i = 0; i < my_inbox->count; ++i) {
      h8_free(my_inbox->items[i]);
    }
    th->remote_ns = h8_now_ns() - remote_start;
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
          "          [--min-size N] [--max-size N] [--remote-pct N]\n"
          "          [--interleaved 0|1]\n",
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
    } else if (strcmp(a, "--interleaved") == 0 && i + 1 < argc) {
      if (h8_parse_int(argv[++i], &opt->interleaved) != 0) return -1;
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
      .interleaved = 0,
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
  double* alloc_phase_ms = calloc((size_t)opt.runs, sizeof(*alloc_phase_ms));
  double* remote_phase_ms = calloc((size_t)opt.runs, sizeof(*remote_phase_ms));
  size_t* minor_faults = calloc((size_t)opt.runs, sizeof(*minor_faults));
  if (!throughput || !rss || !alloc_phase_ms || !remote_phase_ms ||
      !minor_faults) {
    fprintf(stderr, "bench allocation failed\n");
    free(throughput);
    free(rss);
    free(alloc_phase_ms);
    free(remote_phase_ms);
    free(minor_faults);
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
      free(alloc_phase_ms);
      free(remote_phase_ms);
      free(minor_faults);
      return 1;
    }

    for (int i = 0; i < opt.threads; ++i) {
      inboxes[i].cap = opt.interleaved ? opt.iters_per_thread + 1u
                                       : opt.iters_per_thread;
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
    throughput[run] = ops / seconds;
    rss[run] = h8_read_rss_bytes();
    long minflt_delta = usage_after.ru_minflt - usage_before.ru_minflt;
    minor_faults[run] = minflt_delta > 0 ? (size_t)minflt_delta : 0;
    alloc_phase_ms[run] = (double)alloc_ns_max / 1e6;
    remote_phase_ms[run] = (double)remote_ns_max / 1e6;
    printf("run=%d ops/s=%.3f rss=%zu\n", run + 1, throughput[run], rss[run]);
    if (!opt.interleaved) {
      printf("run_phase=%d alloc_ms=%.3f remote_ms=%.3f\n", run + 1,
             alloc_phase_ms[run], remote_phase_ms[run]);
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
  qsort(minor_faults, (size_t)opt.runs, sizeof(*minor_faults), h8_cmp_size_t);
  qsort(alloc_phase_ms, (size_t)opt.runs, sizeof(*alloc_phase_ms), h8_cmp_double);
  qsort(remote_phase_ms, (size_t)opt.runs, sizeof(*remote_phase_ms), h8_cmp_double);

  printf("summary runs=%d threads=%d iters=%zu size=%zu..%zu remote_pct=%d interleaved=%d\n",
         opt.runs, opt.threads, opt.iters_per_thread, opt.min_size, opt.max_size,
         opt.remote_pct, opt.interleaved);
  printf("throughput median=%.3f p25=%.3f p75=%.3f min=%.3f max=%.3f\n",
         h8_percentile(throughput, (size_t)opt.runs, 0.50),
         h8_percentile(throughput, (size_t)opt.runs, 0.25),
         h8_percentile(throughput, (size_t)opt.runs, 0.75),
         throughput[0], throughput[(size_t)opt.runs - 1u]);
  printf("rss median=%zu min=%zu max=%zu\n",
         h8_percentile_size_t(rss, (size_t)opt.runs, 0.50),
         rss[0], rss[(size_t)opt.runs - 1u]);
  printf("page_faults minor_median=%zu minor_min=%zu minor_max=%zu\n",
         h8_percentile_size_t(minor_faults, (size_t)opt.runs, 0.50),
         minor_faults[0], minor_faults[(size_t)opt.runs - 1u]);
  if (!opt.interleaved) {
    printf("phase_ms alloc_median=%.3f remote_median=%.3f\n",
           h8_percentile(alloc_phase_ms, (size_t)opt.runs, 0.50),
           h8_percentile(remote_phase_ms, (size_t)opt.runs, 0.50));
  }

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
  printf("counters_dbg publish_enter=%zu publish_exit=%zu lifecycle_enter=%zu lifecycle_exit=%zu span_publish_enter=%zu span_publish_exit=%zu remote_regular=%zu remote_orphan=%zu pending_notify=%zu pending_calls=%zu pending_carry_hit=%zu pending_requeue=%zu pending_word_set=%zu pending_word_shadow_hit=%zu pending_word_false_pos=%zu pending_word_false_neg=%zu pending_word_rearm=%zu pending_word_repair=%zu pending_words=%zu pending_words_nonzero=%zu pending_bits=%zu orphan_quiesce=%zu orphan_ready=%zu dry_scan=%zu dry_candidate=%zu dry_block_state=%zu dry_block_quiesce=%zu dry_empty=%zu dry_target_closed=%zu dry_would_adopt=%zu handoff_fail=%zu invalid=%zu miss=%zu owner_transition=%zu adopt_scan=%zu adopt_candidate=%zu adopt_block_state=%zu adopt_block_quiesce=%zu adopt_empty=%zu adopt_target_closed=%zu adopt_ok=%zu\n",
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
         debug.pending_word_summary_repair,
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
  printf("pending_finish_shadow count_mask0_bitmap0=%zu count_mask0_bitmap1=%zu mask1_bitmap0=%zu mask1_bitmap1=%zu publish_arm_raced_nonempty=%zu\n",
         debug.pending_finish_count_mask_zero_bitmap_zero,
         debug.pending_finish_count_mask_zero_bitmap_nonzero,
         debug.pending_finish_mask_nonzero_bitmap_zero,
         debug.pending_finish_mask_nonzero_bitmap_nonzero,
         debug.pending_publish_mask_arm_raced_nonempty);
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
  printf("local_scan_detail hint_null=%zu hint_full=%zu hint_state_blocked=%zu scan_usable=%zu scan_full=%zu scan_state_blocked=%zu skip_no_pending=%zu\n",
         debug.local_active_hint_null,
         debug.local_active_hint_full,
         debug.local_active_hint_state_blocked,
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
  printf("span_commit_timing total_ms=%.3f lock_wait_ms=%.3f table_scan_ms=%.3f meta_ms=%.3f mprotect_ms=%.3f\n",
         (double)debug.span_commit_total_ns / 1e6,
         (double)debug.span_commit_lock_wait_ns / 1e6,
         (double)debug.span_commit_table_scan_ns / 1e6,
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
  free(alloc_phase_ms);
  free(remote_phase_ms);
  free(minor_faults);
  return 0;
}
