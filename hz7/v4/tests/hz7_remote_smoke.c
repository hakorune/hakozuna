#include "../hz7.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#endif

enum { H7_REMOTE_SMOKE_COUNT = 24 };

typedef struct H7RemoteSmoke {
  void* ptrs[H7_REMOTE_SMOKE_COUNT];
  size_t sizes[H7_REMOTE_SMOKE_COUNT];
  int failed;
} H7RemoteSmoke;

static int h7_expect(int cond, const char* label) {
  if (!cond) {
    fprintf(stderr, "hz7 remote smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

#ifdef _WIN32
static DWORD WINAPI h7_remote_free_worker(void* user) {
  H7RemoteSmoke* smoke = (H7RemoteSmoke*)user;
  size_t i;
  for (i = 0; i < H7_REMOTE_SMOKE_COUNT; ++i) {
    if (!h7_expect(h7_route(smoke->ptrs[i]) == H7_ROUTE_VALID,
                   "route valid before remote free")) {
      smoke->failed = 1;
      return 1;
    }
    h7_free(smoke->ptrs[i]);
    if (!h7_expect(h7_route(smoke->ptrs[i]) == H7_ROUTE_INVALID,
                   "route invalid after remote free")) {
      smoke->failed = 1;
      return 1;
    }
  }
  return 0;
}
#else
static void* h7_remote_free_worker(void* user) {
  H7RemoteSmoke* smoke = (H7RemoteSmoke*)user;
  size_t i;
  for (i = 0; i < H7_REMOTE_SMOKE_COUNT; ++i) {
    if (!h7_expect(h7_route(smoke->ptrs[i]) == H7_ROUTE_VALID,
                   "route valid before remote free")) {
      smoke->failed = 1;
      return 0;
    }
    h7_free(smoke->ptrs[i]);
    if (!h7_expect(h7_route(smoke->ptrs[i]) == H7_ROUTE_INVALID,
                   "route invalid after remote free")) {
      smoke->failed = 1;
      return 0;
    }
  }
  return 0;
}
#endif

int main(void) {
  H7RemoteSmoke smoke;
  H7Stats stats;
  size_t i;
#ifdef _WIN32
  HANDLE thread;
  DWORD wait_rc;
#else
  pthread_t thread;
#endif
  size_t sizes[H7_REMOTE_SMOKE_COUNT] = {
      64u, 128u, 256u, 512u, 1024u, 2048u, 4096u, 8192u,
      16384u, 32768u, 65536u, 96u, 192u, 384u, 768u, 1536u,
      3072u, 6144u, 12288u, 24576u, 49152u, 65536u, 4096u, 64u};

  memset(&smoke, 0, sizeof(smoke));
  for (i = 0; i < H7_REMOTE_SMOKE_COUNT; ++i) {
    smoke.sizes[i] = sizes[i];
    smoke.ptrs[i] = h7_malloc(sizes[i]);
    if (!h7_expect(smoke.ptrs[i] != 0, "remote alloc")) {
      return 1;
    }
    memset(smoke.ptrs[i], (int)(0x20u + (i & 0x1Fu)), sizes[i]);
  }

#ifdef _WIN32
  thread = CreateThread(0, 0, h7_remote_free_worker, &smoke, 0, 0);
  if (!thread) {
    fprintf(stderr, "hz7 remote smoke failed: CreateThread\n");
    return 1;
  }
  wait_rc = WaitForSingleObject(thread, INFINITE);
  if (wait_rc != WAIT_OBJECT_0) {
    fprintf(stderr, "hz7 remote smoke failed: wait rc=%lu\n",
            (unsigned long)wait_rc);
    CloseHandle(thread);
    return 1;
  }
  CloseHandle(thread);
#else
  if (pthread_create(&thread, 0, h7_remote_free_worker, &smoke) != 0) {
    fprintf(stderr, "hz7 remote smoke failed: pthread_create\n");
    return 1;
  }
  if (pthread_join(thread, 0) != 0) {
    fprintf(stderr, "hz7 remote smoke failed: pthread_join\n");
    return 1;
  }
#endif

  if (!h7_expect(!smoke.failed, "worker failure")) {
    return 1;
  }

  for (i = 0; i < H7_REMOTE_SMOKE_COUNT; ++i) {
    if (!h7_expect(h7_route(smoke.ptrs[i]) == H7_ROUTE_INVALID,
                   "route invalid in main after remote free")) {
      return 1;
    }
  }

  stats = h7_stats();
  if (!h7_expect(stats.active_bytes == 0, "active bytes after remote free") ||
      !h7_expect(stats.direct_count == 0, "direct count after remote free") ||
      !h7_expect(stats.route_register_fail == 0,
                 "route_register_fail after remote free")) {
    return 1;
  }

  printf("hz7-remote-smoke ok\n");
  return 0;
}
