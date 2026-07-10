// HZ12 L1: flush-time owner inbox behavior over the HZ12 span substrate.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#include <psapi.h>

#include "hz12.h"
#include "hz12_inbox.h"
#include "hz12_shadow.h"
#include "hz12_span_accounting.h"

#ifndef HZ12_XOWNER_ACCOUNTING
#define HZ12_XOWNER_ACCOUNTING 1
#endif

#ifndef HZ12_XOWNER_BARE_CORE
#define HZ12_XOWNER_BARE_CORE 0
#endif

#if HZ12_XOWNER_ACCOUNTING
#define H12_XOWNER_ACCOUNT_ALLOC(ptr) h12_span_accounting_on_alloc(ptr)
#define H12_XOWNER_ACCOUNT_RELEASE(ptr) h12_span_accounting_on_release(ptr)
#define H12_XOWNER_ACCOUNT_RESET() h12_span_accounting_reset()
#define H12_XOWNER_ACCOUNT_SCAN() h12_span_accounting_scan()
#else
#define H12_XOWNER_ACCOUNT_ALLOC(ptr) ((void)(ptr))
#define H12_XOWNER_ACCOUNT_RELEASE(ptr) ((void)(ptr))
#define H12_XOWNER_ACCOUNT_RESET() ((void)0)
#define H12_XOWNER_ACCOUNT_SCAN() ((H12WholeSpanShadow){0})
#endif

#ifndef HZ12_DRAIN_INTERVAL
#define HZ12_DRAIN_INTERVAL 256u
#endif

typedef struct H12L1Ring {
  void** slots;
  uint32_t capacity;
  volatile LONG head;
  volatile LONG tail;
} H12L1Ring;

typedef struct H12L1Thread {
  uint32_t id;
  uint32_t min_size;
  uint32_t max_size;
  uint32_t seed;
  H12L1Ring* ring;
  H12InboxDeferred deferred;
  uint64_t allocs;
  uint64_t frees;
  uint64_t push_waits;
  uint64_t pop_waits;
  uint64_t owner_drained;
} H12L1Thread;

static volatile LONG h12_l1_ready;
static volatile LONG h12_l1_start;
static volatile LONG h12_l1_stop;
static volatile LONG h12_l1_production_done;
static volatile LONG h12_l1_consumers_done;
static volatile LONG h12_l1_producers_done;
static uint32_t h12_l1_producer_count;

static void h12_l1_on_alloc(void* ptr, H12L1Thread* state) {
#if HZ12_XOWNER_BARE_CORE
  (void)ptr;
  (void)state;
#else
  h12_shadow_on_alloc(ptr, state->id);
  H12_XOWNER_ACCOUNT_ALLOC(ptr);
  if ((state->allocs & (HZ12_DRAIN_INTERVAL - 1u)) == 0u) {
    state->owner_drained += h12_inbox_drain_owner(state->id);
  }
#endif
}

static void h12_l1_producer_finish(H12L1Thread* state) {
#if HZ12_XOWNER_BARE_CORE
  (void)state;
#else
  h12_inbox_mark_owner_retired(state->id);
  while (InterlockedCompareExchange(&h12_l1_consumers_done, 0, 0) <
         (LONG)h12_l1_producer_count) {
    state->owner_drained += h12_inbox_drain_owner(state->id);
    SwitchToThread();
  }
  state->owner_drained += h12_inbox_drain_owner(state->id);
#endif
}

static void h12_l1_consumer_free(H12L1Thread* state, void* ptr) {
#if HZ12_XOWNER_BARE_CORE
  (void)state;
  hz12_free(ptr);
#else
  h12_inbox_defer_free(&state->deferred, ptr);
#endif
}

static void h12_l1_consumer_finish(H12L1Thread* state) {
#if HZ12_XOWNER_BARE_CORE
  (void)state;
#else
  h12_inbox_flush(&state->deferred);
#endif
}

static uint32_t h12_l1_parse_u32(const char* text, uint32_t fallback) {
  char* end = NULL;
  unsigned long value;
  if (!text) return fallback;
  value = strtoul(text, &end, 10);
  if (end == text || (end && *end != '\0') || value > 0xFFFFFFFFul) {
    return fallback;
  }
  return (uint32_t)value;
}

static uint32_t h12_l1_rng_next(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static uint64_t h12_l1_now_ns(void) {
  LARGE_INTEGER frequency;
  LARGE_INTEGER counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return ((uint64_t)counter.QuadPart * 1000000000ull) /
         (uint64_t)frequency.QuadPart;
}

static int h12_l1_ring_push(H12L1Ring* ring, void* ptr) {
  LONG tail = ring->tail;
  LONG head = ring->head;
  if ((uint32_t)(tail - head) >= ring->capacity) return 0;
  ring->slots[(uint32_t)tail % ring->capacity] = ptr;
  MemoryBarrier();
  ring->tail = tail + 1;
  return 1;
}

static void* h12_l1_ring_pop(H12L1Ring* ring) {
  LONG head = ring->head;
  LONG tail = ring->tail;
  void* ptr;
  if (head == tail) return NULL;
  ptr = ring->slots[(uint32_t)head % ring->capacity];
  MemoryBarrier();
  ring->head = head + 1;
  return ptr;
}

static unsigned __stdcall h12_l1_producer_main(void* arg) {
  H12L1Thread* state = (H12L1Thread*)arg;
  InterlockedIncrement(&h12_l1_ready);
  while (InterlockedCompareExchange(&h12_l1_start, 0, 0) == 0) SwitchToThread();
  while (InterlockedCompareExchange(&h12_l1_stop, 0, 0) == 0) {
    uint32_t span = state->max_size - state->min_size + 1u;
    size_t size = state->min_size + (h12_l1_rng_next(&state->seed) % span);
    void* ptr = hz12_malloc(size);
    if (!ptr) continue;
    state->allocs += 1u;
    h12_l1_on_alloc(ptr, state);
    while (!h12_l1_ring_push(state->ring, ptr)) {
      state->push_waits += 1u;
      if (InterlockedCompareExchange(&h12_l1_stop, 0, 0) != 0) {
        H12_XOWNER_ACCOUNT_RELEASE(ptr);
        hz12_free(ptr);
        break;
      }
      SwitchToThread();
    }
  }
  InterlockedIncrement(&h12_l1_production_done);
  h12_l1_producer_finish(state);
  InterlockedIncrement(&h12_l1_producers_done);
  return 0;
}

static unsigned __stdcall h12_l1_consumer_main(void* arg) {
  H12L1Thread* state = (H12L1Thread*)arg;
  InterlockedIncrement(&h12_l1_ready);
  while (InterlockedCompareExchange(&h12_l1_start, 0, 0) == 0) SwitchToThread();
  for (;;) {
    void* ptr = h12_l1_ring_pop(state->ring);
    if (ptr) {
      h12_l1_consumer_free(state, ptr);
      state->frees += 1u;
      continue;
    }
    if (InterlockedCompareExchange(&h12_l1_stop, 0, 0) != 0 &&
        InterlockedCompareExchange(&h12_l1_production_done, 0, 0) ==
            (LONG)h12_l1_producer_count) {
      break;
    }
    state->pop_waits += 1u;
    SwitchToThread();
  }
  h12_l1_consumer_finish(state);
  InterlockedIncrement(&h12_l1_consumers_done);
  return 0;
}

int main(int argc, char** argv) {
  uint32_t runtime_sec = (argc > 1) ? h12_l1_parse_u32(argv[1], 5u) : 5u;
  uint32_t producers = (argc > 2) ? h12_l1_parse_u32(argv[2], 4u) : 4u;
  uint32_t consumers = (argc > 3) ? h12_l1_parse_u32(argv[3], 4u) : 4u;
  uint32_t ring_capacity = (argc > 4) ? h12_l1_parse_u32(argv[4], 4096u) : 4096u;
  uint32_t min_size = (argc > 5) ? h12_l1_parse_u32(argv[5], 8u) : 8u;
  uint32_t max_size = (argc > 6) ? h12_l1_parse_u32(argv[6], 1024u) : 1024u;
  HANDLE* handles;
  H12L1Ring* rings;
  H12L1Thread* states;
  uint32_t total_threads;
  uint32_t i;
  uint64_t start_ns;
  uint64_t elapsed_ns;
  uint64_t allocs = 0u;
  uint64_t frees = 0u;
  uint64_t waits = 0u;
  uint64_t drained = 0u;
  H12AdoptionShadow adoption_shadow;
  H12WholeSpanShadow whole_span_shadow;
  PROCESS_MEMORY_COUNTERS_EX memory = {0};

  if (producers == 0u || producers != consumers ||
      producers > HZ12_SHADOW_MAX_OWNERS || ring_capacity < 2u ||
      min_size == 0u || max_size < min_size) {
    fprintf(stderr, "usage: %s <seconds> <producers> <consumers> <ring> <min> <max>\n",
            argv[0]);
    return 2;
  }
#if !HZ12_XOWNER_BARE_CORE
  if (!h12_shadow_init(producers) || !h12_inbox_init(producers)) return 3;
#endif
  H12_XOWNER_ACCOUNT_RESET();
  h12_l1_producer_count = producers;
  total_threads = producers + consumers;
  handles = (HANDLE*)calloc(total_threads, sizeof(HANDLE));
  rings = (H12L1Ring*)calloc(producers, sizeof(H12L1Ring));
  states = (H12L1Thread*)calloc(total_threads, sizeof(H12L1Thread));
  if (!handles || !rings || !states) return 4;

  for (i = 0u; i < producers; ++i) {
    rings[i].slots = (void**)calloc(ring_capacity, sizeof(void*));
    if (!rings[i].slots) return 4;
    rings[i].capacity = ring_capacity;
    states[i].id = i;
    states[i].min_size = min_size;
    states[i].max_size = max_size;
    states[i].seed = 0x85ebca6bu ^ (i * 2654435761u);
    states[i].ring = &rings[i];
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_l1_producer_main,
                                         &states[i], 0, NULL);
    states[producers + i].id = producers + i;
    states[producers + i].ring = &rings[i];
#if !HZ12_XOWNER_BARE_CORE
    h12_inbox_deferred_init(&states[producers + i].deferred);
#endif
    handles[producers + i] = (HANDLE)_beginthreadex(NULL, 0, h12_l1_consumer_main,
                                                     &states[producers + i], 0, NULL);
    if (!handles[i] || !handles[producers + i]) return 5;
  }

  while (InterlockedCompareExchange(&h12_l1_ready, 0, 0) < (LONG)total_threads) {
    SwitchToThread();
  }
  start_ns = h12_l1_now_ns();
  InterlockedExchange(&h12_l1_start, 1);
  Sleep(runtime_sec * 1000u);
  InterlockedExchange(&h12_l1_stop, 1);
  WaitForMultipleObjects(total_threads, handles, TRUE, INFINITE);
  elapsed_ns = h12_l1_now_ns() - start_ns;
#if HZ12_XOWNER_BARE_CORE
  adoption_shadow = (H12AdoptionShadow){0};
#else
  adoption_shadow = h12_inbox_adoption_shadow_scan();
#endif
  whole_span_shadow = H12_XOWNER_ACCOUNT_SCAN();

  for (i = 0u; i < total_threads; ++i) {
    allocs += states[i].allocs;
    frees += states[i].frees;
    waits += states[i].push_waits + states[i].pop_waits;
    drained += states[i].owner_drained;
    CloseHandle(handles[i]);
  }
  memory.cb = sizeof(memory);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS*)&memory,
                             sizeof(memory));
  printf("[HZ12_XOWNER_INBOX] producers=%u consumers=%u allocs=%llu "
         "cross_owner_frees=%llu owner_drained=%llu waits=%llu "
         "sharing_factor=2.00 cross_owner_rate=100.0 ops/s=%.2f elapsed=%.3f "
         "peak_rss_mib=%.2f rss_mib=%.2f retired_owners=%u "
         "retired_pending_owners=%u retired_pending_objects=%llu "
         "tracked_spans=%u full_capacity_spans=%u accounting_candidates=%u "
         "tracked_live=%llu release_untracked=%llu release_underflow=%llu\n",
         producers, consumers, (unsigned long long)allocs,
         (unsigned long long)frees, (unsigned long long)drained,
         (unsigned long long)waits,
         (double)frees * 1000000000.0 / (double)elapsed_ns,
         (double)elapsed_ns / 1000000000.0,
         (double)memory.PeakWorkingSetSize / (1024.0 * 1024.0),
         (double)memory.WorkingSetSize / (1024.0 * 1024.0),
         adoption_shadow.retired_owners, adoption_shadow.pending_owners,
         (unsigned long long)adoption_shadow.pending_objects,
         whole_span_shadow.tracked_spans, whole_span_shadow.full_capacity_spans,
         whole_span_shadow.accounting_candidates,
         (unsigned long long)whole_span_shadow.tracked_live_objects,
         (unsigned long long)whole_span_shadow.release_untracked,
         (unsigned long long)whole_span_shadow.release_underflow);
#if !HZ12_XOWNER_BARE_CORE
  h12_inbox_dump(stdout);
#endif

  for (i = 0u; i < producers; ++i) free(rings[i].slots);
  free(states);
  free(rings);
  free(handles);
  return 0;
}
