#ifndef HZ6_PRELOAD_REAL_H
#define HZ6_PRELOAD_REAL_H

#include <stddef.h>

extern __thread int g_hz6_preload_reentry;

void* hz6_preload_real_malloc(size_t size);
void hz6_preload_real_free(void* ptr);
void* hz6_preload_real_calloc(size_t nmemb, size_t size);
void* hz6_preload_real_realloc(void* ptr, size_t size);
int hz6_preload_real_posix_memalign(void** memptr,
                                    size_t alignment,
                                    size_t size);
void* hz6_preload_real_aligned_alloc(size_t alignment, size_t size);
size_t hz6_preload_real_malloc_usable_size(void* ptr);

#endif
