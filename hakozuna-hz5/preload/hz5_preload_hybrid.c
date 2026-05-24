#include "hz5.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Diagnostic LD_PRELOAD shim.
 *
 * This is intentionally not a general HZ5 allocator. It routes only the
 * HZ5-claimed exact profile to HZ5 and delegates everything else to libc.
 */

#ifndef HZ5_PRELOAD_TRACK_BITS
#define HZ5_PRELOAD_TRACK_BITS 20u
#endif

#define HZ5_PRELOAD_TRACK_CAP ((size_t)1u << HZ5_PRELOAD_TRACK_BITS)
#define HZ5_PRELOAD_TOMBSTONE ((void*)(uintptr_t)1u)
#define HZ5_PRELOAD_CLAIM_SIZE 65536u
#define HZ5_PRELOAD_CLAIM_ALIGN 8192u

typedef void* (*Hz5RealMallocFn)(size_t);
typedef void (*Hz5RealFreeFn)(void*);
typedef int (*Hz5RealPosixMemalignFn)(void**, size_t, size_t);
typedef void* (*Hz5RealCallocFn)(size_t, size_t);
typedef void* (*Hz5RealReallocFn)(void*, size_t);
typedef void* (*Hz5RealAlignedAllocFn)(size_t, size_t);

typedef struct Hz5PreloadEntry {
  void* ptr;
} Hz5PreloadEntry;

static Hz5PreloadEntry g_hz5_preload_table[HZ5_PRELOAD_TRACK_CAP];
static pthread_mutex_t g_hz5_preload_lock = PTHREAD_MUTEX_INITIALIZER;
static __thread int g_hz5_preload_inside;
static atomic_flag g_hz5_preload_stats_registered = ATOMIC_FLAG_INIT;
static _Atomic uint64_t g_hz5_preload_malloc_hz5;
static _Atomic uint64_t g_hz5_preload_malloc_libc;
static _Atomic uint64_t g_hz5_preload_posix_hz5;
static _Atomic uint64_t g_hz5_preload_posix_libc;
static _Atomic uint64_t g_hz5_preload_aligned_hz5;
static _Atomic uint64_t g_hz5_preload_aligned_libc;
static _Atomic uint64_t g_hz5_preload_calloc_hz5;
static _Atomic uint64_t g_hz5_preload_calloc_libc;
static _Atomic uint64_t g_hz5_preload_realloc_hz5;
static _Atomic uint64_t g_hz5_preload_realloc_libc;
static _Atomic uint64_t g_hz5_preload_free_hz5;
static _Atomic uint64_t g_hz5_preload_free_libc;
static _Atomic uint64_t g_hz5_preload_track_insert_fail;

static Hz5RealMallocFn g_real_malloc;
static Hz5RealFreeFn g_real_free;
static Hz5RealPosixMemalignFn g_real_posix_memalign;
static Hz5RealCallocFn g_real_calloc;
static Hz5RealReallocFn g_real_realloc;
static Hz5RealAlignedAllocFn g_real_aligned_alloc;

static void hz5_preload_stats_print(void) {
  if (!getenv("HZ5_PRELOAD_STATS")) {
    return;
  }
  fprintf(stderr,
          "[HZ5_PRELOAD_STATS]"
          " malloc_hz5=%llu malloc_libc=%llu"
          " posix_hz5=%llu posix_libc=%llu"
          " aligned_hz5=%llu aligned_libc=%llu"
          " calloc_hz5=%llu calloc_libc=%llu"
          " realloc_hz5=%llu realloc_libc=%llu"
          " free_hz5=%llu free_libc=%llu"
          " track_insert_fail=%llu\n",
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_malloc_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_malloc_libc, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_posix_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_posix_libc, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_aligned_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_aligned_libc, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_calloc_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_calloc_libc, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_realloc_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_realloc_libc, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_free_hz5, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_free_libc, memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(
              &g_hz5_preload_track_insert_fail, memory_order_relaxed));
}

static void hz5_preload_stats_register_once(void) {
  if (atomic_flag_test_and_set_explicit(&g_hz5_preload_stats_registered,
                                        memory_order_acq_rel)) {
    return;
  }
  atexit(hz5_preload_stats_print);
}

static void hz5_preload_resolve(void) {
  hz5_preload_stats_register_once();
  if (!g_real_malloc) {
    g_real_malloc = (Hz5RealMallocFn)dlsym(RTLD_NEXT, "malloc");
  }
  if (!g_real_free) {
    g_real_free = (Hz5RealFreeFn)dlsym(RTLD_NEXT, "free");
  }
  if (!g_real_posix_memalign) {
    g_real_posix_memalign =
        (Hz5RealPosixMemalignFn)dlsym(RTLD_NEXT, "posix_memalign");
  }
  if (!g_real_calloc) {
    g_real_calloc = (Hz5RealCallocFn)dlsym(RTLD_NEXT, "calloc");
  }
  if (!g_real_realloc) {
    g_real_realloc = (Hz5RealReallocFn)dlsym(RTLD_NEXT, "realloc");
  }
  if (!g_real_aligned_alloc) {
    g_real_aligned_alloc =
        (Hz5RealAlignedAllocFn)dlsym(RTLD_NEXT, "aligned_alloc");
  }
}

static size_t hz5_preload_hash(void* ptr) {
  uintptr_t x = (uintptr_t)ptr >> 4;
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  return (size_t)x & (HZ5_PRELOAD_TRACK_CAP - 1u);
}

static int hz5_preload_track_insert(void* ptr) {
  if (!ptr || ptr == HZ5_PRELOAD_TOMBSTONE) {
    return 0;
  }
  size_t idx = hz5_preload_hash(ptr);
  pthread_mutex_lock(&g_hz5_preload_lock);
  for (size_t probe = 0; probe < HZ5_PRELOAD_TRACK_CAP; ++probe) {
    Hz5PreloadEntry* entry =
        &g_hz5_preload_table[(idx + probe) & (HZ5_PRELOAD_TRACK_CAP - 1u)];
    if (!entry->ptr || entry->ptr == HZ5_PRELOAD_TOMBSTONE ||
        entry->ptr == ptr) {
      entry->ptr = ptr;
      pthread_mutex_unlock(&g_hz5_preload_lock);
      return 1;
    }
  }
  pthread_mutex_unlock(&g_hz5_preload_lock);
  return 0;
}

static int hz5_preload_track_remove(void* ptr) {
  if (!ptr || ptr == HZ5_PRELOAD_TOMBSTONE) {
    return 0;
  }
  size_t idx = hz5_preload_hash(ptr);
  pthread_mutex_lock(&g_hz5_preload_lock);
  for (size_t probe = 0; probe < HZ5_PRELOAD_TRACK_CAP; ++probe) {
    Hz5PreloadEntry* entry =
        &g_hz5_preload_table[(idx + probe) & (HZ5_PRELOAD_TRACK_CAP - 1u)];
    if (!entry->ptr) {
      pthread_mutex_unlock(&g_hz5_preload_lock);
      return 0;
    }
    if (entry->ptr == ptr) {
      entry->ptr = HZ5_PRELOAD_TOMBSTONE;
      pthread_mutex_unlock(&g_hz5_preload_lock);
      return 1;
    }
  }
  pthread_mutex_unlock(&g_hz5_preload_lock);
  return 0;
}

static int hz5_preload_claims(size_t size, size_t align) {
  return size == HZ5_PRELOAD_CLAIM_SIZE && align == HZ5_PRELOAD_CLAIM_ALIGN;
}

static void* hz5_preload_hz5_alloc(size_t size, size_t align) {
  g_hz5_preload_inside++;
  void* ptr = hz5_aligned_alloc(size, align);
  g_hz5_preload_inside--;
  if (!ptr) {
    return NULL;
  }
  if (!hz5_preload_track_insert(ptr)) {
    atomic_fetch_add_explicit(&g_hz5_preload_track_insert_fail, 1u,
                              memory_order_relaxed);
    g_hz5_preload_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_inside--;
    return NULL;
  }
  return ptr;
}

void* malloc(size_t size) {
  hz5_preload_resolve();
  if (g_hz5_preload_inside ||
      !hz5_preload_claims(size, HZ5_PRELOAD_CLAIM_ALIGN)) {
    atomic_fetch_add_explicit(&g_hz5_preload_malloc_libc, 1u,
                              memory_order_relaxed);
    return g_real_malloc ? g_real_malloc(size) : NULL;
  }
  void* ptr = hz5_preload_hz5_alloc(size, HZ5_PRELOAD_CLAIM_ALIGN);
  if (ptr) {
    atomic_fetch_add_explicit(&g_hz5_preload_malloc_hz5, 1u,
                              memory_order_relaxed);
    return ptr;
  }
  atomic_fetch_add_explicit(&g_hz5_preload_malloc_libc, 1u,
                            memory_order_relaxed);
  return g_real_malloc ? g_real_malloc(size) : NULL;
}

void free(void* ptr) {
  hz5_preload_resolve();
  if (!ptr) {
    return;
  }
  if (!g_hz5_preload_inside && hz5_preload_track_remove(ptr)) {
    atomic_fetch_add_explicit(&g_hz5_preload_free_hz5, 1u,
                              memory_order_relaxed);
    g_hz5_preload_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_inside--;
    return;
  }
  if (g_real_free) {
    atomic_fetch_add_explicit(&g_hz5_preload_free_libc, 1u,
                              memory_order_relaxed);
    g_real_free(ptr);
  }
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  hz5_preload_resolve();
  if (g_hz5_preload_inside || !hz5_preload_claims(size, alignment)) {
    atomic_fetch_add_explicit(&g_hz5_preload_posix_libc, 1u,
                              memory_order_relaxed);
    return g_real_posix_memalign
               ? g_real_posix_memalign(memptr, alignment, size)
               : ENOMEM;
  }
  void* ptr = hz5_preload_hz5_alloc(size, alignment);
  if (!ptr) {
    atomic_fetch_add_explicit(&g_hz5_preload_posix_libc, 1u,
                              memory_order_relaxed);
    return g_real_posix_memalign
               ? g_real_posix_memalign(memptr, alignment, size)
               : ENOMEM;
  }
  atomic_fetch_add_explicit(&g_hz5_preload_posix_hz5, 1u,
                            memory_order_relaxed);
  *memptr = ptr;
  return 0;
}

void* aligned_alloc(size_t alignment, size_t size) {
  hz5_preload_resolve();
  if (g_hz5_preload_inside || !hz5_preload_claims(size, alignment)) {
    atomic_fetch_add_explicit(&g_hz5_preload_aligned_libc, 1u,
                              memory_order_relaxed);
    return g_real_aligned_alloc ? g_real_aligned_alloc(alignment, size) : NULL;
  }
  void* ptr = hz5_preload_hz5_alloc(size, alignment);
  if (ptr) {
    atomic_fetch_add_explicit(&g_hz5_preload_aligned_hz5, 1u,
                              memory_order_relaxed);
    return ptr;
  }
  atomic_fetch_add_explicit(&g_hz5_preload_aligned_libc, 1u,
                            memory_order_relaxed);
  return g_real_aligned_alloc ? g_real_aligned_alloc(alignment, size) : NULL;
}

void* calloc(size_t nmemb, size_t size) {
  hz5_preload_resolve();
  if (nmemb != 0 && size > SIZE_MAX / nmemb) {
    errno = ENOMEM;
    return NULL;
  }
  size_t total = nmemb * size;
  if (!g_hz5_preload_inside &&
      hz5_preload_claims(total, HZ5_PRELOAD_CLAIM_ALIGN)) {
    void* ptr = hz5_preload_hz5_alloc(total, HZ5_PRELOAD_CLAIM_ALIGN);
    if (ptr) {
      memset(ptr, 0, total);
      atomic_fetch_add_explicit(&g_hz5_preload_calloc_hz5, 1u,
                                memory_order_relaxed);
      return ptr;
    }
  }
  atomic_fetch_add_explicit(&g_hz5_preload_calloc_libc, 1u,
                            memory_order_relaxed);
  return g_real_calloc ? g_real_calloc(nmemb, size) : NULL;
}

void* realloc(void* ptr, size_t size) {
  hz5_preload_resolve();
  if (!ptr) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  if (!g_hz5_preload_inside && hz5_preload_track_remove(ptr)) {
    void* next = malloc(size);
    if (next) {
      memcpy(next, ptr,
             size < HZ5_PRELOAD_CLAIM_SIZE ? size : HZ5_PRELOAD_CLAIM_SIZE);
    }
    g_hz5_preload_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_inside--;
    atomic_fetch_add_explicit(&g_hz5_preload_realloc_hz5, 1u,
                              memory_order_relaxed);
    return next;
  }
  atomic_fetch_add_explicit(&g_hz5_preload_realloc_libc, 1u,
                            memory_order_relaxed);
  return g_real_realloc ? g_real_realloc(ptr, size) : NULL;
}
