#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* hz3_hook_malloc(size_t size);
void  hz3_hook_free(void* ptr);
void* hz3_hook_calloc(size_t n, size_t size);
void* hz3_hook_realloc(void* ptr, size_t size);
int   hz3_hook_posix_memalign(void** memptr, size_t alignment, size_t size);
void* hz3_hook_aligned_alloc(size_t alignment, size_t size);
size_t hz3_hook_malloc_usable_size(void* ptr);

#ifdef __cplusplus
}
#endif
