#include "../src/hz10_pagemap.h"
#include "../src/hz10_platform.h"
#include "../src/hz10_pooled_page.h"
#include "../src/hz10_remote_stack.h"
#include "../src/hz10_retired_ready.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * HZ10PublicFreeStageCost-L0 (docs/HZ10_SPEED_ATTACK_PLAN_L0.md).
 *
 * Decomposes hz10_free()'s public-free cost into named stages -- route,
 * local free, remote claim (plus its internal classification-recomputation
 * sub-cost), remote ready-note, remote publish, and the full remote-free
 * composite -- so the additivity of the pieces can be checked against the
 * measured whole, instead of assuming isolated single-purpose benches
 * (bench/hz10_pagemap_route_bench.c, bench/hz10_remote_rmw_microbench.c)
 * already explain the integrated path.
 *
 * Deliberately does NOT call hz10_malloc()/hz10_free(): each worker thread
 * owns a private set of PAGES pages (slot_count=1, one full quantum each,
 * matching this project's established "slot_count1" row shape) and
 * round-robins through them, matching the route bench's PAGES-style cache
 * dispersion instead of hammering one hot pointer. This is a cost-
 * attribution bench, not a contention bench -- cross-thread remote-free
 * contention is bench/hz10_remote_rmw_microbench.c's job; every stage here
 * runs single-threaded against a thread's OWN pages so route/claim/publish
 * costs are measured without also mixing in CAS contention.
 *
 * Every mode leaves each page at exactly the same baseline in between
 * timed rounds (one live, allocated, unclaimed pointer, empty remote
 * stack) via an untimed pre-phase (before the start barrier) and/or
 * post-phase (after the done barrier) specific to that mode -- see
 * worker_pre()/worker_post() below. Only the barrier-bounded window is
 * charged to the reported ns/op.
 */

typedef enum StageMode {
  STAGE_ROUTE = 0,
  STAGE_ROUTE_FAST,
  STAGE_LOCAL_FREE,
  STAGE_CLASSIFY,
  STAGE_CLAIM,
  STAGE_READY_NOTE,
  STAGE_PUBLISH,
  STAGE_FULL_REMOTE,
  STAGE_COUNT
} StageMode;

static const char* stage_name(StageMode s) {
  switch (s) {
    case STAGE_ROUTE:
      return "route";
    case STAGE_ROUTE_FAST:
      return "route_fast";
    case STAGE_LOCAL_FREE:
      return "local_free";
    case STAGE_CLASSIFY:
      return "claim_classify_recompute";
    case STAGE_CLAIM:
      return "remote_claim";
    case STAGE_READY_NOTE:
      return "remote_ready_note";
    case STAGE_PUBLISH:
      return "remote_publish";
    case STAGE_FULL_REMOTE:
      return "full_remote_free";
    default:
      return "unknown";
  }
}

typedef struct WorkerPage {
  Hz10FreelistPage* page;
  void* ptr; /* the single slot_count=1 address; constant for the page's
             * entire lifetime once first allocated (LIFO, one slot). */
} WorkerPage;

typedef struct BenchState {
  /*
   * Three barriers, not two -- load-bearing, not decorative. An earlier
   * version of this bench used only start/done and read `mode` once at
   * the top of the worker loop, before its own pre-phase. That left a gap
   * with no synchronization point between "workers finish the previous
   * round's untimed post-phase (and this round's untimed pre-phase)" and
   * "main takes the next round's start timestamp": main takes the
   * timestamp and immediately blocks on start_barrier, so any pre/post
   * housekeeping still in flight on the worker side got silently counted
   * inside the NEXT round's elapsed time instead of being excluded. This
   * measured as a large, mode-dependent, reproducible inflation (modes
   * with more untimed housekeeping, e.g. remote_claim's publish+drain+
   * alloc reset, measured slower than modes with none) and produced the
   * impossible result of full_remote_free measuring faster than its own
   * claim+publish sub-costs. ready_barrier fixes this: main only takes
   * its timestamp AFTER every worker has already finished the previous
   * round's post-phase and this round's pre-phase (including re-reading
   * `mode`, which also fixes a second, smaller race at stage-transition
   * boundaries -- see worker_main()).
   */
  pthread_barrier_t ready_barrier;
  pthread_barrier_t start_barrier;
  pthread_barrier_t done_barrier;
  _Atomic(int) stop;
  _Atomic(uint32_t) mode;
  uint32_t threads;
  uint32_t pages_per_thread;
  uint64_t repeat;
  _Atomic(uint64_t) checksum;
  _Atomic(uint64_t) failed;
} BenchState;

typedef struct WorkerArg {
  BenchState* state;
  WorkerPage* pages;
  uint32_t page_count;
  char token_storage; /* address is this worker's owner_thread_token */
} WorkerArg;

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

/* Untimed, runs immediately before the start barrier every round. Only
 * STAGE_PUBLISH needs one: publish() requires an outstanding claim, and the
 * claim itself must not be inside the timed window. */
static void worker_pre(StageMode mode, WorkerPage* wp) {
  if (mode == STAGE_PUBLISH) {
    uint32_t slot_index;
    if (!hz10_page_remote_free_claim(wp->page, wp->ptr, HZ10_GENERATION_ANY,
                                    &slot_index)) {
      fprintf(stderr, "stage-cost: pre-claim for publish failed\n");
    }
  }
}

/* Untimed, runs immediately after the done barrier every round: restores
 * the page to the shared baseline (one live, allocated, unclaimed pointer,
 * nothing outstanding on the remote stack) before the next round's
 * pre-phase/timed section runs. */
static void worker_post(StageMode mode, WorkerPage* wp) {
  switch (mode) {
    case STAGE_LOCAL_FREE:
      wp->ptr = hz10_freelist_page_alloc(wp->page);
      break;
    case STAGE_CLAIM:
      hz10_page_remote_free_publish(wp->page, wp->ptr);
      hz10_page_drain_remote(wp->page);
      wp->ptr = hz10_freelist_page_alloc(wp->page);
      break;
    case STAGE_PUBLISH:
    case STAGE_FULL_REMOTE:
      hz10_page_drain_remote(wp->page);
      wp->ptr = hz10_freelist_page_alloc(wp->page);
      break;
    case STAGE_ROUTE:
    case STAGE_ROUTE_FAST:
    case STAGE_CLASSIFY:
    case STAGE_READY_NOTE:
    default:
      break; /* read-only or genuinely no-op stages: nothing to undo */
  }
  if (!wp->ptr) {
    fprintf(stderr, "stage-cost: worker_post left a NULL ptr (mode=%s)\n",
            stage_name(mode));
  }
}

static void worker_timed(StageMode mode, WorkerPage* wp, void* token,
                        uint64_t* local_checksum, uint64_t* local_failed) {
  switch (mode) {
    case STAGE_ROUTE: {
      H10RouteResult r = hz10_pagemap_route(wp->ptr, wp->page->generation);
      if (r.kind != H10_ROUTE_VALID) {
        *local_failed += 1u;
      }
      *local_checksum += r.slot_index + r.generation;
      break;
    }
    case STAGE_ROUTE_FAST: {
      H10RouteLocalResult r = {0};
      if (!hz10_pagemap_route_local_fast(wp->ptr, &r) ||
          r.owner != wp->page) {
        *local_failed += 1u;
      }
      *local_checksum += r.generation + r.slot_size;
      break;
    }
    case STAGE_LOCAL_FREE: {
      if (wp->page->owner_thread_token == token) {
        hz10_freelist_page_free(wp->page, wp->ptr);
      } else {
        *local_failed += 1u;
      }
      break;
    }
    case STAGE_CLASSIFY: {
      uint32_t slot_index = 0u;
      int ok = hz10_page_classify_for_remote_probe(
          wp->page, wp->ptr, HZ10_GENERATION_ANY, &slot_index);
      if (!ok) {
        *local_failed += 1u;
      }
      *local_checksum += slot_index;
      break;
    }
    case STAGE_CLAIM: {
      uint32_t slot_index = 0u;
      if (!hz10_page_remote_free_claim(wp->page, wp->ptr, HZ10_GENERATION_ANY,
                                      &slot_index)) {
        *local_failed += 1u;
      }
      *local_checksum += slot_index;
      break;
    }
    case STAGE_READY_NOTE: {
      hz10_retired_ready_note_remote_free(wp->page);
      break;
    }
    case STAGE_PUBLISH: {
      hz10_page_remote_free_publish(wp->page, wp->ptr);
      break;
    }
    case STAGE_FULL_REMOTE: {
      H10RouteResult r = hz10_pagemap_route(wp->ptr, HZ10_GENERATION_ANY);
      if (r.kind != H10_ROUTE_VALID || r.owner != wp->page) {
        *local_failed += 1u;
        break;
      }
      uint32_t slot_index;
      if (hz10_page_remote_free_claim(wp->page, wp->ptr, r.generation,
                                      &slot_index)) {
        hz10_retired_ready_note_remote_free(wp->page);
        hz10_page_remote_free_publish(wp->page, wp->ptr);
      } else {
        *local_failed += 1u;
      }
      break;
    }
    default:
      break;
  }
}

/*
 * Round structure per iteration (see BenchState's ready_barrier comment for
 * why three barriers, not two): wait on ready_barrier (this is the point
 * every prior round's post-phase AND this round's mode read + pre-phase
 * must already be complete), then start_barrier brackets the timed
 * section, then done_barrier, then this round's post-phase runs before
 * looping back to ready_barrier for the next round. Main's run_stage()
 * takes its timestamp only after its own ready_barrier wait returns, so
 * none of the untimed housekeeping on either side can leak into elapsed
 * time.
 */
static void* worker_main(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  BenchState* state = arg->state;
  void* token = (void*)&arg->token_storage;
  for (;;) {
    StageMode mode =
        (StageMode)atomic_load_explicit(&state->mode, memory_order_acquire);
    int stop = atomic_load_explicit(&state->stop, memory_order_acquire);
    if (!stop) {
      for (uint32_t p = 0; p < arg->page_count; ++p) {
        worker_pre(mode, &arg->pages[p]);
      }
    }

    pthread_barrier_wait(&state->ready_barrier);
    pthread_barrier_wait(&state->start_barrier);
    if (atomic_load_explicit(&state->stop, memory_order_acquire)) {
      pthread_barrier_wait(&state->done_barrier);
      return NULL;
    }

    uint64_t local_checksum = 0u;
    uint64_t local_failed = 0u;
    for (uint32_t p = 0; p < arg->page_count; ++p) {
      worker_timed(mode, &arg->pages[p], token, &local_checksum,
                  &local_failed);
    }

    pthread_barrier_wait(&state->done_barrier);
    atomic_fetch_add_explicit(&state->checksum, local_checksum,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&state->failed, local_failed,
                              memory_order_relaxed);

    for (uint32_t p = 0; p < arg->page_count; ++p) {
      worker_post(mode, &arg->pages[p]);
    }
  }
}

static uint64_t run_stage(BenchState* state, StageMode mode) {
  atomic_store_explicit(&state->mode, (uint32_t)mode, memory_order_release);
  atomic_store_explicit(&state->checksum, 0u, memory_order_relaxed);
  atomic_store_explicit(&state->failed, 0u, memory_order_relaxed);

  uint64_t elapsed_ns = 0u;
  for (uint64_t r = 0; r < state->repeat; ++r) {
    pthread_barrier_wait(&state->ready_barrier);
    uint64_t start = hz10_platform_now_ns();
    pthread_barrier_wait(&state->start_barrier);
    pthread_barrier_wait(&state->done_barrier);
    elapsed_ns += hz10_platform_now_ns() - start;
  }
  return elapsed_ns;
}

static void report(const char* mode, uint32_t run, uint32_t runs,
                   uint32_t threads, uint64_t ops, uint64_t elapsed_ns) {
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf("hz10_public_entry_stage_cost stage=%s run=%u/%u threads=%u "
        "ops=%llu seconds=%.6f ns_per_op=%.4f\n",
        mode, run, runs, threads, (unsigned long long)ops, seconds,
        (double)elapsed_ns / (double)ops);
}

int main(void) {
  BenchState state = {0};
  state.threads = (uint32_t)env_u64("THREADS", 4ull);
  uint32_t pages_per_thread = (uint32_t)env_u64("PAGES", 4096ull);
  state.pages_per_thread = pages_per_thread;
  state.repeat = env_u64("REPEAT", 20000ull);
  uint32_t runs = (uint32_t)env_u64("RUNS", 5ull);
  uint32_t remote_pct = (uint32_t)env_u64("REMOTE_PCT", 50ull);

  if (state.threads == 0u || pages_per_thread == 0u || state.repeat == 0u ||
      runs == 0u || remote_pct > 100u) {
    fprintf(stderr, "invalid THREADS/PAGES/REPEAT/RUNS/REMOTE_PCT\n");
    return 1;
  }

  pthread_barrier_init(&state.ready_barrier, NULL, state.threads + 1u);
  pthread_barrier_init(&state.start_barrier, NULL, state.threads + 1u);
  pthread_barrier_init(&state.done_barrier, NULL, state.threads + 1u);

  pthread_t* threads = calloc(state.threads, sizeof(*threads));
  WorkerArg* args = calloc(state.threads, sizeof(*args));
  if (!threads || !args) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  for (uint32_t t = 0; t < state.threads; ++t) {
    args[t].state = &state;
    args[t].page_count = pages_per_thread;
    args[t].pages = calloc(pages_per_thread, sizeof(*args[t].pages));
    if (!args[t].pages) {
      fprintf(stderr, "allocation failure (pages)\n");
      return 1;
    }
    for (uint32_t p = 0; p < pages_per_thread; ++p) {
      Hz10FreelistPage* page =
          hz10_pooled_page_create_with_owner(HZ10_PAGE_QUANTUM, 1u);
      if (!page) {
        fprintf(stderr, "setup: page create failed at t=%u p=%u\n", t, p);
        return 1;
      }
      hz10_freelist_page_set_owner_thread(page, (void*)&args[t].token_storage);
      void* ptr = hz10_freelist_page_alloc(page);
      if (!ptr) {
        fprintf(stderr, "setup: initial alloc failed at t=%u p=%u\n", t, p);
        return 1;
      }
      args[t].pages[p].page = page;
      args[t].pages[p].ptr = ptr;
    }
    if (pthread_create(&threads[t], NULL, worker_main, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }

  printf("hz10_public_entry_stage_cost threads=%u pages_per_thread=%u "
        "repeat=%llu runs=%u remote_pct=%u local_pct=%u\n",
        state.threads, pages_per_thread, (unsigned long long)state.repeat,
        runs, remote_pct, 100u - remote_pct);
  printf("hz10_public_entry_stage_cost existing_bench_citations: "
        "run `PAGES=%u make -C hakozuna-hz10 bench-pagemap-route` for the "
        "isolated route-lookup number and `REPEAT=%llu make -C "
        "hakozuna-hz10 bench-remote-rmw-micro` for the isolated publish/"
        "full-publish contention ceiling; cite both alongside this run in "
        "current_task.md, do not recompute them here\n",
        pages_per_thread, (unsigned long long)state.repeat);

  int any_failed = 0;
  uint64_t ops_per_round = (uint64_t)state.threads * pages_per_thread;

  for (uint32_t run = 1u; run <= runs; ++run) {
    double route_ns = 0.0, route_fast_ns = 0.0, local_ns = 0.0,
          classify_ns = 0.0, claim_ns = 0.0, ready_note_ns = 0.0,
          publish_ns = 0.0, full_ns = 0.0;
    for (StageMode s = 0; s < STAGE_COUNT; ++s) {
      uint64_t ops = ops_per_round * state.repeat;
      uint64_t elapsed = run_stage(&state, s);
      report(stage_name(s), run, runs, state.threads, ops, elapsed);
      if (atomic_load_explicit(&state.failed, memory_order_relaxed) != 0u) {
        fprintf(stderr, "stage-cost: stage=%s reported %llu failures\n",
                stage_name(s),
                (unsigned long long)atomic_load_explicit(
                    &state.failed, memory_order_relaxed));
        any_failed = 1;
      }
      double ns_per_op = (double)elapsed / (double)ops;
      switch (s) {
        case STAGE_ROUTE:
          route_ns = ns_per_op;
          break;
        case STAGE_ROUTE_FAST:
          route_fast_ns = ns_per_op;
          break;
        case STAGE_LOCAL_FREE:
          local_ns = ns_per_op;
          break;
        case STAGE_CLASSIFY:
          classify_ns = ns_per_op;
          break;
        case STAGE_CLAIM:
          claim_ns = ns_per_op;
          break;
        case STAGE_READY_NOTE:
          ready_note_ns = ns_per_op;
          break;
        case STAGE_PUBLISH:
          publish_ns = ns_per_op;
          break;
        case STAGE_FULL_REMOTE:
          full_ns = ns_per_op;
          break;
        default:
          break;
      }
    }

    double sum_stages = route_ns + claim_ns + ready_note_ns + publish_ns;
    double interaction_delta = full_ns - sum_stages;
    double local_pct = (double)(100u - remote_pct) / 100.0;
    double remote_pct_f = (double)remote_pct / 100.0;
    /* As specified in docs/HZ10_SPEED_ATTACK_PLAN_L0.md: sum of the
     * isolated per-stage numbers, weighted by how often each stage's
     * caller actually runs it. Reported for citation, but see the
     * corrected_ variant below and the note on this printf -- a large
     * interaction_delta means this raw sum is not the trustworthy number
     * for picking the next optimization target. */
    double weighted_raw_sum = route_ns * 1.0 + local_ns * local_pct +
                             claim_ns * remote_pct_f +
                             ready_note_ns * remote_pct_f +
                             publish_ns * remote_pct_f;
    /* Corrected variant: when interaction_delta is large, the isolated
     * claim/ready_note/publish numbers do not reflect their real adjacent-
     * call cost (see the ready_barrier comment on BenchState -- even with
     * that timing bug fixed, running each stage as its own repeated full
     * sweep over `pages_per_thread` distinct pages is measurably colder,
     * cache-wise, than the three calls running back-to-back on the same
     * page inside one hz10_free()). full_remote_free_ns is measured with
     * the real adjacency (route+claim+ready_note+publish on the same page,
     * same iteration), so subtracting its own internal route_ns out and
     * weighting the remainder by remote_pct avoids both the double-count
     * (route already counted once, unconditionally, in the term below)
     * and the inflated isolated-stage sum. This is the number to use for
     * the "largest weighted contribution" decision in
     * docs/HZ10_SPEED_ATTACK_PLAN_L0.md, not weighted_raw_sum, whenever
     * interaction_delta_ns is large. */
    double weighted_corrected =
        route_ns * 1.0 + local_ns * local_pct +
        (full_ns - route_ns) * remote_pct_f;
    printf(
        "hz10_public_entry_stage_cost_additivity run=%u/%u "
        "sum_stages_ns=%.4f full_remote_free_ns=%.4f "
        "interaction_delta_ns=%.4f "
        "weighted_ns_per_logical_free_raw_sum=%.4f "
        "weighted_ns_per_logical_free_corrected=%.4f "
        "note=classify_ns_%.4f_is_a_claim_ns_sub_breakdown_not_summed; "
        "use corrected (not raw_sum) for the attack-order decision when "
        "abs(interaction_delta_ns) is large\n",
        run, runs, sum_stages, full_ns, interaction_delta, weighted_raw_sum,
        weighted_corrected, classify_ns);
    printf(
        "hz10_public_entry_stage_cost_route_fast_delta run=%u/%u "
        "route_ns=%.4f route_fast_ns=%.4f delta_ns=%.4f\n",
        run, runs, route_ns, route_fast_ns, route_fast_ns - route_ns);
  }

  atomic_store_explicit(&state.stop, 1, memory_order_release);
  pthread_barrier_wait(&state.ready_barrier);
  pthread_barrier_wait(&state.start_barrier);
  pthread_barrier_wait(&state.done_barrier);
  for (uint32_t t = 0; t < state.threads; ++t) {
    pthread_join(threads[t], NULL);
  }

  pthread_barrier_destroy(&state.ready_barrier);
  pthread_barrier_destroy(&state.start_barrier);
  pthread_barrier_destroy(&state.done_barrier);
  for (uint32_t t = 0; t < state.threads; ++t) {
    free(args[t].pages);
  }
  free(args);
  free(threads);
  return any_failed ? 1 : 0;
}
