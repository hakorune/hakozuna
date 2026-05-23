#include "hz5.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
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

static Hz5RealMallocFn g_real_malloc;
static Hz5RealFreeFn g_real_free;
static Hz5RealPosixMemalignFn g_real_posix_memalign;
static Hz5RealCallocFn g_real_calloc;
static Hz5RealReallocFn g_real_realloc;
static Hz5RealAlignedAllocFn g_real_aligned_alloc;

static void hz5_preload_resolve(void) {
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
  return size == 65536u && align == 8192u;
}

static void* hz5_preload_hz5_alloc(size_t size, size_t align) {
  g_hz5_preload_inside++;
  void* ptr = hz5_aligned_alloc(size, align);
  g_hz5_preload_inside--;
  if (!ptr) {
    return NULL;
  }
  if (!hz5_preload_track_insert(ptr)) {
    g_hz5_preload_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_inside--;
    return NULL;
  }
  return ptr;
}

void* malloc(size_t size) {
  hz5_preload_resolve();
  if (g_hz5_preload_inside || !hz5_preload_claims(size, 8192u)) {
    return g_real_malloc ? g_real_malloc(size) : NULL;
  }
  void* ptr = hz5_preload_hz5_alloc(size, 8192u);
  if (ptr) {
    return ptr;
  }
  return g_real_malloc ? g_real_malloc(size) : NULL;
}

void free(void* ptr) {
  hz5_preload_resolve();
  if (!ptr) {
    return;
  }
  if (!g_hz5_preload_inside && hz5_preload_track_remove(ptr)) {
    g_hz5_preload_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_inside--;
    return;
  }
  if (g_real_free) {
    g_real_free(ptr);
  }
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
  hz5_preload_resolve();
  if (g_hz5_preload_inside || !hz5_preload_claims(size, alignment)) {
    return g_real_posix_memalign
               ? g_real_posix_memalign(memptr, alignment, size)
               : ENOMEM;
  }
  void* ptr = hz5_preload_hz5_alloc(size, alignment);
  if (!ptr) {
    return g_real_posix_memalign
               ? g_real_posix_memalign(memptr, alignment, size)
               : ENOMEM;
  }
  *memptr = ptr;
  return 0;
}

void* aligned_alloc(size_t alignment, size_t size) {
  hz5_preload_resolve();
  if (g_hz5_preload_inside || !hz5_preload_claims(size, alignment)) {
    return g_real_aligned_alloc ? g_real_aligned_alloc(alignment, size) : NULL;
  }
  void* ptr = hz5_preload_hz5_alloc(size, alignment);
  if (ptr) {
    return ptr;
  }
  return g_real_aligned_alloc ? g_real_aligned_alloc(alignment, size) : NULL;
}

void* calloc(size_t nmemb, size_t size) {
  hz5_preload_resolve();
  if (nmemb != 0 && size > SIZE_MAX / nmemb) {
    errno = ENOMEM;
    return NULL;
  }
  size_t total = nmemb * size;
  if (!g_hz5_preload_inside && hz5_preload_claims(total, 8192u)) {
    void* ptr = hz5_preload_hz5_alloc(total, 8192u);
    if (ptr) {
      memset(ptr, 0, total);
      return ptr;
    }
  }
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
      memcpy(next, ptr, size < 65536u ? size : 65536u);
    }
    g_hz5_preload_inside++;
    (void)hz5_free(ptr);
    g_hz5_preload_inside--;
    return next;
  }
  return g_real_realloc ? g_real_realloc(ptr, size) : NULL;
}
