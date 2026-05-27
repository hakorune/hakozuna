#include "hz6_source.h"

#include <stdlib.h>

void* hz6_source_system_alloc(size_t bytes) {
  return bytes == 0 ? NULL : malloc(bytes);
}

void hz6_source_system_free(void* ptr) {
  free(ptr);
}

int hz6_source_system_release(void* ptr, size_t bytes) {
  (void)bytes;
  free(ptr);
  return 1;
}
