#ifndef HZ11_SYS_ALLOC_H
#define HZ11_SYS_ALLOC_H

#include <stddef.h>

extern _Thread_local int hz11_resolving;

void hz11_resolver_ensure(void);
void* hz11_sys_malloc(size_t n);
void hz11_sys_free(void* p);
void* hz11_sys_realloc(void* p, size_t n);
void* hz11_sys_calloc(size_t count, size_t size);
size_t hz11_sys_usable_size(void* p);
int hz11_sys_posix_memalign(void** memptr, size_t alignment, size_t size);
void* hz11_sys_aligned_alloc(size_t alignment, size_t size);
void* hz11_sys_memalign(size_t alignment, size_t size);

#endif /* HZ11_SYS_ALLOC_H */
