// HZ12 L4-A: diagnostic live publish/drain followed by a quiescent retirement.

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
#include "hz12_token_inbox.h"

#define HZ12_LIVE_PRODUCERS 2u
#define HZ12_LIVE_BATCH_ITEMS 32u

typedef struct H12LiveProducer {
  H12OwnerToken owner;
  H12OwnerEpochParticipant participant;
  uint32_t seed;
  uint64_t batches;
  uint64_t objects;
  int error;
  int success;
} H12LiveProducer;

static volatile LONG h12_live_ready;
static volatile LONG h12_live_start;
static volatile LONG h12_live_retire;
static volatile LONG h12_live_retired;
static volatile LONG h12_live_failed;

static uint32_t h12_live_rng(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static int h12_live_publish(H12LiveProducer* state) {
  void* objects[HZ12_LIVE_BATCH_ITEMS];
  uint32_t i;
  for (i = 0u; i < HZ12_LIVE_BATCH_ITEMS; ++i) {
    size_t size = 8u + (h12_live_rng(&state->seed) % 1017u);
    objects[i] = hz12_malloc(size);
    if (!objects[i]) {
      uint32_t j;
      for (j = 0u; j < i; ++j) hz12_free(objects[j]);
      return 0;
    }
  }
  if (!h12_token_inbox_publish(state->owner, objects,
                               HZ12_LIVE_BATCH_ITEMS)) {
    return 0;
  }
  state->batches += 1u;
  state->objects += HZ12_LIVE_BATCH_ITEMS;
  return 1;
}

static unsigned __stdcall h12_live_producer(void* arg) {
  H12LiveProducer* state = (H12LiveProducer*)arg;
  if (!h12_owner_epoch_register(&state->participant)) {
    state->error = 1;
    InterlockedIncrement(&h12_live_failed);
    return 1;
  }
  InterlockedIncrement(&h12_live_ready);
  while (InterlockedCompareExchange(&h12_live_start, 0, 0) == 0) {
    SwitchToThread();
  }
  while (InterlockedCompareExchange(&h12_live_retire, 0, 0) == 0) {
    if (!h12_live_publish(state)) {
      state->error = 2;
      InterlockedIncrement(&h12_live_failed);
      return 2;
    }
    /* This is a lifecycle lane, not a throughput benchmark. Keep the bounded
     * inbox below its cap while the owner exercises live drains. */
    Sleep(1);
  }
  /* A final RETIRING batch precedes the producer checkpoint. */
  if (!h12_live_publish(state)) {
    state->error = 3;
    InterlockedIncrement(&h12_live_failed);
    return 3;
  }
  if (!h12_owner_epoch_checkpoint(state->participant)) {
    state->error = 4;
    InterlockedIncrement(&h12_live_failed);
    return 4;
  }
  InterlockedIncrement(&h12_live_retired);
  if (!h12_owner_epoch_unregister(state->participant)) {
    state->error = 5;
    InterlockedIncrement(&h12_live_failed);
    return 5;
  }
  state->success = 1;
  return 0;
}

static uint32_t h12_live_parse_seconds(int argc, char** argv) {
  char* end = NULL;
  unsigned long value;
  if (argc < 2) return 1u;
  value = strtoul(argv[1], &end, 10);
  if (end == argv[1] || *end != '\0' || value == 0u || value > 60u) return 1u;
  return (uint32_t)value;
}

int main(int argc, char** argv) {
  HANDLE handles[HZ12_LIVE_PRODUCERS];
  H12LiveProducer producers[HZ12_LIVE_PRODUCERS] = {0};
  H12OwnerToken owner;
  H12TokenInboxStats inbox;
  H12OwnerRetireGateStats gate;
  uint32_t seconds = h12_live_parse_seconds(argc, argv);
  uint64_t deadline;
  uint64_t batches = 0u;
  uint64_t objects = 0u;
  uint32_t pending;
  uint32_t i;

  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  if (!h12_owner_register(&owner)) return 1;
  for (i = 0u; i < HZ12_LIVE_PRODUCERS; ++i) {
    producers[i].owner = owner;
    producers[i].seed = 0x9e3779b9u ^ (i * 0x85ebca6bu);
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_live_producer,
                                         &producers[i], 0, NULL);
    if (!handles[i]) return 2;
  }
  while (InterlockedCompareExchange(&h12_live_ready, 0, 0) <
         (LONG)HZ12_LIVE_PRODUCERS) {
    SwitchToThread();
  }
  InterlockedExchange(&h12_live_start, 1);
  deadline = GetTickCount64() + (uint64_t)seconds * 1000u;
  while (GetTickCount64() < deadline) {
    (void)h12_token_inbox_drain(owner);
    if (InterlockedCompareExchange(&h12_live_failed, 0, 0) != 0) return 8;
    SwitchToThread();
  }

  if (!h12_owner_begin_retire(owner) || !h12_owner_epoch_begin_retire(owner)) {
    return 3;
  }
  InterlockedExchange(&h12_live_retire, 1);
  while (InterlockedCompareExchange(&h12_live_retired, 0, 0) <
         (LONG)HZ12_LIVE_PRODUCERS) {
    if (InterlockedCompareExchange(&h12_live_failed, 0, 0) != 0) return 9;
    SwitchToThread();
  }
  pending = h12_token_inbox_pending(owner);
  if (pending < HZ12_LIVE_PRODUCERS * HZ12_LIVE_BATCH_ITEMS ||
      h12_owner_retire_gate_ready(owner)) {
    return 4;
  }
  if (h12_token_inbox_drain(owner) != pending ||
      !h12_owner_retire_gate_ready(owner) || !h12_owner_mark_dead(owner)) {
    return 5;
  }
  WaitForMultipleObjects(HZ12_LIVE_PRODUCERS, handles, TRUE, INFINITE);
  for (i = 0u; i < HZ12_LIVE_PRODUCERS; ++i) {
    CloseHandle(handles[i]);
    if (!producers[i].success) {
      fprintf(stderr, "producer %u failed at step %d\n", i,
              producers[i].error);
      return 10;
    }
    batches += producers[i].batches;
    objects += producers[i].objects;
  }
  h12_token_inbox_stats(&inbox);
  h12_owner_retire_gate_stats(&gate);
  if (batches == 0u || objects == 0u || inbox.publish_overflow != 0u ||
      inbox.publish_registry_reject != 0u || gate.blocked_inbox == 0u ||
      gate.open != 1u) {
    return 11;
  }
  printf("[HZ12_TOKEN_RETIRE_LIVE] seconds=%u batches=%llu objects=%llu "
         "final_pending=%u drained=%u gate_open=%llu overflow=0 "
         "registry_reject=0\n",
         seconds, (unsigned long long)batches, (unsigned long long)objects,
         pending, pending, (unsigned long long)gate.open);
  h12_token_inbox_dump(stdout);
  h12_owner_retire_gate_dump(stdout);
  return 0;
}
