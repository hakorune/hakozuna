#include "hz6_preload_real.h"

#include <dlfcn.h>
#include <malloc.h>
#include <pthread.h>

extern void* __libc_malloc(size_t size);
extern void __libc_free(void* ptr);
extern void* __libc_calloc(size_t nmemb, size_t size);
extern void* __libc_realloc(void* ptr, size_t size);
extern int __libc_posix_memalign(void** memptr, size_t alignment, size_t size);

typedef void* (*Hz6RealMallocFn)(size_t);
typedef void (*Hz6RealFreeFn)(void*);
typedef void* (*Hz6RealCallocFn)(size_t, size_t);
typedef void* (*Hz6RealReallocFn)(void*, size_t);
typedef int (*Hz6RealPosixMemalignFn)(void**, size_t, size_t);
typedef void* (*Hz6RealAlignedAllocFn)(size_t, size_t);
typedef size_t (*Hz6RealUsableSizeFn)(void*);
typedef int (*Hz6RealMallocTrimFn)(size_t);

static Hz6RealMallocFn g_real_malloc;
static Hz6RealFreeFn g_real_free;
static Hz6RealCallocFn g_real_calloc;
static Hz6RealReallocFn g_real_realloc;
static Hz6RealPosixMemalignFn g_real_posix_memalign;
static Hz6RealAlignedAllocFn g_real_aligned_alloc;
static Hz6RealUsableSizeFn g_real_malloc_usable_size;
static Hz6RealMallocTrimFn g_real_malloc_trim;
static pthread_once_t g_real_once = PTHREAD_ONCE_INIT;
__thread int g_hz6_preload_reentry;

static void hz6_preload_resolve_real(void) {
  int saved_reentry = g_hz6_preload_reentry;
  g_hz6_preload_reentry = 1;
  g_real_malloc = (Hz6RealMallocFn)dlsym(RTLD_NEXT, "malloc");
  g_real_free = (Hz6RealFreeFn)dlsym(RTLD_NEXT, "free");
  g_real_calloc = (Hz6RealCallocFn)dlsym(RTLD_NEXT, "calloc");
  g_real_realloc = (Hz6RealReallocFn)dlsym(RTLD_NEXT, "realloc");
  g_real_posix_memalign =
      (Hz6RealPosixMemalignFn)dlsym(RTLD_NEXT, "posix_memalign");
  g_real_aligned_alloc =
      (Hz6RealAlignedAllocFn)dlsym(RTLD_NEXT, "aligned_alloc");
  g_real_malloc_usable_size =
      (Hz6RealUsableSizeFn)dlsym(RTLD_NEXT, "malloc_usable_size");
  g_real_malloc_trim = (Hz6RealMallocTrimFn)dlsym(RTLD_NEXT, "malloc_trim");
  g_hz6_preload_reentry = saved_reentry;
}

static void hz6_preload_ensure_real(void) {
  (void)pthread_once(&g_real_once, hz6_preload_resolve_real);
}

void* hz6_preload_real_malloc(size_t size) {
  if (g_hz6_preload_reentry && !g_real_malloc) {
    return __libc_malloc(size);
  }
  hz6_preload_ensure_real();
  if (g_real_malloc) {
    return g_real_malloc(size);
  }
  return __libc_malloc(size);
}

void hz6_preload_real_free(void* ptr) {
  if (g_hz6_preload_reentry && !g_real_free) {
    __libc_free(ptr);
    return;
  }
  hz6_preload_ensure_real();
  if (g_real_free) {
    g_real_free(ptr);
    return;
  }
  __libc_free(ptr);
}

void* hz6_preload_real_calloc(size_t nmemb, size_t size) {
  if (g_hz6_preload_reentry && !g_real_calloc) {
    return __libc_calloc(nmemb, size);
  }
  hz6_preload_ensure_real();
  if (g_real_calloc) {
    return g_real_calloc(nmemb, size);
  }
  return __libc_calloc(nmemb, size);
}

void* hz6_preload_real_realloc(void* ptr, size_t size) {
  if (g_hz6_preload_reentry && !g_real_realloc) {
    return __libc_realloc(ptr, size);
  }
  hz6_preload_ensure_real();
  if (g_real_realloc) {
    return g_real_realloc(ptr, size);
  }
  return __libc_realloc(ptr, size);
}

int hz6_preload_real_posix_memalign(void** memptr,
                                    size_t alignment,
                                    size_t size) {
  if (g_hz6_preload_reentry && !g_real_posix_memalign) {
    return __libc_posix_memalign(memptr, alignment, size);
  }
  hz6_preload_ensure_real();
  if (g_real_posix_memalign) {
    return g_real_posix_memalign(memptr, alignment, size);
  }
  return __libc_posix_memalign(memptr, alignment, size);
}

void* hz6_preload_real_aligned_alloc(size_t alignment, size_t size) {
  if (g_hz6_preload_reentry && !g_real_aligned_alloc) {
    void* ptr = NULL;
    return __libc_posix_memalign(&ptr, alignment, size) == 0 ? ptr : NULL;
  }
  hz6_preload_ensure_real();
  if (g_real_aligned_alloc) {
    return g_real_aligned_alloc(alignment, size);
  }
  void* ptr = NULL;
  return hz6_preload_real_posix_memalign(&ptr, alignment, size) == 0 ? ptr
                                                                      : NULL;
}

size_t hz6_preload_real_malloc_usable_size(void* ptr) {
  if (g_hz6_preload_reentry && !g_real_malloc_usable_size) {
    return 0;
  }
  hz6_preload_ensure_real();
  return g_real_malloc_usable_size ? g_real_malloc_usable_size(ptr) : 0;
}

int hz6_preload_real_malloc_trim(size_t pad) {
  if (g_hz6_preload_reentry && !g_real_malloc_trim) {
    return 0;
  }
  hz6_preload_ensure_real();
  return g_real_malloc_trim ? g_real_malloc_trim(pad) : 0;
}
