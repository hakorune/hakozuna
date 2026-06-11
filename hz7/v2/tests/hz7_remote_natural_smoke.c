#include "../hz7.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef DWORD H7ThreadReturn;
#define H7_THREAD_CALL WINAPI
#define H7_THREAD_OK 0
#define H7_THREAD_FAIL 1
#else
#include <pthread.h>
typedef void* H7ThreadReturn;
#define H7_THREAD_CALL
#define H7_THREAD_OK 0
#define H7_THREAD_FAIL 0
#endif

enum {
  H7_REMOTE_NATURAL_THREADS = 4,
  H7_REMOTE_NATURAL_COUNT = 512
};

typedef struct H7RemoteNaturalState {
  void* ptrs[H7_REMOTE_NATURAL_COUNT];
  size_t sizes[H7_REMOTE_NATURAL_COUNT];
  unsigned worker_index;
  int failed;
} H7RemoteNaturalState;

static int h7_expect(int cond, const char* label) {
  if (!cond) {
    fprintf(stderr, "hz7 remote-natural smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

static size_t h7_remote_natural_size(size_t i) {
  static const size_t sizes[] = {
      16u,    64u,    256u,   1024u,  4096u,  8192u,
      16384u, 32768u, 65536u, 128u,   2048u,  49152u};
  return sizes[i % (sizeof(sizes) / sizeof(sizes[0]))];
}

static H7ThreadReturn H7_THREAD_CALL h7_remote_natural_worker(void* user) {
  H7RemoteNaturalState* state = (H7RemoteNaturalState*)user;
  size_t i;
  for (i = state->worker_index; i < H7_REMOTE_NATURAL_COUNT;
       i += H7_REMOTE_NATURAL_THREADS) {
    if (!h7_expect(h7_route(state->ptrs[i]) == H7_ROUTE_VALID,
                   "route valid before cross-thread free")) {
      state->failed = 1;
      return H7_THREAD_FAIL;
    }
    h7_free(state->ptrs[i]);
    if (!h7_expect(h7_route(state->ptrs[i]) != H7_ROUTE_VALID,
                   "route not valid after cross-thread free")) {
      state->failed = 1;
      return H7_THREAD_FAIL;
    }
  }
  return H7_THREAD_OK;
}

int main(void) {
  H7RemoteNaturalState states[H7_REMOTE_NATURAL_THREADS];
  H7Stats stats;
  size_t i;
  unsigned t;
#ifdef _WIN32
  HANDLE threads[H7_REMOTE_NATURAL_THREADS];
  DWORD wait_rc;
#else
  pthread_t threads[H7_REMOTE_NATURAL_THREADS];
#endif

  stats = h7_stats();
  if (!h7_expect(stats.remote_natural_preset == 1u,
                 "remote natural preset visible") ||
      !h7_expect(stats.route_capacity >= 16384u,
                 "remote natural route capacity")) {
    return 1;
  }

  memset(states, 0, sizeof(states));
  for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
    states[t].worker_index = t;
  }

  for (i = 0; i < H7_REMOTE_NATURAL_COUNT; ++i) {
    size_t size = h7_remote_natural_size(i);
    void* ptr = h7_malloc(size);
    if (!h7_expect(ptr != 0, "remote natural alloc")) {
      return 1;
    }
    memset(ptr, (int)(0x40u + (i & 0x1Fu)), size);
    for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
      states[t].ptrs[i] = ptr;
      states[t].sizes[i] = size;
    }
  }

#ifdef _WIN32
  for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
    threads[t] = CreateThread(0, 0, h7_remote_natural_worker, &states[t], 0, 0);
    if (!threads[t]) {
      fprintf(stderr,
              "hz7 remote-natural smoke failed: CreateThread %u\n",
              t);
      return 1;
    }
  }
  wait_rc = WaitForMultipleObjects(
      H7_REMOTE_NATURAL_THREADS, threads, TRUE, INFINITE);
  if (wait_rc < WAIT_OBJECT_0 ||
      wait_rc >= WAIT_OBJECT_0 + H7_REMOTE_NATURAL_THREADS) {
    fprintf(stderr,
            "hz7 remote-natural smoke failed: wait rc=%lu\n",
            (unsigned long)wait_rc);
    return 1;
  }
  for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
    CloseHandle(threads[t]);
  }
#else
  for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
    if (pthread_create(&threads[t], 0, h7_remote_natural_worker, &states[t]) !=
        0) {
      fprintf(stderr,
              "hz7 remote-natural smoke failed: pthread_create %u\n",
              t);
      return 1;
    }
  }
  for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
    if (pthread_join(threads[t], 0) != 0) {
      fprintf(stderr,
              "hz7 remote-natural smoke failed: pthread_join %u\n",
              t);
      return 1;
    }
  }
#endif

  for (t = 0; t < H7_REMOTE_NATURAL_THREADS; ++t) {
    if (!h7_expect(!states[t].failed, "worker failure")) {
      return 1;
    }
  }

  stats = h7_stats();
  if (!h7_expect(stats.active_bytes == 0, "active bytes after remote natural") ||
      !h7_expect(stats.direct_count == 0, "direct count after remote natural") ||
      !h7_expect(stats.route_register_fail == 0,
                 "route register fail after remote natural")) {
    return 1;
  }

  printf("hz7-remote-natural-smoke ok\n");
  return 0;
}
