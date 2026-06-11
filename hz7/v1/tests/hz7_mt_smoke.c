#include "../hz7.h"

#include <stdio.h>
#include <string.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct H7MtWorker {
  unsigned thread_index;
  unsigned iterations;
  int failed;
} H7MtWorker;

static DWORD WINAPI h7_mt_worker(void* user) {
  H7MtWorker* worker = (H7MtWorker*)user;
  unsigned i;
  for (i = 0; i < worker->iterations; ++i) {
    size_t size = 16u << ((i + worker->thread_index) % 9u);
    unsigned char* ptr = (unsigned char*)h7_malloc(size);
    if (!ptr || h7_route(ptr) != H7_ROUTE_VALID) {
      worker->failed = 1;
      return 1;
    }
    memset(ptr, (int)(0x31u + worker->thread_index), size);
    h7_free(ptr);
  }
  return 0;
}

int main(void) {
  enum { H7_MT_THREADS = 4, H7_MT_ITERS = 50000 };
  H7MtWorker workers[H7_MT_THREADS];
  HANDLE threads[H7_MT_THREADS];
  H7Stats stats;
  DWORD wait_rc;
  unsigned i;

  for (i = 0; i < H7_MT_THREADS; ++i) {
    workers[i].thread_index = i;
    workers[i].iterations = H7_MT_ITERS;
    workers[i].failed = 0;
    threads[i] = CreateThread(0, 0, h7_mt_worker, &workers[i], 0, 0);
    if (!threads[i]) {
      fprintf(stderr, "hz7 mt smoke failed: CreateThread %u\n", i);
      return 1;
    }
  }

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
