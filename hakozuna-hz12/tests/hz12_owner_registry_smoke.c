#include <stdint.h>
#include <stdio.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#include "hz12_owner_registry.h"

#define HZ12_OWNER_SMOKE_THREADS 4u

typedef struct H12OwnerSmokeThread {
  H12OwnerToken token;
  int success;
} H12OwnerSmokeThread;

static volatile LONG h12_owner_smoke_ready;
static volatile LONG h12_owner_smoke_start;
static volatile LONG h12_owner_smoke_registered;
static volatile LONG h12_owner_smoke_retire_start;

static unsigned __stdcall h12_owner_smoke_worker(void* arg) {
  H12OwnerSmokeThread* state = (H12OwnerSmokeThread*)arg;
  InterlockedIncrement(&h12_owner_smoke_ready);
  while (InterlockedCompareExchange(&h12_owner_smoke_start, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!h12_owner_register(&state->token)) return 1;
  if (!h12_owner_publishable(state->token)) return 2;
  InterlockedIncrement(&h12_owner_smoke_registered);
  while (InterlockedCompareExchange(&h12_owner_smoke_retire_start, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!h12_owner_begin_retire(state->token)) return 3;
  if (!h12_owner_publishable(state->token)) return 4;
  if (!h12_owner_mark_dead(state->token)) return 5;
  if (h12_owner_state(state->token) != HZ12_OWNER_DEAD) return 6;
  state->success = 1;
  return 0;
}

int main(void) {
  HANDLE handles[HZ12_OWNER_SMOKE_THREADS];
  H12OwnerSmokeThread workers[HZ12_OWNER_SMOKE_THREADS] = {0};
  H12OwnerToken replacements[HZ12_OWNER_SMOKE_THREADS];
  H12OwnerRegistryStats stats;
  uint32_t i;

  h12_owner_registry_reset();
  for (i = 0u; i < HZ12_OWNER_SMOKE_THREADS; ++i) {
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_owner_smoke_worker,
                                         &workers[i], 0, NULL);
    if (!handles[i]) return 1;
  }
  while (InterlockedCompareExchange(&h12_owner_smoke_ready, 0, 0) <
         (LONG)HZ12_OWNER_SMOKE_THREADS) {
    SwitchToThread();
  }
  InterlockedExchange(&h12_owner_smoke_start, 1);
  while (InterlockedCompareExchange(&h12_owner_smoke_registered, 0, 0) <
         (LONG)HZ12_OWNER_SMOKE_THREADS) {
    SwitchToThread();
  }
  InterlockedExchange(&h12_owner_smoke_retire_start, 1);
  WaitForMultipleObjects(HZ12_OWNER_SMOKE_THREADS, handles, TRUE, INFINITE);
  for (i = 0u; i < HZ12_OWNER_SMOKE_THREADS; ++i) {
    CloseHandle(handles[i]);
    if (!workers[i].success) return 2;
    if (h12_owner_publishable(workers[i].token)) return 3;
  }
  for (i = 0u; i < HZ12_OWNER_SMOKE_THREADS; ++i) {
    uint32_t j;
    int matched_old_slot = 0;
    if (!h12_owner_register(&replacements[i])) return 4;
    if (!h12_owner_publishable(replacements[i])) return 5;
    for (j = 0u; j < HZ12_OWNER_SMOKE_THREADS; ++j) {
      if (replacements[i].slot == workers[j].token.slot) {
        if (replacements[i].generation == workers[j].token.generation) {
          return 6;
        }
        matched_old_slot = 1;
        break;
      }
    }
    if (!matched_old_slot) return 7;
  }
  for (i = 0u; i < HZ12_OWNER_SMOKE_THREADS; ++i) {
    if (h12_owner_publishable(workers[i].token)) return 8;
  }

  h12_owner_registry_stats(&stats);
  if (stats.register_success != 8u || stats.register_reuse != 4u ||
      stats.retire_success != 4u || stats.dead_success != 4u ||
      stats.publish_accept != 12u || stats.publish_dead_reject != 4u ||
      stats.publish_stale_reject != 4u || stats.register_full != 0u ||
      stats.publish_invalid_reject != 0u || stats.invalid_transition != 0u) {
    return 9;
  }
  printf("[HZ12_OWNER_REGISTRY_SMOKE] workers=4 replacements=4 "
         "register=8 reuse=4 retire=4 dead=4 accept=12 dead_reject=4 "
         "stale_reject=4 invalid=0\n");
  h12_owner_registry_dump(stdout);
  return 0;
}
