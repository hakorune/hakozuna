#include "../include/h8.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(H9_MEDIUM_LOCAL_SLAB_PAGE_L1)
#error "h8_hz9_slab_page_smoke requires H9_MEDIUM_LOCAL_SLAB_PAGE_L1"
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef void* h8_smoke_thread_ret_t;
typedef struct H8SmokeThreadStart {
  h8_smoke_thread_ret_t (*fn)(void*);
  void* arg;
} H8SmokeThreadStart;
typedef struct h8_smoke_thread_t {
  HANDLE handle;
  H8SmokeThreadStart* start;
} h8_smoke_thread_t;
#define H8_SMOKE_THREAD_RETVAL(code) ((void*)(uintptr_t)(code))
static DWORD WINAPI h8_smoke_thread_trampoline(void* arg) {
  H8SmokeThreadStart* start = (H8SmokeThreadStart*)arg;
  return (DWORD)(uintptr_t)start->fn(start->arg);
}
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  H8SmokeThreadStart* start = calloc(1, sizeof(*start));
  if (!start) {
    return -1;
  }
  start->fn = fn;
  start->arg = arg;
  thread->start = start;
  thread->handle =
      CreateThread(NULL, 0, h8_smoke_thread_trampoline, start, 0, NULL);
  return thread->handle ? 0 : -1;
}
static int h8_smoke_thread_join(h8_smoke_thread_t thread) {
  DWORD code = 1;
  int ok = WaitForSingleObject(thread.handle, INFINITE) == WAIT_OBJECT_0 &&
           GetExitCodeThread(thread.handle, &code) && code == 0;
  if (thread.handle) {
    CloseHandle(thread.handle);
  }
  free(thread.start);
  return ok ? 0 : -1;
}
#else
#include <pthread.h>
typedef pthread_t h8_smoke_thread_t;
typedef void* h8_smoke_thread_ret_t;
#define H8_SMOKE_THREAD_RETVAL(code) ((void*)(uintptr_t)(code))
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  return pthread_create(thread, NULL, fn, arg);
}
static int h8_smoke_thread_join(h8_smoke_thread_t thread) {
  void* out = NULL;
  if (pthread_join(thread, &out) != 0) {
    return -1;
  }
  return (uintptr_t)out == 0u ? 0 : -1;
}
#endif

static h8_smoke_thread_ret_t remote_free_worker(void* arg) {
  h8_free(arg);
  return H8_SMOKE_THREAD_RETVAL(0);
}

static int warm_remote_hot_64k(void) {
#if defined(H9_SLAB_REMOTE_ADAPTIVE_L1)
  void* warm = h8_malloc(65536);
  if (!warm) {
    fprintf(stderr, "adaptive warm allocation failed\n");
    return 1;
  }
  h8_smoke_thread_t thread;
  if (h8_smoke_thread_create(&thread, remote_free_worker, warm) != 0 ||
      h8_smoke_thread_join(thread) != 0) {
    fprintf(stderr, "adaptive warm remote free failed\n");
    return 1;
  }
#endif
  return 0;
}

static h8_smoke_thread_ret_t local_alloc_free_worker(void* arg) {
  (void)arg;
  if (warm_remote_hot_64k() != 0) {
    return H8_SMOKE_THREAD_RETVAL(2);
  }
  void* ptr = h8_malloc(65536);
  if (!ptr) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  h8_free(ptr);
  return H8_SMOKE_THREAD_RETVAL(0);
}

static h8_smoke_thread_ret_t local_burst_worker(void* arg) {
  (void)arg;
  if (warm_remote_hot_64k() != 0) {
    return H8_SMOKE_THREAD_RETVAL(2);
  }
  void* burst[5] = {0};
  for (size_t i = 0; i < 5; ++i) {
    burst[i] = h8_malloc(65536);
    if (!burst[i]) {
      return H8_SMOKE_THREAD_RETVAL(1);
    }
  }
  for (size_t i = 0; i < 5; ++i) {
    h8_free(burst[i]);
  }
  return H8_SMOKE_THREAD_RETVAL(0);
}

static h8_smoke_thread_ret_t live_alloc_worker(void* arg) {
  if (warm_remote_hot_64k() != 0) {
    return H8_SMOKE_THREAD_RETVAL(2);
  }
  void** out = (void**)arg;
  *out = h8_malloc(65536);
  return H8_SMOKE_THREAD_RETVAL(*out ? 0 : 1);
}

int main(void) {
  if (warm_remote_hot_64k() != 0) {
    return 17;
  }

  void* a = h8_malloc(65536);
  void* b = h8_malloc(65536);
  if (!a || !b || a == b) {
    fprintf(stderr, "slab allocations failed\n");
    return 1;
  }
  memset(a, 0xA9, 128);
  memset(b, 0xB9, 128);
  if (h8_route(a) != H8_ROUTE_VALID || h8_route(b) != H8_ROUTE_VALID) {
    fprintf(stderr, "slab allocations are not route-valid\n");
    return 2;
  }

  h8_free(a);
  if (h8_route(a) != H8_ROUTE_INVALID) {
    fprintf(stderr, "freed slab slot stayed valid\n");
    return 3;
  }
  void* c = h8_malloc(65536);
  if (c != a) {
    fprintf(stderr, "slab did not reuse local free slot\n");
    return 4;
  }

  h8_smoke_thread_t thread;
  if (h8_smoke_thread_create(&thread, remote_free_worker, b) != 0 ||
      h8_smoke_thread_join(thread) != 0) {
    fprintf(stderr, "remote free failed\n");
    return 5;
  }
  if (h8_route(b) != H8_ROUTE_INVALID) {
    fprintf(stderr, "remote-freed slab slot stayed valid\n");
    return 6;
  }
  h8_free(c);
  void* d = h8_malloc(65536);
  void* e = h8_malloc(65536);
  if (!d || !e || (d != b && e != b)) {
    fprintf(stderr, "remote-pending slab slot was not collected\n");
    return 7;
  }
  h8_free(d);
  h8_free(e);

  void* burst[5] = {0};
  for (size_t i = 0; i < 5; ++i) {
    burst[i] = h8_malloc(65536);
    if (!burst[i]) {
      fprintf(stderr, "burst slab allocation failed at %zu\n", i);
      return 8;
    }
  }
  for (size_t i = 0; i < 5; ++i) {
    h8_free(burst[i]);
  }

  if (h8_smoke_thread_create(&thread, local_alloc_free_worker, NULL) != 0 ||
      h8_smoke_thread_join(thread) != 0) {
    fprintf(stderr, "local worker alloc/free failed\n");
    return 9;
  }
  H8DebugStats before_burst = h8_debug_stats();
  if (h8_smoke_thread_create(&thread, local_burst_worker, NULL) != 0 ||
      h8_smoke_thread_join(thread) != 0) {
    fprintf(stderr, "local burst worker failed\n");
    return 10;
  }
  void* live = NULL;
  if (h8_smoke_thread_create(&thread, live_alloc_worker, &live) != 0 ||
      h8_smoke_thread_join(thread) != 0 || !live) {
    fprintf(stderr, "live alloc worker failed\n");
    return 11;
  }
  if (h8_route(live) != H8_ROUTE_VALID) {
    fprintf(stderr, "live slab from exited owner is not route-valid\n");
    return 12;
  }
  H8DebugStats before_final_free = h8_debug_stats();
  h8_free(live);
  if (h8_route(live) != H8_ROUTE_INVALID) {
    fprintf(stderr, "final free after owner exit stayed route-valid\n");
    return 13;
  }

  H8DebugStats stats = h8_debug_stats();
  if (stats.h9_slab_page_release <= before_burst.h9_slab_page_release) {
    fprintf(stderr, "burst worker did not purge empty slab pages on exit\n");
    return 14;
  }
  if (stats.h9_slab_page_release <= before_final_free.h9_slab_page_release) {
    fprintf(stderr, "final free after owner exit did not purge empty page\n");
    return 15;
  }
  if (stats.h9_slab_alloc_hit < 3 || stats.h9_slab_alloc_page_create == 0 ||
      stats.h9_slab_free_valid < 3 || stats.h9_slab_free_remote == 0 ||
      stats.h9_slab_page_release == 0) {
    fprintf(stderr,
            "unexpected slab counters alloc_hit=%zu create=%zu free=%zu "
            "remote=%zu release=%zu\n",
            stats.h9_slab_alloc_hit, stats.h9_slab_alloc_page_create,
            stats.h9_slab_free_valid, stats.h9_slab_free_remote,
            stats.h9_slab_page_release);
    return 16;
  }

  printf("hz9_slab_page_smoke ok\n");
  return 0;
}
