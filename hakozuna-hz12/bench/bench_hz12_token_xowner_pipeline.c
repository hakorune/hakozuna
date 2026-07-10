// HZ12 L4-B: one-owner token-aware 100% cross-owner pipeline diagnostic.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#include "hz12.h"
#include "hz12_owner_epoch.h"
#include "hz12_owner_registry.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_shadow.h"
#include "hz12_span_accounting.h"
#include "hz12_token_inbox.h"

#define HZ12_L4B_RING_CAP 4096u
#define HZ12_L4B_BATCH_CAP 256u

typedef struct H12L4BRing {
  void* slots[HZ12_L4B_RING_CAP];
  volatile LONG head;
  volatile LONG tail;
} H12L4BRing;

typedef struct H12L4BState {
  H12L4BRing ring;
  H12OwnerToken owner;
  H12OwnerEpochParticipant producer_epoch;
  H12OwnerEpochParticipant consumer_epoch;
  uint64_t allocs;
  uint64_t frees;
  uint64_t drained;
  uint64_t waits;
  uint32_t drain_interval;
  int producer_error;
  int consumer_error;
  int producer_ok;
  int consumer_ok;
} H12L4BState;

static volatile LONG h12_l4b_ready;
static volatile LONG h12_l4b_start;
static volatile LONG h12_l4b_stop;
static volatile LONG h12_l4b_producer_done;
static volatile LONG h12_l4b_consumer_done;
static volatile LONG h12_l4b_failed;

static uint32_t h12_l4b_rng(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static int h12_l4b_push(H12L4BRing* ring, void* ptr) {
  LONG tail = ring->tail;
  LONG head = ring->head;
  if ((uint32_t)(tail - head) >= HZ12_L4B_RING_CAP) return 0;
  ring->slots[(uint32_t)tail % HZ12_L4B_RING_CAP] = ptr;
  MemoryBarrier();
  ring->tail = tail + 1;
  return 1;
}

static void* h12_l4b_pop(H12L4BRing* ring) {
  LONG head = ring->head;
  void* ptr;
  if (head == ring->tail) return NULL;
  ptr = ring->slots[(uint32_t)head % HZ12_L4B_RING_CAP];
  MemoryBarrier();
  ring->head = head + 1;
  return ptr;
}

static int h12_l4b_flush(void** batch, uint32_t* count, H12OwnerToken owner) {
  uint32_t i;
  if (*count == 0u) return 1;
  for (i = 0u; i < *count; ++i) {
    uint32_t owner_id;
    if (!h12_shadow_owner_for_ptr(batch[i], &owner_id) || owner_id != 0u) {
      h12_span_accounting_on_release(batch[i]);
      hz12_free(batch[i]);
      batch[i] = NULL;
    }
  }
  /* A bounded inbox may reject a batch and free it ownerlessly. That is an
   * intentional L4-B control outcome, not an object-loss failure. */
  (void)h12_token_inbox_publish(owner, batch, *count);
  *count = 0u;
  return 1;
}

static unsigned __stdcall h12_l4b_producer(void* arg) {
  H12L4BState* state = (H12L4BState*)arg;
  uint32_t seed = 0x9e3779b9u;
  if (!h12_owner_register(&state->owner) ||
      !h12_owner_epoch_register(&state->producer_epoch)) {
    state->producer_error = 1;
    InterlockedExchange(&h12_l4b_failed, 1);
    return 1;
  }
  InterlockedIncrement(&h12_l4b_ready);
  while (InterlockedCompareExchange(&h12_l4b_start, 0, 0) == 0) SwitchToThread();
  while (InterlockedCompareExchange(&h12_l4b_stop, 0, 0) == 0) {
    void* ptr = hz12_malloc(8u + (h12_l4b_rng(&seed) % 1017u));
    if (!ptr) continue;
    h12_shadow_on_alloc(ptr, 0u);
    h12_span_accounting_on_alloc(ptr);
    state->allocs += 1u;
    if ((state->allocs & (state->drain_interval - 1u)) == 0u) {
      state->drained += h12_token_inbox_drain(state->owner);
    }
    while (!h12_l4b_push(&state->ring, ptr)) {
      state->waits += 1u;
      if (InterlockedCompareExchange(&h12_l4b_stop, 0, 0) != 0) {
        h12_span_accounting_on_release(ptr);
        hz12_free(ptr);
        break;
      }
      SwitchToThread();
    }
  }
  if (!h12_owner_begin_retire(state->owner) ||
      !h12_owner_epoch_begin_retire(state->owner) ||
      !h12_owner_epoch_checkpoint(state->producer_epoch)) {
    state->producer_error = 2;
    InterlockedExchange(&h12_l4b_failed, 1);
    return 2;
  }
  InterlockedExchange(&h12_l4b_producer_done, 1);
  while (InterlockedCompareExchange(&h12_l4b_consumer_done, 0, 0) == 0) {
    if (InterlockedCompareExchange(&h12_l4b_failed, 0, 0) != 0) {
      state->producer_error = 3;
      return 3;
    }
    state->drained += h12_token_inbox_drain(state->owner);
    SwitchToThread();
  }
  state->drained += h12_token_inbox_drain(state->owner);
  if (!h12_owner_retire_gate_ready(state->owner)) {
    state->producer_error = 4;
    return 4;
  }
  if (!h12_owner_mark_dead(state->owner)) {
    state->producer_error = 5;
    return 5;
  }
  if (!h12_owner_epoch_unregister(state->producer_epoch)) {
    state->producer_error = 6;
    return 6;
  }
  state->producer_ok = 1;
  return 0;
}

static unsigned __stdcall h12_l4b_consumer(void* arg) {
  H12L4BState* state = (H12L4BState*)arg;
  void* batch[HZ12_L4B_BATCH_CAP];
  uint32_t count = 0u;
  if (!h12_owner_epoch_register(&state->consumer_epoch)) {
    state->consumer_error = 1;
    InterlockedExchange(&h12_l4b_failed, 1);
    return 1;
  }
  InterlockedIncrement(&h12_l4b_ready);
  while (InterlockedCompareExchange(&h12_l4b_start, 0, 0) == 0) SwitchToThread();
  for (;;) {
    void* ptr = h12_l4b_pop(&state->ring);
    if (ptr) {
      batch[count++] = ptr;
      state->frees += 1u;
      if (count == HZ12_L4B_BATCH_CAP &&
          !h12_l4b_flush(batch, &count, state->owner)) {
        state->consumer_error = 2;
        InterlockedExchange(&h12_l4b_failed, 1);
        return 2;
      }
      continue;
    }
    if (InterlockedCompareExchange(&h12_l4b_stop, 0, 0) != 0 &&
        InterlockedCompareExchange(&h12_l4b_producer_done, 0, 0) != 0) {
      break;
    }
    state->waits += 1u;
    SwitchToThread();
  }
  if (!h12_l4b_flush(batch, &count, state->owner) ||
      !h12_owner_epoch_checkpoint(state->consumer_epoch) ||
      !h12_owner_epoch_unregister(state->consumer_epoch)) {
    state->consumer_error = 3;
    InterlockedExchange(&h12_l4b_failed, 1);
    return 3;
  }
  InterlockedExchange(&h12_l4b_consumer_done, 1);
  state->consumer_ok = 1;
  return 0;
}

int main(int argc, char** argv) {
  H12L4BState state = {0};
  H12TokenInboxStats inbox;
  H12OwnerRetireGateStats gate;
  HANDLE producer;
  HANDLE consumer;
  uint32_t seconds = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 1u;
  uint32_t drain_interval =
      argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 10) : 1u;
  uint64_t start;
  uint64_t elapsed;

  if (seconds == 0u || seconds > 30u || drain_interval == 0u ||
      (drain_interval & (drain_interval - 1u)) != 0u) {
    fprintf(stderr, "usage: %s [seconds 1..30] [drain_interval power-of-two]\n",
            argv[0]);
    return 1;
  }
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  if (!h12_shadow_init(1u)) return 2;
  h12_span_accounting_reset();
  state.drain_interval = drain_interval;
  producer = (HANDLE)_beginthreadex(NULL, 0, h12_l4b_producer, &state, 0, NULL);
  consumer = (HANDLE)_beginthreadex(NULL, 0, h12_l4b_consumer, &state, 0, NULL);
  if (!producer || !consumer) return 3;
  while (InterlockedCompareExchange(&h12_l4b_ready, 0, 0) < 2) SwitchToThread();
  start = GetTickCount64();
  InterlockedExchange(&h12_l4b_start, 1);
  Sleep(seconds * 1000u);
  InterlockedExchange(&h12_l4b_stop, 1);
  WaitForSingleObject(producer, INFINITE);
  WaitForSingleObject(consumer, INFINITE);
  elapsed = GetTickCount64() - start;
  CloseHandle(producer);
  CloseHandle(consumer);
  h12_token_inbox_stats(&inbox);
  h12_owner_retire_gate_stats(&gate);
  if (!state.producer_ok || !state.consumer_ok) {
    fprintf(stderr, "pipeline producer_error=%d consumer_error=%d failed=%ld\n",
            state.producer_error, state.consumer_error,
            InterlockedCompareExchange(&h12_l4b_failed, 0, 0));
  }
  if (!state.producer_ok || !state.consumer_ok || state.allocs == 0u ||
      state.frees == 0u || inbox.publish_registry_reject != 0u ||
      inbox.publish_generation_reject != 0u || gate.open != 1u ||
      state.allocs < state.frees) {
    return 4;
  }
  printf("[HZ12_TOKEN_XOWNER_L4B] seconds=%u drain_interval=%u allocs=%llu frees=%llu "
         "ops/s=%.2f drained=%llu waits=%llu inbox_max=%u overflow=%llu "
         "fallback_objects=%llu generation_reject=0 gate_open=1\n",
         seconds, drain_interval, (unsigned long long)state.allocs,
         (unsigned long long)state.frees,
         (double)state.frees * 1000.0 / (double)elapsed,
         (unsigned long long)state.drained, (unsigned long long)state.waits,
         inbox.inbox_current_max, (unsigned long long)inbox.publish_overflow,
         (unsigned long long)inbox.fallback_objects);
  h12_token_inbox_dump(stdout);
  h12_owner_retire_gate_dump(stdout);
  return 0;
}
