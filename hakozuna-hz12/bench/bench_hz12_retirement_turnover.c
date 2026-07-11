#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <process.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#endif

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
#if defined(_WIN32)
  volatile LONG ready, published_spans, drained_spans, retire;
#else
  _Atomic long ready, published_spans, drained_spans, retire;
#endif
} H12TurnoverCycle;

#if defined(_WIN32)
#define H12_ATOMIC_LOAD(p) InterlockedCompareExchange((p), 0, 0)
#define H12_ATOMIC_SET(p, v) InterlockedExchange((p), (v))
#define H12_ATOMIC_INC(p) InterlockedIncrement((p))
#define H12_YIELD() SwitchToThread()
#define H12_WORKER_RETURN(v) return (unsigned)(v)
#else
#define H12_ATOMIC_LOAD(p) atomic_load_explicit((p), memory_order_acquire)
#define H12_ATOMIC_SET(p, v) atomic_store_explicit((p), (v), memory_order_release)
#define H12_ATOMIC_INC(p) atomic_fetch_add_explicit((p), 1, memory_order_acq_rel)
#define H12_YIELD() sched_yield()
#define H12_WORKER_RETURN(v) return (void*)(uintptr_t)(v)
#endif

static double h12_now_us(void) {
#if defined(_WIN32)
  LARGE_INTEGER frequency, counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return 1000000.0 * (double)counter.QuadPart / (double)frequency.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
#endif
}

static uint64_t h12_rss_bytes(void) {
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS_EX memory = {0};
  memory.cb = sizeof(memory);
  return GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory, sizeof(memory))
      ? (uint64_t)memory.WorkingSetSize : 0u;
#else
  long total_pages = 0;
  long pages = 0;
  FILE* file = fopen("/proc/self/statm", "r");
  if (!file) return 0u;
  if (fscanf(file, "%ld %ld", &total_pages, &pages) != 2) pages = 0;
  fclose(file);
  return pages > 0 ? (uint64_t)pages * (uint64_t)sysconf(_SC_PAGESIZE) : 0u;
#endif
}

static uint64_t h12_peak_rss_bytes(void) {
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS_EX memory = {0};
  memory.cb = sizeof(memory);
  return GetProcessMemoryInfo(GetCurrentProcess(),
      (PROCESS_MEMORY_COUNTERS*)&memory, sizeof(memory))
      ? (uint64_t)memory.PeakWorkingSetSize : 0u;
#else
  char line[256];
  unsigned long value_kib;
  FILE* file = fopen("/proc/self/status", "r");
  if (!file) return 0u;
  while (fgets(line, sizeof(line), file)) {
    if (sscanf(line, "VmHWM: %lu kB", &value_kib) == 1) {
      fclose(file);
      return (uint64_t)value_kib * 1024u;
    }
  }
  fclose(file);
  return 0u;
#endif
}

static int h12_compare_double(const void* left, const void* right) {
  const double a = *(const double*)left;
  const double b = *(const double*)right;
  return (a > b) - (a < b);
}

#if defined(_WIN32)
static unsigned __stdcall h12_turnover_worker(void* arg) {
#else
static void* h12_turnover_worker(void* arg) {
#endif
  H12TurnoverCycle* cycle = (H12TurnoverCycle*)arg;
#define H12_WORKER_FAIL(v) do { H12_ATOMIC_SET(&cycle->ready, -(long)(v)); \
  H12_WORKER_RETURN(v); } while (0)
  if (!h12_owner_epoch_register(&cycle->participant)) H12_WORKER_FAIL(1u);
  for (uint32_t span = 0u; span < H12_TURNOVER_SPANS; ++span) {
    H12SnapshotRecycleResult recycle = {0};
    void* expected_base = NULL;
    const uint32_t begin = span * cycle->slots_per_span;
    if (cycle->use_depot) {
      if (!h12_snapshot_recycle_take(cycle->class_id, &recycle)) H12_WORKER_FAIL(2u);
      expected_base = recycle.span_base;
    }
    for (uint32_t slot = 0u; slot < cycle->slots_per_span; ++slot) {
      void* ptr = hz12_malloc(H12_TURNOVER_BYTES);
      if (!ptr) H12_WORKER_FAIL(3u);
      cycle->objects[begin + slot] = ptr;
      if (slot == 0u && expected_base && ptr != expected_base) H12_WORKER_FAIL(4u);
    }
    if (!h12_span_owner_shadow_assign(cycle->objects[begin], cycle->owner)) {
      H12_WORKER_FAIL(5u);
    }
    hz12_thread_cache_reclaim_checkpoint();
  }
  if (!hz12_tls || !hz12_tls->flush_owner_valid ||
      hz12_tls->flush_owner_id != cycle->owner.slot ||
      hz12_tls->flush_owner_generation != cycle->owner.generation) {
    fprintf(stderr,
            "HZ12 owner mismatch tls=%p valid=%u slot=%u/%u generation=%u/%u\n",
            (void*)hz12_tls, hz12_tls ? hz12_tls->flush_owner_valid : 0u,
            hz12_tls ? hz12_tls->flush_owner_id : UINT32_MAX,
            cycle->owner.slot,
            hz12_tls ? hz12_tls->flush_owner_generation : 0u,
            cycle->owner.generation);
    H12_WORKER_FAIL(6u);
  }
  H12_ATOMIC_SET(&cycle->ready, 1);
  for (uint32_t span = 0u; span < H12_TURNOVER_SPANS; ++span) {
    while (H12_ATOMIC_LOAD(&cycle->published_spans) <= (long)span) {
      H12_YIELD();
    }
    hz12_flush_owner_route_drain(hz12_tls);
    hz12_thread_cache_reclaim_checkpoint();
    H12_ATOMIC_INC(&cycle->drained_spans);
  }
  while (H12_ATOMIC_LOAD(&cycle->retire) == 0) {
    H12_YIELD();
  }
  if (!h12_owner_epoch_checkpoint(cycle->participant) ||
      !h12_owner_epoch_unregister(cycle->participant)) {
    H12_WORKER_RETURN(7u);
  }
#if !defined(_WIN32)
  /* Windows FLS performs this teardown after the worker returns. */
  hz12_flush_owner_route_drain(hz12_tls);
  hz12_flush_owner_route_detach(hz12_tls);
#endif
  H12_WORKER_RETURN(0u);
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
  uint64_t reclaimed_spans = 0u;
  uint64_t decommitted_bytes = 0u;
  void** objects = (void**)calloc(total_objects, sizeof(void*));
  if (!objects || !h12_shadow_init(HZ12_SHADOW_MAX_OWNERS)) return 1;
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
    double begin;
    double end;
#if defined(_WIN32)
    HANDLE worker;
    DWORD exit_code = 0u;
#else
    pthread_t worker;
    void* worker_result = NULL;
#endif
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
#if defined(_WIN32)
    worker = (HANDLE)_beginthreadex(NULL, 0, h12_turnover_worker,
                                    &cycle, 0, NULL);
    if (!worker) return 4;
#else
    if (pthread_create(&worker, NULL, h12_turnover_worker, &cycle) != 0) return 4;
#endif
    while (H12_ATOMIC_LOAD(&cycle.ready) == 0) {
      H12_YIELD();
    }
    if (H12_ATOMIC_LOAD(&cycle.ready) < 0) {
      fprintf(stderr, "HZ12 turnover worker failed before ready: %ld\n",
              -H12_ATOMIC_LOAD(&cycle.ready));
#if defined(_WIN32)
      WaitForSingleObject(worker, INFINITE);
      CloseHandle(worker);
#else
      (void)pthread_join(worker, &worker_result);
#endif
      return 4;
    }
    for (uint32_t span = 0u; span < H12_TURNOVER_SPANS; ++span) {
      const uint32_t begin_index = span * slots_per_span;
      for (uint32_t slot = 0u; slot < slots_per_span; ++slot) {
        hz12_free(objects[begin_index + slot]);
      }
      hz12_thread_cache_reclaim_checkpoint();
      H12_ATOMIC_INC(&cycle.published_spans);
      while (H12_ATOMIC_LOAD(&cycle.drained_spans) <= (long)span) {
        H12_YIELD();
      }
    }
    begin = h12_now_us();
    if (!h12_owner_begin_retire(cycle.owner) ||
        !h12_owner_epoch_begin_retire(cycle.owner)) {
      return 5;
    }
    H12_ATOMIC_SET(&cycle.retire, 1);
#if defined(_WIN32)
    WaitForSingleObject(worker, INFINITE);
    GetExitCodeThread(worker, &exit_code);
    CloseHandle(worker);
#else
    if (pthread_join(worker, &worker_result) != 0) return 6;
#endif
    if (
#if defined(_WIN32)
        exit_code != 0u ||
#else
        worker_result != NULL ||
#endif
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
    end = h12_now_us();
    retire_us[generation] = end - begin;
    reclaimed_spans += reclaim.depot_inserted;
    decommitted_bytes += reclaim.decommitted_bytes;
    {
      const uint64_t rss = h12_rss_bytes();
      if (rss > rss_post_max) rss_post_max = rss;
      if (rss > rss_peak) rss_peak = rss;
    }
    {
      const uint64_t peak = h12_peak_rss_bytes();
      if (peak > rss_peak) rss_peak = peak;
    }
  }
  qsort(retire_us, H12_TURNOVER_CYCLES, sizeof(double), h12_compare_double);
  printf("[HZ12_RETIREMENT_TURNOVER] cycles=%u spans=%u objects_per_cycle=%u "
         "generation_first=%u generation_last=%u generation_reuse=ok "
         "reclaimed_spans=%llu decommitted_bytes=%llu "
         "depot_final=%u depot_current=%u depot_max=%u limbo=0 "
         "retire_p50_us=%.3f retire_p99_us=%.3f retire_max_us=%.3f "
         "post_rss_max_mib=%.2f peak_rss_mib=%.2f\n",
         H12_TURNOVER_CYCLES, H12_TURNOVER_SPANS, total_objects,
         first_generation, last_generation,
         (unsigned long long)reclaimed_spans,
         (unsigned long long)decommitted_bytes,
         h12_span_depot_core_count(), h12_span_depot_core_count(),
         h12_span_depot_core_max(),
         retire_us[H12_TURNOVER_CYCLES / 2u],
         retire_us[(H12_TURNOVER_CYCLES - 1u) * 99u / 100u],
         retire_us[H12_TURNOVER_CYCLES - 1u],
         (double)rss_post_max / (1024.0 * 1024.0),
         (double)rss_peak / (1024.0 * 1024.0));
  free(objects);
  return 0;
}
