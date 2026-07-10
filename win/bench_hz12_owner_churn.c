#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdint.h>
#include <stdio.h>

#include "hz12.h"
#include "hz12_flush_owner_route.h"

#define HZ12_CHURN_THREADS 128u
#define HZ12_CHURN_OBJECTS 512u

static DWORD WINAPI hz12_churn_worker(LPVOID argument) {
  void* objects[HZ12_CHURN_OBJECTS];
  uintptr_t seed = (uintptr_t)argument + 1u;
  for (uint32_t i = 0u; i < HZ12_CHURN_OBJECTS; ++i) {
    size_t size = 8u + (size_t)((seed * 131u + i * 17u) % 1017u);
    objects[i] = hz12_malloc(size);
    if (!objects[i]) return 1u;
  }
  for (uint32_t i = 0u; i < HZ12_CHURN_OBJECTS; ++i) {
    hz12_free(objects[i]);
  }
  return 0u;
}

int main(void) {
  H12FlushOwnerRouteStats stats = {0};
  for (uint32_t i = 0u; i < HZ12_CHURN_THREADS; ++i) {
    DWORD exit_code = 1u;
    HANDLE thread = CreateThread(NULL, 0u, hz12_churn_worker,
                                 (LPVOID)(uintptr_t)i, 0u, NULL);
    if (!thread) return 2;
    if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeThread(thread, &exit_code) || exit_code != 0u) {
      CloseHandle(thread);
      return 3;
    }
    CloseHandle(thread);
  }
  hz12_flush_owner_route_stats(&stats);
  printf("[HZ12_OWNER_CHURN] threads=%u attach=%llu reuse=%llu full=%llu "
         "detach=%llu stale_fallback=%llu\n",
         HZ12_CHURN_THREADS,
         (unsigned long long)stats.attach_success,
         (unsigned long long)stats.attach_reuse,
         (unsigned long long)stats.attach_full,
         (unsigned long long)stats.detach_success,
         (unsigned long long)stats.stale_fallback);
  if (stats.attach_success != HZ12_CHURN_THREADS ||
      stats.detach_success != HZ12_CHURN_THREADS ||
      stats.attach_reuse < HZ12_CHURN_THREADS - 64u ||
      stats.attach_full != 0u) {
    return 4;
  }
  return 0;
}
