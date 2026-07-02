#include "../include/h8.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if !defined(H8_HZ9_MEDIUM_LOCAL_MAG)
#error "h8_hz9_local_mag_smoke requires H8_HZ9_MEDIUM_LOCAL_MAG"
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
  void* result;
} H8SmokeThreadStart;
typedef struct h8_smoke_thread_t {
  HANDLE handle;
  H8SmokeThreadStart* start;
} h8_smoke_thread_t;
#define H8_SMOKE_THREAD_RETVAL(code) ((void*)(uintptr_t)(code))
#define H8_SMOKE_THREAD_RETVAL_PTR(ptr) (ptr)
static DWORD WINAPI h8_smoke_thread_trampoline(void* arg) {
  H8SmokeThreadStart* start = (H8SmokeThreadStart*)arg;
  start->result = start->fn(start->arg);
  return 0;
}
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  H8SmokeThreadStart* start =
      (H8SmokeThreadStart*)calloc(1, sizeof(*start));
  if (!start) {
    return -1;
  }
  start->fn = fn;
  start->arg = arg;
  thread->start = start;
  thread->handle =
      CreateThread(NULL, 0, h8_smoke_thread_trampoline, start, 0, NULL);
  if (!thread->handle) {
    free(start);
    thread->start = NULL;
    return -1;
  }
  return 0;
}
static int h8_smoke_thread_join(h8_smoke_thread_t thread, void** out) {
  if (WaitForSingleObject(thread.handle, INFINITE) != WAIT_OBJECT_0) {
    if (thread.handle) {
      CloseHandle(thread.handle);
    }
    free(thread.start);
    return -1;
  }
  if (out) {
    *out = thread.start ? thread.start->result : NULL;
  }
  if (thread.handle) {
    CloseHandle(thread.handle);
  }
  free(thread.start);
  return 0;
}
#else
#include <pthread.h>
typedef pthread_t h8_smoke_thread_t;
typedef void* h8_smoke_thread_ret_t;
#define H8_SMOKE_THREAD_RETVAL(code) ((void*)(uintptr_t)(code))
#define H8_SMOKE_THREAD_RETVAL_PTR(ptr) (ptr)
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  return pthread_create(thread, NULL, fn, arg);
}
static int h8_smoke_thread_join(h8_smoke_thread_t thread, void** out) {
  return pthread_join(thread, out);
}
#endif

static size_t h8_delta(size_t after, size_t before) {
  return after >= before ? after - before : 0u;
}

static h8_smoke_thread_ret_t remote_free_worker(void* arg) {
  h8_free(arg);
  return H8_SMOKE_THREAD_RETVAL(0);
}

static int exercise_mag_size(size_t size, const char* label) {
  H8DebugStats before = h8_debug_stats();
  void* a = h8_malloc(size);
  void* b = h8_malloc(size);
  if (!a || !b || a == b) {
    fprintf(stderr, "%s: setup allocation failed\n", label);
    return 1;
  }
  memset(a, 0xA9, size);
  memset(b, 0xB9, size);

  h8_free(a);
  if (h8_route(a) == H8_ROUTE_VALID) {
    fprintf(stderr, "%s: LOCAL_MAG pointer stayed valid\n", label);
    return 2;
  }
  H8DebugStats after_push = h8_debug_stats();
  if (h8_delta(after_push.medium_hz9_local_mag_free_push,
               before.medium_hz9_local_mag_free_push) == 0u) {
    fprintf(stderr, "%s: local magazine free push did not fire\n", label);
    return 3;
  }

  h8_smoke_thread_t remote_thread;
  if (h8_smoke_thread_create(&remote_thread, remote_free_worker, a) != 0) {
    fprintf(stderr, "%s: remote free thread create failed\n", label);
    return 4;
  }
  if (h8_smoke_thread_join(remote_thread, NULL) != 0) {
    fprintf(stderr, "%s: remote free thread join failed\n", label);
    return 5;
  }

  h8_free(a);
  H8DebugStats after_dup = h8_debug_stats();
  if (h8_delta(after_dup.medium_hz9_local_mag_invalid_state,
               after_push.medium_hz9_local_mag_invalid_state) == 0u) {
    fprintf(stderr, "%s: duplicate LOCAL_MAG free was not rejected\n", label);
    return 6;
  }

  void* c = h8_malloc(size);
  if (c != a) {
    fprintf(stderr, "%s: local magazine did not return freed slot\n", label);
    return 7;
  }
  H8DebugStats after_pop = h8_debug_stats();
  if (h8_delta(after_pop.medium_hz9_local_mag_alloc_hit,
               after_dup.medium_hz9_local_mag_alloc_hit) == 0u ||
      h8_delta(after_pop.medium_hz9_local_mag_state_mismatch,
               before.medium_hz9_local_mag_state_mismatch) != 0u) {
    fprintf(stderr, "%s: local magazine alloc state failure\n", label);
    return 8;
  }

  h8_free(c);
  h8_free(b);
  return 0;
}

static h8_smoke_thread_ret_t owner_exit_source(void* arg) {
  size_t size = *(size_t*)arg;
  void* a = h8_malloc(size);
  void* b = h8_malloc(size);
  if (!a || !b || a == b) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  memset(a, 0xC9, size);
  memset(b, 0xD9, size);
  h8_free(a);
  return H8_SMOKE_THREAD_RETVAL_PTR(b);
}

static int exercise_owner_exit_flush(size_t size) {
  H8DebugStats before = h8_debug_stats();
  h8_smoke_thread_t thread;
  if (h8_smoke_thread_create(&thread, owner_exit_source, &size) != 0) {
    fprintf(stderr, "owner-exit: thread create failed\n");
    return 20;
  }
  void* live = NULL;
  if (h8_smoke_thread_join(thread, &live) != 0 ||
      !live || live == H8_SMOKE_THREAD_RETVAL(1)) {
    fprintf(stderr, "owner-exit: thread result failed\n");
    return 21;
  }
  H8DebugStats after = h8_debug_stats();
  if (h8_delta(after.medium_hz9_local_mag_flush_slots,
               before.medium_hz9_local_mag_flush_slots) == 0u ||
      h8_route(live) != H8_ROUTE_VALID) {
    fprintf(stderr, "owner-exit: LOCAL_MAG flush or live route failed\n");
    return 22;
  }
  h8_free(live);
  return h8_route(live) == H8_ROUTE_VALID ? 23 : 0;
}

static h8_smoke_thread_ret_t owner_exit_empty_source(void* arg) {
  size_t size = *(size_t*)arg;
  void* a = h8_malloc(size);
  void* b = h8_malloc(size);
  if (!a || !b || a == b) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  memset(a, 0xE9, size);
  memset(b, 0xF9, size);
  h8_free(a);
  h8_free(b);
  return H8_SMOKE_THREAD_RETVAL_PTR(a);
}

static int exercise_owner_exit_empty_flush(size_t size) {
  H8DebugStats before = h8_debug_stats();
  h8_smoke_thread_t thread;
  if (h8_smoke_thread_create(&thread, owner_exit_empty_source, &size) != 0) {
    fprintf(stderr, "owner-exit-empty: thread create failed\n");
    return 30;
  }
  void* freed = NULL;
  if (h8_smoke_thread_join(thread, &freed) != 0 ||
      !freed || freed == H8_SMOKE_THREAD_RETVAL(1)) {
    fprintf(stderr, "owner-exit-empty: thread result failed\n");
    return 31;
  }
  H8DebugStats after = h8_debug_stats();
  if (h8_delta(after.medium_hz9_local_mag_flush_slots,
               before.medium_hz9_local_mag_flush_slots) < 2u ||
      h8_route(freed) == H8_ROUTE_VALID) {
    fprintf(stderr, "owner-exit-empty: drain left LOCAL_MAG reachable\n");
    return 32;
  }
  h8_free(freed);
  return h8_route(freed) == H8_ROUTE_VALID ? 33 : 0;
}

int main(void) {
  h8_init();
  int rc = exercise_mag_size(48u * 1024u, "48K");
  if (rc != 0) {
    return rc;
  }
  rc = exercise_mag_size(64u * 1024u, "64K");
  if (rc != 0) {
    return rc;
  }
  rc = exercise_owner_exit_flush(48u * 1024u);
  if (rc != 0) {
    return rc;
  }
  rc = exercise_owner_exit_empty_flush(48u * 1024u);
  if (rc != 0) {
    return rc;
  }
  H8DebugStats stats = h8_debug_stats();
  printf("hz9_local_mag_smoke push=%zu hit=%zu invalid=%zu flush=%zu "
         "mismatch=%zu\n",
         stats.medium_hz9_local_mag_free_push,
         stats.medium_hz9_local_mag_alloc_hit,
         stats.medium_hz9_local_mag_invalid_state,
         stats.medium_hz9_local_mag_flush_slots,
         stats.medium_hz9_local_mag_state_mismatch);
  return stats.medium_hz9_local_mag_state_mismatch == 0u ? 0 : 7;
}
