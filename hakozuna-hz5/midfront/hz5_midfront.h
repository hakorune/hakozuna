#ifndef HZ5_MIDFRONT_H
#define HZ5_MIDFRONT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz5MidFrontFreeResult {
  HZ5_MIDFRONT_FREE_NOT_OWNED = 0,
  HZ5_MIDFRONT_FREE_OK = 1,
  HZ5_MIDFRONT_FREE_INVALID = 2
} Hz5MidFrontFreeResult;

void* hz5_midfront_alloc(size_t size, size_t align);
Hz5MidFrontFreeResult hz5_midfront_free(void* ptr);
int hz5_midfront_can_handle(size_t size, size_t align);
int hz5_midfront_owns(void* ptr);
size_t hz5_midfront_usable_size(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
