#ifndef HZ5_MIDPAGEFRONT_H
#define HZ5_MIDPAGEFRONT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz5MidPageFrontFreeResult {
  HZ5_MIDPAGEFRONT_FREE_NOT_OWNED = 0,
  HZ5_MIDPAGEFRONT_FREE_OK = 1,
  HZ5_MIDPAGEFRONT_FREE_INVALID = 2
} Hz5MidPageFrontFreeResult;

typedef enum Hz5MidPageFrontAllocResult {
  HZ5_MIDPAGEFRONT_ALLOC_UNSUPPORTED = 0,
  HZ5_MIDPAGEFRONT_ALLOC_OK = 1,
  HZ5_MIDPAGEFRONT_ALLOC_OOM = 2
} Hz5MidPageFrontAllocResult;

void* hz5_midpagefront_alloc(size_t size, size_t align);
Hz5MidPageFrontAllocResult hz5_midpagefront_try_alloc(size_t size,
                                                      size_t align,
                                                      void** ptr_out);
Hz5MidPageFrontFreeResult hz5_midpagefront_free(void* ptr);
int hz5_midpagefront_can_handle(size_t size, size_t align);
int hz5_midpagefront_owns(void* ptr);
size_t hz5_midpagefront_usable_size(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
