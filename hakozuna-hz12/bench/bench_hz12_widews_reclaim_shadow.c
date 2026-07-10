// HZ12 L5-A wide working-set attribution. Diagnostic-only, no reclaim behavior.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#include <psapi.h>
#include "hz12.h"
#include "hz12_owner_epoch.h"
#include "hz12_owner_registry.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_retired_reclaim_shadow.h"
#include "hz12_retired_reclaim_detach.h"
#include "hz12_retired_reclaim_decommit.h"
#include "hz12_retired_reclaim_depot_cycle.h"
#include "hz12_retired_reclaim_recycle.h"
#include "hz12_span_accounting.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_span_depot.h"
#include "hz12_token_inbox.h"
#include "hz12_thread_cache.h"

typedef struct H12WideWsThread {
  H12OwnerToken owner;
  uint32_t seed;
  uint32_t iters;
  uint32_t working_set;
  uint64_t allocs;
  uint64_t frees;
  H12OwnerEpochParticipant participant;
} H12WideWsThread;

static volatile LONG h12_widews_work_done;
static volatile LONG h12_widews_retire_start;
static volatile LONG h12_widews_checkpoint_done;

static uint32_t h12_widews_rng(uint32_t* state) {
  *state = (*state * 1664525u) + 1013904223u;
  return *state;
}

static unsigned __stdcall h12_widews_worker(void* arg) {
  H12WideWsThread* state = (H12WideWsThread*)arg;
  void** slots = (void**)calloc(state->working_set, sizeof(void*));
  uint32_t i;
  if (!slots || !h12_owner_epoch_register(&state->participant)) return 1;
  for (i = 0u; i < state->iters; ++i) {
    uint32_t random = h12_widews_rng(&state->seed);
    uint32_t index = random % state->working_set;
    if (slots[index]) {
      h12_span_accounting_on_release(slots[index]);
      hz12_free(slots[index]);
      slots[index] = NULL;
      state->frees += 1u;
    } else {
      size_t size = 16u + (random % 1009u);
      void* ptr = hz12_malloc(size);
      if (!ptr) continue;
      h12_span_accounting_on_alloc(ptr);
      (void)h12_span_owner_shadow_assign(ptr, state->owner);
      memset(ptr, 0xA5, size < 64u ? size : 64u);
      slots[index] = ptr;
      state->allocs += 1u;
    }
  }
  for (i = 0u; i < state->working_set; ++i) {
    if (slots[i]) {
      h12_span_accounting_on_release(slots[i]);
      hz12_free(slots[i]);
      state->frees += 1u;
    }
  }
  free(slots);
  InterlockedIncrement(&h12_widews_work_done);
  while (InterlockedCompareExchange(&h12_widews_retire_start, 0, 0) == 0) {
    SwitchToThread();
  }
  hz12_thread_cache_reclaim_checkpoint();
  if (!h12_owner_epoch_checkpoint(state->participant)) return 2;
  InterlockedIncrement(&h12_widews_checkpoint_done);
  if (!h12_owner_epoch_unregister(state->participant)) return 3;
  return 0;
}

int main(int argc, char** argv) {
  uint32_t thread_count = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 8u;
  uint32_t iters = argc > 2 ? (uint32_t)strtoul(argv[2], NULL, 10) : 200000u;
  uint32_t working_set = argc > 3 ? (uint32_t)strtoul(argv[3], NULL, 10) : 16384u;
  uint32_t detach_budget = argc > 4 ? (uint32_t)strtoul(argv[4], NULL, 10) : 0u;
  uint32_t decommit_budget = argc > 5 ? (uint32_t)strtoul(argv[5], NULL, 10) : 0u;
  uint32_t depot_budget = argc > 6 ? (uint32_t)strtoul(argv[6], NULL, 10) : 0u;
  uint32_t recycle_budget = argc > 7 ? (uint32_t)strtoul(argv[7], NULL, 10) : 0u;
  H12WideWsThread* states;
  HANDLE* handles;
  H12OwnerToken owner;
  H12RetiredReclaimShadow result;
  H12RetiredReclaimShadow after_detach;
  H12RetiredReclaimDetachResult detach;
  H12RetiredReclaimDecommitResult decommit;
  H12RetiredReclaimDepotCycleResult depot_cycle;
  H12RetiredReclaimRecycleResult recycle;
  PROCESS_MEMORY_COUNTERS_EX memory_before_decommit = {0};
  PROCESS_MEMORY_COUNTERS_EX memory_after_decommit = {0};
  uint64_t allocs = 0u;
  uint64_t frees = 0u;
  uint64_t start;
  uint64_t elapsed;
  uint32_t i;
  if (thread_count == 0u || thread_count > 32u || iters == 0u ||
      working_set == 0u || (depot_budget != 0u && recycle_budget != 0u)) {
    return 1;
  }
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  h12_span_owner_shadow_reset();
  h12_span_accounting_reset();
  if (!h12_owner_register(&owner)) return 2;
  states = (H12WideWsThread*)calloc(thread_count, sizeof(*states));
  handles = (HANDLE*)calloc(thread_count, sizeof(*handles));
  if (!states || !handles) return 3;
  start = GetTickCount64();
  for (i = 0u; i < thread_count; ++i) {
    states[i].owner = owner;
    states[i].seed = 0x9e3779b9u ^ (i * 0x85ebca6bu);
    states[i].iters = iters;
    states[i].working_set = working_set;
    handles[i] = (HANDLE)_beginthreadex(NULL, 0, h12_widews_worker,
                                         &states[i], 0, NULL);
    if (!handles[i]) return 4;
  }
  while (InterlockedCompareExchange(&h12_widews_work_done, 0, 0) <
         (LONG)thread_count) {
    SwitchToThread();
  }
  if (!h12_owner_begin_retire(owner) ||
      !h12_owner_epoch_begin_retire(owner)) return 6;
  InterlockedExchange(&h12_widews_retire_start, 1);
  while (InterlockedCompareExchange(&h12_widews_checkpoint_done, 0, 0) <
         (LONG)thread_count) {
    SwitchToThread();
  }
  WaitForMultipleObjects(thread_count, handles, TRUE, INFINITE);
  elapsed = GetTickCount64() - start;
  for (i = 0u; i < thread_count; ++i) {
    DWORD code = 0u;
    GetExitCodeThread(handles[i], &code);
    CloseHandle(handles[i]);
    if (code != 0u) return 5;
    allocs += states[i].allocs;
    frees += states[i].frees;
  }
  result = h12_retired_reclaim_shadow_scan(owner, 1);
  memset(&detach, 0, sizeof(detach));
  memset(&decommit, 0, sizeof(decommit));
  memset(&depot_cycle, 0, sizeof(depot_cycle));
  memset(&recycle, 0, sizeof(recycle));
  after_detach = result;
  if (detach_budget != 0u) {
    if (!h12_retired_reclaim_detach_bounded(owner, detach_budget, &detach)) {
      return 7;
    }
    after_detach = h12_retired_reclaim_shadow_scan(owner, 1);
  }
  memory_before_decommit.cb = sizeof(memory_before_decommit);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_before_decommit,
      sizeof(memory_before_decommit));
  if (decommit_budget != 0u) {
    if (!h12_retired_reclaim_decommit_bounded(
            owner, decommit_budget, &decommit)) return 8;
  }
  memory_after_decommit.cb = sizeof(memory_after_decommit);
  (void)GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory_after_decommit,
      sizeof(memory_after_decommit));
  if (depot_budget != 0u) {
    h12_span_depot_reset();
    if (!h12_retired_reclaim_depot_cycle_bounded(
            owner, depot_budget, &depot_cycle)) {
      h12_retired_reclaim_depot_cycle_dump(stdout, &depot_cycle);
      return 10;
    }
  }
  if (recycle_budget != 0u) {
    if (!h12_retired_reclaim_recycle_bounded(
            owner, recycle_budget, &recycle)) {
      h12_retired_reclaim_recycle_dump(stdout, &recycle);
      return 11;
    }
  }
  printf("[HZ12_WIDEWS_RECLAIM_SHADOW] threads=%u iters=%u ws=%u "
         "allocs=%llu frees=%llu ops/s=%.2f peak_rss_mib=%.2f "
         "owner_spans=%u candidates=%u candidate_mib=%.2f "
         "reclaimable_mib=%.2f detach_budget=%u detached_mib=%.2f "
         "post_reclaimable_mib=%.2f decommit_budget=%u decommitted_mib=%.2f "
         "rss_before_decommit_mib=%.2f rss_after_decommit_mib=%.2f "
         "depot_budget=%u recommitted_mib=%.2f depot_remaining=%u "
         "recycle_budget=%u recycled_mib=%.2f recycle_depot=%u "
         "foreign_scope_blocked=%u\n",
         thread_count, iters, working_set, (unsigned long long)allocs,
         (unsigned long long)frees,
         elapsed ? (double)(allocs + frees) * 1000.0 / (double)elapsed : 0.0,
         (double)memory_after_decommit.PeakWorkingSetSize / (1024.0 * 1024.0),
         result.owner_spans, result.accounting_candidates,
         (double)result.accounting_candidate_bytes / (1024.0 * 1024.0),
         (double)result.reclaimable_bytes / (1024.0 * 1024.0),
         detach_budget, (double)detach.detached_bytes / (1024.0 * 1024.0),
         (double)after_detach.reclaimable_bytes / (1024.0 * 1024.0),
         decommit_budget,
         (double)decommit.decommitted_bytes / (1024.0 * 1024.0),
         (double)memory_before_decommit.WorkingSetSize / (1024.0 * 1024.0),
         (double)memory_after_decommit.WorkingSetSize / (1024.0 * 1024.0),
         depot_budget,
         (double)depot_cycle.recommitted_bytes / (1024.0 * 1024.0),
         depot_cycle.depot_remaining,
         recycle_budget,
         (double)recycle.bytes_redecommitted / (1024.0 * 1024.0),
         recycle.depot_final,
         result.blocked_foreign_cache_scope);
  h12_retired_reclaim_shadow_dump(stdout, &result);
  if (detach_budget != 0u) {
    h12_retired_reclaim_detach_dump(stdout, &detach);
    h12_retired_reclaim_shadow_dump(stdout, &after_detach);
  }
  if (decommit_budget != 0u) {
    h12_retired_reclaim_decommit_dump(stdout, &decommit);
  }
  if (depot_budget != 0u) {
    h12_retired_reclaim_depot_cycle_dump(stdout, &depot_cycle);
  }
  if (recycle_budget != 0u) {
    h12_retired_reclaim_recycle_dump(stdout, &recycle);
  }
  free(handles);
  free(states);
  return (allocs == frees && result.reclaimable_bytes == 0u &&
          (detach_budget == 0u ||
           after_detach.reclaimable_bytes == detach.detached_bytes) &&
          (decommit_budget == 0u ||
           decommit.decommitted_bytes <= detach.detached_bytes) &&
          (depot_budget == 0u ||
           depot_cycle.recommitted_bytes ==
               (uint64_t)depot_budget * HZ12_SPAN_BYTES) &&
          (recycle_budget == 0u ||
           recycle.bytes_redecommitted ==
               (uint64_t)recycle_budget * HZ12_SPAN_BYTES)) ? 0 : 9;
}
