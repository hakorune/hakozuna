#include <stdint.h>
#include <stdio.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#include "hz12.h"
#include "hz12_owner_registry.h"
#include "hz12_token_inbox.h"

#define HZ12_TOKEN_SMOKE_PRODUCERS 2u
#define HZ12_TOKEN_SMOKE_BATCH_ITEMS 32u
#define HZ12_TOKEN_SMOKE_BATCHES 4u

typedef struct H12TokenSmokeProducer {
  H12OwnerToken original;
  H12OwnerToken replacement;
  int success;
} H12TokenSmokeProducer;

static volatile LONG h12_token_smoke_phase;
static volatile LONG h12_token_smoke_arrived;

static void h12_token_smoke_wait_phase(LONG phase) {
  while (InterlockedCompareExchange(&h12_token_smoke_phase, 0, 0) != phase) {
    SwitchToThread();
  }
}

static int h12_token_smoke_publish(H12OwnerToken token, int expected_accept,
                                   uint32_t batches) {
  uint32_t batch_index;
  for (batch_index = 0u; batch_index < batches; ++batch_index) {
    void* objects[HZ12_TOKEN_SMOKE_BATCH_ITEMS];
    uint32_t i;
    int accepted;
    for (i = 0u; i < HZ12_TOKEN_SMOKE_BATCH_ITEMS; ++i) {
      objects[i] = hz12_malloc(64u);
      if (!objects[i]) {
        uint32_t j;
        for (j = 0u; j < i; ++j) hz12_free(objects[j]);
        return 0;
      }
    }
    accepted = h12_token_inbox_publish(token, objects,
                                       HZ12_TOKEN_SMOKE_BATCH_ITEMS);
    if (accepted != expected_accept) return 0;
  }
  return 1;
}

static unsigned __stdcall h12_token_smoke_producer(void* arg) {
  H12TokenSmokeProducer* state = (H12TokenSmokeProducer*)arg;
  h12_token_smoke_wait_phase(1);
  if (!h12_token_smoke_publish(state->original, 1, HZ12_TOKEN_SMOKE_BATCHES)) {
    return 1;
  }
  InterlockedIncrement(&h12_token_smoke_arrived);

  h12_token_smoke_wait_phase(2);
  if (!h12_token_smoke_publish(state->original, 1, HZ12_TOKEN_SMOKE_BATCHES)) {
    return 2;
  }
  InterlockedIncrement(&h12_token_smoke_arrived);

  h12_token_smoke_wait_phase(3);
  if (!h12_token_smoke_publish(state->original, 0, 1u)) return 3;
  InterlockedIncrement(&h12_token_smoke_arrived);

  h12_token_smoke_wait_phase(4);
  if (!h12_token_smoke_publish(state->original, 0, 1u)) return 4;
  InterlockedIncrement(&h12_token_smoke_arrived);

  h12_token_smoke_wait_phase(5);
  if (!h12_token_smoke_publish(state->replacement, 1,
                               HZ12_TOKEN_SMOKE_BATCHES)) {
    return 5;
  }
  state->success = 1;
  InterlockedIncrement(&h12_token_smoke_arrived);
  return 0;
}

static void h12_token_smoke_run_phase(LONG phase) {
  InterlockedExchange(&h12_token_smoke_arrived, 0);
  InterlockedExchange(&h12_token_smoke_phase, phase);
  while (InterlockedCompareExchange(&h12_token_smoke_arrived, 0, 0) <
         (LONG)HZ12_TOKEN_SMOKE_PRODUCERS) {
    SwitchToThread();
  }
}

int main(void) {
  HANDLE handles[HZ12_TOKEN_SMOKE_PRODUCERS];
  H12TokenSmokeProducer producers[HZ12_TOKEN_SMOKE_PRODUCERS] = {0};
  H12OwnerRegistryStats registry;
  H12TokenInboxStats inbox;
  H12OwnerToken original;
  H12OwnerToken replacement;
  uint32_t i;
  const uint32_t first_drain = HZ12_TOKEN_SMOKE_PRODUCERS *
                               HZ12_TOKEN_SMOKE_BATCHES *
                               HZ12_TOKEN_SMOKE_BATCH_ITEMS * 2u;
  const uint32_t second_drain = HZ12_TOKEN_SMOKE_PRODUCERS *
                                HZ12_TOKEN_SMOKE_BATCHES *
                                HZ12_TOKEN_SMOKE_BATCH_ITEMS;

  h12_owner_registry_reset();
  h12_token_inbox_reset();
  if (!h12_owner_register(&original)) return 1;
  for (i = 0u; i < HZ12_TOKEN_SMOKE_PRODUCERS; ++i) {
    producers[i].original = original;
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_token_smoke_producer,
                                         &producers[i], 0, NULL);
    if (!handles[i]) return 2;
  }

  h12_token_smoke_run_phase(1);
  if (!h12_owner_begin_retire(original)) return 3;
  h12_token_smoke_run_phase(2);
  if (h12_token_inbox_drain(original) != first_drain) return 4;
  if (!h12_owner_mark_dead(original)) return 5;

  h12_token_smoke_run_phase(3);
  if (!h12_owner_register(&replacement)) return 6;
  if (replacement.slot != original.slot ||
      replacement.generation == original.generation) {
    return 7;
  }
  h12_token_smoke_run_phase(4);
  for (i = 0u; i < HZ12_TOKEN_SMOKE_PRODUCERS; ++i) {
    producers[i].replacement = replacement;
  }
  h12_token_smoke_run_phase(5);
  if (h12_token_inbox_drain(replacement) != second_drain) return 8;

  WaitForMultipleObjects(HZ12_TOKEN_SMOKE_PRODUCERS, handles, TRUE, INFINITE);
  for (i = 0u; i < HZ12_TOKEN_SMOKE_PRODUCERS; ++i) {
    CloseHandle(handles[i]);
    if (!producers[i].success) return 9;
  }
  h12_token_inbox_stats(&inbox);
  h12_owner_registry_stats(&registry);
  if (inbox.publish_attempt != 28u || inbox.publish_accept != 24u ||
      inbox.publish_registry_reject != 4u ||
      inbox.publish_generation_reject != 0u || inbox.publish_overflow != 0u ||
      inbox.fallback_objects != 128u || inbox.drain_batches != 2u ||
      inbox.drain_objects != 768u || inbox.drain_generation_reject != 0u ||
      inbox.inbox_current_max != first_drain || registry.register_success != 2u ||
      registry.register_reuse != 1u || registry.retire_success != 1u ||
      registry.dead_success != 1u || registry.publish_accept != 24u ||
      registry.publish_dead_reject != 2u || registry.publish_stale_reject != 2u ||
      registry.invalid_transition != 0u) {
    return 10;
  }
  printf("[HZ12_TOKEN_INBOX_SMOKE] first_drain=%u second_drain=%u "
         "accepted=24 fallback=128 stale=2 dead=2 inbox_max=%u\n",
         first_drain, second_drain, inbox.inbox_current_max);
  h12_token_inbox_dump(stdout);
  h12_owner_registry_dump(stdout);
  return 0;
}
