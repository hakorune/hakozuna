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

void* hz5_p1_alloc_aligned(size_t size, size_t alignment);
void hz5_p1_free(void* ptr);
int hz5_p1_owns(void* ptr);
int hz5_p1_validate(void* ptr, size_t size, size_t alignment);

void* hz5_p2_alloc_aligned(size_t size, size_t alignment);
void hz5_p2_free(void* ptr);
size_t hz5_p2_flush_remote(void);
size_t hz5_p2_drain_current_owner(void);
void hz5_stats_print_once(void);

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
