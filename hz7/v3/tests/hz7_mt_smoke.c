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

typedef struct H7MtWorker {
  unsigned thread_index;
  unsigned iterations;
  int failed;
} H7MtWorker;

static H7ThreadReturn H7_THREAD_CALL h7_mt_worker(void* user) {
  H7MtWorker* worker = (H7MtWorker*)user;
  unsigned i;
  for (i = 0; i < worker->iterations; ++i) {
    size_t size = 16u << ((i + worker->thread_index) % 9u);
    unsigned char* ptr = (unsigned char*)h7_malloc(size);
    if (!ptr || h7_route(ptr) != H7_ROUTE_VALID) {
      worker->failed = 1;
      return H7_THREAD_FAIL;
    }
    memset(ptr, (int)(0x31u + worker->thread_index), size);
    h7_free(ptr);
  }
  return H7_THREAD_OK;
}

int main(void) {
  enum { H7_MT_THREADS = 4, H7_MT_ITERS = 50000 };
  H7MtWorker workers[H7_MT_THREADS];
#ifdef _WIN32
  HANDLE threads[H7_MT_THREADS];
  DWORD wait_rc;
#else
  pthread_t threads[H7_MT_THREADS];
#endif
  H7Stats stats;
  unsigned i;

  for (i = 0; i < H7_MT_THREADS; ++i) {
    workers[i].thread_index = i;
    workers[i].iterations = H7_MT_ITERS;
    workers[i].failed = 0;
#ifdef _WIN32
    threads[i] = CreateThread(0, 0, h7_mt_worker, &workers[i], 0, 0);
    if (!threads[i]) {
      fprintf(stderr, "hz7 mt smoke failed: CreateThread %u\n", i);
      return 1;
    }
#else
    if (pthread_create(&threads[i], 0, h7_mt_worker, &workers[i]) != 0) {
      fprintf(stderr, "hz7 mt smoke failed: pthread_create %u\n", i);
      return 1;
    }
#endif
  }

#ifdef _WIN32
  wait_rc = WaitForMultipleObjects(H7_MT_THREADS, threads, TRUE, INFINITE);
  if (wait_rc < WAIT_OBJECT_0 || wait_rc >= WAIT_OBJECT_0 + H7_MT_THREADS) {
    fprintf(stderr, "hz7 mt smoke failed: wait rc=%lu\n", (unsigned long)wait_rc);
    return 1;
  }
  for (i = 0; i < H7_MT_THREADS; ++i) {
    CloseHandle(threads[i]);
    if (workers[i].failed) {
      fprintf(stderr, "hz7 mt smoke failed: worker %u\n", i);
      return 1;
    }
  }
#else
  for (i = 0; i < H7_MT_THREADS; ++i) {
    if (pthread_join(threads[i], 0) != 0) {
      fprintf(stderr, "hz7 mt smoke failed: pthread_join %u\n", i);
      return 1;
    }
    if (workers[i].failed) {
      fprintf(stderr, "hz7 mt smoke failed: worker %u\n", i);
      return 1;
    }
  }
#endif

  stats = h7_stats();
  if (stats.active_bytes != 0 || stats.route_register_fail != 0) {
    fprintf(stderr,
            "hz7 mt smoke failed: active=%zu route_register_fail=%zu\n",
            stats.active_bytes,
            stats.route_register_fail);
    return 1;
  }

  printf("hz7-mt-smoke ok\n");
  return 0;
}
