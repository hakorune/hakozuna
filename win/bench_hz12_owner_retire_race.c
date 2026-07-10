#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdatomic.h>

#include "hz12.h"
#include "hz12_flush_owner_route.h"

#define HZ12_RACE_ROUNDS 20u
#define HZ12_RACE_CONSUMERS 4u
#define HZ12_RACE_OBJECTS 8192u

typedef struct H12RaceContext {
  HANDLE ready;
  HANDLE go;
  void* objects[HZ12_RACE_OBJECTS];
  _Atomic uint32_t alloc_failed;
} H12RaceContext;

typedef struct H12ConsumerContext {
  H12RaceContext* race;
  uint32_t begin;
  uint32_t end;
} H12ConsumerContext;

static DWORD WINAPI hz12_race_owner(LPVOID argument) {
  H12RaceContext* race = (H12RaceContext*)argument;
  for (uint32_t i = 0u; i < HZ12_RACE_OBJECTS; ++i) {
    size_t size = 8u + (size_t)((i * 29u) % 1017u);
    race->objects[i] = hz12_malloc(size);
    if (!race->objects[i]) {
      atomic_store_explicit(&race->alloc_failed, 1u, memory_order_relaxed);
      break;
    }
  }
  SetEvent(race->ready);
  WaitForSingleObject(race->go, INFINITE);
  return 0u;
}

static DWORD WINAPI hz12_race_consumer(LPVOID argument) {
  H12ConsumerContext* consumer = (H12ConsumerContext*)argument;
  WaitForSingleObject(consumer->race->go, INFINITE);
  for (uint32_t i = consumer->begin; i < consumer->end; ++i) {
    hz12_free(consumer->race->objects[i]);
  }
  return 0u;
}

static DWORD WINAPI hz12_race_replacement(LPVOID argument) {
  (void)argument;
  for (uint32_t i = 0u; i < 1024u; ++i) {
    void* ptr = hz12_malloc(8u + (i % 1017u));
    if (!ptr) return 1u;
    hz12_free(ptr);
  }
  return 0u;
}

static int hz12_join_thread(HANDLE thread) {
  DWORD exit_code = 1u;
  int ok = WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0 &&
           GetExitCodeThread(thread, &exit_code) && exit_code == 0u;
  CloseHandle(thread);
  return ok;
}

int main(void) {
  H12FlushOwnerRouteStats stats = {0};
  for (uint32_t round = 0u; round < HZ12_RACE_ROUNDS; ++round) {
    H12RaceContext race = {0};
    H12ConsumerContext consumer_contexts[HZ12_RACE_CONSUMERS];
    HANDLE consumers[HZ12_RACE_CONSUMERS];
    HANDLE owner;
    HANDLE replacement;
    race.ready = CreateEvent(NULL, TRUE, FALSE, NULL);
    race.go = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!race.ready || !race.go) return 2;
    owner = CreateThread(NULL, 0u, hz12_race_owner, &race, 0u, NULL);
    if (!owner || WaitForSingleObject(race.ready, INFINITE) != WAIT_OBJECT_0 ||
        atomic_load_explicit(&race.alloc_failed, memory_order_relaxed)) {
      return 3;
    }
    for (uint32_t i = 0u; i < HZ12_RACE_CONSUMERS; ++i) {
      consumer_contexts[i].race = &race;
      consumer_contexts[i].begin = i * (HZ12_RACE_OBJECTS / HZ12_RACE_CONSUMERS);
      consumer_contexts[i].end = (i + 1u) *
                                 (HZ12_RACE_OBJECTS / HZ12_RACE_CONSUMERS);
      consumers[i] = CreateThread(NULL, 0u, hz12_race_consumer,
                                  &consumer_contexts[i], 0u, NULL);
      if (!consumers[i]) return 4;
    }
    SetEvent(race.go);
    if (!hz12_join_thread(owner)) return 5;
    for (uint32_t i = 0u; i < HZ12_RACE_CONSUMERS; ++i) {
      if (!hz12_join_thread(consumers[i])) return 6;
    }
    replacement = CreateThread(NULL, 0u, hz12_race_replacement, NULL, 0u, NULL);
    if (!replacement || !hz12_join_thread(replacement)) return 7;
    CloseHandle(race.go);
    CloseHandle(race.ready);
  }
  hz12_flush_owner_route_stats(&stats);
  printf("[HZ12_OWNER_RETIRE_RACE] rounds=%u attach=%llu reuse=%llu full=%llu "
         "detach=%llu stale_fallback=%llu\n",
         HZ12_RACE_ROUNDS,
         (unsigned long long)stats.attach_success,
         (unsigned long long)stats.attach_reuse,
         (unsigned long long)stats.attach_full,
         (unsigned long long)stats.detach_success,
         (unsigned long long)stats.stale_fallback);
  if (stats.attach_success != HZ12_RACE_ROUNDS * 6u ||
      stats.detach_success != stats.attach_success ||
      stats.attach_reuse == 0u || stats.attach_full != 0u) {
    return 8;
  }
  return 0;
}
