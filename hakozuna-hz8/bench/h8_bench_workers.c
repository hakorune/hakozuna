#include "../include/h8.h"
#include "h8_bench_support.h"

#include <sched.h>
#include <stdlib.h>
#include <string.h>

static void* h8_bench_thread_working_set_ring(void* arg) {
  H8BenchThread* th = (H8BenchThread*)arg;
  const H8BenchOptions* opt = th->opt;
  size_t working_set = opt->live_window;
  void** slots = calloc(working_set, sizeof(*slots));
  if (!slots) {
    th->error = 3;
    pthread_barrier_wait(th->barrier);
    pthread_barrier_wait(th->barrier);
    return NULL;
  }

  size_t live = 0u;
  uint64_t work_start = h8_now_ns();
  for (size_t i = 0; i < opt->iters_per_thread; ++i) {
    uint32_t random = h8_rng_next(&th->rng);
    size_t index = (size_t)(random % (uint32_t)working_set);
    if (slots[index]) {
      h8_free(slots[index]);
      slots[index] = NULL;
      --live;
      ++th->working_set_frees;
      continue;
    }

    size_t span = opt->max_size - opt->min_size + 1u;
    size_t size = opt->min_size + (size_t)(random % span);
#if defined(H8_BENCH_ATTRIBUTION)
    h8_bench_note_alloc(th, size);
#endif
    void* ptr = h8_malloc(size);
    if (!ptr) {
      th->error = 1;
      break;
    }
    memset(ptr, 0xA5, size < 64u ? size : 64u);
    slots[index] = ptr;
    ++live;
    ++th->working_set_allocs;
    if (live > th->working_set_max_live) th->working_set_max_live = live;
  }

  for (size_t i = 0; i < working_set; ++i) {
    if (!slots[i]) continue;
    h8_free(slots[i]);
    ++th->working_set_frees;
  }
  th->alloc_ns = h8_now_ns() - work_start;
  free(slots);

  pthread_barrier_wait(th->barrier);
  pthread_barrier_wait(th->barrier);
  return NULL;
}

static void* h8_bench_thread_interleaved(void* arg) {
  H8BenchThread* th = (H8BenchThread*)arg;
  const H8BenchOptions* opt = th->opt;
  int next = (th->index + 1) % opt->threads;
  int prev = (th->index + opt->threads - 1) % opt->threads;
  H8Inbox* next_inbox = &th->inboxes[next];
  H8Inbox* my_inbox = &th->inboxes[th->index];

  uint64_t work_start = h8_now_ns();
  for (size_t i = 0; i < opt->iters_per_thread; ++i) {
    th->interleaved_drain_calls++;
    size_t drained_now = h8_drain_inbox(my_inbox);
    th->interleaved_drain_objects += drained_now;
    if (drained_now == 0) {
      th->interleaved_drain_empty++;
    }

    size_t size = h8_rand_range(&th->rng, opt->min_size, opt->max_size);
#if defined(H8_BENCH_ATTRIBUTION)
    h8_bench_note_alloc(th, size);
#endif
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
        th->interleaved_push_yields++;
        th->interleaved_drain_calls++;
        size_t pushed_drain = h8_drain_inbox(my_inbox);
        th->interleaved_drain_objects += pushed_drain;
        if (pushed_drain == 0) {
          th->interleaved_drain_empty++;
        }
        sched_yield();
      }
      th->interleaved_remote_enqueue++;
#if defined(H8_BENCH_ATTRIBUTION)
      h8_bench_note_remote_live(th, size);
#endif
    } else {
      th->interleaved_local_free++;
      h8_free(ptr);
    }
  }
  th->alloc_ns = h8_now_ns() - work_start;

  atomic_store_explicit(&th->done[th->index], 1, memory_order_release);

  uint64_t tail_start = h8_now_ns();
  for (;;) {
    th->interleaved_drain_calls++;
    size_t drained = h8_drain_inbox(my_inbox);
    th->interleaved_drain_objects += drained;
    if (drained == 0) {
      th->interleaved_drain_empty++;
    }
    if (atomic_load_explicit(&th->done[prev], memory_order_acquire) &&
        drained == 0) {
      th->interleaved_drain_calls++;
      size_t final_drain = h8_drain_inbox(my_inbox);
      th->interleaved_drain_objects += final_drain;
      if (final_drain == 0) {
        th->interleaved_drain_empty++;
        break;
      }
    }
    if (drained == 0) {
      th->interleaved_finish_yields++;
      sched_yield();
    }
  }
  th->remote_ns = h8_now_ns() - tail_start;

  pthread_barrier_wait(th->barrier);
  return NULL;
}

void* h8_bench_thread_main(void* arg) {
  H8BenchThread* th = (H8BenchThread*)arg;
  const H8BenchOptions* opt = th->opt;
  if (opt->working_set_ring) {
    return h8_bench_thread_working_set_ring(arg);
  }
  if (opt->interleaved) {
    return h8_bench_thread_interleaved(arg);
  }

  int next = (th->index + 1) % opt->threads;
  H8Inbox* next_inbox = &th->inboxes[next];
  H8Inbox* my_inbox = &th->inboxes[th->index];

  uint64_t alloc_start = h8_now_ns();
  for (size_t i = 0; i < opt->iters_per_thread; ++i) {
    size_t size = h8_rand_range(&th->rng, opt->min_size, opt->max_size);
#if defined(H8_BENCH_ATTRIBUTION)
    uint32_t class_id = h8_bench_note_alloc(th, size);
#endif
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
#if defined(H8_BENCH_ATTRIBUTION)
      (void)class_id;
      h8_bench_note_remote_live(th, size);
#endif
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
