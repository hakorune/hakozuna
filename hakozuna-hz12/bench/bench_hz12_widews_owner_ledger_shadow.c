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
#include "hz12_reclaim_policy_shadow.h"
#include "hz12_reclaim_entry.h"
#include "hz12_retired_reclaim_decommit.h"
#include "hz12_retired_reclaim_detach.h"
#include "hz12_retired_reclaim_recycle.h"
#include "hz12_snapshot_reclaim.h"
#include "hz12_snapshot_recycle.h"
#include "hz12_shadow.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_depot_core.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_thread_cache.h"
#include "hz12_token_inbox.h"

#ifndef H12_LEDGER_WIDE_SPANS
#define H12_LEDGER_WIDE_SPANS 64u
#endif

#ifndef HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
#define HZ12_SNAPSHOT_RECLAIM_BEHAVIOR 0u
#endif
#ifndef HZ12_SNAPSHOT_RECYCLE_BEHAVIOR
#define HZ12_SNAPSHOT_RECYCLE_BEHAVIOR 0u
#endif
#ifndef HZ12_SNAPSHOT_RECYCLE_ROLLBACK
#define HZ12_SNAPSHOT_RECYCLE_ROLLBACK 0u
#endif
#ifndef HZ12_RECLAIM_POLICY_SHADOW
#define HZ12_RECLAIM_POLICY_SHADOW 0u
#endif
#ifndef HZ12_SNAPSHOT_RECLAIM_NO_CAP
#define HZ12_SNAPSHOT_RECLAIM_NO_CAP 0u
#endif
#ifndef HZ12_RECLAIM_POLICY_BEHAVIOR
#define HZ12_RECLAIM_POLICY_BEHAVIOR 0u
#endif

#ifndef HZ12_OWNER_LEDGER_RECLAIM_BEHAVIOR
#define HZ12_OWNER_LEDGER_RECLAIM_BEHAVIOR 0u
#endif

typedef struct H12LedgerWideState {
  void** objects;
  uint32_t slots_per_span;
  uint32_t total_objects;
  uint8_t class_id;
  H12OwnerToken owner;
  H12OwnerEpochParticipant participant;
  volatile LONG ready;
  volatile LONG published_spans;
  volatile LONG drained_spans;
  volatile LONG retire;
  volatile LONG checkpointed;
} H12LedgerWideState;

#if HZ12_RECLAIM_POLICY_SHADOW
typedef struct H12PolicyLockProbe {
  double* samples_us;
  uint32_t capacity;
  uint32_t count;
  uint8_t class_id;
  volatile LONG ready;
  volatile LONG stop;
} H12PolicyLockProbe;

static int h12_compare_double(const void* left, const void* right) {
  const double a = *(const double*)left;
  const double b = *(const double*)right;
  return (a > b) - (a < b);
}

static unsigned __stdcall h12_policy_lock_probe(void* arg) {
  H12PolicyLockProbe* probe = (H12PolicyLockProbe*)arg;
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  InterlockedExchange(&probe->ready, 1);
  while (InterlockedCompareExchange(&probe->stop, 0, 0) == 0 &&
         probe->count < probe->capacity) {
    LARGE_INTEGER begin;
    LARGE_INTEGER end;
    QueryPerformanceCounter(&begin);
    hz12_returned_lock_probe(probe->class_id);
    QueryPerformanceCounter(&end);
    probe->samples_us[probe->count++] = 1000000.0 *
        (double)(end.QuadPart - begin.QuadPart) /
        (double)frequency.QuadPart;
  }
  return 0u;
}
#endif

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
  state->class_id = class_id;
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
#if HZ12_SNAPSHOT_RECYCLE_BEHAVIOR
  H12SnapshotRecycleResult snapshot_recycle = {0};
  void* recycled_object = NULL;
#endif
#endif
  PROCESS_MEMORY_COUNTERS_EX memory_before = {0};
  PROCESS_MEMORY_COUNTERS_EX memory_after = {0};
  HANDLE owner_thread;
  DWORD exit_code = 0u;
  uint32_t snapshot_candidates = 0u;
  uint32_t snapshot_mismatches = 0u;
#if HZ12_RECLAIM_POLICY_SHADOW
  H12ReclaimPolicyShadow policy_before = {0};
  H12ReclaimPolicyShadow policy_after = {0};
  LARGE_INTEGER policy_frequency;
  LARGE_INTEGER policy_start;
  LARGE_INTEGER policy_end;
  double policy_before_us = 0.0;
  double policy_after_us = 0.0;
  double policy_lock_p99_us = 0.0;
  double policy_lock_max_us = 0.0;
  QueryPerformanceFrequency(&policy_frequency);
#endif

  h12_span_accounting_reset();
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR || HZ12_RECLAIM_POLICY_SHADOW
  h12_span_depot_core_reset();
#endif
#if HZ12_SNAPSHOT_RECLAIM_NO_CAP
  if (h12_span_depot_core_reserve(HZ12_SPAN_DEPOT_CAP) !=
      HZ12_SPAN_DEPOT_CAP) {
    return 21;
  }
#endif
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
  h12_reclaim_entry_reset();
#endif
  h12_span_owner_shadow_reset();
  h12_owner_batch_ledger_reset();
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  if (!h12_owner_register(&state.owner)) return 1;
#if HZ12_SNAPSHOT_RECLAIM_BEHAVIOR
  if (!h12_reclaim_entry_begin(state.owner) ||
      h12_reclaim_entry_begin(state.owner) ||
      !h12_reclaim_entry_end(state.owner) ||
      !h12_reclaim_entry_begin(state.owner) ||
      !h12_reclaim_entry_end(state.owner)) {
    return 20;
  }
#endif
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
#if HZ12_RECLAIM_POLICY_SHADOW
  QueryPerformanceCounter(&policy_start);
  if (!h12_reclaim_policy_shadow_query(state.owner, &policy_before) ||
      policy_before.would_reclaim) {
    return 18;
  }
  QueryPerformanceCounter(&policy_end);
  policy_before_us = 1000000.0 *
      (double)(policy_end.QuadPart - policy_start.QuadPart) /
      (double)policy_frequency.QuadPart;
#endif
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
#if HZ12_RECLAIM_POLICY_SHADOW
  {
    const uint32_t expected_planned =
        H12_LEDGER_WIDE_SPANS < HZ12_RECLAIM_POLICY_MAX_SPANS
            ? H12_LEDGER_WIDE_SPANS : HZ12_RECLAIM_POLICY_MAX_SPANS;
    const uint8_t expected_threshold = (uint8_t)(
        (uint64_t)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES >=
        HZ12_RECLAIM_POLICY_MIN_BYTES);
    H12PolicyLockProbe lock_probe = {0};
    HANDLE lock_probe_thread;
    lock_probe.capacity = 65536u;
    lock_probe.class_id = state.class_id;
    lock_probe.samples_us = (double*)calloc(lock_probe.capacity,
                                            sizeof(double));
    if (!lock_probe.samples_us) return 23;
    lock_probe_thread = (HANDLE)_beginthreadex(
        NULL, 0, h12_policy_lock_probe, &lock_probe, 0, NULL);
    if (!lock_probe_thread) return 24;
    while (InterlockedCompareExchange(&lock_probe.ready, 0, 0) == 0) {
      SwitchToThread();
    }
    QueryPerformanceCounter(&policy_start);
    if (!h12_reclaim_policy_shadow_query(state.owner, &policy_after)) {
      return 19;
    }
    QueryPerformanceCounter(&policy_end);
    InterlockedExchange(&lock_probe.stop, 1);
    WaitForSingleObject(lock_probe_thread, INFINITE);
    CloseHandle(lock_probe_thread);
    if (lock_probe.count == 0u) return 25;
    qsort(lock_probe.samples_us, lock_probe.count, sizeof(double),
          h12_compare_double);
    policy_lock_p99_us = lock_probe.samples_us[
        (size_t)(lock_probe.count - 1u) * 99u / 100u];
    policy_lock_max_us = lock_probe.samples_us[lock_probe.count - 1u];
    free(lock_probe.samples_us);
    policy_after_us = 1000000.0 *
        (double)(policy_end.QuadPart - policy_start.QuadPart) /
        (double)policy_frequency.QuadPart;
    if (
      !policy_after.owner_gate_open || !policy_after.flush_owner_found ||
      policy_after.flush_owner_pending != 0u ||
      policy_after.owner_spans != H12_LEDGER_WIDE_SPANS ||
      policy_after.complete_spans != H12_LEDGER_WIDE_SPANS ||
      policy_after.incomplete_spans != 0u ||
      policy_after.reclaimable_bytes !=
          (uint64_t)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES ||
      policy_after.planned_spans != expected_planned ||
      policy_after.depot_available != HZ12_SPAN_DEPOT_CAP ||
      policy_after.planned_bytes !=
          (uint64_t)expected_planned * HZ12_SPAN_BYTES ||
      policy_after.threshold_met != expected_threshold ||
      policy_after.would_reclaim != expected_threshold) {
      return 19;
    }
  }
#endif
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
#if HZ12_SNAPSHOT_RECLAIM_NO_CAP
  if (h12_snapshot_reclaim_retired_bounded(
          state.owner, H12_LEDGER_WIDE_SPANS, &snapshot_reclaim) ||
      snapshot_reclaim.reserved != 0u || snapshot_reclaim.detached != 0u ||
      snapshot_reclaim.decommitted != 0u ||
      snapshot_reclaim.depot_inserted != 0u ||
      snapshot_reclaim.owner_cleared != 0u ||
      snapshot_reclaim.limbo_spans != 0u || snapshot_reclaim.failed != 0u) {
    return 22;
  }
#else
#if HZ12_RECLAIM_POLICY_BEHAVIOR
  if (!policy_after.would_reclaim) return 26;
#endif
  if (!h12_snapshot_reclaim_retired_bounded(
          state.owner, H12_LEDGER_WIDE_SPANS, &snapshot_reclaim) ||
      snapshot_reclaim.candidates != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.detached != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.decommitted != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.reserved != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.depot_inserted != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.owner_cleared != H12_LEDGER_WIDE_SPANS ||
      snapshot_reclaim.limbo_spans != 0u ||
      snapshot_reclaim.failed != 0u ||
      snapshot_reclaim.decommitted_bytes !=
          (uint64_t)H12_LEDGER_WIDE_SPANS * HZ12_SPAN_BYTES) {
    return 12;
  }
#endif
  memory_after.cb = sizeof(memory_after);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_after, sizeof(memory_after));
#if HZ12_SNAPSHOT_RECYCLE_BEHAVIOR
#if HZ12_SNAPSHOT_RECYCLE_ROLLBACK
  recycled_object = hz12_malloc(64u);
  if (!recycled_object ||
      h12_snapshot_recycle_take(state.class_id, &snapshot_recycle) ||
      !snapshot_recycle.rollback || snapshot_recycle.recommitted ||
      snapshot_recycle.route_attached || snapshot_recycle.owner_assigned ||
      snapshot_recycle.current_installed ||
      h12_span_depot_core_count() != H12_LEDGER_WIDE_SPANS) {
    return 16;
  }
  {
    MEMORY_BASIC_INFORMATION memory;
    H12OwnerToken unexpected_owner;
    uint32_t route_owner;
    uint32_t route_generation;
    uint32_t recycled_span_id = (uint32_t)(
        ((uintptr_t)snapshot_recycle.span_base -
         (uintptr_t)hz12_arena_base) >> HZ12_SPAN_SHIFT);
    if (VirtualQuery(snapshot_recycle.span_base, &memory, sizeof(memory)) !=
            sizeof(memory) || memory.State != MEM_RESERVE ||
        hz12_span_class[recycled_span_id] != 0u ||
        h12_shadow_owner_token_for_ptr(snapshot_recycle.span_base,
                                       &route_owner, &route_generation) ||
        h12_span_owner_shadow_query(recycled_span_id, &unexpected_owner)) {
      return 17;
    }
  }
  hz12_free(recycled_object);
  hz12_thread_cache_reclaim_checkpoint();
#else
  if (!h12_snapshot_recycle_take(state.class_id, &snapshot_recycle) ||
      !snapshot_recycle.recommitted || !snapshot_recycle.route_attached ||
      !snapshot_recycle.owner_assigned ||
      !snapshot_recycle.current_installed || snapshot_recycle.rollback ||
      h12_span_depot_core_count() != H12_LEDGER_WIDE_SPANS - 1u) {
    return 13;
  }
  recycled_object = hz12_malloc(64u);
  if (recycled_object != snapshot_recycle.span_base) return 14;
  hz12_free(recycled_object);
  hz12_thread_cache_reclaim_checkpoint();
#endif
#endif
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
         "decommitted=%u reserved=%u depot=%u owner_cleared=%u limbo=%u "
         "failed=%u bytes=%llu rss_delta=%lld\n",
         snapshot_reclaim.budget, snapshot_reclaim.candidates,
         snapshot_reclaim.detached, snapshot_reclaim.decommitted,
         snapshot_reclaim.reserved,
         snapshot_reclaim.depot_inserted,
         snapshot_reclaim.owner_cleared, snapshot_reclaim.limbo_spans,
         snapshot_reclaim.failed,
         (unsigned long long)snapshot_reclaim.decommitted_bytes,
         (long long)memory_before.WorkingSetSize -
             (long long)memory_after.WorkingSetSize);
#endif
#if HZ12_SNAPSHOT_RECYCLE_BEHAVIOR
#if HZ12_SNAPSHOT_RECYCLE_ROLLBACK
  printf("[HZ12_SNAPSHOT_RECYCLE_ROLLBACK] class=%u rollback=%u "
         "depot_remaining=%u reserve=1 route=0 owner=0\n",
         (unsigned)snapshot_recycle.class_id,
         (unsigned)snapshot_recycle.rollback, h12_span_depot_core_count());
#else
  {
    H12OwnerToken recycled_owner;
    uint32_t route_owner;
    uint32_t route_generation;
    uint32_t recycled_span_id;
    recycled_span_id = (uint32_t)(
        ((uintptr_t)snapshot_recycle.span_base -
         (uintptr_t)hz12_arena_base) >> HZ12_SPAN_SHIFT);
    if (!hz12_tls || !hz12_tls->flush_owner_valid ||
        !h12_shadow_owner_token_for_ptr(snapshot_recycle.span_base,
                                        &route_owner, &route_generation) ||
        !h12_span_owner_shadow_query(recycled_span_id, &recycled_owner) ||
        route_owner != hz12_tls->flush_owner_id ||
        route_generation != hz12_tls->flush_owner_generation ||
        recycled_owner.slot != hz12_tls->flush_owner_id ||
        recycled_owner.generation != hz12_tls->flush_owner_generation ||
        (recycled_owner.slot == state.owner.slot &&
         recycled_owner.generation == state.owner.generation)) {
      return 15;
    }
  }
  printf("[HZ12_SNAPSHOT_RECYCLE] class=%u recommitted=%u route=%u owner=%u "
         "current=%u rollback=%u depot_remaining=%u exercised=%u\n",
         (unsigned)snapshot_recycle.class_id,
         (unsigned)snapshot_recycle.recommitted,
         (unsigned)snapshot_recycle.route_attached,
         (unsigned)snapshot_recycle.owner_assigned,
         (unsigned)snapshot_recycle.current_installed,
         (unsigned)snapshot_recycle.rollback, h12_span_depot_core_count(),
         recycled_object == snapshot_recycle.span_base);
#endif
#endif
#if HZ12_RECLAIM_POLICY_SHADOW
  printf("[HZ12_RECLAIM_POLICY_SHADOW] before=%u after=%u owner_spans=%u "
         "complete=%u incomplete=%u reclaimable=%llu planned_spans=%u "
         "planned_bytes=%llu depot_available=%u pending=%u "
         "before_us=%.3f after_us=%.3f lock_p99_us=%.3f lock_max_us=%.3f\n",
         (unsigned)policy_before.would_reclaim,
         (unsigned)policy_after.would_reclaim, policy_after.owner_spans,
         policy_after.complete_spans, policy_after.incomplete_spans,
         (unsigned long long)policy_after.reclaimable_bytes,
         policy_after.planned_spans,
         (unsigned long long)policy_after.planned_bytes,
         policy_after.depot_available,
         policy_after.flush_owner_pending, policy_before_us, policy_after_us,
         policy_lock_p99_us, policy_lock_max_us);
#endif
#if HZ12_RECLAIM_POLICY_BEHAVIOR
  printf("[HZ12_RECLAIM_POLICY_BEHAVIOR] authorized=%u reserved=%u "
         "decommitted=%u depot=%u owner_cleared=%u limbo=%u\n",
         (unsigned)policy_after.would_reclaim, snapshot_reclaim.reserved,
         snapshot_reclaim.decommitted, snapshot_reclaim.depot_inserted,
         snapshot_reclaim.owner_cleared, snapshot_reclaim.limbo_spans);
#endif
  free(state.objects);
  return 0;
}
