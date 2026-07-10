// HZ12 L0: diagnostic-only cross-owner owner-routing projection.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#include "hz12.h"
#include "hz12_shadow.h"

typedef struct H12Ring {
  void** slots;
  uint32_t capacity;
  volatile LONG head;
  volatile LONG tail;
} H12Ring;

typedef struct H12Thread {
  uint32_t id;
  uint32_t min_size;
  uint32_t max_size;
  uint32_t seed;
  H12Ring* ring;
  H12ShadowCache shadow;
  uint64_t allocs;
  uint64_t frees;
  uint64_t push_waits;
  uint64_t pop_waits;
  uint64_t cleanup_local;
} H12Thread;

static volatile LONG h12_ready;
static volatile LONG h12_start;
static volatile LONG h12_stop;
static volatile LONG h12_producers_done;
static uint32_t h12_producer_count;

static uint32_t h12_parse_u32(const char* text, uint32_t fallback) {
  char* end = NULL;
  unsigned long value;
  if (!text) return fallback;
  value = strtoul(text, &end, 10);
  if (end == text || (end && *end != '\0') || value > 0xFFFFFFFFul) {
    return fallback;
  }
  return (uint32_t)value;
}

static uint32_t h12_rng_next(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static uint64_t h12_now_ns(void) {
  LARGE_INTEGER frequency;
  LARGE_INTEGER counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return ((uint64_t)counter.QuadPart * 1000000000ull) /
         (uint64_t)frequency.QuadPart;
}

static int h12_ring_push(H12Ring* ring, void* ptr) {
  LONG tail = ring->tail;
  LONG head = ring->head;
  if ((uint32_t)(tail - head) >= ring->capacity) return 0;
  ring->slots[(uint32_t)tail % ring->capacity] = ptr;
  MemoryBarrier();
  ring->tail = tail + 1;
  return 1;
}

static void* h12_ring_pop(H12Ring* ring) {
  LONG head = ring->head;
  LONG tail = ring->tail;
  void* ptr;
  if (head == tail) return NULL;
  ptr = ring->slots[(uint32_t)head % ring->capacity];
  MemoryBarrier();
  ring->head = head + 1;
  return ptr;
}

static unsigned __stdcall h12_producer_main(void* arg) {
  H12Thread* state = (H12Thread*)arg;
  InterlockedIncrement(&h12_ready);
  while (InterlockedCompareExchange(&h12_start, 0, 0) == 0) SwitchToThread();
  while (InterlockedCompareExchange(&h12_stop, 0, 0) == 0) {
    uint32_t span = state->max_size - state->min_size + 1u;
    size_t size = state->min_size + (h12_rng_next(&state->seed) % span);
    void* ptr = hz12_malloc(size);
    if (!ptr) continue;
    h12_shadow_on_alloc(ptr, state->id);
    state->allocs += 1u;
    while (!h12_ring_push(state->ring, ptr)) {
      state->push_waits += 1u;
      if (InterlockedCompareExchange(&h12_stop, 0, 0) != 0) {
        hz12_free(ptr);
        state->cleanup_local += 1u;
        break;
      }
      SwitchToThread();
    }
  }
  InterlockedIncrement(&h12_producers_done);
  return 0;
}

static unsigned __stdcall h12_consumer_main(void* arg) {
  H12Thread* state = (H12Thread*)arg;
  InterlockedIncrement(&h12_ready);
  while (InterlockedCompareExchange(&h12_start, 0, 0) == 0) SwitchToThread();
  for (;;) {
    void* ptr = h12_ring_pop(state->ring);
    if (ptr) {
      h12_shadow_on_free(&state->shadow, ptr);
      hz12_free(ptr);
      state->frees += 1u;
      continue;
    }
    if (InterlockedCompareExchange(&h12_stop, 0, 0) != 0 &&
        InterlockedCompareExchange(&h12_producers_done, 0, 0) ==
            (LONG)h12_producer_count) {
      break;
    }
    state->pop_waits += 1u;
    SwitchToThread();
  }
  h12_shadow_flush(&state->shadow);
  return 0;
}

int main(int argc, char** argv) {
  uint32_t runtime_sec = (argc > 1) ? h12_parse_u32(argv[1], 5u) : 5u;
  uint32_t producers = (argc > 2) ? h12_parse_u32(argv[2], 4u) : 4u;
  uint32_t consumers = (argc > 3) ? h12_parse_u32(argv[3], 4u) : 4u;
  uint32_t ring_capacity = (argc > 4) ? h12_parse_u32(argv[4], 4096u) : 4096u;
  uint32_t min_size = (argc > 5) ? h12_parse_u32(argv[5], 8u) : 8u;
  uint32_t max_size = (argc > 6) ? h12_parse_u32(argv[6], 1024u) : 1024u;
  HANDLE* handles;
  H12Ring* rings;
  H12Thread* states;
  uint32_t total_threads;
  uint32_t i;
  uint64_t start_ns;
  uint64_t elapsed_ns;
  uint64_t allocs = 0u;
  uint64_t frees = 0u;
  uint64_t waits = 0u;

  if (producers == 0u || producers != consumers ||
      producers > HZ12_SHADOW_MAX_OWNERS || ring_capacity < 2u ||
      min_size == 0u || max_size < min_size) {
    fprintf(stderr, "usage: %s <seconds> <producers=consumers> <ring> <min> <max>\n",
            argv[0]);
    return 2;
  }
  if (!h12_shadow_init(producers)) return 3;
  h12_producer_count = producers;
  total_threads = producers + consumers;
  handles = (HANDLE*)calloc(total_threads, sizeof(HANDLE));
  rings = (H12Ring*)calloc(producers, sizeof(H12Ring));
  states = (H12Thread*)calloc(total_threads, sizeof(H12Thread));
  if (!handles || !rings || !states) return 4;

  for (i = 0u; i < producers; ++i) {
    rings[i].slots = (void**)calloc(ring_capacity, sizeof(void*));
    if (!rings[i].slots) return 4;
    rings[i].capacity = ring_capacity;
    states[i].id = i;
    states[i].min_size = min_size;
    states[i].max_size = max_size;
    states[i].seed = 0x9e3779b9u ^ (i * 2654435761u);
    states[i].ring = &rings[i];
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_producer_main,
                                         &states[i], 0, NULL);
    states[producers + i].id = i;
    states[producers + i].ring = &rings[i];
    /* Consumer identities are disjoint from producer/span-owner identities;
     * this pipeline therefore projects every completed free as cross-owner. */
    h12_shadow_cache_init(&states[producers + i].shadow, producers + i);
    handles[producers + i] = (HANDLE)_beginthreadex(NULL, 0, h12_consumer_main,
                                                     &states[producers + i], 0, NULL);
    if (!handles[i] || !handles[producers + i]) return 5;
  }

  while (InterlockedCompareExchange(&h12_ready, 0, 0) < (LONG)total_threads) {
    SwitchToThread();
  }
  start_ns = h12_now_ns();
  InterlockedExchange(&h12_start, 1);
  Sleep(runtime_sec * 1000u);
  InterlockedExchange(&h12_stop, 1);
  WaitForMultipleObjects(total_threads, handles, TRUE, INFINITE);
  elapsed_ns = h12_now_ns() - start_ns;

  for (i = 0u; i < total_threads; ++i) {
    allocs += states[i].allocs;
    frees += states[i].frees;
    waits += states[i].push_waits + states[i].pop_waits;
    CloseHandle(handles[i]);
  }
  printf("[HZ12_XOWNER_SHADOW] producers=%u consumers=%u allocs=%llu "
         "cross_owner_frees=%llu waits=%llu sharing_factor=2.00 "
         "cross_owner_rate=100.0 ops/s=%.2f elapsed=%.3f\n",
         producers, consumers, (unsigned long long)allocs,
         (unsigned long long)frees, (unsigned long long)waits,
         (double)frees * 1000000000.0 / (double)elapsed_ns,
         (double)elapsed_ns / 1000000000.0);
  h12_shadow_dump(stdout);

  for (i = 0u; i < producers; ++i) free(rings[i].slots);
  free(states);
  free(rings);
  free(handles);
  return 0;
}
