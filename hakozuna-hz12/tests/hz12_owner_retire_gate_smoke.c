#include <stdint.h>
#include <stdio.h>

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

#define HZ12_RETIRE_GATE_PRODUCERS 2u
#define HZ12_RETIRE_GATE_BATCH_ITEMS 32u
#define HZ12_RETIRE_GATE_BATCHES 4u

typedef struct H12RetireGateProducer {
  H12OwnerEpochParticipant participant;
  H12OwnerToken owner;
  int success;
} H12RetireGateProducer;

static volatile LONG h12_retire_gate_ready;
static volatile LONG h12_retire_gate_published;
static volatile LONG h12_retire_gate_start;

static unsigned __stdcall h12_retire_gate_producer(void* arg) {
  H12RetireGateProducer* state = (H12RetireGateProducer*)arg;
  uint32_t batch;
  if (!h12_owner_epoch_register(&state->participant)) return 1;
  InterlockedIncrement(&h12_retire_gate_ready);
  while (InterlockedCompareExchange(&h12_retire_gate_start, 0, 0) == 0) {
    SwitchToThread();
  }
  for (batch = 0u; batch < HZ12_RETIRE_GATE_BATCHES; ++batch) {
    void* objects[HZ12_RETIRE_GATE_BATCH_ITEMS];
    uint32_t i;
    for (i = 0u; i < HZ12_RETIRE_GATE_BATCH_ITEMS; ++i) {
      objects[i] = hz12_malloc(64u);
      if (!objects[i]) return 2;
    }
    if (!h12_token_inbox_publish(state->owner, objects,
                                 HZ12_RETIRE_GATE_BATCH_ITEMS)) {
      return 3;
    }
  }
  if (!h12_owner_epoch_checkpoint(state->participant)) return 4;
  InterlockedIncrement(&h12_retire_gate_published);
  if (!h12_owner_epoch_unregister(state->participant)) return 5;
  state->success = 1;
  return 0;
}

int main(void) {
  HANDLE handles[HZ12_RETIRE_GATE_PRODUCERS];
  H12RetireGateProducer producers[HZ12_RETIRE_GATE_PRODUCERS] = {0};
  H12OwnerRetireGateStats gate;
  H12OwnerToken owner;
  uint32_t i;
  const uint32_t expected = HZ12_RETIRE_GATE_PRODUCERS *
                            HZ12_RETIRE_GATE_BATCHES *
                            HZ12_RETIRE_GATE_BATCH_ITEMS;

  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  if (!h12_owner_register(&owner)) return 1;
  for (i = 0u; i < HZ12_RETIRE_GATE_PRODUCERS; ++i) {
    producers[i].owner = owner;
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_retire_gate_producer,
                                         &producers[i], 0, NULL);
    if (!handles[i]) return 2;
  }
  while (InterlockedCompareExchange(&h12_retire_gate_ready, 0, 0) <
         (LONG)HZ12_RETIRE_GATE_PRODUCERS) {
    SwitchToThread();
  }
  if (!h12_owner_begin_retire(owner) || !h12_owner_epoch_begin_retire(owner)) {
    return 3;
  }
  if (h12_owner_retire_gate_ready(owner)) return 4;
  InterlockedExchange(&h12_retire_gate_start, 1);
  while (InterlockedCompareExchange(&h12_retire_gate_published, 0, 0) <
         (LONG)HZ12_RETIRE_GATE_PRODUCERS) {
    SwitchToThread();
  }
  if (h12_owner_retire_gate_ready(owner)) return 5;
  if (h12_token_inbox_drain(owner) != expected) return 6;
  if (!h12_owner_retire_gate_ready(owner)) return 7;
  if (!h12_owner_mark_dead(owner)) return 8;
  WaitForMultipleObjects(HZ12_RETIRE_GATE_PRODUCERS, handles, TRUE, INFINITE);
  for (i = 0u; i < HZ12_RETIRE_GATE_PRODUCERS; ++i) {
    CloseHandle(handles[i]);
    if (!producers[i].success) return 9;
  }
  h12_owner_retire_gate_stats(&gate);
  if (gate.query != 3u || gate.open != 1u || gate.blocked_epoch != 1u ||
      gate.blocked_inbox != 1u || gate.last_pending != 0u) {
    return 10;
  }
  printf("[HZ12_OWNER_RETIRE_GATE_SMOKE] batches=8 drained=%u "
         "blocked_epoch=1 blocked_inbox=1 open=1 owner_dead=1\n", expected);
  h12_owner_retire_gate_dump(stdout);
  return 0;
}
