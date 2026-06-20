#include "h8_internal.h"

#include <stdlib.h>

#ifdef H8_BUILD_LD_PRELOAD
#include <dlfcn.h>

static void* (*h8_real_malloc_fn)(size_t) = NULL;
static void* (*h8_real_calloc_fn)(size_t, size_t) = NULL;
static void (*h8_real_free_fn)(void*) = NULL;

static void h8_load_real_allocators(void) {
  if (h8_real_malloc_fn) {
    return;
  }
  h8_real_malloc_fn = dlsym(RTLD_NEXT, "malloc");
  h8_real_calloc_fn = dlsym(RTLD_NEXT, "calloc");
  h8_real_free_fn = dlsym(RTLD_NEXT, "free");
}
#endif

void h8_system_init(void) {
#ifdef H8_BUILD_LD_PRELOAD
  h8_load_real_allocators();
#endif
}

void* h8_sys_malloc(size_t size) {
#ifdef H8_BUILD_LD_PRELOAD
  h8_load_real_allocators();
  return h8_real_malloc_fn ? h8_real_malloc_fn(size) : NULL;
#else
  return malloc(size);
#endif
}

void* h8_sys_calloc(size_t count, size_t size) {
#ifdef H8_BUILD_LD_PRELOAD
  h8_load_real_allocators();
  return h8_real_calloc_fn ? h8_real_calloc_fn(count, size) : NULL;
#else
  return calloc(count, size);
#endif
}

void h8_sys_free(void* ptr) {
#ifdef H8_BUILD_LD_PRELOAD
  h8_load_real_allocators();
  if (h8_real_free_fn) {
    h8_real_free_fn(ptr);
  }
#else
  free(ptr);
#endif
}
