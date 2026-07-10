#ifndef HZ12_SYS_ALLOC_H
#define HZ12_SYS_ALLOC_H

#include <stddef.h>
#include "hz12_port.h"

extern HZ12_THREAD_LOCAL int hz12_resolving;

void hz12_resolver_ensure(void);
void* hz12_sys_malloc(size_t n);
void hz12_sys_free(void* p);
void* hz12_sys_realloc(void* p, size_t n);
void* hz12_sys_calloc(size_t count, size_t size);
size_t hz12_sys_usable_size(void* p);
int hz12_sys_posix_memalign(void** memptr, size_t alignment, size_t size);
void* hz12_sys_aligned_alloc(size_t alignment, size_t size);
void* hz12_sys_memalign(size_t alignment, size_t size);

#endif /* HZ12_SYS_ALLOC_H */
