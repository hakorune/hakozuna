#ifndef HZ5_LARGEFRONT_H
#define HZ5_LARGEFRONT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz5LargeFrontFreeResult {
  HZ5_LARGEFRONT_FREE_NOT_OWNED = 0,
  HZ5_LARGEFRONT_FREE_OK = 1,
  HZ5_LARGEFRONT_FREE_INVALID = 2
} Hz5LargeFrontFreeResult;

void* hz5_largefront_alloc(size_t size, size_t align);
Hz5LargeFrontFreeResult hz5_largefront_free(void* ptr);
int hz5_largefront_can_handle(size_t size, size_t align);
int hz5_largefront_owns(void* ptr);
size_t hz5_largefront_usable_size(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
