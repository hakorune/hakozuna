#ifndef HZ5_H
#define HZ5_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#define HZ5_API __declspec(dllexport)
#else
#define HZ5_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Public HZ5 ABI only. Internal P1/P2 entrypoints live in hz5_internal.h. */

typedef enum Hz5FreeResult {
  HZ5_FREE_OK_HZ5 = 1,
  HZ5_FREE_OK_FALLBACK = 2,
  HZ5_FREE_INVALID = 3
} Hz5FreeResult;

HZ5_API void* hz5_malloc(size_t size);
HZ5_API void* hz5_aligned_alloc(size_t size, size_t align);
HZ5_API Hz5FreeResult hz5_free(void* ptr);
HZ5_API int hz5_owns(void* ptr);
HZ5_API const void* hz5_allocator_descriptor_v1(void);

#ifdef __cplusplus
}
#endif

#endif
