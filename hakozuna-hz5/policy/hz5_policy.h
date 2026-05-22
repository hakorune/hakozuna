#ifndef HZ5_POLICY_H
#define HZ5_POLICY_H

#include "hz5.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz5PolicyHooks {
  void* (*p17_wrapper_alloc)(size_t size, size_t align);
  void (*p17_raw_release)(void* raw);
} Hz5PolicyHooks;

void* hz5_policy_alloc_aligned(size_t size, size_t align,
                               const Hz5PolicyHooks* hooks);
Hz5FreeResult hz5_policy_free(void* ptr, const Hz5PolicyHooks* hooks);

#ifdef __cplusplus
}
#endif

#endif
