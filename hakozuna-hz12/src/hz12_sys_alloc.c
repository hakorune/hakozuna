#include "hz12_sys_alloc.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
#include <malloc.h>
#include <stdlib.h>
#else
#include <dlfcn.h>
#endif

HZ12_THREAD_LOCAL int hz12_resolving = 0;

#if defined(_WIN32)
void hz12_resolver_ensure(void) {}

void* hz12_sys_malloc(size_t n) {
  return malloc(n);
}

void hz12_sys_free(void* p) {
  free(p);
}

void* hz12_sys_realloc(void* p, size_t n) {
  return realloc(p, n);
}

void* hz12_sys_calloc(size_t count, size_t size) {
  return calloc(count, size);
}

size_t hz12_sys_usable_size(void* p) {
  return p ? _msize(p) : 0u;
}

int hz12_sys_posix_memalign(void** memptr, size_t alignment, size_t size) {
  (void)alignment;
  (void)size;
  if (memptr) {
    *memptr = NULL;
  }
  return ENOMEM;
}

void* hz12_sys_aligned_alloc(size_t alignment, size_t size) {
  (void)alignment;
  (void)size;
  return NULL;
}

void* hz12_sys_memalign(size_t alignment, size_t size) {
  (void)alignment;
  (void)size;
  return NULL;
}
#else
static void* (*hz12_real_malloc)(size_t) = NULL;
static void (*hz12_real_free)(void*) = NULL;
static void* (*hz12_real_realloc)(void*, size_t) = NULL;
static void* (*hz12_real_calloc)(size_t, size_t) = NULL;
static size_t (*hz12_real_usable_size)(void*) = NULL;
static int (*hz12_real_posix_memalign)(void**, size_t, size_t) = NULL;
static void* (*hz12_real_aligned_alloc)(size_t, size_t) = NULL;
static void* (*hz12_real_memalign)(size_t, size_t) = NULL;

static volatile int hz12_resolved = 0;

#define HZ12_BOOTSTRAP_BYTES 65536u
static char hz12_bootstrap_buf[HZ12_BOOTSTRAP_BYTES];
static volatile size_t hz12_bootstrap_off = 0;

static void* hz12_bootstrap_alloc(size_t n) {
  n = (n + 15u) & ~(size_t)15u;
  size_t off = __atomic_fetch_add(&hz12_bootstrap_off, n, __ATOMIC_RELAXED);
  if (off + n > (size_t)HZ12_BOOTSTRAP_BYTES) {
    return NULL;
  }
  return hz12_bootstrap_buf + off;
}

static int hz12_is_bootstrap(const void* p) {
  return (const char*)p >= hz12_bootstrap_buf &&
         (const char*)p < hz12_bootstrap_buf + HZ12_BOOTSTRAP_BYTES;
}

void hz12_resolver_ensure(void) {
  if (hz12_resolved) {
    return;
  }
  if (hz12_resolving) {
    return;
  }
  hz12_resolving = 1;
  hz12_real_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
  hz12_real_free = (void (*)(void*))dlsym(RTLD_NEXT, "free");
  hz12_real_realloc = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
  hz12_real_calloc = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  hz12_real_usable_size = (size_t (*)(void*))dlsym(RTLD_NEXT, "malloc_usable_size");
  hz12_real_posix_memalign =
      (int (*)(void**, size_t, size_t))dlsym(RTLD_NEXT, "posix_memalign");
  hz12_real_aligned_alloc =
      (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "aligned_alloc");
  hz12_real_memalign = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "memalign");
  hz12_resolving = 0;
  hz12_resolved = 1;
}

void* hz12_sys_malloc(size_t n) {
  hz12_resolver_ensure();
  if (hz12_resolving) {
    return hz12_bootstrap_alloc(n);
  }
  if (hz12_real_malloc) {
    return hz12_real_malloc(n);
  }
  return hz12_bootstrap_alloc(n);
}

void hz12_sys_free(void* p) {
  if (!p) {
    return;
  }
  if (hz12_is_bootstrap(p)) {
    return;
  }
  hz12_resolver_ensure();
  if (hz12_real_free) {
    hz12_real_free(p);
  }
}

void* hz12_sys_realloc(void* p, size_t n) {
  if (hz12_is_bootstrap(p)) {
    void* np = hz12_sys_malloc(n);
    if (np && p) {
      memcpy(np, p, n);
    }
    return np;
  }
  hz12_resolver_ensure();
  if (hz12_real_realloc) {
    return hz12_real_realloc(p, n);
  }
  return NULL;
}

void* hz12_sys_calloc(size_t count, size_t size) {
  hz12_resolver_ensure();
  if (hz12_real_calloc) {
    return hz12_real_calloc(count, size);
  }
  size_t total = count * size;
  if (count != 0u && size > SIZE_MAX / count) {
    return NULL;
  }
  void* p = hz12_sys_malloc(total);
  if (p) {
    memset(p, 0, total);
  }
  return p;
}

size_t hz12_sys_usable_size(void* p) {
  hz12_resolver_ensure();
  if (hz12_real_usable_size) {
    return hz12_real_usable_size(p);
  }
  return 0;
}

int hz12_sys_posix_memalign(void** memptr, size_t alignment, size_t size) {
  hz12_resolver_ensure();
  if (hz12_real_posix_memalign) {
    return hz12_real_posix_memalign(memptr, alignment, size);
  }
  return ENOMEM;
}

void* hz12_sys_aligned_alloc(size_t alignment, size_t size) {
  hz12_resolver_ensure();
  if (hz12_real_aligned_alloc) {
    return hz12_real_aligned_alloc(alignment, size);
  }
  return NULL;
}

void* hz12_sys_memalign(size_t alignment, size_t size) {
  hz12_resolver_ensure();
  if (hz12_real_memalign) {
    return hz12_real_memalign(alignment, size);
  }
  return NULL;
}
#endif
