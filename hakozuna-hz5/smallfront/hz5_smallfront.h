#ifndef HZ5_SMALLFRONT_H
#define HZ5_SMALLFRONT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz5SmallFrontFreeResult {
  HZ5_SMALLFRONT_FREE_NOT_OWNED = 0,
  HZ5_SMALLFRONT_FREE_OK = 1,
  HZ5_SMALLFRONT_FREE_INVALID = 2
} Hz5SmallFrontFreeResult;

void* hz5_smallfront_alloc(size_t size, size_t align);
Hz5SmallFrontFreeResult hz5_smallfront_free(void* ptr);
int hz5_smallfront_owns(void* ptr);
size_t hz5_smallfront_usable_size(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
