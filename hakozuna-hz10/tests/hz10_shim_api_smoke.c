#include "hz10_pagemap.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef void* (*hz10_malloc_fn)(size_t);
typedef void (*hz10_free_fn)(void*);
typedef void* (*hz10_calloc_fn)(size_t, size_t);
typedef void* (*hz10_realloc_fn)(void*, size_t);
typedef size_t (*hz10_malloc_usable_size_fn)(void*);
typedef int (*hz10_posix_memalign_fn)(void**, size_t, size_t);
typedef void* (*hz10_aligned_alloc_fn)(size_t, size_t);
typedef void* (*hz10_memalign_fn)(size_t, size_t);
typedef uint64_t (*hz10_foreign_free_count_fn)(void);

typedef struct Hz10ShimApi {
  hz10_malloc_fn malloc_fn;
  hz10_free_fn free_fn;
  hz10_calloc_fn calloc_fn;
  hz10_realloc_fn realloc_fn;
  hz10_malloc_usable_size_fn usable_size_fn;
  hz10_posix_memalign_fn posix_memalign_fn;
  hz10_aligned_alloc_fn aligned_alloc_fn;
  hz10_memalign_fn memalign_fn;
  hz10_foreign_free_count_fn foreign_free_count_fn;
} Hz10ShimApi;

static void* load_symbol(void* handle, const char* name) {
  dlerror();
  void* sym = dlsym(handle, name);
  const char* err = dlerror();
  if (err || !sym) {
    fprintf(stderr, "shim_api: dlsym(%s) failed: %s\n", name,
            err ? err : "NULL");
    exit(2);
  }
  return sym;
}

static Hz10ShimApi load_api(void) {
  void* handle = dlopen("./libhz10.so", RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    fprintf(stderr, "shim_api: dlopen failed: %s\n", dlerror());
    exit(2);
  }
  Hz10ShimApi api;
  api.malloc_fn = (hz10_malloc_fn)load_symbol(handle, "malloc");
  api.free_fn = (hz10_free_fn)load_symbol(handle, "free");
  api.calloc_fn = (hz10_calloc_fn)load_symbol(handle, "calloc");
  api.realloc_fn = (hz10_realloc_fn)load_symbol(handle, "realloc");
  api.usable_size_fn =
      (hz10_malloc_usable_size_fn)load_symbol(handle, "malloc_usable_size");
  api.posix_memalign_fn =
      (hz10_posix_memalign_fn)load_symbol(handle, "posix_memalign");
  api.aligned_alloc_fn =
      (hz10_aligned_alloc_fn)load_symbol(handle, "aligned_alloc");
  api.memalign_fn = (hz10_memalign_fn)load_symbol(handle, "memalign");
  api.foreign_free_count_fn = (hz10_foreign_free_count_fn)load_symbol(
      handle, "hz10_shim_tolerated_foreign_free_count");
  return api;
}

static int check_malloc_zero_and_usable(const Hz10ShimApi* api) {
  void* ptr = api->malloc_fn(0u);
  if (!ptr) {
    fprintf(stderr, "shim_api: malloc(0) returned NULL\n");
    return 1;
  }
  size_t usable = api->usable_size_fn(ptr);
  if (usable < HZ10_MIN_ALIGN) {
    fprintf(stderr, "shim_api: malloc(0) usable too small: %zu\n", usable);
    return 1;
  }
  api->free_fn(ptr);
  return 0;
}

static int check_calloc(const Hz10ShimApi* api) {
  unsigned char* ptr = (unsigned char*)api->calloc_fn(4u, 17u);
  if (!ptr) {
    fprintf(stderr, "shim_api: calloc failed\n");
    return 1;
  }
  for (size_t i = 0; i < 68u; ++i) {
    if (ptr[i] != 0u) {
      fprintf(stderr, "shim_api: calloc returned dirty byte\n");
      return 1;
    }
  }
  api->free_fn(ptr);

  errno = 0;
  if (api->calloc_fn((size_t)-1, 2u) != NULL || errno != ENOMEM) {
    fprintf(stderr, "shim_api: calloc overflow not rejected with ENOMEM\n");
    return 1;
  }
  return 0;
}

static int check_realloc(const Hz10ShimApi* api) {
  unsigned char* ptr = (unsigned char*)api->malloc_fn(24u);
  if (!ptr) {
    fprintf(stderr, "shim_api: malloc for realloc failed\n");
    return 1;
  }
  size_t usable = api->usable_size_fn(ptr);
  for (size_t i = 0; i < usable; ++i) {
    ptr[i] = (unsigned char)(i + 1u);
  }

  void* same = api->realloc_fn(ptr, usable);
  if (same != ptr) {
    fprintf(stderr, "shim_api: realloc within usable size moved pointer\n");
    return 1;
  }

  unsigned char* grown = (unsigned char*)api->realloc_fn(ptr, usable + 1u);
  if (!grown || grown == ptr) {
    fprintf(stderr, "shim_api: realloc grow failed or stayed in place\n");
    return 1;
  }
  for (size_t i = 0; i < usable; ++i) {
    if (grown[i] != (unsigned char)(i + 1u)) {
      fprintf(stderr, "shim_api: realloc grow did not preserve bytes\n");
      return 1;
    }
  }
  api->free_fn(grown);

  void* zero = api->realloc_fn(NULL, 0u);
  if (!zero) {
    fprintf(stderr, "shim_api: realloc(NULL,0) did not use malloc(0) rule\n");
    return 1;
  }
  if (api->realloc_fn(zero, 0u) != NULL) {
    fprintf(stderr, "shim_api: realloc(ptr,0) did not return NULL\n");
    return 1;
  }
  return 0;
}

static int check_alignment(const Hz10ShimApi* api) {
  void* ptr = NULL;
  int rc = api->posix_memalign_fn(&ptr, 4096u, 100u);
  if (rc != 0 || !ptr || ((uintptr_t)ptr & 4095u) != 0u) {
    fprintf(stderr, "shim_api: posix_memalign(4096) failed rc=%d ptr=%p\n",
            rc, ptr);
    return 1;
  }
  api->free_fn(ptr);

  ptr = api->aligned_alloc_fn(4096u, 4096u);
  if (!ptr || ((uintptr_t)ptr & 4095u) != 0u) {
    fprintf(stderr, "shim_api: aligned_alloc(4096) failed ptr=%p\n", ptr);
    return 1;
  }
  api->free_fn(ptr);

  ptr = api->memalign_fn(64u, 100u);
  if (!ptr || ((uintptr_t)ptr & 63u) != 0u) {
    fprintf(stderr, "shim_api: memalign(64) failed ptr=%p\n", ptr);
    return 1;
  }
  api->free_fn(ptr);

  ptr = (void*)UINTPTR_MAX;
  if (api->posix_memalign_fn(&ptr, 3u, 100u) != EINVAL) {
    fprintf(stderr, "shim_api: invalid posix_memalign alignment accepted\n");
    return 1;
  }
  errno = 0;
  if (api->aligned_alloc_fn(4096u, 100u) != NULL || errno != EINVAL) {
    fprintf(stderr, "shim_api: invalid aligned_alloc size accepted\n");
    return 1;
  }
  errno = 0;
  if (api->memalign_fn((size_t)HZ10_PAGE_QUANTUM * 2u, 100u) != NULL ||
      errno != ENOMEM) {
    fprintf(stderr, "shim_api: over-quantum memalign not rejected\n");
    return 1;
  }
  return 0;
}

static int child_unknown_free(int tolerate) {
  Hz10ShimApi api = load_api();
  if (tolerate) {
    setenv("HZ10_SHIM_TOLERATE_FOREIGN", "1", 1);
  } else {
    unsetenv("HZ10_SHIM_TOLERATE_FOREIGN");
  }
  int local = 0;
  api.free_fn(&local);
  return tolerate ? 0 : 1;
}

static int check_unknown_free_policy(void) {
  pid_t pid = fork();
  if (pid == 0) {
    _exit(child_unknown_free(0));
  }
  int status = 0;
  if (pid < 0 || waitpid(pid, &status, 0) < 0 ||
      !WIFSIGNALED(status) || WTERMSIG(status) != SIGABRT) {
    fprintf(stderr, "shim_api: unknown free did not abort\n");
    return 1;
  }

  pid = fork();
  if (pid == 0) {
    _exit(child_unknown_free(1));
  }
  status = 0;
  if (pid < 0 || waitpid(pid, &status, 0) < 0 ||
      !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "shim_api: tolerated unknown free did not return\n");
    return 1;
  }

  Hz10ShimApi api = load_api();
  setenv("HZ10_SHIM_TOLERATE_FOREIGN", "1", 1);
  uint64_t before = api.foreign_free_count_fn();
  int local = 0;
  api.free_fn(&local);
  uint64_t after = api.foreign_free_count_fn();
  unsetenv("HZ10_SHIM_TOLERATE_FOREIGN");
  if (after != before + 1u) {
    fprintf(stderr, "shim_api: tolerated foreign free was not counted\n");
    return 1;
  }
  return 0;
}

int main(void) {
  Hz10ShimApi api = load_api();
  if (check_malloc_zero_and_usable(&api) || check_calloc(&api) ||
      check_realloc(&api) || check_alignment(&api) ||
      check_unknown_free_policy()) {
    return 1;
  }
  printf("hz10_shim_api_smoke ok\n");
  return 0;
}
