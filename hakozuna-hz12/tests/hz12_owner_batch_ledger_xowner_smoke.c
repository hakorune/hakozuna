#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "hz12.h"
#include "hz12_flush_owner_route.h"
#include "hz12_owner_batch_ledger.h"
#include "hz12_owner_epoch.h"
#include "hz12_owner_ledger_retire_gate.h"
#include "hz12_owner_registry.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_thread_cache.h"
#include "hz12_token_inbox.h"

typedef struct H12LedgerXownerState {
  void** objects;
  uint32_t object_count;
  H12OwnerToken owner;
  H12OwnerEpochParticipant participant;
  volatile LONG ready;
  volatile LONG foreign_flushed;
  volatile LONG owner_drained;
} H12LedgerXownerState;

static unsigned __stdcall h12_ledger_owner_thread(void* arg) {
  H12LedgerXownerState* state = (H12LedgerXownerState*)arg;
  void* first;
  uint8_t class_id;
  size_t slot_bytes;
  uint32_t span_id;
  if (!h12_owner_epoch_register(&state->participant)) return 1u;
  first = hz12_malloc(64u);
  if (!first || !hz12_span_classify(first, &class_id)) return 2u;
  slot_bytes = hz12_class_slot_size(class_id);
  if (slot_bytes == 0u) return 3u;
  state->object_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  state->objects = (void**)calloc(state->object_count, sizeof(void*));
  if (!state->objects) return 4u;
  state->objects[0] = first;
  h12_span_accounting_on_alloc(first);
  for (uint32_t i = 1u; i < state->object_count; ++i) {
    state->objects[i] = hz12_malloc(64u);
    if (!state->objects[i]) return 5u;
    h12_span_accounting_on_alloc(state->objects[i]);
  }
  span_id = (uint32_t)(((uintptr_t)first - (uintptr_t)hz12_arena_base) >>
                       HZ12_SPAN_SHIFT);
  {
    H12OwnerToken observed;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != state->owner.slot ||
        observed.generation != state->owner.generation) {
      return 6u;
    }
  }
  InterlockedExchange(&state->ready, 1);
  while (InterlockedCompareExchange(&state->foreign_flushed, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!hz12_tls) return 7u;
  hz12_flush_owner_route_drain(hz12_tls);
  hz12_thread_cache_reclaim_checkpoint();
  if (!h12_owner_epoch_checkpoint(state->participant)) return 8u;
  if (!h12_owner_epoch_unregister(state->participant)) return 9u;
  InterlockedExchange(&state->owner_drained, 1);
  return 0u;
}

int main(void) {
  H12LedgerXownerState state = {0};
  H12OwnerBatchLedgerAgreement agreement;
  H12OwnerBatchLedgerRecord ledger;
  H12OwnerLedgerRetireGate before_gate;
  H12OwnerLedgerRetireGate after_gate;
  HANDLE owner_thread;
  DWORD exit_code = 0u;

  h12_span_accounting_reset();
  h12_span_owner_shadow_reset();
  h12_owner_batch_ledger_reset();
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  if (!h12_owner_register(&state.owner)) return 1;
  owner_thread = (HANDLE)_beginthreadex(NULL, 0, h12_ledger_owner_thread,
                                        &state, 0, NULL);
  if (!owner_thread) return 2;
  while (InterlockedCompareExchange(&state.ready, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!state.objects || state.object_count == 0u) return 3;
  for (uint32_t i = 0u; i < state.object_count; ++i) {
    h12_span_accounting_on_release(state.objects[i]);
    hz12_free(state.objects[i]);
  }
  hz12_thread_cache_reclaim_checkpoint();
  if (!h12_owner_batch_ledger_query(state.objects[0], &ledger) ||
      ledger.outstanding_slots != state.object_count ||
      ledger.owner_drain_objects != 0u || ledger.ledger_candidate) {
    return 4;
  }
  if (!h12_owner_begin_retire(state.owner) ||
      !h12_owner_epoch_begin_retire(state.owner)) {
    return 5;
  }
  if (!h12_owner_ledger_retire_gate_query(state.owner, &before_gate) ||
      before_gate.gate_open || before_gate.flush_owner_pending == 0u) {
    return 6;
  }
  InterlockedExchange(&state.foreign_flushed, 1);
  while (InterlockedCompareExchange(&state.owner_drained, 0, 0) == 0) {
    if (WaitForSingleObject(owner_thread, 0u) == WAIT_OBJECT_0) break;
    SwitchToThread();
  }
  WaitForSingleObject(owner_thread, INFINITE);
  GetExitCodeThread(owner_thread, &exit_code);
  CloseHandle(owner_thread);
  if (exit_code != 0u || state.owner_drained == 0) return 7;
  if (!h12_owner_batch_ledger_query(state.objects[0], &ledger) ||
      !ledger.ledger_candidate || ledger.outstanding_slots != 0u ||
      ledger.owner_drain_objects != state.object_count ||
      ledger.rejected_transitions != 0u) {
    return 8;
  }
  agreement = h12_owner_batch_ledger_compare_atomic();
  if (agreement.compared_spans != 1u || agreement.matching_spans != 1u ||
      agreement.mismatched_spans != 0u) {
    return 9;
  }
  if (!h12_owner_ledger_retire_gate_query(state.owner, &after_gate) ||
      !after_gate.gate_open || after_gate.flush_owner_pending != 0u ||
      !after_gate.ledger_agrees || after_gate.mismatched_spans != 0u) {
    return 10;
  }
  printf("[HZ12_OWNER_BATCH_LEDGER_XOWNER_SMOKE] slots=%u owner=%u:%u "
         "drained=%u outstanding=%u pending_before=%u pending_after=%u "
         "match=%u mismatch=%u gate=%u\n",
         state.object_count, state.owner.slot, state.owner.generation,
         ledger.owner_drain_objects, ledger.outstanding_slots,
         before_gate.flush_owner_pending, after_gate.flush_owner_pending,
         agreement.matching_spans, agreement.mismatched_spans,
         (unsigned)after_gate.gate_open);
  free(state.objects);
  return 0;
}
