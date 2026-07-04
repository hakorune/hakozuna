#include "../src/hz10_freelist_page.h"
#include "../src/hz10_platform.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Remote-free RMW attribution microbench.
 *
 * This deliberately does not call hz10_free(). It isolates the atomic
 * mechanics the real path pays after routing has already found a page:
 * pending-bit fetch_or, remote_push_count fetch_add, Treiber-stack CAS
 * publish, and owner-side exchange/fetch_and drain. Workers are persistent
 * so the measured loop is not dominated by pthread_create/join.
 */

typedef enum BenchMode {
  MODE_PENDING = 0,
  MODE_TREIBER = 1,
  MODE_PENDING_TREIBER = 2,
  MODE_COUNTER_TREIBER = 3,
  MODE_FULL_PUBLISH = 4,
  MODE_COUNT
} BenchMode;

typedef struct BenchNode {
  struct BenchNode* next;
  char pad[64 - sizeof(struct BenchNode*)];
} BenchNode;

typedef struct BenchState {
  pthread_barrier_t start_barrier;
  pthread_barrier_t done_barrier;
  BenchNode* nodes;
  _Atomic(void*) heads[HZ10_REMOTE_STRIPE_COUNT];
  _Atomic(uint64_t)* pending_words;
  _Atomic(uint64_t) counter;
  _Atomic(int) stop;
  uint32_t slot_count;
  uint32_t pending_word_count;
  uint32_t threads;
  uint64_t repeat;
  _Atomic(uint32_t) mode;
  _Atomic(uint64_t) failed;
} BenchState;

typedef struct WorkerArg {
  BenchState* state;
  uint32_t tid;
  uint32_t begin;
  uint32_t end;
  uint32_t stripe;
} WorkerArg;

static uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = getenv(name);
  if (!value || !value[0]) {
    return fallback;
  }
  return strtoull(value, NULL, 10);
}

static const char* mode_name(BenchMode mode) {
  switch (mode) {
    case MODE_PENDING:
      return "pending_fetch_or";
    case MODE_TREIBER:
      return "treiber_push";
    case MODE_PENDING_TREIBER:
      return "pending_plus_treiber";
    case MODE_COUNTER_TREIBER:
      return "counter_plus_treiber";
    case MODE_FULL_PUBLISH:
      return "pending_counter_treiber";
    default:
      return "unknown";
  }
}

static void pending_claim(BenchState* state, uint32_t slot) {
  uint32_t word = slot / 64u;
  uint64_t bit = UINT64_C(1) << (slot % 64u);
  uint64_t old = atomic_fetch_or_explicit(&state->pending_words[word], bit,
                                         memory_order_acq_rel);
  if ((old & bit) != 0u) {
    atomic_fetch_add_explicit(&state->failed, 1u, memory_order_relaxed);
  }
}

static void treiber_push(BenchState* state, BenchNode* node,
                         uint32_t stripe) {
  _Atomic(void*)* head = &state->heads[stripe];
  void* old = atomic_load_explicit(head, memory_order_relaxed);
  do {
    node->next = (BenchNode*)old;
  } while (!atomic_compare_exchange_weak_explicit(
      head, &old, node, memory_order_release, memory_order_relaxed));
}

static void* worker_main(void* raw) {
  WorkerArg* arg = (WorkerArg*)raw;
  BenchState* state = arg->state;
  for (;;) {
    pthread_barrier_wait(&state->start_barrier);
    if (atomic_load_explicit(&state->stop, memory_order_acquire)) {
      pthread_barrier_wait(&state->done_barrier);
      return NULL;
    }
    BenchMode mode =
        (BenchMode)atomic_load_explicit(&state->mode, memory_order_acquire);
    for (uint32_t i = arg->begin; i < arg->end; ++i) {
      if (mode == MODE_PENDING || mode == MODE_PENDING_TREIBER ||
          mode == MODE_FULL_PUBLISH) {
        pending_claim(state, i);
      }
      if (mode == MODE_COUNTER_TREIBER || mode == MODE_FULL_PUBLISH) {
        atomic_fetch_add_explicit(&state->counter, 1u,
                                  memory_order_relaxed);
      }
      if (mode == MODE_TREIBER || mode == MODE_PENDING_TREIBER ||
          mode == MODE_COUNTER_TREIBER || mode == MODE_FULL_PUBLISH) {
        treiber_push(state, &state->nodes[i], arg->stripe);
      }
    }
    pthread_barrier_wait(&state->done_barrier);
  }
}

static uint32_t drain_stacks(BenchState* state, int clear_pending) {
  uint32_t drained = 0u;
  for (uint32_t s = 0u; s < HZ10_REMOTE_STRIPE_COUNT; ++s) {
    BenchNode* head = (BenchNode*)atomic_exchange_explicit(
        &state->heads[s], NULL, memory_order_acquire);
    while (head) {
      BenchNode* next = head->next;
      if (clear_pending) {
        uint32_t slot = (uint32_t)(head - state->nodes);
        uint32_t word = slot / 64u;
        uint64_t bit = UINT64_C(1) << (slot % 64u);
        atomic_fetch_and_explicit(&state->pending_words[word], ~bit,
                                  memory_order_acq_rel);
      }
      drained += 1u;
      head = next;
    }
  }
  return drained;
}

static uint64_t run_mode(BenchState* state, BenchMode mode,
                         uint64_t* owner_ns_out) {
  atomic_store_explicit(&state->mode, (uint32_t)mode, memory_order_release);
  atomic_store_explicit(&state->counter, 0u, memory_order_relaxed);
  atomic_store_explicit(&state->failed, 0u, memory_order_relaxed);
  for (uint32_t w = 0; w < state->pending_word_count; ++w) {
    atomic_store_explicit(&state->pending_words[w], 0u, memory_order_relaxed);
  }
  for (uint32_t s = 0; s < HZ10_REMOTE_STRIPE_COUNT; ++s) {
    atomic_store_explicit(&state->heads[s], NULL, memory_order_relaxed);
  }

  uint64_t elapsed_ns = 0u;
  uint64_t owner_ns = 0u;
  for (uint64_t r = 0; r < state->repeat; ++r) {
    uint64_t start = hz10_platform_now_ns();
    pthread_barrier_wait(&state->start_barrier);
    pthread_barrier_wait(&state->done_barrier);
    elapsed_ns += hz10_platform_now_ns() - start;

    start = hz10_platform_now_ns();
    uint32_t drained = 0u;
    if (mode == MODE_TREIBER || mode == MODE_PENDING_TREIBER ||
        mode == MODE_COUNTER_TREIBER || mode == MODE_FULL_PUBLISH) {
      drained = drain_stacks(state, mode == MODE_PENDING_TREIBER ||
                                        mode == MODE_FULL_PUBLISH);
      if (drained != state->slot_count) {
        atomic_fetch_add_explicit(&state->failed, 1u,
                                  memory_order_relaxed);
      }
    } else {
      for (uint32_t w = 0; w < state->pending_word_count; ++w) {
        atomic_store_explicit(&state->pending_words[w], 0u,
                              memory_order_relaxed);
      }
    }
    owner_ns += hz10_platform_now_ns() - start;
  }
  *owner_ns_out = owner_ns;
  return elapsed_ns;
}

static void report(const char* phase, const char* mode, uint32_t threads,
                   uint64_t ops, uint64_t elapsed_ns) {
  double seconds = (double)elapsed_ns / 1000000000.0;
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }
  printf("hz10_remote_rmw_micro phase=%s mode=%s threads=%u ops=%llu "
         "seconds=%.6f ops_per_s=%.2f ns_per_op=%.2f\n",
         phase, mode, threads, (unsigned long long)ops, seconds,
         (double)ops / seconds, (double)elapsed_ns / (double)ops);
}

int main(void) {
  BenchState state = {0};
  state.threads = (uint32_t)env_u64("THREADS", 4ull);
  state.slot_count = (uint32_t)env_u64("SLOT_COUNT", 4096ull);
  state.repeat = env_u64("REPEAT", 20000ull);
  uint32_t runs = (uint32_t)env_u64("RUNS", 3ull);
  if (state.threads == 0u || state.slot_count == 0u ||
      (state.slot_count % state.threads) != 0u || state.repeat == 0u ||
      runs == 0u) {
    fprintf(stderr, "invalid THREADS/SLOT_COUNT/REPEAT/RUNS\n");
    return 1;
  }

  state.pending_word_count = (state.slot_count + 63u) / 64u;
  state.nodes = calloc(state.slot_count, sizeof(*state.nodes));
  state.pending_words = calloc(state.pending_word_count,
                               sizeof(*state.pending_words));
  pthread_t* threads = calloc(state.threads, sizeof(*threads));
  WorkerArg* args = calloc(state.threads, sizeof(*args));
  if (!state.nodes || !state.pending_words || !threads || !args) {
    fprintf(stderr, "allocation failure\n");
    return 1;
  }

  pthread_barrier_init(&state.start_barrier, NULL, state.threads + 1u);
  pthread_barrier_init(&state.done_barrier, NULL, state.threads + 1u);
  uint32_t per_thread = state.slot_count / state.threads;
  for (uint32_t t = 0; t < state.threads; ++t) {
    args[t].state = &state;
    args[t].tid = t;
    args[t].begin = t * per_thread;
    args[t].end = (t + 1u == state.threads) ? state.slot_count
                                             : (t + 1u) * per_thread;
    args[t].stripe = t & (HZ10_REMOTE_STRIPE_COUNT - 1u);
    if (pthread_create(&threads[t], NULL, worker_main, &args[t]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }

  printf("hz10_remote_rmw_micro threads=%u slot_count=%u repeat=%llu "
         "runs=%u stripes=%u\n",
         state.threads, state.slot_count, (unsigned long long)state.repeat,
         runs, (unsigned)HZ10_REMOTE_STRIPE_COUNT);

  int failed = 0;
  uint64_t ops = (uint64_t)state.slot_count * state.repeat;
  for (uint32_t run = 1u; run <= runs; ++run) {
    for (BenchMode mode = 0; mode < MODE_COUNT; ++mode) {
      uint64_t owner_ns = 0u;
      uint64_t worker_ns = run_mode(&state, mode, &owner_ns);
      const char* name = mode_name(mode);
      printf("run=%u/%u ", run, runs);
      report("worker", name, state.threads, ops, worker_ns);
      printf("run=%u/%u ", run, runs);
      report("owner", name, 1u, ops, owner_ns);
      if (atomic_load_explicit(&state.failed, memory_order_relaxed) != 0u) {
        failed = 1;
      }
    }
  }

  atomic_store_explicit(&state.stop, 1, memory_order_release);
  pthread_barrier_wait(&state.start_barrier);
  pthread_barrier_wait(&state.done_barrier);
  for (uint32_t t = 0; t < state.threads; ++t) {
    pthread_join(threads[t], NULL);
  }

  pthread_barrier_destroy(&state.start_barrier);
  pthread_barrier_destroy(&state.done_barrier);
  free(args);
  free(threads);
  free(state.pending_words);
  free(state.nodes);
  return failed ? 1 : 0;
}
