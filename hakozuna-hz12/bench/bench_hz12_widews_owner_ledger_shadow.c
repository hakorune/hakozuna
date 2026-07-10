#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>

#include "hz12.h"
#include "hz12_flush_owner_route.h"
#include "hz12_owner_batch_ledger.h"
#include "hz12_owner_epoch.h"
#include "hz12_owner_ledger_retire_gate.h"
#include "hz12_owner_registry.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_retired_reclaim_decommit.h"
#include "hz12_retired_reclaim_detach.h"
#include "hz12_retired_reclaim_recycle.h"
#include "hz12_snapshot_reclaim.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_depot_core.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_thread_cache.h"
#include "hz12_token_inbox.h"

#define H12_LEDGER_WIDE_SPANS 64u

#ifndef HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
#define HZ12_SNAPSHOT_RECLAIM_BEHAVIOR 0u
#endif

#ifndef HZ12_OWNER_LEDGER_RECLAIM_BEHAVIOR
#define HZ12_OWNER_LEDGER_RECLAIM_BEHAVIOR 0u
#endif

typedef struct H12LedgerWideState {
  void** objects;
  uint32_t slots_per_span;
  uint32_t total_objects;
  H12OwnerToken owner;
  H12OwnerEpochParticipant participant;
  volatile LONG ready;
  volatile LONG published_spans;
  volatile LONG drained_spans;
  volatile LONG retire;
  volatile LONG checkpointed;
} H12LedgerWideState;

static unsigned __stdcall h12_ledger_wide_owner(void* arg) {
  H12LedgerWideState* state = (H12LedgerWideState*)arg;
  uint8_t class_id;
  size_t slot_bytes;
  uint32_t span_id;
  if (!h12_owner_epoch_register(&state->participant)) return 1u;
  state->objects = (void**)calloc(H12_LEDGER_WIDE_SPANS * 1024u,
                                  sizeof(void*));
  if (!state->objects) return 2u;
  state->objects[0] = hz12_malloc(64u);
  if (!state->objects[0] ||
      !hz12_span_classify(state->objects[0], &class_id)) return 3u;
  slot_bytes = hz12_class_slot_size(class_id);
  if (slot_bytes == 0u) return 4u;
  state->slots_per_span = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  state->total_objects = H12_LEDGER_WIDE_SPANS * state->slots_per_span;
  h12_span_accounting_on_alloc(state->objects[0]);
  for (uint32_t i = 1u; i < state->total_objects; ++i) {
    state->objects[i] = hz12_malloc(64u);
    if (!state->objects[i]) return 5u;
    h12_span_accounting_on_alloc(state->objects[i]);
  }
  span_id = (uint32_t)(((uintptr_t)state->objects[0] -
                        (uintptr_t)hz12_arena_base) >> HZ12_SPAN_SHIFT);
  {
    H12OwnerToken observed;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != state->owner.slot ||
        observed.generation != state->owner.generation) {
      return 6u;
    }
  }
  InterlockedExchange(&state->ready, 1);
  for (uint32_t span = 0u; span < H12_LEDGER_WIDE_SPANS; ++span) {
    while (InterlockedCompareExchange(&state->published_spans, 0, 0) <=
           (LONG)span) {
      SwitchToThread();
    }
    hz12_flush_owner_route_drain(hz12_tls);
    hz12_thread_cache_reclaim_checkpoint();
    InterlockedIncrement(&state->drained_spans);
  }
  while (InterlockedCompareExchange(&state->retire, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!h12_owner_epoch_checkpoint(state->participant)) return 7u;
  if (!h12_owner_epoch_unregister(state->participant)) return 8u;
  InterlockedExchange(&state->checkpointed, 1);
  return 0u;
}

int main(void) {
  H12LedgerWideState state = {0};
  H12OwnerBatchLedgerAgreement agreement;
  H12OwnerLedgerRetireGate gate;
  H12RetiredReclaimDetachResult detach = {0};
  H12RetiredReclaimDecommitResult decommit = {0};
  H12RetiredReclaimRecycleResult recycle = {0};
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
  H12SnapshotReclaimResult snapshot_reclaim = {0};
#endif
  PROCESS_MEMORY_COUNTERS_EX memory_before = {0};
  PROCESS_MEMORY_COUNTERS_EX memory_after = {0};
  HANDLE owner_thread;
  DWORD exit_code = 0u;
  uint32_t snapshot_candidates = 0u;
  uint32_t snapshot_mismatches = 0u;

  h12_span_accounting_reset();
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
  h12_span_depot_core_reset();
#endif
  h12_span_owner_shadow_reset();
  h12_owner_batch_ledger_reset();
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  if (!h12_owner_register(&state.owner)) return 1;
  owner_thread = (HANDLE)_beginthreadex(NULL, 0, h12_ledger_wide_owner,
                                        &state, 0, NULL);
  if (!owner_thread) return 2;
  while (InterlockedCompareExchange(&state.ready, 0, 0) == 0) {
    SwitchToThread();
  }
  for (uint32_t span = 0u; span < H12_LEDGER_WIDE_SPANS; ++span) {
    uint32_t begin = span * state.slots_per_span;
    for (uint32_t i = 0u; i < state.slots_per_span; ++i) {
      void* ptr = state.objects[begin + i];
      h12_span_accounting_on_release(ptr);
      hz12_free(ptr);
    }
    hz12_thread_cache_reclaim_checkpoint();
    InterlockedIncrement(&state.published_spans);
    while (InterlockedCompareExchange(&state.drained_spans, 0, 0) <=
           (LONG)span) {
      SwitchToThread();
    }
  }
  if (!h12_owner_begin_retire(state.owner) ||
      !h12_owner_epoch_begin_retire(state.owner)) {
    return 3;
  }
  InterlockedExchange(&state.retire, 1);
  while (InterlockedCompareExchange(&state.checkpointed, 0, 0) == 0) {
    if (WaitForSingleObject(owner_thread, 0u) == WAIT_OBJECT_0) break;
    SwitchToThread();
  }
  WaitForSingleObject(owner_thread, INFINITE);
  GetExitCodeThread(owner_thread, &exit_code);
  CloseHandle(owner_thread);
  if (exit_code != 0u || state.checkpointed == 0) return 4;
  agreement = h12_owner_batch_ledger_compare_owner_atomic(state.owner);
  if (agreement.compared_spans != H12_LEDGER_WIDE_SPANS ||
      agreement.matching_spans != H12_LEDGER_WIDE_SPANS ||
      agreement.mismatched_spans != 0u ||
      agreement.ledger_candidates != H12_LEDGER_WIDE_SPANS ||
      agreement.atomic_candidates != H12_LEDGER_WIDE_SPANS) {
    return 5;
  }
  if (!h12_owner_ledger_retire_gate_query(state.owner, &gate) ||
      !gate.gate_open || gate.flush_owner_pending != 0u ||
      gate.compared_spans != H12_LEDGER_WIDE_SPANS ||
      gate.mismatched_spans != 0u) {
    return 6;
  }
  for (uint32_t span_id = 0u; span_id < HZ12_SPAN_COUNT; ++span_id) {
    H12OwnerToken observed;
    H12ReturnedSpanSnapshot snapshot;
    void* span_base;
    uint8_t class_plus_one;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != state.owner.slot ||
        observed.generation != state.owner.generation) {
      continue;
    }
    class_plus_one = hz12_span_class[span_id];
    if (class_plus_one == 0u) continue;
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!hz12_returned_snapshot_span(
            (uint8_t)(class_plus_one - 1u), span_base, &snapshot) ||
        !snapshot.complete) {
      snapshot_mismatches += 1u;
    } else {
      snapshot_candidates += 1u;
    }
  }
  if (snapshot_candidates != H12_LEDGER_WIDE_SPANS ||
      snapshot_mismatches != 0u) {
    return 11;
  }
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
  memory_before.cb = sizeof(memory_before);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_before, sizeof(memory_before));
  if (!h12_snapshot_reclaim_retired_bounded(
          state.owner, H12_LEDGER_WIDE_SPANS, &snapshot_reclaim) ||
      snapshot_reclaim.candidates != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.detached != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.decommitted != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.depot_inserted != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.failed != 0u ||
      snapshot_reclaim.decommitted_bytes !=
          (uint64_t)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES) {
    return 12;
  }
  memory_after.cb = sizeof(memory_after);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_after, sizeof(memory_after));
#elif HZ12_OWNER_LEDGER_RECLAIM_BEHAVIOR
  memory_before.cb = sizeof(memory_before);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_before, sizeof(memory_before));
  if (!h12_retired_reclaim_detach_bounded(
          state.owner, H12_LEDGER_WIDE_SPANS, &detach) ||
      detach.succeeded != H12_LEDGER_WIDE_SPANS || detach.failed != 0u ||
      detach.detached_bytes !=
          (uint64_t)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES) {
    return 7;
  }
  if (!h12_retired_reclaim_decommit_bounded(
          state.owner, H12_LEDGER_WIDE_SPANS, &decommit) ||
      decommit.succeeded != H12_LEDGER_WIDE_SPANS || decommit.failed != 0u ||
      decommit.decommitted_bytes !=
          (uint64_t)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES) {
    return 8;
  }
  if (!h12_retired_reclaim_recycle_bounded(
          state.owner, H12_LEDGER_WIDE_SPANS, &recycle) ||
      recycle.spans_exercised != H12_LEDGER_WIDE_SPANS ||
      recycle.second_detach != H12_LEDGER_WIDE_SPANS ||
      recycle.second_decommit != H12_LEDGER_WIDE_SPANS ||
      recycle.final_put != H12_LEDGER_WIDE_SPANS ||
      recycle.failures != 0u || recycle.address_mismatch != 0u ||
      recycle.owner_mismatch != 0u) {
    return 9;
  }
  agreement = h12_owner_batch_ledger_compare_owner_atomic(state.owner);
  if (agreement.compared_spans != H12_LEDGER_WIDE_SPANS ||
      agreement.matching_spans != H12_LEDGER_WIDE_SPANS ||
      agreement.mismatched_spans != 0u ||
      agreement.ledger_candidates != H12_LEDGER_WIDE_SPANS ||
      agreement.atomic_candidates != H12_LEDGER_WIDE_SPANS) {
    fprintf(stderr,
            "[HZ12_L6F_AGREEMENT_FAIL] compared=%u matching=%u "
            "mismatched=%u ledger_candidates=%u atomic_candidates=%u\n",
            agreement.compared_spans, agreement.matching_spans,
            agreement.mismatched_spans, agreement.ledger_candidates,
            agreement.atomic_candidates);
    return 10;
  }
  memory_after.cb = sizeof(memory_after);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_after, sizeof(memory_after));
#endif
  printf("[HZ12_WIDEWS_OWNER_LEDGER_SHADOW] spans=%u slots=%u "
         "candidate_bytes=%llu pending=%u matches=%u mismatches=%u gate=%u "
         "detached=%u decommitted=%u recycled=%u detach2=%u decommit2=%u "
         "depot=%u snapshot=%u snapshot_mismatch=%u "
         "rss_before=%llu rss_after=%llu\n",
         H12_LEDGER_WIDE_SPANS, state.total_objects,
         (unsigned long long)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES,
         gate.flush_owner_pending, agreement.matching_spans,
         agreement.mismatched_spans, (unsigned)gate.gate_open,
         detach.succeeded, decommit.succeeded,
         recycle.spans_exercised, recycle.second_detach,
         recycle.second_decommit, recycle.depot_final,
         snapshot_candidates, snapshot_mismatches,
         (unsigned long long)memory_before.WorkingSetSize,
         (unsigned long long)memory_after.WorkingSetSize);
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
  printf("[HZ12_SNAPSHOT_RECLAIM] budget=%u candidates=%u detached=%u "
         "decommitted=%u depot=%u failed=%u bytes=%llu rss_delta=%lld\n",
         snapshot_reclaim.budget, snapshot_reclaim.candidates,
         snapshot_reclaim.detached, snapshot_reclaim.decommitted,
         snapshot_reclaim.depot_inserted,
         snapshot_reclaim.failed,
         (unsigned long long)snapshot_reclaim.decommitted_bytes,
         (long long)memory_before.WorkingSetSize -
             (long long)memory_after.WorkingSetSize);
#endif
  free(state.objects);
  return 0;
}
