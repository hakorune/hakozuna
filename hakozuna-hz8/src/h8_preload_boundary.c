#include "h8_internal.h"

#include <string.h>
#include <stdlib.h>

#ifdef H8_BUILD_LD_PRELOAD
__attribute__((visibility("default"))) void* malloc(size_t size) {
  return h8_malloc_inner(size);
}

__attribute__((visibility("default"))) void* calloc(size_t count, size_t size) {
  return h8_calloc(count, size);
}

__attribute__((visibility("default"))) void* realloc(void* ptr, size_t size) {
  return h8_realloc_inner(ptr, size);
}

__attribute__((visibility("default"))) void free(void* ptr) {
  h8_free_inner(ptr);
}
#endif

void* h8_malloc(size_t size) {
  return h8_malloc_inner(size);
}

void* h8_calloc(size_t count, size_t size) {
  if (count != 0 && size > SIZE_MAX / count) {
    return NULL;
  }
  size_t total = count * size;
  void* ptr = h8_malloc_inner(total);
  if (ptr) {
    memset(ptr, 0, total);
  }
  return ptr;
}

void* h8_realloc(void* ptr, size_t size) {
  return h8_realloc_inner(ptr, size);
}

void h8_free(void* ptr) {
  h8_free_inner(ptr);
}
