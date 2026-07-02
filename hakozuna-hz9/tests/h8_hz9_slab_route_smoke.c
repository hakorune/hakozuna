#include "../src/h8_internal.h"
#include "../src/h8_hz9_slab_route_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY)
#error "h8_hz9_slab_route_smoke requires H9_MEDIUM_LOCAL_SLAB_ROUTE_BOUNDARY"
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
static DWORD WINAPI h8_smoke_thread_trampoline(void* arg) {
  H8SmokeThreadStart* start = (H8SmokeThreadStart*)arg;
  start->result = start->fn(start->arg);
  return 0;
}
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  H8SmokeThreadStart* start = h8_sys_calloc(1, sizeof(*start));
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
  int ok = WaitForSingleObject(thread.handle, INFINITE) == WAIT_OBJECT_0;
  if (thread.handle) {
    CloseHandle(thread.handle);
  }
  h8_sys_free(thread.start);
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
  return pthread_join(thread, NULL);
}
#endif

static h8_smoke_thread_ret_t remote_free_worker(void* arg) {
  h8_free(arg);
  return H8_SMOKE_THREAD_RETVAL(0);
}

static int expect_route(void* ptr, H8RouteKind expected, const char* label) {
  H8RouteKind route = h8_route(ptr);
  if (route != expected) {
    fprintf(stderr, "%s: route=%d expected=%d\n", label, (int)route,
            (int)expected);
    return 1;
  }
  return 0;
}

static uint8_t* reserve_aligned_page(size_t* raw_bytes_out, void** raw_out) {
  size_t bytes = H9_SLAB_PAGE_BYTES;
  size_t reserve = bytes + H9_SLAB_PAGE_BYTES;
  void* raw = h8_platform_reserve(reserve);
  if (!raw) {
    return NULL;
  }
  uintptr_t aligned =
      ((uintptr_t)raw + H9_SLAB_PAGE_BYTES - 1u) &
      ~((uintptr_t)H9_SLAB_PAGE_BYTES - 1u);
  if (h8_platform_commit((void*)aligned, bytes) != 0) {
    h8_platform_release(raw, reserve);
    return NULL;
  }
  *raw_bytes_out = reserve;
  *raw_out = raw;
  return (uint8_t*)aligned;
}

int main(void) {
  h8_init();
  if (!h8_thread_ctx_get()) {
    fprintf(stderr, "ctx init failed\n");
    return 1;
  }

  const size_t bytes = H9_SLAB_PAGE_BYTES;
  const uint32_t slot_size = 8192u;
  const uint16_t slot_count = 4u;
  size_t raw_bytes = 0;
  void* raw = NULL;
  uint8_t* base = reserve_aligned_page(&raw_bytes, &raw);
  if (!base) {
    fprintf(stderr, "reserve failed\n");
    return 2;
  }

  H9MediumSlabRoutePage* page = h9_slab_route_test_register(
      base, bytes, 0u, slot_size, slot_count, h8_owner_current());
  if (!page) {
    fprintf(stderr, "register failed\n");
    h8_platform_release(base, bytes);
    return 3;
  }

  void* slot0 = base;
  void* slot1 = base + slot_size;
  void* interior = base + 1u;
  void* tail = base + ((size_t)slot_size * slot_count);
  void* outside = base + bytes;

  if (expect_route(slot0, H8_ROUTE_VALID, "slot0") ||
      expect_route(slot1, H8_ROUTE_VALID, "slot1") ||
      expect_route(interior, H8_ROUTE_INVALID, "interior") ||
      expect_route(tail, H8_ROUTE_INVALID, "tail") ||
      expect_route(outside, H8_ROUTE_MISS, "outside")) {
    return 4;
  }

  memset(slot0, 0xA5, 1024);
  void* grown = h8_realloc(slot0, 1024);
  if (!grown) {
    fprintf(stderr, "realloc failed\n");
    return 5;
  }
  if (expect_route(slot0, H8_ROUTE_INVALID, "slot0 after realloc")) {
    return 6;
  }
  h8_free(grown);

  h8_smoke_thread_t thread;
  if (h8_smoke_thread_create(&thread, remote_free_worker, slot1) != 0 ||
      h8_smoke_thread_join(thread) != 0) {
    fprintf(stderr, "remote free thread failed\n");
    return 7;
  }
  if (expect_route(slot1, H8_ROUTE_INVALID, "slot1 after remote free")) {
    return 8;
  }

  H8DebugStats stats = h8_debug_stats();
  if (stats.h9_slab_route_valid < 2 || stats.h9_slab_route_invalid < 3 ||
      stats.h9_slab_free_valid < 2 || stats.h9_slab_free_remote < 1 ||
      stats.h9_slab_usable_valid < 1 || stats.h9_slab_free_invalid != 0) {
    fprintf(stderr,
            "unexpected counters route_valid=%zu route_invalid=%zu "
            "free_valid=%zu free_remote=%zu usable=%zu free_invalid=%zu\n",
            stats.h9_slab_route_valid, stats.h9_slab_route_invalid,
            stats.h9_slab_free_valid, stats.h9_slab_free_remote,
            stats.h9_slab_usable_valid, stats.h9_slab_free_invalid);
    return 9;
  }

  h9_slab_route_test_unregister(page);
  h8_platform_release(raw, raw_bytes);
  printf("hz9_slab_route_smoke ok\n");
  return 0;
}
