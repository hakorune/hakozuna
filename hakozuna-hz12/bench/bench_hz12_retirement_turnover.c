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
#include "hz12_owner_epoch.h"
#include "hz12_owner_registry.h"
#include "hz12_owner_retire_gate.h"
#include "hz12_reclaim_entry.h"
#include "hz12_reclaim_policy_shadow.h"
#include "hz12_shadow.h"
#include "hz12_snapshot_reclaim.h"
#include "hz12_snapshot_recycle.h"
#include "hz12_span.h"
#include "hz12_span_depot_core.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_thread_cache.h"
#include "hz12_token_inbox.h"

#define H12_TURNOVER_CYCLES 8u
#define H12_TURNOVER_SPANS 64u
#define H12_TURNOVER_BYTES 64u

typedef struct H12TurnoverCycle {
  void** objects;
  H12OwnerToken owner;
  H12OwnerEpochParticipant participant;
  uint32_t slots_per_span;
  uint32_t total_objects;
  uint8_t class_id;
  uint8_t use_depot;
  volatile LONG ready;
  volatile LONG published_spans;
  volatile LONG drained_spans;
  volatile LONG retire;
} H12TurnoverCycle;

static int h12_compare_double(const void* left, const void* right) {
  const double a = *(const double*)left;
  const double b = *(const double*)right;
  return (a > b) - (a < b);
}

static unsigned __stdcall h12_turnover_worker(void* arg) {
  H12TurnoverCycle* cycle = (H12TurnoverCycle*)arg;
  if (!h12_owner_epoch_register(&cycle->participant)) return 1u;
  for (uint32_t span = 0u; span < H12_TURNOVER_SPANS; ++span) {
    H12SnapshotRecycleResult recycle = {0};
    void* expected_base = NULL;
    const uint32_t begin = span * cycle->slots_per_span;
    if (cycle->use_depot) {
      if (!h12_snapshot_recycle_take(cycle->class_id, &recycle)) return 2u;
      expected_base = recycle.span_base;
    }
    for (uint32_t slot = 0u; slot < cycle->slots_per_span; ++slot) {
      void* ptr = hz12_malloc(H12_TURNOVER_BYTES);
      if (!ptr) return 3u;
      cycle->objects[begin + slot] = ptr;
      if (slot == 0u && expected_base && ptr != expected_base) return 4u;
    }
    if (!h12_span_owner_shadow_assign(cycle->objects[begin], cycle->owner)) {
      return 5u;
    }
    hz12_thread_cache_reclaim_checkpoint();
  }
  if (!hz12_tls || !hz12_tls->flush_owner_valid ||
      hz12_tls->flush_owner_id != cycle->owner.slot ||
      hz12_tls->flush_owner_generation != cycle->owner.generation) {
    return 6u;
  }
  InterlockedExchange(&cycle->ready, 1);
  for (uint32_t span = 0u; span < H12_TURNOVER_SPANS; ++span) {
    while (InterlockedCompareExchange(&cycle->published_spans, 0, 0) <=
           (LONG)span) {
      SwitchToThread();
    }
    hz12_flush_owner_route_drain(hz12_tls);
    hz12_thread_cache_reclaim_checkpoint();
    InterlockedIncrement(&cycle->drained_spans);
  }
  while (InterlockedCompareExchange(&cycle->retire, 0, 0) == 0) {
    SwitchToThread();
  }
  if (!h12_owner_epoch_checkpoint(cycle->participant) ||
      !h12_owner_epoch_unregister(cycle->participant)) {
    return 7u;
  }
  return 0u;
}

int main(void) {
  const uint8_t class_id = hz12_size_class(H12_TURNOVER_BYTES);
  const uint32_t slots_per_span = (uint32_t)(
      HZ12_SPAN_BYTES / hz12_class_slot_size(class_id));
  const uint32_t total_objects = H12_TURNOVER_SPANS * slots_per_span;
  double retire_us[H12_TURNOVER_CYCLES];
  uint32_t first_generation = 0u;
  uint32_t last_generation = 0u;
  uint64_t rss_post_max = 0u;
  uint64_t rss_peak = 0u;
  void** objects = (void**)calloc(total_objects, sizeof(void*));
  LARGE_INTEGER frequency;
  if (!objects || !h12_shadow_init(HZ12_SHADOW_MAX_OWNERS)) return 1;
  QueryPerformanceFrequency(&frequency);
  h12_span_owner_shadow_reset();
  h12_owner_registry_reset();
  h12_owner_epoch_reset();
  h12_token_inbox_reset();
  h12_owner_retire_gate_reset();
  h12_span_depot_core_reset();
  h12_reclaim_entry_reset();

  for (uint32_t generation = 0u; generation < H12_TURNOVER_CYCLES;
       ++generation) {
    H12TurnoverCycle cycle = {0};
    H12ReclaimPolicyShadow policy = {0};
    H12SnapshotReclaimResult reclaim = {0};
    PROCESS_MEMORY_COUNTERS_EX memory = {0};
    LARGE_INTEGER begin;
    LARGE_INTEGER end;
    HANDLE worker;
    DWORD exit_code = 0u;
    cycle.objects = objects;
    cycle.class_id = class_id;
    cycle.slots_per_span = slots_per_span;
    cycle.total_objects = total_objects;
    cycle.use_depot = (uint8_t)(generation != 0u);
    if (!h12_owner_register(&cycle.owner)) return 2;
    if (generation == 0u) first_generation = cycle.owner.generation;
    last_generation = cycle.owner.generation;
    if (cycle.owner.slot != 0u ||
        cycle.owner.generation != first_generation + generation) {
      return 3;
    }
    worker = (HANDLE)_beginthreadex(NULL, 0, h12_turnover_worker,
                                    &cycle, 0, NULL);
    if (!worker) return 4;
    while (InterlockedCompareExchange(&cycle.ready, 0, 0) == 0) {
      SwitchToThread();
    }
    for (uint32_t span = 0u; span < H12_TURNOVER_SPANS; ++span) {
      const uint32_t begin_index = span * slots_per_span;
      for (uint32_t slot = 0u; slot < slots_per_span; ++slot) {
        hz12_free(objects[begin_index + slot]);
      }
      hz12_thread_cache_reclaim_checkpoint();
      InterlockedIncrement(&cycle.published_spans);
      while (InterlockedCompareExchange(&cycle.drained_spans, 0, 0) <=
             (LONG)span) {
        SwitchToThread();
      }
    }
    QueryPerformanceCounter(&begin);
    if (!h12_owner_begin_retire(cycle.owner) ||
        !h12_owner_epoch_begin_retire(cycle.owner)) {
      return 5;
    }
    InterlockedExchange(&cycle.retire, 1);
    WaitForSingleObject(worker, INFINITE);
    GetExitCodeThread(worker, &exit_code);
    CloseHandle(worker);
    if (exit_code != 0u ||
        !h12_reclaim_policy_shadow_query(cycle.owner, &policy) ||
        !policy.would_reclaim ||
        !h12_snapshot_reclaim_retired_bounded(
            cycle.owner, policy.planned_spans, &reclaim) ||
        reclaim.depot_inserted != H12_TURNOVER_SPANS ||
        reclaim.owner_cleared != H12_TURNOVER_SPANS ||
        reclaim.limbo_spans != 0u ||
        h12_span_depot_core_count() != H12_TURNOVER_SPANS ||
        !h12_owner_mark_dead(cycle.owner) ||
        !h12_owner_epoch_finish_retire(cycle.owner)) {
      return 6;
    }
    QueryPerformanceCounter(&end);
    retire_us[generation] = 1000000.0 *
        (double)(end.QuadPart - begin.QuadPart) /
        (double)frequency.QuadPart;
    memory.cb = sizeof(memory);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             (PROCESS_MEMORY_COUNTERS*)&memory,
                             sizeof(memory))) {
      if (memory.WorkingSetSize > rss_post_max) {
        rss_post_max = memory.WorkingSetSize;
      }
      if (memory.PeakWorkingSetSize > rss_peak) {
        rss_peak = memory.PeakWorkingSetSize;
      }
    }
  }
  qsort(retire_us, H12_TURNOVER_CYCLES, sizeof(double), h12_compare_double);
  printf("[HZ12_RETIREMENT_TURNOVER] cycles=%u spans=%u objects_per_cycle=%u "
         "generation_first=%u generation_last=%u depot=%u limbo=0 "
         "retire_p50_us=%.3f retire_p99_us=%.3f retire_max_us=%.3f "
         "post_rss_max_mib=%.2f peak_rss_mib=%.2f\n",
         H12_TURNOVER_CYCLES, H12_TURNOVER_SPANS, total_objects,
         first_generation, last_generation, h12_span_depot_core_count(),
         retire_us[H12_TURNOVER_CYCLES / 2u],
         retire_us[(H12_TURNOVER_CYCLES - 1u) * 99u / 100u],
         retire_us[H12_TURNOVER_CYCLES - 1u],
         (double)rss_post_max / (1024.0 * 1024.0),
         (double)rss_peak / (1024.0 * 1024.0));
  free(objects);
  return 0;
}
