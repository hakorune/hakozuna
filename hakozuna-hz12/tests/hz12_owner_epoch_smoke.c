#include <stdint.h>
#include <stdio.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#include "hz12_owner_epoch.h"
#include "hz12_owner_registry.h"

#define HZ12_OWNER_EPOCH_SMOKE_THREADS 2u

typedef struct H12OwnerEpochSmokeWorker {
  H12OwnerEpochParticipant participant;
  int success;
} H12OwnerEpochSmokeWorker;

static volatile LONG h12_owner_epoch_smoke_ready;
static volatile LONG h12_owner_epoch_smoke_checkpoint;
static volatile LONG h12_owner_epoch_smoke_start;

static unsigned __stdcall h12_owner_epoch_smoke_worker(void* arg) {
  H12OwnerEpochSmokeWorker* state = (H12OwnerEpochSmokeWorker*)arg;
  if (!h12_owner_epoch_register(&state->participant)) return 1;
  InterlockedIncrement(&h12_owner_epoch_smoke_ready);
  while (InterlockedCompareExchange(&h12_owner_epoch_smoke_start, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!h12_owner_epoch_checkpoint(state->participant)) return 2;
  InterlockedIncrement(&h12_owner_epoch_smoke_checkpoint);
  if (!h12_owner_epoch_unregister(state->participant)) return 3;
  state->success = 1;
  return 0;
}

int main(void) {
  HANDLE handles[HZ12_OWNER_EPOCH_SMOKE_THREADS];
  H12OwnerEpochSmokeWorker workers[HZ12_OWNER_EPOCH_SMOKE_THREADS] = {0};
  H12OwnerEpochStats epoch;
  H12OwnerRegistryStats registry;
  H12OwnerToken owner;
  uint32_t i;

  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  if (!h12_owner_register(&owner)) return 1;
  for (i = 0u; i < HZ12_OWNER_EPOCH_SMOKE_THREADS; ++i) {
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_owner_epoch_smoke_worker,
                                         &workers[i], 0, NULL);
    if (!handles[i]) return 2;
  }
  while (InterlockedCompareExchange(&h12_owner_epoch_smoke_ready, 0, 0) <
         (LONG)HZ12_OWNER_EPOCH_SMOKE_THREADS) {
    SwitchToThread();
  }
  if (!h12_owner_begin_retire(owner)) return 3;
  if (!h12_owner_epoch_begin_retire(owner)) return 4;
  if (h12_owner_epoch_ready_to_dead(owner)) return 5;
  InterlockedExchange(&h12_owner_epoch_smoke_start, 1);
  while (InterlockedCompareExchange(&h12_owner_epoch_smoke_checkpoint, 0, 0) <
         (LONG)HZ12_OWNER_EPOCH_SMOKE_THREADS) {
    SwitchToThread();
  }
  if (!h12_owner_epoch_ready_to_dead(owner)) return 6;
  if (!h12_owner_mark_dead(owner)) return 7;
  WaitForMultipleObjects(HZ12_OWNER_EPOCH_SMOKE_THREADS, handles, TRUE,
                         INFINITE);
  for (i = 0u; i < HZ12_OWNER_EPOCH_SMOKE_THREADS; ++i) {
    CloseHandle(handles[i]);
    if (!workers[i].success) return 8;
  }
  h12_owner_epoch_stats(&epoch);
  h12_owner_registry_stats(&registry);
  if (epoch.participant_register != 2u || epoch.participant_unregister != 2u ||
      epoch.retire_begin != 1u || epoch.checkpoint != 2u ||
      epoch.ready_yes != 1u || epoch.ready_no != 1u ||
      epoch.stale_checkpoint != 0u || epoch.active_participants != 0u ||
      epoch.pending_participants != 0u || registry.retire_success != 1u ||
      registry.dead_success != 1u || registry.invalid_transition != 0u) {
    return 9;
  }
  printf("[HZ12_OWNER_EPOCH_SMOKE] participants=2 checkpoints=2 "
         "ready_before=0 ready_after=1 owner_dead=1\n");
  h12_owner_epoch_dump(stdout);
  return 0;
}
